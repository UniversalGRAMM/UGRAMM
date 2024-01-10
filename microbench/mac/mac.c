#include <stdio.h>

volatile int* N = (int*)0;
static int* a = (int*)(1*sizeof(int));
static int* b = (int*)(9*sizeof(int));

int main() {
    int i;
    int n = *N;

    int sum = 0;

    for (i = 1; i < n-1; i++) {
    //DFGLoop: loop
        sum += a[i] * b[i];
    }

    return sum;
}
