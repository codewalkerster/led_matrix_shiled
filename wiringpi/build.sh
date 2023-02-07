#!/bin/sh
rm -rf *.o dot_shift_test
gcc -o dot_shift_test dot_shift_test.c -lwiringPi -lwiringPiDev -lpthread $(pkg-config --cflags --libs libwiringpi2)
