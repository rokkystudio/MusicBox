#include <Arduino.h>

#include "MusicBox.h"

//=====================================================================//
/**
 * main.cpp
 *
 * Обвязка для CLion/CMake (как Arduino core):
 *  - init()
 *  - setup()
 *  - бесконечный loop()
 *
 * Этот файл НЕ предназначен для копирования в Arduino IDE.
 * В Arduino IDE main() уже есть внутри core.
 */

extern "C" void initVariant(void) __attribute__((weak));
extern "C" void serialEventRun(void) __attribute__((weak));

int main() {
    init();

    if (initVariant) {
        initVariant();
    }

    setup();

    // ReSharper disable once CppDFAEndlessLoop
    while (true) {
        loop();

        // Совместимость с Arduino main(): если есть serialEventRun — вызываем.
        if (serialEventRun) {
            serialEventRun();
        }
    }
}
