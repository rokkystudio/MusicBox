#pragma once

#include <Arduino.h>
#include <avr/pgmspace.h>

#include "Music.h"

/**
 * @file Songs.h
 *
 * Хранилище песен в PROGMEM.
 *
 * ФОРМАТ ПЕСНИ (линейный поток uint8_t):
 *  - данные идут парами байт: [cmd/note, val]
 *      cmd0, val0, cmd1, val1, cmd2, val2, ...
 *
 * cmd/note:
 *  - PAUSE (0)    : пауза, val = durFlags
 *  - 1..127       : MIDI-нота (используй макросы C4F / C4D и т.д. из Music.h), val = durFlags
 *  - TEMPO (0xFF) : смена темпа, val = tempo10 (9 -> 90 BPM)
 *  - TRANS (0xFE) : транспозиция, val = int8_t (0, +1, -1, ...)
 *
 * Конец песни:
 *  - маркера нет, конец = конец массива (по длине)
 *
 * Примечание:
 *  - Пунктирные длительности храним отдельными константами: L8D/L4D/L2D/L1D.
 *  - Если в данных встретится неизвестный cmd (128..253), плеер по договорённости
 *    должен игнорировать это, не падая.
 */

//=====================================================================//

/**
 * Метаданные песни (указатель + длина).
 *
 * Таблица songs[] хранится в PROGMEM, поэтому:
 *  - data указывает на массив uint8_t в PROGMEM
 *  - len — размер массива в байтах
 */
typedef struct {
	const uint8_t *data;
	uint16_t len;
} SongInfo;

/** Макрос для записи песни в таблицу songs[]. */
#define SONG_ENTRY(x) { x, (uint16_t)sizeof(x) }


//=====================================================================//
// James Lord Pierpont - Jingle Bells
//=====================================================================//
const uint8_t jinglebells[] PROGMEM =
{
	TEMPO, 22, TRANS, 40,	// ~220 BPM
	C3F, L04, A3F, L04, G3F, L04, F3F, L04,	// такт 1
	C3F, L2D, C3F, L08, C3F, L08,	// такт 2
	C3F, L04, A3F, L04, G3F, L04, F3F, L04,	// такт 3
	D3F, L2D, PAUSE, L04,	// такт 4

	D3F, L04, A3D, L04, A3F, L04, G3F, L04,	// такт 5
	E3F, L2D, PAUSE, L04,	// такт 6
	C4F, L04, C4F, L04, A3D, L04, G3F, L04,	// такт 7
	A3F, L2D, PAUSE, L04,	// такт 8

	C3F, L04, A3F, L04, G3F, L04, F3F, L04,	// такт 9
	C3F, L2D, PAUSE, L04,	// такт 10
	C3F, L04, A3F, L04, G3F, L04, F3F, L04,	// такт 11
	D3F, L2D, D3F, L04,	// такт 12

	D3F, L04, A3D, L04, A3F, L04, G3F, L04,	// такт 13
	C4F, L04, C4F, L04, C4F, L04, C4F, L08, C4F, L08,	// такт 14
	D4F, L04, C4F, L04, A3D, L04, G3F, L04,	// такт 15
	F3F, L02, C4F, L02,	// такт 16

	A3F, L04, A3F, L04, A3F, L02,	// такт 17
	A3F, L04, A3F, L04, A3F, L02,	// такт 18
	A3F, L04, C4F, L04, F3F, L4D, G3F, L08,	// такт 19
	A3F, L01,	// такт 20

	A3D, L04, A3D, L04, A3D, L4D, A3D, L08,	// такт 21
	A3D, L04, A3F, L04, A3F, L04, A3F, L08, A3F, L08,	// такт 22
	A3F, L04, G3F, L04, G3F, L04, A3F, L04,	// такт 23
	G3F, L02, C4F, L02,	// такт 24

	A3F, L04, A3F, L04, A3F, L02,	// такт 25
	A3F, L04, A3F, L04, A3F, L02,	// такт 26
	A3F, L04, C4F, L04, F3F, L4D, G3F, L08,	// такт 27
	A3F, L01,	// такт 28

	A3D, L04, A3D, L04, A3D, L4D, A3D, L08,	// такт 29
	A3D, L04, A3F, L04, A3F, L04, A3F, L08, A3F, L08,	// такт 30
	C4F, L04, C4F, L04, A3D, L04, G3F, L04,	// такт 31
	F3F, L01,	// такт 32
};

