add_library(sound INTERFACE)

target_sources(sound INTERFACE
		${CMAKE_CURRENT_LIST_DIR}/i2s.c
		${CMAKE_CURRENT_LIST_DIR}/i2s.h
)

target_link_libraries(sound INTERFACE hardware_pio hardware_clocks hardware_dma)

target_include_directories(sound INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}
)

#pico_generate_pio_header(sound
#  ${CMAKE_CURRENT_LIST_DIR}/sound_i2s.pio
#)
