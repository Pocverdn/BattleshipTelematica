#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 

#include <pthread.h>

#ifdef _WIN32
#include <winsock2.h> 
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib") 
#elif 
#include <arpa/inet.h>
#endif

#define PORT 8080
//14
#define BUFFER_SIZE 1024
#define BUFFER_SIZE_Confirm 1
#ifdef _WIN32
DWORD WINAPI handle_client(LPVOID client_socket) {
    SOCKET new_socket = *(SOCKET*)client_socket;
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
    return 0;
}
#else
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
#endif
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

#ifdef _WIN32
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) ==SOCKET_ERROR) {
        perror("Setsockopt failed");
        return 1;
    }
#else
    
if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
    perror("Setsockopt failed");
    return 1;
}
    #endif
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

 #ifdef _WIN32
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) ==SOCKET_ERROR) {
        perror("Bind failed");
        return 1;
    }
    if (listen(server_fd, 3) ==SOCKET_ERROR) {
        perror("Listen failed");
        return 1;
    }
#else
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        return 1;
    }
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        return 1;
    }
#endif
    

    printf("Server is listening on port %d...\n", PORT);
#ifdef _WIN32
while (1) {
    new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen);
    if (new_socket == INVALID_SOCKET) {
        printf("Accept failed: %d\n", WSAGetLastError());
        continue;
    }


    HANDLE thread_handle;
    DWORD thread_id;
    thread_handle = CreateThread(NULL, 0, handle_client, (LPVOID)&new_socket, 0, &thread_id);
    if (thread_handle == NULL) {
        printf("Thread creation failed: %d\n", GetLastError());
        closesocket(new_socket);
        continue;
    }
    CloseHandle(thread_handle);
}
#else
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
#endif

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
