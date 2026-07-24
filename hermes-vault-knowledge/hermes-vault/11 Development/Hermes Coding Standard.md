---
title: Hermes Coding Standard
aliases: ["Coding Standard", "Convenciones de Codigo"]
tags: [development, hermes, cpp, standard]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: .ai/CODING_STANDARD.md
related: ["Maintainability over Cleverness", "ADR-002 - C++20 como Lenguaje", "Filosofia de Repositorios"]
---

# Summary

Convenciones de nomenclatura y buenas prácticas de C++ obligatorias para todo el código de Hermes.

# Explanation

**Nomenclatura**

| Elemento | Convención | Ejemplo |
|---|---|---|
| Clases | PascalCase | `InterviewService` |
| Funciones | camelCase | `transcribeAudio()` |
| Miembros privados | `m_` + camelCase | `m_repository` |
| Interfaces | prefijo `I` | `ITranscriber` |
| Constantes | UPPER_CASE | `MAX_FILE_SIZE` |
| Namespaces | `hermes::modulo` | `hermes::storage` |

**Reglas estructurales**
- Una clase por archivo.
- Composición preferida sobre herencia.
- Prohibidas las variables globales.
- Uso obligatorio de [[RAII]].
- Prohibido `new`/`delete` crudos.
- Preferir `unique_ptr`; usar `shared_ptr` solo cuando la propiedad es realmente compartida.
- Todo método público debe estar documentado.

# Why it matters

Un estándar de nomenclatura consistente permite que cualquier colaborador nuevo lea el código sin necesidad de explicación adicional, en línea con el principio [[Maintainability over Cleverness]] y con la meta de que Hermes sea comprensible como proyecto open-source.

# Best Practices

- Nombrar interfaces siempre con prefijo `I` (`ITranscriber`, `ILLMClient`), coherente con la [[Filosofia de Repositorios]].
- Usar `m_` para todo miembro privado, sin excepciones.

# Common Mistakes

- Usar `new`/`delete` manual en vez de `unique_ptr`/`shared_ptr`.
- Mezclar convenciones (por ejemplo, `snake_case` en funciones) heredadas de otros lenguajes o librerías.

# Hermes Usage

Este estándar aplica a todo el backend en C++20 (ver [[ADR-002 - C++20 como Lenguaje]]) y es verificado en revisión de código.

# Related Notes

- [[Maintainability over Cleverness]]
- [[ADR-002 - C++20 como Lenguaje]]
- [[Filosofia de Repositorios]]
- [[05 C++]]

# References

- .ai/CODING_STANDARD.md (Priority 1)
