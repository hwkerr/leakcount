#include <stdlib.h>

int main()
{
  void *p1, *p2;
  p1 = malloc(1345);
  p2 = malloc(2);
  free(p1);
  free(p2);

  return 0;
}
