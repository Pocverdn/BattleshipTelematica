#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Librerias sockets para linux
#include <arpa/inet.h>
#include <unistd.h> 
#include <pthread.h>

// Puerto
//#define PORT 8080

//14 bytes para envio de posiciones de barcos - 1 byte para comfirmación de disparo
#define BUFFER_SIZE 14
#define BUFFER_SIZE_Confirm 1

typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;

typedef struct {
    int server_fd;
    sockaddr_in address;
} Server;


void *handle_client(void *client_socket) {
    int new_socket = *(int *)client_socket;
    char buffer[BUFFER_SIZE] = {0};
    const char *hello = "Hello from server";

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(new_socket, buffer, BUFFER_SIZE, 0);

        if (bytes_received <= 0) {
            printf("Cliente desconectado\n");
            break;
        }

        printf("Mensaje del cliente: %s\n", buffer);

        send(new_socket, hello, strlen(hello), 0);
        //printf("Mensaje enviado al cliente\n");
    }

    close(new_socket); 

    return NULL;
}

int setup_server(Server *server, char* IP, char* port) {
    server->server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->server_fd == -1) {
        perror("Socket creation failed");
        return -1;
    }
    
    int opt = 1;
    if (setsockopt(server->server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        return -1;
    }

    server->address.sin_family = AF_INET;
    server->address.sin_addr.s_addr = inet_addr(IP);
    server->address.sin_port = htons(atoi(port));

    if (bind(server->server_fd, (sockaddr *)&server->address, sizeof(server->address)) < 0) {
        perror("Bind failed");
        return -1;
    }
    
    if (listen(server->server_fd, 3) < 0) {
        perror("Listen failed");
        return -1;
    }

    printf("Server is listening on port %d...\n", atoi(port));
    return 0;
}

void accept_clients(Server *server) {
    int addrlen = sizeof(server->address);
    pthread_t thread_id;

    while (1) {
        int new_socket = accept(server->server_fd, (sockaddr *)&server->address, (socklen_t *)&addrlen);
        if (new_socket < 0) {
            perror("Accept failed");
            continue;
        }
        if (pthread_create(&thread_id, NULL, handle_client, (void *)&new_socket) != 0) {
            perror("Thread creation failed");
            close(new_socket);
            continue;
        }
        pthread_detach(thread_id);
    }
}


int main(int argc, char* argv[]) {
    Server server;
    if (setup_server(&server,argv[1] ,argv[2]) < 0) {
        return 1;
    }
    accept_clients(&server);
    close(server.server_fd);
    return 0;
}
