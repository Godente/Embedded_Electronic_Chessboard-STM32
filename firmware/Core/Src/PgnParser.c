#include "PgnParser.h"
#include "Board.h"
#include "Rules.h"
#include "Move.h"
#include <string.h>
#include <ctype.h>

static const char *p_pgn = NULL;
static uint32_t pgn_index = 0;

void PGN_Init(const char *pgn_text) {
    p_pgn = pgn_text;
    pgn_index = 0;
}

/**
 * @brief Estrae il prossimo token mossa pulito dal PGN saltando intestazioni, commenti e numeri
 */
static bool PGN_NextToken(char *token, uint8_t max_len) {
    if (p_pgn == NULL) return false;

    uint8_t t_idx = 0;

    while (p_pgn[pgn_index] != '\0') {
        char ch = p_pgn[pgn_index];

        // 1. Salta i metadati tra quadre [...]
        if (ch == '[') {
            while (p_pgn[pgn_index] != ']' && p_pgn[pgn_index] != '\0') pgn_index++;
            if (p_pgn[pgn_index] == ']') pgn_index++;
            continue;
        }

        // 2. Salta i commenti tra graffe {...}
        if (ch == '{') {
            while (p_pgn[pgn_index] != '}' && p_pgn[pgn_index] != '\0') pgn_index++;
            if (p_pgn[pgn_index] == '}') pgn_index++;
            continue;
        }

        // 3. Salta i commenti tra tonde (...) o NAG ($1, $3, etc.)
        if (ch == '(' || ch == '$') {
            while (!isspace((unsigned char)p_pgn[pgn_index]) && p_pgn[pgn_index] != '\0') pgn_index++;
            continue;
        }

        // 4. Salta spazi e a capo
        if (isspace((unsigned char)ch)) {
            pgn_index++;
            continue;
        }

        // 5. Salta i numeri di mossa (es. "1.", "16...", "2.")
        if (isdigit((unsigned char)ch)) {
            uint32_t temp_idx = pgn_index;
            while (isdigit((unsigned char)p_pgn[temp_idx])) temp_idx++;
            if (p_pgn[temp_idx] == '.') { // È un numero di mossa
                while (p_pgn[temp_idx] == '.') temp_idx++; // Salta tutti i punti
                pgn_index = temp_idx;
                continue;
            }
        }

        // 6. Legge il token della mossa valido
        while (p_pgn[pgn_index] != '\0' && !isspace((unsigned char)p_pgn[pgn_index]) &&
               p_pgn[pgn_index] != '{' && p_pgn[pgn_index] != '[' && p_pgn[pgn_index] != ')') {
            
            if (t_idx < max_len - 1) {
                token[t_idx++] = p_pgn[pgn_index];
            }
            pgn_index++;
        }

        token[t_idx] = '\0';

        // Ignora i risultati finali della partita
        if (strcmp(token, "1-0") == 0 || strcmp(token, "0-1") == 0 || strcmp(token, "1/2-1/2") == 0 || strcmp(token, "*") == 0) {
            return false;
        }

        if (t_idx > 0) return true; // Token valido trovato
    }

    return false;
}

/**
 * @brief Converte un token SAN (es. "Nf3", "Bxf3", "e4", "O-O") in coordinate {from_r, from_c, to_r, to_c}
 */
static bool SAN_To_Move(const char *san, int8_t team, uint8_t move[4]) {
    // Gestione Arrocco
    if (strcmp(san, "O-O") == 0 || strcmp(san, "O-O+") == 0 || strcmp(san, "O-O#") == 0) {
        move[0] = (team == -1) ? 7 : 0; move[1] = 4;
        move[2] = (team == -1) ? 7 : 0; move[3] = 6;
        return true;
    }
    if (strcmp(san, "O-O-O") == 0 || strcmp(san, "O-O-O+") == 0 || strcmp(san, "O-O-O#") == 0) {
        move[0] = (team == -1) ? 7 : 0; move[1] = 4;
        move[2] = (team == -1) ? 7 : 0; move[3] = 2;
        return true;
    }

    char clean_san[10];
    uint8_t c_len = 0;

    // Rimuove caratteri speciali come +, #, !, ?
    for (uint8_t i = 0; san[i] != '\0'; i++) {
        if (san[i] != '+' && san[i] != '#' && san[i] != '!' && san[i] != '?') {
            clean_san[c_len++] = san[i];
        }
    }
    clean_san[c_len] = '\0';

    if (c_len < 2) return false;

    // Identifica la casella di destinazione (ultimi due caratteri)
    uint8_t to_col = clean_san[c_len - 2] - 'a';
    uint8_t to_row = '8' - clean_san[c_len - 1];

    // Identifica il tipo di pezzo
    uint8_t target_piece_type = 6; // Pedone di default
    uint8_t start_idx = 0;

    if (isupper((unsigned char)clean_san[0])) {
        switch (clean_san[0]) {
            case 'R': target_piece_type = 1; break; // Torre
            case 'N': target_piece_type = 2; break; // Cavallo
            case 'B': target_piece_type = 3; break; // Alfiere
            case 'Q': target_piece_type = 4; break; // Regina
            case 'K': target_piece_type = 5; break; // Re
            default: break;
        }
        start_idx = 1;
    }

    // Gestione ambiguità (es. Nbd7 o R1d4)
    int8_t req_from_col = -1;
    int8_t req_from_row = -1;

    if (c_len - start_idx > 2) {
        char spec = clean_san[start_idx];
        if (spec >= 'a' && spec <= 'h' && clean_san[start_idx + 1] != 'x') {
            req_from_col = spec - 'a';
        } else if (spec >= '1' && spec <= '8') {
            req_from_row = '8' - spec;
        }
    }

    // Scansiona la scacchiera per trovare il pezzo legale corrispondente
    for (uint8_t r = 0; r < 8; r++) {
        for (uint8_t c = 0; c < 8; c++) {
            uint8_t p = board[r * 8 + c];

            if (p != EMPTY && piece_team(p) == team && piece_type(p) == target_piece_type) {
                if (req_from_col != -1 && c != req_from_col) continue;
                if (req_from_row != -1 && r != req_from_row) continue;

                // Calcola le mosse legali per questo specifico pezzo
                legal_moves(r, c);

                if (legal_move[to_row * 8 + to_col]) {
                    move[0] = r;
                    move[1] = c;
                    move[2] = to_row;
                    move[3] = to_col;
                    return true;
                }
            }
        }
    }

    return false;
}

bool PGN_GetNextMove(uint8_t move[4], int8_t current_team) {
    char token[16];

    while (PGN_NextToken(token, sizeof(token))) {
        if (SAN_To_Move(token, current_team, move)) {
            return true;
        }
    }

    return false; // Fine del PGN
}