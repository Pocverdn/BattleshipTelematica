#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    //Librerias sockets para windows
    #include <winsock2.h> 
    #include <ws2tcpip.h>
    #include <windows.h>
    #pragma comment(lib, "ws2_32.lib")
    #include <thread>
#else
    //Librerias sockets para linux
    #include <arpa/inet.h>
    #include <stdio.h>
    #include <string.h>
    #include <sys/socket.h>
    #include <unistd.h>
    #include <libconfig.h>
#endif

// Puerto
#define BUFFER_SIZE 1024

// Parser de archivo
typedef struct {
    char server_ip[256];
    int PORTLINUX;
} Config;

void trim(char *str) {
    char *end;
    while (*str == ' ') str++;  // Trim leading spaces
    end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\n' || *end == '\r')) end--;  // Trim trailing spaces
    *(end + 1) = '\0';
}

void parse_config(const char *filename, char *server_ip, int *port) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening config file");
        exit(1);
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char key[256], value[256];

        if (line[0] == '#' || line[0] == '\n') continue;  // Skip comments and empty lines

        if (sscanf(line, "%[^=]=%s", key, value) == 2) {
            trim(value);

            if (strcmp(key, "serverip") == 0) {
                strcpy(server_ip, value);  // Store IP as a string
            } else if (strcmp(key, "port") == 0) {
                *port = atoi(value);
            }
        }
    }
    fclose(file);
}


#ifdef _WIN32

    //Tutorial used:https://learn.microsoft.com/en-us/windows/win32/winsock/creating-a-socket-for-the-client

    int main(int argc, char *argv[]){
        
        int recvbuflen = BUFFER_SIZE;

        const char *sendbuf = "this is a test";
        char recvbuf[BUFFER_SIZE];

        int iResult;

        #ifdef _WIN32
            WSADATA wsaData;

            if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
                printf("WSAStartup failed\n");
                return 1;
            }
        #endif

        // Declaro addrinfo para resolver direcciones de dominio o ip

        struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;

        ZeroMemory( &hints, sizeof(hints) );
        hints.ai_family   = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;


        // Resuelvo la dirección del server (IP)

        iResult = getaddrinfo(argv[1], PORT, &hints, &result);
        if (iResult != 0) {
            printf("getaddrinfo failed: %d\n", iResult);
            WSACleanup();
            return 1;
        }

        SOCKET ConnectSocket = INVALID_SOCKET;

        ptr=result;

        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

        if (ConnectSocket == INVALID_SOCKET) {
            printf("Error at socket(): %ld\n", WSAGetLastError());
            freeaddrinfo(result);
            WSACleanup();
            return 1;
        }

        // Conectar el socket

        iResult = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            printf("Connection failed: %d\n", WSAGetLastError());
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            return 1;
        }


        freeaddrinfo(result);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("Unable to connect to server!\n");
            WSACleanup();
            return 1;
        }

        // Enviar un buffer inicial

        iResult = send(ConnectSocket, sendbuf, (int) strlen(sendbuf), 0);
        if (iResult == SOCKET_ERROR) {
            printf("send failed: %d\n", WSAGetLastError());
            closesocket(ConnectSocket);
            WSACleanup();
            return 1;
        }

        printf("Bytes Sent: %ld\n", iResult);

        // Cerrar la coneción una vez los datos sean enviados

        iResult = shutdown(ConnectSocket, SD_SEND);
        if (iResult == SOCKET_ERROR) {
            printf("shutdown failed: %d\n", WSAGetLastError());
            closesocket(ConnectSocket);
            WSACleanup();
            return 1;
        }


        // Recibir datos (!!!Por ahora no ha sido probado!!!)

        do {
            iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
            if (iResult > 0){
                printf("Bytes received: %d\n", iResult);
                recvbuf[iResult] = '\0';
                printf("Message received: %s\n", recvbuf);
            }else if (iResult == 0){
                printf("Connection closed\n");
            }else{
                printf("recv failed: %d\n", WSAGetLastError());
            }
        } while (iResult > 0);



        #ifdef _WIN32
            closesocket(ConnectSocket);
        #else
            close(ConnectSocket);
        #endif

        return 0;



    }
#else

    int main(int argc, char const* argv[])
    {

        int status, valread, client_fd;
        struct sockaddr_in serv_addr;
        char* hello = "Hello from client";
        char buffer[1024] = { 0 };

        //Extract config variables
        char server_ip[256] = "";
        int PORTLINUX = 0;
        parse_config("adress.config", server_ip, &PORTLINUX);
        printf("Server IP: %s\n", server_ip);
        printf("Port: %d\n", PORTLINUX);
        //

        if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            printf("\n Socket creation error \n");
            return -1;
        }

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PORTLINUX);


        if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
            printf(
                "\nInvalid address/ Address not supported \n");
            return -1;
        }

        if ((status = connect(client_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0) {
            printf("\nConnection Failed \n");
            return -1;
        }

        printf("Connected to server! Type 'exit' to close connection.\n");

        while (1) {
            char msg[BUFFER_SIZE];
            printf("Enter message: ");
            fgets(msg, BUFFER_SIZE, stdin);

            
            msg[strcspn(msg, "\n")] = 0;

            if (strcmp(msg, "exit") == 0) {
                printf("Closing connection...\n");
                break;
            }

            send(client_fd, msg, strlen(msg), 0);
            printf("Message sent: %s\n", msg);

            valread = read(client_fd, buffer, BUFFER_SIZE - 1);
            if (valread > 0) {
                buffer[valread] = '\0';
                printf("Server: %s\n", buffer);
            }
        }

        close(client_fd);
        return 0;

    }

#endif