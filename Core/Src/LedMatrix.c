#include "LedMatrix.h"
#include "Board.h"
#include "Rules.h"
#include "Move.h"
#include <stdlib.h>

extern SPI_HandleTypeDef hspi1;

// Mappatura colonne fisiche MAX7219
const uint8_t LED_COL_MASK[8] = {
    0x40, // Col A (bit 0) -> SEG A
    0x20, // Col B (bit 1) -> SEG B
    0x10, // Col C (bit 2) -> SEG C
    0x08, // Col D (bit 3) -> SEG D
    0x04, // Col E (bit 4) -> SEG E
    0x02, // Col F (bit 5) -> SEG F
    0x01, // Col G (bit 6) -> SEG G
    0x80  // Col H (bit 7) -> SEG DP
};

void MAX7219_write(uint8_t address, uint8_t data) {
    uint8_t txData[2] = {address, data};
    HAL_GPIO_WritePin(MAX7219_CS_GPIO_Port, MAX7219_CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, txData, 2, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(MAX7219_CS_GPIO_Port, MAX7219_CS_Pin, GPIO_PIN_SET);
}

void MAX7219_init(void) {
    MAX7219_write(0x09, 0x00); // Decode mode OFF
    MAX7219_write(0x0A, 0x03); // Intensità luminosa media/bassa
    MAX7219_write(0x0B, 0x07); // Scansiona 8 righe
    MAX7219_write(0x0C, 0x01); // Normal Operation
    MAX7219_write(0x0F, 0x00); // Display Test OFF
}

void MAX7219_setall(uint8_t state) {
    uint8_t val = (state != 0) ? 0xFF : 0x00;
    for (uint8_t i = 1; i <= 8; i++) {
        MAX7219_write(i, val);
    }
}

uint8_t map_logical_to_LED(uint8_t logical_byte) {
    uint8_t hw_byte = 0;
    for (uint8_t col = 0; col < 8; col++) {
        if (logical_byte & (1 << col)) {
            uint8_t target_col = INVERT_COLS ? (7 - col) : col;
            hw_byte |= LED_COL_MASK[target_col];
        }
    }
    return hw_byte;
}

void display_team_pieces(int8_t team) {
    for (uint8_t r = 0; r < 8; r++) {
        uint8_t logical_row_byte = 0;
        for (uint8_t c = 0; c < 8; c++) {
            uint8_t piece = board[r * 8 + c];
            if (piece != EMPTY && piece_team(piece) == team) {
                logical_row_byte |= (1 << c);
            }
        }
        uint8_t hw_led_byte = map_logical_to_LED(logical_row_byte);
        uint8_t target_display_row = INVERT_ROWS ? (7 - r) : r;
        MAX7219_write(target_display_row + 1, hw_led_byte);
    }
}

void display_legal_moves_on_LED(uint8_t f_r, uint8_t f_c) {
    uint8_t blink_on = (HAL_GetTick() / 200) % 2;

    for (uint8_t r = 0; r < 8; r++) {
        uint8_t logical_row_byte = 0;
        for (uint8_t c = 0; c < 8; c++) {
            if (legal_move[r * 8 + c]) {
                uint8_t target_piece = board[r * 8 + c];
                uint8_t source_piece = board[f_r * 8 + f_c];

                bool is_capture = (target_piece != EMPTY && piece_team(target_piece) != piece_team(source_piece)) ||
                                  (piece_type(source_piece) == 6 && r == en_passant_row && c == en_passant_col);

                if (is_capture) {
                    if (blink_on) {
                        logical_row_byte |= (1 << c);
                    }
                } else {
                    logical_row_byte |= (1 << c);
                }
            }
        }
        uint8_t hw_led_byte = map_logical_to_LED(logical_row_byte);
        uint8_t target_display_row = INVERT_ROWS ? (7 - r) : r;
        MAX7219_write(target_display_row + 1, hw_led_byte);
    }
}

// Mostra sulla matrice 8x8 le scritte: "1-1", "ST" o "PC"
void display_mode_icon(uint8_t mode) {
    // Bitmap 8x8 per "1-1" (Modalità 1v1)
    static const uint8_t icon_1v1[8] = {
        0x00,
        0x42, // . # . . . . # .
        0x63, // # # . . . # # .
        0x42, // . # . . . . # .
        0x42, // . # . . . . # .
        0x42, // . # . . . . # .
        0xE7, // # # # . . # # #
        0x00
    };

    // Bitmap 8x8 per "ST" (Modalità Studio PGN)
    static const uint8_t icon_ST[8] = {
        0x00,
        0x77, // # # # . # # # .
        0x21, // # . . . . # . .
        0x27, // # # # . . # . .
        0x24, // . . # . . # . .
        0x27, // # # # . . # . .
        0x00,
        0x00
    };

    // Bitmap 8x8 per "PC" (Modalità Giocatore vs PC)
    static const uint8_t icon_PC[8] = {
        0x00,
        0x77, // # # # . # # # .
        0x15, // # . # . # . . .
        0x17, // # # # . # . . .
        0x11, // # . . . # . . .
        0x71, // # . . . # # # .
        0x00,
        0x00
    };

    const uint8_t *icon = icon_1v1;
    if (mode == 1) icon = icon_ST;
    else if (mode == 2) icon = icon_PC;

    // Disegna l'icona selezionata
    for (uint8_t r = 0; r < 8; r++) {
        uint8_t hw_led_byte = map_logical_to_LED(icon[r]);
        uint8_t target_display_row = INVERT_ROWS ? (7 - r) : r;
        MAX7219_write(target_display_row + 1, hw_led_byte);
    }

    HAL_Delay(1200); // Mantiene la scritta visibile per 1200ms
}

void animate_checkmate(uint8_t to_r, uint8_t to_c, int8_t mated_team) {
    uint8_t king_r = (mated_team == -1) ? white_king_row : black_king_row;
    uint8_t king_c = (mated_team == -1) ? white_king_col : black_king_col;

    // 1. Lampeggio Alternato Prolungato
    for (int i = 0; i < 20; i++) {
        MAX7219_setall(0);
        if (i % 2 == 0) {
            uint8_t disp_r = INVERT_ROWS ? (7 - to_r) : to_r;
            MAX7219_write(disp_r + 1, map_logical_to_LED(1 << to_c));
        } else {
            uint8_t disp_r = INVERT_ROWS ? (7 - king_r) : king_r;
            MAX7219_write(disp_r + 1, map_logical_to_LED(1 << king_c));
        }
        HAL_Delay(200);
    }

    // 2. Singolo Raggio Continuo
    int8_t dr = 0, dc = 0;
    if (king_r > to_r) dr = 1;
    else if (king_r < to_r) dr = -1;

    if (king_c > to_c) dc = 1;
    else if (king_c < to_c) dc = -1;

    MAX7219_setall(0);
    uint8_t ray_board[8] = {0};
    uint8_t curr_r = to_r;
    uint8_t curr_c = to_c;

    while (1) {
        ray_board[curr_r] |= (1 << curr_c);

        for (uint8_t row = 0; row < 8; row++) {
            uint8_t disp_r = INVERT_ROWS ? (7 - row) : row;
            MAX7219_write(disp_r + 1, map_logical_to_LED(ray_board[row]));
        }
        HAL_Delay(90);

        if (curr_r == king_r && curr_c == king_c) break;

        if (curr_r != king_r) curr_r += dr;
        if (curr_c != king_c) curr_c += dc;
    }

    HAL_Delay(400);

    // 3. Esplosione del Re
    for (int pass = 0; pass < 2; pass++) {
        for (int radius = 0; radius <= 7; radius++) {
            MAX7219_setall(0);
            for (uint8_t r = 0; r < 8; r++) {
                uint8_t logical_row = 0;
                for (uint8_t c = 0; c < 8; c++) {
                    int dist_r = abs((int)r - (int)king_r);
                    int dist_c = abs((int)c - (int)king_c);
                    int dist = (dist_r > dist_c) ? dist_r : dist_c;

                    if (dist == radius) {
                        logical_row |= (1 << c);
                    }
                }
                if (logical_row != 0) {
                    uint8_t disp_r = INVERT_ROWS ? (7 - r) : r;
                    MAX7219_write(disp_r + 1, map_logical_to_LED(logical_row));
                }
            }
            HAL_Delay(140);
        }
    }

    HAL_Delay(250);

    // 4. Cascata della Vittoria
    int start_r = (mated_team == 1) ? 0 : 7;
    int end_r   = (mated_team == 1) ? 7 : 0;
    int step    = (start_r < end_r) ? 1 : -1;

    uint8_t cascade_board[8] = {0};

    for (int r = start_r; ; r += step) {
        cascade_board[r] = 0xFF;
        for (uint8_t row = 0; row < 8; row++) {
            uint8_t disp_r = INVERT_ROWS ? (7 - row) : row;
            MAX7219_write(disp_r + 1, map_logical_to_LED(cascade_board[row]));
        }
        HAL_Delay(80);
        if (r == end_r) break;
    }

    HAL_Delay(300);

    for (int r = start_r; ; r += step) {
        cascade_board[r] = 0x00;
        for (uint8_t row = 0; row < 8; row++) {
            uint8_t disp_r = INVERT_ROWS ? (7 - row) : row;
            MAX7219_write(disp_r + 1, map_logical_to_LED(cascade_board[row]));
        }
        HAL_Delay(80);
        if (r == end_r) break;
    }
}