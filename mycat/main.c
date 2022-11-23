#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

int main (int argc, const char **argv)
{
	for (int idx = 1; idx < argc; idx++)
	{
		pid_t pid = fork();

		if (!pid)
		{
			// Child
			
			int num = atoi (argv[idx]);

			usleep (num * 100);
			printf ("%d ", num);

			return 0;
		}
	}

	for (int i = 1; i < argc; i++) { wait (NULL); }

	putc ('\n', stdout);

	execl ("/usr/bin/python3", "python3", NULL);

	return 0;
}
