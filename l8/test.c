#include <stdio.h>

int main() {
  int x = 4;
  int y = 5;
  int z = 6;
  for (int i = 0; i < 10; i++) {
    x = 3;
    y = 4;
    z = i + 3;
  }

  printf("x: %d\n", x);
  printf("y: %d\n", y);
  printf("z: %d\n", z);
  
  return 0;
}
