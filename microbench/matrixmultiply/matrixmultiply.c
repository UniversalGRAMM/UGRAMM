// matrix multiply
// Author: Andrew Canis
// Date: June 13, 2012

#include <stdio.h>

// matrices are SIZE x SIZE
#define SIZE 20

static int (*A1)[SIZE] = (int(*)[SIZE])0xA00;
static int (*B1)[SIZE] = (int(*)[SIZE])0xB00;

volatile int resultAB1[SIZE][SIZE];

__attribute__((noinline)) int multiply(int i, int j) {
    int k, sum = 0;
    for(k = 0; k < SIZE; k++) {
    //DFGLoop: loop
        sum += A1[i][k] * B1[k][j];
    }
    resultAB1[i][j] = sum;
    return sum;
}

int main(void) {
    int i, j;
    
    unsigned long long count = 0;
    for(i = 0; i < SIZE; i++) {
        for(j = 0; j < SIZE; j++) {
            count += multiply(i, j);
        }
    }

    printf ("Result: %lld\n", count);
    if (count == 962122000) {
        printf("RESULT: PASS\n");
    } else {
        printf("RESULT: FAIL\n");
    }
    return count;

}


