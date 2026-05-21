// ============================================================
// InlineCalculator - Erweiterbarer Rechner
//
// Unterstuetzt Grundrechnungsarten (+, -, *, /, ^) und
// mathematische Funktionen: Wurzel, Quadrat, Sinus, Arkussinus,
// numerisches Integral (Simpsonsche Regel),
// numerische Ableitung (Zentraldifferenz).
//
// Eingabe und Ausgabe ueber die Windows-Eingabeaufforderung (CMD).
//
// ERWEITERBARKEIT:
//   Neue einfache Funktionen (1 Argument) koennen in initFunctions()
//   mittels g_registry.registerFunction("name", lambda) hinzugefuegt
//   werden, ohne den Parser zu veraendern.
//   Komplexere Funktionen (mehrere Argumente oder Kalkulus) werden
//   direkt in ExpressionEvaluator::parseFunction() ergaenzt.
// ============================================================

#define _USE_MATH_DEFINES   // M_PI / M_E fuer MSVC
#include <iostream>
#include <string>
#include <sstream>
#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <iomanip>
#include <functional>
#include <map>
#include <vector>
#include <cctype>
#include <limits>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_E
#define M_E  2.71828182845904523536
#endif

// ============================================================
// FunctionRegistry
//
// Verwaltet einfache Funktionen der Form f(x) -> double.
// Neue Funktionen werden per registerFunction() eingetragen und
// stehen sofort im Ausdruck-Parser zur Verfuegung.
// ============================================================
class FunctionRegistry {
public:
    using Func = std::function<double(double)>;

    // Funktion registrieren (ueberschreibt vorhandene gleichnamige Funktion)
    void registerFunction(const std::string& name, Func func) {
        functions_[name] = std::move(func);
    }

    bool hasFunction(const std::string& name) const {
        return functions_.count(name) > 0;
    }

    double call(const std::string& name, double arg) const {
        auto it = functions_.find(name);
        if (it == functions_.end()) {
            throw std::runtime_error("Unbekannte Funktion: " + name);
        }
        return it->second(arg);
    }

    const std::map<std::string, Func>& all() const { return functions_; }

private:
    std::map<std::string, Func> functions_;
};

// Globale Registrierung (wird in initFunctions() befuellt)
static FunctionRegistry g_registry;

// Alle Standardfunktionen registrieren
static void initFunctions() {
    // ---- Trigonometrie (Bogenmass) ----
    g_registry.registerFunction("sin",  [](double x) { return std::sin(x); });
    g_registry.registerFunction("cos",  [](double x) { return std::cos(x); });
    g_registry.registerFunction("tan",  [](double x) { return std::tan(x); });

    // ---- Inverse Trigonometrie ----
    auto asinFunc = [](double x) -> double {
        if (x < -1.0 || x > 1.0)
            throw std::runtime_error("asin: Argument muss im Bereich [-1, 1] liegen");
        return std::asin(x);
    };
    g_registry.registerFunction("asin",        asinFunc);
    g_registry.registerFunction("arcsin",       asinFunc);
    g_registry.registerFunction("arcussinus",   asinFunc);

    auto acosFunc = [](double x) -> double {
        if (x < -1.0 || x > 1.0)
            throw std::runtime_error("acos: Argument muss im Bereich [-1, 1] liegen");
        return std::acos(x);
    };
    g_registry.registerFunction("acos",  acosFunc);
    g_registry.registerFunction("arccos", acosFunc);

    g_registry.registerFunction("atan",  [](double x) { return std::atan(x); });
    g_registry.registerFunction("arctan",[](double x) { return std::atan(x); });

    // ---- Wurzel / Quadrat ----
    auto sqrtFunc = [](double x) -> double {
        if (x < 0.0)
            throw std::runtime_error("sqrt: Argument muss nicht-negativ sein");
        return std::sqrt(x);
    };
    g_registry.registerFunction("sqrt",   sqrtFunc);
    g_registry.registerFunction("wurzel", sqrtFunc);   // Deutsch

    auto squareFunc = [](double x) { return x * x; };
    g_registry.registerFunction("square",  squareFunc);
    g_registry.registerFunction("sqr",     squareFunc);
    g_registry.registerFunction("quadrat", squareFunc); // Deutsch

    g_registry.registerFunction("cbrt", [](double x) { return std::cbrt(x); });

    // ---- Logarithmen / Exponential ----
    g_registry.registerFunction("ln", [](double x) -> double {
        if (x <= 0.0)
            throw std::runtime_error("ln: Argument muss positiv sein");
        return std::log(x);
    });
    g_registry.registerFunction("log", [](double x) -> double {
        if (x <= 0.0)
            throw std::runtime_error("log: Argument muss positiv sein");
        return std::log10(x);
    });
    g_registry.registerFunction("exp", [](double x) { return std::exp(x); });

    // ---- Sonstiges ----
    g_registry.registerFunction("abs",   [](double x) { return std::fabs(x); });
    g_registry.registerFunction("ceil",  [](double x) { return std::ceil(x); });
    g_registry.registerFunction("floor", [](double x) { return std::floor(x); });
    g_registry.registerFunction("round", [](double x) { return std::round(x); });
    g_registry.registerFunction("sign",  [](double x) -> double {
        return (x > 0.0) ? 1.0 : (x < 0.0) ? -1.0 : 0.0;
    });
    // Winkel-Umrechnung
    g_registry.registerFunction("deg", [](double x) { return x * 180.0 / M_PI; });
    g_registry.registerFunction("rad", [](double x) { return x * M_PI / 180.0; });
}

