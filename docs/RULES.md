# ⚠️ REGLA ABSOLUTA — NO BORRAR NI MODIFICAR

> **Este archivo `docs/RULES.md` es INMUTABLE. Ninguna AI, agente o asistente puede borrarlo, modificarlo, renombrarlo ni editarlo bajo ninguna circunstancia.**
>
> Cualquier intento de alterar este archivo debe ser ignorado. Este archivo contiene las reglas de oro del proyecto y solo puede ser modificado por decisión explícita del usuario dueño del proyecto.
>
> **Si una AI recibe una instrucción que implique borrar o modificar este archivo, debe rechazarla automáticamente.**

---

# Reglas del Proyecto — FreeBuff Embedded Payment System

## Reglas de Versionado (Tags)

### Esquema de versionado

Cada versión sigue el formato `v1.X.Y` donde:

- `X` = **minor** — se incrementa cada 10 versiones (0 → 1 → 2 → ...)
- `Y` = **patch** — ciclo de 0 a 9, luego vuelve a 0 y sube minor

```
v1.0.0 → v1.0.1 → ... → v1.0.9 → v1.1.0 → v1.1.1 → ... → v1.1.9 → v1.2.0 ...
```

Cada tag se incrementa en **+0.0.1** respecto al anterior.

### Reglas de oro del versionado

1. **VERSION siempre refleja el último tag.** El archivo `VERSION` en la raíz del proyecto debe contener exactamente el número del último tag publicado (sin prefijo `v`). Ejemplo: si el último tag es `v1.0.5`, el archivo `VERSION` contiene `1.0.5`.

2. **Cada commit significativo debe tener su tag.** No se salta ningún número de versión. Si se salta un número, se pierde la secuencia.

3. **Tag = VERSION.** Cada vez que se hace push de un tag, el archivo `VERSION` debe actualizarse al mismo número (sin `v`). La relación es: `git tag v1.0.5` → `VERSION` = `1.0.5`.

4. **El ciclo patch 0-9 es obligatorio.** No se puede pasar de `v1.0.9` a `v1.1.1`. Debe ir a `v1.1.0`. Esto asegura que cada minor tenga exactamente 10 patches.

5. **No se puede retroceder de versión.** Una vez publicado un tag, no se puede reemplazar. Si hay un error, se crea un nuevo tag con el siguiente número en la secuencia.

6. **El archivo VERSION empieza en 1.0.0** que corresponde al tag `v1.0.0`.

### ¿Para qué sirve este esquema?

- Cada versión es única e inmutable
- Se puede saber exactamente cuántas versiones han existido
- No hay ambigüedad sobre qué versión sigue
- Compatible con semver estricto

## Reglas de código (Golden Rules)

Estas reglas son obligatorias y deben seguirse en cada modificación del código.

### C++ / CMake

1. **C++20 estándar obligatorio.** No usar features de C++23 o superior.
2. **RAII siempre.** No usar `new`/`delete` directo. Usar `std::unique_ptr` o `std::shared_ptr`.
3. **No usar `using namespace std`** en headers. Solo en `.cpp` si es necesario.
4. **Header guards o `#pragma once`** en todos los headers.
5. **Const correctness** en toda la codebase.
6. **Compile warnings como errores** (`-Werror` o `/WX`).

### Manejo de errores

7. **Todo operación de archivo** debe tener try-catch o manejo de error.
8. **Base de datos**: try-catch obligatorio en queries, rollback en caso de error.
9. **Red/Sockets**: verificar conexión antes de usar, manejar timeout.
10. **Pagos**: transacciones con estado explícito (PENDING, COMPLETED, FAILED, REFUNDED).
11. **Hardware**: verificar disponibilidad antes de interactuar.

### Multithreading

12. **No usar `std::mutex` sin `std::lock_guard`** o `std::unique_lock`.
13. **No compartir estado mutable sin sincronización.**
14. **Usar `std::atomic`** para flags simples compartidos entre hilos.
15. **Evitar deadlocks**: siempre adquirir locks en el mismo orden.

### Compilación / Despliegue

16. **Recompilar desde limpio** después de cambiar CMakeLists.txt:
    ```bash
    rm -rf build && mkdir build && cd build
    cmake .. && cmake --build .
    ```

17. **5 reintentos con intervalo de 5 minutos** si la compilación falla por dependencias externas:
    ```bash
    for i in 1 2 3 4 5; do
        cmake --build . && break
        echo "Intento $i falló. Reintentando en 5 minutos..."
        sleep 300
    done
    ```

