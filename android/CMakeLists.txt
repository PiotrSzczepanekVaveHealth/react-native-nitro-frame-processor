project(nitroframeprocessor)
cmake_minimum_required(VERSION 3.9.0)

set(PACKAGE_NAME nitroframeprocessor)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 20)

set(CVIE_SUPPORTED_ANDROID_ABIS arm64-v8a)
list(FIND CVIE_SUPPORTED_ANDROID_ABIS "${ANDROID_ABI}" CVIE_ANDROID_ABI_INDEX)
if(CVIE_ANDROID_ABI_INDEX EQUAL -1)
  set(CVIE_ANDROID_ABI_SUPPORTED FALSE)
  set(FRAME_PROCESSOR_SOURCE ../cpp/FrameProcessorUnavailable.cpp)
  message(STATUS "Building ${PACKAGE_NAME} without CVIE for unsupported ABI ${ANDROID_ABI}")
else()
  set(CVIE_ANDROID_ABI_SUPPORTED TRUE)
  set(FRAME_PROCESSOR_SOURCE ../cpp/FrameProcessor.cpp)
  message(STATUS "Building ${PACKAGE_NAME} with CVIE for ABI ${ANDROID_ABI}")
endif()

# Define C++ library and add all sources
add_library(
  ${PACKAGE_NAME}
  SHARED
  src/main/cpp/cpp-adapter.cpp
  ../cpp/NitroFrameProcessorCpp.cpp
  ${FRAME_PROCESSOR_SOURCE}
  ../cpp/NeedleEnhancement.cpp
)

# Add Nitrogen specs :)
include(${CMAKE_SOURCE_DIR}/../nitrogen/generated/android/nitroframeprocessor+autolinking.cmake)

# Set up local includes
include_directories("src/main/cpp" "../cpp" "CVIESDK/include")

find_library(LOG_LIB log)

# Link all libraries together
target_link_libraries(
        ${PACKAGE_NAME}
        ${LOG_LIB}
        android # <-- Android core
)

if(CVIE_ANDROID_ABI_SUPPORTED)
  add_library(cvie64 SHARED IMPORTED)
  set_target_properties(cvie64 PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/CVIESDK/bin/${ANDROID_ABI}/libcvie64.so")
  target_link_libraries(${PACKAGE_NAME} cvie64)
endif()
