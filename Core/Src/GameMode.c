#include "GameMode.h"
#include "Board.h"
#include "Rules.h"
#include "Move.h"
#include "LedMatrix.h"
#include "UartComm.h"

typedef enum {
    STATE_SELECT_PIECE,
    STATE_SELECT_DESTINATION,
    STATE_GAME_OVER
} GameState_t;

static GameMode_t current_mode = MODE_HUMAN_VS_HUMAN;
static GameState_t state = STATE_SELECT_PIECE;
static int8_t current_team = -1; // -1 = Bianco (Umano), 1 = Nero (PC / Umano 2)
static uint8_t from_row = 0, from_col = 0;

void GameController_Init(GameMode_t mode) {
    current_mode = mode;
    initialize_board();
    current_team = -1;
    state = STATE_SELECT_PIECE;

    UART_SendMessage("--- Chessboard Ready ---\r\n");
    display_team_pieces(current_team);
}

void GameController_Process(uint8_t curr_phys_board[8], uint8_t prev_phys_board[8]) {
    uint8_t uart_move[4];

    // === MODALITÀ 1: GIOCATORE VS GIOCATORE (Locale) ===
    if (current_mode == MODE_HUMAN_VS_HUMAN) {
        
        if (state == STATE_SELECT_DESTINATION) {
            display_legal_moves_on_LED(from_row, from_col);
        }

        for (uint8_t r = 0; r < 8; r++) {
            uint8_t pressed_bits = (~prev_phys_board[r] & curr_phys_board[r]);
            if (pressed_bits != 0) {
                for (uint8_t c = 0; c < 8; c++) {
                    if (pressed_bits & (1 << c)) {

                        if (state == STATE_SELECT_PIECE) {
                            uint8_t piece = board[r * 8 + c];
                            if (piece != EMPTY && piece_team(piece) == current_team) {
                                from_row = r;
                                from_col = c;
                                legal_moves(from_row, from_col);
                                display_legal_moves_on_LED(from_row, from_col);
                                state = STATE_SELECT_DESTINATION;
                            }
                        } 
                        else if (state == STATE_SELECT_DESTINATION) {
                            if (from_row == r && from_col == c) { // Annulla
                                display_team_pieces(current_team);
                                state = STATE_SELECT_PIECE;
                            } 
                            else if (legal_move[r * 8 + c]) { // Mossa Eseguita
                                // 1. Invia la mossa via UART PRIMA di muovere il pezzo
                                UART_SendMove(from_row, from_col, r, c);

                                // 2. Esegue lo spostamento nel motore
                                uint8_t move[4] = {from_row, from_col, r, c};
                                move_piece(move);

                                current_team *= -1;

                                if (is_checkmate(current_team)) {
                                    UART_SendResult(current_team);
                                    animate_checkmate(r, c, current_team);
                                    state = STATE_GAME_OVER;
                                } else {
                                    display_team_pieces(current_team);
                                    state = STATE_SELECT_PIECE;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // === MODALITÀ 2: STUDIO (Riproduzione mosse inviate dal PC) ===
    else if (current_mode == MODE_STUDY_PGN) {
        if (UART_ReceiveMove(uart_move)) {
            move_piece(uart_move);
            current_team *= -1;
            display_team_pieces(current_team);
            
            if (is_checkmate(current_team)) {
                animate_checkmate(uart_move[2], uart_move[3], current_team);
                state = STATE_GAME_OVER;
            }
        }
    }

    // === MODALITÀ 3: GIOCATORE VS PC ===
    else if (current_mode == MODE_VS_PC) {
        // --- TURNO UMANO (Bianco: -1) ---
        if (current_team == -1) {
            if (state == STATE_SELECT_DESTINATION) {
                display_legal_moves_on_LED(from_row, from_col);
            }

            for (uint8_t r = 0; r < 8; r++) {
                uint8_t pressed_bits = (~prev_phys_board[r] & curr_phys_board[r]);
                if (pressed_bits != 0) {
                    for (uint8_t c = 0; c < 8; c++) {
                        if (pressed_bits & (1 << c)) {

                            if (state == STATE_SELECT_PIECE) {
                                uint8_t piece = board[r * 8 + c];
                                if (piece != EMPTY && piece_team(piece) == current_team) {
                                    from_row = r;
                                    from_col = c;
                                    legal_moves(from_row, from_col);
                                    display_legal_moves_on_LED(from_row, from_col);
                                    state = STATE_SELECT_DESTINATION;
                                }
                            } 
                            else if (state == STATE_SELECT_DESTINATION) {
                                if (from_row == r && from_col == c) {
                                    display_team_pieces(current_team);
                                    state = STATE_SELECT_PIECE;
                                } 
                                else if (legal_move[r * 8 + c]) {
                                    // Invia mossa al PC prima dello spostamento
                                    UART_SendMove(from_row, from_col, r, c);

                                    uint8_t move[4] = {from_row, from_col, r, c};
                                    move_piece(move);

                                    current_team = 1; // Passa il turno al PC (Nero)

                                    if (is_checkmate(current_team)) {
                                        animate_checkmate(r, c, current_team);
                                        state = STATE_GAME_OVER;
                                    } else {
                                        display_team_pieces(current_team);
                                        state = STATE_SELECT_PIECE;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } 
        // --- TURNO PC (Nero: 1) ---
        else if (current_team == 1) {
            if (UART_ReceiveMove(uart_move)) {
                move_piece(uart_move);
                current_team = -1; // Torna il turno all'Umano (Bianco)
                display_team_pieces(current_team);

                if (is_checkmate(current_team)) {
                    animate_checkmate(uart_move[2], uart_move[3], current_team);
                    state = STATE_GAME_OVER;
                } else {
                    state = STATE_SELECT_PIECE;
                }
            }
        }
    }
}