cmake_minimum_required(VERSION 3.22)

find_package(Zephyr)
project(hello_world_print)

target_sources(app PRIVATE src/main.c)