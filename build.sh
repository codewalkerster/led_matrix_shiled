#!/bin/sh
rm -rf *.o dot_test dot_shift_test
gcc -o dot_test dot_test.c
gcc -o dot_shift_test dot_shift_test.c
