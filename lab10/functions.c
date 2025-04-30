#include <stdio.h>

void print_multiple_three(int x) {
    if (x % 3 == 0) {
        printf("%d is a multiple of 3\n", x);
    }
}

int main() {
    int i = 0;

    while (i < 10) {
        print_multiple_three(i);
        i = i + 1;
    }

    return 0;
}
