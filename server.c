#include <fcntl.h>     
#include <sys/file.h>  

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
//Librerias sockets para linux
#include <arpa/inet.h>
#include <unistd.h> 
#include <pthread.h>
#include <time.h>
// Puerto
//#define PORT 8080

//14 bytes para envio de posiciones de barcos - 1 byte para comfirmación de disparo
#define BUFFER_SIZE 14
#define BUFFER_SIZE_Confirm 1

// Maximo de secciones
#define MAX_SESSIONS 10

// Logica del juego
#define SIZE 10
#define TOTAL_SHIPS 9

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
    unsigned char hp;
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

GameSession game_sessions[MAX_SESSIONS];
int current_session = 0;
pthread_mutex_t session_mutex = PTHREAD_MUTEX_INITIALIZER;

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

    printf("El primer byte de la cadena codificada: %02X\n", encoded[0]);
    return encoded;
}

void initializeBoard(char board[SIZE][SIZE]) {
    for(int i = 0; i < SIZE; i++){
        for(int j = 0; j < SIZE; j++){
            board[i][j] = '~'; // Agua no revelada
        }
    }
}
//

void safe_log(const char* message) {
    int fd = open("log.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
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
//
bool placeShipSize(char board[SIZE][SIZE], ship s) {
    for(int i = 0; i < s.size; ++i) {
        int x = s.posX + (s.dir ? i : 0);
        int y = s.posY + (s.dir ? 0 : i);
        board[x][y] = 'B';
    }

    return true;
}

void setShips(char board[SIZE][SIZE], ship ships[TOTAL_SHIPS], char* username) {
    printf("\nJugador %s, coloca tus %d barcos.\n", username, TOTAL_SHIPS);
    for (int i = 0; i < TOTAL_SHIPS; ++i) {
        placeShipSize(board, ships[i]);
    }
}

void showBoard(char board[SIZE][SIZE], ship ships[TOTAL_SHIPS]) {
    printf("  ");
    for (int j = 0; j < SIZE; ++j) printf("%d ", j);
    printf("\n");

    for (int i = 0; i < SIZE; ++i) {
        printf("%d ", i);
        for (int j = 0; j < SIZE; ++j) {
            int isShip = 0;

            for (int k = 0; k < TOTAL_SHIPS; ++k) {
                ship s = ships[k];
                for (int l = 0; l < s.size; ++l) {
                    int x = s.posX + (s.dir ? l : 0);
                    int y = s.posY + (s.dir ? 0 : l);

                    if (x == i && y == j) {
                        isShip = 1;
                        break;
                    }
                }
                if (isShip) break;
            }

            if (isShip) {
                printf("B ");
            } else if (board[i][j] == 'X' || board[i][j] == 'O') {
                printf("%c ", board[i][j]);
            } else {
                printf("~ ");
            }
        }
        printf("\n");
    }
}

bool shoot(char board[SIZE][SIZE], int x, int y) {
    if (x < 0 || x >= SIZE || y < 0 || y >= SIZE) {
        printf("Coordenadas inválidas.\n");
        return false;
    }

    printf("%c \n", board[x][y]);

    if (board[x][y] == 'B') {
        board[x][y] = 'X';
        printf("¡Acierto!\n");
        return true;
    } else if (board[x][y] == 'X' || board[x][y] == 'O') {
        printf("Ya disparaste ahí. Pierdes turno.\n");
        return false;
    } else {
        board[x][y] = 'O';
        printf("¡Agua!\n");
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

int countShips(ship ships[TOTAL_SHIPS]) {
    int total = 0;
    for (int i = 0; i < TOTAL_SHIPS; ++i)
        total += ships[i].size;
    return total;
}

struct ship* deserializeShips(const char* serialized, int* count) {
    static struct ship ships[9]; // Static para que persista luego del return
    *count = 0;

    char input[BUFFER_SIZE];
    strncpy(input, serialized, BUFFER_SIZE);
    input[BUFFER_SIZE - 1] = '\0';

    char* token = strtok(input, ";");
    while (token != NULL && *count < 9) {
        int x, y, size, dir;
        if (sscanf(token, "%d %d %d %d", &x, &y, &size, &dir) == 4) {
            ships[*count].posX = x;
            ships[*count].posY = y;
            ships[*count].size = size;
            ships[*count].dir = dir;
            (*count)++;
        }
        token = strtok(NULL, ";");
    }

    return ships;
}

/*
int send_encoded_ships(int client_fd, ship ships[]) {
    unsigned char *buffer = encode(ships);
    int sent = send(client_fd, buffer, strlen(buffer), 0);
    printf("Sent: %d\n", sent);
    if (sent == -1) {
        perror("Error enviando barco codificado");
        return -1;
    }
    printf("Sent: %d\n", sent);
}
*/

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

int send_encoded_ships(int client_fd, ship ships[]) {
    unsigned char *buffer = encode(ships);
    int sent = write(client_fd, buffer, 14);
    printf("Sent: %d\n", sent);
    if (sent != 14) {
        perror("Error enviando barcos codificados");
        return -1;
    }
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

void *handle_games(void *client_socket){
    int new_socket = *(int *)client_socket;
    free(client_socket);

    ship player_ships[TOTAL_SHIPS];
    char username[50] = {0};
    char email[50] = {0};
    char log_msg[256];
    const char *hello = "Message received";

    int bytes_received = recv(new_socket, username, sizeof(username) - 1, 0);
    int bytes_receivede = recv(new_socket, email, sizeof(email) - 1, 0);
    if (bytes_received>0) {
        username[bytes_received] = '\0';
        email[bytes_receivede] = '\0';
        printf("Usuario conectado: %s\n", username);
        printf("Email conectado: %s\n", email);
        snprintf(log_msg, sizeof(log_msg), "Usuario conectado: %s | Email: %s", username, email);
        safe_log(log_msg);
    } else {
        printf("No se recibió nombre de usuario\n");
        close(new_socket);
        return NULL;
    }

    if (receive_encoded_ships(new_socket, player_ships) < 0) {
        close(new_socket);
        return NULL;
    }

    pthread_mutex_lock(&session_mutex);

    GameSession *session = &game_sessions[current_session];

    if (session->player1_fd == 0) {
        
        session->player1_fd = new_socket;
        strncpy(session->player1_name, username, sizeof(session->player1_name));
        memcpy(session->ships1, player_ships, sizeof(ship) * TOTAL_SHIPS);

        for (int i = 0; i < 9; ++i) {
            printf("Barco #%d -> X: %d, Y: %d, Tamaño: %d, Dirección: %s\n",
                   i + 1,
                   session->ships1[i].posX,
                   session->ships1[i].posY,
                   session->ships1[i].size,
                   session->ships1[i].dir ? "Vertical" : "Horizontal");   
        }

    } else {
        session->player2_fd = new_socket;
        strncpy(session->player2_name, username, sizeof(session->player2_name));
        memcpy(session->ships2, player_ships, sizeof(ship) * TOTAL_SHIPS);
        
        char board1[SIZE][SIZE];
        char board2[SIZE][SIZE];

        initializeBoard(board1);
        initializeBoard(board2);

        showBoard(board1, session->ships1);
        showBoard(board2, session->ships2);

        setShips(board1, session->ships1, session->player1_name);
        setShips(board2, session->ships2, session->player2_name);

        printf("%c\n",board1[1][1]);

        int hits1 = 0;
        int hits2 = 0;

        int totalHitsNeeded = countShips(session->ships1);

        printf("\n---- ¡Comienza el juego! ----\n");
        bool turn = true;
        unsigned char at;

        while (hits1 < totalHitsNeeded && hits2 < totalHitsNeeded) {
            printf("\nNuevo turno\n");
            if (turn) {
                send(session->player1_fd, "Tu turno", strlen("Tu turno") + 1, 0);

                
                
                int bytes_received = recv(session->player1_fd, &at, sizeof(at), 0);

                
                if (bytes_received <= 0) {
                    printf("Error al recibir ataque de jugador 1\n");
                    break;
                }

                attack att = decodeAttack(at);

                printf("Jugador 1 ataca: x = %d, y = %d\n", att.posX, att.posY);
                snprintf(log_msg, sizeof(log_msg),"Jugador 1 ataca: x = %d, y = %d\n", att.posX, att.posY);
                safe_log(log_msg);
                if (shoot(board2, att.posX, att.posY)) {
                    hits1++;
                    send(session->player1_fd, "Acierto", strlen("Acierto") + 1, 0);
                }else {
                    send(session->player1_fd, "Agua", strlen("Agua") + 1, 0);
                }
                

            } else {
                send(session->player2_fd, "Tu turno", strlen("Tu turno") + 1, 0);

                int bytes_received = recv(session->player2_fd, &at, sizeof(at), 0);

                
                if (bytes_received <= 0) {
                    printf("Error al recibir ataque de jugador 2\n");
                    break;
                }

                attack att = decodeAttack(at);
                snprintf(log_msg, sizeof(log_msg),"Jugador 2 ataca: x = %d, y = %d\n", att.posX, att.posY);
                safe_log(log_msg);
                printf("Jugador 2 ataca: x = %d, y = %d\n", att.posX, att.posY);

                if (shoot(board1, att.posX, att.posY)) {
                    hits2++;
                    send(session->player2_fd, "Acierto", strlen("Acierto") + 1, 0);
                }else {
                    send(session->player2_fd, "Agua", strlen("Agua") + 1, 0);
                }
                

            }
            turn = !turn;
        }

        if (hits1 >= totalHitsNeeded) {
            send(session->player1_fd, "¡Ganaste!", 9, 0);
            send(session->player2_fd, "Perdiste", 8, 0);
        } else {
            send(session->player2_fd, "¡Ganaste!", 9, 0);
            send(session->player1_fd, "Perdiste", 8, 0);
        }
        
        current_session++;
        if (current_session >= MAX_SESSIONS) {
            current_session = 0;
        }
    }
    
    pthread_mutex_unlock(&session_mutex);
    
    
    return NULL;

}


extern inline int setup_server(Server *server, char* IP, char* port) {
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

extern inline void accept_clients(Server *server) {
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

        if (pthread_create(&thread_id, NULL, handle_games, (void *)new_socket) != 0) {
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
