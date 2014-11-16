include_directories (${PROJECT_SOURCE_DIR}/src)

link_directories(${PROJECT_BINARY_DIR}/src)

add_executable(grf_request grf_request.c)
add_executable(grfctl      grfctl.c grf_scan.c)

target_link_libraries(grf_request grf)
target_link_libraries(grfctl      grf)

install(TARGETS grf_request   DESTINATION bin)
install(TARGETS grfctl        DESTINATION bin)