//=====================================================================//
// Mamoru Fujisawa- Totoro - A Huge Tree
//=====================================================================//
const uint8_t totoro[] PROGMEM =
{
	TEMPO, 9, TRANS, 30,	// ~90 BPM
	G2D, L16, C4F, L16, A3D, L16, G3F, L16, C4F, L08, A3D, L16, G3F, L16,
	D3D, L16, C4F, L16, A3D, L16, G3F, L16, C4F, L08, A3D, L16, G3F, L16,	// такт 1

	A2D, L16, A3D, L16, F3F, L16, D3D, L16, A3D, L08, F3F, L16, D3D, L16,
	F3F, L16, A3D, L16, F3F, L16, D3D, L16, A3D, L08, F3F, L16, D3D, L16,	// такт 2

	G3D, L16, C5F, L16, A4D, L16, G4F, L16, C5F, L08, A4D, L16, G4F, L16,
	G3D, L16, C5F, L16, A4D, L16, G4F, L16, C5F, L08, A4D, L16, D4F, L16,	// такт 3

	PAUSE, L16, A4D, L16, F4F, L16, D4D, L16, A4D, L08, F4F, L16, D4D, L16,
	A4D, L08, PAUSE, L08, C4F, L08, D4D, L08,	// такт 4

	F4F, L08, F3F, L08, F4F, L08, G4F, L08, D4D, L08, F3F, L08, C4F, L08, D4D, L08,	// такт 5
	F4F, L08, D3D, L08, F4F, L08, A4D, L08, G4F, L08, D3D, L08, G4F, L08, A4D, L08,	// такт 6
	C5F, L08, C3F, L08, C5F, L08, D5D, L08, D5F, L08, C5F, L08, A4D, L08, G4D, L08,	// такт 7
	G4F, L08, D3F, L08, F4F, L04, G4F, L08, G3F, L08, C4F, L04,	// такт 8

	F4F, L08, C3F, L08, F4F, L08, G4F, L08, D4D, L08, C3F, L08, C4F, L08, D4D, L08,	// такт 9
	F4F, L08, D3D, L08, F4F, L08, A4D, L08, G4F, L08, D3D, L08, G4F, L08, A4D, L08,	// такт 10
	C5F, L08, C3F, L08, C5F, L08, D5D, L08, D5F, L08, C5F, L08, G4F, L08, C4F, L08,	// такт 11
	D4D, L08, G3F, L08, A3D, L08, C4F, L04, D4D, L08, D4F, L08, C4D, L08,	// такт 12

	G4D, L08, G4D, L08, C4D, L08, A2D, L08, F3F, L08, A3D, L08, C4D, L08, A2D, L08,	// такт 13
	C4D, L08, A3D, L08, G4D, L08, G4D, L08, F3F, L08, G4F, L08, F4F, L08, G4F, L08,	// такт 14
	G4D, L08, G4F, L08, F4F, L08, D4D, L08, A3D, L08, G3F, L08, C3F, L08, G3F, L08,	// такт 15
	C4F, L08, D4D, L08, G4F, L08, D4D, L04, D4F, L04, A2D, L08,	// такт 16

	G4D, L08, G4D, L08, C4D, L08, A2D, L08, F3F, L08, A3D, L08, C4D, L08, A2D, L08,	// такт 17
	C4D, L08, A3D, L08, G4D, L08, G4D, L08, F3F, L08, G4F, L08, F4F, L08, G4F, L08,	// такт 18
	G4D, L08, A4D, L08, D4D, L08, D3D, L08, C4F, L08, D3D, L08, A3D, L08, C4F, L08,	// такт 19
	A3D, L02, PAUSE, L04, C4F, L08, D4D, L08,	// такт 20

	F4F, L08, F3F, L08, F4F, L08, G4F, L08, D4D, L08, F3F, L08, C4F, L08, D4D, L08,	// такт 21
	F4F, L08, D3D, L08, F4F, L08, A4D, L08, G4F, L08, D3D, L08, G4F, L08, A4D, L08,	// такт 22
	C5F, L08, C3F, L08, C5F, L08, D5D, L08, D5F, L08, C5F, L08, A4D, L08, G4D, L08,	// такт 23
	G4F, L08, D3F, L08, F4F, L04, G4F, L08, G3F, L08, C4F, L08, D4D, L08,	// такт 24

	F4F, L08, C3F, L08, F4F, L08, G4F, L08, D4D, L08, C3F, L08, C4F, L08, D4D, L08,	// такт 25
	F4F, L08, D3D, L08, F4F, L08, A4D, L08, G4F, L08, D3D, L08, G4F, L08, A4D, L08,	// такт 26
	C5F, L08, C5F, L08, C5F, L08, D5D, L08, D5F, L08, C5F, L08, G4F, L08, C4F, L08,	// такт 27
	D4D, L08, G3F, L08, A3D, L08, C4F, L08, C4F, L08,	// такт 28
};

