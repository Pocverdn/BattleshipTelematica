#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
//Librerias sockets para linux
#include <arpa/inet.h>
#include <unistd.h> 
#include <pthread.h>

// Puerto
//#define PORT 8080

//14 bytes para envio de posiciones de barcos - 1 byte para comfirmación de disparo
#define BUFFER_SIZE 2500
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

typedef struct {
    bool ship;
    bool attacked;
} map;

typedef struct {
    map p1[10][10];
    map p2[10][10];
} session;

typedef struct ship {
    unsigned char posX;  // 4 bits
    unsigned char posY;  // 4 bits
    unsigned char size;  // 3 bits
    bool dir;            // 0 = horizontal, 1 = vertical
} ship;

struct attack {
    unsigned char posX;  // 4 bits
    unsigned char posY;  // 4 bits
};

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
    //printf("%x", *arr);
    //printf("El pos X del cuarto barco: ");
    //printf("%x", decode[3].posX);
    //printf("\n");
    return decode;
}




/*

SAMUEL AQUI ESTA LA ANTERIOR FORMA DE MANEJAR LOS CLIENTES JUNTO A SUS PRUEBAS, ADAPTELO PARA LA NUEVA VERSIÓN


extern inline void gameStart(struct ship ships[], bool player, session* ses) {
    if (player) { //player es si es jugador uno o no
        for (char i = 0; i < 9; i++) {
            for (char j = 0; j < ships[i].size; j++) {
                ses->p1[ships[i].posX+(j* ships[i].dir)][ships[i].posY + (j * ships[i].dir)].ship=1;
            }
        }
    }
    else {
        for (char i = 0; i < 9; i++) {
            for (char j = 0; j < ships[i].size; j++) {
                ses->p2[ships[i].posX + (j * ships[i].dir)][ships[i].posY + (j * ships[i].dir)].ship = 1;
            }
        }
    }
}

void *handle_client(void *client_socket) {
    int new_socket = *(int *)client_socket;
    free(client_socket);

    char buffer[BUFFER_SIZE] = {0};
    char username[50] = {0};

    const char *hello = "Message received";

    int bytes_received = recv(new_socket, username, sizeof(username) - 1, 0);

    printf("Usuario conectado: %s\n", username);

    //Inicio del juego, fuera del buffer
    //Necesito que cuando hagan la conexion me pogan un bool con el numero del jugador y que ambos threads de un juego compartan un puntero a la misma session en el servidor 
    //Mientraas para pruebas
    bool TESTplayer = 0;
    session TESTs = {0, 0};

    //los valores de prueba terminan aca

    memset(buffer, 0, BUFFER_SIZE);
    int bytes_received = recv(new_socket, buffer, BUFFER_SIZE, 0);

    if (bytes_received <= 0) {
        printf("Cliente desconectado\n");
        
    }
    else {
        struct ship *ships = decode(buffer);
        gameStart(ships, TESTplayer,&TESTs);
    }

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        bytes_received = recv(new_socket, buffer, BUFFER_SIZE - 1, 0);

        if (bytes_received <= 0) {
            printf("Cliente desconectado\n");
            break;
        }

        printf("%s\n", buffer);

        send(new_socket, hello, strlen(hello), 0);
        //printf("Mensaje enviado al cliente\n");
    }

    close(new_socket); 

    return NULL;
}

*/

void initializeBoard(char board[SIZE][SIZE]) {
    for(int i = 0; i < SIZE; i++){
        for(int j = 0; j < SIZE; j++){
            board[i][j] = '~'; // Agua no revelada
        }
    }
}

bool placeShipSize(char board[SIZE][SIZE], ship s) {
    for(int i = 0; i < s.size; ++i) {
        int x = s.posX + (s.dir ? i : 0);
        int y = s.posY + (s.dir ? 0 : i);

        if (x >= SIZE || y >= SIZE || board[x][y] != '~') {
            return false; // fuera del tablero o espacio ocupado
        }
    }

    for(int i = 0; i < s.size; ++i) {
        int x = s.posX + (s.dir ? i : 0);
        int y = s.posY + (s.dir ? 0 : i);
        board[x][y] = 'B';
    }

    return true;
}

void setShips(char board[SIZE][SIZE], ship ships[TOTAL_SHIPS], int playerNumber) {
    printf("\nJugador %d, coloca tus %d barcos.\n", playerNumber, TOTAL_SHIPS);
    for (int i = 0; i < TOTAL_SHIPS; ++i) {
        ship s;
        bool valid = false;

        do {
            int x, y, size, dir;
            printf("Barco #%d - Ingresa X Y Tamano Direccion (H=0/V=1): ", i + 1);
            scanf("%d %d %d %d", &x, &y, &size, &dir);

            s.posX = x;
            s.posY = y;
            s.size = size;
            s.dir = dir;

            bool cabe = (s.dir == 0 && ((s.posX + s.size) <= SIZE)) ||
                        (s.dir == 1 && ((s.posY + s.size) <= SIZE));

            if (cabe) {
                if (placeShipSize(board, s)) {
                    ships[i] = s;
                    valid = true;
                } else {
                    printf("Esa posición ya está ocupada. Intenta de nuevo.\n");
                }
            } else {
                printf("Posición inválida. Intenta de nuevo.\n");
            }

        } while (!valid);
    }
}

void showBoard(char board[SIZE][SIZE]) {
    printf("  ");
    for (int j = 0; j < SIZE; ++j) printf("%d ", j);
    printf("\n");

    for (int i = 0; i < SIZE; ++i) {
        printf("%d ", i);
        for (int j = 0; j < SIZE; ++j) {
            if (board[i][j] == 'X' || board[i][j] == 'O')
                printf("%c ", board[i][j]);
            else
                printf("~ ");
        }
        printf("\n");
    }
}

bool shoot(char board[SIZE][SIZE], int x, int y) {
    if (x < 0 || x >= SIZE || y < 0 || y >= SIZE) {
        printf("Coordenadas inválidas.\n");
        return false;
    }

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

void *handle_games(void *client_socket){

    int new_socket = *(int *)client_socket;
    free(client_socket);

    char buffer[BUFFER_SIZE] = {0};
    char username[50] = {0};

    const char *hello = "Message received";

    int bytes_received = recv(new_socket, username, sizeof(username) - 1, 0);
    if (bytes_received > 0) {
        username[bytes_received] = '\0';
        printf("Usuario conectado: %s\n", username);
    } else {
        printf("No se recibió nombre de usuario\n");
        close(new_socket);
        return NULL;
    }

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        bytes_received = recv(new_socket, buffer, BUFFER_SIZE - 1, 0);

        if (bytes_received <= 0) {
            printf("Cliente desconectado\n");
            break;
        }

        printf("Datos recibidos: %s\n", buffer);

        int count = 0;
        struct ship* ships = deserializeShips(buffer, &count);

        printf("Barcos recibidos (%d):\n", count);
        for (int i = 0; i < count; ++i) {
            printf("Barco #%d -> X: %d, Y: %d, Tamaño: %d, Dirección: %s\n",
                   i + 1,
                   ships[i].posX,
                   ships[i].posY,
                   ships[i].size,
                   ships[i].dir ? "Vertical" : "Horizontal");
        }

        send(new_socket, hello, strlen(hello), 0);
    }

    close(new_socket); 

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
