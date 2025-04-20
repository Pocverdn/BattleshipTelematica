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



void safe_log(const char* message, const char* path, const char* ip) {
    char host_ip[INET_ADDRSTRLEN] = "unknown";

    int temp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (temp_sock != -1) {
        struct sockaddr_in remote_addr;
        memset(&remote_addr, 0, sizeof(remote_addr));
        remote_addr.sin_family = AF_INET;
        remote_addr.sin_port = htons(80); 
        inet_pton(AF_INET, "8.8.8.8", &remote_addr.sin_addr); 

        connect(temp_sock, (struct sockaddr*)&remote_addr, sizeof(remote_addr));

        struct sockaddr_in local_addr;
        socklen_t addr_len = sizeof(local_addr);
        if (getsockname(temp_sock, (struct sockaddr*)&local_addr, &addr_len) == 0) {
            inet_ntop(AF_INET, &local_addr.sin_addr, host_ip, sizeof(host_ip));
        }

        close(temp_sock);
    }

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
    fprintf(log_file, "[%04d-%02d-%02d %02d:%02d:%02d] Host IP: %s - %s (IP: %s)\n", //Imprime la fecha en el log
            t->tm_year + 1900,
            t->tm_mon + 1,
            t->tm_mday,
            t->tm_hour,
            t->tm_min,
            t->tm_sec,
            host_ip,
            message,
            ip); 

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

        if (x > SIZE || y > SIZE || board[x][y] != '~') {
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
	char sizes[9] = { 1,1,1,2,2,3,3,4,5 }; //El arreglo con los tamaños de los barcos
	for (int i = 0; i < 9; ++i) {
	struct ship s;
	bool put = false;
		do {
			int x, y, size;
			int dir;
			if (randp) {
				cout << "Barco #" << i + 1 << " de tamaño " << (int)sizes[i] <<" - Ingresa X Y Direccion(H=0/V=1): "; //En este bloque el usuario pone los barcos manualmente
				cin >> x >> y >> dir;
                if(dir != 0 && dir != 1){
                    cout << "Dirección inválida. Debe ser 0 (horizontal) o 1 (vertical).\n";
                    continue;
                }
			}
			else {
				x = rand() % 10; //En este el programa pone los barcos de forma aleatoria
				y = rand() % 10;
				dir = rand() % 1;
			}

			s.posX = x;
			s.posY = y;
			s.size = sizes[i];
			s.dir = dir;

			bool cabe = (s.dir == 0 && ((s.posX + s.size) <= 10)) || (s.dir == 1 && ((s.posY + s.size) <= 10)); //Una operación logica para saber si el barco cabe en las dimensiones del mapa.


			if (placeShipSize(board, s) && cabe) {
				ships[i] = s;
				put = true;
			}
			else if (randp) { //Un else if para decirle al usuario que el barco no cabe, es un else if para no imprimirlo cuando esta en mode aleatorio.
				cout << "Posicion inválida. Intenta de nuevo.\n";
			}

		} while (!put);
	}

}

void showBoard(char board[SIZE][SIZE], ship ships[TOTAL_SHIPS], char secondBoard[SIZE][SIZE]) {
    cout << "  ";
    for (int j = 0; j < SIZE; ++j) cout << j << " ";  //Imprime el mapa en base de la información de los barcos
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
                cout << "🚢 ";
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
        cout << "\nCoordenadas invalidas.\n";
        return false;
    }

    if (board[x][y] == 'B') { //Actualisa el mapa del enemigo local para ver donde as atacado
        board[x][y] = 'X';
        cout << "\n\n¡Acierto!\n\n";
        return true;
    } else if (board[x][y] == 'X' || board[x][y] == 'O') {
        cout << "\nYa disparaste ahí. Pierdes turno.\n";
        return false;
    } else {
        board[x][y] = 'O';
        cout << "\n\n¡Agua!\n\n";
        return false;
    }
}

int countShoot(char board[SIZE][SIZE]) {
    int count = 0; //Cuenta los barcos golpiados para saber cuando acabar el juego.
    for (int i = 0; i < SIZE; ++i)
        for (int j = 0; j < SIZE; ++j)
            if (board[i][j] == 'X') count++;
    return count;
}

int countShips(ship ships[TOTAL_SHIPS]){
    int total = 0; //para saber los golpes totales requeridos
    for (int i = 0; i < TOTAL_SHIPS; ++i){
        total += ships[i].size;
    }
        
    return total;
}

