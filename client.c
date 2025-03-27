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
    
#endif

// Puerto
#define PORT "8080"
#define PORTLINUX 8080

#define BUFFER_SIZE 1024


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

        if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            printf("\n Socket creation error \n");
            return -1;
        }

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PORTLINUX);


        if (inet_pton(AF_INET, "44.203.190.123", &serv_addr.sin_addr)
            <= 0) {
            printf(
                "\nInvalid address/ Address not supported \n");
            return -1;
        }

        if ((status
            = connect(client_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0) {
        printf("\nConnection Failed \n");
        return -1;
        }

        send(client_fd, hello, strlen(hello), 0);
        printf("Hello message sent\n");
        
        valread = read(client_fd, buffer, 1024 - 1); 
        printf("%s\n", buffer);

        close(client_fd);
        return 0;

    }

#endif