---
title: OpenSSL - Documentacion Oficial
aliases: ["OpenSSL"]
tags: [reference, openssl, security, no-adoptado]
status: stable
created: 2026-07-24
updated: 2026-07-24
source: https://docs.openssl.org/
related: ["Alcance y Exclusiones", "Privacy First"]
---

# Summary

Documentación oficial de OpenSSL: **no es una dependencia actual de Hermes** (no aparece en `.ai/PROJECT.md` ni en ninguna ADR), pero está listada en `docs/documentation_tech.yml` como fuente oficial disponible.

# Explanation

Fuentes oficiales:
- https://docs.openssl.org/
- https://github.com/openssl/openssl

Verificado 2026-07-24: OpenSSL es un toolkit criptográfico con herramientas de línea de comandos y librerías (`libcrypto` para primitivas criptográficas, `libssl` para TLS/SSL), con guías para implementar clientes/servidores TLS y QUIC.

# Why it matters

Podría volverse relevante si en el futuro Hermes expone su REST API sobre HTTPS (por ejemplo, para un escenario multiusuario en red local) — hoy no es necesario porque Hermes es [[Local First]] y de un solo usuario por defecto.

# Best Practices

Si se introduce HTTPS en el futuro, documentar la decisión como una nueva ADR, evaluando también si Crow ya provee una integración TLS antes de añadir OpenSSL directamente.

# Common Mistakes

Asumir que Hermes ya maneja TLS/HTTPS porque esta nota existe: esta es solo una fuente de referencia disponible, no una dependencia adoptada.

# Hermes Usage

Ninguno actualmente.

# Related Notes

- [[Alcance y Exclusiones]]
- [[Privacy First]]

# References

- https://docs.openssl.org/ (Priority 2, docs/documentation_tech.yml)
- https://github.com/openssl/openssl (Priority 4, docs/documentation_tech.yml)
