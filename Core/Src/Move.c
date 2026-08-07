#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "Move.h"
#include "Board.h"
#include "Rules.h"

#define DEBUG 0

uint8_t distance(uint8_t origin, uint8_t dest) {
    return abs(origin - dest);
}

void move_piece(uint8_t * move){
    uint8_t from_row = move[0], from_col = move[1];
    uint8_t to_row = move[2], to_col = move[3];
    uint8_t piece = board[from_row * 8 + from_col];
    
    // Verifica en-passant
    bool is_en_passant_capture = (piece_type(piece) == 6 && 
        to_col != from_col &&
        to_row == en_passant_row &&
        to_col == en_passant_col &&
        board[to_row * 8 + to_col] == EMPTY);
    
    if (is_en_passant_capture) {
        if (piece_team(piece) == -1) { // Bianco mangia verso l’alto
            board[(to_row + 1) * 8 + to_col] = EMPTY; // Rimuove il pedone avversario
        } else { // Nero mangia verso il basso
            board[(to_row - 1) * 8 + to_col] = EMPTY; // Rimuove il pedone avversario
        }
    }

    if(piece == white_king) {
        white_king_moved = true;
        white_king_row = to_row;
        white_king_col = to_col;
    } else if(piece == black_king) {
        black_king_moved = true;
        black_king_row = to_row;
        black_king_col = to_col;
    } else if(piece == white_rook && from_row == 7 && from_col == 0) {
        white_rook_a1_moved = true;
    } else if(piece == white_rook && from_row == 7 && from_col == 7) {
        white_rook_h1_moved = true;
    } else if(piece == black_rook && from_row == 0 && from_col == 0) {
        black_rook_a8_moved = true;
    } else if(piece == black_rook && from_row == 0 && from_col == 7) {
        black_rook_h8_moved = true;
    }

    // Esegui anche l'arrocco se il re si muove di 2 colonne
    if (piece_type(piece) == 5 && abs(from_col - to_col) == 2) {
    int8_t team = piece_team(piece);
    // Bianco
        if (team == -1 && from_row == 7) {
            if (to_col == 6) { // corto
            board[7 * 8 + 5] = board[7 * 8 + 7]; // muove torre h1 → f1
            board[7 * 8 + 7] = EMPTY;
            white_rook_h1_moved = true;
            } else if (to_col == 2) { // lungo
            board[7 * 8 + 3] = board[7 * 8 + 0]; // torre a1 → d1
            board[7 * 8 + 0] = EMPTY;
            white_rook_a1_moved = true;
            }
        } else if (team == 1 && from_row == 0) {
        if (to_col == 6) { // corto
            board[0 * 8 + 5] = board[0 * 8 + 7]; // torre h8 → f8
            board[0 * 8 + 7] = EMPTY;
            black_rook_h8_moved = true;
            } else if (to_col == 2) { // lungo
            board[0 * 8 + 3] = board[0 * 8 + 0]; // torre a8 → d8
            board[0 * 8 + 0] = EMPTY;
            black_rook_a8_moved = true;
            }
        }
    }

    board[to_row * 8 + to_col] = board[from_row * 8 + from_col]; //spostare da origine a destinazione
    board[from_row * 8 + from_col] = EMPTY; //svuotare la casella precedente

    //en-passant e promozione
    if(piece_type(piece) == 6) {
        int8_t direction = (piece_team(piece) == -1) ? -1 : 1;
        if(abs(to_row - from_row) == 2){
            en_passant_row = from_row + direction;
            en_passant_col = from_col;
        } else {
            en_passant_row = 8; // reset en_passant
            en_passant_col = 8; 
        }
        promote_piece(to_row, to_col, piece_team(piece));
    } else {
        en_passant_row = 8; 
        en_passant_col = 8; 
    }
    printf("DEBUG: en_passant_row=%d col=%d\n", en_passant_row, en_passant_col);
    printf("DEBUG: from [%d,%d] to [%d,%d], tipo=%d, team=%d\n", from_row, from_col, to_row, to_col, piece_type(piece), piece_team(piece));
}    
//traduce una mossa scritta come stringa (es. "a2-a4") in un array di coordinate numeriche
void translate_to_move(uint8_t * move, char * mossa){
    move[0] = 8 - (mossa[1] - '0'); //riga origine
    move[1] = mossa[0] - 'a'; //colonna origine
    move[2] = 8 - (mossa[4] - '0');
    move[3] = mossa[3] - 'a'; //colonna destinazione
    #if DEBUG
    printf("DEBUG: mossa: %s\n", mossa);
    printf("DEBUG:[%d, %d] to [%d, %d]\n", move[0], move[1], move[2], move[3]);
    #endif
}
//verifica se la sintassi della mossa è corretta
bool is_valid_syntax(char * move_str) {
    //lunghezza di 5 e trattino al centro (es. "a2-a4")
    if (strlen(move_str) != 5 || move_str[2] != '-') return false;

    // Controlla che le colonne siano tra 'a' e 'h'
    if (move_str[0] < 'a' || move_str[0] > 'h' || move_str[3] < 'a' || move_str[3] > 'h') return false;
    
    // Controlla che le righe siano tra '1' e '8'
    if (move_str[1] < '1' || move_str[1] > '8' || move_str[4] < '1' || move_str[4] > '8') return false;
    
    return true;
}
//mostra le mossse possibili del pezzo sulla matrice boolean
void legal_moves(uint8_t row, uint8_t col) {
    uint8_t piece = board[row * 8 + col];
    int8_t team = piece_team(piece);

    if( piece == EMPTY) {
        //printf("Non c'è nessun pezzo in [%d, %d]\n", row, col);
        return;
    }

    switch (piece_type(piece)) {
        case 1: // Torre
            legal_moves_rook(row, col, team);
            break;
        case 2: // Cavallo
            legal_moves_knight(row, col, team);
            break;
        case 3: // Alfiere
            legal_moves_bishop(row, col, team);
            break;
        case 4: // Regina
            legal_moves_queen(row, col, team);
            break;
        case 5: // Re
            legal_moves_king(row, col, team);
            break;
        case 6: // Pedone
            legal_moves_pawn(row, col, team);
            break;
        default:
            printf("Mossa non valida per il pezzo %c\n", piece);
    }
}

