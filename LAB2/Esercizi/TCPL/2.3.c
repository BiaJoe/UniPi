#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#define BASE  16
#define FIRST 'a'
#define LAST  'f'

#define ERROR "\nErrore! Mettere un numero giusto\n\n"
#define NUN -1

int htoi(char* s); 
int htoiChar(char c);
int ctoi(char c);
int powInt(int b, int e);


int main(int argc, char *argv[]){
    int res;

    if(argc < 2 || (res = htoi(argv[1])) == NUN) {
        printf(ERROR);
        return 1;
    }

    printf("\n%s --> %d\n\n", argv[1], res);
    return 0;
}

int powInt(int b, int e){
    if(e < 0) return NUN;
    int res = 1;
    while(e > 0){
        res *= b;
        e--;
    }
    return res;
}

int ctoi(char c){
    char cs[] = {c, '\0'};
    return atoi(cs);
}

int htoiChar(char c){
    if(isdigit(c)) return ctoi(c);
    c = tolower(c);
    if(c < FIRST || c > LAST) return NUN;
    return c - FIRST + 10; 
}

int htoi(char* s){
    int res = 0;
    int len = strlen(s) - 2;
    int c16;

    if( 
        s[0] != '0' || 
        tolower(s[1]) != 'x' || 
        len < 1
    ) return NUN;

    for(int i = 2; s[i] != '\0'; i++){
        c16 = htoiChar(s[i]);
        if(c16 == NUN) return NUN;
        res += powInt(BASE, len -1) * c16;
        len--;
    }

    return res;
}