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
    ServerState *state;
} ThreadArgs;

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

void initializeBoard(char board[SIZE][SIZE]) {
    for(int i = 0; i < SIZE; i++){
        for(int j = 0; j < SIZE; j++){
            board[i][j] = '~'; // Agua no revelada
        }
    }
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

bool shoot(int socket, char board[SIZE][SIZE], struct ship ships[TOTAL_SHIPS], bool *sunk, int x, int y) {
    if (x < 0 || x >= SIZE || y < 0 || y >= SIZE) {
        printf("Coordenadas inválidas.\n");
        return false;
    }

    // Verificar si ya se disparó en esa posición
    if (board[x][y] == 'X' || board[x][y] == 'O') {
        printf("Ya disparaste ahí. Pierdes turno.\n");
        return false;
    }

    // Revisar si el disparo impacta un barco
    for (int i = 0; i < TOTAL_SHIPS; i++) {
        ship *s = &ships[i];
        for (int j = 0; j < s->size; j++) {
            int impactX = s->posX + (s->dir ? j : 0);
            int impactY = s->posY + (s->dir ? 0 : j);

            if (x == impactX && y == impactY) {
                s->impacts++;
                board[x][y] = 'X';
                printf("¡Acierto!\n");

                if (s->impacts == s->size) {
                    printf("¡barco %d hundido (tamaño %d)!\n", i + 1, s->size);
                    *sunk = true;
                }

                return true;
            }
        }
    }

    // Si no impactó ningún barco
    board[x][y] = 'O';
    printf("¡Agua!\n");
    return false;
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

void receive_player_info(int socket, char *username, char *email,char* path) {
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
    safe_log(log_msg,path);
}

void initialize_session(GameSession *session, int socket, const char *username, ship *ships) {
    session->player1_fd = socket;
    strncpy(session->player1_name, username, sizeof(session->player1_name));
    memcpy(session->ships1, ships, sizeof(ship) * TOTAL_SHIPS);

    for (int i = 0; i < TOTAL_SHIPS; ++i) {
        printf("Barco #%d -> X: %d, Y: %d, Tamaño: %d, Dirección: %s\n",
               i + 1, ships[i].posX, ships[i].posY, ships[i].size,
               ships[i].dir ? "Vertical" : "Horizontal");
    }
}

void send_turn_messages(int active_fd, int waiting_fd) {
    send(active_fd, "turn", strlen("turn") + 1, 0);
    send(waiting_fd, "wait", strlen("wait") + 1, 0);
}

void handle_turn(GameSession *session, char board[SIZE][SIZE], struct ship ships[TOTAL_SHIPS], int attacker_fd, int defender_fd, int *hits, char *username, bool *giveUp, char* path) {
    
    fd_set read_fds;
    struct timeval timeout;
    unsigned char at;
    
    FD_ZERO(&read_fds);
    FD_SET(attacker_fd, &read_fds);

    timeout.tv_sec = 30;
    timeout.tv_usec = 0;

    int activity = select(attacker_fd + 1, &read_fds, NULL, NULL, &timeout);

    if (activity == -1) {
        perror("Error en select");
        
    } else if (activity == 0) {
        // Tiempo agotado
        send(attacker_fd, "timeout", strlen("timeout") + 1, 0);
        printf("Jugador %s perdió su turno.\n", username);
    } else {
        bool sunk = false;
        int bytes = recv(attacker_fd, &at, 1, 0);
        if (bytes <= 0) {
            perror("Error recibiendo ataque");
        }

        attack att = decodeAttack(at);
        char log_msg[128];
        snprintf(log_msg, sizeof(log_msg), "Jugador %s ataca: x = %d, y = %d", username, att.posX, att.posY);
        printf("%s\n", log_msg);
        safe_log(log_msg,path);

        if (att.posX == 10 && att.posY == 10) {
            char surrender_msg[64];
            snprintf(surrender_msg, sizeof(surrender_msg), "Jugador %s se ha rendido.", username);
            printf("%s\n", surrender_msg);
            safe_log(surrender_msg, path);

            *giveUp = true;
        
            send(defender_fd, "Ganaste", strlen("Ganaste") + 1, 0);
        }

        if (shoot(attacker_fd, board, ships, &sunk, att.posX, att.posY)) {
            (*hits)++;

            if(sunk){
                printf("Hundido \n");
                send(attacker_fd, "Hundir", strlen("Hundir") + 1, 0);
                char impact_msg[32];
                snprintf(impact_msg, sizeof(impact_msg), "Impacto %d %d", att.posX, att.posY);
                send(defender_fd, impact_msg, strlen(impact_msg) + 1, 0);
            }else{
                send(attacker_fd, "Acierto", strlen("Acierto") + 1, 0);
                char impact_msg[32];
                snprintf(impact_msg, sizeof(impact_msg), "Impacto %d %d", att.posX, att.posY);
                send(defender_fd, impact_msg, strlen(impact_msg) + 1, 0);
            }
            
        } else {
            send(attacker_fd, "Agua", strlen("Agua") + 1, 0);
        }
    }
}

void play_game(GameSession *session, char *path) {
    // current_session = (current_session + 1) % MAX_SESSIONS;

    char board1[SIZE][SIZE], board2[SIZE][SIZE];
    initializeBoard(board1);
    initializeBoard(board2);

    showBoard(board1, session->ships1);
    showBoard(board2, session->ships2);

    setShips(board1, session->ships1, session->player1_name);
    setShips(board2, session->ships2, session->player2_name);

    int hits1 = 0, hits2 = 0;
    int totalHits = countShips(session->ships1);
    bool turn = true;
    bool giveUp1 = false, giveUp2 = false;

    printf("\n---- ¡Comienza el juego! ----\n");

    while (hits1 < totalHits && hits2 < totalHits) {
        printf("\nNuevo turno\n");

        if (turn) {
            send_turn_messages(session->player1_fd, session->player2_fd);
            handle_turn(session, board2, session->ships2, session->player1_fd, session->player2_fd, &hits1, session->player1_name, &giveUp1, path);

            if (giveUp1) {
                break;
            }

        } else {
            send_turn_messages(session->player2_fd, session->player1_fd);
            handle_turn(session, board1, session->ships1, session->player2_fd, session->player1_fd, &hits2, session->player2_name, &giveUp2, path);

            if (giveUp2) {
                break;
            }
        }

        turn = !turn;
    }

    if (hits1 >= totalHits) {
        send(session->player1_fd, "Ganaste", 9, 0);
        send(session->player2_fd, "Perdiste", 8, 0);
        printf("🎉 %s ganó la partida contra %s\n", session->player1_name, session->player2_name);
    } else {
        send(session->player2_fd, "Ganaste", 9, 0);
        send(session->player1_fd, "Perdiste", 8, 0);
        printf("🎉 %s ganó la partida contra %s\n", session->player2_name, session->player1_name);
    }
}

void *handle_games(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    int new_socket = args->client_socket;
    char *path = args->path;
    ServerState *state = args->state;

    ship player_ships[TOTAL_SHIPS];
    char username[50], email[50];

    receive_player_info(new_socket, username, email, path);

    if (receive_encoded_ships(new_socket, player_ships) < 0) {
        close(new_socket);
        free(args);
        return NULL;
    }

    pthread_mutex_lock(&state->session_mutex);

    int paired = 0;

    for (int i = 0; i < state->current_session; ++i) {
        GameSession *session = &state->sessions[i];
        if (session->player1_fd != 0 && session->player2_fd == 0) {
            
            session->player2_fd = new_socket;
            strncpy(session->player2_name, username, sizeof(session->player2_name));
            memcpy(session->ships2, player_ships, sizeof(ship) * TOTAL_SHIPS);
            paired = 1;

            pthread_mutex_unlock(&state->session_mutex);

            printf("Jugador %s se ha emparejado con %s (sesión %d)\n",
                   username, session->player1_name, i);

            play_game(session, path); 
            free(args);
            return NULL;
        }
    }

    if (state->current_session < MAX_SESSIONS) {
        GameSession *session = &state->sessions[state->current_session];
        memset(session, 0, sizeof(GameSession));

        initialize_session(session, new_socket, username, player_ships);
        state->current_session++;
        printf("Jugador %s está esperando oponente...\n", username);
    } else {
        printf("¡Límite de sesiones alcanzado!\n");
        close(new_socket);
    }

    pthread_mutex_unlock(&state->session_mutex);
    free(args);
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

extern inline void accept_clients(Server *server, char *path, ServerState *state) {
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

        ThreadArgs *args = malloc(sizeof(ThreadArgs));
        if (!args) {
            perror("malloc");
            close(*new_socket);
            free(new_socket);
            continue;
        }

        args->client_socket = *new_socket;
        strncpy(args->path, path, sizeof(args->path));
        args->path[sizeof(args->path) - 1] = '\0';
        args->state = state;

        if (pthread_create(&thread_id, NULL, handle_games, (void *)args) != 0) {
            perror("Thread creation failed");
            close(*new_socket);
            free(new_socket);
            free(args);
            continue;
        }

        pthread_detach(thread_id);
        free(new_socket);
    }
}

int main(int argc, char* argv[]) {
    Server server;
    ServerState state;
    state.current_session = 0;
    pthread_mutex_init(&state.session_mutex, NULL);
    if (setup_server(&server, argv[1], argv[2]) < 0) {
        return 1;
    }
    
    while (true) {
        accept_clients(&server, argv[3], &state);
    }

    close(server.server_fd);

    return 0;
}
