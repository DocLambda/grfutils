include_directories (${PROJECT_SOURCE_DIR}/src)

link_directories(${PROJECT_BINARY_DIR}/src)

add_executable(grfctl      grfctl.c grf_scan.c grf_request.c)

target_link_libraries(grfctl      grf m)

install(TARGETS grfctl        DESTINATION bin)
