#=====================================================================#
# AVR toolchain for ATtiny85 (Digispark) - local copy inside project
#=====================================================================#

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR avr)

# Чтобы CMake не пытался запускать тестовые бинарники на хосте
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# --- MCU и частота ---
set(AVR_MCU attiny85)
set(AVR_F_CPU 16500000UL)

# --- Локальный avr-gcc (внутри проекта) ---
set(AVR_GCC_ROOT "${CMAKE_SOURCE_DIR}/toolchains/avr-gcc/7.3.0-atmel3.6.1-arduino7")
set(AVR_BIN      "${AVR_GCC_ROOT}/bin")

# --- Компиляторы ---
set(CMAKE_C_COMPILER   "${AVR_BIN}/avr-gcc.exe")
set(CMAKE_CXX_COMPILER "${AVR_BIN}/avr-g++.exe")
set(CMAKE_ASM_COMPILER "${AVR_BIN}/avr-gcc.exe")

# --- Binutils (чтобы .hex/size работали без PATH) ---
set(CMAKE_AR      "${AVR_BIN}/avr-ar.exe")
set(CMAKE_OBJCOPY "${AVR_BIN}/avr-objcopy.exe")
set(CMAKE_SIZE    "${AVR_BIN}/avr-size.exe")

# --- Флаги компиляции ---
set(COMMON_FLAGS "-mmcu=${AVR_MCU} -DF_CPU=${AVR_F_CPU} -Os -ffunction-sections -fdata-sections")

set(CMAKE_C_FLAGS   "${COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS "${COMMON_FLAGS} -fno-exceptions -fno-rtti -fno-threadsafe-statics")

# --- Линковка ---
set(CMAKE_EXE_LINKER_FLAGS "-mmcu=${AVR_MCU} -Wl,--gc-sections")
