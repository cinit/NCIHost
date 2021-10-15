cmake_minimum_required(VERSION 3.10)

project(nciclient)

set(CMAKE_CXX_STANDARD 17)

add_library(nciclient SHARED
        version.c ipc_handle_jni.cpp IpcConnector.cpp NciHostDaemonProxy.cpp NciHostDaemonProxy.h)

set_target_properties(nciclient PROPERTIES
        CXX_EXTENSIONS OFF
        POSITION_INDEPENDENT_CODE ON
        )
target_compile_definitions(nciclient PRIVATE NCI_HOST_VERSION=\"${NCI_HOST_VERSION}\")

target_link_libraries(nciclient rpcprotocol ${ANDROID_LIBS})
