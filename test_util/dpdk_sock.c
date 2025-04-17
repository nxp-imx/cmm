/* SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0)
 * Copyright 2025 NXP
*/
/*
 * socket_server.c
 *
 * A simple Unix domain socket server that:
 *  1. Creates and binds to a socket file
 *  2. Listens for incoming connections
 *  3. Reads messages from connected clients and prints them to stdout
 *
 * Usage:
 *   gcc -o socket_server socket_server.c
 *   ./socket_server [socket_path]
 *
 * If no socket_path is provided, it defaults to "/tmp/mysocket".
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#define DEFAULT_SOCKET_PATH "/tmp/fpr_cmm.sock"
#define BUFFER_SIZE 256

int main(int argc, char *argv[]) {
    const char *socket_path = (argc > 1 ? argv[1] : DEFAULT_SOCKET_PATH);
    int server_fd, client_fd;
    struct sockaddr_un addr;
    char buffer[BUFFER_SIZE];
    ssize_t num_read;

    // 1. Create a Unix domain socket
    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // 2. Remove existing socket file, if any
    unlink(socket_path);

    // 3. Bind the socket to the specified file path
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 4. Listen for incoming connections
    if (listen(server_fd, 5) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on socket: %s\n", socket_path);

    // 5. Accept and handle client connections in a loop
    while (1) {
        if ((client_fd = accept(server_fd, NULL, NULL)) == -1) {
            perror("accept");
            continue;  // try next
        }

        printf("Client connected. Reading messages...\n");

        // 6. Read messages and print
        while ((num_read = read(client_fd, buffer, BUFFER_SIZE - 1)) > 0) {
            buffer[num_read] = '\0';
            printf("%s", buffer);
        }

        if (num_read == -1) {
            perror("read");
        } else {
            printf("\nClient disconnected.\n");
        }

        close(client_fd);
    }

    // Cleanup (unreachable in this loop, but good practice)
    close(server_fd);
    unlink(socket_path);
    return 0;
}

