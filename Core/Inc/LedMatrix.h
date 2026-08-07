#ifndef LED_MATRIX_H
#define LED_MATRIX_H

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

// Impostazioni di orientamento hardware
#define INVERT_ROWS    1   // 1 = Bianchi in basso, Neri in alto
#define INVERT_COLS    0   // 0 = Orientamento colonne standard

// Inizializzazione e controllo base
void MAX7219_init(void);
void MAX7219_write(uint8_t address, uint8_t data);
void MAX7219_setall(uint8_t state);
uint8_t map_logical_to_LED(uint8_t logical_byte);

// Funzioni di visualizzazione scacchiera
void display_team_pieces(int8_t team);
void display_legal_moves_on_LED(uint8_t f_r, uint8_t f_c);

// Mostra le scritte sulla matrice: "1-1", "ST", "PC"
void display_mode_icon(uint8_t mode);

// Animazioni
void animate_checkmate(uint8_t to_r, uint8_t to_c, int8_t mated_team);

#endif /* LED_MATRIX_H */