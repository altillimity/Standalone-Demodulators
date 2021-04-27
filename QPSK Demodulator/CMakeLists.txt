cmake_minimum_required(VERSION 3.0)

project(QPSK-Demodulator)

if(APPLE)
    include_directories(/usr/local/include)
    link_directories(/usr/local/lib)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
endif()

add_executable(QPSK-Demodulator src/main.cpp src/window.cpp src/utils.cpp src/dsp.cpp src/constellation.cpp src/dsp/delay_one_imag.cpp)
add_executable(QPSK-Demodulator-Batch src/batch.cpp src/utils.cpp src/dsp.cpp  src/dsp/delay_one_imag.cpp)

set(wxWidgets_CONFIGURATION mswu)
find_package(wxWidgets COMPONENTS core base REQUIRED)
include(${wxWidgets_USE_FILE})

target_link_libraries(QPSK-Demodulator PUBLIC ${wxWidgets_LIBRARIES})

find_library(DSP_LIBRARY dsp REQUIRED)
target_link_libraries(QPSK-Demodulator PUBLIC ${DSP_LIBRARY})
target_link_libraries(QPSK-Demodulator-Batch PUBLIC ${DSP_LIBRARY})