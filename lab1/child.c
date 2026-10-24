#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdbool.h>

bool is_prime(long long num) {
    if (num < 2) return false;
    for (long long i = 2; i*i <= num; i++) {
        if (num%i == 0) return false;
    }
    return true;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        const char msg[] = "error: file path not provided to child\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    const char *filepath = argv[1];

    char buffer[1024];
    ssize_t bytes = 0;
    ssize_t written = 0;

    while ((bytes = read(STDIN_FILENO, buffer, sizeof(buffer) - 1))) {
        if (bytes < 0) {
            const char msg[] = "error: failed to read from stdin\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            exit(EXIT_FAILURE);
        }

        buffer[bytes] = '\0';

        long long num = atoll(buffer);

        if (num < 0 || is_prime(num)) {
            const char term_msg[] = "TERMINATE\n";
            write(STDOUT_FILENO, term_msg, sizeof(term_msg) - 1);
            exit(EXIT_SUCCESS);
        }

        int fd = open(filepath, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd < 0) {
            const char msg[] = "error: failed to open target file\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            exit(EXIT_FAILURE);
        }

        written = write(fd, buffer, bytes);
        if (written != bytes) {
            const char msg[] = "error: failed to write to file\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            close(fd);
            exit(EXIT_FAILURE);
        }

        close(fd);

        written = write(STDOUT_FILENO, buffer, bytes);
        if (written != bytes) {
            const char msg[] = "error: failed to write to stdout\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            exit(EXIT_FAILURE);
        }
    }

    exit(EXIT_SUCCESS);
}