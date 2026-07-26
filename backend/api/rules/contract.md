## Contrato de respuesta — invariante

**Todas** las respuestas siguen esta estructura sin excepción:

```json
{
  "success": true | false,
  "data": <objeto | array | null>,
  "error": { "code": "SCREAMING_SNAKE_CASE", "message": "..." } | null
}
```

- Si `success: true` → `error` es `null`
- Si `success: false` → `data` es `null`
- `error.code` siempre en `SCREAMING_SNAKE_CASE`

Nunca romper este contrato. Nunca devolver una estructura diferente por conveniencia.