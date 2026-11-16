# ♟️ Cheese&Breadboard – Embedded Electronic Chessboard

![Microcontroller](https://img.shields.io/badge/MCU-STM32_Nucleo--F401RE-03234B?style=flat-square&logo=stmicroelectronics)
![Language](https://img.shields.io/badge/Language-C-A8B9CC?style=flat-square&logo=c)
![Peripherals](https://img.shields.io/badge/Interfaces-SPI%20%7C%20UART%20%7C%20GPIO-blue?style=flat-square)
![Docs Language](https://img.shields.io/badge/Docs_%26_Slides-Italian_🇮🇹-green?style=flat-square)

**Cheese&Breadboard** is an embedded electronic chessboard built from scratch on an **STM32 Nucleo-F401RE** (ARM Cortex-M4) microcontroller. Written entirely in **C**, the system combines physical piece detection, LED visual feedback, an on-board FIDE-compliant chess engine, and interrupt-driven PC communication.

> **Language Note:** This repository overview is in English, while the attached technical report, presentation slides, and inline firmware comments are written in **Italian** 🇮🇹.

---

## ✨ Key Features

* **8×8 Pressure Sensor Matrix:** Real-time piece detection utilizing non-blocking software *debouncing* for reliable state transitions across all 64 squares.
* **Visual Move Guidance:** **SPI**-driven 8×8 LED matrix controlled via a **MAX7219** driver to highlight selected pieces, valid moves, captures, and check states.
* **On-Board FIDE Chess Engine:** Full rule validation written in C, supporting special moves (castling, *en passant*, pawn promotion) as well as check and checkmate detection.
* **Embedded PGN Parser:** Decodes *Portable Game Notation* (PGN) files directly on the MCU to guide users step-by-step through classic games.
* **Interrupt-Driven UART:** Asynchronous serial communication with a host PC for board visualization, debugging, and human-vs-engine gameplay.

---

## 🎮 Operating Modes

1. **Local 1v1:** Over-the-board play between two human players with real-time rule enforcement and valid-move LED hints.
2. **PGN Study Mode:** Interactive game replay where LEDs indicate the next move from a loaded PGN match.
3. **Player vs. PC:** Play against a desktop chess engine over the UART serial interface.

---

## 🛠️ Hardware Architecture

| Component | Role | Interface |
| :--- | :--- | :--- |
| **STM32 Nucleo-F401RE** | Main MCU (ARM Cortex-M4) | — |
| **8×8 Pressure Sensor Matrix** | Piece position sensing (64 squares) | GPIO (Row/Column scanning) |
| **MAX7219 + 8×8 LED Matrix** | Visual feedback for legal moves & alerts | SPI |
| **USB / ST-LINK** | Power, flashing, and PC communication | UART (Interrupt-driven) |

---

## 📂 Repository Structure

```text
├── firmware/           # C source code, peripheral drivers & chess engine (comments in IT 🇮🇹)
├── docs/
│   ├── relazione.pdf   # Full technical report (in Italian 🇮🇹)
│   └── slide.pdf       # Project presentation slides (in Italian 🇮🇹)
└── README.md           # Project overview (in English 🇬🇧)