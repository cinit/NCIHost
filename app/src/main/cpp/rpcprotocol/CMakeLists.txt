cmake_minimum_required(VERSION 3.10)

project(rpcprotocol)

set(CMAKE_CXX_STANDARD 17)

add_library(rpcprotocol STATIC
        ArgList.cpp rpc_proc.cpp FixedThreadPool.cpp shared_memory.cpp
        RemoteException.cpp SharedBuffer.cpp IpcProxy.cpp IpcProxy.h LpcResult.cpp LpcResult.h Log.cpp Log.h io_utils.cpp io_utils.h)

set_target_properties(rpcprotocol PROPERTIES
        CXX_EXTENSIONS OFF
        POSITION_INDEPENDENT_CODE ON
        )

target_link_libraries(rpcprotocol)