void legal_moves_rook(uint8_t row, uint8_t col, int8_t team){
    uint8_t move[4];
    move[0] = row; move[1] = col;

    initialize_boolmatrix(legal_move);

    // direzioni rettilinee: su, giù, sinistra, destra
    const int8_t directions[8] = {
        -1,  0, // su
         1,  0, // giù
         0, -1, // sinistra
         0,  1  // destra
    };

    for (int8_t d = 0; d < 8; d += 2) {
        int8_t dr = directions[d];
        int8_t dc = directions[d + 1];
        int8_t r = row + dr;
        int8_t c = col + dc;

        while (r >= 0 && r < 8 && c >= 0 && c < 8) {
            move[2] = r;
            move[3] = c;

            if (is_valid_move(move, team)) {
                legal_move[r * 8 + c] = true;

                // se c'è un pezzo nemico, fermati dopo averlo "mangiato"
                if (board[r * 8 + c] != EMPTY) break;
            } else {
                break; // non posso andare oltre un ostacolo
            }

            r += dr;
            c += dc;
        }
    }
}

void legal_moves_knight(uint8_t row, uint8_t col, int8_t team){
    uint8_t move[4];
    move[0] = row; move[1] = col;

    initialize_boolmatrix(legal_move);

    //le 8 possibili mosse del cavallo
    const int8_t knight_direction[16] = {
        - 2, - 1,
        - 2, + 1,
        + 2, - 1,
        + 2, + 1, 
        - 1, - 2,
        - 1, + 2,
        + 1, - 2,
        + 1, + 2,
    };

    for (uint8_t i = 0; i < 16; i += 2) {
        int8_t r = move[0] + knight_direction[i];
        int8_t c = move[1] + knight_direction[i + 1];

        if (r >= 0 && r < 8 && c >= 0 && c < 8) {
            move[2] = r;
            move[3] = c;

            if (is_valid_move(move, team)) {
                legal_move[r * 8 + c] = true;
            }
        }
    }
}

void legal_moves_bishop(uint8_t row, uint8_t col, int8_t team){
    uint8_t move[4];
    move[0] = row; move[1] = col;
    
    initialize_boolmatrix(legal_move);

    // direzioni rettilinee: su, giù, sinistra, destra
    const int8_t directions[8] = {
        -1,  -1, // up_sx
         1,   1, // giù_dx
        -1,   1, // up_dx
         1,  -1  // giù_sx
    };

    for (int8_t d = 0; d < 8; d += 2) {
        int8_t dr = directions[d];
        int8_t dc = directions[d + 1];
        int8_t r = row + dr;
        int8_t c = col + dc;

        while (r >= 0 && r < 8 && c >= 0 && c < 8) {
            move[2] = r;
            move[3] = c;

            if (is_valid_move(move, team)) {
                legal_move[r * 8 + c] = true;

                // se c'è un pezzo nemico, fermati dopo averlo "mangiato"
                if (board[r * 8 + c] != EMPTY) break;
            } else {
                break; // non posso andare oltre un ostacolo
            }

            r += dr;
            c += dc;
        }
    }
}

