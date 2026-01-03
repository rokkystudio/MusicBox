#pragma once
// ReSharper disable CppVariableCanBeMadeConstexpr

/**
 * @file Player.h
 *
 * Плеер для музыкальной шкатулки под ATtiny85:
 *  - один динамик (PB0 -> GND) через Timer0 PWM (OCR0A)
 *  - гирлянда/LED (PB1 -> MOSFET -> GND) через Timer0 PWM (OCR0B)
 *  - Timer1 в CTC режиме даёт аудио-тик (частота задаётся одним #define),
 *    поверх которого программно делаем "нотные тики".
 *
 * ФОРМАТ ПЕСНИ (ЛИНЕЙНЫЙ uint8_t):
 *  - данные идут парами байт: [cmd/note, val]
 *  - cmd/note:
 *      PAUSE (0)    = пауза, val = durFlags
 *      1..127       = MIDI-нота, val = durFlags
 *      TEMPO (0xFF) = смена темпа, val = tempo10 (9->90 BPM)
 *      TRANS (0xFE) = транспозиция, val = int8_t (0, +1, -1, ...)
 *
 * Конец песни:
 *  - маркера нет, конец = конец массива (по длине SongInfo.len)
 *
 * ГИРЛЯНДА:
 *  - реализована в Lights.h (Player только дергает Lights_tick() и Lights_applyTempoTicksPer16()).
 *
 * Ошибки в данных:
 *  - неизвестные cmd (128..253) — игнорируем, звук не портим.
 */

#include <Arduino.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "Songs.h"
#include "Synth.h"
#include "Lights.h"	// гирлянда

/**
 * Аппаратные пины (Digispark / ATtiny85)
 */
#define PIN_SPEAKER				0	// PB0 -> Buzzer PWM (OC0A)
#define PIN_LIGHTS				1	// PB1 -> LED/Garland PWM (OC0B)

/**
 * Тайминги.
 *
 * Один #define с "явной частотой дискретизации" (аудио-тик Timer1).
 *
 * ВАЖНО:
 *  - для идеального строя notes_add[] должен быть рассчитан под реальную F_AUDIO
 *    (которая получается после округления OCR1C).
 */
#define PLAYER_SAMPLE_RATE_HZ	24000UL

/** Темп по умолчанию, если в песне нет TEMPO (tempo10=9 -> 90 BPM). */
#define PLAYER_DEFAULT_TEMPO10	9

/** Минимальная длительность ноты (страховка) в "нотных тиках". */
#define NOTE_MIN_DELAY_TICKS	4

/**
 * Timer1 (ATtiny85) - фиксированный прескалер
 *
 * Примечание:
 *  - AUDIO_PRESCALER_DIV используется в расчётах частоты, поэтому держим как макрос
 *    и проверяем на ноль на этапе препроцессора.
 */
#define AUDIO_PRESCALER_BITS	_BV(CS12)	// /8 (CS13..10 = 0b0100)
#define AUDIO_PRESCALER_DIV		8UL

#if (AUDIO_PRESCALER_DIV == 0) || (PLAYER_SAMPLE_RATE_HZ == 0)
	#error "AUDIO_PRESCALER_DIV and PLAYER_SAMPLE_RATE_HZ must be non-zero"
#endif

/** Цель для "нотных тиков" (примерно как было ~195 Гц). Это НЕ настройка пользователя. */
static const uint16_t NOTE_TICK_TARGET_HZ  = 196;

/**
 * Глобальное состояние плеера (МОНО)
 *
 * Примечание:
 *  - Это header-only проект, поэтому состояние держим здесь.
 *  - clang-tidy может ругаться на "dynamic initialization in header",
 *    хотя эти POD-и фактически статически обнуляются.
 *  - Глушим диагностикой NOLINT на блок.
 */
volatile Channel channel;			// NOLINT
volatile LightsState lights;		// NOLINT

/** Позиция в песне — БАЙТОВЫЙ индекс (0,2,4,...) в линейном массиве. */
volatile int16_t  song_pos            = -2;

/** Длина текущей песни (в байтах), всегда чётная: пары cmd, val. */
volatile uint16_t song_len            = 0;

/** Текущая задержка до следующего события (в "нотных тиках"). */
volatile uint16_t note_delay          = 1;

