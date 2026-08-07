#ifndef RULES_H
#define RULES_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

uint8_t piece_type(uint8_t piece);
int8_t piece_team(uint8_t piece);
bool is_valid_move(uint8_t *move, int8_t team);
void promote_piece(uint8_t row, uint8_t col, int8_t team);
bool eatable(uint8_t x, uint8_t y, int8_t team);
bool is_straight(uint8_t * move);
bool is_straight_free(uint8_t * move);
bool is_diagonal(uint8_t * move);
bool is_diagonal_free(uint8_t * move);
bool is_check(int8_t team);
bool is_checkmate(int8_t team);
bool is_castling(uint8_t * move, int8_t team);
bool is_valid_move(uint8_t * move, int8_t team);
bool is_piece_pin(uint8_t *move, int8_t team);
bool is_square_attacked(uint8_t target_row, uint8_t target_col, int8_t attacking_team);

#endif