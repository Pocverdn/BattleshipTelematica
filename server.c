#include <fcntl.h>     
#include <sys/file.h>  

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <netinet/in.h>
//Librerias sockets para linux
#include <arpa/inet.h>
#include <unistd.h> 
#include <pthread.h>
#include <time.h>
#include <signal.h>


#include "protocolo.h"

// Logica del juego
#define SIZE 10
#define TOTAL_SHIPS 9

// Logica del juego

void initializeBoard(char board[SIZE][SIZE]) { //Llena el tablero con agua.
    for(int i = 0; i < SIZE; i++){
        for(int j = 0; j < SIZE; j++){
            board[i][j] = '~'; // Agua no revelada
        }
    }
}

bool placeShipSize(char board[SIZE][SIZE], ship s) { //Pone un barco
    for(int i = 0; i < s.size; ++i) {
        int x = s.posX + (s.dir ? i : 0);
        int y = s.posY + (s.dir ? 0 : i);
        board[x][y] = 'B';
    }

    return true;
}

void setShips(char board[SIZE][SIZE], ship ships[TOTAL_SHIPS], char* username) { //Manda a poner los barcos
    printf("\nJugador %s, coloca tus %d barcos.\n", username, TOTAL_SHIPS);
    for (int i = 0; i < TOTAL_SHIPS; ++i) {
        placeShipSize(board, ships[i]);
    }
}

