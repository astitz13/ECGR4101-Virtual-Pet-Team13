# ECGR4101-Virtual-Pet
Virtual Pet project for ECGR 4101 Advanced Embedded Systems, Lab Group 13

# Project Overview / The Pet
Perry’s fullness and happiness change over time and through user interactions. To increase fullness, select FEED from the menu and press B1. To increase happiness, select GAME and win the mini-game. After 10 seconds without input, Perry enters agent mode and waits for a sound or button press to call him back. If fullness reaches zero, Perry dies, while reaching zero happiness causes him to run away. In either case, hold the joystick’s CENTER button for two seconds to restart the virtual pet.

# Hardware and Connection
- NUCLEO-L496ZG-P
- ILI9341 display/X-NUCLEO-GFX01M2
- Display: SPI1, PA5/PA6/PA7
- LCD CS: PA9, DC: PB10, RESET: PA1
- MPU6050: I2C1 on PG13/PG14
- Microphone: PF3/ADC3
- Audio output: PE0/TIM16 PWM
- Joystick/button connections

# Controls
- Joystick directions and center button
- B1 usage
- Menu navigation
- Feeding and playing
- Agent-mode voice interaction
- Tilting to move between game lanes

# Software Architecture

<p align="center">
  <img src="Software%20Architecture.png"
       alt="Software Architecture"
       width="900">
</p>

# Team Members
- Full name
  - Dylan Hull
  - Gabriel Almodovar
  - Luke Robinson
  - Austin Stitz
  - Austin Stoeckert
