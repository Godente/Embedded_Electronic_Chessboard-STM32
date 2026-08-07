#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>


#include "Rules.h"
#include "Board.h" 
#include "Move.h"
#define INVALID_PIECE 255 

#define DEBUG 0

//restituisce il tipo 
uint8_t piece_type(uint8_t piece) {
    switch (piece){
        case white_rook: return 1;
        case white_knight: return 2;
        case white_bishop: return 3;
        case white_queen: return 4;
        case white_king: return 5;
        case white_pawn: return 6;
        case black_rook: return 1;
        case black_knight: return 2;
        case black_bishop: return 3;
        case black_queen: return 4;
        case black_king: return 5;
        case black_pawn: return 6;
        
        default: return INVALID_PIECE;
    }
}

int8_t piece_team(uint8_t piece) {
    if (piece >= white_rook && piece <= white_pawn) return -1; //bianco
    else if (piece >= black_rook && piece <= black_pawn) return 1; //nero
    return 0; //vuoto o pezzo sconosciuto
}

//Promuove solo a regina
void promote_piece(uint8_t row, uint8_t col, int8_t team) {
    uint8_t *square = &board[row * 8 + col];
    
    if (team == -1 && row == 0 && *square == white_pawn) {
        *square = white_queen;
    } else if (team == 1 && row == 7 && *square == black_pawn) {
        *square = black_queen;
    }
}

bool eatable(uint8_t x, uint8_t y, int8_t team) {
    uint8_t target = board[x * 8 + y];
    return target != EMPTY && piece_team(target) != team;
}

bool is_straight(uint8_t * move) {
    uint8_t from_row = move[0], from_col = move[1];
    uint8_t to_row = move[2], to_col = move[3];
    return (from_row == to_row || from_col == to_col);
}

bool is_straight_free(uint8_t * move) {
    uint8_t from_row = move[0], from_col = move[1];
    uint8_t to_row   = move[2], to_col   = move[3];

    if (from_row == to_row) { // mossa orizzontale
        //uint8_t start = (from_col < to_col) ? from_col + 1 : to_col + 1;
        uint8_t start;
        if (from_col < to_col) {
            start = from_col + 1;
        } else {
            start = to_col + 1;
        }
        //uint8_t end   = (from_col < to_col) ? to_col : from_col;
        uint8_t end;
        if (from_col < to_col) {
            end = to_col;
        } else {
            end = from_col;
        }

        for (uint8_t col = start; col < end; col++) {
            if (board[from_row * 8 + col] != EMPTY) return false;
        }
    } else if (from_col == to_col) { // mossa verticale
        //uint8_t start = (from_row < to_row) ? from_row + 1 : to_row + 1; 
        uint8_t start;
        if (from_row < to_row) {
            start = from_row + 1;
        } else {
            start = to_row + 1;
        }
        //uint8_t end   = (from_row < to_row) ? to_row : from_row;
        uint8_t end;
        if (from_row < to_row) {
            end = to_row;
        } else {
            end = from_row;
        }
        for (uint8_t row = start; row < end; row++) {
            if (board[row * 8 + from_col] != EMPTY) return false;
        }
    }

    return true;
}

bool is_diagonal(uint8_t * move) {
    uint8_t from_row = move[0], from_col = move[1];
    uint8_t to_row = move[2], to_col = move[3];
    return (abs(from_row - to_row) == abs(from_col - to_col));
}

bool is_diagonal_free(uint8_t * move) {
    uint8_t from_row = move[0], from_col = move[1];
    uint8_t to_row   = move[2], to_col   = move[3];

    int8_t row_step;
    if (to_row > from_row) {
        row_step = 1;  // Mi muovo verso il basso (verso le righe maggiori)
    } else {
        row_step = -1; // Mi muovo verso l'alto (verso le righe minori)
    }
    
    int8_t col_step;
    if (to_col > from_col) {
        col_step = 1;  // Mi muovo verso il basso (verso le righe maggiori)
    } else {
        col_step = -1; // Mi muovo verso l'alto (verso le righe minori)
    }

    for (int8_t row = from_row + row_step, col = from_col + col_step;
         row != to_row;
         row += row_step, col += col_step) {
        if (board[row * 8 + col] != EMPTY) return false;
    }

    return true;
}

bool is_check(int8_t team){
    uint8_t king_row, king_col;
    int8_t attaking_team;
    // Interpelliamo le var. del re idoneo
    if(team == -1) { // Bianco
        king_row = white_king_row;
        king_col = white_king_col;
        attaking_team = 1;
    } else if(team == 1) { // Nero
        king_row = black_king_row;
        king_col = black_king_col;
        attaking_team = -1;
    } 
    return is_square_attacked(king_row, king_col, attaking_team);
} 

