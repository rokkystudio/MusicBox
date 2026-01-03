#pragma once

#include <Arduino.h>
#include <avr/pgmspace.h>

/**
 * @file Synth.h
 * Примитивный синтезатор для шкатулки (DDS + огибающая).
 *
 * Здесь лежит всё, что относится к формированию звука:
 *  - таблица приращений фазы notes_add[] (для MIDI нот)
 *  - waveform[] (один период)
 *  - envelope[] (огибающая громкости)
 *  - маленькие inline-функции для:
 *      * старта/тишины
 *      * noteOn (расчёт add по MIDI-ноте)
 *      * генерации одного PCM-сэмпла (0..255) для PWM
 *
 * Важно:
 *  - notes_add[] рассчитана под конкретную частоту аудио-тика (sample rate).
 *    Если меняешь sample rate в плеере — для идеального строя нужно пересчитать notes_add (util/freqs.py).
 */

// соответствует MIDI-ноте 21 (A0)
#define SYNTH_MIDI_BASE			21
#define SYNTH_NOTES_ADD_COUNT	100

//=====================================================================//
// Структура канала (моно DDS)
//=====================================================================//
typedef struct {
	uint16_t count;
	uint16_t add;
	uint16_t env_count;		// Q8.8 индекс огибающей: (env_count >> 8) -> 0..255
} Channel;

//=====================================================================//
// Таблицы (PROGMEM)
//=====================================================================//

// increment amount for different keys (piano key range) - see util/freqs.py
const uint16_t notes_add[SYNTH_NOTES_ADD_COUNT] PROGMEM = {
	75,  80,  84,  89,  95,  100,  106,  113,  119,  126,  134,  142,
	150,  159,  169,  179,  189,  200,  212,  225,  238,  253,  268,  284,
	300,  318,  337,  357,  378,  401,  425,  450,  477,  505,  535,  567,
	601,  636,  674,  714,  757,  802,  850,  900,  954,  1010,  1070,  1134,
	1201,  1273,  1349,  1429,  1514,  1604,  1699,  1800,  1907,  2021,  2141,  2268,
	2403,  2546,  2697,  2858,  3028,  3208,  3398,  3600,  3814,  4041,  4282,  4536,
	4806,  5092,  5395,  5715,  6055,  6415,  6797,  7201,  7629,  8083,  8563,  9072,
	9612,  10184,  10789,  11431,  12110,  12830,  13593,  14402,  15258,  16165,  17127,  18145,
	19224,  20367,  21578,  22861,
};

// one period of the note waveform - see util/sin.py // sin^2
const uint8_t waveform[64] PROGMEM = {
	128, 152, 173, 191, 207, 220, 230, 238, 244, 248, 251, 253, 254, 255, 255, 255,
	255, 255, 255, 255, 254, 253, 251, 248, 244, 238, 230, 220, 207, 191, 173, 152,
	128, 104, 83, 65, 49, 36, 26, 18, 12, 8, 5, 3, 2, 1, 1, 1,
	1, 1, 1, 1, 2, 3, 5, 8, 12, 18, 26, 36, 49, 65, 83, 104,
};

// envelope for note (scale amount)
// source: Roman Lut (http://www.deep-shadows.com/hax/wordpress/?page_id=1111)
const uint8_t envelope[128] PROGMEM = {
	0xFF, 0xFA, 0xF5, 0xF0, 0xEB, 0xE7, 0xE2, 0xDE, 0xD9, 0xD5, 0xD1, 0xCD, 0xC9, 0xC5, 0xC1, 0xBD,
	0xB9, 0xB6, 0xB2, 0xAE, 0xAB, 0xA8, 0xA4, 0xA1, 0x9E, 0x9B, 0x98, 0x95, 0x92, 0x8F, 0x8C, 0x89,
	0x86, 0x84, 0x81, 0x7F, 0x7C, 0x7A, 0x77, 0x75, 0x73, 0x70, 0x6E, 0x6C, 0x6A, 0x68, 0x66, 0x64,
	0x62, 0x60, 0x5E, 0x5C, 0x5A, 0x58, 0x57, 0x55, 0x53, 0x52, 0x50, 0x4E, 0x4D, 0x4B, 0x4A, 0x48,
	0x47, 0x45, 0x44, 0x43, 0x41, 0x40, 0x3F, 0x3E, 0x3C, 0x3B, 0x3A, 0x39, 0x38, 0x37, 0x36, 0x35,
	0x33, 0x32, 0x31, 0x30, 0x30, 0x2F, 0x2E, 0x2D, 0x2C, 0x2B, 0x2A, 0x29, 0x28, 0x27, 0x26, 0x25,
	0x23, 0x22, 0x21, 0x20, 0x1F, 0x1E, 0x1D, 0x1C, 0x1A, 0x19, 0x18, 0x17, 0x16, 0x15, 0x14, 0x13,
	0x11, 0x10, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x00, 0x00
};

//=====================================================================//
// Inline-методы формирования звука
//=====================================================================//

//---------------------------------------------------------------------//
// Полная тишина (глушим канал + огибающую за пределы диапазона)
//---------------------------------------------------------------------//
static inline void Synth_silence(volatile Channel &ch) {
	ch.add = 0;
	// env_index >= 128 -> out=0
	ch.env_count = static_cast<uint16_t>(128u << 8);
}

//---------------------------------------------------------------------//
// Инициализация канала (тишина)
//---------------------------------------------------------------------//
static inline void Synth_begin(volatile Channel &ch) {
	ch.count = 0;
	Synth_silence(ch);
}

//---------------------------------------------------------------------//
// Включить ноту по MIDI-номеру (1..127), сброс фазы и огибающей
//---------------------------------------------------------------------//
static inline void Synth_noteOn(volatile Channel &ch, uint8_t midiNote)
{
	// индекс таблицы = midi_note - 21 (A0)
	int16_t idx = static_cast<int16_t>(midiNote) - static_cast<int16_t>(SYNTH_MIDI_BASE);

	if (idx < 0) idx = 0;
	if (idx > static_cast<int16_t>((SYNTH_NOTES_ADD_COUNT - 1))) {
		idx = static_cast<int16_t>((SYNTH_NOTES_ADD_COUNT - 1));
	}

	uint16_t add = pgm_read_word(&notes_add[idx]);

	ch.add = add;
	ch.env_count = 0;
	ch.count = 0;
}

//---------------------------------------------------------------------//
// Сгенерировать один аудио-сэмпл (0..255) для PWM
//---------------------------------------------------------------------//
static inline uint8_t Synth_renderSample(volatile Channel &ch)
{
	// DDS: phase accumulator
	ch.count = static_cast<uint16_t>(ch.count + ch.add);

	auto env_index = static_cast<uint8_t>(ch.env_count >> 8);
	if (env_index >= 128) return 0;

	ch.env_count++;

	uint8_t wave_val = pgm_read_byte(&waveform[(ch.count >> 10) & 0x3F]);
	uint8_t env_val  = pgm_read_byte(&envelope[env_index]);

	return static_cast<uint8_t>(
		(static_cast<uint16_t>(wave_val) * static_cast<uint16_t>(env_val)) >> 8 & 0xFF
	);
}
