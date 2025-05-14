add_library(joysticks INTERFACE)

target_sources(joysticks INTERFACE
		${CMAKE_CURRENT_LIST_DIR}/Joystics.c
		${CMAKE_CURRENT_LIST_DIR}/Joystics.h
		${CMAKE_CURRENT_LIST_DIR}/util_i2c_joy.c
		${CMAKE_CURRENT_LIST_DIR}/util_i2c_joy.h
		${CMAKE_CURRENT_LIST_DIR}/util_i2c_MCP23017.c
		${CMAKE_CURRENT_LIST_DIR}/util_i2c_MCP23017.h
		${CMAKE_CURRENT_LIST_DIR}/util_i2c_PCF857X.c
		${CMAKE_CURRENT_LIST_DIR}/util_i2c_PCF857X.h
		${CMAKE_CURRENT_LIST_DIR}/util_NES_Joy.c
		${CMAKE_CURRENT_LIST_DIR}/util_NES_Joy.h
		${CMAKE_CURRENT_LIST_DIR}/util_Wii_Joy.c
		${CMAKE_CURRENT_LIST_DIR}/util_Wii_Joy.h
)

target_link_libraries(joysticks INTERFACE hardware_i2c)

target_include_directories(joysticks INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}
)

#pico_generate_pio_header(audio
#  ${CMAKE_CURRENT_LIST_DIR}/audio_i2s.pio
#)
