#include "PgnData.h"

// Risale di due livelli da build/Debug fino alla cartella Core/Src/
__asm__(
    ".section .rodata\n"
    ".global CURRENT_STUDY_PGN\n"
    "CURRENT_STUDY_PGN:\n"
    ".incbin \"../../Core/Src/MatchToStudy.pgn\"\n"
    ".byte 0\n"
);