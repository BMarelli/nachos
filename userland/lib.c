#include "syscall.h"

// strlen returns the length of a string
unsigned int strlen(const char *str) {
    unsigned int len = 0;

    while (str[len] != '\0') len++;

    return len;
}

// puts writes a string to the console
int puts(const char *str) { return Write(str, strlen(str), CONSOLE_OUTPUT); }

// atoi converts a string to an integer
int atoi(const char *str) {
    int sign = 1;
    if (*str == '-') {
        sign = -1;
        str++;
    }

    int num = 0;
    while (*str != '\0') {
        num = num * 10 + (*str - '0');
        str++;
    }

    return num * sign;
}

// itoa converts an integer to a string
void itoa(int num, char *str) {
    if (num == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }

    int i = 0;
    if (num < 0) {
        str[i++] = '-';
        num = -num;
    }

    int n = num;
    while (n > 0) {
        n /= 10;
        i++;
    }

    str[i] = '\0';
    i--;
    while (num > 0) {
        str[i] = num % 10 + '0';
        num /= 10;
        i--;
    }
}
