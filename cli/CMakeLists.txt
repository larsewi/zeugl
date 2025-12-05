# CLI executable
add_executable(zeugl_cli main.c)

# Link with the zeugl library
target_link_libraries(zeugl_cli PRIVATE zeugl)

# Include directories
target_include_directories(zeugl_cli PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/lib
)

# Set output name to match autotools (just "zeugl")
set_target_properties(zeugl_cli PROPERTIES OUTPUT_NAME zeugl)

# Install executable
install(TARGETS zeugl_cli DESTINATION bin)
