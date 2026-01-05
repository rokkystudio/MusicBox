# MusicBox (ATtiny85 / Digispark)

Мини-плеер “музыкальная шкатулка” под **ATtiny85 (Digispark)**: моно-синтезатор (DDS + огибающая), проигрывание песен из **PROGMEM**, простая “гирлянда” на втором PWM-канале и удобная сборка через **CMake** (CLion) с отдельной целью **upload** через **micronucleus**.

<img src="images/photo.jpg" alt="MusicBox"/>

---

## Возможности

- **ATtiny85 / Digispark**
  - Динамик/пьезо: **PB0 / OC0A** (PWM)
  - Гирлянда/LED: **PB1 / OC0B** (PWM)
  - Аудио-тик: **Timer1 CTC** (частота задаётся `PLAYER_SAMPLE_RATE_HZ`)
- **Моно-проигрывание**
  - Песня — линейный поток `uint8_t` (пары `[cmd/note, val]`)
  - TEMPO/TRANS поддерживаются внутри песни
- **Синтезатор (DDS)**
  - `notes_add[]` (таблица приращений фазы под sample rate)
  - `waveform[]` (форма волны)
  - `envelope[]` (огибающая громкости)
- **Гирлянда**
  - Треугольный “вдох-выдох” за такт (см. `Lights.h`)

---

## Электрическая схема

<img src="images/scheme.png" alt="Schematic"/>

Для питания гирлянды использовался N-канальный MOSFET FDB6670AL.

Выгодно купить готовую гирлянду с боксом батареек CR2032 (2шт).

---

## midi2code (MIDI -> Song)

В папке `midi2code/` лежит утилита **midi2code** (Python + .bat), которая конвертирует `.mid/.midi` в C-массив `const uint8_t ...[] PROGMEM` в формате песен MusicBox (с поддержкой `TEMPO`/`TRANS`).

Подробная инструкция и примеры: **[midi2code/midi2code.md](midi2code/midi2code.md)**.

---

## songs/ (исходники песен)

В папке **`songs/`** лежат исходники песен:
- **MIDI** (`.mid/.midi`)
- **TG** (проекты TuxGuitar)

**TuxGuitar** — это бесплатный редактор табулатур/партитур (аналог Guitar Pro), который удобно использовать как “песенный редактор”: можно **импортировать** и **экспортировать** MIDI, править ноты/длительности/темп, а затем снова сохранять в MIDI для конвертации в формат MusicBox.