bool is_square_attacked(uint8_t target_row, uint8_t target_col, int8_t attacking_team) {
    
    if (target_row == 7 && target_col == 5) {
        printf("--- DEBUG: Controllo attacco su f1 (%d, %d) da parte del team %d ---\n", target_row, target_col, attacking_team);
    }
    // Attacchi di pedoni
    int8_t pawn_origin_direction = (attacking_team == -1) ? +1 : -1; // Bianco viene da riga+1, Nero da riga-1
    for (int8_t dc = -1; dc <= 1; dc += 2) {
        int8_t r = target_row + pawn_origin_direction;
        int8_t c = target_col + dc;
        if (r >= 0 && r < 8 && c >= 0 && c < 8) {
            uint8_t piece = board[r * 8 + c];
            if (piece_type(piece) == 6 && piece_team(piece) == attacking_team) {
                printf("DEBUG: Falso attacco da PEDONE in [%d, %d]\n", r, c);
                return true;
            }
        }
    }

    // Attacchi di cavallo
    const int knight_moves[8][2] = {
        {-2, -1}, {-2, +1}, {+2, -1}, {+2, +1},
        {-1, -2}, {-1, +2}, {+1, -2}, {+1, +2}
    };
    for (int i = 0; i < 8; i++) {
        int r = target_row + knight_moves[i][0];
        int c = target_col + knight_moves[i][1];
        if (r >= 0 && r < 8 && c >= 0 && c < 8) {
            uint8_t piece = board[r * 8 + c];
            if (piece_type(piece) == 2 && piece_team(piece) == attacking_team) {
                printf("DEBUG: Falso attacco da CAVALLO in [%d, %d]\n", r, c);
                return true;
            }
        }
    }
    // Attacchi in linea retta (torre, regina)
    const int directions[4][2] = { {-1, 0}, {+1, 0}, {0, -1}, {0, +1} };
    for (int i = 0; i < 4; i++) {
        int r = target_row, c = target_col;
        while (true) {
            r += directions[i][0];
            c += directions[i][1];
            if (r < 0 || r >= 8 || c < 0 || c >= 8) break;
            
            uint8_t piece_on_line = board[r * 8 + c];
            if (piece_on_line == EMPTY) continue;
            
            // Confronta il team del pezzo trovato con il team attaccante passato come parametro
            if (piece_team(piece_on_line) == attacking_team) {
                uint8_t type = piece_type(piece_on_line);
                if (type == 1 || type == 4) {
                    if (target_row == 7 && target_col == 5) printf("DEBUG: Falso attacco da TORRE/REGINA in [%d, %d]\n", r, c);
                    return true;
                } // È una torre o regina nemica
            }
            break; // La linea è bloccata da un altro pezzo (amico o nemico non a scorrimento)
        }
    }
    // Attacchi diagonali (alfiere, regina)
    const int diag_dirs[4][2] = { {-1, -1}, {-1, +1}, {+1, -1}, {+1, +1} };
    for (int i = 0; i < 4; i++) {
        int r = target_row, c = target_col;
        while (true) {
            r += diag_dirs[i][0];
            c += diag_dirs[i][1];
            if (r < 0 || r >= 8 || c < 0 || c >= 8) break;

            uint8_t piece_on_line = board[r * 8 + c];
            if (piece_on_line == EMPTY) continue;

            if (piece_team(piece_on_line) == attacking_team) {
                uint8_t type = piece_type(piece_on_line);
                if (type == 3 || type == 4) {
                    if (target_row == 7 && target_col == 5) printf("DEBUG: Falso attacco da ALFIERE/REGINA in [%d, %d]\n", r, c);
                    return true;
                 } // È un alfiere o regina nemica
            }
            break; // Linea bloccata
        }
    }
    // Attacchi del re 
    for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {
            if (dr == 0 && dc == 0) continue;
            int r = target_row + dr;
            int c = target_col + dc;
            if (r >= 0 && r < 8 && c >= 0 && c < 8) {
                uint8_t piece = board[r * 8 + c];
                if (piece_type(piece) == 5 && piece_team(piece) == attacking_team) {
                    return true;
                }
            }
        }
    }

    return false; // Nessun attacco trovato
}

