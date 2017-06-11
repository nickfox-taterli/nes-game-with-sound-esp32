ESP32 Game Playes with NES
====================================================================

Play NES Game with sound, emulate a NES to full speed, albeit with some framedrop due to the way the display is driven.

Warning
-------

Not any unsupported by anyone.


Compiling
---------

This code is an esp-idf project. You will need esp-idf to compile it. Newer versions of esp-idf may introduce incompatibilities with this code;


Display
-------

To display the NES output, please connect a 320x240 ili9341-based SPI display (or ST7735.) to the ESP32 in this way:

    =====  =======================
    Pin    GPIO
    =====  =======================
    MISO   25
    MOSI   23
    CLK    19
    CS     22
    DC     21
    RST    18
    BCKL   5
    =====  =======================

(BCKL = backlight enable)

Also connect the power supply and ground. For now, the LCD is controlled using a SPI peripheral, fed using the 2nd CPU. This is less than ideal; feeding
the SPI controller using DMA is better, but was left out due to this being a proof of concept.


Controller
----------

To control the NES, connect TM1638 with key as such:

    =====  =======================
    TM1638    GPIO
    =====  =======================
    STB    4
    CLK    16
    DIO    17
    =====  =======================
	
    =====  =====
    K      GPIO
    =====  =====
    RST    27
    =====  =====	

Also connect the power and ground lines. TM1638 Only Support 5V VCC.

Sound
----------

Also connect to PAM8403 etc:

    =====  =====
    Pin    GPIO
    =====  =====
    DAC    26
    =====  =====

ROM
---
This NES emulator does not come with a ROM. Please supply your own and flash to address 0xF0000. You can use the flashrom.sh script as a template for doing so.But ROM must be smaller than your (Flash Size - 1MB).

