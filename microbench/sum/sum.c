#include <stdio.h>

volatile int* n = (int*)0;
static int* a = (int*)(1*sizeof(int));

// Simple accumulation
int main() {

    int N = *n;
    int sum = 0;
    int i;
    for (i = 0; i < N; i++) {
        //DFGLoop: loop
        sum += a[i];
    }
    printf("sum = %d\n", sum);

    return sum;
}
