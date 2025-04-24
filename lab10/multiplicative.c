#include <stdio.h>

int main() {
    int a = 5, b = 3, c;
    double x = 2.5, y = 4.0, z;

    c = a * b;
    z = x * y;

    printf("Multiplication of doubles: %f * %f = %f\n", x, y, z);

    c = a / b;
    z = x / y;

    c = a % b;

    return 0;
}