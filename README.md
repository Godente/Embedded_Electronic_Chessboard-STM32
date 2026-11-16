<div align="center">

# ♟️ Cheese&Breadboard
### Embedded Electronic Chessboard

**A physical chessboard that senses every move, validates the game, and guides players with LEDs.**

![STM32 Nucleo-F401RE](https://img.shields.io/badge/MCU-STM32_Nucleo--F401RE-03234B?style=for-the-badge&logo=stmicroelectronics&logoColor=white)
![Language C](https://img.shields.io/badge/Language-C-A8B9CC?style=for-the-badge&logo=c&logoColor=white)
![SPI · UART · GPIO](https://img.shields.io/badge/Interfaces-SPI%20%7C%20UART%20%7C%20GPIO-6B8E23?style=for-the-badge)
![Documentation in Italian](https://img.shields.io/badge/Docs-Italian-9B5DE5?style=for-the-badge)

[Features](#-features) · [Modes](#-play-modes) · [Hardware](#-hardware) · [Repository](#-repository)

</div>

---

## ♟️ About the project

**Cheese&Breadboard** is an embedded electronic chessboard built from scratch around the **STM32 Nucleo-F401RE** (ARM Cortex-M4). Written entirely in **C**, it combines a pressure-sensor board, LED move guidance, an on-board chess engine, and UART communication with a PC.

> 🧠 **The idea:** play on a real board while the firmware takes care of move validation and visual feedback.

## ✨ Features

|  | Feature | What it does |
|:--:|---|---|
| 🎯 | **Piece detection** | An 8×8 pressure-sensor matrix detects piece positions in real time, with non-blocking software debouncing. |
| 💡 | **LED move guidance** | An 8×8 LED matrix, driven by a MAX7219 over SPI, highlights selected pieces, legal moves, captures, and check states. |
| ♜ | **Chess rules** | The on-board engine validates moves and supports castling, *en passant*, pawn promotion, check, and checkmate detection. |
| 📖 | **PGN study** | An embedded parser reads Portable Game Notation (PGN) games on the MCU and guides players through them. |
| 🔌 | **PC connection** | Interrupt-driven UART enables board visualization, debugging, and human-vs-engine gameplay. |

## 🎮 Play modes

| Mode | Description |
|---|---|
| **Local 1v1** | Two players play over the board with real-time rule enforcement and LED hints for valid moves. |
| **PGN Study** | Replay a loaded game move by move, with LEDs indicating the next move. |
| **Player vs. PC** | Play against a desktop chess engine through the UART serial interface. |

## 🛠️ Hardware

| Component | Role | Interface |
|---|---|---|
| **STM32 Nucleo-F401RE** | Main MCU (ARM Cortex-M4) | — |
| **8×8 pressure-sensor matrix** | Detects piece positions on all 64 squares | GPIO · row/column scanning |
| **MAX7219 + 8×8 LED matrix** | Displays move guidance and alerts | SPI |
| **USB / ST-LINK** | Power, flashing, and PC communication | UART · interrupt-driven |

## 📂 Repository

```text
.
├── firmware/           # C source, peripheral drivers, and chess engine
├── docs/
│   ├── relazione.pdf   # Technical report (Italian)
│   └── slide.pdf       # Project presentation (Italian)
└── README.md
```

<div align="center">

**Built with ♟️, C, and a lot of breadboard wiring.**

</div>
