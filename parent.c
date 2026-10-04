#include <stdint.h>
#include <stdbool.h>

#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

static char CHILD_PROGRAM_NAME[] = "child";

int main(int argc, char **argv) {
	char filename[256];
	ssize_t len = read(STDIN_FILENO, filename, sizeof(filename) - 1);
	if (len <= 0) {
		exit(EXIT_FAILURE);
	}
	filename[len - 1] = '\0';

	char progpath[1024];
	{
		ssize_t path_len = readlink("/proc/self/exe", progpath,
		                            sizeof(progpath) - 1);
		if (path_len == -1) {
			const char msg[] = "error: failed to read full program path\n";
			write(STDERR_FILENO, msg, sizeof(msg));
			exit(EXIT_FAILURE);
		}
		while (progpath[path_len] != '/')
			--path_len;

		progpath[path_len] = '\0';
	}

	int parent_to_child[2];
	if (pipe(parent_to_child) == -1) {
		const char msg[] = "error: failed to create pipe\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
	}

	const pid_t child = fork();

	switch (child) {
	case -1: {
		const char msg[] = "error: failed to spawn new process\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
	} break;

	case 0: {
		{
			pid_t pid = getpid();

			char msg[64];
			const int32_t length = snprintf(msg, sizeof(msg),
				"%d: I'm a child\n", pid);
			write(STDOUT_FILENO, msg, length);
		}

		close(parent_to_child[1]);

		dup2(parent_to_child[0], STDIN_FILENO);
		close(parent_to_child[0]);

		{
			char path[1024];
			snprintf(path, sizeof(path) - 1, "%s/%s", progpath, CHILD_PROGRAM_NAME);

			char *const args[] = {CHILD_PROGRAM_NAME, filename, NULL};

			int32_t status = execv(path, args);

			if (status == -1) {
				const char msg[] = "error: failed to exec into new exectuable image\n";
				write(STDERR_FILENO, msg, sizeof(msg));
				exit(EXIT_FAILURE);
			}
		}
	} break;

	default: {
		{
			pid_t pid = getpid();

			char msg[64];
			const int32_t length = snprintf(msg, sizeof(msg),
				"%d: I'm a parent, my child has PID %d\n", pid, child);
			write(STDOUT_FILENO, msg, length);
		}

		close(parent_to_child[0]);

		char buf[10];
		ssize_t bytes;

		while ((bytes = read(STDIN_FILENO, buf, sizeof(buf)))) {
			if (bytes < 0) {
				const char msg[] = "error: failed to read from stdin\n";
				write(STDERR_FILENO, msg, sizeof(msg));
				exit(EXIT_FAILURE);
			} else if (buf[0] == '\n') {
				break;
			}

			write(parent_to_child[1], buf, bytes);
		}

		close(parent_to_child[1]);

		wait(NULL);
		
	} break;
	}
}