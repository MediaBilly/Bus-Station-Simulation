#include <stdio.h>

int main(int argc, char const *argv[])
{
  void *ptr = NULL;
  int offset = 1;
  printf("%p %p\n",ptr,ptr + offset);
  int *num = ptr;
  printf("%p %p\n",num,num + offset);
  double *d = ptr;
  printf("%p %p\n",d,d + offset);
  return 0;
}
