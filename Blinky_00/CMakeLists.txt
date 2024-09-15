cmake_minimum_required(VERSION 3.22)
set(BOARD esp32)

find_package(Zephyr)
project(blinky_external_led)

target_sources(app PRIVATE src/main.c)
