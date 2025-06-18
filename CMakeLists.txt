
cmake_minimum_required(VERSION 3.23)
project(Stego)

set(CMAKE_CXX_STANDARD 17)

add_executable(Stego main.cpp cmake-build-debug/Algorithm.cpp cmake-build-debug/Algorithm.h audio.cpp audio.h Algorithms.cpp Algorithms.h)
include_directories(/Users/terroor/ffbase)
include_directories(/Users/terroor/ffaudio)
include_directories(/Users/terroor/ffos)