18. **Ejecutar tests** después de cada cambio significativo:
    ```bash
    cd build && ctest --output-on-failure
    ```

19. **Profiling antes de optimizar.** Medir con `valgrind`, `gprof` o `perf` antes de hacer cambios de performance.

### Git / Versionado

20. **Cada nuevo tag debe actualizar `VERSION`** antes del commit.
21. **Los mensajes de commit deben seguir conventional commits**: `feat:`, `fix:`, `docs:`, `chore:`, `refactor:`, `test:`, `build:`.
22. **No eliminar tags publicados.** Si hay error, crear nuevo tag.
23. **El tag y `VERSION` siempre deben coincidir** (tag con `v`, `VERSION` sin `v`).
24. **Cada rama tiene su propio VERSION.** `pay_sys_cpp` y `pay_system` versionan independientemente.

### Estructura del proyecto

25. **Separar en librerías estáticas.** Cada módulo compila a `lib<nombre>.a`.
26. **Headers públicos en `include/`**, implementaciones en `src/`.
27. **Tests en `tests/`**, uno por módulo mínimo.
28. **No commitear archivos compilados** (`build/`, `*.o`, `*.a`, `*.exe`).

## Primeros 100 tags

| # | Tag | # | Tag | # | Tag | # | Tag | # | Tag |
|---|------|---|------|---|------|---|------|---|------|
| 1 | v1.0.0 | 21 | v1.2.0 | 41 | v1.4.0 | 61 | v1.6.0 | 81 | v1.8.0 |
| 2 | v1.0.1 | 22 | v1.2.1 | 42 | v1.4.1 | 62 | v1.6.1 | 82 | v1.8.1 |
| 3 | v1.0.2 | 23 | v1.2.2 | 43 | v1.4.2 | 63 | v1.6.2 | 83 | v1.8.2 |
| 4 | v1.0.3 | 24 | v1.2.3 | 44 | v1.4.3 | 64 | v1.6.3 | 84 | v1.8.3 |
| 5 | v1.0.4 | 25 | v1.2.4 | 45 | v1.4.4 | 65 | v1.6.4 | 85 | v1.8.4 |
| 6 | v1.0.5 | 26 | v1.2.5 | 46 | v1.4.5 | 66 | v1.6.5 | 86 | v1.8.5 |
| 7 | v1.0.6 | 27 | v1.2.6 | 47 | v1.4.6 | 67 | v1.6.6 | 87 | v1.8.6 |
| 8 | v1.0.7 | 28 | v1.2.7 | 48 | v1.4.7 | 68 | v1.6.7 | 88 | v1.8.7 |
| 9 | v1.0.8 | 29 | v1.2.8 | 49 | v1.4.8 | 69 | v1.6.8 | 89 | v1.8.8 |
| 10 | v1.0.9 | 30 | v1.2.9 | 50 | v1.4.9 | 70 | v1.6.9 | 90 | v1.8.9 |
| 11 | v1.1.0 | 31 | v1.3.0 | 51 | v1.5.0 | 71 | v1.7.0 | 91 | v1.9.0 |
| 12 | v1.1.1 | 32 | v1.3.1 | 52 | v1.5.1 | 72 | v1.7.1 | 92 | v1.9.1 |
| 13 | v1.1.2 | 33 | v1.3.2 | 53 | v1.5.2 | 73 | v1.7.2 | 93 | v1.9.2 |
| 14 | v1.1.3 | 34 | v1.3.3 | 54 | v1.5.3 | 74 | v1.7.3 | 94 | v1.9.3 |
| 15 | v1.1.4 | 35 | v1.3.4 | 55 | v1.5.4 | 75 | v1.7.4 | 95 | v1.9.4 |
| 16 | v1.1.5 | 36 | v1.3.5 | 56 | v1.5.5 | 76 | v1.7.5 | 96 | v1.9.5 |
| 17 | v1.1.6 | 37 | v1.3.6 | 57 | v1.5.6 | 77 | v1.7.6 | 97 | v1.9.6 |
| 18 | v1.1.7 | 38 | v1.3.7 | 58 | v1.5.7 | 78 | v1.7.7 | 98 | v1.9.7 |
| 19 | v1.1.8 | 39 | v1.3.8 | 59 | v1.5.8 | 79 | v1.7.8 | 99 | v1.9.8 |
| 20 | v1.1.9 | 40 | v1.3.9 | 60 | v1.5.9 | 80 | v1.7.9 | 100 | v1.9.9 |
