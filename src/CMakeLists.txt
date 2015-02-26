set(GRFUTILS_SOURCES grf_radio_uart.c grf_comm.c grf_logging.c)

include_directories("${PROJECT_BINARY_DIR}")

add_library(grf SHARED ${GRFUTILS_SOURCES})

install(TARGETS grf LIBRARY DESTINATION lib)

install(FILES grf.h grf_radio.h DESTINATION include)
