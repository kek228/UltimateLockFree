cmake_minimum_required(VERSION 3.15)
project(UltimateLockFree)

set(CMAKE_CXX_STANDARD 20)

add_executable(SeqCst MemoryModels/SeqCst.cpp)
