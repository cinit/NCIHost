cmake_minimum_required(VERSION 3.10)

project(ncihostd)

add_executable(ncihostd startup.c daemon.cpp)

set_target_properties(ncihostd PROPERTIES
        CXX_EXTENSIONS OFF
        POSITION_INDEPENDENT_CODE ON
        )

set_target_properties(ncihostd PROPERTIES
        PREFIX "lib"
        SUFFIX ".so"
        )

target_compile_definitions(ncihostd PRIVATE NCI_HOST_VERSION=\"${NCI_HOST_VERSION}\")

target_link_libraries(ncihostd c dl rpcprotocol ${ANDROID_LIBS})
