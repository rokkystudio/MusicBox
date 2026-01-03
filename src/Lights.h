#pragma once

#include <avr/io.h>

/**
 * @file Lights.h
 * Гирлянда/LED: очень простой "вдох-выдох" (треугольник) за 1 такт.
 *
 * Идея:
 *  - за один музыкальный такт (по умолчанию 16 шестнадцатых) делаем:
 *      1 раз плавно вверх до LED_MAX_PWM
 *      1 раз плавно вниз до 0
 *  - обновление делаем только на "нотном тике" (не на каждом аудио-сэмпле)
 *
 * Аппаратно:
 *  - PB1 (OC0B) -> PWM Timer0
 *  - Lights_tick() пишет яркость в OCR0B
 *
 * Входные данные:
 *  - ticksPer16: сколько "нотных тиков" приходится на 1/16 (зависит от темпа)
 */

// "Такт" в шестнадцатых (4/4 = 16). Для 3/4 можно поставить 12.
#define LED_BAR_LEN16			16

// Ограничитель яркости гирлянды (0..255).
#define LED_MAX_PWM				15

#define LED_MAX_Q8				((uint16_t)((uint16_t)LED_MAX_PWM << 8))

//=====================================================================//
// Состояние гирлянды
//=====================================================================//
typedef struct {
	uint16_t q8;		// яркость в Q8.8
	int16_t  step_q8;	// шаг (Q8.8), знак = направление (вверх/вниз)
} LightsState;

//---------------------------------------------------------------------//
// Инициализация (выключено)
//---------------------------------------------------------------------//
static inline void Lights_begin(volatile LightsState &st) {
	st.q8      = 0;
	st.step_q8 = 0;
	OCR0B      = 0;
}

//---------------------------------------------------------------------//
// Сброс фазы "такта" (синхронизация с началом песни)
//---------------------------------------------------------------------//
static inline void Lights_reset(volatile LightsState &st) {
	st.q8 = 0;
	if (st.step_q8 < 0) {
		st.step_q8 = static_cast<int16_t>(-st.step_q8);
	}
	OCR0B = 0;
}

//---------------------------------------------------------------------//
// Применить ticksPer16 (сколько "нотных тиков" на 1/16)
// и пересчитать скорость "дыхания".
//---------------------------------------------------------------------//
static inline void Lights_applyTempoTicksPer16(volatile LightsState &st, const uint8_t ticksPer16)
{
	if (ticksPer16 == 0) {
		// защита от мусора в данных
		st.step_q8 = 0;
		OCR0B = 0;
		return;
	}

	// 1 такт = LED_BAR_LEN16 * ticksPer16
	const auto barTicks  = static_cast<uint16_t>(
		static_cast<uint16_t>(LED_BAR_LEN16) * static_cast<uint16_t>(ticksPer16)
	);

	const auto halfTicks = static_cast<uint16_t>(barTicks / 2u);

	// ReSharper disable once CppRedundantBooleanExpressionArgument
	if (halfTicks == 0 || LED_MAX_PWM == 0) {
		st.step_q8 = 0;
		OCR0B = 0;
		return;
	}

	// stepAbs = ceil((LED_MAX_PWM<<8) / halfTicks)
	auto stepAbs = static_cast<uint16_t>(
		(static_cast<uint32_t>(LED_MAX_Q8) + (halfTicks - 1u)) / static_cast<uint32_t>(halfTicks)
	);

	// int16_t safety (на практике halfTicks всегда достаточно большой)
	if (stepAbs > 0x7FFFu) {
		stepAbs = 0x7FFFu;
	}

	// сохраняем направление
	if (st.step_q8 < 0) {
		st.step_q8 = static_cast<int16_t>(
			-static_cast<int16_t>(stepAbs)
		);
	} else {
		st.step_q8 = static_cast<int16_t>(stepAbs);
	}

	// если вдруг шаг стал 0 (очень медленно) — пусть будет хотя бы 1 LSB Q8.8
	if (st.step_q8 == 0) {
		st.step_q8 = 1;
	}
}

//---------------------------------------------------------------------//
// Один "нотный тик" гирлянды: треугольник 0..LED_MAX_PWM..0
//---------------------------------------------------------------------//
static inline void Lights_tick(volatile LightsState &st)
{
	if (st.step_q8 == 0) {
		OCR0B = 0;
		return;
	}

	if (st.step_q8 > 0) {
		const auto step = static_cast<uint16_t>(st.step_q8);
		const auto next = static_cast<uint16_t>(st.q8 + step);

		if (next >= LED_MAX_Q8) {
			st.q8 = LED_MAX_Q8;
			st.step_q8 = static_cast<int16_t>(-st.step_q8);
		} else {
			st.q8 = next;
		}
	} else {
		const auto step = static_cast<uint16_t>(-st.step_q8);

		if (st.q8 <= step) {
			st.q8 = 0;
			st.step_q8 = static_cast<int16_t>(-st.step_q8);
		} else {
			st.q8 = static_cast<uint16_t>(st.q8 - step);
		}
	}

	OCR0B = static_cast<uint8_t>(st.q8 >> 8);
}
