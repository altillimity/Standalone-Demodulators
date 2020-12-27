cmake_minimum_required(VERSION 3.0)

project(C-BPSK-Demodulator)
file(GLOB_RECURSE C-BPSK-Demodulator_CPPS src/*.cpp)

if(APPLE)
    include_directories(/usr/local/include)
    link_directories(/usr/local/lib)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
endif()

add_executable(C-BPSK-Demodulator ${C-BPSK-Demodulator_CPPS})

set(wxWidgets_CONFIGURATION mswu)
find_package(wxWidgets COMPONENTS core base REQUIRED)
include(${wxWidgets_USE_FILE})

target_link_libraries(C-BPSK-Demodulator PUBLIC ${wxWidgets_LIBRARIES})

find_library(DSP_LIBRARY dsp REQUIRED)
target_link_libraries(C-BPSK-Demodulator PUBLIC ${DSP_LIBRARY})