


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define ABS(x) ((x) < 0 ? -(x) : (x))
#define MANHATTAN(x1,y1,x2,y2) (ABS((x1) - (x2)) + ABS((y1) - (y2)))


int main(){

	int x1 = 1;
	int y1 = 1;
	int x2 = 5;
	int y2 = 5;


	int d = MANHATTAN(x1,y1,x2,y2);
	printf("d((%d, %d), (%d, %d)) = %d\n", x1,y1,x2,y2,d);

	return 0;
}