#include <stdio.h>

volatile int * N = (int*)0xf00;
static int *a = (int*)0xa00;
static int *b = (int*)0xb00;
static int *m = (int*)0xc00;

int main() {
    int i, j;
    int n = *N;

    int sum = 0;

    for (i = 1; i < n-1; i++) {
        //DFGLoop: loop
        sum += (a[i] * 2 + a[i + 1] * 2) * (b[i] * 2 + b[i + 3] * 2) * (b[i + 3] * 3) * b[i];
    }

    return sum;
}
