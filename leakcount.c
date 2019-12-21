#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>

extern char **environ;

int main(int argc, char **argv)
{
	char *program = "";
	if (argc > 1)
		program = argv[1];
	else {
		fprintf(stderr, "Improper usage.\n");
		return 0;
	}

	// Sets args for the test program based on argv
	char *args[argc];
	int index;
	for (index = 0; index < argc; index++)
	{
		args[index] = argv[index+1];
	}
	args[argc-1] = NULL;
	setenv("LD_PRELOAD","./memory_shim.so",1);

	execvpe(args[0], args, environ);
	fprintf(stderr, "Failed to execute program: %s", args[0]);

	return 0;
}
