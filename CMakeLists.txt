cmake_minimum_required(VERSION 2.8)

project(oldschoolengine2)

set(CMAKE_CXX_STANDARD 11)
add_definitions(-Wall -Wcast-qual -Wextra -Wshadow -fno-exceptions -fno-rtti -pedantic -flto -m64)

include_directories(src)

file(GLOB_RECURSE sourceFiles ./src/*.cpp)
file(GLOB_RECURSE headerFiles ./src/*.h)

set(CMAKE_EXECUTABLE_SUFFIX ".html")

set(linkFlags "-s DISABLE_EXCEPTION_CATCHING=1 -s TOTAL_MEMORY=64MB --shell-file ${CMAKE_CURRENT_LIST_DIR}/src/shell.html -s WASM=1 --preload-file diskimages")

set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_EXE_LINKER_FLAGS "${linkFlagsDebug} ${linkFlags}")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${linkFlagsDebug} ${linkFlags}")

add_executable(oldschoolengine2 ${sourceFiles} ${headerFiles})

set_target_properties(oldschoolengine2 PROPERTIES LINK_FLAGS_DEBUG "${linkFlagsDebug} ${linkFlags}")
set_target_properties(oldschoolengine2 PROPERTIES LINK_FLAGS_RELEASE "${linkFlags}")