/** Индекс текущей песни. */
volatile uint8_t  song_index          = 0;

/** Текущая транспозиция (полутона), применяется к MIDI-нотам 1..127. */
volatile int8_t   song_transpose      = 0;

/** Делитель аудио-тиков до "нотного тика" (рассчитан из sample rate). */
volatile uint8_t  note_tick_div_top   = 1;

/** Счётчик делителя до "нотного тика". */
volatile uint8_t  note_tick_div_cnt   = 0;

/** Реальная частота "нотного тика" (для расчёта tempo->ticks). */
volatile uint16_t f_note_hz           = 0;

/** Темп текущей песни: сколько "нотных тиков" в 1/16. */
volatile uint8_t  song_ticks_per_16   = 1;

//=====================================================================//

/**
 * Настроить пины PB0/PB1 на выход.
 */
static inline void initPins() {
	// PB0 -> динамик (OC0A)
	// PB1 -> гирлянда (OC0B)
	DDRB |= _BV(PB0) | _BV(PB1);
}

/**
 * Настроить Timer0 на Fast PWM:
 *  - OC0A (PB0) = динамик
 *  - OC0B (PB1) = гирлянда
 */
static inline void initTimer0Pwm()
{
	// Timer0 — Fast PWM, OC0A (PB0), OC0B (PB1), без делителя
	TCCR0A = 0;
	TCCR0B = 0;

	TCCR0A |= _BV(COM0A1);
	TCCR0A |= _BV(COM0B1);

	TCCR0A |= _BV(WGM00) | _BV(WGM01);
	TCCR0B |= _BV(CS00);

	OCR0A = 0;
	OCR0B = 0;
}

/**
 * Настроить Timer1 на CTC для аудио-тиков и включить прерывание.
 *
 * Также рассчитывает:
 *  - note_tick_div_top (делитель до "нотного тика")
 *  - f_note_hz (реальную частоту "нотного тика")
 */
static inline void initTimer1Audio()
{
	// Timer1 — CTC, аудио тики
	// OCR1C+1 = round(F_CPU / (prescaler * sample_rate))
	uint32_t denom = static_cast<uint32_t>(AUDIO_PRESCALER_DIV) *
					 static_cast<uint32_t>(PLAYER_SAMPLE_RATE_HZ);

	uint32_t ocr1c_plus1 = (static_cast<uint32_t>(F_CPU) + (denom / 2UL)) / denom;	// округление
	if (ocr1c_plus1 < 1UL) {
		ocr1c_plus1 = 1UL;
	}
	if (ocr1c_plus1 > 256UL) {
		ocr1c_plus1 = 256UL;
	}

	const auto ocr = static_cast<uint8_t>(ocr1c_plus1 - 1UL);

	TCCR1 = 0;
	TCCR1 |= _BV(CTC1);
	TCCR1 |= AUDIO_PRESCALER_BITS;		// /8

	OCR1C = ocr;
	OCR1A = ocr;

	TIMSK |= _BV(OCIE1A);

	// Рассчитываем реальную F_AUDIO и делитель до "нотного тика"
	uint32_t f_audio_hz = static_cast<uint32_t>(F_CPU) /
		(static_cast<uint32_t>(AUDIO_PRESCALER_DIV) * ocr1c_plus1);

	uint32_t div = (f_audio_hz + (static_cast<uint32_t>(NOTE_TICK_TARGET_HZ) / 2UL)) /
		static_cast<uint32_t>(NOTE_TICK_TARGET_HZ);

	if (div < 1UL) div = 1UL;
	if (div > 255UL) div = 255UL;

	note_tick_div_top = static_cast<uint8_t>(div);

	uint32_t fn = f_audio_hz / static_cast<uint32_t>(note_tick_div_top);

	if (fn < 1UL) fn = 1UL;
	if (fn > 65535UL) fn = 65535UL;

	f_note_hz = static_cast<uint16_t>(fn);
}

/**
 * Применить ticksPer16 (только для плеера) + отдать в Lights.
 *
 * @param ticksPer16 Сколько "нотных тиков" приходится на 1/16.
 */