//=====================================================================//
// Daniel Rosenfeld - Minecraft - Wet Hands
//=====================================================================//
const uint8_t minecraft[] PROGMEM =
{
	TEMPO, 10, TRANS, 30,	// ~100 BPM
	A2F, L08, E3F, L08, A3F, L08, B3F, L08, C4D, L08, B3F, L08, A3F, L08, E3F, L08,	// такт 1
	D3F, L08, A3F, L08, C4D, L08, E4F, L08, C4D, L08, A3F, L08, PAUSE, L04,	// такт 2
	A2F, L08, E3F, L08, A3F, L08, B3F, L08, C4D, L08, B3F, L08, A3F, L08, E3F, L08,	// такт 3
	D3F, L08, A3F, L08, C4D, L08, E4F, L08, C4D, L08, A3F, L08, PAUSE, L04,	// такт 4
	G4D, L08, E3F, L08, A3F, L08, E3F, L08, A2F, L08, E3F, L08, A4F, L08, A3F, L08,	// такт 5
	F4D, L08, A3F, L08, C4D, L08, A3F, L08, D3F, L08, A3F, L08, E4F, L08, F4D, L08,	// такт 6
	G4D, L08, E3F, L08, A3F, L08, E3F, L08, A2F, L08, E3F, L08, B3F, L08, C4D, L08,	// такт 7
	D3F, L08, A3F, L08, C4D, L08, A3F, L08, D3F, L08, A3F, L08, C4D, L08, E4F, L08,	// такт 8
	G4F, L08, D3F, L08, G3F, L08, F4D, L08, D4F, L08, G3F, L08, A3F, L08, B3F, L08,	// такт 9
	G2F, L08, D3F, L08, A3F, L08, D3F, L08, B3F, L02,	// такт 10
	G4F, L08, D3F, L08, F4D, L08, G3F, L08, D4F, L08, G3F, L08, A3F, L08, B3F, L08,	// такт 11
	G2F, L08, D3F, L08, A3F, L08, D3F, L08, G2F, L08, D3F, L08, C4D, L04,	// такт 12
	E3F, L08, A3F, L08, B3F, L08, C4D, L08, E4F, L08, C4D, L08, B3F, L08, A3F, L08,	// такт 13
	A2F, L08, E3F, L08, A3F, L08, B3F, L08, C4D, L08, B3F, L08, A3F, L08, E3F, L08,	// такт 14
	D4F, L08, F3D, L08, B3F, L08, C4D, L08, A3F, L08, F3D, L08, E4F, L08, F4D, L08,	// такт 15
	B2F, L08, D4F, L08, F3D, L08, B3F, L08, F3D, L08, B3F, L08, B3F, L08, C4D, L08,	// такт 16
	D4F, L08, D3F, L08, C4D, L08, D4F, L08, G2F, L08, F4D, L08, D3F, L08, D4F, L08,	// такт 17
	C4D, L02, B3F, L04, PAUSE, L04,	// такт 18
	E2F, L08, B2F, L08, E3F, L08, G3D, L08, B3F, L08, G3D, L08, E3F, L08, B2F, L08,	// такт 19
	E2F, L08, B2F, L08, E3F, L08, G3D, L08, B3F, L08, G3D, L08, B2F, L04,	// такт 20
	G4F, L08, F4D, L08, E4F, L08, D4F, L08, E4F, L08, D4F, L08, E4F, L08, F4D, L08,	// такт 21
	A2F, L08, E4F, L08, E3F, L08, A3F, L08, A4F, L08, C4D, L08, A3F, L08, E3F, L08,	// такт 22
	G4D, L08, E4F, L08, B3F, L08, G3D, L08, E3F, L02,	// такт 23
	B3F, L08, G3D, L08, E3F, L08, B2F, L08, E3F, L02,	// такт 24
	E2F, L08, E3F, L08, G3D, L08, E4F, L08, B3F, L08, G3D, L08, PAUSE, L04,	// такт 25
	E2F, L08, E3F, L08, G3D, L08, E4F, L08, B3F, L08, G3D, L08,	// такт 26
};

