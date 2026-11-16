#ifndef PGN_PARSER_H
#define PGN_PARSER_H

#include "main.h"
#include <stdbool.h>
#include <stdint.h>

// Carica una nuova partita PGN in formato stringa grezzo
void PGN_Init(const char *pgn_text);

// Estrae ed esegue la mossa successiva del PGN.
// Ritorna true se la mossa è stata trovata ed eseguita, false se la partita è finita.
bool PGN_GetNextMove(uint8_t move[4], int8_t current_team);

#endif /* PGN_PARSER_H */