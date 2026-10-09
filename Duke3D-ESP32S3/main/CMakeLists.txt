# ESP32-S3 platform layer (this directory) + the engine and game, shared with
# the MG24 port (../Engine, ../Game).
set(DN3D_ROOT ${CMAKE_CURRENT_LIST_DIR}/..)

file(GLOB ENGINE_SRCS ${DN3D_ROOT}/Engine/src/*.c)
file(GLOB_RECURSE GAME_SRCS ${DN3D_ROOT}/Game/src/*.c)
file(GLOB PORT_SRCS ${CMAKE_CURRENT_LIST_DIR}/*.c)

idf_component_register(
    SRCS ${PORT_SRCS} ${ENGINE_SRCS} ${GAME_SRCS}
    # Same precedence as the original project: engine/game headers win over
    # same-named port headers (display.h, keyboard.h); "port/x.h" resolves here.
    INCLUDE_DIRS . ${DN3D_ROOT}/Engine/src ${DN3D_ROOT}/Game/src port
    REQUIRES esp_driver_spi esp_driver_i2c esp_driver_i2s esp_driver_gpio
             esp_driver_sdspi esp_driver_sdmmc fatfs sdmmc esp_partition
             esp_psram esp_timer spi_flash heap esp_system bt nvs_flash
)

# Board: set by the top-level CMakeLists.txt from -DDUKE_BOARD=... (see
# platformio.ini). Board checks (#if BOARD == BOARD_xxx) must see real values
# even in files that don't include config_enum_values.h (same values as there).
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    BOARD=${DUKE_BOARD} BOARD_TDECK=7 BOARD_WIO_TRACKER_L2=8
    BOARD_THEGAMEPAD_ARDUINONANOMATTER=6)
# The engine was written for -Os on GCC 12 ARM: silence the noise newer GCC adds,
# and keep pointer-type punning semantics the code relies on.
target_compile_options(${COMPONENT_LIB} PRIVATE
    -O2 -fno-strict-aliasing
    -w -Wno-error
    # GCC 14 turned these into errors; the 1996 code base relies on them.
    -Wno-error=int-conversion -Wno-error=incompatible-pointer-types
    -Wno-error=implicit-function-declaration -Wno-error=implicit-int
    -Wno-error=return-mismatch
)
