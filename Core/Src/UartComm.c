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

static uint16_t move_number = 1;

void UART_Init(UART_HandleTypeDef *huart) {
    p_huart = huart;
    move_number = 1;
    HAL_UART_Receive_IT(p_huart, &rx_byte, 1);
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

    // 1. ARROCCO
    if (type == 5 && abs((int)from_col - (int)to_col) == 2) {
        if (to_col == 6) strcpy(san_move, "O-O");
        else if (to_col == 2) strcpy(san_move, "O-O-O");
        idx = strlen(san_move);
    } 
    else {
        // 2. LETTERA PEZZO
        switch (type) {
            case 1: san_move[idx++] = 'R'; break;
            case 2: san_move[idx++] = 'N'; break;
            case 3: san_move[idx++] = 'B'; break;
            case 4: san_move[idx++] = 'Q'; break;
            case 5: san_move[idx++] = 'K'; break;
            case 6: break;
            default: break;
        }

        // 3. CATTURA
        bool is_en_passant = (type == 6 && to_row == en_passant_row && to_col == en_passant_col);
        bool is_capture = (target != EMPTY && target != 0) || is_en_passant;

        if (is_capture) {
            if (type == 6) san_move[idx++] = f_col;
            san_move[idx++] = 'x';
        }

        // 4. DESTINAZIONE
        san_move[idx++] = t_col;
        san_move[idx++] = t_row;
    }

    // 5. SIMULAZIONE TEMPORANEA PER SCACCO (+) / SCACCO MATTO (#)
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

    // Ripristino scacchiera
    board[from_row * 8 + from_col] = piece;
    board[to_row * 8 + to_col] = target;
    if (type == 5) {
        if (team == -1) { white_king_row = old_k_row; white_king_col = old_k_col; }
        else { black_king_row = old_k_row; black_king_col = old_k_col; }
    }
    san_move[idx] = '\0';

    // 6. FORMATTAZIONE PGN E INVIO
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

bool UART_ReceiveMove(uint8_t move[4]) {
    if (move_ready) {
        memcpy(move, parsed_move, 4);
        move_ready = false;
        return true;
    }
    return false;
}

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