void showBoard(char board[SIZE][SIZE], ship ships[TOTAL_SHIPS]) { //Impreme el tablero basado en la información de los barcos.
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
                printf("🚢 ");
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

int countShips(ship ships[TOTAL_SHIPS]) { //Cuanta la cantidad total de partes de barcos
    int total = 0;
    for (int i = 0; i < TOTAL_SHIPS; ++i)
        total += ships[i].size;
    return total;
}

void initialize_session(GameSession *session, int socket, const char *username, ship *ships, const char *ip) { //Recibe la información de los barcos de cada jugador
    session->player1_fd = socket; 
    strncpy(session->player1_ip, ip, sizeof(session->player1_ip));
    session->player1_ip[sizeof(session->player1_ip) - 1] = '\0';
    strncpy(session->player1_name, username, sizeof(session->player1_name));
    memcpy(session->ships1, ships, sizeof(ship) * TOTAL_SHIPS);

    for (int i = 0; i < TOTAL_SHIPS; ++i) {
        printf("Barco #%d -> X: %d, Y: %d, Tamaño: %d, Dirección: %s\n",
               i + 1, ships[i].posX, ships[i].posY, ships[i].size,
               ships[i].dir ? "Vertical" : "Horizontal");
    }
}

void handle_turn(GameSession *session, char board[SIZE][SIZE], struct ship ships[TOTAL_SHIPS], int attacker_fd, int defender_fd, int *hits, char *username, bool *giveUp, char* path, char* attacker_ip,char* defender_ip)
 {
    
    unsigned char at; //Maneja la lógica de juego, que pasa despues de los ataques, mandar el timeout, etc.
    unsigned char response[2];
    bool sunk = false;

    fd_set readfds;
    struct timeval timeout;

    timeout.tv_sec = 31;
    timeout.tv_usec = 0;

    FD_ZERO(&readfds);
    FD_SET(attacker_fd, &readfds);

    int activity = select(attacker_fd + 1, &readfds, NULL, NULL, &timeout);
    //printf("%x", activity);

    if (activity == 0) {
        // Tiempo agotado
        printf("El jugador %s no realizó su movimiento a tiempo. Turno perdido.\n", username);
        char timeout_msg[128];
        snprintf(timeout_msg, sizeof(timeout_msg), "T | Tiempo agotado. El jugador %s no realizó su movimiento a tiempo.", username);
        safe_log(timeout_msg, path, attacker_ip);
        return;
    } else if (activity < 0) {
        perror("Error en select");
        return;
    }

    int bytes = recv(attacker_fd, &at, 1, 0);
    if (bytes <= 0) {
        perror("Error recibiendo ataque");
        send(defender_fd, "C", 1, 0);
        *giveUp = true;
        return;
    }

    /*Banderas:
    T = timeout.
    t = turno.
    S = surrender.
    C = Desconexión.
    D = le diste.
    d = te dieron.
    G = ganaste.
    P = perdiste.
    H = hundiste.
    A = agua.
    W = wait
    */


    response[1] = at;
    attack att = decodeAttack(at);

    char attack_msg[128];
    snprintf(attack_msg, sizeof(attack_msg), "t | Jugador %s ataca: x = %d, y = %d", username, att.posX, att.posY);
    printf("%s\n", attack_msg);
    safe_log(attack_msg, path, attacker_ip);

    if (att.posX == 10 && att.posY == 10) {
        char surrender_msg[128];
        snprintf(surrender_msg, sizeof(surrender_msg), "S | Jugador %s se ha rendido.", username);
        printf("%s\n", surrender_msg);
        safe_log(surrender_msg, path, defender_ip);

        *giveUp = true;
        response[0] = 'G';
        send(defender_fd, response, 2, 0);
        return;
    }

    if (shoot(attacker_fd, board, ships, &sunk, att.posX, att.posY)) {
        (*hits)++;

        if (sunk) {
            printf("Hundido\n");
            safe_log("H | Barco hundido", path, attacker_ip);
            response[0] = 'H';
            send(attacker_fd, response, 2, 0);
            response[0] = 'd';
            send(defender_fd, response, 2, 0);
        } else {
            safe_log("D | Acierto", path, attacker_ip);
            response[0] = 'D';
            send(attacker_fd, response, 2, 0);
            response[0] = 'd';
            send(defender_fd, response, 2, 0);
        }
    } else {
        safe_log("A | Agua", path, attacker_ip);
        response[0] = 'A';
        send(attacker_fd, response, 2, 0);
    }
}

void play_game(GameSession *session, char *path) {
    // current_session = (current_session + 1) % MAX_SESSIONS;

    char board1[SIZE][SIZE], board2[SIZE][SIZE]; //Establece los tableros de cada jugador y pone sus barcos.
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

    while (hits1 < totalHits && hits2 < totalHits) { //Manaja a quien le toca atacar y manda los mensajes de los resultaods
        printf("\nNuevo turno\n");

        if (turn) {
            send_turn_messages(session->player1_fd, session->player2_fd);
            handle_turn(session, board2, session->ships2, session->player1_fd, session->player2_fd, &hits1, session->player1_name, &giveUp1, path,session->player1_ip,session->player2_ip);

            if (giveUp1) {
                break;
            }

        } else {
            send_turn_messages(session->player2_fd, session->player1_fd);
            handle_turn(session, board1, session->ships1, session->player2_fd, session->player1_fd, &hits2, session->player2_name, &giveUp2, path,session->player1_ip,session->player2_ip);

            if (giveUp2) {
                break; 
            }
        }

        turn = !turn;
    }

    if (hits1 >= totalHits) { //Manda los mensajes al ganador y el perdedor cuando se cierra el loop de arriba
        send(session->player1_fd, "G", 1, 0);
        send(session->player2_fd, "P", 1, 0);
    
        printf("🎉 %s ganó la partida contra %s\n", session->player1_name, session->player2_name);
        safe_log("G | Has ganado la partida", path, session->player1_ip);
        safe_log("P | Has perdido la partida", path, session->player2_ip);
    } else {
        send(session->player2_fd, "G", 1, 0);
        send(session->player1_fd, "P", 1, 0);
    
        printf("🎉 %s ganó la partida contra %s\n", session->player2_name, session->player1_name);
        safe_log("G | Has ganado la partida", path, session->player2_ip);
        safe_log("P | Has perdido la partida", path, session->player1_ip);
    }
    
}

void *handle_games(void *arg) { //Empieza el juego y maneja las seciones
    ThreadArgs *args = (ThreadArgs *)arg;
    int new_socket = args->client_socket;
    char *path = args->path;
    ServerState *state = args->state;
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    char client_ip[INET_ADDRSTRLEN];

    if (getpeername(new_socket, (struct sockaddr*)&addr, &len) == 0) {
        inet_ntop(AF_INET, &addr.sin_addr, client_ip, sizeof(client_ip));
        printf("Client IP: %s\n", client_ip);
    }

    ship player_ships[TOTAL_SHIPS];
    char username[50], email[50];

    receive_player_info(new_socket, username, email, path,client_ip);

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
            strncpy(session->player2_ip, client_ip, sizeof(session->player2_ip));
            session->player2_ip[sizeof(session->player2_ip) - 1] = '\0';
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

        initialize_session(session, new_socket, username, player_ships,client_ip);
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

extern inline void accept_clients(Server* server, char* path, ServerState* state) { //maneja que se conecte el cliente. Antes estaba en protocolo.h
    int addrlen = sizeof(server->address);
    pthread_t thread_id;

    while (1) {
        int* new_socket = malloc(sizeof(int));
        if (!new_socket) {
            perror("Memory allocation failed");
            continue;
        }

        *new_socket = accept(server->server_fd, (sockaddr*)&server->address, (socklen_t*)&addrlen);
        if (*new_socket < 0) {
            perror("Accept failed");
            free(new_socket);
            continue;
        }

        ThreadArgs* args = malloc(sizeof(ThreadArgs));
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

        if (pthread_create(&thread_id, NULL, handle_games, (void*)args) != 0) {
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

void stablish_connection(char* argv[]) {
    Server server;  //Creamos las structuras para manejar las conecxiones
    ServerState state;
    state.current_session = 0;
    pthread_mutex_init(&state.session_mutex, NULL); //Se crea el mutex para manejar los threads
    setup_server(&server, argv[1], argv[2]);

    accept_clients(&server, argv[3], &state);


    close(server.server_fd); //Esta funcion solo se debe cerrar cuando se le diga al servidor que se apage
    return;
}

int main(int argc, char* argv[]) {
    signal(SIGPIPE, SIG_IGN); //Ponemos esta bandera para que no se cierre el servidor cuando se cierra un socket de manera invalida.
    stablish_connection(argv);

    return 0;
}