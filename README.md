# Standalone-Demodulators
A few standalones demodulators replacing GNU Radio flowcharts. Still WIP! But usable.

# QPSK Demodulator

Simple QPSK demodulator containing presets for common VHF, L-Band and X-Band satellites.
This includes :
 - MetOp-A/B/C
 - FengYun-3A/B/C
 - SNPP, JPSS-1
 - X-Band FengYun MPT
 - FY3D
 - Aqua
 - and others!

![Example while demodulating a MetOp AHRPT baseband](https://github.com/altillimity/Standalone-Demodulators/raw/master/images/QPSK.png)

# C-BPSK Demodulator

Simple BPSK (split phase with center carrier) demodulator containing presets for common L-Band satellites.
This includes :
 - METEOR-M2/N2-2
 - NOAA 15 to 19

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
