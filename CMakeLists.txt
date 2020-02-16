cmake_minimum_required(VERSION 3.15)
project(UltimateLockFree)

set(CMAKE_CXX_STANDARD 20)

add_executable(SeqCst MemoryModels/SeqCst.cpp)
add_executable(Relexed MemoryModels/Relexed.cpp)
add_executable(AcqRel MemoryModels/AcqRel.cpp)
add_executable(ReleaseSequence MemoryModels/ReleaseSequence.cpp)

add_executable(V1_Stack LockFreeStack/V1_Stack.cpp)
add_executable(V2_Stack LockFreeStack/V2_Stack.cpp)

