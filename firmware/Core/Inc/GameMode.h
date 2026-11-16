#ifndef GAME_MODE_H
#define GAME_MODE_H

#include "main.h"
#include <stdint.h>

typedef enum {
    MODE_HUMAN_VS_HUMAN, // Partita classica su scacchiera
    MODE_STUDY_PGN,       // Riproduzione mosse inviate dal PC
    MODE_VS_PC           // Giocatore vs Motore su PC
} GameMode_t;

void GameController_Init(GameMode_t mode);
void GameController_Process(uint8_t curr_phys_board[8], uint8_t prev_phys_board[8]);

#endif /* GAME_MODE_H */