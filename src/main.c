#include <file.h>

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>


void print_usage(char *app)
{
	printf("Usage: %s [file]\n", app);
}

/* Parse and validate user params */
static bool parse_args(int argc, char *argv[], char *path)
{
	if (argc < 1 || argc > 2) {
		fprintf(stderr, "Error; Invalid argument count\n");
		print_usage(argv[0]);
		return false;
	}

	/* Parse file path */
	if (argc == 2) {
		path = argv[1];
		if(!file_exist(path)) {
			fprintf(stderr, "Error: File %s doesn't exist\n",
				path);
			return false;
		}
	} else {
		path = NULL;
	}

	return true;
}

int main(int argc, char *argv[])
{
	bool res;
	char *p = NULL;

	res = parse_args(argc, argv, p);
	if (!res)
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}
