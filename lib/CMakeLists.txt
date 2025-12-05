# Library sources
set(LIBZEUGL_SOURCES
    zeugl.c
    filecopy.h
    filecopy.c
    immutable.h
    signals.h
    signals.c
    whackamole.h
    whackamole.c
    logger.h
    utils.h
)

# Add platform-specific immutable implementation
if(IMMUTABLE_IOCTL)
    list(APPEND LIBZEUGL_SOURCES immutable_ioctl.c)
elseif(IMMUTABLE_CHFLAGS)
    list(APPEND LIBZEUGL_SOURCES immutable_chflags.c)
elseif(IMMUTABLE_WEAK)
    list(APPEND LIBZEUGL_SOURCES immutable_weak.c)
endif()

# Add logger implementation if debug is enabled
if(ENABLE_DEBUG)
    list(APPEND LIBZEUGL_SOURCES logger.c)
endif()

# Create shared library
add_library(zeugl SHARED ${LIBZEUGL_SOURCES})

# Link with pthread
target_link_libraries(zeugl PRIVATE Threads::Threads)

# Set library properties
set_target_properties(zeugl PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
    PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/../include/zeugl.h
)

# Include config.h
target_include_directories(zeugl PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

# Install library
install(TARGETS zeugl
    LIBRARY DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)
