# ALMA

ALMA es un lenguaje de la familia Lisp que genera código C. Un programa de ALMA
se ejecuta al compilar: el código C que produce son datos de ALMA (listas), así
que se puede construir, transformar y generar con el propio lenguaje antes de
convertirlo en C.

Es una prueba de concepto en desarrollo.

**Documentación: https://hectareagalbis.github.io/ALMA/**

## Compilar

Hace falta CMake (3.10 o posterior), `g++` con soporte de C++23 y `gcc`.

```sh
mkdir -p build && cd build
cmake .. && make
```

Esto genera `build/src/alma2c`, que traduce un archivo de ALMA a C.

## El primer programa

Guarda esto en `hola.alma`:

```lisp
(function main int ()
  (puts "Hola mundo")
  (return 0))
```

El script `alma` lo traduce con `alma2c` y lo compila con `gcc`:

```sh
./alma -o hola hola.alma
./hola
```

El C generado es:

```c
#include <stdio.h>

int main(void)
{
    puts("Hola mundo");
    return 0;
}
```

`alma2c` sin argumentos abre un REPL. En `examples/` hay más ejemplos, y en la
[documentación](https://hectareagalbis.github.io/ALMA/) está todo el lenguaje.

## Licencia

MIT. Ver `LICENSE`, `AUTHORS` y `CONTRIBUTORS`.
