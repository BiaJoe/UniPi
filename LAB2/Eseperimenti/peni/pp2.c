#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define SMALL 8
#define BIG 17

#define PRINT(S) printf("#S\n");
#define PRINT_STRING(X) printf("%s\n",X);
#define END return 0;
#define ERROR "ERROR"

char* handleLength(int length);
int isNumber(const char *string);

int main(int argc, char* argv[]){
  int len;
  char* res;
  
  if(argc < 2 || !isNumber(argv[1])) {
    PRINT(Usage: argv[0] <penis length (number)>)
    return 1;
  }

  len = atoi(argv[1]);
  res = handleLength(len);
  PRINT_STRING(res)

  END;
}

int isNumber(const char *string){
  while(*string){
    if(!isdigit(*string)) return 0;
    string++;
  }
  return 1;
}

char* handleLength(int length){
  if(length <= 0)     return "pussy gang!";
  if(length <= SMALL) return "micropenis";
  if(length <= BIG)   return "average";
  if(length > BIG)    return "huge!";
  return ERROR;
}