static inline void applyTempoTicksPer16(uint8_t ticksPer16)
{
	if (ticksPer16 == 0) {
		ticksPer16 = 1;
	}

	song_ticks_per_16 = ticksPer16;
	Lights_applyTempoTicksPer16(lights, song_ticks_per_16);
}

/**
 * Применить tempo10 (9->90 BPM) к текущей песне.
 *
 * Формула:
 *  - ticksPer16 = round( (F_NOTE_HZ * 15) / BPM )
 *
 * @param tempo10 Темп в десятках BPM (9 -> 90 BPM).
 */
static inline void applyTempo10(uint8_t tempo10)
{
	if (tempo10 == 0) {
		tempo10 = static_cast<uint8_t>(PLAYER_DEFAULT_TEMPO10);
	}

	uint16_t bpm = static_cast<uint16_t>(tempo10) * 10u;
	if (bpm < 20u) {
		bpm = 20u;
	}

	uint16_t noteHz = f_note_hz;
	if (noteHz == 0) {
		noteHz = NOTE_TICK_TARGET_HZ;
	}

	uint32_t num = static_cast<uint32_t>(noteHz) * 15UL;
	num += bpm / 2u;		// округление
	uint32_t t = num / static_cast<uint32_t>(bpm);

	if (t < 1UL) {
		t = 1UL;
	}
	if (t > 255UL) {
		t = 255UL;
	}

	applyTempoTicksPer16(static_cast<uint8_t>(t));
}

/**
 * Преобразование durFlags -> note_delay (в тиках F_NOTE).
 *
 * @param durFlags Длительность в формате Music.h (DUR_MASK_16_COUNT).
 * @return Количество "нотных тиков", минимум NOTE_MIN_DELAY_TICKS.
 */
static inline uint16_t durationToTicks(const uint8_t durFlags)
{
	auto len16 = static_cast<uint8_t>(durFlags & DUR_MASK_16_COUNT);

	// len16 == 0 трактуем как целую ноту (16/16)
	if (len16 == 0) {
		len16 = 16;
	}

	uint16_t ticks = static_cast<uint16_t>(len16) * static_cast<uint16_t>(song_ticks_per_16);

	if (ticks < NOTE_MIN_DELAY_TICKS) {
		ticks = NOTE_MIN_DELAY_TICKS;
	}

	return ticks;
}

/**
 * Перейти на следующую песню (внутри ISR).
 *
 * Примечание:
 *  - Внутри ISR нельзя долго думать: всё делаем просто и быстро.
 */
static inline void nextSongInternal()
{
	song_pos = -2;
	note_delay = 200;
	song_transpose = 0;

	uint8_t idx = song_index;
	idx++;

	if (idx >= static_cast<uint8_t>(NUM_SONGS)) {
		idx = 0;
	}

	song_index = idx;
	song_len = pgm_read_word(&songs[song_index].len);

	applyTempo10(0);
	Synth_silence(channel);
	Lights_reset(lights);
}

/**
 * ISR: записать один аудио-сэмпл в PWM (OCR0A).
 */
static inline void isrRenderAudioSample() {
	OCR0A = Synth_renderSample(channel);
}

/**
 * ISR: один "нотный тик":
 *  - обновить гирлянда
 *  - уменьшить задержку
 *  - при нуле задержки: разобрать следующий cmd/val из песни
 */
