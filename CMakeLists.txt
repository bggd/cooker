cmake_minimum_required(VERSION 3.12)

project(GLTF2VEProj C CXX)

add_executable(gltf2ve
  src/gltf2ve.cpp
  src/cgltf.c)
set_property(TARGET gltf2ve PROPERTY CXX_STANDARD 20)
