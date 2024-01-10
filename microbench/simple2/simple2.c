// Simple loop pipelining test
// Author: Andrew Canis
// Date: June 29, 2012

#include <stdio.h>

#define SIZE 20

volatile int* a = (int*)0x100;
volatile int* b = (int*)0x200;

int main(void) {

    int i;
    volatile int mul[SIZE];

    for (i = 0; i < SIZE; i++) {
        //DFGLoop: loop
        mul[i] = a[i] * b[i];
    }

    return mul[SIZE-1];
}
