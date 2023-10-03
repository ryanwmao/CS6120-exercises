#include <stdio.h>

int main() {
  int x = 4;
  int y = 3;
  int j = x - y;
  printf("%d\n", j);
  if (j + 1) {
    printf("true!\n");
  }

  x = 1;
  if (!(x + 1)) {
    printf("false!\n");
  }


  return 0;
}