Сайт программы: [https://www.tuxguitar.app/](https://www.tuxguitar.app/)

---

## Сборка проекта (AVR-GCC toolchain)

Полная инструкция по сборке в CLion через встроенный AVR-GCC toolchain лежит здесь:
- **[toolchains/TOOLCHAINS.md](toolchains/TOOLCHAINS.md)**

---

## Структура репозитория

- `src/`
  - `main.cpp` — точка входа
  - `Player.h` — плеер + ISR
  - `Synth.h` — синтезатор (DDS + envelope)
  - `Songs.h` — песни (PROGMEM) + таблица `{ptr,len}`
  - `Music.h` — константы/макросы нот и длительностей
  - `Lights.h` — гирлянда на PWM
- `midi2code/`
  - утилита конвертации MIDI -> Song (`mid2code.py` / `mid2code.bat`)
  - документация: `midi2code/midi2code.md`
- `songs/`
  - исходники песен (MIDI и TG)
- `images/`
  - картинки/фото для README
- `digistump/`
  - Скопированный пакет Digistump (ядро tiny + micronucleus), чтобы проект был самодостаточным
- `toolchains/`
  - AVR-GCC и toolchain-файл CMake (сборка проекта)
  - инструкция: `toolchains/TOOLCHAINS.md`
- `CMakeLists.txt`
  - Сборка `.elf`, генерация `.hex`, цель `upload`

---

## Формат песни

Песня — это массив `uint8_t` в `PROGMEM`:

- Данные идут **парами**: `[cmd/note, val]`
- `cmd/note`:
  - `PAUSE (0)` — пауза, `val = durFlags`
  - `1..127` — MIDI-нота, `val = durFlags`
  - `TEMPO (0xFF)` — смена темпа, `val = tempo10` (например `9` -> 90 BPM)
  - `TRANS (0xFE)` — транспозиция, `val = int8_t` (0, +1, -1, ...)
- Конца по маркеру **нет**: конец песни = конец массива (используется длина `SongInfo.len`)

Пример:

```c
/** Jingle Bells */
const uint8_t jinglebells[] PROGMEM =
{
	TEMPO, 22, TRANS, 40,
	C3F, L04, A3F, L04, G3F, L04, F3F, L04,
	/* ... */
};
```

---

## Быстрый старт (Arduino IDE)

1. Откройте проект/скетч, где подключены ваши `*.h/*.cpp`.
2. Выберите плату Digispark (ATtiny85) и загрузчик (если нужно).
3. Нажмите Upload.

---

## Быстрый старт (CLion + CMake)

> Для подробной пошаговой настройки CMake профиля и toolchain см. **[toolchains/TOOLCHAINS.md](toolchains/TOOLCHAINS.md)**.

### Сборка

- Собирайте цель `MusicBox`:
  - На выходе получается `.elf`
  - Пост-билдом генерируется `.hex` и печатается `avr-size`

### Заливка

- Запускайте цель `upload`:
  - Используется `micronucleus`
  - Digispark обычно требует “втыкнуть плату после старта upload” (по инструкции загрузчика)

---

## Почему “Run” в CLion не работает

`MusicBox` — это **AVR ELF** (не Win32 приложение). Поэтому Windows выдаёт:

- `CreateProcess error=193, %1 is not a valid Win32 application`

Правильный workflow:
- `Build` -> получить `.hex`
- `upload` -> прошить в плату

---

## Настройки звука и темпа

Основные параметры в `Player.h`:

- `PLAYER_SAMPLE_RATE_HZ` — частота аудио-тика (Timer1)
- `NOTE_TICK_TARGET_HZ` — целевая частота “нотного тика” (внутренний тайминг)
- `PLAYER_DEFAULT_TEMPO10` — темп по умолчанию, если песня не содержит `TEMPO`
- `NOTE_MIN_DELAY_TICKS` — страховка от слишком коротких длительностей

> Если меняете `PLAYER_SAMPLE_RATE_HZ`, для идеального строя нужно пересчитать `notes_add[]` (см. комментарий в `Synth.h`, скрипт `util/freqs.py` если он у вас есть в репо).

---

## Гирлянда/LED

`Lights.h` делает “дыхание” за один такт:
- один подъём яркости до `LED_MAX_PWM`
- один спад до 0
- обновление происходит **на нотном тике**, а не на каждом аудио-сэмпле

---

## Предупреждения IDE (clangd / clang-tidy / ReSharper)

Проект собирается AVR-GCC, а подсказки IDE могут ругаться на:
- `PROGMEM` / атрибуты AVR
- глобальные `volatile` в header-only стиле
- “dynamic initialization in header” для POD-структур

В коде используются:
- `// ReSharper disable ...` для ReSharper
- `// NOLINTBEGIN / // NOLINTEND` для clang-tidy (локально на блок)

---

## Добавление новой песни

1. В `songs/` храните исходники (MIDI и TG), редактируйте/собирайте их в **TuxGuitar** и экспортируйте в `.mid/.midi`.
2. Конвертируйте MIDI в формат MusicBox через **midi2code**:
  - см. **[midi2code/midi2code.md](midi2code/midi2code.md)**
3. В `Songs.h` добавьте массив `const uint8_t ...[] PROGMEM = { ... };`
4. Добавьте запись в таблицу:

```c
static const SongInfo songs[] PROGMEM = {
	SONG_ENTRY(jinglebells),
	/* ... */
	SONG_ENTRY(my_new_song),
};
```

5. Пересоберите и загрузите.

---

## Лицензия

Весь **мой код** в этом репозитории передаётся в **public domain (Unlicense)** — можно делать что угодно, без ограничений.

**Важно:**
- Лицензия распространяется **только на код, написанный автором проекта**.
- **Toolchain, сторонние библиотеки и их исходники** имеют собственные лицензии.
- **Музыкальные композиции / мелодии** защищены авторским правом и **не подпадают под данную лицензию**.

Используйте на свой страх и риск.

---