bool is_castling(uint8_t * move, int8_t team) {
    uint8_t from_row = move[0], from_col = move[1];
    uint8_t to_row   = move[2], to_col   = move[3];

    //Per arroccare bisogna muovere il re di 2 caselle
    if (piece_type(board[from_row * 8 + from_col]) != 5) return false; // Non è un re
    if (abs(from_col - to_col) != 2 || from_row != to_row) return false; // Non è un arrocco

    if (is_check(team)) return false; // Il re non può arroccare se è in scacco

    if (team == -1) { // Bianco
        if (white_king_moved) return false;

        if (to_col == 6) { // corto (g1)
            if (white_rook_h1_moved) return false;
            if (board[7*8 + 5] != EMPTY || board[7*8 + 6] != EMPTY) return false;
            //if (is_square_attacked(7, 4, 1) || is_square_attacked(7, 5, 1) || is_square_attacked(7, 6, 1)) return false;
            bool e1_attacked = is_square_attacked(7, 4, 1);
            printf("DEBUG: Casella e1 attaccata? %s\n", e1_attacked ? "SI" : "NO");

            bool f1_attacked = is_square_attacked(7, 5, 1);
            printf("DEBUG: Casella f1 attaccata? %s\n", f1_attacked ? "SI" : "NO");

            bool g1_attacked = is_square_attacked(7, 6, 1);
            printf("DEBUG: Casella g1 attaccata? %s\n", g1_attacked ? "SI" : "NO");

            if (e1_attacked || f1_attacked || g1_attacked) {
            printf("DEBUG FAIL: Una delle caselle di passaggio è attaccata.\n");
            return false;
            }
        } else if (to_col == 2) { // lungo (c1)
            if (white_rook_a1_moved) return false;
            if (board[7*8 + 1] != EMPTY || board[7*8 + 2] != EMPTY || board[7*8 + 3] != EMPTY) return false;
            if (is_square_attacked(7, 4, 1) || is_square_attacked(7, 3, 1) || is_square_attacked(7, 2, 1)) return false;
        } else return false;
    } else {
        if (black_king_moved) return false;

        if (to_col == 6) { // corto (g8)
            if (black_rook_h8_moved) return false;
            if (board[0*8 + 5] != EMPTY || board[0*8 + 6] != EMPTY) return false;
            if (is_square_attacked(0, 4, -1) || is_square_attacked(0, 5, -1) || is_square_attacked(0, 6, -1)) return false;
        } else if (to_col == 2) { // lungo (c8)
            if (black_rook_a8_moved) return false;
            if (board[0*8 + 1] != EMPTY || board[0*8 + 2] != EMPTY || board[0*8 + 3] != EMPTY) return false;
            if (is_square_attacked(0, 4, -1) || is_square_attacked(0, 3, -1) || is_square_attacked(0, 2, -1)) return false;
        } else return false;
    }
    return true; // Arrocco valido
}

bool is_checkmate(int8_t team) {
    // Controlla se il re è in scacco
    if (!is_check(team)) return false;

    // Prova a muovere ogni pezzo del team per vedere se può uscire dallo scacco
    for (uint8_t row = 0; row < 8; row++) {
        for (uint8_t col = 0; col < 8; col++) {
            uint8_t piece = board[row * 8 + col];
            if(piece_team(piece) != team || piece == EMPTY) continue; // Salta i vuoi e i pezzi dell'altro team
            
            // Genera tutte le mosse legali per questo pezzo
            switch (piece_type(piece)) {
                case 0: legal_moves_pawn(row, col, team);   break;
                case 1: legal_moves_knight(row, col, team); break;
                case 2: legal_moves_bishop(row, col, team); break;
                case 3: legal_moves_rook(row, col, team);   break;
                case 4: legal_moves_queen(row, col, team);  break;
                case 5: legal_moves_king(row, col, team);   break;
                default: continue;
            }
            //Prova ogni mossa legale
            for (uint8_t r = 0; r < 8; r++) {
                for (uint8_t c = 0; c < 8; c++) {
                    if(!legal_move[r * 8 + c]) continue; // Salta le mosse illegali

                    uint8_t captured = board[r * 8 + c]; // Salva il pezzo catturato
                    uint8_t move[4] = {row, col, r, c}; // Mossa da provare
                    uint8_t temp_piece = board[row * 8 + col]; // Salva il pezzo da muovere
                    // Esegui la mossa
                    move_piece(move);
                    // Controlla se il re è ancora in scacco dopo la mossa
                    bool still_in_check = is_check(team);
                    // Ripristina lo stato del gioco
                    board[row * 8 + col] = temp_piece; // Ripristina il pezzo
                    board[r * 8 + c] = captured; // Ripristina il pezzo catturato
                    // Ripristina le coordinate del re
                    if(piece_type(temp_piece) == 5) { 
                        if(team == -1) { 
                            white_king_row = row;
                            white_king_col = col;
                        } else if(team == 1) { 
                            black_king_row = row;
                            black_king_col = col;
                        }
                    }
                    if(!still_in_check) return false;
                }
            }
        }
    }
    return true; //è scacco matto
}

