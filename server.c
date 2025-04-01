#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
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

typedef struct {
    bool ship;
    bool attacked;
} map;

typedef struct {
    map p1[10][10];
    map p2[10][10];
} session;

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




inline struct ship* decode(unsigned char arr[]) {
    //printf("%X", arr);
    //printf("%X", arr[1]);
    static struct ship decode[9] = { 0 };
    unsigned char bPos = 0;

    for (char i = 0; i < 9; i++) {

        decode[i].posX = (arr[bPos / 8] & (0xF << bPos % 8)) >> bPos % 8;
        bPos = bPos + 4;
        decode[i].posY = (arr[bPos / 8] & (0xF << bPos % 8)) >> bPos % 8;
        bPos = bPos + 4;
        decode[i].size = (arr[bPos / 8] & (0x7 << bPos % 8)) >> bPos % 8;
        bPos = bPos + 3;
        decode[i].dir = (arr[bPos / 8] & (0x1 << bPos % 8)) >> bPos % 8;
        bPos = bPos + 1;


    }
    //printf("%x", *arr);
    printf("El pos X del cuarto barco: ");
    printf("%x", decode[3].posX);
    printf("\n");
    return decode;
}

inline void gameStart(struct ship ships[], bool player, session* ses) {
    if (player) { //player es si es jugador uno o no
        for (char i = 0; i < 9; i++) {
            for (char j = 0; j < ships[i].size; j++) {
                ses->p1[ships[i].posX+(j* ships[i].dir)][ships[i].posY + (j * ships[i].dir)].ship=1;
            }
        }
    }
    else {
        for (char i = 0; i < 9; i++) {
            for (char j = 0; j < ships[i].size; j++) {
                ses->p2[ships[i].posX + (j * ships[i].dir)][ships[i].posY + (j * ships[i].dir)].ship = 1;
            }
        }
    }
}

void *handle_client(void *client_socket) {
    int new_socket = *(int *)client_socket;
    free(client_socket);

    char buffer[BUFFER_SIZE] = {0};
    const char *hello = "Message received";



        //Inicio del juego, fuera del buffer
        //Necesito que cuando hagan la conexion me pogan un bool con el numero del jugador y que ambos threads de un juego compartan un puntero a la misma session en el servidor 
        //Mientraas para pruebas
        bool TESTplayer = 0;
        session TESTs = {0, 0};

        //los valores de prueba terminan aca

        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(new_socket, buffer, BUFFER_SIZE, 0);

        if (bytes_received <= 0) {
            printf("Cliente desconectado\n");
            
        }
        else {
            struct ship ships[9] = decode(buffer);
            gameStart(ships, TESTplayer,&TESTs);
        }

        while (1) {
            memset(buffer, 0, BUFFER_SIZE);
            int bytes_received = recv(new_socket, buffer, BUFFER_SIZE, 0);

            if (bytes_received <= 0) {
                printf("Cliente desconectado\n");
                break;
            }

        printf("%s\n", buffer);

        send(new_socket, hello, strlen(hello), 0);
        //printf("Mensaje enviado al cliente\n");
    }

    close(new_socket); 

    return NULL;
}

inline int setup_server(Server *server, char* IP, char* port) {
    server->server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->server_fd == -1) {
        perror("Socket creation failed");
        exit(-1);
    }
    
    int opt = 1;
    if (setsockopt(server->server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        exit(-1);
    }

    server->address.sin_family = AF_INET;
    server->address.sin_addr.s_addr = inet_addr(IP);
    server->address.sin_port = htons(atoi(port));

    if (bind(server->server_fd, (sockaddr *)&server->address, sizeof(server->address)) < 0) {
        perror("Bind failed");
        exit(-1);
    }
    
    if (listen(server->server_fd, 3) < 0) {
        perror("Listen failed");
        exit(-1);
    }

    printf("Server is listening on port %d...\n", atoi(port));
    return 0;
}

inline void accept_clients(Server *server) {
    int addrlen = sizeof(server->address);
    pthread_t thread_id;

    while (1) {
        int *new_socket = malloc(sizeof(int));
        if (!new_socket) {
            perror("Memory allocation failed");
            continue;
        }

        *new_socket = accept(server->server_fd, (sockaddr *)&server->address, (socklen_t *)&addrlen);
        if (*new_socket < 0) {
            perror("Accept failed");
            free(new_socket);
            continue;
        }

        if (pthread_create(&thread_id, NULL, handle_client, (void *)new_socket) != 0) {
            perror("Thread creation failed");
            close(*new_socket);
            free(new_socket);
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