void game(int sock, char board[SIZE][SIZE], ship ships[TOTAL_SHIPS], char enemyBoard[SIZE][SIZE], int totalH, char* path, const char* server_ip) {
    attack att;
    char buffer[2];
    int totalHits = 0;
    int totalHitsNeeded = totalH;

    /*Banderas:
    T = timeout.
    t = turno.
    S = surrender.
    D = le diste.
    d = te dieron.
    G = ganaste.
    P = perdiste.
    H = hundiste.
    A = agua.
    W = wait
    */

    while (true) { //Este ciclo es en el qeu se quedan hasta acabar la partida
        memset(buffer, 0, sizeof(buffer));
        int received = recv(sock, buffer, sizeof(buffer), 0);
        if (received <= 0) {
            cerr << "Conexión cerrada o error.\n";
            break;
        }

        string msg(buffer);

        if (buffer[0] == 't') {  //Cada if es un caso diferente de lo que pasa cada turno.
            
            std::ostringstream log_msg;
            log_msg << buffer[0] << " | MENSAJE: Tu turno " << msg;
            safe_log(log_msg.str().c_str(), path, server_ip);
            
            cout << "\n>>> Es tu turno de atacar.\n\n";

            cout << "Tienes 30 segundos para ingresar tus coordenadas (Digite las coordenadas 10 10 para rendirse).\n";

            unsigned short x = 0, y = 0;
            bool inputReceived = false;
            int remainingTime = 30; // Tiempo restante en segundos
            
            while (remainingTime > 0) {
                fd_set readfds;
                struct timeval timeout;
                timeout.tv_sec = 10; // Verificar cada segundo
                timeout.tv_usec = 0;
            
                FD_ZERO(&readfds);
                FD_SET(STDIN_FILENO, &readfds);
            
                cout << "\rTiempo restante: " << remainingTime << " segundos. Ingresa las coordenadas Y X: " << std::flush;
            
                int activity = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout);
            
                if (activity > 0 && FD_ISSET(STDIN_FILENO, &readfds)) {
                    cin >> x >> y;

                    if ((x < 0 || x >= SIZE || y < 0 || y >= SIZE) && !(x == 10 && y == 10)) {
                        cout << "\nCoordenadas fuera del rango permitido (0-9). Intenta de nuevo.\n";
                        continue; // Volver a solicitar las coordenadas
                    }

                    inputReceived = true;
                    break;
                }
            
                remainingTime=remainingTime-10;
            }

            att.posX = x;
            att.posY = y;
            std::ostringstream oss;
            oss << "Input: X = " << x << ", Y = " << y;
            safe_log(oss.str().c_str(), path, server_ip);
            unsigned char serialized = encodeAttack(att);

            if ((att.posX == 10 && att.posY == 10)) {

                send(sock, &serialized, sizeof(serialized), 0);
                cout << "\n😢 Te has rendido.\n\n";
                break;

            }

            if(!inputReceived && !(att.posX == 10 && att.posY == 10)){
                cout << "\n⏳ Tiempo agotado. Pasas tu turno automáticamente.\n";
            }

            send(sock, &serialized, sizeof(serialized), 0);

            memset(buffer, 0, sizeof(buffer));
            int result = recv(sock, buffer, sizeof(buffer), 0);
            if (result <= 0) {
                cerr << "Conexión cerrada o error al recibir resultado del disparo.\n";
                break;
            }

            trim(msg = string(buffer));
            std::ostringstream log_msg2;
            log_msg2 << buffer[0] << " | MENSAJE: " << msg;
            safe_log(log_msg2.str().c_str(), path, server_ip);

            if (buffer[0] == 'D') {
                std::ostringstream resLog;
                enemyBoard[att.posX][att.posY] = 'X';
                cout << "\n\n¡Acierto!\n\n";
                totalHits++;
                resLog << "D | Acierto. Total aciertos: " << totalHits << "/" << totalHitsNeeded;
                safe_log(resLog.str().c_str(), path, server_ip);
            
            } else if (buffer[0] == 'H') {
                std::ostringstream resLog;
                enemyBoard[att.posX][att.posY] = 'X';
                cout << "\n\n💥 ¡Hundiste el barco! 💥\n\n";
                totalHits++;
                resLog << "H | Hundido. Total aciertos: " << totalHits << "/" << totalHitsNeeded;
                safe_log(resLog.str().c_str(), path, server_ip);
            
            } else if (buffer[0] == 'A') {
                std::ostringstream resLog;
                enemyBoard[att.posX][att.posY] = 'O';
                cout << "\n\n¡Agua!\n\n";
                resLog << "A | Agua.";
                safe_log(resLog.str().c_str(), path, server_ip);
            }
            

            showBoard(board, ships, enemyBoard);

        } else if (buffer[0] == 'd') {
            attack atk = decodeAttack(buffer[1]);
            board[atk.posX][atk.posY] = 'X';
            std::ostringstream oss;
            oss << "¡Tu enemigo te ha dado en X: " << (short)atk.posX << " Y: " << (short)atk.posY;
            safe_log(oss.str().c_str(), path, server_ip);

            cout << "\n💥 ¡Tu enemigo te ha dado en X: " << (short)atk.posX << " Y: " << (short)atk.posY << "\n\n";
            showBoard(board, ships, enemyBoard);

        } else if (buffer[0] == 'w') {
            cout << "\n Turno del enemigo \n";
        
            std::ostringstream log_msg;
            log_msg << "w | MENSAJE: Turno del enemigo";
            safe_log(log_msg.str().c_str(), path, server_ip);
        }
        else if (buffer[0] == 'G') {
            std::ostringstream winLog;
            winLog << "G | Resultado: Has ganado la partida.";
            safe_log(winLog.str().c_str(), path, server_ip);
            cout << "\n🎉 ¡Has ganado la partida!\n\n";
            break;

        } else if (buffer[0] == 'P') {
            std::ostringstream loseLog;
            loseLog << "P | Resultado: Has perdido la partida.";
            safe_log(loseLog.str().c_str(), path, server_ip);
            cout << "\n😢 Has perdido la partida.\n\n";
            break;
            
        }
    }
}

