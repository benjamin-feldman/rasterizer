cmake_minimum_required(VERSION 3.20)

project(rasterizer LANGUAGES CXX)

add_executable(rasterizer
  main.cpp
  clipping.cpp
  lighting.cpp
  model.cpp
  rasterizer.cpp
  renderer.cpp
)

target_compile_features(rasterizer PRIVATE cxx_std_20)
set_target_properties(rasterizer PROPERTIES
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
)

if(MSVC)
  target_compile_options(rasterizer PRIVATE /W4)
else()
  target_compile_options(rasterizer PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/head.OBJ")
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/head.OBJ"
                 "${CMAKE_CURRENT_BINARY_DIR}/head.OBJ"
                 COPYONLY)
endif()
