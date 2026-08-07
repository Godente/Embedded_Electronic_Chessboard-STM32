#include "GameMode.h"
#include "Board.h"
#include "Rules.h"
#include "Move.h"
#include "LedMatrix.h"
#include "UartComm.h"
#include "PgnParser.h"
#include "PgnData.h"

typedef enum {
    STATE_SELECT_PIECE,
    STATE_SELECT_DESTINATION,
    STATE_GAME_OVER
} GameState_t;

// === STATI GLOBALI DI GIOCO ===
static GameMode_t current_mode = MODE_HUMAN_VS_HUMAN;
static GameState_t state = STATE_SELECT_PIECE;
static int8_t current_team = -1; // -1 = Bianco, 1 = Nero
static uint8_t from_row = 0, from_col = 0;

static bool move_made = false;            // Blocca cambio modalità se la partita è iniziata
static uint32_t last_button_press = 0;    // Anti-rimbalzo pulsante B1

// === VARIABILI PER MODALITÀ STUDIO ===
static uint8_t study_target_move[4] = {0};
static bool study_move_active = false;

void GameController_Init(GameMode_t mode) {
    current_mode = mode;
    initialize_board();
    current_team = -1;
    state = STATE_SELECT_PIECE;
    move_made = false;

    UART_ResetGame();
    UART_SendMessage("--- CheeseBreadboard Ready ---\r\n");

    // Inizializzazione specifica per la Modalità Studio
    if (current_mode == MODE_STUDY_PGN) {
        PGN_Init(CURRENT_STUDY_PGN);
        study_move_active = PGN_GetNextMove(study_target_move, current_team);
    }

    display_mode_icon(current_mode);
    display_team_pieces(current_team);
}

void GameController_Process(uint8_t curr_phys_board[8], uint8_t prev_phys_board[8]) {
    uint8_t uart_move[4];

    // === 1. GESTIONE PULSANTE B1 (CAMBIO MODALITÀ PRIMA DELLA 1a MOSSA) ===
    if (!move_made) {
        if (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) == GPIO_PIN_RESET) {
            if (HAL_GetTick() - last_button_press > 300) {
                last_button_press = HAL_GetTick();

                // Ruota modalità: 1v1 -> Studio -> PC -> 1v1
                current_mode = (GameMode_t)((current_mode + 1) % 3);

                // Mostra la scritta sulla matrice LED ("1 1", "ST", "PC")
                display_mode_icon(current_mode);

                // Se la modalità diventa Studio, inizializza il parser PGN
                if (current_mode == MODE_STUDY_PGN) {
                    PGN_Init(CURRENT_STUDY_PGN);
                    study_move_active = PGN_GetNextMove(study_target_move, current_team);
                }

                display_team_pieces(current_team);
                state = STATE_SELECT_PIECE;
            }
        }
    }

    // === 2. MODALITÀ GIOCATORE VS GIOCATORE (1v1 Locale) ===
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
                                move_made = true;

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

    // === 3. MODALITÀ STUDIO (Guida interattiva PGN) ===
    else if (current_mode == MODE_STUDY_PGN) {
        static bool castling_rook_phase = false; // true quando l'utente deve muovere la Torre
        static uint8_t rook_move[4] = {0};

        if (study_move_active) {
            
            // Determinazione coordinate correnti (Re o Torre se siamo in fase arrocco)
            uint8_t cur_from_r = castling_rook_phase ? rook_move[0] : study_target_move[0];
            uint8_t cur_from_c = castling_rook_phase ? rook_move[1] : study_target_move[1];
            uint8_t cur_to_r   = castling_rook_phase ? rook_move[2] : study_target_move[2];
            uint8_t cur_to_c   = castling_rook_phase ? rook_move[3] : study_target_move[3];

            // 1. ILLUMINAZIONE LED
            if (state == STATE_SELECT_PIECE) {
                // Illumina il pezzo da sollevare (Re o Torre)
                display_single_square(cur_from_r, cur_from_c);
            } 
            else if (state == STATE_SELECT_DESTINATION) {
                // Illumina la casella di destinazione
                display_single_square(cur_to_r, cur_to_c);
            }

            // 2. SCANSIONE PULSANTI FISICI
            for (uint8_t r = 0; r < 8; r++) {
                uint8_t pressed_bits = (~prev_phys_board[r] & curr_phys_board[r]);
                if (pressed_bits != 0) {
                    for (uint8_t c = 0; c < 8; c++) {
                        if (pressed_bits & (1 << c)) {

                            // FASE 1: L'utente solleva/seleziona il pezzo indicato
                            if (state == STATE_SELECT_PIECE) {
                                if (r == cur_from_r && c == cur_from_c) {
                                    from_row = r;
                                    from_col = c;
                                    state = STATE_SELECT_DESTINATION;
                                }
                            } 
                            // FASE 2: L'utente posiziona il pezzo sulla destinazione
                            else if (state == STATE_SELECT_DESTINATION) {
                                if (r == cur_to_r && c == cur_to_c) {
                                    
                                    // Se stavamo muovendo il RE e la mossa è un ARROCCO
                                    uint8_t piece = board[study_target_move[0] * 8 + study_target_move[1]];
                                    bool is_castling = (!castling_rook_phase && 
                                                        piece_type(piece) == 5 && 
                                                        abs((int)study_target_move[1] - (int)study_target_move[3]) == 2);

                                    if (is_castling) {
                                        // 1. Esegui subito lo spostamento software (che sposta sia Re che Torre nel codice)
                                        move_piece(study_target_move);

                                        // 2. Calcola le coordinate della Torre per la guida fisica
                                        rook_move[0] = study_target_move[0]; // Stessa riga del Re
                                        rook_move[2] = study_target_move[0];

                                        if (study_target_move[3] == 6) { // Arrocco Corto
                                            rook_move[1] = 7; // Torre da h -> f
                                            rook_move[3] = 5;
                                        } else { // Arrocco Lungo
                                            rook_move[1] = 0; // Torre da a -> d
                                            rook_move[3] = 3;
                                        }

                                        // 3. Attiva la fase guidata per la Torre
                                        castling_rook_phase = true;
                                        state = STATE_SELECT_PIECE; // Ora richiede di sollevare la Torre
                                    } 
                                    else {
                                        // Mossa normale oppure completamento della Fase Torre dell'arrocco
                                        if (!castling_rook_phase) {
                                            UART_SendMove(study_target_move[0], study_target_move[1], 
                                                          study_target_move[2], study_target_move[3]);
                                            move_piece(study_target_move);
                                        } else {
                                            // Fase Torre completata!
                                            UART_SendMove(study_target_move[0], study_target_move[1], 
                                                          study_target_move[2], study_target_move[3]);
                                            castling_rook_phase = false;
                                        }

                                        move_made = true;
                                        current_team *= -1;

                                        if (is_checkmate(current_team)) {
                                            UART_SendResult(current_team);
                                            animate_checkmate(r, c, current_team);
                                            state = STATE_GAME_OVER;
                                            study_move_active = false;
                                        } else {
                                            // Passa alla mossa successiva del PGN
                                            study_move_active = PGN_GetNextMove(study_target_move, current_team);
                                            state = STATE_SELECT_PIECE;
                                        }
                                    }
                                } 
                                else if (r == from_row && c == from_col) {
                                    // Annulla la selezione
                                    state = STATE_SELECT_PIECE;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // === 4. MODALITÀ GIOCATORE VS PC ===
    else if (current_mode == MODE_VS_PC) {
        if (current_team == -1) { // Turno Umano
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
        else if (current_team == 1) { // Turno PC (via UART)
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