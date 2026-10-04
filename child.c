#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

int main(int argc, char **argv) {
    char buf[10];
    ssize_t bytes;

    int32_t file = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0600);
    if (file == -1) {
        const char msg[] = "error: failed to open requested file\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    while ((bytes = read(STDIN_FILENO, buf, sizeof(buf) - 1))) {
        if (bytes < 0) {
            const char msg[] = "error: failed to read from stdin\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE);
        }

        buf[bytes] = '\0';

        char *token = strtok(buf, " \t\n\r");
        if (!token) {
            continue;
        }

        char *endptr;
        float result = strtof(token, &endptr);
        if (endptr == token) {
            const char msg[] = "error: invalid number format\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            close(file);
            exit(EXIT_FAILURE);
            return -1;
        }
        
        int32_t divisor_count = 0;
        bool div_by_zero = false;

        while ((token = strtok(NULL, " \t\n\r")) != NULL) {
            float divisor = strtof(token, &endptr);
            if (endptr == token) {
                const char msg[] = "error: invalid number format\n";
                write(STDERR_FILENO, msg, sizeof(msg));
                close(file);
                exit(EXIT_FAILURE);
                return -1;
            }

            if (divisor == 0.0f) {
                div_by_zero = true;
                break;
            }

            result /= divisor;
            divisor_count++;
        }

        if (div_by_zero) {
            const char msg[] = "error: division by zero\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE);
        }

        if (divisor_count > 0) {
            char result_str[64];
            int32_t len = snprintf(result_str, sizeof(result_str), "%f\n", result);
            
            if (len > 0) {
                int32_t written = write(file, result_str, len);
                if (written != len) {
                    const char msg[] = "error: failed to write to file\n";
                    write(STDERR_FILENO, msg, sizeof(msg));
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    close(file);
}