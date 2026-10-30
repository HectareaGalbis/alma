# ALMA

ALMA es un lenguaje de la familia Lisp que genera código C. Un programa de ALMA
se ejecuta al compilar: el código C que produce son datos de ALMA (listas), así
que se puede construir, transformar y generar con el propio lenguaje antes de
convertirlo en C. El código generado se comprueba antes de escribirse: tipos,
declaraciones, ámbitos, etiquetas...

```lisp
(function main int ()
  (puts "Hola mundo")
  (return 0))
```

```c
#include <stdio.h>

int main(void)
{
    puts("Hola mundo");
    return 0;
}
```

Es una prueba de concepto en desarrollo.

## Compilar

Hace falta CMake (3.10 o posterior) y `g++` con soporte de C++23. Para compilar
el C generado hace falta `gcc`.

```sh
mkdir -p build && cd build
cmake .. && make
```

Esto genera `build/src/alma2c`, que traduce un archivo de ALMA a C:

```sh
build/src/alma2c programa.alma -o programa.c
```

El tipo de compilación se elige con `CMAKE_BUILD_TYPE`:

| Tipo | Opciones |
|---|---|
| `Release` (por defecto) | `-O2`, `NDEBUG` |
| `Debug` | `-g` y avisos |
| `FastDebug` | `-O2`, avisos y `-fsanitize=address` |

```sh
cmake -DCMAKE_BUILD_TYPE=Debug ..
```

## Instalar

`cmake --install` copia `alma2c` y el script `alma` en `<prefijo>/bin`:

```sh
cmake --install build                      # en /usr/local/bin (puede necesitar sudo)
cmake --install build --prefix ~/.local    # en ~/.local/bin
```

## Usar ALMA

El script `alma` traduce los archivos `.alma` con `alma2c` y llama a `gcc`.
Todos los demás argumentos se pasan a `gcc` sin cambios, así que se pueden
mezclar archivos `.alma`, `.c` y `.o` y usar cualquier opción de `gcc`:

```sh
./alma -o hola examples/01_hola_mundo.alma           # ejecutable
./alma -c programa.alma                              # objeto programa.o
./alma -Wall -O2 -o prog main.alma util.c -lm        # cualquier opción de gcc
./alma --emit-c programa.alma                        # solo genera programa.c
```

`alma2c` se busca en `$ALMA2C`, junto al script, en `build/src/` y en el `PATH`.

## El lenguaje

### Tipos

Los tipos se escriben como listas: el nombre y después los especificadores.
`const` va antes del nombre para calificar el tipo base, o después de `*` para
calificar ese puntero.

| ALMA | C |
|---|---|
| `int` | `int` |
| `(char *)` | `char *` |
| `(char * *)` | `char **` |
| `(int 10)` | `int [10]` |
| `(int ((char *) int))` | `int (char *, int)` |
| `(int ((char *) &rest))` | `int (char *, ...)` |
| `(void (int) *)` | `void (*)(int)` |
| `(const int)` | `const int` |
| `(const char *)` | `const char *` |
| `(char * const)` | `char *const` |
| `(const char * const)` | `const char *const` |

No se puede asignar a nada `const` (tampoco a través de punteros ni a miembros
de un struct `const`), y `char *` se convierte en `const char *`, pero no al
revés.

### Definiciones y declaraciones

```lisp
(function suma int ((a int) (b int))      ; definición
  (return (+ a b)))
(function externa int ((x int)))          ; sin cuerpo: declaración
(function nada void () ())                ; cuerpo vacío

(variable contador int 0)                 ; variable global (valor constante)
(variable externa int)                    ; sin valor: extern int externa;

(struct Punto (x int) (y int))            ; struct + typedef
(union Valor (i int) (f float))
(enum Color ROJO VERDE (AZUL 5) NEGRO)    ; enum + typedef
```

Un `enum` es un tipo entero y sus constantes son constantes `int`: se pueden
usar en un `case` o para inicializar variables globales. El valor de una
constante debe ser un entero constante y puede usar las anteriores.

Dentro de una función, `(variable i int 0)` define una variable local, con o
sin valor.

### Expresiones

Cada operación se escribe entre paréntesis en el C generado, así que la
precedencia de C no importa.

