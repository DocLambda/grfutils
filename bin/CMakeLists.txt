include_directories (${PROJECT_SOURCE_DIR}/src)

link_directories(${PROJECT_BINARY_DIR}/src)

add_executable(grf_scan    grf_scan.c)
add_executable(grf_request grf_request.c)

target_link_libraries(grf_scan    grf)
target_link_libraries(grf_request grf)

install(TARGETS grf_scan      DESTINATION bin)
install(TARGETS grf_request   DESTINATION bin)
