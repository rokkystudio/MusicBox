cmake_minimum_required(VERSION 3.16)

#=====================================================================#
# Project
#=====================================================================#
project(MusicBox C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

#=====================================================================#
# Paths (Digistump package copied into the repo)
#=====================================================================#
set(DIGISTUMP_AVR_ROOT "${CMAKE_SOURCE_DIR}/digistump/hardware/avr/1.6.7")
set(DIGISTUMP_CORE_DIR "${DIGISTUMP_AVR_ROOT}/cores/tiny")
set(DIGISTUMP_VARIANT_DIR "${DIGISTUMP_AVR_ROOT}/variants/digispark")

#=====================================================================#
# User sources
#=====================================================================#
file(GLOB USER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.c"
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

add_executable(MusicBox
    ${USER_SOURCES}
    src/MusicBox.h
    src/Player.h
    src/Songs.h
    src/Music.h
        src/Synth.h
    src/Lights.h
)

#=====================================================================#
# Arduino core sources (Digistump tiny)
#=====================================================================#
file(GLOB DIGISTUMP_CORE_SOURCES CONFIGURE_DEPENDS
    "${DIGISTUMP_CORE_DIR}/*.c"
    "${DIGISTUMP_CORE_DIR}/*.cpp"
    "${DIGISTUMP_CORE_DIR}/*.S"
)

# Убираем Arduino-овский main.cpp, чтобы не конфликтовал с твоим src/main.cpp
list(FILTER DIGISTUMP_CORE_SOURCES EXCLUDE REGEX ".*/main\\.cpp$")

target_sources(MusicBox PRIVATE
    ${DIGISTUMP_CORE_SOURCES}
)

#=====================================================================#
# Include paths
#=====================================================================#
target_include_directories(MusicBox PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${DIGISTUMP_CORE_DIR}"
    "${DIGISTUMP_VARIANT_DIR}"
)

#=====================================================================#
# Arduino compatibility macros
#=====================================================================#
target_compile_definitions(MusicBox PRIVATE
    ARDUINO=10819
    ARDUINO_ARCH_AVR
    ARDUINO_AVR_DIGISPARK
    __AVR_ATtiny85__
)

#=====================================================================#
# HEX generation (.elf -> .hex) + size
#=====================================================================#
set(HEX_FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.hex")

add_custom_command(TARGET MusicBox POST_BUILD
    # Используем binutils из toolchain-avr.cmake (не зависим от PATH)
    COMMAND ${CMAKE_OBJCOPY} -O ihex -R .eeprom $<TARGET_FILE:MusicBox> "${HEX_FILE}"
    COMMAND ${CMAKE_SIZE} --mcu=${AVR_MCU} -C $<TARGET_FILE:MusicBox>
    BYPRODUCTS "${HEX_FILE}"
    COMMENT "Generating ${PROJECT_NAME}.hex"
)

#=====================================================================#
# Upload target (Digispark uses micronucleus)
#=====================================================================#
if(WIN32)
    file(GLOB_RECURSE MICRONUCLEUS_CANDIDATES
        "${CMAKE_SOURCE_DIR}/digistump/tools/micronucleus/*/micronucleus*.exe"
    )
else()
    file(GLOB_RECURSE MICRONUCLEUS_CANDIDATES
        "${CMAKE_SOURCE_DIR}/digistump/tools/micronucleus/*/micronucleus*"
    )
endif()

list(LENGTH MICRONUCLEUS_CANDIDATES MICRONUCLEUS_COUNT)

if(MICRONUCLEUS_COUNT GREATER 0)
    list(GET MICRONUCLEUS_CANDIDATES 0 MICRONUCLEUS_EXE)

    add_custom_target(upload
        COMMAND "${MICRONUCLEUS_EXE}" --run "${HEX_FILE}"
        DEPENDS MusicBox
        USES_TERMINAL
        COMMENT "Uploading via micronucleus (for Digispark/ATtiny85)"
    )
endif()
