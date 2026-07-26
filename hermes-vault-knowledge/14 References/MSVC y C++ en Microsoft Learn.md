---
title: MSVC y C++ en Microsoft Learn
aliases: ["MSVC"]
tags: [reference, msvc, cpp, windows]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: https://learn.microsoft.com/cpp/
related: ["ADR-002 - C++20 como Lenguaje", "Visual Studio - Documentacion Oficial", "Windows Development - Microsoft Learn"]
---

# Summary

Documentación oficial de Microsoft para C++, C y el compilador MSVC — relevante si Hermes se compila con la toolchain de Visual Studio en Windows.

# Explanation

Fuente: https://learn.microsoft.com/cpp/

Verificado 2026-07-24: cubre instalación de Visual Studio con workloads de C++, referencia de la librería estándar de C++ y del C runtime, MFC/ATL, C++/CLI y C++/WinRT, programación paralela, uso de C++ en VS Code (Windows, WSL, Linux, macOS), y perfilado/depuración de código nativo.

# Why it matters

Si el equipo compila Hermes con MSVC en Windows (alternativa a MinGW/Clang), esta es la referencia oficial del compilador y su librería estándar específica, distinta de la referencia general de [[C++ - Referencias del Lenguaje|cppreference]].

# Best Practices

No asumir que todo el código válido en MSVC es portable a GCC/Clang (y viceversa); verificar conformidad con el estándar C++20 usando [[C++ - Referencias del Lenguaje]] como referencia neutral.

# Common Mistakes

Depender de extensiones específicas de MSVC sin documentarlo, dificultando compilar Hermes en Linux/macOS si el proyecto lo requiere a futuro.

# Hermes Usage

Referencia de la toolchain de compilación en Windows, complementaria a [[ADR-002 - C++20 como Lenguaje]].

# Related Notes

- [[ADR-002 - C++20 como Lenguaje]]
- [[Visual Studio - Documentacion Oficial]]
- [[Windows Development - Microsoft Learn]]

# References

- https://learn.microsoft.com/cpp/ (Priority 2, docs/documentation_tech.yml)
