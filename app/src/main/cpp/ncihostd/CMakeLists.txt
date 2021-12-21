cmake_minimum_required(VERSION 3.10)

project(ncihostd)

set(CMAKE_CXX_STANDARD 17)

add_executable(ncihostd startup/startup.c startup/daemon.cpp ipc/IpcStateController.cpp
        service/front/NciHostDaemonImpl.cpp service/front/NciClientProxy.cpp service/HwServiceStatus.cpp
        elfsym/ElfView.cpp elfsym/ProcessView.cpp inject/Injector.cpp inject/arch/ptrace_inject_utils.cpp
        inject/arch/ptrace_inject_arm_impl.cpp inject/arch/ptrace_inject_x86_impl.cpp inject/SysServicePatch.cpp
        service/ServiceManager.cpp service/hw/BaseHwHalHandler.cpp service/hw/nxpnci/NxpHalHandler.cpp
        service/xposed/BaseRemoteAndroidService.cpp service/xposed/AndroidNfcService.cpp)

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    SET(CLANG_CXX_EXTRA_OPT "-Werror=unknown-warning-option -Werror=format-invalid-specifier -Werror=call-to-pure-virtual-from-ctor-dtor")
    SET(CLANG_C_EXTRA_OPT "-Werror=format-invalid-specifier")
else ()
    SET(CLANG_CXX_EXTRA_OPT "")
    SET(CLANG_C_EXTRA_OPT "")
endif ()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CLANG_CXX_EXTRA_OPT} -fPIC -Werror=delete-non-virtual-dtor -Werror=return-type -Werror=non-virtual-dtor -Wno-invalid-offsetof")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CLANG_C_EXTRA_OPT} -fPIC -Werror=return-type")

set_target_properties(ncihostd PROPERTIES
        CXX_EXTENSIONS OFF
        POSITION_INDEPENDENT_CODE ON
        )

set_target_properties(ncihostd PROPERTIES
        PREFIX "lib"
        SUFFIX ".so"
        )

include_directories(..)

target_compile_definitions(ncihostd PRIVATE NCI_HOST_VERSION=\"${NCI_HOST_VERSION}\")

target_link_libraries(ncihostd c dl rpcprotocol ${ANDROID_LIBS})
