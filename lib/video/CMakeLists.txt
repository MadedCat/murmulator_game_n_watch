add_library(video INTERFACE)

target_sources(video INTERFACE
		${CMAKE_CURRENT_LIST_DIR}/video.c
        #${CMAKE_CURRENT_LIST_DIR}/video.h
)

target_link_libraries(video INTERFACE hardware_pio hardware_dma)

target_include_directories(video INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}
)
