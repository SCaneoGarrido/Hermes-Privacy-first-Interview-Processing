# ADR 0001: Motor de base de datos y librería cliente en C++

## Estado

Aceptado — 2026-07-27

## Contexto

El proyecto arrancó pensando en SQLite (embebido, sin servidor), pero se decidió
migrar a MySQL desde el inicio para dejar el proyecto listo con una base de
datos robusta desde el día uno, en vez de migrar más adelante. En el momento
de esta decisión no existía aún código de acceso a datos (`backend/api/shared/database`
estaba vacío), por lo que el cambio no implicó migración de datos ni de código.

Se decidió correr MySQL dentro de Docker (`docker-compose.yml` en la raíz del
repo) en vez de instalarlo nativo en la máquina de cada desarrollador, para
evitar drift de configuración entre entornos y para que el setup local sea
igual al que se usará en producción.

Para la librería cliente en C++ se evaluaron dos opciones:

1. **mysql-connector-cpp** (connector oficial de Oracle/MySQL), usando el
   feature `jdbc` para tener una API relacional clásica sobre el puerto 3306.
2. **MariaDB Connector/C** (`libmariadb`), con un wrapper C++ propio.

Al intentar compilar la opción 1 se encontró que el feature `jdbc` depende del
puerto `libmysql` de vcpkg, el cual declara explícitamente
`"supports": "!android & !mingw & !uwp & !xbox"` — no soporta MinGW, que es el
toolchain de este proyecto (no hay Visual Studio instalado, se compila con
MinGW-w64). La build falla en la etapa de `vcpkg install`.

La alternativa dentro de mysql-connector-cpp sin el feature `jdbc` (X DevAPI)
sí compila en MinGW, pero:

- Usa el protocolo X (puerto 33060 por defecto) en vez del protocolo clásico
  de MySQL (3306), lo que exige exponer y documentar un puerto extra en Docker.
- Arrastra una cadena de dependencias pesada (protobuf, openssl, rapidjson,
  zlib, lz4, zstd), con tiempos de compilación largos.

## Decisión

Se usa **MariaDB Connector/C** (`libmariadb`, paquete de vcpkg) como cliente
de base de datos, en vez de mysql-connector-cpp.

Motivo principal: el proyecto está pensado para ser open source, y agregar
complejidad de dependencias o de build hace más difícil que terceros puedan
clonar, compilar y contribuir. `libmariadb`:

- Soporta MinGW sin problemas (`"supports": "!uwp & !xbox"`).
- Habla el protocolo clásico de MySQL (puerto 3306), sin puertos ni plugins
  adicionales que documentar.
- Solo depende de `zlib` (+ iconv en Windows), build mucho más rápido que la
  cadena de protobuf/openssl que exige mysql-connector-cpp.
- Es la misma librería cliente que usan por debajo herramientas como
  MySQL Workbench, PHP o Python — protocolo maduro y ampliamente soportado.

El costo asumido es escribir y mantener un wrapper C++ propio (RAII) sobre la
API en C de `libmariadb`, en vez de usar la API C++ nativa que trae el
connector oficial. Se considera un costo menor y controlado, alineado con el
estilo del resto del backend (excepciones propias, logger propio).

## Consecuencias

- `backend/vcpkg.json` depende de `libmariadb` (target de CMake:
  `unofficial::libmariadb`, vía `find_package(unofficial-libmariadb CONFIG REQUIRED)`).
- `docker-compose.yml` solo necesita exponer el puerto clásico (3306), sin
  puerto X adicional.
- Falta escribir la capa de acceso a datos real (`Connection`,
  `PreparedStatement` con RAII) en `backend/api/shared/database/`, hoy vacía
  salvo por `connection_test.cpp`, que es solo una prueba de conectividad y no
  parte de la app.

## Nota de compatibilidad (MinGW)

Con el build estatico de `libmariadb` en `x64-mingw-static`, el constructor
global que normalmente inicializa la libreria (WSAStartup, etc.) no se
ejecuta solo de forma confiable. Sin esa inicializacion, `mysql_real_connect`
falla con `Lost connection to server at 'handshake: reading initial
communication packet'` aunque el servidor y la red esten bien (verificado con
lectura de socket cruda y con el cliente oficial de MariaDB, ambos exitosos).

Solucion: llamar explicitamente a `mysql_library_init(0, nullptr, nullptr)`
antes de cualquier otra funcion de la API, y `mysql_library_end()` al
terminar. Ver `connection_test.cpp` para el patron a seguir en la futura capa
de acceso a datos.
