#pragma once
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h> 

#define SIZE 10
#define TOTAL_SHIPS 9
#define BUFFER_SIZE 14
#define MAX_SESSIONS 10
#define BUFFER_SIZE_Confirm 1

#ifdef __cplusplus
#include <iostream>
#include <sstream>
#include <cstring>
#include <cstdlib>
//Aca lo de C++

struct Config {
    char server_ip[256];
    int PORTLINUX;
};

void trim(std::string& str) {
    size_t first = str.find_first_not_of(' ');
    size_t last = str.find_last_not_of(" \n\r");
    str = str.substr(first, (last - first + 1));
}

struct ship {
    unsigned char posX;
    unsigned char posY;
    unsigned char size;
    bool dir;
    int impacts;
};

struct attack {
    unsigned char posX;
    unsigned char posY;
};

inline void encode(const ship arr[], unsigned char* encoded) {
    memset(encoded, 0, 14); // Limpia el buffer antes de usarlo
    unsigned char bPos = 0;  //Podemos añadir una variable "K" para que la cantidad de barcos sea dynamica.
                                //Con variable dynamica el tama�o de este char seria (Roof)K*3/2. cada barco es de 12 bits, y lo que mandamos esta en bytes, lo mismo aplica para las otras funciones.

    for (char i = 0; i < 9; i++) {
        encoded[bPos / 8] |= (arr[i].posX << (bPos % 8));
        bPos += 4;
        encoded[bPos / 8] |= (arr[i].posY << (bPos % 8));
        bPos += 4;
        encoded[bPos / 8] |= (arr[i].size << (bPos % 8));
        bPos += 3;
        encoded[bPos / 8] |= (arr[i].dir << (bPos % 8));
        bPos += 1;
    }
}

inline void decode(const unsigned char* arr, ship* decoded) {
    memset(decoded, 0, sizeof(ship) * 9); // Limpia el buffer antes de usarlo
    unsigned char bPos = 0;

    for (char i = 0; i < 9; ++i) {
        decoded[i].posX = (arr[bPos / 8] >> (bPos % 8)) & 0xF;
        bPos += 4;

        decoded[i].posY = (arr[bPos / 8] >> (bPos % 8)) & 0xF;
        bPos += 4;

        decoded[i].size = (arr[bPos / 8] >> (bPos % 8)) & 0x7;
        bPos += 3;

        decoded[i].dir = (arr[bPos / 8] >> (bPos % 8)) & 0x1;
        bPos += 1;
    }
}

unsigned char encodeAttack(struct attack A) {
	unsigned char encoded;

	encoded =  A.posX;
	encoded = encoded | (A.posY << 4);

	return encoded;
}

struct attack decodeAttack(unsigned char A) {
	struct attack decoded;

	decoded.posX = A & 0xF;
	decoded.posY = (A & 0xF0) >> 4;

	return decoded;
}

void parse_config(const char* filename, char* server_ip, int* port) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        perror("Error opening config file");
        exit(1);
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line[0] == '#' || line.empty()) continue;

        std::istringstream iss(line);
        std::string key, value;
        if (std::getline(iss, key, '=') && std::getline(iss, value)) {
            trim(value);
            if (key == "serverip") {
                strncpy(server_ip, value.c_str(), 255);
            }
            else if (key == "port") {
                *port = std::atoi(value.c_str());
            }
        }
    }

    std::cout << "Server IP: " << server_ip << std::endl;
    std::cout << "Port: " << port << std::endl;
    file.close();
}

