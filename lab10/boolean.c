#include <stdio.h>

int main() {
    int a = 1;
    int b = 0;

    int c = a && b;
    c = a || b;
    c = !a;
    

    int x = 5, y = 10;
    c = (x < y);
    c = (x > y);
    c = (x == y);
    c = (x != y);
    c = (x <= y);
    c = (x >= y);

    return 0;
}