cmake_minimum_required(VERSION 3.22)

find_package(Zephyr)
project(blinky_multiple_leds)

target_sources(app PRIVATE src/main.c)