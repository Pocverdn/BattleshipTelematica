#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>

using namespace std;

#define BUFFER_SIZE 2500

const int SIZE = 10;
const int TOTAL_SHIPS = 9;

struct Config {
    char server_ip[256];
    int PORTLINUX;
};

struct ship {
    unsigned char posX;  // 4 bits
    unsigned char posY;  // 4 bits
    unsigned char size;  // 3 bits
    bool dir;            // 0 = horizontal, 1 = vertical
};

struct attack {
    unsigned char posX;  // 4 bits
    unsigned char posY;  // 4 bits
};

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
    std::cout << "El primer byte de la cadena codificada: " << std::hex << (int)encoded[0] << std::endl;
    return encoded;
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

void setShips(char board[SIZE][SIZE], ship ships[TOTAL_SHIPS], int playerNumber) {
    cout << "\nJugador " << playerNumber << ", coloca tus " << TOTAL_SHIPS << " barcos.\n";
    for (int i = 0; i < TOTAL_SHIPS; ++i) {
        ship s;
        bool put = false;
        do {
            int x, y, size;
            int dir;
            cout << "Barco #" << i+1 << " - Ingresa X Y Tamano Direccion(H=0/V=1): ";
            cin >> x >> y >> size >> dir;

            s.posX = x;
            s.posY = y;
            s.size = size;
            s.dir = dir;

            bool cabe = (s.dir == 0 && ((s.posX + s.size) <= SIZE)) || (s.dir == 1 && ((s.posY + s.size) <= SIZE));


            if (placeShipSize(board, s) && cabe) {
                ships[i] = s;
                put = true;
            
            }else {
                cout << "Posicion inválida. Intenta de nuevo.\n";
            }   

        } while(!put);
    }
}

void showBoard(char board[SIZE][SIZE]){
    cout << "  ";
    for (int j = 0; j < SIZE; ++j) cout << j << " ";
    cout << endl;
    for (int i = 0; i < SIZE; ++i) {
        cout << i << " ";
        for (int j = 0; j < SIZE; ++j) {
            if (board[i][j] == 'X' || board[i][j] == 'O')
                cout << board[i][j] << " ";
            else
                cout << "~ ";
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


void trim(std::string& str) {
    size_t first = str.find_first_not_of(' ');
    size_t last = str.find_last_not_of(" \n\r");
    str = str.substr(first, (last - first + 1));
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
            } else if (key == "port") {
                *port = std::atoi(value.c_str());
            }
        }
    }
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

std::string serializeShips(ship ships[], int total) {
    std::string result;

    for (int i = 0; i < total; ++i) {
        result += std::to_string(ships[i].posX) + " " +
                  std::to_string(ships[i].posY) + " " +
                  std::to_string(ships[i].size) + " " +
                  std::to_string(ships[i].dir ? 1 : 0);

        if (i < total - 1)
            result += ";";
    }

    return result;
}

void chat_with_server(int client_fd) {
    char buffer[BUFFER_SIZE] = {0};
    std::string username;

    std::cout << "Enter your username: ";
    std::getline(std::cin >> std::ws, username);

    send(client_fd, username.c_str(), username.length(), 0);

    char board1[SIZE][SIZE];
    ship ships1[TOTAL_SHIPS];

    initializeBoard(board1);
    setShips(board1, ships1, 1);

    unsigned char* serialized = encode(ships1);

    for (int i = 0; i < 9; ++i) {
        printf("Barco #%d -> X: %d, Y: %d, Tamaño: %d, Dirección: %s\n",
               i + 1,
               ships1[i].posX,
               ships1[i].posY,
               ships1[i].size,
               ships1[i].dir ? "Vertical" : "Horizontal");
    }


    send(client_fd, serialized, 14, 0);
    
}

int main(int argc, char* argv[]) {

    // Conexiones de cliente a soket

    Config config;
    parse_config("adress.config", config.server_ip, &config.PORTLINUX);

    std::cout << "Server IP: " << config.server_ip << std::endl;
    std::cout << "Port: " << config.PORTLINUX << std::endl;

    int client_fd = connect_to_server(config);
    if (client_fd < 0) return -1;

    chat_with_server(client_fd);
    close(client_fd);


    // Juego de battleShip


    return 0;
}