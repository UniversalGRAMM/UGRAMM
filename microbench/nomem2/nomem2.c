#include <stdio.h>

static volatile int n = 10;

int main() {

    int N = n;
    int sum = 0;
    int i;
    int j = 0;
    for (i = 0; i < N; i++) {
        //DFGLoop: loop
        sum += i*3 + 5;
    }
    printf("sum = %d\n", sum);

    return sum;
}
