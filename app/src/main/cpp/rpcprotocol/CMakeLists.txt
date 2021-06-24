cmake_minimum_required(VERSION 3.10)

project(rpcprotocol)

set(CMAKE_CXX_STANDARD 17)

add_library(rpcprotocol STATIC
        ArgList.cpp rpc_proc.cpp ThreadPool.cpp shared_memory.cpp RemoteDaemonException.cpp)

set_target_properties(rpcprotocol PROPERTIES
        CXX_EXTENSIONS OFF
        POSITION_INDEPENDENT_CODE ON
        )

target_link_libraries(rpcprotocol)
