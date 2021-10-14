cmake_minimum_required(VERSION 3.10)

project(ncihostd)

set(CMAKE_CXX_STANDARD 17)

add_executable(ncihostd startup.c daemon.cpp IpcStateController.cpp IpcStateController.h NciHostDaemonImpl.cpp NciHostDaemonImpl.h)

set_target_properties(ncihostd PROPERTIES
        CXX_EXTENSIONS OFF
        POSITION_INDEPENDENT_CODE ON
        )

set_target_properties(ncihostd PROPERTIES
        PREFIX "lib"
        SUFFIX ".so"
        )

target_compile_definitions(ncihostd PRIVATE NCI_HOST_VERSION=\"${NCI_HOST_VERSION}\")
target_compile_definitions(ncihostd PRIVATE NCI_HOST_BUILD_UUID=\"${NCI_HOST_BUILD_UUID}\")

target_link_libraries(ncihostd c dl rpcprotocol ${ANDROID_LIBS})
