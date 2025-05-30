#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>

#define COLOR_RESET   "\x1b[0m"
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_WHITE   "\x1b[37m"

#define PORT 8080
#define BUFFER_SIZE 1024
#define RECONNECT_TIMEOUT 180  // 3 minutes in seconds

struct termios orig_termios;

void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);  // Turn off echo and canonical mode
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void redraw_line(const char *prompt, const char *input_buf) {
    printf("\r\033[K%s%s%s%s", COLOR_YELLOW, prompt, input_buf, COLOR_RESET);
    fflush(stdout);
}

int connect_to_server() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation");
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(sockfd);
        return -1;
    }

    return sockfd;
}

int attempt_reconnect(const char *name) {
    time_t start_time = time(NULL);
    time_t current_time;
    int sockfd;

    printf("\n%sServer disconnected. Attempting to reconnect...%s\n", COLOR_RED, COLOR_RESET);

    while (1) {
        current_time = time(NULL);
        if (current_time - start_time >= RECONNECT_TIMEOUT) {
            printf("%sReconnection timeout. Giving up after 3 minutes.%s\n", COLOR_RED, COLOR_RESET);
            return -1;
        }

        sockfd = connect_to_server();
        if (sockfd >= 0) {
            printf("%sReconnected to server!%s\n", COLOR_GREEN, COLOR_RESET);
            // Send name again
            send(sockfd, name, strlen(name), 0);
            return sockfd;
        }

        printf("\r%sReconnecting... (%d seconds remaining)%s",
               COLOR_YELLOW,
               (int)(RECONNECT_TIMEOUT - (current_time - start_time)),
               COLOR_RESET);
        fflush(stdout);

        sleep(2);  // Wait 2 seconds before next attempt
    }
}

int is_arrow_key_sequence(char ch, char *seq_buffer, int *seq_pos) {
    if (ch == 27) {  // ESC character
        *seq_pos = 1;
        seq_buffer[0] = ch;
        return 0;  // Not complete yet
    }

    if (*seq_pos == 1 && ch == '[') {
        seq_buffer[1] = ch;
        *seq_pos = 2;
        return 0;  // Not complete yet
    }

    if (*seq_pos == 2 && (ch == 'A' || ch == 'B')) {  // Up or Down arrow
        *seq_pos = 0;  // Reset sequence
        return 1;  // Complete arrow key sequence, ignore it
    }

    // Reset if it's not an arrow key sequence
    if (*seq_pos > 0 && (ch != '[' || *seq_pos != 1)) {
        *seq_pos = 0;
    }

    return 0;
}

int main() {
    int sockfd;
    char buffer[BUFFER_SIZE];
    char input_buf[BUFFER_SIZE] = {0};
    int input_len = 0;
    char name[50];
    char arrow_seq_buffer[3];
    int arrow_seq_pos = 0;

    // Initial connection
    sockfd = connect_to_server();
    if (sockfd < 0) {
        printf("%sCannot connect to server. Make sure the server is running.%s\n", COLOR_RED, COLOR_RESET);
        exit(EXIT_FAILURE);
    }

    printf("%sEnter your name: %s", COLOR_CYAN, COLOR_RESET);
    fflush(stdout);
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = '\0';
    send(sockfd, name, strlen(name), 0);

    char prompt[64];
    snprintf(prompt, sizeof(prompt), "[%s]: ", name);

    enable_raw_mode();

    while (1) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(sockfd, &read_fds);
        int maxfd = sockfd + 1;

        if (select(maxfd, &read_fds, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) continue;  // Interrupted by signal, continue
            perror("select");
            break;
        }

        // Incoming message
        if (FD_ISSET(sockfd, &read_fds)) {
            int n = read(sockfd, buffer, sizeof(buffer) - 1);
            if (n <= 0) {
                close(sockfd);
                disable_raw_mode();

                sockfd = attempt_reconnect(name);
                if (sockfd < 0) {
                    printf("\nExiting...\n");
                    exit(EXIT_FAILURE);
                }

                enable_raw_mode();
                redraw_line(prompt, input_buf);
                continue;
            }
            buffer[n] = '\0';

            // Don't change colors - server already sends colored messages
            printf("\r\033[K%s\n", buffer); // Clear line, print msg as-is
            redraw_line(prompt, input_buf); // Reprint typed input
        }

        // User typing
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            char ch;
            if (read(STDIN_FILENO, &ch, 1) > 0) {
                // Handle arrow key sequences
                if (is_arrow_key_sequence(ch, arrow_seq_buffer, &arrow_seq_pos)) {
                    continue;  // Ignore arrow keys
                }

                // Skip if we're in the middle of an arrow key sequence
                if (arrow_seq_pos > 0) {
                    continue;
                }

                if (ch == 127 || ch == 8) { // Backspace
                    if (input_len > 0) {
                        input_buf[--input_len] = '\0';
                        redraw_line(prompt, input_buf);
                    }
                } else if (ch == '\n' || ch == '\r') { // Enter
                    input_buf[input_len] = '\0';
                    printf("\r\033[K%s%s%s%s\n", COLOR_YELLOW, prompt, input_buf, COLOR_RESET);

                    if (send(sockfd, input_buf, strlen(input_buf), 0) < 0) {
                        printf("%sConnection lost while sending message%s\n", COLOR_RED, COLOR_RESET);
                        close(sockfd);
                        disable_raw_mode();

                        sockfd = attempt_reconnect(name);
                        if (sockfd < 0) {
                            printf("\nExiting...\n");
                            exit(EXIT_FAILURE);
                        }

                        enable_raw_mode();
                    }

                    if (strcmp(input_buf, "quit") == 0) break;
                    input_len = 0;
                    memset(input_buf, 0, sizeof(input_buf));
                    redraw_line(prompt, input_buf);
                } else if (input_len < BUFFER_SIZE - 1 && ch >= 32 && ch <= 126) { // Printable characters only
                    input_buf[input_len++] = ch;
                    input_buf[input_len] = '\0';
                    redraw_line(prompt, input_buf);
                }
            }
        }
    }

    disable_raw_mode();
    close(sockfd);
    return 0;
}