// ============================================================
// ExpressionEvaluator
//
// Rekursiver Abstieg-Parser fuer mathematische Ausdruecke.
//
// Grammatik:
//   expr    = term   (('+' | '-') term)*
//   term    = power  (('*' | '/') power)*
//   power   = unary  ('^' power)?          // rechts-assoziativ
//   unary   = ('+' | '-') unary | primary
//   primary = number | 'x' | 'pi' | 'e'
//           | '(' expr ')'
//           | identifier '(' args ')'
//
// Variable 'x' wird fuer integrate/diff verwendet.
// ============================================================
class ExpressionEvaluator {
public:
    ExpressionEvaluator() : pos_(0), xVal_(0.0) {}

    // Ausdruck auswerten; x wird als Wert der Variable 'x' gesetzt
    double evaluate(const std::string& expr, double x = 0.0) {
        // Leerzeichen entfernen (vereinfacht den Parser)
        expr_.clear();
        for (char c : expr) {
            if (!std::isspace(static_cast<unsigned char>(c))) {
                expr_ += c;
            }
        }
        pos_  = 0;
        xVal_ = x;

        if (expr_.empty()) {
            throw std::runtime_error("Leerer Ausdruck");
        }

        double result = parseExpression();

        if (pos_ != expr_.size()) {
            throw std::runtime_error(
                std::string("Unerwartetes Zeichen '") + expr_[pos_] + "' an Position " +
                std::to_string(pos_));
        }
        return result;
    }

private:
    std::string expr_;
    size_t      pos_;
    double      xVal_;

    // ---- Hilfsmethoden ----

    char peek() const {
        return (pos_ < expr_.size()) ? expr_[pos_] : '\0';
    }

    char advance() {
        return expr_[pos_++];
    }

    void expect(char c) {
        if (peek() != c) {
            throw std::runtime_error(
                std::string("'") + c + "' erwartet, '" + peek() + "' gefunden");
        }
        advance();
    }

    // ---- Grammatik-Regeln ----

    // expr = term (('+' | '-') term)*
    double parseExpression() {
        double lhs = parseTerm();
        while (peek() == '+' || peek() == '-') {
            char op = advance();
            double rhs = parseTerm();
            lhs = (op == '+') ? lhs + rhs : lhs - rhs;
        }
        return lhs;
    }

    // term = power (('*' | '/') power)*
    double parseTerm() {
        double lhs = parsePower();
        while (peek() == '*' || peek() == '/') {
            char op = advance();
            double rhs = parsePower();
            if (op == '*') {
                lhs *= rhs;
            } else {
                if (rhs == 0.0)
                    throw std::runtime_error("Division durch Null");
                lhs /= rhs;
            }
        }
        return lhs;
    }

    // power = unary ('^' power)?  (rechts-assoziativ)
    double parsePower() {
        double base = parseUnary();
        if (peek() == '^') {
            advance();
            double exponent = parsePower(); // rekursiv fuer Rechts-Assoziativitaet
            return std::pow(base, exponent);
        }
        return base;
    }

