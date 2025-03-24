#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define SMALL 8
#define BIG   17

#define QUESTION            "Write your PP length please: "
#define WRONG_INPUT_WARNING "You must insert a number!"
#define RESULT              "Your PP status is: "
#define PUSSY               "pussy gang!"
#define MICROPENIS          "micropenis"
#define AVERAGE             "average"
#define HUGE                "HUGE!"

#define ASK(PROMPT_STRING_VARIABLE,VARIABLE)   printf("%s", PROMPT_STRING_VARIABLE); fflush(stdout); scanf("%s", VARIABLE);
#define PRINT(STRING)          printf(#STRING "\n");
#define PRINT_STRING(VARIABLE) printf("%s\n",VARIABLE);
#define LINE                   printf("\n");

#define BUFFER 100
#define ERROR  "ERROR"

char* handleLength(int length);
int isNumber(const char *string);

int main(){
  int correctInputFlag = 1, continueFlag = 0;
  char input[BUFFER], 
       result[BUFFER] = RESULT, 
       *question = QUESTION

  do {
    LINE
    ASK(question,input);
    if(isNumber(input)) correctInputFlag = 0;
    else { PRINT() }
  } while(correctInputFlag);

  strcat(result, handleLength(atoi(input)));
  PRINT_STRING(result)
  LINE
  return 0;
}

int isNumber(const char *string){
  while(*string){
    if(!isdigit(*string)) return 0;
    string++;
  }
  return 1;
}

char* resultMaker(char *result, char *string){
  strcat(result, string);
  return result;
}

char* handleLength(int length){
  if(length <= 0)     return PUSSY;
  if(length <= SMALL) return MICROPENIS;
  if(length <= BIG)   return AVERAGE;
  if(length > BIG)    return HUGE;
  return ERROR;
}
