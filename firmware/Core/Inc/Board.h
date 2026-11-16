#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>
#include <stdbool.h>

#define EMPTY         0
#define white_rook    1
#define white_knight  2
#define white_bishop  3
#define white_queen   4
#define white_king    5
#define white_pawn    6
#define black_rook    7
#define black_knight  8
#define black_bishop  9
#define black_queen   10
#define black_king    11
#define black_pawn    12

extern char board[64];
extern bool legal_move[64];
extern uint8_t white_king_row, white_king_col;
extern uint8_t black_king_row, black_king_col;

extern bool white_king_moved;
extern bool black_king_moved;
extern bool white_rook_a1_moved;
extern bool white_rook_h1_moved;
extern bool black_rook_a8_moved;
extern bool black_rook_h8_moved;
extern uint8_t en_passant_row;
extern uint8_t en_passant_col;

void initialize_board(void);
void initialize_boolmatrix(bool *matrix);
void print_board(void);
void print_legal_moves(void);
void print_board_ascii(void);
void print_board_pretty();
#endif