inline int connect_to_server(const Config& config) {
    int client_fd;
    sockaddr_in serv_addr{};

    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(config.PORTLINUX);

    if (inet_pton(AF_INET, config.server_ip, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return -1;
    }

    if (connect(client_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        return -1;
    }

    return client_fd;
}

inline void registration( std::string &email, std::string &username, int client_fd) {
    //char buffer[BUFFER_SIZE] = { 0 };
    std::cout << "Enter your username: ";
    std::getline(std::cin >> std::ws, username);
    send(client_fd, username.c_str(), username.length(), 0);
    std::cout << "Enter your email: ";
    std::getline(std::cin >> std::ws, email);
    send(client_fd, email.c_str(), email.length(), 0);

    return;
}

#else
#include <stdbool.h>

typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;

typedef struct {
    int server_fd;
    sockaddr_in address;
} Server;

typedef struct ship {
    unsigned char posX;  // 4 bits
    unsigned char posY;  // 4 bits
    unsigned char size;  // 3 bits
    bool dir;            // 0 = horizontal, 1 = vertical
    int impacts; 
} ship;

typedef struct {
    unsigned char posX;  // 4 bits
    unsigned char posY;  // 4 bits
} attack;

typedef struct {
    int player1_fd;
    int player2_fd;
    char player1_name[50];
    char player2_name[50];
    struct ship ships1[TOTAL_SHIPS];
    struct ship ships2[TOTAL_SHIPS];
} GameSession;

typedef struct {
    GameSession sessions[MAX_SESSIONS];
    int current_session;
    pthread_mutex_t session_mutex;
} ServerState;

typedef struct {
    int client_socket;
    char path[256];
    ServerState* state;
} ThreadArgs;

void decode(const unsigned char* arr, ship* decoded) {
    memset(decoded, 0, sizeof(ship) * 9); // Limpia el buffer antes de usarlo
    unsigned char bPos = 0;

    for (char i = 0; i < 9; ++i) {
        decoded[i].posX = (arr[bPos / 8] >> (bPos % 8)) & 0xF;
        bPos += 4;

        decoded[i].posY = (arr[bPos / 8] >> (bPos % 8)) & 0xF;
        bPos += 4;

        decoded[i].size = (arr[bPos / 8] >> (bPos % 8)) & 0x7;
        bPos += 3;

        decoded[i].dir = (arr[bPos / 8] >> (bPos % 8)) & 0x1;
        bPos += 1;
    }
}



void encode(const ship arr[], unsigned char* encoded) {
    memset(encoded, 0, 14); // Limpia el buffer antes de usarlo
    unsigned char bPos = 0;

    for (char i = 0; i < 9; i++) {
        encoded[bPos / 8] |= (arr[i].posX << (bPos % 8));
        bPos += 4;
        encoded[bPos / 8] |= (arr[i].posY << (bPos % 8));
        bPos += 4;
        encoded[bPos / 8] |= (arr[i].size << (bPos % 8));
        bPos += 3;
        encoded[bPos / 8] |= (arr[i].dir << (bPos % 8));
        bPos += 1;
    }
}

int receive_encoded_ships(int client_fd, ship ships[]) {
    unsigned char buffer[BUFFER_SIZE];
    int bytes = read(client_fd, buffer, BUFFER_SIZE);
    //printf("bytes: %d\n", bytes);
    
    if (bytes != BUFFER_SIZE) {
        perror("Error leyendo buffer codificado");
        return -1;
    }

    memset(ships, 0, sizeof(ship) * TOTAL_SHIPS);

    decode(buffer, ships);

    return 0;
}

attack decodeAttack(unsigned char A) {
	attack decoded;


	decoded.posX = A & 0xF;
	decoded.posY = (A & 0xF0) >> 4;

	//printf("La posX del ataque: ");
	//printf("%x", decoded.posX);
	//printf("\n");
	//printf("La posY del ataque: ");
	return decoded;
}

unsigned char encodeAttack(attack A) {
    unsigned char encoded;

    encoded =  A.posX;
    encoded = encoded | (A.posY << 4);

    return encoded;
}



void safe_log(const char* message, const char* path) {
    int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("open");
        return;
    }

    if (flock(fd, LOCK_EX) == -1) {
        perror("flock");
        close(fd);
        return;
    }

    FILE* log_file = fdopen(fd, "a");
    if (!log_file) {
        perror("fdopen");
        close(fd);
        return; 
    }

    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    fprintf(log_file, "[%04d-%02d-%02d %02d:%02d:%02d] %s\n",
            t->tm_year + 1900,
            t->tm_mon + 1,
            t->tm_mday,
            t->tm_hour,
            t->tm_min,
            t->tm_sec,
            message);

    fflush(log_file);  
    flock(fd, LOCK_UN); 
    fclose(log_file);  
}


void receive_player_info(int socket, char* username, char* email, char* path) {
    int bytes_username = recv(socket, username, 49, 0);
    int bytes_email = recv(socket, email, 49, 0);

    if (bytes_username <= 0 || bytes_email <= 0) {
        perror("Error recibiendo datos del usuario");
        close(socket);
        pthread_exit(NULL);
    }

    username[bytes_username] = '\0';
    email[bytes_email] = '\0';

    printf("Usuario conectado: %s\nEmail conectado: %s\n", username, email);

    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "Usuario conectado: %s | Email: %s", username, email);
    safe_log(log_msg, path);
}

void send_turn_messages(int active_fd, int waiting_fd) {
    send(active_fd, "t", 1, 0);
    send(waiting_fd, "w", 1, 0);
}

extern inline int setup_server(Server* server, char* IP, char* port) {
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

    if (bind(server->server_fd, (sockaddr*)&server->address, sizeof(server->address)) < 0) {
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

#endif