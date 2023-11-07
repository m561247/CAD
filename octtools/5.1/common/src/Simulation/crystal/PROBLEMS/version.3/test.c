#include <stdio.h>

main ()
{
	char *line;
	char c_line[255];

	line = "build /this/is/a/test";

	test (line);
	strcpy (c_line, "cmd2 /this/is/the/second/test a");
	test (c_line);
}

test (line)
char *line;
{
	int class, index, nargs;
	char cmd[100], residue[200];

	nargs = sscanf(line, " %99s %[^]", cmd, residue);

	printf ("INPUT LINE: %s\n", line);
	printf ("CMD: %s\n", cmd);
	printf ("RESIDUE: %s\n", residue);
	printf ("Number of args: %d\n", nargs);
}
