# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(periodic_adv)

target_sources(app PRIVATE src/main.c src/button.c src/button.h src/led.c src/led.h src/diwo_label.h src/bsp.c src/bsp.h src/battery.c src/battery.h src/wdt.h src/wdt.c src/main.h src/nfc.c src/nfc.h src/acc_lis2_de.c src/power.c src/setting.c)
zephyr_include_directories("B:\\Projects\\nRF52\\v2.3.0\\zephyr\\drivers\\sensor\\lis2dh")
zephyr_compile_definitions()
set_source_files_properties(src/peripheral.c src/service.c PROPERTIES HEADER_FILE_ONLY TRUE)
