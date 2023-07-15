#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	if (argc < 1) {
		puts("Usage: c_compiler <integer>");
		exit(0);
	}
	puts("global main");
	puts("section .text");
	puts("main:");
	printf("mov rax, %d\n", atoi(argv[1]));
	puts("ret");
	return 0;
}