//=====================================================================//
// Thomas Oliphant - Deck the Halls
//=====================================================================//
const uint8_t deckhalls[] PROGMEM =
{
	TEMPO, 18, TRANS, 20,	// ~180 BPM
	C5F, L4D, A4D, L08, A4F, L04, G4F, L04,	// такт 1
	F4F, L04, G4F, L04, A4F, L04, F4F, L04,	// такт 2
	G4F, L08, A4F, L08, A4D, L08, G4F, L08, A4F, L4D, G4F, L08,	// такт 3
	F4F, L04, E4F, L04, F4F, L02,	// такт 4
	C5F, L4D, A4D, L08, A4F, L04, G4F, L04,	// такт 5
	F4F, L04, G4F, L04, A4F, L04, F4F, L04,	// такт 6
	G4F, L08, A4F, L08, A4D, L08, G4F, L08, A4F, L4D, G4F, L08,	// такт 7
	F4F, L04, E4F, L04, F4F, L02,	// такт 8
	G4F, L4D, A4F, L08, A4D, L04, G4F, L04,	// такт 9
	A4F, L4D, A4D, L08, C5F, L04, G4F, L04,	// такт 10
	A4F, L08, B4F, L08, C5F, L04, D5F, L08, E5F, L08, F5F, L04,	// такт 11
	E5F, L04, D5F, L04, C5F, L02,	// такт 12
	C5F, L4D, A4D, L08, A4F, L04, G4F, L04,	// такт 13
	F4F, L04, G4F, L04, A4F, L04, F4F, L04,	// такт 14
	D5F, L04, D5F, L04, C5F, L4D, A4D, L08,	// такт 15
	A4F, L04, G4F, L04, F4F, L02,	// такт 16

	C5F, L4D, A4D, L08, A4F, L04, G4F, L04,	// такт 1
	F4F, L04, G4F, L04, A4F, L04, F4F, L04,	// такт 2
	G4F, L08, A4F, L08, A4D, L08, G4F, L08, A4F, L4D, G4F, L08,	// такт 3
	F4F, L04, E4F, L04, F4F, L02,	// такт 4
	C5F, L4D, A4D, L08, A4F, L04, G4F, L04,	// такт 5
	F4F, L04, G4F, L04, A4F, L04, F4F, L04,	// такт 6
	G4F, L08, A4F, L08, A4D, L08, G4F, L08, A4F, L4D, G4F, L08,	// такт 7
	F4F, L04, E4F, L04, F4F, L02,	// такт 8
	G4F, L4D, A4F, L08, A4D, L04, G4F, L04,	// такт 9
	A4F, L4D, A4D, L08, C5F, L04, G4F, L04,	// такт 10
	A4F, L08, B4F, L08, C5F, L04, D5F, L08, E5F, L08, F5F, L04,	// такт 11
	E5F, L04, D5F, L04, C5F, L02,	// такт 12
	C5F, L4D, A4D, L08, A4F, L04, G4F, L04,	// такт 13
	F4F, L04, G4F, L04, A4F, L04, F4F, L04,	// такт 14
	D5F, L04, D5F, L04, C5F, L4D, A4D, L08,	// такт 15
	A4F, L04, G4F, L04, F4F, L02,	// такт 16
};

