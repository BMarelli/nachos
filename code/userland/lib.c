#include "lib.h"

unsigned strlen(const char *s)
{
  unsigned i;
  for (i = 0; s[i] != '\0'; i++)
    ;
  return i;
}

void puts2(const char *s)
{
  Write(s, sizeof(s), CONSOLE_OUTPUT);
}

void reverse(char s[])
{
  int i, j;
  char c;

  for (i = 0, j = strlen(s) - 1; i < j; i++, j--)
  {
    c = s[i];
    s[i] = s[j];
    s[j] = c;
  }
}

void itoa(int n, char str[])
{
  int i, sign;

  if ((sign = n) < 0)
    n = -n;
  i = 0;
  do
  {
    str[i++] = n % 10 + '0';
  } while ((n /= 10) > 0);
  if (sign < 0)
    str[i++] = '-';
  str[i] = '\0';
  reverse(str);
}