bool is_piece_pin(uint8_t *move, int8_t team) {
    uint8_t from_row = move[0], from_col = move[1];
    uint8_t to_row   = move[2], to_col   = move[3];

    uint8_t piece = board[from_row * 8 + from_col];
    uint8_t captured = board[to_row * 8 + to_col];

    // Applica temporaneamente la mossa
    board[to_row * 8 + to_col] = piece;
    board[from_row * 8 + from_col] = EMPTY;

    // Se è il re, aggiorna temporaneamente la sua posizione
    uint8_t saved_king_row, saved_king_col;
    if (piece_type(piece) == 5) {
        if (team == -1) {
            saved_king_row = white_king_row;
            saved_king_col = white_king_col;
            white_king_row = to_row;
            white_king_col = to_col;
        } else {
            saved_king_row = black_king_row;
            saved_king_col = black_king_col;
            black_king_row = to_row;
            black_king_col = to_col;
        }
    }

    // Verifica se il re è sotto scacco dopo la mossa
    bool in_check = is_check(team);

    // Ripristina la situazione
    board[from_row * 8 + from_col] = piece;
    board[to_row * 8 + to_col] = captured;

    if (piece_type(piece) == 5) {
        if (team == -1) {
            white_king_row = saved_king_row;
            white_king_col = saved_king_col;
        } else {
            black_king_row = saved_king_row;
            black_king_col = saved_king_col;
        }
    }

    return in_check; // Se è in check dopo la mossa → era inchiodato
}

bool is_valid_move(uint8_t * move, int8_t team) {
    uint8_t from_row = move[0], from_col = move[1];
    uint8_t to_row = move[2], to_col = move[3];

    #if (DEBUG)
    printf("DEBUG: Pezzo in partenza: %d, team atteso: %d\n", piece_type(board[from_row*8 + from_col]), piece_team(board[from_row*8 + from_col]));
    #endif

    // Controlla se la mossa è valida
    if (from_row >= 8 || from_col >= 8 || to_row >= 8 || to_col >= 8) return false;
    // Controlla se il pezzo da muovere esiste e appartiene al team corretto
    uint8_t piece = board[from_row * 8 + from_col];
    if (piece == EMPTY || piece_team(piece) != team) return false;
    // Controlla se la destinazione è occupata da un pezzo dello stesso team
    if (board[to_row * 8 + to_col] != EMPTY && !eatable(to_row, to_col, team)) return false;
  
    switch(piece_type(piece)) {
        case 1: // Torre
            if (!is_straight(move) || !is_straight_free(move)) return false;
            break;
        case 2: // Cavallo
        if (!((abs(from_row - to_row) == 2 && abs(from_col - to_col) == 1) || (abs(from_row - to_row) == 1 && abs(from_col - to_col) == 2))) return false;
        break;
        case 3: // Alfiere
            if (!is_diagonal(move) || !is_diagonal_free(move)) return false;
            break;
        case 4: // Regina
            if (is_straight(move)) {
            if (!is_straight_free(move)) return false;
            } else if (is_diagonal(move)) {
            if (!is_diagonal_free(move)) return false;
            } else return false;
            break;
        case 5: // Re
            if (distance(from_row, to_row) <= 1 && distance(from_col, to_col) <= 1){ break;
            } else if (abs(from_col - to_col) == 2 && from_row == to_row) {
                // Arrocco
                if (is_castling(move, team)) return true;
            } else {
                return false;} // Mossa non valida per il re
            break;
        case 6: // Pedone
        if (from_col == to_col) {
         // Movimento in avanti
            if (team == -1) { // Bianco
                if (from_row == 6) { // primo spostamento pedone
                    if (!((to_row == 5 && board[5 * 8 + from_col] == EMPTY) ||
                      (to_row == 4 && board[5 * 8 + from_col] == EMPTY 
                                   && board[4 * 8 + from_col] == EMPTY)))
                    return false;
                } else {
                if (to_row != from_row - 1 || board[to_row * 8 + from_col] != EMPTY) return false;
                }
            } else if (team == 1) { // Nero
                if (from_row == 1) { // primo spostamento pedone
                    if (!((to_row == 2 && board[2 * 8 + from_col] == EMPTY) ||
                          (to_row == 3 && board[2 * 8 + from_col] == EMPTY 
                                       && board[3 * 8 + from_col] == EMPTY)))
                    return false;
                } else {
                if (to_row != from_row + 1 || board[to_row * 8 + from_col] != EMPTY) return false;
                }
            }
        } else {
        // Movimento diagonale (mangia)
        if (abs(from_col - to_col) != 1) return false;
        if ((team == -1 && to_row != from_row - 1) || (team == 1 && to_row != from_row + 1)) return false;
        // Controlla se il pedone può mangiare in en passant
        if (board[to_row * 8 + to_col] == EMPTY && to_row == en_passant_row && to_col == en_passant_col) return true;
        if (!eatable(to_row, to_col, team)) return false;
        }
        break;
        default:
            return false; // Pezzo sconosciuto
    }
    
    // Controlla se il pezzo è inchiodato: non può muoversi se causa scacco
    if (is_piece_pin(move, team)) return false;

    return true;
}