static inline void isrNoteTick()
{
	note_tick_div_cnt++;
	if (note_tick_div_cnt < note_tick_div_top) {
		return;
	}
	note_tick_div_cnt = 0;

	Lights_tick(lights);

	if (note_delay > 0) {
		note_delay--;
	}

	if (note_delay != 0) {
		return;
	}

	const auto *song = static_cast<const uint8_t*>(pgm_read_ptr(&songs[song_index].data));
	uint16_t len = song_len;

	// Обрабатываем TEMPO/TRANS и мусор подряд без задержки
	for (uint8_t guard = 0; guard < 64; guard++)
	{
		auto nextPos = static_cast<int16_t>(song_pos + 2);

		// конец песни = конец массива
		if (nextPos < 0 || len < 2u || static_cast<uint16_t>(nextPos + 1) >= len) {
			nextSongInternal();
			return;
		}

		song_pos = nextPos;

		uint8_t cmd = pgm_read_byte(&song[song_pos]);
		uint8_t val = pgm_read_byte(&song[song_pos + 1]);

		// TEMPO, tempo10
		if (cmd == static_cast<uint8_t>(TEMPO)) {
			applyTempo10(val);
			continue;
		}

		// TRANS, int8_t
		if (cmd == static_cast<uint8_t>(TRANS)) {
			song_transpose = static_cast<int8_t>(val);
			continue;
		}

		// PAUSE, durFlags
		if (cmd == static_cast<uint8_t>(PAUSE)) {
			note_delay = durationToTicks(val);
			Synth_silence(channel);
			break;
		}

		// нота: 1..127
		if (cmd <= 127) {
			note_delay = durationToTicks(val);

			int16_t nn = static_cast<int16_t>(cmd) + static_cast<int16_t>(song_transpose);
			if (nn < 1) {
				nn = 1;
			}
			if (nn > 127) {
				nn = 127;
			}

			Synth_noteOn(channel, static_cast<uint8_t>(nn));
			break;
		}

		// неизвестный cmd (128..253): просто пропускаем пару
	}

	// Если подряд попался только TEMPO/TRANS/мусор, чтобы не зависнуть — даём короткую тишину.
	if (note_delay == 0) {
		note_delay = NOTE_MIN_DELAY_TICKS;
		Synth_silence(channel);
	}
}

//=====================================================================//

/**
 * Player
 * Статический "класс-фасад" над глобальным состоянием плеера.
 */
class Player
{
  public:
	/** Инициализация плеера. */
	static void begin();

	/** Выбрать песню по индексу. */
	static void setSong(uint8_t index);

	/** Переключить на следующую песню. */
	static void nextSong();

	/** Переключить на предыдущую песню. */
	static void prevSong();
};

//=====================================================================//

/**
 * Инициализация:
 *  - пины
 *  - синтезатор и гирлянда
 *  - Timer0 PWM и Timer1 аудио-тик
 *  - дефолтный темп
 */
inline void Player::begin()
{
	initPins();

	// DDS (моноканал)
	Synth_begin(channel);

	song_pos          = -2;
	song_len          = pgm_read_word(&songs[0].len);
	note_delay        = 1;
	song_index        = 0;
	song_transpose    = 0;

	note_tick_div_top = 1;
	note_tick_div_cnt = 0;
	f_note_hz         = 0;

	// гирлянда
	Lights_begin(lights);

	initTimer0Pwm();
	initTimer1Audio();

	// темп по умолчанию (если песня не задаёт TEMPO)
	applyTempo10(0);

	sei();
}

/**
 * Выбрать песню по индексу.
 *
 * @param index Индекс (0..NUM_SONGS-1). Если вышли за границы — берём 0.
 */
inline void Player::setSong(uint8_t index)
{
	if (index >= static_cast<uint8_t>(NUM_SONGS)) {
		index = 0;
	}

	cli();

	song_index        = index;
	song_pos          = -2;
	song_len          = pgm_read_word(&songs[song_index].len);
	note_delay        = 1;
	note_tick_div_cnt = 0;

	// дефолт, песня переопределит TEMPO/TRANS сама
	song_transpose = 0;
	applyTempo10(0);

	// синхронизация гирлянды с началом песни
	Lights_reset(lights);

	// глушим канал до первой ноты
	Synth_silence(channel);

	sei();
}

/**
 * Переключить на следующую песню.
 */
inline void Player::nextSong()
{
	uint8_t idx = song_index;
	idx++;

	if (idx >= static_cast<uint8_t>(NUM_SONGS)) {
		idx = 0;
	}

	setSong(idx);
}

/**
 * Переключить на предыдущую песню.
 */
inline void Player::prevSong()
{
	uint8_t idx = song_index;

	if (idx == 0) {
		idx = static_cast<uint8_t>(NUM_SONGS) - 1;
	} else {
		idx--;
	}

	setSong(idx);
}

//=====================================================================//

/**
 * Прерывание Timer1 — МОНО аудио + авто-плеер + гирлянда
 */
ISR(TIM1_COMPA_vect)
{
	// Аудио-сэмпл (DDS + огибающая)
	isrRenderAudioSample();

	// Нотный тик + гирлянда + проигрывание
	isrNoteTick();
}
