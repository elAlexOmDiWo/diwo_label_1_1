# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(periodic_adv)

target_sources(app PRIVATE src/main.c src/button.c src/button.h src/led.c src/led.h src/diwo_label.h src/bsp.c src/bsp.h src/battery.c src/battery.h src/wdt.h src/wdt.c)
zephyr_compile_definitions()
