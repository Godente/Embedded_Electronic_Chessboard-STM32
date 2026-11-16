#ifndef MOVE_H
#define MOVE_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

uint8_t distance(uint8_t origin, uint8_t dest);

void move_piece(uint8_t *move);
void translate_to_move(uint8_t *move, char *mossa);
bool is_valid_syntax(char *move_str);
void legal_moves(uint8_t row, uint8_t col);
void legal_moves_rook(uint8_t row, uint8_t col, int8_t team);
void legal_moves_knight(uint8_t row, uint8_t col, int8_t team);
void legal_moves_bishop(uint8_t row, uint8_t col, int8_t team);
void legal_moves_queen(uint8_t row, uint8_t col, int8_t team);
void legal_moves_king(uint8_t row, uint8_t col, int8_t team);
void legal_moves_pawn(uint8_t row, uint8_t col, int8_t team);

#endif