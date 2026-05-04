project(nitroframeprocessor)
cmake_minimum_required(VERSION 3.9.0)

set(PACKAGE_NAME nitroframeprocessor)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 20)

# Define C++ library and add all sources
add_library(
  ${PACKAGE_NAME}
  SHARED
  src/main/cpp/cpp-adapter.cpp
  ../cpp/NitroFrameProcessorCpp.cpp
  ../cpp/FrameProcessor.cpp
)

# Add Nitrogen specs :)
include(${CMAKE_SOURCE_DIR}/../nitrogen/generated/android/nitroframeprocessor+autolinking.cmake)

# Set up local includes
include_directories("src/main/cpp" "../cpp" "CVIESDK/include")

add_library(cvie64 SHARED IMPORTED)
set_target_properties(cvie64 PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/CVIESDK/bin/${ANDROID_ABI}/libcvie64.so")

find_library(LOG_LIB log)

# Link all libraries together
target_link_libraries(
        ${PACKAGE_NAME}
        cvie64
        ${LOG_LIB}
        android # <-- Android core
)
