#include <stdio.h>

int main() {
    int a = 5;
    int b = 10;      // dead
    int c = a + 2;
    int d = c * 3;   // dead

    if (a > 0)
        goto L1;
    else
        goto L1;

L1:
    printf("%d\n", c);
    return 0;
}