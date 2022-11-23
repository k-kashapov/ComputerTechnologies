#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

const int BUFF_SIZE = 4096;

ssize_t safe_write (int fd_dest, const char *src, size_t num)
{
	ssize_t bytes_written = 0;

	while (bytes_written < num)
	{
		ssize_t write_ret = write (fd_dest, 
								   src + bytes_written,
								   num - bytes_written);
		if (write_ret < 0)
		{
			return write_ret;
		}

		bytes_written += write_ret;
	}

	return bytes_written;
}

ssize_t mycat (int fd_src, int fd_dest)
{
	char buff[BUFF_SIZE];

	while (true)
	{
		ssize_t read_ret = read (fd_src, buff, BUFF_SIZE);
		if (read_ret < 0)
		{
			perror ("Error reading from file descriptor");
			return read_ret;
		}

		if (read_ret == 0)
		{
			break;
		}

		ssize_t write_ret = safe_write (fd_dest, buff, read_ret);	
		if (write_ret < 0)
		{
			perror ("Error writing to file descriptor");
			return write_ret;
		}
	}

	return 0;
}

int main (int argc, const char **argv)
{
	if (argc < 2)
	{
		return mycat (0, 1);
	}

	for (int iter = 1; iter < argc; iter++)
	{
		int curr_fd = open (argv[iter], O_RDONLY);

		#ifdef _DEBUG
		printf ("FILE: %s\n", argv[iter]);
		#endif // _DEBUG

		if (curr_fd < 0)
		{
			fprintf (stderr, "mycat: %s: %s\n", argv[iter], strerror (errno));
			return curr_fd;
		}

		ssize_t cat_val = mycat (curr_fd, 1);
		close (curr_fd);

		if (cat_val < 0)
		{
			return cat_val;
		}
	}

	return 0;
}
