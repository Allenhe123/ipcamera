cmake_minimum_required(VERSION 3.10)
project(onvif)

AUX_SOURCE_DIRECTORY(. DIR_SRCS)

add_library(onvif SHARED ${DIR_SRCS})
target_link_libraries(onvif swresample ssl crypto)
