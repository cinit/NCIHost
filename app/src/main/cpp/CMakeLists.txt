cmake_minimum_required(VERSION 3.10)

project(nci_host_natives)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

find_library( # Defines the name of the path variable that stores the
        # location of the NDK library.
        ANDROID_LIBS
        # Specifies the name of the NDK library that
        # CMake needs to locate.
        log)

add_subdirectory(rpcprotocol)
add_subdirectory(libnciclient)
add_subdirectory(ncihostd)
add_subdirectory(libbasehalpatch)
add_subdirectory(libnxphalpatch)
add_subdirectory(libqtiesepmpatch)
