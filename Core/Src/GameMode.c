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
static int8_t current_team = -1; // -1 = Bianco, 1 = Nero
static uint8_t from_row = 0, from_col = 0;

static bool move_made = false;            // Blocca il cambio modalità una volta iniziata la partita
static uint32_t last_button_press = 0;    // Timer anti-rimbalzo per il pulsante

void GameController_Init(GameMode_t mode) {
    current_mode = mode;
    initialize_board();
    current_team = -1;
    state = STATE_SELECT_PIECE;
    move_made = false;

    UART_ResetGame();
    UART_SendMessage("--- CheeseBreadboard Ready ---\r\n");
    UART_SendMessage("Mode: 1v1\r\n");

    // Mostra prima l'icona "1-1" per la modalità predefinita
    display_mode_icon(current_mode);
    display_team_pieces(current_team);
}

void GameController_Process(uint8_t curr_phys_board[8], uint8_t prev_phys_board[8]) {
    uint8_t uart_move[4];

    // === GESTIONE PRESSIONE PULSANTE B1 (Cambio Modalità Prima della 1a Mossa) ===
    if (!move_made) {
        // Sul microcontrollore Nucleo B1_Pin è Active-LOW (pressione = GPIO_PIN_RESET)
        if (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) == GPIO_PIN_RESET) {
            if (HAL_GetTick() - last_button_press > 300) { // Debounce di 300ms
                last_button_press = HAL_GetTick();

                // Ruota la modalità: 1v1 -> Studio -> PC -> 1v1
                current_mode = (GameMode_t)((current_mode + 1) % 3);

                // Mostra la scritta sulla matrice LED
                display_mode_icon(current_mode);

                // Notifica il cambio via UART
                if (current_mode == MODE_HUMAN_VS_HUMAN) {
                    UART_SendMessage("Mode: 1v1\r\n");
                } else if (current_mode == MODE_STUDY_PGN) {
                    UART_SendMessage("Mode: Studio PGN\r\n");
                } else if (current_mode == MODE_VS_PC) {
                    UART_SendMessage("Mode: vs PC\r\n");
                }

                // Ripristina la visualizzazione dei pezzi
                display_team_pieces(current_team);
                state = STATE_SELECT_PIECE;
            }
        }
    }

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
                                move_made = true; // Prima mossa eseguita: blocca il cambio modalità

                                UART_SendMove(from_row, from_col, r, c);

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

    // === MODALITÀ 2: STUDIO (Ricezione mosse dal PC) ===
    else if (current_mode == MODE_STUDY_PGN) {
        if (UART_ReceiveMove(uart_move)) {
            move_made = true;
            move_piece(uart_move);
            current_team *= -1;
            display_team_pieces(current_team);
            
            if (is_checkmate(current_team)) {
                UART_SendResult(current_team);
                animate_checkmate(uart_move[2], uart_move[3], current_team);
                state = STATE_GAME_OVER;
            }
        }
    }

    // === MODALITÀ 3: GIOCATORE VS PC ===
    else if (current_mode == MODE_VS_PC) {
        if (current_team == -1) { // Turno Umano (Bianco)
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
                                    move_made = true;

                                    UART_SendMove(from_row, from_col, r, c);

                                    uint8_t move[4] = {from_row, from_col, r, c};
                                    move_piece(move);

                                    current_team = 1; // Passa al PC

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
        else if (current_team == 1) { // Turno PC (Nero)
            if (UART_ReceiveMove(uart_move)) {
                move_made = true;
                move_piece(uart_move);
                current_team = -1;
                display_team_pieces(current_team);

                if (is_checkmate(current_team)) {
                    UART_SendResult(current_team);
                    animate_checkmate(uart_move[2], uart_move[3], current_team);
                    state = STATE_GAME_OVER;
                } else {
                    state = STATE_SELECT_PIECE;
                }
            }
        }
    }
}