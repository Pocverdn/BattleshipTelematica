#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <time.h>
#include <limits>
#include <pthread.h>
#include <sys/file.h>  
#include <fcntl.h>
#include "protocolo.h"
using namespace std;

//#define BUFFER_SIZE 14

#define SIZE 10
#define TOTAL_SHIPS  9



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

void initializeBoard(char board[SIZE][SIZE]){
    for(int i = 0; i < SIZE; i++){
        for (int j = 0; j < SIZE; j++){
            board[i][j] = '~'; // signo para el agua no revelada
        }
    }
}

bool placeShipSize(char board[SIZE][SIZE], ship s) {
    for (int i = 0; i < s.size; ++i) {
        int x = s.posX + (s.dir ? i : 0);
        int y = s.posY + (s.dir ? 0 : i);

        if (x >= SIZE || y >= SIZE || board[x][y] != '~') {
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

void setShips(char board[10][10],struct ship ships[9], string username) {
	cout << "\nJugador " << username << ", quieres colocar tus barcos (1 para sí o 0 para no)?\n";
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
				cout << "Barco #" << i + 1 << " de tamaño " << (int)sizes[i] <<" - Ingresa X Y Direccion(H=0/V=1): ";
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

void showBoard(char board[SIZE][SIZE], ship ships[TOTAL_SHIPS], char secondBoard[SIZE][SIZE]) {
    cout << "  ";
    for (int j = 0; j < SIZE; ++j) cout << j << " ";
    cout << "   "; 
    for (int j = 0; j < SIZE; ++j) cout << j << " ";
    cout << endl;

    for (int i = 0; i < SIZE; ++i) {
        cout << i << " ";
        for (int j = 0; j < SIZE; ++j) {
            bool isShip = false;

            for (int k = 0; k < TOTAL_SHIPS; ++k) {
                ship s = ships[k];
                for (int l = 0; l < s.size; ++l) {
                    int x = s.posX + (s.dir ? l : 0);
                    int y = s.posY + (s.dir ? 0 : l);

                    if (x == i && y == j && board[x][y] != 'X') {
                        isShip = true;
                        break;
                    }
                }
                if (isShip) break;
            }

            if (isShip) {
                cout << "B ";
            } else if (board[i][j] == 'X' || board[i][j] == 'O') {
                cout << board[i][j] << " ";
            } else {
                cout << "~ ";
            }
        }

        cout << "   ";

        for (int j = 0; j < SIZE; ++j) {
            if (secondBoard[i][j] == 'X' || secondBoard[i][j] == 'O') {
                cout << secondBoard[i][j] << " ";
            } else {
                cout << "~ ";
            }
        }
        cout << endl;
    }
}

bool shoot(char board[SIZE][SIZE], int x, int y) {
    if (x < 0 || x >= SIZE || y < 0 || y >= SIZE) {
        cout << "Coordenadas invalidas.\n";
        return false;
    }

    if (board[x][y] == 'B') {
        board[x][y] = 'X';
        cout << "¡Acierto!\n";
        return true;
    } else if (board[x][y] == 'X' || board[x][y] == 'O') {
        cout << "Ya disparaste ahí. Pierdes turno.\n";
        return false;
    } else {
        board[x][y] = 'O';
        cout << "¡Agua!\n";
        return false;
    }
}

int countShoot(char board[SIZE][SIZE]) {
    int count = 0;
    for (int i = 0; i < SIZE; ++i)
        for (int j = 0; j < SIZE; ++j)
            if (board[i][j] == 'X') count++;
    return count;
}

int countShips(ship ships[TOTAL_SHIPS]){
    int total = 0;
    for (int i = 0; i < TOTAL_SHIPS; ++i){
        total += ships[i].size;
    }
        
    return total;
}

void* timed_in(void* att) {

    //printf("Thread");
    unsigned short* mgs = (unsigned short*)att;

    //printf("%d", mgs[0]);
    cin >> mgs[0] >> mgs[1];
    //printf("owo");
    //printf("%x", mgs[0]);
    return NULL;
}

void game(int sock, char board[SIZE][SIZE], ship ships[TOTAL_SHIPS], char enemyBoard[SIZE][SIZE], int totalH,char* path) {
    attack att;
    char buffer[32];
    int totalHits = 0;
    int totalHitsNeeded = totalH;

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int received = recv(sock, buffer, sizeof(buffer), 0);
        if (received <= 0) {
            cerr << "Conexión cerrada o error.\n";
            break;
        }

        string msg(buffer);
        trim(msg);
        safe_log(msg.c_str(),path);
        if (msg.rfind("Impacto", 0) == 0) {
            int x, y;
            sscanf(msg.c_str(), "Impacto %d %d", &x, &y);
            board[x][y] = 'X';
            std::ostringstream oss;
            oss << "¡Tu enemigo te ha dado en X: " << x << " Y: " << y;
            safe_log(oss.str().c_str(), path);
            cout << "\n💥 ¡Tu enemigo te ha dado en X: " << x << " Y: " << y << "\n\n";
            showBoard(board, ships, enemyBoard);
            //msg = "turn";
        }
        if (msg == "turn") {
            cout << "\n>>> Es tu turno de atacar.\n";

            //short x, y;
            unsigned short* x = new unsigned short[2]; //x[0] es x t x[1] Y
            void* arg;
            x[0] = 0;
            x[1] = 247;

            arg = x;
            do {
                cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                pthread_t thread_id;
                cout << "Digite las coordenadas 10 10 para rendirse\n";
                cout << "Ingresa coordenadas Y X: ";
                pthread_create(&thread_id, NULL, timed_in, arg);
                time_t tiempo;
                time_t current;
                time(&tiempo);
                tiempo = tiempo + 27;
                do {

                    time(&current);
                    //cout << (tiempo - current) << "\r";
                } while ((current < tiempo) & (x[1]==247));
            pthread_cancel(thread_id);
            x[1] = (x[1] == 247 ? 0 : x[1]);
            } while (x[0] > SIZE || x[1] > SIZE);//while (x[0] < 0 || x[0] > SIZE || x[1] < 0 || x[1] > SIZE);

            att.posX = x[0];
            att.posY = x[1];
            std::ostringstream oss;
            oss << "Input: X = " << x[0] << ", Y = " << x[1];
            safe_log(oss.str().c_str(), path);
            delete x;
            unsigned char serialized = encodeAttack(att);

            system("clear");

            if (att.posX == 10 && att.posY == 10) {

                send(sock, &serialized, sizeof(serialized), 0);
                cout << "\n😢 Te has rendido.\n\n";
                break;

            }

            send(sock, &serialized, sizeof(serialized), 0);

            memset(buffer, 0, sizeof(buffer));
            int result = recv(sock, buffer, sizeof(buffer), 0);
            if (result <= 0) {
                cerr << "Conexión cerrada o error al recibir resultado del disparo.\n";
                break;
            }

            trim(msg = string(buffer));
            
            if (msg == "timeout") {
                cout << "⏰ Te quedaste sin tiempo para atacar. Pierdes el turno.\n\n";
        
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cin.clear();
        
            } else if (msg == "Acierto") {
                enemyBoard[att.posX][att.posY] = 'X';
                cout << "¡Acierto!\n\n";
                totalHits++;
            } else if (msg == "Hundir") {
                enemyBoard[att.posX][att.posY] = 'X';
                cout << "\n💥 ¡Hundiste el barco! 💥\n\n";
                totalHits++;
            } else if (msg == "Agua") {
                enemyBoard[att.posX][att.posY] = 'O';
                cout << "\n¡Agua!\n\n";
            }

            showBoard(board, ships, enemyBoard);

        } else if(msg == "wait"){
            cout << "\n Turno del enemigo \n";

        }  else if (msg == "Ganaste") {
            cout << "\n🎉 ¡Has ganado la partida!\n\n";
            break;
        } else if (msg == "Perdiste") {
            cout << "\n😢 Has perdido la partida.\n\n";
            break;
        }

    }

}

void chat_with_server(int client_fd,char* path) {
    
    std::string username;
    std::string email;

    registration(email, username, client_fd);
    
    char board1[SIZE][SIZE];
    char board2[SIZE][SIZE];
    ship ships1[TOTAL_SHIPS];

    initializeBoard(board1);
    initializeBoard(board2);
    setShips(board1, ships1, username);
    showBoard(board1, ships1, board2);

    std::cout << "\n\nEsperando oponente\n\n";
    char log_msg[512];
    snprintf(log_msg, sizeof(log_msg), "Usuario conectado: %s, Email conectado: %s", username.c_str(), email.c_str());
    safe_log(log_msg, path);

    int totalHitsNeeded = countShips(ships1);

    unsigned char serialized[14];
    encode(ships1, serialized);
    send(client_fd, serialized, sizeof(serialized), 0);

    game(client_fd, board1, ships1, board2, totalHitsNeeded,path);

}

inline void randSeed() {
    time_t tiempo;
    time(&tiempo);
    //printf("%d", tiempo);
    srand(tiempo);
    //printf("\n%d", rand());
}

int main(int argc, char* argv[]) {

    // Conexiones de cliente a soket
    randSeed();
    Config config;
    parse_config("address.config", config.server_ip, &config.PORTLINUX);

    


    while(true){

        int client_fd = connect_to_server(config);
        if (client_fd < 0) return -1;

        chat_with_server(client_fd,argv[1]);

        int playing;
        cout << "\n¿Quieres jugar otra partida? (1 = sí / 0 = no): ";
        cin >> playing;

        if(!playing){
            cout << "Gracias por jugar. ¡Hasta la próxima!\n";
            break;
        }

        close(client_fd);

    }

    return 0;
}