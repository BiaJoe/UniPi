#include <stdio.h>

void main(){
  char str[20];
  printf("inserisci stringa: ");
  fflush(stdout);
  scanf("%s", str);

  printf("hai scritto: %s\n", str);
}