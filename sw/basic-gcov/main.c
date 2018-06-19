#include <stdio.h>

int main (void) {
  int i;

  for(i=1;i<13;i++)
  {
    if(i%2 == 0)
    printf("%d is divisible by 2 \n",i);

    if(i%5==0)
    printf("%d is divisible by 5 \n",i);

    if (i%14==0)
    printf("%d is divisible by 14 \n",i);
  }

  return 0;
}
