# Analizador léxico SQL — Laboratorio 4

Módulo independiente (`include/minidb/lexer`, `src/lexer`). No depende de `storage/`, `buffer/` ni `index/`;
no ejecuta consultas ni consulta el catálogo.

## Compilar, ejecutar y probar

```bash
cmake -B build && cmake --build build -j
./build/lexer_test                              # pruebas unitarias
./build/lexer_demo -e "SELECT a FROM t WHERE a >= 18;"
./build/lexer_demo consulta.sql                 # o por stdin: ./build/lexer_demo < consulta.sql
```

Salida de `lexer_demo`: una línea `TIPO  'lexema'  línea:columna` por token y, al final, los errores. Código de
salida 0 sin errores, 1 con errores léxicos, 2 si falla la lectura.

## Especificación de tokens

| Categoría | Tipo (`TokenType`) | Regla |
|---|---|---|
| Palabras reservadas | `KEYWORD_SELECT/FROM/WHERE/AND/OR` | Sin distinguir mayúsculas; el lexema conserva la grafía original |
| Identificador | `IDENTIFIER` | `[A-Za-z_][A-Za-z0-9_]*`, solo ASCII; conserva mayúsculas |
| Entero | `INTEGER_LITERAL` | `[0-9]+` (el rango int32 lo valida el parser) |
| Decimal | `DECIMAL_LITERAL` | `[0-9]+ '.' [0-9]+` |
| Cadena | `STRING_LITERAL` | `'...'`; `''` dentro = comilla; admite UTF-8 y saltos de línea; el lexema va sin comillas |
| Relacionales | `EQUAL` `NOT_EQUAL` `LESS` `GREATER` `LESS_EQUAL` `GREATER_EQUAL` | `=`, `<>` y `!=` (ambos → `NOT_EQUAL`), `<`, `>`, `<=`, `>=` |
| Puntuación | `COMMA` `SEMICOLON` | `,` `;` |
| Extras | `STAR` `LEFT_PAREN` `RIGHT_PAREN` | `*` `(` `)` para `SELECT *` y condiciones agrupadas |
| Fin | `EOF` | Siempre el último token, con la posición del final de la entrada |

Reglas generales: espacios, tabulaciones, `\r` y `\n` se ignoran; los operadores usan *maximal munch*
(`>=` nunca se parte en `>` y `=`); línea y columna son base 1 y la columna cuenta caracteres UTF-8.
No existen comentarios, signo `-` ni `.` fuera de un decimal: se informan como carácter no admitido.

## Errores léxicos

Cada error lleva tipo, mensaje, lexema y posición. Nunca se convierte en token y el análisis continúa,
de modo que se reportan todos de una vez.

| Tipo | Ejemplo |
|---|---|
| `INVALID_CHARACTER` | `@`, `#`, `€` |
| `UNTERMINATED_STRING` | `'Ana` |
| `MALFORMED_NUMBER` | `12.`, `1.2.3`, `12abc` |
| `INVALID_OPERATOR` | `!` suelto |

## Pruebas (`tests/lexer_test.cpp`, 34 casos, todos pasan)

| Escenario del enunciado | Casos |
|---|---|
| 1. Sentencia válida | ejemplo del enunciado, AND/OR/`*`/paréntesis |
| 2. Operadores compuestos | `>= <= <> !=`, sin espacios, separados |
| 3. Literales | enteros, decimales, cadenas, `''`, cadena vacía, UTF-8 |
| 4. Reservadas vs identificadores | mayúsculas/minúsculas, prefijos (`selected`, `orange`) |
| 5. Espacios | tabs, `\r\n`, líneas vacías, posiciones línea:columna |
| 6. Errores | carácter inválido, cadena sin cerrar, números mal formados, `!`, múltiples errores |
| 7. Casos límite | entrada vacía, solo espacios, sentencia incompleta, token al final, EOF único |
