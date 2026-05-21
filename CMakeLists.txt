cmake_minimum_required(VERSION 3.10)

project(InlineCalculator
    VERSION 1.0
    DESCRIPTION "Erweiterbarer Taschenrechner fuer den Windows Kommandoprozessor"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(InlineCalculator src/main.cpp)

if(MSVC)
    # MSVC: Warnstufe 4, Laufzeit-Bibliothek statisch linken fuer einfache Weitergabe
    target_compile_options(InlineCalculator PRIVATE /W4 /MT)
    # M_PI / M_E ueber _USE_MATH_DEFINES (bereits in main.cpp definiert)
else()
    target_compile_options(InlineCalculator PRIVATE -Wall -Wextra -Wpedantic)
    # Mathematik-Bibliothek auf Unix/Linux
    target_link_libraries(InlineCalculator PRIVATE m)
endif()

# Installationsregel (optional)
install(TARGETS InlineCalculator RUNTIME DESTINATION bin)
