# InlineCalculator

Erweiterbarer Taschenrechner fuer den Windows Kommandoprozessor (CMD), geschrieben in C++14.

## Funktionsumfang

### Grundrechnungsarten
| Ausdruck    | Beschreibung      |
|-------------|-------------------|
| `3 + 4`     | Addition          |
| `10 - 3`    | Subtraktion       |
| `5 * 6`     | Multiplikation    |
| `10 / 4`    | Division          |
| `2 ^ 8`     | Potenz (2 hoch 8) |
| `(3+4) * 2` | Klammerrechnung   |

### Mathematische Funktionen
| Ausdruck          | Beschreibung                       |
|-------------------|------------------------------------|
| `sqrt(16)`        | Quadratwurzel (Ergebnis: 4)        |
| `wurzel(16)`      | Quadratwurzel (Deutsch-Alias)      |
| `square(5)`       | Quadrat 5^2 (Ergebnis: 25)         |
| `quadrat(5)`      | Quadrat (Deutsch-Alias)            |
| `sin(pi/2)`       | Sinus (Bogenmass)                  |
| `asin(1)`         | Arkussinus                         |
| `arcsin(0.5)`     | Arkussinus                         |
| `arcussinus(1)`   | Arkussinus (Deutsch-Alias)         |
| `cos(0)`          | Kosinus                            |
| `tan(pi/4)`       | Tangens                            |
| `ln(e)`           | Natuerlicher Logarithmus           |
| `log(100)`        | Dekadischer Logarithmus (Basis 10) |
| `exp(1)`          | Exponentialfunktion e^x            |
| `abs(-5)`         | Absolutwert                        |
| `deg(pi)`         | Bogenmass -> Grad                  |
| `rad(180)`        | Grad -> Bogenmass                  |

### Kalkulus (Variable `x`)
| Ausdruck                          | Beschreibung                        |
|-----------------------------------|-------------------------------------|
| `integral(x^2, 0, 1)`            | Numerisches Integral (Simpsonsregel)|
| `integral(sin(x), 0, pi, 500)`   | Integral mit 500 Schritten          |
| `diff(x^2, 3)`                   | Numerische Ableitung bei x=3        |
| `diff(sin(x), 0)`                | Ableitung von sin(x) bei x=0        |
| `differential(x^3, 2)`           | Alias fuer diff                     |

### Konstanten
| Name | Wert          |
|------|---------------|
| `pi` | 3.14159265... |
| `e`  | 2.71828182... |

## Kompilieren und Ausfuehren

### Voraussetzungen
- C++14-kompatibler Compiler (GCC, Clang, MSVC)
- CMake 3.10 oder neuer

### Windows (Visual Studio / MSVC)
```bat
mkdir build
cd build
cmake ..
cmake --build . --config Release
Release\InlineCalculator.exe
```

### Windows (MinGW / g++)
```bat
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
mingw32-make
InlineCalculator.exe
```

### Linux / macOS
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
./InlineCalculator
```

## Erweiterbarkeit

Neue einfache Funktionen der Form `f(x) -> double` koennen ohne Aenderung
des Parsers in `initFunctions()` registriert werden:

```cpp
// Beispiel: Hyperbelfunktionen hinzufuegen
g_registry.registerFunction("sinh", [](double x) { return std::sinh(x); });
g_registry.registerFunction("cosh", [](double x) { return std::cosh(x); });
g_registry.registerFunction("tanh", [](double x) { return std::tanh(x); });
```

Fuer Funktionen mit mehreren Argumenten oder Kalkulus-Semantik (wie `integral`/`diff`)
wird `ExpressionEvaluator::parseFunction()` in `src/main.cpp` erweitert.

## Beispielsitzung

```
+----------------------------------------------------------+
|          InlineCalculator  v1.0                          |
|  Erweiterbarer Rechner - Windows Kommandoprozessor       |
+----------------------------------------------------------+

  Tippen Sie einen Ausdruck und druecken Sie ENTER.
  'hilfe' zeigt alle Funktionen, 'beenden' beendet das Programm.

> 3 + 4 * 2
= 11

> sqrt(2)
= 1.414213562

> sin(pi / 6)
= 0.5

> integral(x^2, 0, 1)
= 0.3333333333

> diff(x^2, 3)
= 5.99999999

> hilfe        (zeigt alle Befehle)
> beenden      (Programm beenden)
```
