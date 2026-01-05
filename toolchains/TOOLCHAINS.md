# TOOLCHAINS.md

Инструкция по сборке **MusicBox (AVR/Digispark ATtiny85)** в CLion.
AVR тулчейн уже лежит внутри проекта.

- AVR GCC: `toolchains/avr-gcc/7.3.0-atmel3.6.1-arduino7/`
- Digistump core/variants/tools: `digistump/`
- CMake toolchain: `toolchain-avr.cmake` (использует avr-gcc из папки выше)

---

## 1) Настройка CMake профиля в CLion
**Settings → Build, Execution, Deployment → CMake**

Создай профиль **AVR**:
- **Build directory:** `cmake-build-avr`
- **CMake options:**
    - `-DCMAKE_TOOLCHAIN_FILE=toolchain-avr.cmake`

Важно:
- В поле **Toolchain** в этом профиле выбери **MinGW** (или любой не-MSVC host toolchain).
    - Это нужно, чтобы CLion не ругался на несовместимость, т.к. CMake будет использовать GNU (`avr-gcc`).

Нажми **Reload CMake Project**.

---

## 2) Быстрая проверка (что реально выбран avr-gcc)
В CMake output должно быть:
- `The C compiler identification is GNU 7.3.0`
- `.../toolchains/avr-gcc/7.3.0-atmel3.6.1-arduino7/bin/avr-gcc.exe`
- `.../bin/avr-g++.exe`

Если так — всё ок.

---

## 3) Сборка
Выбери профиль **AVR** и собери таргет `MusicBox`.
Артефакты лежат в `cmake-build-avr/`:
- `MusicBox.elf`
- `MusicBox.hex` (если включена генерация через `avr-objcopy` в CMakeLists.txt)

---

## Типовая проблема: “GCC used by CMake is not compatible with Visual Studio”
Причина: в профиле выбран **Visual Studio**, а CMake через `toolchain-avr.cmake` включает **GNU**.

Решение: в профиле **AVR** переключить **Toolchain** на **MinGW** (или другой не-MSVC), затем **Reload**.

---
