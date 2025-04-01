#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

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
#define BUFFER_SIZE 14 
#define BUFFER_attack 1 

// Parser de archivo
typedef struct {
    char server_ip[256];
    int PORTLINUX;
} Config;


struct ship {
    unsigned char  posX;//4 bits
    unsigned char  posY;//4 bits
    unsigned char  size;//3 bits
    bool dir;
};

struct attack {
    unsigned char  posX;//4 bits
    unsigned char  posY;//4 bits
};


typedef struct {
    bool ship;
    bool attacked;
} map;


inline unsigned char* encode(struct ship arr[]) { //Podemos a�adir una variable "K" para que la cantidad de barcos sea dynamica.
    static unsigned char encoded[14] = { 0 }; //Con variable dynamica el tama�o de este char seria (Roof)K*3/2. cada barco es de 12 bits, y lo que mandamos esta en bytes, lo mismo aplica para las otras funciones.
    unsigned char bPos = 0;

    for (char i = 0; i < 9; i++) {
        encoded[bPos / 8] = encoded[bPos / 8] | (arr[i].posX << bPos % 8);
        bPos = bPos + 4;
        encoded[bPos / 8] = encoded[bPos / 8] | (arr[i].posY << bPos % 8);
        bPos = bPos + 4;
        encoded[bPos / 8] = encoded[bPos / 8] | (arr[i].size << bPos % 8);
        bPos = bPos + 3;
        encoded[bPos / 8] = encoded[bPos / 8] | (arr[i].dir << bPos % 8);
        bPos = bPos + 1;

    }
    printf("El primer byte de la cadena codificada: ");
    printf("%x", encoded[0]);
    printf("\n");
    return encoded;
}

inline struct ship* inputShips() {
    static struct ship ship[9] = { 0 };
   
    for (char i = 0; i < 9; i++) {
        printf("coordenadas, tamaño y luego dirección (1 o 0) del barco.");
        scanf("%d", &ship[i].posX);
        scanf("%d", &ship[i].posY);
        scanf("%d", &ship[i].size);
        scanf("%d", &ship[i].dir);
    }
}

inline void trim(char *str) {
    char *end;
    while (*str == ' ') str++;  // Trim leading spaces
    end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\n' || *end == '\r')) end--;  // Trim trailing spaces
    *(end + 1) = '\0';
}

inline void parse_config(const char *filename, char *server_ip, int *port) {
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

    SOCKET connect_to_server(const Config *config) {
        WSADATA wsaData;
        struct addrinfo *result = NULL, *ptr = NULL, hints;
        SOCKET ConnectSocket = INVALID_SOCKET;
        int iResult;

        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            printf("WSAStartup failed\n");
            return INVALID_SOCKET;
        }

        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        char port_str[10];
        sprintf(port_str, "%d", config->PORTWINDOWS); // Convertir puerto a string

        iResult = getaddrinfo(config->server_ip, port_str, &hints, &result);
        if (iResult != 0) {
            printf("getaddrinfo failed: %d\n", iResult);
            WSACleanup();
            return INVALID_SOCKET;
        }

        for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
            ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
            if (ConnectSocket == INVALID_SOCKET) {
                printf("Error at socket(): %ld\n", WSAGetLastError());
                freeaddrinfo(result);
                WSACleanup();
                return INVALID_SOCKET;
            }

            if (connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen) == SOCKET_ERROR) {
                closesocket(ConnectSocket);
                ConnectSocket = INVALID_SOCKET;
                continue;
            }
            break;
        }

        freeaddrinfo(result);

        if (ConnectSocket == INVALID_SOCKET) {
            printf("Unable to connect to server!\n");
            WSACleanup();
            return INVALID_SOCKET;
        }

        return ConnectSocket;
    }


    void chat_with_server(SOCKET ConnectSocket) {
        char sendbuf[1024];
        char recvbuf[1024];
        int recvbuflen = 1024;
        int iResult;
    
        printf("Connected to server! Type 'exit' to close connection.\n");
    
        while (1) {
            printf("Enter message: ");
            fgets(sendbuf, sizeof(sendbuf), stdin);
            sendbuf[strcspn(sendbuf, "\n")] = 0;
    
            if (strcmp(sendbuf, "exit") == 0) {
                printf("Closing connection...\n");
                break;
            }
    
            iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
            if (iResult == SOCKET_ERROR) {
                printf("Send failed: %d\n", WSAGetLastError());
                closesocket(ConnectSocket);
                WSACleanup();
                return;
            }
    
            iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
            if (iResult > 0) {
                recvbuf[iResult] = '\0';
                printf("Server: %s\n", recvbuf);
            } else if (iResult == 0) {
                printf("Connection closed\n");
                break;
            } else {
                printf("recv failed: %d\n", WSAGetLastError());
                break;
            }
        }
    
        closesocket(ConnectSocket);
        WSACleanup();
    }