void chat_with_server(int client_fd,char* path,const char* server_ip) {
    
    std::string username;
    std::string email;

    registration(email, username, client_fd);
    
    char board1[SIZE][SIZE];
    char board2[SIZE][SIZE];
    ship ships1[TOTAL_SHIPS];  //Iniicialisamos las variables.

    initializeBoard(board1);
    initializeBoard(board2);
    setShips(board1, ships1, username);
    showBoard(board1, ships1, board2);  //Se conectan los jugadores y empieza la partida

    std::cout << "\n\nEsperando oponente\n\n";
    char log_msg[512];
    snprintf(log_msg, sizeof(log_msg), "Usuario conectado: %s, Email conectado: %s", username.c_str(), email.c_str());
    safe_log(log_msg, path,server_ip);

    int totalHitsNeeded = countShips(ships1);

    unsigned char serialized[14];
    encode(ships1, serialized);
    send(client_fd, serialized, sizeof(serialized), 0);

    game(client_fd, board1, ships1, board2, totalHitsNeeded,path,server_ip);

}

inline void randSeed() {
    time_t tiempo; //Se genera la semilla las funciones de azar usando la hora del sistema.
    time(&tiempo);
    //printf("%d", tiempo);
    srand(tiempo);
    //printf("\n%d", rand());
}

inline void start(char* log) {
    Config config;
    parse_config("address.config", config.server_ip, &config.PORTLINUX);
    while (strlen(log) == 0) {
        printf("Log path cannot be empty. Please enter a valid log path: ");
        fgets(log, 256, stdin);
        
        // Remove trailing newline, if present
        size_t len = strlen(log);
        if (len > 0 && log[len - 1] == '\n') {
            log[len - 1] = '\0';
        }
    }
    
    while (true) { //Se maneja la conexion y luego se empieza el juego.
        auto [client_fd, server_ip] = connect_to_server(config);
        if (client_fd < 0) return;

        chat_with_server(client_fd, log, server_ip.c_str());

        int playing;
        cout << "\n¿Quieres jugar otra partida? (1 = sí / 0 = no): ";
        cin >> playing;

        if (!playing) {
            cout << "Gracias por jugar. ¡Hasta la próxima!\n";
            break;
        }

        system("clear");

        close(client_fd);

    }
    return;
}

int main(int argc, char* argv[]) {
    char log[256] = "";
    randSeed();
    if (argc > 1) {
        strncpy(log, argv[1], sizeof(log) - 1);
    }
    // Conexiones de cliente a socket
    start(log);

    return 0;
}