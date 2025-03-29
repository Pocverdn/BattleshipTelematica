#include <stdbool.h>
#include <stdio.h>

struct ship {
	unsigned char  posX;//4 bits
	unsigned char  posY;//4 bits
	unsigned char  size;//3 bits
	bool dir;
};

//Siempre son 12 barcos.

unsigned char* encode (struct ship arr[]) {
	static unsigned char encoded[14] = { 0 };
	unsigned char bPos=0;

	for (char i=0; i < 9;i++) {
		encoded[bPos / 8] = encoded[bPos / 8] | (arr[i].posX << bPos%8);
		bPos = bPos + 4;
		encoded[bPos / 8] = encoded[bPos / 8] | (arr[i].posY << bPos % 8);
		bPos = bPos + 4;
		encoded[bPos / 8] = encoded[bPos / 8] | (arr[i].size << bPos % 8);
		bPos = bPos + 3;
		encoded[bPos / 8] = encoded[bPos / 8] | (arr[i].dir << bPos % 8);
		bPos = bPos + 1;

		
	}
	printf("%x", encoded[0]);
	printf("\n");
	return encoded;
} //nota, lo retornaos como el C stile string, se puede mandar de una.

struct ship* decode(unsigned char arr[]) {
	//printf("%X", arr);
	//printf("%X", arr[1]);
	static struct ship decode[9] = { 0 };
	unsigned char bPos = 0;

	for (char i = 0; i < 9;i++) {

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
	printf("%x", decode[3].posX);
	return decode;
}

//El main es solo para pruebas, lo quitamos despues.
int main(int argc, char* argv[]) {
	struct ship navy[9];
	navy[0].posX = 1;
	navy[0].posY = 1;
	navy[0].size = 2;
	navy[0].dir = 0;
	navy[1].posX = 9;
	navy[1].posY = 1;
	navy[1].size = 0;
	navy[1].dir = 0;
	navy[2].posX = 0;
	navy[2].posY = 0;
	navy[2].size = 0;
	navy[2].dir = 0;
	navy[3].posX = 9;
	navy[3].posY = 9;
	navy[3].size = 2;
	navy[3].dir = 0;
	navy[4].posX = 9;
	navy[4].posY = 9;
	navy[4].size = 2;
	navy[4].dir = 0;
	navy[5].posX = 9;
	navy[5].posY = 9;
	navy[5].size = 2;
	navy[5].dir = 0;
	navy[6].posX = 9;
	navy[6].posY = 9;
	navy[6].size = 2;
	navy[6].dir = 0;
	navy[7].posX = 9;
	navy[7].posY = 9;
	navy[7].size = 2;
	navy[7].dir = 0;
	navy[8].posX = 9;
	navy[8].posY = 9;
	navy[8].size = 2;
	navy[8].dir = 1;

	//decode(encode(navy));
	decode(encode(navy));

	//printf("%x", *(cNavy+3));
	//printf("%X",cNavy);
	//printf("%X", *cNavy);
	//printf("\n");

	//char test[3] = { 'B',69,'a' };

	//printf("%d",sizeof(bool));

	//printf("%02x", *(test+1));

}