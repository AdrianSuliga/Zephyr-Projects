cmake_minimum_required(VERSION 3.22)

find_package(Zephyr)
project(zbus-test)

file(GLOB SRC_FILES "src/*.c")
target_sources(app PRIVATE ${SRC_FILES})


