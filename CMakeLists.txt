# Set the minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

project(services C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(JANSSON REQUIRED jansson)

include_directories(${GTK3_INCLUDE_DIRS})
link_directories(${GTK3_LIBRARY_DIRS})

add_definitions(${GTK3_CFLAGS_OTHER})

include_directories(${JANSSON_INCLUDE_DIRS})
link_directories(${JANSSON_LIBRARY_DIRS})

add_definitions(${JANSSON_CFLAGS_OTHER})

add_executable(services main.c)

target_link_libraries(services ${GTK3_LIBRARIES} ${JANSSON_LIBRARIES})

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/design.glade DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