| ALMA | C |
|---|---|
| `(+ a b c)` `(- a b)` `(* a b)` `(/ a b)` `(% a b)` | `+ - * / %` |
| `(- a)` `(* p)` `(& x)` `(aref a i)` | `-a`, `*p`, `&x`, `a[i]` |
| `(== a b)` `(!= a b)` `(< a b)` `(> a b)` `(<= a b)` `(>= a b)` | comparaciones |
| `(and a b)` `(or a b)` `(not a)` | `&&`, `\|\|`, `!` |
| `(bit-and a b)` `(bit-or a b)` `(bit-xor a b)` `(bit-not a)` `(<< a n)` `(>> a n)` | operadores de bits |
| `(= x v)` `(+= x v)` ... `(bit-or= x v)` | asignaciones |
| `(++ x)` `(-- x)` `(post++ x)` `(post-- x)` | incrementos |
| `(? c a b)` `(comma a b)` `(sizeof x)` `(cast tipo v)` | `?:`, `,`, `sizeof`, cast |
| `(f a b)` | llamada a una función |
| `(p x)` `(p origen x)` | `p.x`, `p.origen.x` (o `->` si `p` es un puntero) |

### Sentencias

```lisp
(if condicion sentencia [sentencia])      ; (block ...) agrupa varias
(while condicion sentencias...)
(do-while condicion sentencias...)
(for (inicio condicion paso) sentencias...)
(switch valor (case 1 sentencias...) (default sentencias...))
(break) (continue) (return [valor])
(label nombre) (goto nombre)
```

### Comprobaciones

Antes de escribir el C, `alma2c` comprueba, entre otras cosas:

- que las funciones y variables usadas estén declaradas,
- el número y el tipo de los argumentos de las llamadas,
- los tipos de los operadores y de las asignaciones, con las reglas básicas de C,
- que no haya redefiniciones ni declaraciones incompatibles,
- que no se modifique nada `const` ni se pierda `const` en una conversión,
- que `return` devuelva el tipo de la función,
- que `break`, `continue` y `goto` estén donde pueden estar,
- que las variables globales se inicialicen con constantes.

### Biblioteca estándar

Las funciones, variables y macros de la biblioteca estándar de C ya están
declaradas (`printf`, `malloc`, `strlen`, `sqrt`, `EOF`, `NULL`...), con `const`
donde C lo tiene. Solo se añaden al C los `#include` de lo que se usa.

### Macros del preprocesador

`macro` define una macro cuyo cuerpo es código de ALMA que devuelve el código C
que la sustituye. Funciona como el preprocesador de C: los argumentos se
expanden antes, el resultado se vuelve a expandir y una macro no se expande
dentro de su propia expansión. Una macro tiene preferencia sobre una función con
el mismo nombre.

```lisp
(macro cuadrado (x) `(* ,x ,x))
(macro when (condicion &rest cuerpo) `(if ,condicion (block ,@cuerpo)))
```

### ALMA generando C

`function`, `variable`, `struct`, `union`, `enum` y `macro` son macros de ALMA que se
expanden a una llamada a `emit`. `emit` es una función que recibe código C como
datos, así que un programa puede construirlo con todo lo que hay en ALMA:
`defun`, `defmacro`, `let`, `lambda`, `if`, `list`, quasiquote...

```lisp
; Genera una función que devuelve una constante por cada elemento de la lista
(defun constantes (lista)
  (if lista
    (progn
      (emit `(function ,(car (car lista)) int () (return ,(car (cdr (car lista))))))
      (constantes (cdr lista)))))

