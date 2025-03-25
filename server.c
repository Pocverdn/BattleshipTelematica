#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <arpa/inet.h>
#include <pthread.h>

#ifdef _WIN32
#include <winsock2.h> 
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")  
#endif

#define PORT 8080
#define BUFFER_SIZE 1024

void *handle_client(void *client_socket) {
    int new_socket = *(int *)client_socket;
    char buffer[BUFFER_SIZE] = {0};
    const char *hello = "Hello from server";


    recv(new_socket, buffer, BUFFER_SIZE, 0);
    printf("Message from client: %s\n", buffer);

    send(new_socket, hello, strlen(hello), 0);
    printf("Hello message sent\n");


    #ifdef _WIN32
        closesocket(new_socket);  
    #else
        close(new_socket); 
    #endif

    return NULL;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    pthread_t thread_id;

#ifdef _WIN32
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }
#endif


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

#ifdef _WIN32
    WSACleanup();
#endif

    #ifdef _WIN32
        closesocket(server_fd);  
    #else
        close(server_fd);  
    #endif

    return 0;
}
