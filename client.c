﻿#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

//Librerias sockets para linux
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <libconfig.h>


// Buffer
#define BUFFER_SIZE 2500
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

extern inline unsigned char* encode(struct ship arr[]) { //Podemos a�adir una variable "K" para que la cantidad de barcos sea dynamica.
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

extern inline struct ship* inputShips() {
    static struct ship ship[9] = { 0 };
   
    for (char i = 0; i < 9; i++) {
        printf("%x \n", i);
        printf("coordenada X: ");
        scanf("%x", &ship[i].posX);
        printf("coordenada Y: ");
        scanf("%x", &ship[i].posY);
        printf("tamaño: ");
        scanf("%x", &ship[i].size);

        printf("dirección (1 o 0): ");

        scanf("%x", &ship[i].dir);
    }
    printf("Se pudo terminar el loop");
    return ship;
}

extern inline map* gameStart(struct ship ships[]) {
    static map mapa[10][10];
        for (char i = 0; i < 9; i++) {
            for (char j = 0; j < ships[i].size; j++) {
                mapa[ships[i].posX + (j * ships[i].dir)][ships[i].posY + (j * ships[i].dir)].ship = 1;
            }
        }
        return &mapa[0][0];
    
}

extern inline void trim(char *str) {
    char *end;
    while (*str == ' ') str++;  // Trim leading spaces
    end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\n' || *end == '\r')) end--;  // Trim trailing spaces
    *(end + 1) = '\0';
}

extern inline void parse_config(const char *filename, char *server_ip, int *port) {
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
    
extern inline int connect_to_server(const Config *config) {
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

extern inline void chat_with_server(int client_fd) {
    char buffer[BUFFER_SIZE] = {0};
    char username[50];

    printf("Enter your username: ");
    scanf(" %49[^\n]", username); // Ingresas tu nombre de usuario
    username[strcspn(username, "\n")] = 0;

    send(client_fd, username, strlen(username), 0); // El cliente envia el nombre al cliente para ponerte en cola

    //printf("Connected to server as %s! Type 'exit' to close connection.\n", username);

    
    struct ship* navy = inputShips();
    printf("Prueba2 ");
    map* mapa = gameStart(navy);
    printf("Prueba2 ");
    unsigned char* sendbuf = encode(navy);

    send(client_fd, sendbuf, 14, 0);
    

    while (1) {
        char msg[BUFFER_SIZE];
        printf("[%s]: ", username);
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
        snprintf(formatted_msg, BUFFER_SIZE, "%.*s: %.*s", (BUFFER_SIZE - 3) / 2, username, (BUFFER_SIZE - 3) / 2, msg);
        
        send(client_fd, formatted_msg, strlen(formatted_msg), 0);
        //printf("Message sent\n");

        int valread = read(client_fd, buffer, BUFFER_SIZE - 1); // Se recibe el mensaje
        if (valread > 0) {
            buffer[valread] = '\0'; // Se verifica que el mensaje sea un string
            //printf("%s\n", buffer); // Se imprime el mensaje recibido
        }
    }
}


//Tutorial used:https://www.geeksforgeeks.org/socket-programming-cc/

int main(int argc, char *argv[]) {
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

