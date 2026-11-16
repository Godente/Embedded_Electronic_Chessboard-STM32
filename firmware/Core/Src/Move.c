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
    uint8_t target_piece = board[to_row * 8 + to_col];
    
    // 1. Verifica ed esecuzione En-Passant
    bool is_en_passant_capture = (piece_type(piece) == 6 && 
        to_col != from_col &&
        to_row == en_passant_row &&
        to_col == en_passant_col &&
        target_piece == EMPTY);
    
    if (is_en_passant_capture) {
        if (piece_team(piece) == -1) { // Bianco mangia verso l'alto
            board[(to_row + 1) * 8 + to_col] = EMPTY;
        } else { // Nero mangia verso il basso
            board[(to_row - 1) * 8 + to_col] = EMPTY;
        }
    }

    // 2. Aggiornamento flag movimenti Re e Torri MOSSE
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

    // 3. Aggiornamento flag se una Torre viene CATTURATA sulla casella di partenza
    if (to_row == 7 && to_col == 0) white_rook_a1_moved = true;
    if (to_row == 7 && to_col == 7) white_rook_h1_moved = true;
    if (to_row == 0 && to_col == 0) black_rook_a8_moved = true;
    if (to_row == 0 && to_col == 7) black_rook_h8_moved = true;

    // 4. Esecuzione Arrocco (spostamento della Torre)
    if (piece_type(piece) == 5 && abs((int)from_col - (int)to_col) == 2) {
        int8_t team = piece_team(piece);
        if (team == -1 && from_row == 7) { // Bianco
            if (to_col == 6) { // Corto
                board[7 * 8 + 5] = board[7 * 8 + 7];
                board[7 * 8 + 7] = EMPTY;
                white_rook_h1_moved = true;
            } else if (to_col == 2) { // Lungo
                board[7 * 8 + 3] = board[7 * 8 + 0];
                board[7 * 8 + 0] = EMPTY;
                white_rook_a1_moved = true;
            }
        } else if (team == 1 && from_row == 0) { // Nero
            if (to_col == 6) { // Corto
                board[0 * 8 + 5] = board[0 * 8 + 7];
                board[0 * 8 + 7] = EMPTY;
                black_rook_h8_moved = true;
            } else if (to_col == 2) { // Lungo
                board[0 * 8 + 3] = board[0 * 8 + 0];
                board[0 * 8 + 0] = EMPTY;
                black_rook_a8_moved = true;
            }
        }
    }

    // 5. Spostamento del pezzo principale
    board[to_row * 8 + to_col] = board[from_row * 8 + from_col];
    board[from_row * 8 + from_col] = EMPTY;

    // 6. Gestione stato En-Passant e Promozione
    if(piece_type(piece) == 6) {
        int8_t direction = (piece_team(piece) == -1) ? -1 : 1;
        if(abs((int)to_row - (int)from_row) == 2){
            en_passant_row = from_row + direction;
            en_passant_col = from_col;
        } else {
            en_passant_row = 8; // Reset
            en_passant_col = 8; 
        }
        promote_piece(to_row, to_col, piece_team(piece));
    } else {
        en_passant_row = 8; 
        en_passant_col = 8; 
    }
}    

// Traduce una mossa scritta come stringa (es. "a2-a4") in un array di coordinate numeriche
void translate_to_move(uint8_t * move, char * mossa){
    move[0] = 8 - (mossa[1] - '0'); // Riga origine
    move[1] = mossa[0] - 'a';       // Colonna origine
    move[2] = 8 - (mossa[4] - '0'); // Riga destinazione
    move[3] = mossa[3] - 'a';       // Colonna destinazione
}

// Verifica la sintassi della mossa (es. "a2-a4")
bool is_valid_syntax(char * move_str) {
    if (strlen(move_str) != 5 || move_str[2] != '-') return false;
    if (move_str[0] < 'a' || move_str[0] > 'h' || move_str[3] < 'a' || move_str[3] > 'h') return false;
    if (move_str[1] < '1' || move_str[1] > '8' || move_str[4] < '1' || move_str[4] > '8') return false;
    return true;
}

// Popola la matrice booleana legal_move per il pezzo selezionato
void legal_moves(uint8_t row, uint8_t col) {
    // Azzeramento UNICO ed essenziale all'inizio della ricerca
    initialize_boolmatrix(legal_move);

    uint8_t piece = board[row * 8 + col];
    int8_t team = piece_team(piece);

    if (piece == EMPTY) {
        return;
    }

    switch (piece_type(piece)) {
        case 1: legal_moves_rook(row, col, team);   break;
        case 2: legal_moves_knight(row, col, team); break;
        case 3: legal_moves_bishop(row, col, team); break;
        case 4: legal_moves_queen(row, col, team);  break;
        case 5: legal_moves_king(row, col, team);   break;
        case 6: legal_moves_pawn(row, col, team);   break;
        default: break;
    }
}

