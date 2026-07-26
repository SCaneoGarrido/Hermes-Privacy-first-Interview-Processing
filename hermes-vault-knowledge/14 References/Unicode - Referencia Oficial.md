---
title: Unicode - Referencia Oficial
aliases: ["Unicode"]
tags: [reference, unicode, text]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: https://home.unicode.org/
related: ["Sprint 5 - Whisper Integration", "05 C++"]
---

# Summary

Sitio oficial del Unicode Consortium, estándar de codificación de texto relevante para el manejo de transcripciones multi-idioma en Hermes.

# Explanation

Fuente: https://home.unicode.org/

El intento de verificación automática del 2026-07-24 no pudo confirmar el contenido completo de la página (contenido truncado en la herramienta de fetch). Se documenta con base en conocimiento general ampliamente establecido: el Unicode Consortium mantiene el estándar Unicode, que define los "code points" y las codificaciones de transformación (UTF-8, UTF-16, UTF-32) usadas para representar texto en prácticamente todos los idiomas. **Acción sugerida:** reverificar esta fuente con una consulta directa antes de citarla en una nota técnica profunda.

# Why it matters

Las entrevistas procesadas por Hermes pueden estar en distintos idiomas (ver "Detección idioma" en [[Sprint 5 - Whisper Integration]]); el manejo correcto de Unicode (especialmente UTF-8 en C++) es necesario para no corromper texto no latino en las transcripciones, resúmenes o exportaciones.

# Best Practices

Tratar todo el texto interno de Hermes como UTF-8 de punta a punta (desde whisper.cpp hasta la exportación a DOCX/PDF), evitando conversiones implícitas a codificaciones de una sola página de código (por ejemplo, Windows-1252).

# Common Mistakes

Asumir codificación ASCII o Latin-1 al leer/escribir archivos de texto en Windows, corrompiendo caracteres de idiomas no ingleses.

# Hermes Usage

Relevante para [[Sprint 5 - Whisper Integration]] (detección de idioma) y para cualquier nota futura de `std::filesystem`/manejo de strings en [[05 C++]].

# Related Notes

- [[Sprint 5 - Whisper Integration]]
- [[05 C++]]

# References

- https://home.unicode.org/ (Priority 2, docs/documentation_tech.yml — contenido no verificado en profundidad, ver nota arriba)
