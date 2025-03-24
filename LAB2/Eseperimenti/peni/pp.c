#include <stdio.h>
#define PISELLINO_LUNGHEZZA 9
#define PISELLINO_CARATTERE 'p'
#define PISELLONE_LUNGHEZZA 33
#define PISELLONE_CARATTERE 'P'
#define NON_PISELLO 0

void main(){
  int c = getchar();

  if(c == PISELLINO_CARATTERE) {
    printf("%d\n", PISELLINO_LUNGHEZZA);
    return;  
  }

  if(c == PISELLONE_CARATTERE) {
    printf("%d\n", PISELLONE_LUNGHEZZA);
    return;
  }

  printf("%d\n", NON_PISELLO);
  
  //return 0;
}