void legal_moves_rook(uint8_t row, uint8_t col, int8_t team){
    uint8_t move[4] = {row, col, 0, 0};

    const int8_t directions[8] = {
        -1,  0, // Su
         1,  0, // Giù
         0, -1, // Sinistra
         0,  1  // Destra
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
                if (board[r * 8 + c] != EMPTY) break; // Ferma il raggio se incontra un pezzo
            } else {
                break;
            }

            r += dr;
            c += dc;
        }
    }
}

void legal_moves_knight(uint8_t row, uint8_t col, int8_t team){
    uint8_t move[4] = {row, col, 0, 0};

    const int8_t knight_direction[16] = {
        -2, -1,  -2, +1,
        +2, -1,  +2, +1, 
        -1, -2,  -1, +2,
        +1, -2,  +1, +2
    };

    for (uint8_t i = 0; i < 16; i += 2) {
        int8_t r = row + knight_direction[i];
        int8_t c = col + knight_direction[i + 1];

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
    uint8_t move[4] = {row, col, 0, 0};

    const int8_t directions[8] = {
        -1, -1, // Su-Sx
         1,  1, // Giù-Dx
        -1,  1, // Su-Dx
         1, -1  // Giù-Sx
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
                if (board[r * 8 + c] != EMPTY) break;
            } else {
                break;
            }

            r += dr;
            c += dc;
        }
    }
}

void legal_moves_queen(uint8_t row, uint8_t col, int8_t team){
    uint8_t move[4] = {row, col, 0, 0};

    const int8_t directions[16] = {
        -1,  0,   1,  0,   0, -1,   0,  1,
        -1, -1,   1,  1,  -1,  1,   1, -1
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
                if (board[r * 8 + c] != EMPTY) break;
            } else {
                break;
            }

            r += dr;
            c += dc;
        }
    }    
}

void legal_moves_king(uint8_t row, uint8_t col, int8_t team){
    uint8_t move[4] = {row, col, 0, 0};

    const int8_t king_direction[16] = {
        -1, -1,  -1,  0,  -1, +1,
         0, -1,           0, +1,
         1, -1,   1,  0,   1, +1
    };

    for (uint8_t i = 0; i < 16; i += 2) {
        int8_t r = row + king_direction[i];
        int8_t c = col + king_direction[i + 1];

        if (r >= 0 && r < 8 && c >= 0 && c < 8) {
            move[2] = r;
            move[3] = c;

            if (is_valid_move(move, team)) {
                legal_move[r * 8 + c] = true;
            }
        }
    }

    // Tenta Arrocco Corto e Lungo
    uint8_t castling_move[4] = {row, col, row, col + 2};
    if (is_castling(castling_move, team)) {
        legal_move[row * 8 + col + 2] = true;
    }
    castling_move[3] = col - 2;
    if (is_castling(castling_move, team)) {
        legal_move[row * 8 + col - 2] = true;
    }
}

void legal_moves_pawn(uint8_t row, uint8_t col, int8_t team){
    uint8_t move[4] = {row, col, 0, 0};

    if (team == -1) { // Bianco (sale verso row 0)
        // Advance 1 step
        if (row > 0) {
            move[2] = row - 1;
            move[3] = col;
            if (is_valid_move(move, team)) {
                legal_move[(row - 1) * 8 + col] = true;

                // Advance 2 steps solo SE il primo passo era libero!
                if (row == 6) {
                    move[2] = row - 2;
                    if (is_valid_move(move, team)) {
                        legal_move[(row - 2) * 8 + col] = true;
                    }
                }
            }
        }
    } else if (team == 1) { // Nero (scende verso row 7)
        // Advance 1 step
        if (row < 7) {
            move[2] = row + 1;
            move[3] = col;
            if (is_valid_move(move, team)) {
                legal_move[(row + 1) * 8 + col] = true;

                // Advance 2 steps solo SE il primo passo era libero!
                if (row == 1) {
                    move[2] = row + 2;
                    if (is_valid_move(move, team)) {
                        legal_move[(row + 2) * 8 + col] = true;
                    }
                }
            }
        }
    }

    // Movimento diagonale (Cattura standard ed En-Passant)
    for (int8_t dc = -1; dc <= 1; dc += 2) {
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