# PULSE — storage module build configuration

add_library(storage STATIC
    storage_engine_stub.cpp
)

target_include_directories(storage PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)