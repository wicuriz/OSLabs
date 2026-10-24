#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <limits.h>
#include <string.h>
#include <mach-o/dyld.h>

int main() {
	char raw_path[PATH_MAX];
	uint32_t size = sizeof(raw_path);

	if (_NSGetExecutablePath(raw_path, &size) != 0) {
		const char msg[] = "error: failed to read full program path\n";
		write(STDERR_FILENO, msg, sizeof(msg) - 1);
		exit(EXIT_FAILURE);
	}
	char path[PATH_MAX];
	if (realpath(raw_path, path) == NULL) {
		strncpy(path, raw_path, sizeof(path));
	}
	uint32_t len = sizeof(path);
	while(path[len] != '/') {
		len--;
	}
	path[len] = '\0';

	char file_name[NAME_MAX];
	ssize_t file_name_len = read(STDIN_FILENO, file_name, sizeof(file_name) - 1);
	if (file_name_len <= 0) {
		const char msg[] = "error: failed to read file name\n";
		write(STDERR_FILENO, msg, sizeof(msg) - 1);
		exit(EXIT_FAILURE);
	}
	if (file_name[file_name_len - 1] == '\n') {
		file_name[file_name_len - 1] = '\0';
	} else {
		file_name[file_name_len] = '\0';
	}
	char file_path[PATH_MAX + NAME_MAX + 2];
	snprintf(file_path, sizeof(file_path), "%s/%s", path, file_name);

	int parent_to_child[2];
	if (pipe(parent_to_child) == -1) {
		const char msg[] = "error: failed to create a 'parent_to_child' pipe\n";
		write(STDERR_FILENO, msg, sizeof(msg) - 1);
		exit(EXIT_FAILURE);
	}

	int child_to_parent[2];
	if (pipe(child_to_parent) == -1) {
		const char msg[] = "error: failed to create a 'child_to_parent' pipe\n";
		write(STDERR_FILENO, msg, sizeof(msg) - 1);
		exit(EXIT_FAILURE);
	}

	const pid_t child_1 = fork();
	if (child_1 == -1) {
		const char msg[] = "error: failed to spawn a new process\n";
		write(STDERR_FILENO, msg, sizeof(msg) - 1);
		exit(EXIT_FAILURE);
	}
	if (child_1 == 0) {
		close(parent_to_child[1]);
		close(child_to_parent[0]);

		dup2(parent_to_child[0], STDIN_FILENO);
		close(parent_to_child[0]);

		dup2(child_to_parent[1], STDOUT_FILENO);
		close(child_to_parent[1]);

		char process_path[PATH_MAX + NAME_MAX + 2];
		snprintf(process_path, sizeof(process_path), "%s/child", path);

		execlp(process_path, "child", file_path, NULL);

		const char msg[] = "failed to exec into new executable image\n";
		write(STDERR_FILENO, msg, sizeof(msg) - 1);
		exit(EXIT_FAILURE);
	}

	close(parent_to_child[0]);
	close(child_to_parent[1]);

	char buffer[1024];
	ssize_t bytes = 0;
	ssize_t written = 0;

	while ((bytes = read(STDIN_FILENO, buffer, sizeof(buffer)))) {
		if (bytes < 0) {
			const char msg[] = "error: failed to read from stdin\n";
			write(STDERR_FILENO, msg, sizeof(msg) - 1);
			exit(EXIT_FAILURE);
		} else if (buffer[0] == '\n') {
			break;
		}

		written = write(parent_to_child[1], buffer, bytes);
		if (written != bytes) {
			const char msg[] = "error: failed to write to parent_to_child\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            exit(EXIT_FAILURE);
		}

		bytes = read(child_to_parent[0], buffer, sizeof(buffer));
		if (bytes < 0) {
            const char msg[] = "error: failed to read from child_to_parent\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            exit(EXIT_FAILURE);
        }
		buffer[bytes] = '\0';

		if (strncmp(buffer, "TERMINATE", 9) == 0) {
			break;
		}
		written = write(STDOUT_FILENO, buffer, bytes);
		if (written != bytes) {
			const char msg[] = "error: failed to write to stdout\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            exit(EXIT_FAILURE);
		}
	}

	close(parent_to_child[1]);
	close(child_to_parent[0]);

	wait(NULL);

	exit(EXIT_SUCCESS);
}
