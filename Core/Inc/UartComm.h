#ifndef UART_COMM_H
#define UART_COMM_H

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

// Inizializzazione della comunicazione
void UART_Init(UART_HandleTypeDef *huart);
// Invio notazione e messaggi
void UART_SendMove(uint8_t from_row, uint8_t from_col, uint8_t to_row, uint8_t to_col);
void UART_SendMessage(const char *msg);
void UART_SendResult(int8_t mated_team);

// Ricezione non bloccante (per modalità Studio / PC)
bool UART_ReceiveMove(uint8_t move[4]);

#endif /* UART_COMM_H */