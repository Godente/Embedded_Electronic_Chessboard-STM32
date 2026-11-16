#include "UartComm.h"
#include "Board.h"
#include "Rules.h"
#include "Move.h"
#include <stdio.h>
#include <string.h>

#define RX_BUFFER_SIZE 32

static UART_HandleTypeDef *p_huart = NULL;
static uint16_t move_number = 1;

// Variabili globali per la gestione della ricezione ad interruzione (Slide 10)
static uint8_t rxData = 0;                          // Appoggio per il singolo byte
static char rxBuffer[RX_BUFFER_SIZE];              // Buffer di accumulo stringa
static volatile uint8_t rxIndex = 0;                // Indice del buffer
static volatile uint8_t rxFlag = 0;                 // Flag di notifica stringa completata

void UART_Init(UART_HandleTypeDef *huart) {
    p_huart = huart;
    move_number = 1;
    rxIndex = 0;
    rxFlag = 0;

    // Avvio della prima ricezione in interrupt mode (Slide 11)
    if (p_huart != NULL) {
        HAL_UART_Receive_IT(p_huart, &rxData, 1);
    }
}

void UART_ResetGame(void) {
    move_number = 1;
}

void UART_SendMessage(const char *msg) {
    if (p_huart == NULL) return;
    HAL_UART_Transmit(p_huart, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
}

void UART_SendMove(uint8_t from_row, uint8_t from_col, uint8_t to_row, uint8_t to_col) {
    if (p_huart == NULL) return;

    uint8_t piece = board[from_row * 8 + from_col];
    uint8_t target = board[to_row * 8 + to_col];
    uint8_t type = piece_type(piece);
    int8_t team = piece_team(piece);

    char f_col = 'a' + from_col;
    char t_col = 'a' + to_col;
    char t_row = '8' - to_row;

    char san_move[16] = {0};
    uint8_t idx = 0;

    if (type == 5 && abs((int)from_col - (int)to_col) == 2) {
        if (to_col == 6) strcpy(san_move, "O-O");
        else if (to_col == 2) strcpy(san_move, "O-O-O");
        idx = strlen(san_move);
    } 
    else {
        switch (type) {
            case 1: san_move[idx++] = 'R'; break;
            case 2: san_move[idx++] = 'N'; break;
            case 3: san_move[idx++] = 'B'; break;
            case 4: san_move[idx++] = 'Q'; break;
            case 5: san_move[idx++] = 'K'; break;
            case 6: break;
            default: break;
        }

        bool is_en_passant = (type == 6 && to_row == en_passant_row && to_col == en_passant_col);
        bool is_capture = (target != EMPTY && target != 0) || is_en_passant;

        if (is_capture) {
            if (type == 6) san_move[idx++] = f_col;
            san_move[idx++] = 'x';
        }

        san_move[idx++] = t_col;
        san_move[idx++] = t_row;
    }

    board[to_row * 8 + to_col] = piece;
    board[from_row * 8 + from_col] = EMPTY;

    uint8_t old_k_row = (team == -1) ? white_king_row : black_king_row;
    uint8_t old_k_col = (team == -1) ? white_king_col : black_king_col;
    if (type == 5) {
        if (team == -1) { white_king_row = to_row; white_king_col = to_col; }
        else { black_king_row = to_row; black_king_col = to_col; }
    }

    int8_t enemy_team = -team;
    if (is_checkmate(enemy_team)) {
        san_move[idx++] = '#';
    } else if (is_check(enemy_team)) {
        san_move[idx++] = '+';
    }

    board[from_row * 8 + from_col] = piece;
    board[to_row * 8 + to_col] = target;
    if (type == 5) {
        if (team == -1) { white_king_row = old_k_row; white_king_col = old_k_col; }
        else { black_king_row = old_k_row; black_king_col = old_k_col; }
    }
    san_move[idx] = '\0';

    char final_out[32] = {0};
    if (team == -1) { 
        snprintf(final_out, sizeof(final_out), "%d. %s ", move_number, san_move);
    } else { 
        snprintf(final_out, sizeof(final_out), "%s\r\n", san_move);
        move_number++;
    }

    HAL_UART_Transmit(p_huart, (uint8_t*)final_out, strlen(final_out), HAL_MAX_DELAY);
}

void UART_SendResult(int8_t mated_team) {
    if (p_huart == NULL) return;
    if (mated_team == 1) { 
        HAL_UART_Transmit(p_huart, (uint8_t*)"1-0\r\n", 5, HAL_MAX_DELAY);
    } else if (mated_team == -1) { 
        HAL_UART_Transmit(p_huart, (uint8_t*)"\r\n0-1\r\n", 7, HAL_MAX_DELAY);
    }
}

/**
 * @brief Polling del flag rxFlag (Slide 12)
 */
bool UART_ReceiveMove(uint8_t move[4]) {
    if (rxFlag) {
        // Parsing delle coordinate (es. "e7-e5" oppure "e7e5")
        char c1 = rxBuffer[0];
        char r1 = rxBuffer[1];
        char c2 = (rxBuffer[2] == '-') ? rxBuffer[3] : rxBuffer[2];
        char r2 = (rxBuffer[2] == '-') ? rxBuffer[4] : rxBuffer[3];

        bool valid = false;

        if (c1 >= 'a' && c1 <= 'h' && r1 >= '1' && r1 <= '8' &&
            c2 >= 'a' && c2 <= 'h' && r2 >= '1' && r2 <= '8') {

            move[0] = '8' - r1;
            move[1] = c1 - 'a';
            move[2] = '8' - r2;
            move[3] = c2 - 'a';

            UART_SendMessage("-> Mossa PC Ricevuta!\r\n");
            valid = true;
        }

        // Reset del buffer e del flag (Slide 12)
        memset(rxBuffer, 0, RX_BUFFER_SIZE);
        rxFlag = 0;

        return valid;
    }
    return false;
}

/**
 * @brief Callback ISR eseguita al termine di ogni byte ricevuto (Slide 13)
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (p_huart != NULL && huart->Instance == p_huart->Instance) {
        
        // Accumulo byte nel buffer se c'è spazio (Slide 13)
        if (rxIndex < RX_BUFFER_SIZE - 1) {
            if (rxData != ' ' && rxData != '\r') { // Ignora spazi e carriage return
                rxBuffer[rxIndex++] = rxData;
            }
        }

        // Verifica chiusura della stringa o raggiungimento limite (Slide 13)
        if (rxData == '\n' || rxIndex >= RX_BUFFER_SIZE - 1) {
            rxBuffer[rxIndex] = '\0';
            rxFlag = 1;  // Notifica la fine del pacchetto al main (Slide 13)
            rxIndex = 0; // Reset indice (Slide 13)
        }

        // FONDAMENTALE: Riavvio dell'ascolto per il byte successivo (Slide 13)
        HAL_UART_Receive_IT(huart, &rxData, 1);
    }
}