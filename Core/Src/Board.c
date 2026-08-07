#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <locale.h>

#include "Board.h"

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

//RICORDA che queste sotto sono var. globali
char board[64];

bool legal_move[64]; //utile per il controllo led

uint8_t white_king_row, white_king_col;
uint8_t black_king_row, black_king_col;
//Var. per en.passant
uint8_t en_passant_row = 8; // Valore fuori dalla scacchiera
uint8_t en_passant_col = 8;
// Var. per verificare arrocco
bool white_king_moved = false;
bool black_king_moved = false;
bool white_rook_a1_moved = false;
bool white_rook_h1_moved = false;
bool black_rook_a8_moved = false;
bool black_rook_h8_moved = false;



void initialize_boolmatrix(bool * matrix){
  for (int8_t i = 0; i < 64; i++) {
    matrix[i] = false;
  }
}

void initialize_board() {
  for (uint8_t i = 0; i < 64; i++) board[i] = EMPTY;

  board[0] = board[7] = black_rook;
  board[1] = board[6] = black_knight;
  board[2] = board[5] = black_bishop;
  board[3] = black_queen; board[4] = black_king;
  black_king_row = 0; black_king_col = 4;
  for (uint8_t i = 8; i < 16; i++) board[i] = black_pawn;

  board[56] = board[63] = white_rook;
  board[57] = board[62] = white_knight;
  board[58] = board[61] = white_bishop;
  board[59] = white_queen; board[60] = white_king;
  white_king_row = 7; white_king_col = 4;
  for(uint8_t i = 48; i < 56; i++) board[i] = white_pawn;
}

/*
void print_board() {
  // Stampa la scacchiera in formato testuale
  char symbols[13] = {'.', 'R', 'N', 'B', 'Q', 'K', 'P',
                      'r', 'n', 'b', 'q', 'k', 'p'};

  for (uint8_t i = 0; i < 8; i++) {
    printf("%d ", 8 - i);
    for (uint8_t j = 0; j < 8; j++) {
      printf("%c ", symbols[board[i * 8 + j]]);
    }
    printf("\n");
  }
  printf("  a b c d e f g h\n");
}

void print_legal_moves() {
  for (uint8_t i = 0; i < 8; i++) {
    printf("%d ", 8 - i);
    for (uint8_t j = 0; j < 8; j++) {
      printf("%c ", legal_move[i * 8 + j] ? 'o' : '.');
    }
    printf("\n");
  }
  printf("  a b c d e f g h\n");
}

void print_board_ascii() {

    char symbols[13] = {'.', 'R', 'N', 'B', 'Q', 'K', 'P',
                      'r', 'n', 'b', 'q', 'k', 'p'};


    printf("\n            Board Goduriosa\n\n");
    printf("      a   b   c   d   e   f   g   h\n");
    printf("    +---+---+---+---+---+---+---+---+\n");

    for (int8_t i = 0; i < 8; i++) {
        printf(" %d  |", 8 - i);
        for (int8_t j = 0; j < 8; j++) {
            char piece = symbols[board[i * 8 + j]];
            if (piece == 0) piece = '.'; // Casella vuota
            printf(" %c |", piece);
        }
        printf("  %d\n", 8 - i);
        printf("    +---+---+---+---+---+---+---+---+\n");
    }

    printf("      a   b   c   d   e   f   g   h\n\n");
}

void print_board_pretty() {
    setlocale(LC_ALL, "en_US.UTF-8");

    // Indici: 0 = vuoto, 1–6 = bianchi, 7–12 = neri
    const char *symbols[13] = {
        "·",
        "♜",
        "♞",
        "♝",
        "♛",
        "♚",
        "♟",
        "♖",
        "♘",
        "♗",
        "♕",
        "♔",
        "♙"
    };

    printf("\n            ER GODURIA\n\n");
    printf("      a   b   c   d   e   f   g   h\n");
    printf("    ┌───┬───┬───┬───┬───┬───┬───┬───┐\n");

    for (int8_t i = 0; i < 8; i++) {
        printf(" %d  │", 8 - i);
        for (int8_t j = 0; j < 8; j++) {
            uint8_t index = board[i * 8 + j];
            if (index > 12) index = 0; // fallback
            printf(" %s │", symbols[index]);
        }
        printf("  %d\n", 8 - i);
        if (i < 7)
            printf("    ├───┼───┼───┼───┼───┼───┼───┼───┤\n");
    }

    printf("    └───┴───┴───┴───┴───┴───┴───┴───┘\n");
    printf("      a   b   c   d   e   f   g   h\n\n");
}*/