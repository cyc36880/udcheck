cmake_minimum_required(VERSION 3.15)

file(GLOB_RECURSE SOURCES "src/*.c")

add_library(udcheck ${SOURCES})