//=====================================================================//
// John Williams - Somewhere In My Memory
//=====================================================================//
const uint8_t in_my_memory[] PROGMEM =
{
	TEMPO, 13, TRANS, 20,	// ~130 BPM
	D4F, L08, A4F, L08, F4D, L08, A4F, L08, A5F, L08, A4F, L08, F4D, L08, A4F, L08,	// такт 1
	D6F, L08, D4F, L08, G4F, L08, D4F, L08, A5F, L02,	// такт 2
	E5F, L08, B3F, L08, B5F, L08, B3F, L08, A5F, L08, A3F, L08, D5F, L08, G5F, L08,	// такт 3
	F5D, L08, B3F, L08, D4F, L08, B3F, L08, E5F, L08, G4F, L08, A3F, L04,	// такт 4
	A4F, L08, A3F, L08, F4D, L08, A3F, L08, A4F, L08, A3F, L08, F4D, L04,	// такт 5
	D5F, L08, D3F, L08, G3F, L08, D3F, L08, A4F, L04, A3F, L04,	// такт 6
	B4F, L08, D5F, L08, G4F, L08, D3F, L08, F4D, L08, A4F, L08, D4F, L04,	// такт 7
	F4D, L4D, G3F, L08, E4F, L02,	// такт 8
	A4F, L04, F4D, L08, A3F, L08, A4F, L04, F4D, L08, A3F, L08,	// такт 9
	D5F, L08, G3F, L08, B3F, L08, G3F, L08, A4F, L04, A3F, L04,	// такт 10
	E4F, L04, B4F, L04, A4F, L08, D3F, L08, D4F, L08, G4F, L08,	// такт 11
	F4D, L4D, G3F, L08, E4F, L02,	// такт 12
	A4F, L04, F4D, L08, A3F, L08, A4F, L04, F4D, L08, A3F, L08,	// такт 13
	D5F, L08, G3F, L08, B3F, L04, A4F, L04, A3F, L04,	// такт 14
	B4F, L08, D5F, L08, G4F, L04, F4D, L08, A4F, L08, D4F, L04,	// такт 15
	F4D, L4D, G3F, L08, E4F, L4D,	// такт 16
	A3F, L4D, F4D, L08, D4F, L08, E4F, L08, D3F, L08, E4F, L08,	// такт 17
	D3F, L04, B3F, L08, G4D, L08, E4F, L08, B4F, L04,	// такт 18
	A4F, L02, A4D, L08, F4D, L08, C5D, L04,	// такт 19
	B4F, L04, D4F, L04, D5F, L08, B4F, L08, C5D, L04,	// такт 20
	A4F, L04, A4F, L04, F4D, L08, A3F, L08, A4F, L04,	// такт 21
	F4D, L08, A3F, L08, D5F, L08, G3F, L08, B3F, L08, G3F, L08, A4F, L04,	// такт 22
	A3F, L04, E4F, L04, B4F, L04, A4F, L08, D3F, L08,	// такт 23
	D4F, L08, G4F, L08, F4D, L4D, G3F, L08,	// такт 24
	E4F, L02, A4F, L04, F4D, L04,	// такт 25
	A4F, L04, E4F, L04, D5F, L04, B3F, L08, G3F, L08,	// такт 26
	A4F, L04, A3F, L04, D4F, L08, A3F, L08, B4F, L16, C5D, L16, D5F, L08,	// такт 27
	A4F, L08, A3F, L08, D4F, L08, A3F, L08, D4F, L08, A3F, L08, B4F, L16, C5D, L16, D5F, L08,	// такт 28
	A4F, L08, A3F, L08, F4D, L08, A3F, L08, D4F, L08, A3F, L08, B4F, L16, C5D, L16, D5F, L08,	// такт 29
	A4F, L08, A3F, L08, D4F, L08, A3F, L08, D4F, L04,	// такт 30
	D5F, L02, C5D, L04, D5F, L08, A5F, L16, E5F, L16,	// такт 31
	F5D, L8D, A4F, L16, D5F, L08, A5F, L16, E5F, L16, F5D, L08, A4F, L04, E5F, L08,	// такт 32
	F5D, L08, A4F, L08, D5F, L08, F5D, L08, A5F, L04, A5F, L08, D5F, L08,	// такт 33
	F5D, L08, D5F, L08, A5F, L08, D5F, L08, F5D, L08, D5F, L08, D6F, L08, D5F, L08,	// такт 34
	G4F, L08, D5F, L08, A5F, L08, D5F, L08, C5D, L08, D5F, L08, G4F, L08, D5F, L08,	// такт 35
	B5F, L08, D6F, L08, A5F, L08, E5F, L08, F5D, L08, D5F, L08, G4F, L08, D5F, L08,	// такт 36
	G5F, L04, D5F, L08, A3F, L08, A3F, L08, PAUSE, L08, D4F, L08, A4F, L08,	// такт 37
	D5F, L08, A5F, L08, E6F, L08, D6F, L4D,	// такт 38
};


