cmake_minimum_required(VERSION 3.10)

project(nciclient)

set(CMAKE_CXX_STANDARD 17)

add_library(nciclient SHARED
        version.c ipc_handle_jni.cpp IpcConnector.cpp NciClientImpl.cpp NciHostDaemonProxy.cpp NciHostDaemonProxy.h)

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    SET(CLANG_CXX_EXTRA_OPT "-Werror=unknown-warning-option -Werror=format-invalid-specifier -Werror=call-to-pure-virtual-from-ctor-dtor")
    SET(CLANG_C_EXTRA_OPT "-Werror=format-invalid-specifier")
else ()
    SET(CLANG_CXX_EXTRA_OPT "")
    SET(CLANG_C_EXTRA_OPT "")
endif ()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CLANG_CXX_EXTRA_OPT} -fPIC -Werror=delete-non-virtual-dtor -Werror=return-type -Werror=non-virtual-dtor -Wno-invalid-offsetof")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CLANG_C_EXTRA_OPT} -fPIC -Werror=return-type")

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-allow-shlib-undefined,--no-undefined")

set_target_properties(nciclient PROPERTIES
        CXX_EXTENSIONS OFF
        POSITION_INDEPENDENT_CODE ON
        )

include_directories(..)

target_compile_definitions(nciclient PRIVATE NCI_HOST_VERSION=\"${NCI_HOST_VERSION}\")

target_link_libraries(nciclient rpcprotocol ${ANDROID_LIBS})
