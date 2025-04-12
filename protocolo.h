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

#endif
