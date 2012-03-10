#include <stdio.h>

void fill_binary_string(int val, char * p)
{
	int i;

	for(i = 0; i < 8; i++) {
		p[ i+((i<4)?0:1) ] = (0 == (val & 0x80)) ? '0' : '1';
		val = val << 1;
	}
	p[4] = ' ';
	p[9] = '\0';
}

int main(int argc, char * argv[])
{
	int i;
	char b[10];

	for(i = 0; i < 16; i++) {
		fill_binary_string(i, b);

		printf("0x%X   (%2d)      %s\n", i, i, b);
	}

	printf("--------\n");

	for(i = 0; i < 26; i++) {
		int v;

		v = (int)('a') + i;
		fill_binary_string(v, b);
		printf("0x%X   (%c/%3d)  %s\n", v, (char)(v), v, b);
	}

	printf("--------\n");

	for(i = 0; i < 26; i++) {
		int v;

		v = (int)('A') + i;
		fill_binary_string(v, b);
		printf("0x%X   (%c/%3d)  %s\n", v, (char)(v), v, b);
	}

	printf("--------\n");

	for(i = 0; i < 10; i++) {
		int v;

		v = (int)('0') + i;
		fill_binary_string(v, b);
		printf("0x%X   (%c/%3d)  %s\n", v, (char)(v), v, b);
	}

	return 0;
}