(constantes '((uno 1) (dos 2) (tres 3)))
```

### ALMA en tiempo de compilación

Todo lo que no es C se ejecuta al compilar. Además de los vectores, ALMA tiene:

| Forma | Descripción |
|---|---|
| `t`, `nil` | verdadero; falso y lista vacía |
| `(quote x)`, `'x`, `` `x ``, `,x`, `,@x` | quote y quasiquote |
| `(if c a [b])`, `(progn ...)`, `(let ((a 1) (b 2)) ...)` | control y variables locales |
| `(defun nombre (args) ...)`, `(lambda (args) ...)` | funciones: los argumentos se evalúan |
| `(defmacro nombre (args) ...)` | macros: el resultado se vuelve a evaluar |
| `(defspecial nombre (args) ...)` | operadores especiales: los argumentos no se evalúan y el resultado no se vuelve a evaluar |
| `(set-symbol-value 'x v)` | dar valor a un símbolo |
| `car`, `cdr`, `list`, `eq`, `eval`, `+` | listas, comparación, evaluación y suma de enteros |
| `(print x)` | escribe `x` en la salida estándar al compilar y lo devuelve |
| `(load "archivo.alma")` | carga y ejecuta otro archivo |

```lisp
(defspecial cita (x) x)
(print (cita (a b c)))                   ; escribe (a b c)
```

#### Lista de parámetros

`defun`, `lambda`, `defmacro` y `defspecial` aceptan la misma lista de parámetros:

| Parámetro | Descripción |
|---|---|
| `a` | obligatorio |
| `(b defecto b-p)` | opcional; `defecto` y `b-p` se pueden omitir. `b-p` es `t` si se pasó el argumento |
| `:k x`, `:k (x defecto x-p)` | por keyword: se pasa como `:k valor` |
| `&rest r` | lista con los argumentos que sobran tras los opcionales |
| `&rest-keys otras` | tabla hash (`eql`) con los keywords recibidos que no están declarados |

Los obligatorios, los opcionales y los keywords van en ese orden; `&rest` y
`&rest-keys` pueden ir en cualquier posición. Un opcional o keyword sin valor por
defecto vale `nil`. Los valores por defecto se evalúan en cada llamada y pueden
usar los parámetros anteriores.

En la llamada, los keywords se buscan después de los argumentos obligatorios y
opcionales, mezclados con los del `&rest`, y nunca se meten en él. Un keyword no
declarado es un error salvo que haya `&rest-keys`, y repetir un keyword también
es un error. Si no se declara ningún keyword ni `&rest-keys`, los keywords son
valores normales.

```lisp
(defun f (a (b 10) (c (+ a b) c-p) :k x :j (y 5 y-p) &rest r &rest-keys otras)
  (list a b c c-p x y y-p r otras))

(f 1)                    ; (1 10 11 nil nil 5 nil nil <hash-table eql 0>)
(f 1 2 3 4 :k 7 5 :z 9)  ; (1 2 3 t 7 5 nil (4 5) <hash-table eql 1>)

(defun g (a &rest r) (list a r))
(g 1 :k 2)               ; (1 (:k 2))
```

### Cargar archivos

`load` ejecuta otro archivo de ALMA como si su contenido estuviera en el punto
donde se llama: lo que defina (funciones de ALMA, macros, declaraciones de C)
queda disponible después. Una ruta relativa se resuelve desde el directorio del
archivo que llama a `load`, no desde el directorio actual.

`*current-file*` contiene la ruta absoluta del archivo que se está cargando.

```lisp
(load "lib/util.alma")                   ; relativo a este archivo
(print *current-file*)
```

### Introspección

Desde ALMA se puede consultar lo que `emit` conoce, incluida la biblioteca
estándar. Las funciones reciben nombres como símbolos y devuelven los tipos con
la sintaxis de ALMA, así que sirven para generar código. Devuelven `nil` si lo
consultado no existe.

| Función | Ejemplo | Resultado |
|---|---|---|
| `c-declared-p`, `c-defined-p` | `(c-defined-p 'suma)` | `t` |
| `c-function-p`, `c-variable-p`, `c-constant-p`, `c-standard-p` | `(c-constant-p 'EOF)` | `t` |
| `c-struct-p`, `c-union-p`, `c-enum-p`, `c-macro-p` | `(c-enum-p 'Color)` | `t` |
| `c-type`, `c-header` | `(c-type 'printf)` | `(int ((const char *) &rest))` |
| `c-function-result`, `c-function-parameters`, `c-function-variadic-p` | `(c-function-parameters 'suma)` | `(int int)` |
| `c-record-defined-p`, `c-record-members`, `c-member-type` | `(c-record-members 'Punto)` | `((x int) (y int))` |
| `c-enum-constants` | `(c-enum-constants 'Color)` | `(ROJO VERDE AZUL NEGRO)` |
| `c-type-string`, `c-pointer-p`, `c-array-p`, `c-const-p` | `(c-type-string '(const char *))` | `"const char *"` |
| `c-functions`, `c-variables`, `c-records`, `c-enums` | `(c-records)` | `(Punto Valor)` |

Por ejemplo, una función de ALMA puede generar una función de C que imprima
todos los campos de cualquier struct a partir de `(c-record-members 'Punto)`.

Solo se ven las declaraciones globales de las formas ya emitidas: las macros se
expanden antes de emitir su forma, así que no ven las variables locales.

### Vectores

ALMA tiene vectores, implementados con una cola doble, para manipular datos al
compilar. Aplicar un vector a un índice devuelve su elemento.

```lisp
(set-symbol-value 'v (vector 1 2 3))     ; [1 2 3]
(v 0)                                    ; 1
(vector-set v 1 'dos)                    ; [1 dos 3]
(vector-push-back v 4)                   ; [1 dos 3 4]
(vector-pop-front v)                     ; 1
```

| Función | Descripción |
|---|---|
| `(vector a b ...)`, `(make-vector n [valor])` | crear un vector |
| `(v i)`, `(vector-set v i x)` | leer y escribir un elemento |
| `vector-front`, `vector-back` | primer y último elemento |
| `vector-push-front`, `vector-push-back` | añadir al principio o al final |
| `vector-pop-front`, `vector-pop-back` | quitar del principio o del final |
| `(vector-insert v i x)`, `(vector-erase v i)` | insertar antes de `i` y quitar |
| `vector-length`, `vector-empty-p`, `vectorp` | consultas |
| `vector-clear`, `vector-to-list`, `list-to-vector` | vaciar y convertir |

Las funciones que modifican el vector lo devuelven; las que quitan un elemento
devuelven ese elemento. Los vectores son datos de ALMA y no se pueden emitir
como C.

## Ejemplos

En `examples/`:

| Archivo | Contenido |
|---|---|
| `01_hola_mundo.alma` | Programa mínimo |
| `02_operadores.alma` | Operadores, asignaciones, `cast`, `sizeof` |
| `03_control.alma` | `if`, bucles, `switch`, `break`, `continue`, `goto` |
| `04_funciones.alma` | Declaraciones, recursión, punteros a función |
| `05_punteros_y_arrays.alma` | Punteros, aritmética de punteros, arrays |
| `06_structs_y_unions.alma` | Structs anidados, listas enlazadas, unions |
| `07_biblioteca_estandar.alma` | `string.h`, `stdlib.h`, `math.h`... (necesita `-lm`) |
| `08_macros.alma` | Macros del preprocesador |
| `09_generar_codigo.alma` | Generar C desde ALMA con `emit` |
| `10_marcianitos.alma` | Un juego de marcianitos con SDL2 |

Cada ejemplo indica cómo compilarlo.

## Limitaciones

- No hay literales con decimales, hexadecimales ni de carácter.
- No hay `volatile`, `restrict`, inicializadores de arrays o structs, ni arrays
  o structs globales definidos.
- Un `enum` no se puede declarar sin definir sus constantes, como en C estándar.
- No se pueden incluir cabeceras que no sean de la biblioteca estándar: las
  funciones de otras bibliotecas se declaran con `function` sin cuerpo.
- Los nombres de tipo son de una sola palabra (`long`, pero no `unsigned long`).
- `0` no se acepta como puntero nulo: hay que usar `NULL`.

## Estructura del código

| Ruta | Contenido |
|---|---|
| `alma` | script que traduce con `alma2c` y compila con `gcc` |
| `src/main.cpp` | programa `alma2c` |
| `src/alma.cpp`, `src/reader.cpp` | intérprete y lector de ALMA |
| `src/object/` | objetos de ALMA: símbolos, listas, vectores, funciones, macros... |
| `src/interpiler/` | traducción a C: tipos, declaraciones, operadores, biblioteca estándar y emisión del código |
| `src/gc/` | recolector de basura |
| `src/interfaces/` | interfaces comunes de los objetos (evaluar, expandir, emitir...) |
| `examples/` | ejemplos |

## Licencia

MIT. Ver `LICENSE`, `AUTHORS` y `CONTRIBUTORS`.