#else
    

    int connect_to_server(const Config *config) {
        int client_fd;
        struct sockaddr_in serv_addr;

        if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("Socket creation error");
            return -1;
        }

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(config->PORTLINUX);

        if (inet_pton(AF_INET, config->server_ip, &serv_addr.sin_addr) <= 0) {
            perror("Invalid address/ Address not supported");
            return -1;
        }

        if (connect(client_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
            perror("Connection Failed");
            return -1;
        }

        return client_fd;
    }


    void chat_with_server(int client_fd) {
        char buffer[BUFFER_SIZE] = {0};
        char username[50];

        printf("Enter your username: ");
        fgets(username, sizeof(username), stdin);
        username[strcspn(username, "\n")] = 0;
    
        printf("Connected to server as %s! Type 'exit' to close connection.\n", username);

        while (1) {
            char msg[BUFFER_SIZE];
            printf("[%s] Enter message: ", username);
            fgets(msg, BUFFER_SIZE, stdin);
            
            msg[strcspn(msg, "\n")] = 0;
            if (strcmp(msg, "exit") == 0) {
                printf("Closing connection...\n");
                break;
            }

            if (strlen(username) + strlen(msg) + 3 > BUFFER_SIZE) {
                fprintf(stderr, "Error: Mensaje demasiado largo\n");
                continue;
            }

            char formatted_msg[BUFFER_SIZE];
            snprintf(formatted_msg, BUFFER_SIZE, "%.*s: %.*s", 50, username, BUFFER_SIZE - 50 - 3, msg);
            
            send(client_fd, formatted_msg, strlen(formatted_msg), 0);
            //printf("Message sent\n");
    
            int valread = read(client_fd, buffer, BUFFER_SIZE - 1);
            if (valread > 0) {
                buffer[valread] = '\0';
                //printf("Server response: %s\n", buffer);
            }
        }
    }
    

#endif


//Tutorial used:https://learn.microsoft.com/en-us/windows/win32/winsock/creating-a-socket-for-the-client
#ifdef _WIN32

    int main() {
        Config config;
        parse_config("adress.config", config.server_ip, &config.PORTWINDOWS);

        printf("Server IP: %s\n", config.server_ip);
        printf("Port: %d\n", config.PORTWINDOWS);

        SOCKET ConnectSocket = connect_to_server(&config);
        if (ConnectSocket == INVALID_SOCKET) return 1;

        chat_with_server(ConnectSocket);
        return 0;
    }
    
#else

//Tutorial used:https://www.geeksforgeeks.org/socket-programming-cc/

    int main(int argc, char const* argv[]) {
        Config config;
        parse_config("adress.config", config.server_ip, &config.PORTLINUX);

        printf("Server IP: %s\n", config.server_ip);
        printf("Port: %d\n", config.PORTLINUX);

        int client_fd = connect_to_server(&config);
        if (client_fd < 0) return -1;

        chat_with_server(client_fd);
        close(client_fd);
        return 0;
    }

#endif