#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Librerias sockets para linux
#include <arpa/inet.h>
#include <unistd.h> 
#include <pthread.h>

// Puerto
#define PORT 8080

//14 bytes para envio de posiciones de barcos - 1 byte para comfirmación de disparo
#define BUFFER_SIZE 1024
#define BUFFER_SIZE_Confirm 1

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

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    pthread_t thread_id;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        return 1;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        return 1;
    }

        
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);


    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        return 1;
    }
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        return 1;
    }
        
    printf("Server is listening on port %d...\n", PORT);

    while (1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) {
            perror("Accept failed");
            continue;
        }


        if (pthread_create(&thread_id, NULL, handle_client, (void *)&new_socket) != 0) {
            perror("Thread creation failed");
            continue;
        }

        pthread_detach(thread_id);
    }

    close(server_fd);  

    return 0;
}