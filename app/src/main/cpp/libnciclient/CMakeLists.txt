cmake_minimum_required(VERSION 3.10)

project(nciclient)

add_library(nciclient SHARED
        version.c IpcConnection.cpp)

set_target_properties(nciclient PROPERTIES
        CXX_EXTENSIONS OFF
        POSITION_INDEPENDENT_CODE ON
        )
target_compile_definitions(nciclient PRIVATE NCI_HOST_VERSION=\"${NCI_HOST_VERSION}\")

target_link_libraries(nciclient rpcprotocol ${ANDROID_LIBS})
