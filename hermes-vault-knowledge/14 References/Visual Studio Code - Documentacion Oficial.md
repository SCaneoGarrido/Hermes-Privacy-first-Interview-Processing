---
title: Visual Studio Code - Documentacion Oficial
aliases: ["VS Code", "Visual Studio Code"]
tags: [reference, vscode, tooling]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: https://code.visualstudio.com/docs
related: ["11 Development", "Visual Studio - Documentacion Oficial"]
---

# Summary

Documentación oficial de Visual Studio Code, editor alternativo (multiplataforma) para desarrollar Hermes.

# Explanation

Fuente: https://code.visualstudio.com/docs

Verificado 2026-07-24: cubre funcionalidades del editor, debugging, testing, control de versiones, desarrollo remoto y configuración enterprise. La extensión oficial de Microsoft C/C++ (`ms-vscode.cpptools`) y el soporte de CMake Tools permiten compilar y depurar Hermes en Windows, WSL, Linux o macOS desde el mismo editor.

# Why it matters

A diferencia de Visual Studio (solo Windows/Mac, IDE completo), VS Code permite a colaboradores en Linux/macOS trabajar en Hermes con la misma configuración de CMake + vcpkg, sin cambiar de toolchain — relevante para un proyecto open-source con contribuidores en distintos sistemas operativos.

# Best Practices

Versionar una configuración de VS Code compartida (`.vscode/settings.json`, `.vscode/c_cpp_properties.json`) en el repositorio para que cualquier colaborador tenga IntelliSense correcto desde el primer clone.

# Common Mistakes

Depender de configuración local de VS Code no versionada, causando que cada colaborador reconfigure manualmente las rutas de include de vcpkg.

# Hermes Usage

Editor recomendado para colaboradores multiplataforma; documentado en [[11 Development]].

# Related Notes

- [[11 Development]]
- [[Visual Studio - Documentacion Oficial]]

# References

- https://code.visualstudio.com/docs (Priority 2, docs/documentation_tech.yml)
