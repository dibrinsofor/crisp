#include <stdio.h>
#include <stdbool.h>


void pString(int n) {
	while (n > 0) {
		printf("Hell0 W0rld!\n");
		n = n - 1;
	}
	return;
}

int main(int argc, char** argv) {

	bool check = true;

	int i;
	char c;
	char *d = "Dibri's string in C";

	i = 2;
	c = 'c';

	if (check || check < 1) {
		printf("Check is true\n");
	} else {
		printf("Check is not true\n");
	}

	while (i > 0) {
		puts("Looping tracksss\n");
		i -= 1;
	}

	for (int i = 0; i < 5; i++) {
		// printf("%dth loop. freaky, freaky, freaky fresh\n", i);
		printf("Hello, World!\n");
	}

	int c2 = 5;
	while (c2 >= 0) {
		printf("Hello, W0rld!\n");
		c2 -= 1;
	}


	pString(7);

	// puts("Hello, world!\n");
	printf("%i is %c and %s\n", i, c, d);
	return 0;
}
