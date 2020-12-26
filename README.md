# Standalone-Demodulators
A few standalones demodulators replacing GNU Radio flowcharts. Still WIP! But usable.

# QPSK Demodulator

Simple QPSK demodulator containing presets for common VHF, L-Band and X-Band satellites.

![Example while demodulating a MetOp AHRPT baseband](https://github.com/altillimity/Standalone-Demodulators/raw/master/images/QPSK.png)

# Building

This will require :
 - [libdsp](https://github.com/altillimity/libdsp)
 - [wxwidgets](https://www.wxwidgets.org/)

 Then, building is done by cmake :
 ```
git clone https://github.com/altillimity/Standalone-Demodulators.git
cd Standalone-Demodulators
mkdir build && cd build
cmake ..
make -j2
```

Windows builds are already included in this repo, so Windows users do not have to build from source.
