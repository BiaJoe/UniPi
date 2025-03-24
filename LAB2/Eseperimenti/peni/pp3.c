#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define SMALL 8
#define BIG 17

#define BUFFER 20
#define ERROR "ERROR"

char* handleLength(int length);
int isNumber(const char *string);

int main(){
  int len;
  char input[BUFFER];

  printf("Write your PP length please: ");
  fflush(stdout);
  scanf("%s", input);

  len = atoi(input);
  printf("%s\n", handleLength(len));

  return 0;
}

char* handleLength(int length){
  if(length <= 0)     return "pussy gang!";
  if(length <= SMALL) return "micropenis";
  if(length <= BIG)   return "average";
  if(length > BIG)    return "HUGE!";
  return ERROR;
}