void legal_moves_queen(uint8_t row, uint8_t col, int8_t team){
    uint8_t move[4];
    move[0] = row; move[1] = col;

    initialize_boolmatrix(legal_move);

    // direzioni rettilinee: su, giù, sinistra, destra
    const int8_t directions[16] = {
        -1,  0, // su
         1,  0, // giù
         0, -1, // sinistra
         0,  1, // destra
        -1, -1, // up_sx
         1,  1, // giù_dx
        -1,  1, // up_dx
         1, -1  // giù_sx
    };

    for (int8_t d = 0; d < 16; d += 2) {
        int8_t dr = directions[d];
        int8_t dc = directions[d + 1];
        int8_t r = row + dr;
        int8_t c = col + dc;

        while (r >= 0 && r < 8 && c >= 0 && c < 8) {
            move[2] = r;
            move[3] = c;

            if (is_valid_move(move, team)) {
                legal_move[r * 8 + c] = true;

                // se c'è un pezzo nemico, fermati dopo averlo "mangiato"
                if (board[r * 8 + c] != EMPTY) break;
            } else {
                break; // non posso andare oltre un ostacolo
            }

            r += dr;
            c += dc;
        }
    }    
}

void legal_moves_king(uint8_t row, uint8_t col, int8_t team){
    uint8_t move[4];
    move[0] = row; move[1] = col;

    initialize_boolmatrix(legal_move);

    // le 8 possibili mosse del re
    const int8_t king_direction[16] = {
        -1, -1, // up_sx
        -1,  0, // up
        -1, +1, // up_dx
         0, -1, // left
         0, +1, // right
         1, -1, // down_sx
         1,  0, // down
         1, +1  // down_dx
    };

    for (uint8_t i = 0; i < 16; i += 2) {
        int8_t r = move[0] + king_direction[i];
        int8_t c = move[1] + king_direction[i + 1];

        if (r >= 0 && r < 8 && c >= 0 && c < 8) {
            move[2] = r;
            move[3] = c;

            if (is_valid_move(move, team)) {
                legal_move[r * 8 + c] = true;
            }
        }
    }
    //Tenta arrocco
    uint8_t castling_move[4] = {row, col, row, col + 2}; //Arrocco corto
    if (is_castling(castling_move, team)) {
        legal_move[row * 8 + col + 2] = true; // Arrocco corto
    }
    castling_move[3] = col - 2; // Arrocco lungo
    if (is_castling(castling_move, team)) {
        legal_move[row * 8 + col - 2] = true; // Arrocco lungo
    }
}

void legal_moves_pawn(uint8_t row, uint8_t col, int8_t team){
    uint8_t move[4];
    move[0] = row; move[1] = col;

    initialize_boolmatrix(legal_move);

    // Movimento in avanti
    if (team == -1) { // Bianco
        if (row > 0) {
            move[2] = row - 1;
            move[3] = col;
            if (is_valid_move(move, team)) {
                legal_move[(row - 1) * 8 + col] = true;
            }
        }
        // Primo spostamento pedone
        if (row == 6) {
            move[2] = row - 2;
            if (is_valid_move(move, team)) {
                legal_move[(row - 2) * 8 + col] = true;
            }
        }
    } else if (team == 1) { // Nero
        if (row < 7) {
            move[2] = row + 1;
            move[3] = col;
            if (is_valid_move(move, team)) {
                legal_move[(row + 1) * 8 + col] = true;
            }
        }
        // Primo spostamento pedone
        if (row == 1) {
            move[2] = row + 2;
            if (is_valid_move(move, team)) {
                legal_move[(row + 2) * 8 + col] = true;
            }
        }
    }

    // Movimento diagonale (mangia)
    for (int8_t dc = -1; dc <= 1; dc += 2) { // solo sinistra e destra
        int8_t c = col + dc;
        if (c >= 0 && c < 8) {
            move[2] = row + ((team == -1) ? -1 : +1);
            move[3] = c;
            if (is_valid_move(move, team)) {
                legal_move[(move[2]) * 8 + c] = true;
            }
        }
    }
}