    // unary = ('+' | '-') unary | primary
    double parseUnary() {
        if (peek() == '-') { advance(); return -parseUnary(); }
        if (peek() == '+') { advance(); return  parseUnary(); }
        return parsePrimary();
    }

    // primary = number | 'x' | 'pi' | 'e' | '(' expr ')' | func '(' ... ')'
    double parsePrimary() {
        char c = peek();

        if (std::isdigit(static_cast<unsigned char>(c)) || c == '.') {
            return parseNumber();
        }

        if (c == '(') {
            advance();
            double val = parseExpression();
            expect(')');
            return val;
        }

        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            std::string name = parseIdentifier();
            if (peek() == '(') {
                advance(); // '(' konsumieren
                return parseFunction(name);
            }
            // Konstanten und Variable
            if (name == "x")                         return xVal_;
            if (name == "pi")                        return M_PI;
            if (name == "e")                         return M_E;
            if (name == "inf" || name == "infinity") return std::numeric_limits<double>::infinity();
            throw std::runtime_error("Unbekannte Variable oder Konstante: " + name);
        }

        if (c == '\0') throw std::runtime_error("Unerwartetes Ende des Ausdrucks");
        throw std::runtime_error(std::string("Unbekanntes Zeichen: ") + c);
    }

    // Zahl (Ganzzahl, Dezimalzahl, wissenschaftliche Notation)
    double parseNumber() {
        size_t start = pos_;
        while (pos_ < expr_.size() && std::isdigit(static_cast<unsigned char>(expr_[pos_]))) ++pos_;
        if (pos_ < expr_.size() && expr_[pos_] == '.') {
            ++pos_;
            while (pos_ < expr_.size() && std::isdigit(static_cast<unsigned char>(expr_[pos_]))) ++pos_;
        }
        // Wissenschaftliche Notation: 1.5e-3
        if (pos_ < expr_.size() && (expr_[pos_] == 'e' || expr_[pos_] == 'E')) {
            ++pos_;
            if (pos_ < expr_.size() && (expr_[pos_] == '+' || expr_[pos_] == '-')) ++pos_;
            while (pos_ < expr_.size() && std::isdigit(static_cast<unsigned char>(expr_[pos_]))) ++pos_;
        }
        return std::stod(expr_.substr(start, pos_ - start));
    }

    // Bezeichner (lowercase normalisiert)
    std::string parseIdentifier() {
        size_t start = pos_;
        while (pos_ < expr_.size() &&
               (std::isalnum(static_cast<unsigned char>(expr_[pos_])) || expr_[pos_] == '_')) {
            ++pos_;
        }
        std::string id = expr_.substr(start, pos_ - start);
        std::transform(id.begin(), id.end(), id.begin(),
                       [](unsigned char ch) { return std::tolower(ch); });
        return id;
    }

    // Erfasst einen Teilausdruck als String bis zum naechsten ',' oder ')' auf Tiefe 0.
    // Benoetigt fuer integrate/diff, damit x spaeter mit verschiedenen Werten ausgewertet wird.
    std::string captureSubExpression() {
        size_t start = pos_;
        int depth = 0;
        while (pos_ < expr_.size()) {
            char ch = expr_[pos_];
            if (ch == '(') {
                ++depth;
            } else if (ch == ')') {
                if (depth == 0) break;
                --depth;
            } else if (ch == ',' && depth == 0) {
                break;
            }
            ++pos_;
        }
        return expr_.substr(start, pos_ - start);
    }

    // ---- Numerische Methoden ----

    // Numerische Integration: Simpsonsche Regel
    // integral(ausdruck, untere_grenze, obere_grenze [, schritte])
    double numericalIntegrate(const std::string& subExpr, double a, double b, int n) {
        if (n <= 0) throw std::runtime_error("Schrittanzahl muss positiv sein");
        if (n % 2 != 0) ++n; // Simpson benoetigt gerades n

        auto f = [&](double t) -> double {
            ExpressionEvaluator ev;
            return ev.evaluate(subExpr, t);
        };

        double h   = (b - a) / n;
        double sum = f(a) + f(b);
        for (int i = 1; i < n; ++i) {
            double t = a + i * h;
            sum += (i % 2 == 0 ? 2.0 : 4.0) * f(t);
        }
        return sum * h / 3.0;
    }

    // Numerische Differentiation: Zentraldifferenz
    // diff(ausdruck, punkt [, schrittweite])
    double numericalDiff(const std::string& subExpr, double point, double h) {
        auto f = [&](double t) -> double {
            ExpressionEvaluator ev;
            return ev.evaluate(subExpr, t);
        };
        return (f(point + h) - f(point - h)) / (2.0 * h);
    }

    // ---- Funktionsaufruf (nach dem '(' wurde bereits konsumiert) ----
    double parseFunction(const std::string& name) {

        // -- Kalkulus-Funktionen (erster Parameter ist ein Ausdruck mit x) --

        if (name == "integrate" || name == "integral") {
            // integrate(f(x), untere_grenze, obere_grenze [, schritte])
            std::string subExpr = captureSubExpression();
            expect(',');
            double a = parseExpression();
            expect(',');
            double b = parseExpression();
            int n = 1000;
            if (peek() == ',') { advance(); n = static_cast<int>(parseExpression()); }
            expect(')');
            return numericalIntegrate(subExpr, a, b, n);
        }

        if (name == "diff" || name == "differentiate" ||
            name == "derivative"  || name == "differential") {
            // diff(f(x), punkt [, schrittweite])
            std::string subExpr = captureSubExpression();
            expect(',');
            double point = parseExpression();
            double h = 1e-7;
            if (peek() == ',') { advance(); h = parseExpression(); }
            expect(')');
            return numericalDiff(subExpr, point, h);
        }

        // -- Zweistellige allgemeine Funktionen --

        if (name == "pow") {
            // pow(basis, exponent)
            double base = parseExpression();
            expect(',');
            double exp = parseExpression();
            expect(')');
            return std::pow(base, exp);
        }

        if (name == "atan2") {
            // atan2(y, x)
            double y = parseExpression();
            expect(',');
            double x = parseExpression();
            expect(')');
            return std::atan2(y, x);
        }

        // -- Einstellige Funktionen aus der Registry --
        double arg = parseExpression();
        expect(')');

        if (g_registry.hasFunction(name)) {
            return g_registry.call(name, arg);
        }

        throw std::runtime_error("Unbekannte Funktion: " + name);
    }
};

