#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
//#ifdef _WIN32
#include <time.h>
#include <iostream>
/*#else
#include <sys/time.h>
#endif*/
using namespace std;

struct ship {
	unsigned char  posX;//4 bits
	unsigned char  posY;//4 bits
	unsigned char  size;//3 bits
	bool dir;
	unsigned char hp = 0;
};

struct attack {
	unsigned char  posX;//4 bits
	unsigned char  posY;//4 bits
};

//Siempre son 12 barcos.
void initializeBoard(char board[10][10]) {
	for (int i = 0; i < 10; i++) {
		for (int j = 0; j < 10; j++) {
			board[i][j] = '~'; // signo para el agua no revelada
		}
	}
}

bool placeShipSize(char board[10][10],struct ship s) {
	for (int i = 0; i < s.size; ++i) {
		int x = s.posX + (s.dir ? i : 0);
		int y = s.posY + (s.dir ? 0 : i);

		if (x >= 10 || y >= 10 || board[x][y] != '~') {
			return false; // fuera del tablero o espacio ocupado
		}
	}

	for (int i = 0; i < s.size; ++i) {
		int x = s.posX + (s.dir ? i : 0);
		int y = s.posY + (s.dir ? 0 : i);
		board[x][y] = 'B';
	}

	return true;
}



void setShips(char board[10][10],struct ship ships[9], int playerNumber) {
	cout << "\nJugador " << playerNumber << ", quieres colocar tus barcos (1 para sí o 0 para no)?\n";
	bool randp;
	cin >> randp;
	char sizes[9] = { 1,1,1,2,2,3,3,4,5 };
	for (int i = 0; i < 9; ++i) {
	struct ship s;
	bool put = false;
		do {
			int x, y, size;
			int dir;
			if (randp) {
				cout << "Barco #" << i + 1 << " - Ingresa X Y Tamano Direccion(H=0/V=1): ";
				cin >> x >> y >> dir;
			}
			else {
				x = rand() % 10;
				y = rand() % 10;
				dir = rand() % 1;
			}

			s.posX = x;
			s.posY = y;
			s.size = sizes[i];
			s.dir = dir;

			bool cabe = (s.dir == 0 && ((s.posX + s.size) <= 10)) || (s.dir == 1 && ((s.posY + s.size) <= 10));


			if (placeShipSize(board, s) && cabe) {
				ships[i] = s;
				put = true;
			}
			else if (randp) {
				cout << "Posicion inválida. Intenta de nuevo.\n";
			}

		} while (!put);
	}

}

void setShipsServer(char board[10][10], struct ship ships[9], int playerNumber) {
	for (int i = 0; i < 9; ++i) 
		placeShipSize(board, ships[i]);
}

void showBoard(char board[10][10]) {
	cout << "  ";
	for (int j = 0; j < 10; ++j) cout << j << " ";
	cout << endl;
	for (int i = 0; i < 10; ++i) {
		cout << i << " ";
		for (int j = 0; j < 10; ++j) {
			cout << board[i][j] << " ";
		}
		cout << endl;
	}
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

unsigned char* encode (struct ship arr[]) { //Podemos a�adir una variable "K" para que la cantidad de barcos sea dynamica.
	static unsigned char encoded[14] = { 0 }; //Con variable dynamica el tama�o de este char seria (Roof)K*3/2. cada barco es de 12 bits, y lo que mandamos esta en bytes, lo mismo aplica para las otras funciones.
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
	printf("El primer byte de la cadena codificada: ");
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
	printf("El pos X del cuarto barco: ");
	printf("%x", decode[3].posX);
	printf("\n");
	return decode;
}

unsigned char encodeAttack(struct attack A) {
	unsigned char encoded;


	encoded =  A.posX;
	encoded = encoded | (A.posY << 4);

	printf("El byte del ataque: ");
	printf("%x", encoded);
	printf("\n");
	return encoded;
}

struct attack decodeAttack(unsigned char A) {
	struct attack decoded;


	decoded.posX = A & 0xF;
	decoded.posY = (A & 0xF0) >> 4;

	printf("La posX del ataque: ");
	printf("%x", decoded.posX);
	printf("\n");
	printf("La posY del ataque: ");
	return decoded;
}


//El main es solo para pruebas, lo quitamos despues.
int main(int argc, char* argv[]) {

	time_t tiempo;
	time(&tiempo);
	printf("%d", tiempo);
	srand(tiempo);
	printf("\n%d", rand());
	//struct ship* navy = inputShips();
	char board1[10][10];
	struct ship ships[9];

	initializeBoard(board1);
	setShips(board1, ships, 1);
	showBoard(board1);
	//decode(encode(navy));
	struct ship* navy = decode(encode(ships));
	for (int i = 0; i < 9; ++i) {
		printf("Barco #%d -> X: %d, Y: %d, Tamaño: %d, Dirección: %s\n",
			i + 1,
			navy[i].posX,
			navy[i].posY,
			navy[i].size,
			navy[i].dir ? "Vertical" : "Horizontal");
	}
	struct attack a;
	a.posX = 4;
	a.posY = 6;
	unsigned char b [2];
	b[1] = encodeAttack(a);
	b[0] = 'A';
	printf("%x %x \n",b[0],b[1]);

	printf("%X\n", decodeAttack(encodeAttack(a)).posY);

	printf("\nEl posX de la prueba:%X\n", decodeAttack(b[1]).posX);

	//time_t Tiempo;
	time_t current;
	time(&tiempo);
	tiempo = tiempo + 10;
	do {

		time(&current);
		cout << (tiempo - current) << "\r";
	} while ((current < tiempo));
}
