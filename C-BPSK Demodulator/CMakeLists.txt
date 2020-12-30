cmake_minimum_required(VERSION 3.0)

project(C-BPSK-Demodulator)

if(APPLE)
    include_directories(/usr/local/include)
    link_directories(/usr/local/lib)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
endif()

add_executable(C-BPSK-Demodulator src/main.cpp src/window.cpp src/utils.cpp src/dsp.cpp src/constellation.cpp src/dsp/noaa_deframer.cpp)
add_executable(C-BPSK-Demodulator-Batch src/batch.cpp src/utils.cpp src/dsp.cpp src/dsp/noaa_deframer.cpp)

set(wxWidgets_CONFIGURATION mswu)
find_package(wxWidgets COMPONENTS core base REQUIRED)
include(${wxWidgets_USE_FILE})

target_link_libraries(C-BPSK-Demodulator PUBLIC ${wxWidgets_LIBRARIES})
target_link_libraries(C-BPSK-Demodulator-Batch PUBLIC ${wxWidgets_LIBRARIES})


find_library(DSP_LIBRARY dsp REQUIRED)
target_link_libraries(C-BPSK-Demodulator PUBLIC ${DSP_LIBRARY})
target_link_libraries(C-BPSK-Demodulator-Batch PUBLIC ${DSP_LIBRARY})