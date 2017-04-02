
BOARD_TAG=uno
USER_LIB_PATH := $(realpath ../)
ARDUINO_LIBS := TimerOne PinChangeInt
ISP_PROG := usbtiny

include /usr/share/arduino/Arduino.mk