//=====================================================================//
// Arthur Warrell - We Wish You a Merry Christmas
//=====================================================================//
const uint8_t christmas[] PROGMEM =
{
	TEMPO, 14, TRANS, 20,	// ~140 BPM
	G4F, L08, PAUSE, L08, C5F, L08, PAUSE, L08, C5F, L08, D5F, L08, C5F, L08, B4F, L08,	// такт 1
	A4F, L08, PAUSE, L08, A4F, L08, PAUSE, L08, A4F, L08, PAUSE, L08, D5F, L08, PAUSE, L08,	// такт 2
	D5F, L08, E5F, L08, A4F, L08, C5F, L08, B4F, L08, PAUSE, L08, B4F, L08, PAUSE, L08,	// такт 3
	B4F, L08, PAUSE, L08, E5F, L08, PAUSE, L08, E5F, L08, F5F, L08, E5F, L08, D5F, L08,	// такт 4
	C5F, L08, PAUSE, L08, A4F, L08, PAUSE, L08, G4F, L08, PAUSE, L08, A4F, L08, PAUSE, L08,	// такт 5
	D5F, L08, PAUSE, L08, B4F, L08, PAUSE, L08, C5F, L08, PAUSE, L08, C4F, L08, PAUSE, L08,	// такт 6
	G4F, L08, PAUSE, L08, C5F, L08, PAUSE, L08, C5F, L08, D5F, L08, C5F, L08, B4F, L08,	// такт 7
	A4F, L08, PAUSE, L08, A4F, L08, PAUSE, L08, A4F, L08, PAUSE, L08, D5F, L08, PAUSE, L08,	// такт 8
	D5F, L08, E5F, L08, D5F, L08, C5F, L08, B4F, L08, PAUSE, L08, B4F, L08, PAUSE, L08,	// такт 9
	B4F, L08, PAUSE, L08, E5F, L08, PAUSE, L08, E5F, L08, F5F, L08, E5F, L08, D5F, L08,	// такт 10
	C5F, L08, PAUSE, L08, A4F, L08, PAUSE, L08, G4F, L08, PAUSE, L08, A4F, L08, PAUSE, L08,	// такт 11
	D5F, L08, PAUSE, L08, B4F, L08, PAUSE, L08, C5F, L08,	// такт 12

	G4F, L08, PAUSE, L08, C5F, L08, PAUSE, L08, C5F, L08, D5F, L08, C5F, L08, B4F, L08,	// такт 1
	A4F, L08, PAUSE, L08, A4F, L08, PAUSE, L08, A4F, L08, PAUSE, L08, D5F, L08, PAUSE, L08,	// такт 2
	D5F, L08, E5F, L08, A4F, L08, C5F, L08, B4F, L08, PAUSE, L08, B4F, L08, PAUSE, L08,	// такт 3
	B4F, L08, PAUSE, L08, E5F, L08, PAUSE, L08, E5F, L08, F5F, L08, E5F, L08, D5F, L08,	// такт 4
	C5F, L08, PAUSE, L08, A4F, L08, PAUSE, L08, G4F, L08, PAUSE, L08, A4F, L08, PAUSE, L08,	// такт 5
	D5F, L08, PAUSE, L08, B4F, L08, PAUSE, L08, C5F, L08, PAUSE, L08, C4F, L08, PAUSE, L08,	// такт 6
	G4F, L08, PAUSE, L08, C5F, L08, PAUSE, L08, C5F, L08, D5F, L08, C5F, L08, B4F, L08,	// такт 7
	A4F, L08, PAUSE, L08, A4F, L08, PAUSE, L08, A4F, L08, PAUSE, L08, D5F, L08, PAUSE, L08,	// такт 8
	D5F, L08, E5F, L08, D5F, L08, C5F, L08, B4F, L08, PAUSE, L08, B4F, L08, PAUSE, L08,	// такт 9
	B4F, L08, PAUSE, L08, E5F, L08, PAUSE, L08, E5F, L08, F5F, L08, E5F, L08, D5F, L08,	// такт 10
	C5F, L08, PAUSE, L08, A4F, L08, PAUSE, L08, G4F, L08, PAUSE, L08, A4F, L08, PAUSE, L08,	// такт 11
	D5F, L08, PAUSE, L08, B4F, L08, PAUSE, L08, C5F, L08,	// такт 12
};

