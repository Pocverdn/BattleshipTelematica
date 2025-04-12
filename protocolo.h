#pragma once
#include <stdio.h>
#include <string.h>


#define BUFFER_SIZE 14
#define BUFFER_SIZE_Confirm 1

#define SIZE 10
#define TOTAL_SHIPS 9


#ifdef __cplusplus
//Aca lo de C++

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

inline unsigned char* encode(ship arr[]) {
    static unsigned char encoded[14] = { 0 };
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

    return encoded;
}

inline ship* decode(const unsigned char arr[]) {
    static ship decoded[9] = { 0 };
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

    return decoded;
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

int connect_to_server(const Config& config) {
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

#else

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

extern inline struct ship* decode(unsigned char arr[]) {
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

    return decode;
}

unsigned char* encode(ship arr[]) {
    static unsigned char encoded[14] = { 0 };
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

    //printf("El primer byte de la cadena codificada: %02X\n", encoded[0]);
    return encoded;
}

int receive_encoded_ships(int client_fd, ship ships[]) {
    unsigned char buffer[BUFFER_SIZE];
    int bytes = read(client_fd, buffer, BUFFER_SIZE);
    printf("bytes: %d\n", bytes);
    
    if (bytes != BUFFER_SIZE) {
        perror("Error leyendo buffer codificado");
        return -1;
    }


    struct ship *decoded = decode(buffer);
    memcpy(ships, decoded, sizeof(struct ship) * TOTAL_SHIPS);
    return 0;
}

attack decodeAttack(unsigned char A) {
	attack decoded;


	decoded.posX = A & 0xF;
	decoded.posY = (A & 0xF0) >> 4;

	printf("La posX del ataque: ");
	printf("%x", decoded.posX);
	printf("\n");
	printf("La posY del ataque: ");
	return decoded;
}

unsigned char encodeAttack(attack A) {
    unsigned char encoded;

    encoded =  A.posX;
    encoded = encoded | (A.posY << 4);

    return encoded;
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
    send(active_fd, "turn", strlen("turn") + 1, 0);
    send(waiting_fd, "wait", strlen("wait") + 1, 0);
}

#endif
