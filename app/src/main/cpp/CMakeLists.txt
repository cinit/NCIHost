cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)

find_library( # Defines the name of the path variable that stores the
        # location of the NDK library.
        ANDROID_LIBS
        # Specifies the name of the NDK library that
        # CMake needs to locate.
        log)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Werror=format-invalid-specifier -Werror=return-type -Wno-invalid-offsetof")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -Werror=format-invalid-specifier -Werror=return-type -Wno-invalid-offsetof")

add_subdirectory(libnciclient)
add_subdirectory(ncihostd)
