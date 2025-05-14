# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD_HEADER_DIRS ${CMAKE_CURRENT_LIST_DIR}/boards )

set(PICO_BOARD murmulator CACHE STRING "Board type")
set(PICO_PLATFORM "rp2040" CACHE STRING "Chip type")

#set(PICO_BOARD murmulator2 CACHE STRING "Board type")
#set(PICO_PLATFORM "rp2350" CACHE STRING "Chip type")

set (VERSION_MAJOR 0)
set (VERSION_MINOR 1)
set (VERSION_PATCH 15)
set (SOFT_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})
set (SOFT_VERSION_FN ${VERSION_MAJOR}_${VERSION_MINOR}_${VERSION_PATCH})

configure_file ("src/config.h.in" "../src/config.h" @ONLY)

include(pico_sdk_import.cmake)

project(murmulator C CXX ASM)



option(VGA "Enable VGA" OFF)
option(TFT "Enable TFT display" OFF)
option(HDMI "Enable HDMI display" OFF)
option(TV "Enable TV composite output" OFF)
option(SOFTTV "Enable TV soft composite output" OFF)

SET(PROJECT_NAME "Murmulator_GnW_vga-hdmi-tft_${SOFT_VERSION_FN}_m1_p1")
SET(BUILD_NAME "${PROJECT_NAME}_${SOFT_VERSION}")

set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}") #/bin/
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
#set(CMAKE_launchTargetPath "${OUTPUT_DIR}")



# Heap and Stack size
set(PICO_HEAP_SIZE 1024)
set(PICO_STACK_SIZE 1024)

pico_sdk_init()

add_subdirectory(lib/fatfs)
add_subdirectory(lib/sdcard)
add_subdirectory(lib/joysticks)
add_subdirectory(lib/sound)
add_subdirectory(lib/video)

# Main Executable
add_executable(${PROJECT_NAME})

pico_define_boot_stage2(slower_boot2 ${PICO_DEFAULT_BOOT_STAGE2_FILE})
    target_compile_definitions(slower_boot2 PRIVATE PICO_FLASH_SPI_CLKDIV=4)
    pico_set_boot_stage2(${PROJECT_NAME} slower_boot2)

file(GLOB_RECURSE SRC "src/*.cpp" "src/*.c")
message(STATUS "Add source files:")
foreach (SRC_FILE IN LISTS SRC)
    message(STATUS "${SRC_FILE}")
endforeach ()
message(STATUS "")

# Source files
file(GLOB_RECURSE SOURCES src/*.c src/*.cpp)
message(STATUS "${SOURCES}")
target_sources(${PROJECT_NAME} PRIVATE ${SOURCES})

# Common options
add_compile_options(-O2)
add_definitions(-DPICO_STDIO_USB)
#add_definitions(-DPICO_PLATFORM=rp2350)

# Include directories
#target_include_directories(murmulator PRIVATE src)

# Link Libraries

target_link_libraries(${PROJECT_NAME} PRIVATE
    fatfs
    sdcard
    joysticks
    sound
    video

    pico_stdlib 
    hardware_spi 
    hardware_i2c 
    hardware_pwm 
    hardware_adc 
    hardware_pio 
    hardware_irq 
    hardware_dma 
    pico_time 
    hardware_timer 
    pico_rand 
    pico_stdio 
    pico_stdio_usb
    pico_multicore
    
)


set(LINK_LIBS "fatfs sdcard joysticks sound")

# Conditional configurations
if(DEFINED VGA_HDMI)
    add_definitions(-DVGA_HDMI)
    target_link_libraries(${PROJECT_NAME} video ${LINK_LIBS} ${LINK_LIBS} )
elif(DEFINED COMPOSITE_TV)
    add_definitions(-DCOMPOSITE_TV)
    target_link_libraries(${PROJECT_NAME} tv_out ${LINK_LIBS} ${LINK_LIBS} )
elif(DEFINED SOFT_COMPOSITE_TV)
    add_definitions(-DSOFT_COMPOSITE_TV -DSOFT_CVBS_NO_HUD)
    target_link_libraries(${PROJECT_NAME} tv_out_soft ${LINK_LIBS} ${LINK_LIBS} )
endif()

# Ignore certain libraries in specific environments
if(DEFINED VGA_HDMI)
    target_compile_definitions(${PROJECT_NAME} PRIVATE -DNO_TV_OUT -DNO_TFT)
elif(DEFINED COMPOSITE_TV)
    target_compile_definitions(${PROJECT_NAME} PRIVATE -DNO_VIDEO -DNO_TFT)
elif(DEFINED SOFT_COMPOSITE_TV)
    target_compile_definitions(${PROJECT_NAME} PRIVATE -DNO_VIDEO -DNO_TV_OUT -DNO_TFT)
endif()

# Output binary and UF2
pico_add_extra_outputs(${PROJECT_NAME})
target_link_options(${PROJECT_NAME} PRIVATE -Xlinker --print-memory-usage --data-sections --function-sections)
