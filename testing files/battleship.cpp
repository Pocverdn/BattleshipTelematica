#include <iostream>
#include <stdio.h>
using namespace std;

const int SIZE = 10;
const int TOTAL_SHIPS = 3;

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
        bool cabe = false;
        do {
            int x, y, size;
            int dir;
            cout << "Barco #" << i+1 << " - Ingresa X Y Tamano Direccion(H=0/V=1): ";
            cin >> x >> y >> size >> dir;

            s.posX = x;
            s.posY = y;
            s.size = size;
            s.dir = dir;

            cabe = (s.dir == 0 && ((s.posX + s.size) <= SIZE)) || (s.dir == 1 && ((s.posY + s.size) <= SIZE));

            cout << s.posX << endl;
            cout << s.size << endl;
            cout << s.posX + s.size << endl;
            cout << SIZE<< endl;

            if (cabe) {
                if (placeShipSize(board, s)) {
                    ships[i] = s;
                    break;
                }
            }else {
                cout << "Posicion inválida. Intenta de nuevo.\n";
            }   

        } while(!cabe);
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


int main() {
    char board1[SIZE][SIZE]
    ship ships1[TOTAL_SHIPS]
    int hits1 = 0,

    initializeBoard(board1);

    setShips(board1, ships1, 1);

    int totalHitsNeeded = countShips(ships1);

    cout << "\n---- ¡Comienza el juego! ----\n";
    bool turn = true;

    while (hits1 < totalHitsNeeded && hits2 < totalHitsNeeded) {
        int x, y;
        if (turn) {
            cout << "\nTurno Jugador 1\n";
            showBoard(board2);
            cout << "Ingresa coordenadas de disparo (x y): ";
            cin >> x >> y;
            if (shoot(board2, x, y)) hits1++;
        } else {
            cout << "\nTurno Jugador 2\n";
            showBoard(board1);
            cout << "Ingresa coordenadas de disparo (x y): ";
            cin >> x >> y;
            if (shoot(board1, x, y)) hits2++;
        }
        turn = !turn;
    }


    cout << "\n==== ¡Fin del juego! ====\n";
    if (hits1 >= totalHitsNeeded)
        cout << "¡Jugador 1 gana!\n";
    
    if (hits2 >= totalHitsNeeded)
        cout << "¡Jugador 2 gana!\n";

    return 0;

}