/**
 * @file uart.h
 * @brief UART Header File.
 * @details Contains definitions, global buffer declarations, and function prototypes
 *          for the UART communication module.
 */

#ifndef __UART_H__
#define __UART_H__

#include "sys.h"

// --- Hardware Definitions ---
// RS485 Transmit Enable Pin Definition (Port 0, Pin 1)
sbit RS485_TX_EN=P0^1;

// --- Function Prototypes ---

/**
 * @brief Initialize UART5 Configuration
 */
void UART5_Init(void);

/**
 * @brief Send a single byte over UART5
 * @param dat Data byte to send
 */
void UART5_Sendbyte(u8 dat);

/**
 * @brief Send a buffer of bytes over UART5
 * @param pstr Pointer to buffer
 * @param strlen Number of bytes to send
 */
void UART5_SendStr(u8 *pstr,u8 strlen);

// --- Global External Variables ---
/** @brief UART Receive Circular Buffer. */
extern volatile u8 Rx_Buffer[32];
/** @brief Head index of the circular receive buffer, written by ISR. */
extern volatile u8 Rx_Head;
/** @brief Tail index of the circular receive buffer, read by the main loop. */
extern volatile u8 Rx_Tail;

#endif
