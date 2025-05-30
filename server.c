#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#define PORT 8080
#define COLOR_RESET   "\x1b[0m"
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_WHITE   "\x1b[37m"
#define MAXLINE 1024
#define SA struct sockaddr
#define max(x, y) ((x) > (y) ? (x) : (y))
#define MAX_CLIENTS 10
#define HISTORY_SIZE 100

char message_history[HISTORY_SIZE][MAXLINE];
int history_count = 0;
int history_start = 0;

const char *colors[MAX_CLIENTS] =
{
    COLOR_RED, COLOR_GREEN, COLOR_YELLOW, COLOR_BLUE,
    COLOR_MAGENTA, COLOR_CYAN, COLOR_WHITE,
    COLOR_RED, COLOR_GREEN, COLOR_YELLOW
};

void add_to_history(const char *msg)
{
    int index;

    if (history_count < HISTORY_SIZE)
    {
        index = (history_start + history_count);
        history_count++;
    }
    else
    {
        index = history_start;
        history_start = (history_start + 1) % HISTORY_SIZE;
    }

    int msg_size = strlen(msg);
    // Only copy the actual message length, not the entire MAXLINE
    strncpy(message_history[index], msg, msg_size);
    message_history[index][msg_size] = '\0'; // Ensure null termination
}

void send_history_to_client(int sockfd)
{
    for (int i = 0; i < history_count; i++)
    {
        int index = (history_start + i) % HISTORY_SIZE;
        int msg_len = strlen(message_history[index]);
        // Create buffer just big enough for message + newline + null terminator
        char msg_with_newline[msg_len + 2];
        snprintf(msg_with_newline, sizeof(msg_with_newline), "%s\n", message_history[index]);
        send(sockfd, msg_with_newline, strlen(msg_with_newline), 0);
    }
}

typedef struct
{
    int sockfd;
    int first_msg_received;
    char name[MAXLINE];
    const char* color;
} Client;

int main(int argc, char **argv)
{
    int tcp_sockfd;
    tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);


    struct sockaddr_in servaddr, cliaddr;
    char buf[MAXLINE] = {0};

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);
    servaddr.sin_port = htons(PORT);
    bind(tcp_sockfd, (SA *)&servaddr, sizeof(servaddr));

    listen(tcp_sockfd, MAX_CLIENTS + 1);
    printf("%slistening on port 8080%s\n", COLOR_CYAN, COLOR_RESET);

    Client clients[MAX_CLIENTS];
    int clientscount = 0;

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        clients[i].sockfd = -1;
        clients[i].color = colors[i];
    }

    fd_set rset, allset;
    FD_ZERO(&allset);
    FD_SET(tcp_sockfd, &allset);
    int max_fd = tcp_sockfd;

    while (1)
    {
        rset = allset;

        if (select(max_fd + 1, &rset, NULL, NULL, NULL) < 0)
        {
            perror("select");
            continue;
        }

        if (FD_ISSET(tcp_sockfd, &rset))
        {
            socklen_t clilen = sizeof(cliaddr);
            int new_sockfd;

            if (clientscount < MAX_CLIENTS)
            {
                new_sockfd = accept(tcp_sockfd, (SA *)&cliaddr, &clilen);
                if (new_sockfd < 0)
                {
                    perror("accept failed");
                    continue;
                }
                clientscount++;

                FD_SET(new_sockfd, &allset);
                if (new_sockfd > max_fd)
                    max_fd = new_sockfd;

                // Register new client
                for (int i = 0; i < MAX_CLIENTS; i++)
                {
                    if (clients[i].sockfd == -1)
                    {
                        clients[i].sockfd = new_sockfd;
                        clients[i].first_msg_received = 0;
                        memset(clients[i].name, 0, MAXLINE);
                        break;
                    }
                }
            }
        }

        // Handle existing clients
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (clients[i].sockfd != -1 && FD_ISSET(clients[i].sockfd, &rset))
            {
                memset(buf, 0, MAXLINE);
                int valread = read(clients[i].sockfd, buf, MAXLINE - 1);

                if (valread <= 0)
                {
                    printf("%s%s disconnected%s\n", COLOR_CYAN, clients[i].name, COLOR_RESET);
                    fflush(stdout);
                    close(clients[i].sockfd);
                    FD_CLR(clients[i].sockfd, &allset);
                    clients[i].first_msg_received = 0;
                    clients[i].sockfd = -1;
                    clientscount--;

                    // Update max_fd if necessary
                    if (clients[i].sockfd == max_fd)
                    {
                        max_fd = tcp_sockfd;
                        for (int j = 0; j < MAX_CLIENTS; j++)
                        {
                            if (clients[j].sockfd > max_fd)
                                max_fd = clients[j].sockfd;
                        }
                    }
                }
                else
                {
                    buf[valread] = '\0';

                    // Remove newline from buffer if present
                    char *newline = strchr(buf, '\n');
                    if (newline) *newline = '\0';

                    if (clients[i].first_msg_received == 0)
                    {
                        strncpy(clients[i].name, buf, MAXLINE - 1);
                        clients[i].name[MAXLINE - 1] = '\0';
                        clients[i].first_msg_received = 1;
                        printf("New member added to the chat: %s\n", clients[i].name);
                        fflush(stdout);

                        send_history_to_client(clients[i].sockfd);
                        continue;
                    }

                    if (strcmp(buf, "quit") == 0)
                    {
                        printf("%s said 'quit', closing connection\n", clients[i].name);
                        fflush(stdout);
                        close(clients[i].sockfd);
                        FD_CLR(clients[i].sockfd, &allset);
                        clients[i].first_msg_received = 0;
                        clients[i].sockfd = -1;
                        clientscount--;

                        // Update max_fd if necessary
                        if (clients[i].sockfd == max_fd)
                        {
                            max_fd = tcp_sockfd;
                            for (int j = 0; j < MAX_CLIENTS; j++)
                            {
                                if (clients[j].sockfd > max_fd)
                                    max_fd = clients[j].sockfd;
                            }
                        }
                    }
                    else
                    {
                        char message[MAXLINE];
                        snprintf(message, sizeof(message), "%s[%s]: %s%s",
                                clients[i].color, clients[i].name, buf, COLOR_RESET);

                        add_to_history(message);

                        // Broadcast to other clients
                        for (int j = 0; j < MAX_CLIENTS; j++)
                        {
                            if (clients[j].sockfd != -1 && clients[j].sockfd != clients[i].sockfd)
                            {
                                send(clients[j].sockfd, message, strlen(message), 0);
                            }
                        }

                        printf("%s[%s]: %s%s\n", clients[i].color, clients[i].name, buf, COLOR_RESET);
                        fflush(stdout);
                    }
                }
            }
        }
    }

    return 0;
}