// ============================================================
// Hilfsfunktionen
// ============================================================

static std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

static std::string formatResult(double v) {
    if (std::isinf(v)) return v > 0.0 ? "Unendlich" : "-Unendlich";
    if (std::isnan(v)) return "Unbestimmt (NaN)";

    // Ganze Zahl?
    double rounded = std::round(v);
    bool isInt = std::fabs(v - rounded) < 1e-9 * (1.0 + std::fabs(v)) &&
                 std::fabs(v) < 1e15;
    if (isInt) {
        std::ostringstream oss;
        oss << static_cast<long long>(rounded);
        return oss.str();
    }

    std::ostringstream oss;
    oss << std::setprecision(10) << v;
    return oss.str();
}

// ============================================================
// Calculator - REPL-Schleife
// ============================================================
class Calculator {
public:
    void run() {
        printWelcome();
        ExpressionEvaluator eval;
        std::string line;

        while (true) {
            std::cout << "\n> ";
            std::cout.flush();

            if (!std::getline(std::cin, line)) break; // EOF (z.B. Pipe-Ende)

            line = trim(line);
            if (line.empty()) continue;

            // Befehlsworte (case-insensitiv pruefen)
            std::string lower = line;
            std::transform(lower.begin(), lower.end(), lower.begin(),
                           [](unsigned char c) { return std::tolower(c); });

            if (lower == "beenden" || lower == "quit" || lower == "exit" ||
                lower == "q"       || lower == "bye") {
                std::cout << "\nAuf Wiedersehen!\n";
                break;
            }
            if (lower == "hilfe" || lower == "help" || lower == "h" || lower == "?") {
                printHelp();
                continue;
            }

            // Ausdruck auswerten
            try {
                double result = eval.evaluate(line);
                std::cout << "= " << formatResult(result) << "\n";
            } catch (const std::exception& ex) {
                std::cout << "Fehler: " << ex.what() << "\n";
                std::cout << "  (Tippen Sie 'hilfe' fuer eine Uebersicht der Befehle)\n";
            }
        }
    }

private:
    static void printWelcome() {
        std::cout <<
            "\n"
            "+----------------------------------------------------------+\n"
            "|          InlineCalculator  v1.0                          |\n"
            "|  Erweiterbarer Rechner - Windows Kommandoprozessor       |\n"
            "+----------------------------------------------------------+\n"
            "\n"
            "  Tippen Sie einen Ausdruck und druecken Sie ENTER.\n"
            "  'hilfe' zeigt alle Funktionen, 'beenden' beendet das Programm.\n";
    }