//=====================================================================//
// James Horner - Titanic - My Heart Will Go On
//=====================================================================//
const uint8_t titanic[] PROGMEM =
{
	TEMPO, 10, TRANS, 20,	// ~100 BPM
	E4F, L08, F4D, L08, F4D, L08, G4D, L02, G4D, L08,	// такт 1
	F4D, L08, E4F, L08, F4D, L08, B4F, L02, B4F, L08,	// такт 2
	A4F, L08, G4D, L08, E4F, L04, C4D, L02,	// такт 3
	A3F, L02, A3F, L08, B3F, L4D,	// такт 4
	E4F, L08, F4D, L08, F4D, L08, G4D, L02, G4D, L08,	// такт 5
	A4F, L16, G4D, L16, F4D, L16, E4F, L16, F4D, L08, B4F, L02, B4F, L08,	// такт 6
	G4D, L08, B4F, L08, C5D, L02,	// такт 7
	B4F, L02,	// такт 8
	F4D, L2D, PAUSE, L04,	// такт 9
	C4D, L08, E4F, L08, G4D, L08, E5F, L04, B4F, L08, E4F, L04,	// такт 10
	B3F, L08, E4F, L08, G4D, L08, E5F, L04, B4F, L08, E4F, L04,	// такт 11
	A3F, L08, E4F, L08, A4F, L08, E5F, L04, B4F, L08, E4F, L04,	// такт 12
	B3F, L08, E4F, L08, E5F, L08, E4F, L08, E5F, L08, E4F, L08, D5D, L04,	// такт 13
	C4D, L08, E4F, L08, G4D, L08, E5F, L04, B4F, L08, E4F, L08, C4D, L08,	// такт 14
	B3F, L08, E4F, L08, G4D, L08, E5F, L04, B4F, L08, E4F, L04,	// такт 15
	A3F, L08, E4F, L08, A4F, L08, E5F, L04, B4F, L08, E4F, L08, A3F, L08,	// такт 16
	B3F, L08, E4F, L08, E5F, L08, E4F, L08, E5F, L08, E4F, L08, D5D, L08, E4F, L08,	// такт 17
	E5F, L08, B3F, L08, E4F, L08, E5F, L08, E5F, L08, B3F, L08, E5F, L08, E4F, L08,	// такт 18
	D5D, L08, B3F, L08, E5F, L08, B3F, L08, E4F, L08, B3F, L08, E5F, L08, E4F, L08,	// такт 19
	D5D, L08, A3F, L08, E5F, L08, A3F, L08, C4D, L08, A3F, L08, F5D, L08, A4F, L08,	// такт 20
	G5D, L08, B3F, L08, E4F, L08, B3F, L08, F5D, L08, B3F, L08, F4D, L08, B3F, L08,	// такт 21
	E5F, L08, B3F, L08, E4F, L08, E5F, L08, E5F, L08, B3F, L08, E5F, L08, E4F, L08,	// такт 22
	D5D, L08, B3F, L08, E5F, L08, B3F, L08, E4F, L08, B3F, L08, E5F, L08, E4F, L08,	// такт 23
	C5D, L08, A3F, L08, C4D, L08, A3F, L08, E4F, L08, A3F, L08, E3F, L08, F3D, L08,	// такт 24
	A3F, L08, B3F, L08, C4D, L08, E4F, L08, A4F, L08, B4F, L08, C5D, L08, D5D, L08,	// такт 25
	E5F, L08, B3F, L08, E4F, L08, E5F, L08, E5F, L08, B3F, L08, E5F, L08, E4F, L08,	// такт 26
	D5D, L08, B3F, L08, E5F, L08, B3F, L08, E4F, L08, B3F, L08, E5F, L08, E4F, L08,	// такт 27
	D5D, L08, A3F, L08, E5F, L08, A3F, L08, C4D, L08, A3F, L08, F5D, L08, A3F, L08,	// такт 28
	G5D, L08, B3F, L08, E4F, L08, B3F, L08, F5D, L08, B3F, L08, F4D, L08, B3F, L08,	// такт 29
	E5F, L08, B3F, L08, E4F, L08, E5F, L08, E5F, L08, B3F, L08, E5F, L08, E4F, L08,	// такт 30
	D5D, L08, B3F, L08, E5F, L08, B3F, L08, E4F, L08, B3F, L08, E5F, L08, E4F, L08,	// такт 31
	C6D, L08, A3F, L08, C4D, L08, A3F, L08, E4F, L08, A3F, L08, C4D, L08, A3F, L08,	// такт 32
	E3F, L08, A3F, L08, C4D, L08, A3F, L08, C5D, L04, D5D, L04,	// такт 33
	E5F, L04, C4D, L08, G4D, L08, C5D, L08, G4D, L08, C4D, L08, G4D, L08,	// такт 34
	F5D, L04, B4F, L08, F4D, L08, C5D, L08, F4D, L08, B4F, L08, F4D, L08,	// такт 35
	B5F, L04, A4F, L08, E4F, L08, A5F, L08, E4F, L08, G5D, L08,	// такт 36
	F5D, L4D, B4F, L08, F4D, L08, G5D, L08, F4D, L08, A5F, L08,	// такт 37
	F4D, L08, G5D, L04, G4D, L08, C5D, L08, F5D, L08, G4D, L08, E5F, L08,	// такт 38
	G4D, L08, D5D, L08, F4D, L08, E5F, L08, F4D, L08, B4F, L08, F4D, L08, B4F, L08,	// такт 39
	D5D, L08, C5D, L04, C4D, L08, A3F, L08, E4F, L08, A3F, L08, C4D, L08,	// такт 40
	A3F, L08, A3F, L08, B3F, L08, C4D, L08, E4F, L08, A4F, L08, B4F, L08, C5D, L08,	// такт 41
	D5D, L08, E5F, L04, C4D, L08, G4D, L08, C5D, L08, G4D, L08, C4D, L08,	// такт 42
	G4D, L08, F5D, L04, B4F, L08, F4D, L08, C5D, L08, F4D, L08, B4F, L08,	// такт 43
	F4D, L08, B5F, L04, A4F, L08, E4F, L08, A5F, L08, E4F, L08, G5D, L08,	// такт 44
	F5D, L4D, B4F, L08, F4D, L08, G5D, L08, F4D, L08, A5F, L08,	// такт 45
	F4D, L08, G5D, L04, G4D, L08, C5D, L08, F5D, L08, G4D, L08, E5F, L08,	// такт 46
	G4D, L08, D5D, L08, F4D, L08, E5F, L08, F4D, L08, B4F, L08, F4D, L08, D5D, L08,	// такт 47
	F4D, L08, D5D, L08, E4F, L08, E5F, L08, E4F, L08, A4F, L08, E4F, L08, F5D, L08,	// такт 48
	E4F, L08, G5D, L08, B3F, L08, E4F, L08, B3F, L08, F5D, L08, B3F, L08, F4D, L08,	// такт 49
	B3F, L08, E5F, L08, G4D, L08, C4D, L08, G4D, L08, E4F, L08, PAUSE, L08, A4F, L16, G4D, L16,	// такт 50
	F4D, L16, E4F, L16, F4D, L08, B4F, L08, B3F, L08, F4D, L08, D4D, L08, PAUSE, L08, G4D, L08,	// такт 51
	B4F, L08, C5D, L08, E4F, L08, A3F, L08, E4F, L08, B4F, L08, E4F, L08, A3F, L08,	// такт 52
	E4F, L08, B3F, L08, F4D, L08, B4F, L08, D5D, L08, F5D, L08, B5F, L08, D6D, L08,	// такт 53
	F6D, L08,	// такт 54
};


/**
 * Таблица песен (в PROGMEM, чтобы не занимать SRAM).
 *
 * Каждый элемент: { pointer, length }.
 * Порядок в таблице соответствует индексам для Player::setSong(index).
 */
static const SongInfo songs[] PROGMEM = {
	SONG_ENTRY(jinglebells),
	SONG_ENTRY(totoro),
	SONG_ENTRY(minecraft),
	SONG_ENTRY(titanic),
	SONG_ENTRY(in_my_memory),
	SONG_ENTRY(christmas),
	SONG_ENTRY(deckhalls),
};

/** Количество песен в таблице songs[]. */
#define NUM_SONGS (sizeof(songs) / sizeof(songs[0]))
