---
title: Compilar y Ejecutar el Backend con CMake
aliases: ["Build y Run Backend", "Correr Hermes", "CMake Build Hermes"]
tags: [development, hermes, cmake, vcpkg, cpp, build]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: backend/CMakeLists.txt, backend/vcpkg.json
related: ["CMake - Documentacion Oficial", "Crow - Documentacion Oficial", "Sprint 0 - Foundation", "Hermes Coding Standard"]
---

# Summary

Flujo verificado para compilar y ejecutar el backend de Hermes con CMake + vcpkg (manifest mode) en Windows, sin Visual Studio instalado.

# Explanation

**Requisitos previos**
- Variable de entorno `VCPKG_ROOT` apuntando a la instalación local de vcpkg.
- MinGW-w64 instalado y accesible en el `PATH` (g++, gcc, ninja, cmake).
- No se necesita Visual Studio: el proyecto está configurado para compilar con MinGW.

**Comandos (PowerShell, desde `backend/`)**

```powershell
cmake -S . -B build
cmake --build build
.\build\backend.exe
```

- `cmake -S . -B build` configura el proyecto y, gracias al manifest mode de vcpkg (`vcpkg.json` junto al `CMakeLists.txt`), instala automáticamente todas las dependencias declaradas (`fmt`, `crow`, etc.). No hace falta ejecutar `vcpkg install` a mano.
- `cmake --build build` compila. Vuelve a ejecutar la configuración solo si detecta cambios en `CMakeLists.txt` o `vcpkg.json`.
- El backend usa Crow; al arrancar imprime `Crow/master server is running at http://0.0.0.0:18080`. Esa es la dirección de **escucha** (todas las interfaces), no una URL navegable — hay que entrar por `http://localhost:18080/` o `http://127.0.0.1:18080/`.

# Why it matters

Sin este flujo documentado, cualquier colaborador nuevo (o una sesión de IA sin memoria previa) repetiría desde cero la misma cadena de errores de configuración de vcpkg/MinGW, perdiendo tiempo significativo. Es la base del entregable "Primer proyecto compilando" de [[Sprint 0 - Foundation]].

# Best Practices

- Fijar el triplet de vcpkg a `x64-mingw-static` (tanto `VCPKG_TARGET_TRIPLET` como `VCPKG_HOST_TRIPLET`) dentro del propio `CMakeLists.txt`, porque esta máquina no tiene Visual Studio y vcpkg asume por defecto el triplet `x64-windows` (requiere MSVC) tanto para el paquete destino como para las herramientas host (`vcpkg-cmake`, `vcpkg-cmake-config`).
- Preferir el triplet **static** sobre **dynamic** en MinGW: vcpkg solo copia automáticamente las DLLs de dependencias junto al `.exe` para triplets `windows/uwp/xbox`, no para triplets `mingw`. Con `-dynamic` el ejecutable compila pero falla en tiempo de ejecución (`STATUS_DLL_NOT_FOUND`) porque no encuentra `libfmtd.dll`/`libcrow.dll`.
- En Windows + MinGW, enlazar explícitamente `ws2_32` y `mswsock` cuando se usa Asio (dependencia de Crow) — con MSVC estas librerías de sockets se enlazan solas vía `#pragma comment(lib, ...)`, pero MinGW lo ignora y falla el link con `undefined reference to WSAStartup` y similares.
- Activar `set(CMAKE_EXPORT_COMPILE_COMMANDS ON)` en el `CMakeLists.txt` y apuntar `"C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools"` en `.vscode/settings.json`, para que el IntelliSense de VS Code encuentre los headers de vcpkg y no marque `#include "crow.h"` en rojo.
- Confiar en manifest mode de vcpkg: nunca hace falta invocar `vcpkg.exe` a mano (de hecho no está en el `PATH` de PowerShell por defecto).

# Common Mistakes

- Ejecutar `vcpkg install <paquete>` manualmente — innecesario en manifest mode y además falla porque `vcpkg.exe` no está en el `PATH`.
- Usar el target `crow::crow` (minúsculas) en `target_link_libraries` — el target real que exporta el port de vcpkg es `Crow::Crow`.
- Abrir `http://0.0.0.0:18080` en el navegador esperando que responda — es la dirección de bind, no una URL válida para un cliente; usar `localhost`/`127.0.0.1`.
- Compilar `main.cpp` con el botón "Run C/C++ File" o la tarea autogenerada `gcc.exe build active file` de `.vscode/tasks.json` — esa tarea invoca `gcc` directo sobre el archivo activo, sin los include paths de vcpkg, y falla con `crow.h: No such file or directory` aunque el build real vía CMake funcione perfecto.
- Dejar el triplet en `x64-mingw-dynamic` sin copiar las DLLs manualmente — el ejecutable compila y "parece" funcionar hasta que se ejecuta y crashea por `STATUS_DLL_NOT_FOUND`.

# Hermes Usage

Este es el flujo de build/run real y verificado del backend de Hermes (`backend/CMakeLists.txt`), entregable de [[Sprint 0 - Foundation]] y base de trabajo diario para todo el desarrollo en C++20 sobre [[Crow - Documentacion Oficial|Crow]].

# Related Notes

- [[CMake - Documentacion Oficial]]
- [[Crow - Documentacion Oficial]]
- [[Sprint 0 - Foundation]]
- [[Hermes Coding Standard]]
- [[ADR-002 - C++20 como Lenguaje]]
- [[ADR-003 - Crow como Framework REST]]

# References

- backend/CMakeLists.txt (verificado directamente en el entorno de desarrollo, 2026-07-24)
- backend/vcpkg.json
- https://cmake.org/documentation/ (Priority 2)
- https://crowcpp.org/master/ (Priority 2)
