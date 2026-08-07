#include "UartComm.h"
#include "Board.h"
#include "Rules.h"
#include "Move.h"
#include <stdio.h>
#include <string.h>

static UART_HandleTypeDef *p_huart = NULL;
static char rx_buffer[10];
static uint8_t rx_index = 0;
static uint8_t rx_byte = 0;
static bool move_ready = false;
static uint8_t parsed_move[4] = {0};

void UART_Init(UART_HandleTypeDef *huart) {
    p_huart = huart;
    HAL_UART_Receive_IT(p_huart, &rx_byte, 1);
}

void UART_SendMessage(const char *msg) {
    if (p_huart == NULL) return;
    HAL_UART_Transmit(p_huart, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
}

//Invia la mossa in Notazione Algebrica Standard (SAN - es. e4, Nf3, dxe5, O-O, Rd8#)
void UART_SendMove(uint8_t from_row, uint8_t from_col, uint8_t to_row, uint8_t to_col) {
    if (p_huart == NULL) return;

    // Legge lo stato della scacchiera PRIMA dello spostamento
    uint8_t piece = board[from_row * 8 + from_col];
    uint8_t target = board[to_row * 8 + to_col];

    uint8_t type = piece_type(piece);

    char f_col = 'a' + from_col;
    char t_col = 'a' + to_col;
    char t_row = '8' - to_row;

    char san[16] = {0};
    uint8_t idx = 0;

    // 1. GESTIONE ARROCCO
    if (type == 5 && abs((int)from_col - (int)to_col) == 2) {
        if (to_col == 6) {
            strcpy(san, "O-O");     // Arrocco Corto
        } else if (to_col == 2) {
            strcpy(san, "O-O-O");   // Arrocco Lungo
        }
        idx = strlen(san);
    } 
    else {
        // 2. LETTERA DEL PEZZO (I pedoni NON hanno lettera iniziale)
        switch (type) {
            case 1: san[idx++] = 'R'; break; // Torre (Rook)
            case 2: san[idx++] = 'N'; break; // Cavallo (Knight)
            case 3: san[idx++] = 'B'; break; // Alfiere (Bishop)
            case 4: san[idx++] = 'Q'; break; // Regina (Queen)
            case 5: san[idx++] = 'K'; break; // Re (King)
            case 6: break;                   // Pedone: nessuna lettera
            default: break;
        }

        // 3. CATTURA (Solo se la casella di arrivo NON è vuota o è en-passant)
        bool is_en_passant = (type == 6 && to_row == en_passant_row && to_col == en_passant_col);
        bool is_capture = (target != EMPTY && target != 0) || is_en_passant;

        if (is_capture) {
            if (type == 6) { 
                san[idx++] = f_col; // Per i pedoni che mangiano si indica la colonna d'origine (es. dxe5)
            }
            san[idx++] = 'x';
        }

        // 4. CASELLA DI DESTINAZIONE
        san[idx++] = t_col;
        san[idx++] = t_row;
    }

    // A capo per la seriale
    san[idx++] = '\r';
    san[idx++] = '\n';
    san[idx] = '\0';

    HAL_UART_Transmit(p_huart, (uint8_t*)san, strlen(san), HAL_MAX_DELAY);
}

void UART_SendResult(int8_t mated_team) {
    if (p_huart == NULL) return;

    if (mated_team == 1) { 
        // Il Nero (1) è sotto matto -> Vince il Bianco
        HAL_UART_Transmit(p_huart, (uint8_t*)"1-0\r\n", 5, HAL_MAX_DELAY);
    } else if (mated_team == -1) { 
        // Il Bianco (-1) è sotto matto -> Vince il Nero
        HAL_UART_Transmit(p_huart, (uint8_t*)"0-1\r\n", 5, HAL_MAX_DELAY);
    }
}

// Controlla se è arrivata una mossa via UART dal PC
bool UART_ReceiveMove(uint8_t move[4]) {
    if (move_ready) {
        memcpy(move, parsed_move, 4);
        move_ready = false;
        return true;
    }
    return false;
}

// Callback interruzione ricezione seriale
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
        if (rx_byte == '\n' || rx_byte == '\r') {
            if (rx_index == 5 && rx_buffer[2] == '-') {
                parsed_move[0] = '8' - rx_buffer[1];
                parsed_move[1] = rx_buffer[0] - 'a';
                parsed_move[2] = '8' - rx_buffer[4];
                parsed_move[3] = rx_buffer[3] - 'a';
                move_ready = true;
            }
            rx_index = 0;
        } else if (rx_index < sizeof(rx_buffer) - 1) {
            rx_buffer[rx_index++] = rx_byte;
        }
        HAL_UART_Receive_IT(p_huart, &rx_byte, 1);
    }
}