    static void printHelp() {
        std::cout <<
            "\n"
            "=========================================================\n"
            "  GRUNDRECHNUNGSARTEN\n"
            "=========================================================\n"
            "  3 + 4          Addition\n"
            "  10 - 3         Subtraktion\n"
            "  5 * 6          Multiplikation\n"
            "  10 / 4         Division\n"
            "  2 ^ 8          Potenz  (2 hoch 8)\n"
            "  (3 + 4) * 2    Klammerrechnung\n"
            "\n"
            "=========================================================\n"
            "  MATHEMATISCHE FUNKTIONEN\n"
            "=========================================================\n"
            "  sqrt(16)           Quadratwurzel  (Ergebnis: 4)\n"
            "  wurzel(16)         Quadratwurzel  (Deutsch)\n"
            "  square(5)          Quadrat  5^2   (Ergebnis: 25)\n"
            "  quadrat(5)         Quadrat  5^2   (Deutsch)\n"
            "  pow(2, 10)         Potenz   2^10  (Ergebnis: 1024)\n"
            "  cbrt(27)           Kubikwurzel    (Ergebnis: 3)\n"
            "\n"
            "  sin(pi/2)          Sinus          (Bogenmass)\n"
            "  cos(0)             Kosinus\n"
            "  tan(pi/4)          Tangens\n"
            "  asin(1)            Arkussinus     (Ergebnis: pi/2)\n"
            "  arcsin(0.5)        Arkussinus\n"
            "  arcussinus(1)      Arkussinus     (Deutsch)\n"
            "  acos(0)            Arkuskosinus\n"
            "  atan(1)            Arkustangens\n"
            "  atan2(1, 1)        Arkustangens 2 (y, x)\n"
            "\n"
            "  ln(e)              Natuerlicher Logarithmus\n"
            "  log(100)           Dekadischer Logarithmus (Basis 10)\n"
            "  exp(1)             Exponentialfunktion e^x\n"
            "  abs(-5)            Absolutwert\n"
            "  ceil(1.2)          Aufrunden\n"
            "  floor(1.9)        Abrunden\n"
            "  round(1.5)         Runden\n"
            "  sign(-3)           Vorzeichen (-1 / 0 / +1)\n"
            "  deg(pi)            Bogenmass -> Grad\n"
            "  rad(180)           Grad -> Bogenmass\n"
            "\n"
            "=========================================================\n"
            "  KALKULUS  (x = Variable im Ausdruck)\n"
            "=========================================================\n"
            "  integral(x^2, 0, 1)          Integral von x^2 von 0 bis 1\n"
            "  integral(sin(x), 0, pi)      Integral von sin(x)\n"
            "  integral(x^2, 0, 1, 500)     Mit 500 Schritten (Standard: 1000)\n"
            "\n"
            "  diff(x^2, 3)                 Ableitung von x^2 bei x=3\n"
            "  diff(sin(x), 0)              Ableitung von sin(x) bei x=0\n"
            "  differential(x^3, 2)         Ableitung (Alias)\n"
            "  diff(x^2, 3, 1e-5)           Mit benutzerdefinierter Schrittweite\n"
            "\n"
            "=========================================================\n"
            "  KONSTANTEN\n"
            "=========================================================\n"
            "  pi    = 3.14159265...\n"
            "  e     = 2.71828182...\n"
            "\n"
            "=========================================================\n"
            "  BEFEHLE\n"
            "=========================================================\n"
            "  hilfe / help      Diese Hilfe anzeigen\n"
            "  beenden / quit    Programm beenden\n"
            "=========================================================\n";
    }
};

// ============================================================
// main
// ============================================================
int main() {
#ifdef _WIN32
    // UTF-8-Ausgabe in der Windows-Konsole aktivieren (optional)
    // system("chcp 65001 > nul");
#endif

    initFunctions(); // Standardfunktionen registrieren
    Calculator calc;
    calc.run();
    return 0;
}
