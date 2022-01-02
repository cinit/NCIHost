cmake_minimum_required(VERSION 3.10)

project(rpcprotocol)

set(CMAKE_CXX_STANDARD 17)

add_library(rpcprotocol STATIC
        protocol/ArgList.cpp utils/FixedThreadPool.cpp utils/shared_memory.cpp utils/SELinux.cpp
        utils/auto_close_fd.cpp utils/ProcessUtils.cpp
        protocol/RemoteException.cpp utils/SharedBuffer.cpp protocol/IpcTransactor.cpp protocol/LpcResult.cpp
        protocol/BaseIpcObject.cpp protocol/BaseIpcProxy.cpp INciHostDaemon.cpp
        log/Log.cpp utils/io_utils.cpp utils/Uuid.cpp utils/SignalHandler.cpp utils/FileMemMap.cpp utils/TextUtils.cpp
        log/SessionLog.h log/DefaultSessionLogImpl.cpp log/LogEntryRecord.cpp)

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    SET(CLANG_CXX_EXTRA_OPT "-Werror=unknown-warning-option -Werror=format-invalid-specifier -Werror=call-to-pure-virtual-from-ctor-dtor")
    SET(CLANG_C_EXTRA_OPT "-Werror=format-invalid-specifier")
else ()
    SET(CLANG_CXX_EXTRA_OPT "")
    SET(CLANG_C_EXTRA_OPT "")
endif ()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CLANG_CXX_EXTRA_OPT} -fPIC -Werror=delete-non-virtual-dtor -Werror=return-type -Werror=non-virtual-dtor -Wno-invalid-offsetof")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CLANG_C_EXTRA_OPT} -fPIC -Werror=return-type")

set_target_properties(rpcprotocol PROPERTIES
        CXX_EXTENSIONS OFF
        POSITION_INDEPENDENT_CODE ON
        )

target_link_libraries(rpcprotocol)
