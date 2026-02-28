@echo off
cmake -G "MinGW Makefiles" -S . -B build
cmake --build build -- -j8
pause