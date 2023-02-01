project(monprobe)
cmake_minimum_required(VERSION 2.8)
MESSAGE( STATUS "PROJECT_SOURCE_DIR: " ${PROJECT_SOURCE_DIR} )
#include_directories("/home/dvb/include/")
include_directories(${PROJECT_SOURCE_DIR})
aux_source_directory(. SRC_LIST)
list(APPEND CMAKE_CXX_FLAGS "-Werror -std=c++14 -Werror -O2 -static-libstdc++ -s -Bstatic -pthread")
#list(APPEND CMAKE_CXX_FLAGS "-Wall -Wextra -Wshadow -Wformat=2 -Wlogical-op -std=c++14 -Werror -O2 -static-libstdc++ -s -Bstatic -pthread")
set(HTTP_LIBRARY ${PROJECT_SOURCE_DIR}/lib64/libcivetweb19.a)
add_executable(${PROJECT_NAME} ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} ${HTTP_LIBRARY} dl)

 
