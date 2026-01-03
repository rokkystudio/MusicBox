#pragma once

#include <Arduino.h>
#include <avr/power.h>

#include "Player.h"

// Digispark / ATtiny85 USB board
// P0..P5 адресуются как 0..5

/**
 * @file MusicBox.h
 *
 * “Скетч” Arduino в виде заголовка.
 *
 * Для Arduino IDE:
 *  - можно просто скопировать содержимое этого файла в .ino (без правок)
 *    или переименовать в MusicBox.ino и собрать как обычный скетч.
 *
 * Для CLion/CMake:
 *  - этот файл подключается ТОЛЬКО из src/main.cpp, который реализует main()
 *    так же, как это делает Arduino core.
 *
 * Примечание:
 *  - setup()/loop() объявлены как inline, чтобы избежать конфликтов при
 *    случайном повторном включении заголовка в другом .cpp.
 */

inline void setup() {
    // Отключаем то, что можно (если макросы существуют для этого чипа)
    #ifdef power_adc_disable
        power_adc_disable();
    #endif
    #ifdef power_spi_disable
        power_spi_disable();
    #endif
    #ifdef power_twi_disable
        power_twi_disable();
    #endif
    #ifdef power_usart0_disable
        power_usart0_disable();
    #endif
    #ifdef power_timer2_disable
        power_timer2_disable();
    #endif

    Player::begin();
    Player::setSong(0);	// 0..9, если мимо NUM_SONGS — будет сыгран 0
}

inline void loop() {
    // Timer0 занят PWM, поэтому millis()/delay() могут быть некорректны.
    // Ничего не делаем.
}
