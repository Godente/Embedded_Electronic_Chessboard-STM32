/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body with Enhanced Checkmate Animations
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "Board.h"
#include "Rules.h"
#include "Move.h"

#include <stdlib.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
// Stati del flusso di gioco
typedef enum {
    STATE_SELECT_PIECE,
    STATE_SELECT_DESTINATION,
    STATE_GAME_OVER
} GameState_t;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// === IMPOSTAZIONI DI ORIENTAMENTO HARDWARE ===
#define INVERT_ROWS    1   // Imposta a 1 per scambiare Alto/Basso (Bianchi/Neri)
#define INVERT_COLS    0   // Imposta a 1 per scambiare Destra/Sinistra (Re/Regina)

// Mappatura colonne fisiche MAX7219 (da Colonna A [bit 0] a Colonna H [bit 7])
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

// Trasmette un comando (indirizzo + dato) al chip MAX7219 via SPI
void MAX7219_write(uint8_t address, uint8_t data) {
    uint8_t txData[2] = {address, data};
    HAL_GPIO_WritePin(MAX7219_CS_GPIO_Port, MAX7219_CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi1, txData, 2, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(MAX7219_CS_GPIO_Port, MAX7219_CS_Pin, GPIO_PIN_SET);
}

// Inizializza i registri interni del MAX7219
void MAX7219_init(void) {
    MAX7219_write(0x09, 0x00); // Decode mode OFF (controllo LED diretto)
    MAX7219_write(0x0A, 0x03); // Intensità luminosa media/bassa
    MAX7219_write(0x0B, 0x07); // Scansiona tutte le 8 righe (0-7)
    MAX7219_write(0x0C, 0x01); // Normal Operation (Esce da shutdown)
    MAX7219_write(0x0F, 0x00); // Display Test OFF
}

// Imposta lo stato di tutti i 64 LED (1 = tutti accesi, 0 = tutti spenti)
void MAX7219_setall(uint8_t state) {
    uint8_t val = (state != 0) ? 0xFF : 0x00;
    for (uint8_t i = 1; i <= 8; i++) {
        MAX7219_write(i, val);
    }
}

// Converte un byte logico (bit 0=Col A ... bit 7=Col H) nel byte fisico richiesto dal MAX7219
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

// Scansiona la matrice di pulsanti (PC0..PC7 = Righe, PB0..PB7 = Colonne)
uint8_t scan_chessboard(uint8_t boardState[8]) {
    uint8_t anyPressed = 0;

    // Imposta tutte le righe (PC0..PC7) ad HIGH
    GPIOC->ODR |= 0xFF;

    for (uint8_t r = 0; r < 8; r++) {
        // Abbassa a LOW (0V) soltanto la riga corrente
        GPIOC->ODR &= ~(1 << r);

        // Breve ritardo hardware per la stabilizzazione del segnale
        for (volatile int delay = 0; delay < 10; delay++);

        // Leggi gli 8 bit della Porta B (PB0..PB7)
        uint8_t raw_colData = ~(GPIOB->IDR & 0xFF);
        uint8_t colData = 0;

        // Inversione colonne in lettura (se attiva)
        if (INVERT_COLS) {
            for (uint8_t c = 0; c < 8; c++) {
                if (raw_colData & (1 << c)) {
                    colData |= (1 << (7 - c));
                }
            }
        } else {
            colData = raw_colData;
        }

        // Inversione righe in salvataggio (se attiva)
        uint8_t target_row = INVERT_ROWS ? (7 - r) : r;
        boardState[target_row] = colData;
        
        if (colData != 0) {
            anyPressed = 1;
        }

        // Riporta SEMPRE la riga corrente ad HIGH prima di passare alla successiva
        GPIOC->ODR |= (1 << r);
    }

    return anyPressed;
}

// Illumina i LED di tutte le caselle che contengono i pezzi del giocatore di turno
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

// Illumina le mosse legali. Se una mossa permette la CATTURA, il relativo LED LAMPEGGIA!
void display_legal_moves_on_LED(uint8_t f_r, uint8_t f_c) {
    // Stato di lampeggio basato sul timer di sistema HAL (toggle ogni 200ms)
    uint8_t blink_on = (HAL_GetTick() / 200) % 2;

    for (uint8_t r = 0; r < 8; r++) {
        uint8_t logical_row_byte = 0;
        for (uint8_t c = 0; c < 8; c++) {
            if (legal_move[r * 8 + c]) {
                uint8_t target_piece = board[r * 8 + c];
                uint8_t source_piece = board[f_r * 8 + f_c];

                // Verifica se la mossa è una cattura standard o en-passant
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

// Sequenza Estesa e Coreografica di Scacco Matto
void animate_checkmate(uint8_t to_r, uint8_t to_c, int8_t mated_team) {
    // Coordinate del Re sconfitto
    uint8_t king_r = (mated_team == -1) ? white_king_row : black_king_row;
    uint8_t king_c = (mated_team == -1) ? white_king_col : black_king_col;

    // --- FASE 1: Lampeggio Alternato Prolungato (4 secondi) ---
    for (int i = 0; i < 15; i++) {
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

    // --- FASE 2: Effetto Scia / Raggio d'Attacco (dall'attaccante al Re) ---
    int8_t dr = 0, dc = 0;
    if (king_r > to_r) dr = 1;
    else if (king_r < to_r) dr = -1;

    if (king_c > to_c) dc = 1;
    else if (king_c < to_c) dc = -1;

    // Ripeti il lancio del raggio luminoso per 3 volte
    for (int loop = 0; loop < 1; loop++) {
        MAX7219_setall(0);
        uint8_t curr_r = to_r;
        uint8_t curr_c = to_c;

        while (1) {
            uint8_t disp_r = INVERT_ROWS ? (7 - curr_r) : curr_r;
            MAX7219_write(disp_r + 1, map_logical_to_LED(1 << curr_c));
            HAL_Delay(100);

            if (curr_r == king_r && curr_c == king_c) break;

            if (curr_r != king_r) curr_r += dr;
            if (curr_c != king_c) curr_c += dc;
        }
        HAL_Delay(150);
    }

    // --- FASE 3: Esplosione del Re Prolungata (2 cicli di onde concentriche) ---
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

    // --- FASE 4: Cascata della Vittoria (scorre verso il vincitore) ---
    int start_r = (mated_team == 1) ? 0 : 7;
    int end_r   = (mated_team == 1) ? 7 : 0;
    int step    = (start_r < end_r) ? 1 : -1;

    uint8_t cascade_board[8] = {0};

    // Ondata 1: Si riempie verso la parte del vincitore
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

    // Ondata 2: Si svuota verso la parte del vincitore
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

/* USER CODE END 0 */


int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  MAX7219_init();

  // Test iniziale dei LED
  MAX7219_setall(1);
  HAL_Delay(1000);
  MAX7219_setall(0);
  HAL_Delay(200);

  // Inizializza la scacchiera
  initialize_board();

  int8_t current_team = -1; // -1 = Bianco, 1 = Nero
  GameState_t state = STATE_SELECT_PIECE;

  uint8_t prev_phys_board[8] = {0};
  uint8_t curr_phys_board[8] = {0};

  uint8_t from_row = 0, from_col = 0;

  display_team_pieces(current_team);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      scan_chessboard(curr_phys_board);

      // Aggiornamento dinamico del display nella fase di selezione destinazione (per far lampeggiare i LED di cattura)
      if (state == STATE_SELECT_DESTINATION) {
          display_legal_moves_on_LED(from_row, from_col);
      }

      // Rilevamento pressione pulsante (fronte di salita 0->1)
      for (uint8_t r = 0; r < 8; r++) {
        uint8_t pressed_bits = (~prev_phys_board[r] & curr_phys_board[r]);

        if(pressed_bits != 0) {
          for (uint8_t c = 0; c < 8; c++){
            if (pressed_bits & (1 << c)) {

                // --- SELEZIONE PEZZO DA MUOVERE ---
                if (state == STATE_SELECT_PIECE) {
                  uint8_t piece = board[r * 8 + c];

                  // Verificare appartenenza pezzo al giocatore
                  if (piece != EMPTY && piece_team(piece) == current_team) {
                    from_row = r;
                    from_col = c;

                    // Calcola mosse legali per il pezzo selezionato
                    legal_moves(from_row, from_col);

                    // Mostra le mosse legali sul display LED (con lampeggio catture)
                    display_legal_moves_on_LED(from_row, from_col);

                    state = STATE_SELECT_DESTINATION;
                  }
                }
                // --- SELEZIONE CASELLA DI DESTINAZIONE ---
                else if (state == STATE_SELECT_DESTINATION) {

                  // Nel caso di riclicco sulla stessa casella (annullamento)
                  if (from_row == r && from_col == c) {
                    display_team_pieces(current_team);
                    state = STATE_SELECT_PIECE;
                  }
                  // Nel caso di selezione di una casella legale
                  else if (legal_move[r * 8 + c]) {
                    uint8_t move[4] = {from_row, from_col, r, c};
                    move_piece(move);
                    
                    current_team *= -1; // Cambia turno

                    // Controlla se il nuovo giocatore di turno è sotto Scacco Matto
                    if (is_checkmate(current_team)) {
                        animate_checkmate(r, c, current_team);
                        state = STATE_GAME_OVER;
                    } else {
                        display_team_pieces(current_team);
                        state = STATE_SELECT_PIECE;
                    }
                  }
                // Selezione casella non legale: ignora e resta nello stato corrente
                }
            }
          }
        }
      }

      // Aggiorna lo stato precedente della matrice di pulsanti
      for (uint8_t r = 0; r < 8; r++) {
        prev_phys_board[r] = curr_phys_board[r];
      }

      HAL_Delay(30); // Frequenza di aggiornamento a 33 Hz per la fluidità del lampeggio
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(MAX7219_CS_GPIO_Port, MAX7219_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PC0 PC1 PC2 PC3
                           PC4 PC5 PC6 PC7 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : MAX7219_CS_Pin */
  GPIO_InitStruct.Pin = MAX7219_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(MAX7219_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PB0 PB1 PB2 PB3
                           PB4 PB5 PB6 PB7 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */