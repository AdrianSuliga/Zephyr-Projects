cmake_minimum_required(VERSION 3.22)

find_package(Zephyr)
project(mqtt-publish)

target_sources(app PRIVATE src/main.c)