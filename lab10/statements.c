#include <stdio.h>

int main() {
    int i = 0;
    while (i < 20) {
        if (i % 3 == 1) {
            printf("%d\n", i);
        }
        i = i + 1;
    }
    return 0;
}
