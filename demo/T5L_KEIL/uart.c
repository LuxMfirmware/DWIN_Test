/**
 * @file uart.c
 * @brief UART Communication Driver.
 * @details This file manages UART5 (UART3 in some DWIN docs terminology relative to SFRs,
 *          but logically handled as the primary comms channel here).
 *          It implements a circular buffer for reception and basic blocking transmission.
 */

#include "uart.h"

volatile u8 Rx_Buffer[32];
volatile u8 Rx_Head = 0;
volatile u8 Rx_Tail = 0;

/**
 * @brief Initialize UART5
 * @details Configures UART5 for communication (Receive/Transmit enabled).
 *          Disables RS485 TX enable pin initially.
 */
void UART5_Init(void)
{
    SCON3T=0x80;        // Enable UART5 Transmit
    SCON3R=0x80;        // Enable UART5 Receive
    // Baud rate setting (Assuming specific system clock divider for desired baud)
    BODE3_DIV_H=0x00;
    BODE3_DIV_L=0xE0;
    ES3R=1;             // Enable UART5 Receive Interrupt
    RS485_TX_EN=0;      // Set RS485 to Receive Mode (Low)
    EA=1;               // Enable Global Interrupts
}

/**
 * @brief Send a single byte via UART5
 * @param dat The byte to send.
 * @details This function blocks until transmission is complete.
 */
void UART5_Sendbyte(u8 dat)
{
    SBUF3_TX = dat;             // Load data into transmit buffer
    while((SCON3T&0x01)==0);    // Wait for Transmit Interrupt Flag (TI)
    SCON3T &=0xFE;              // Clear TI Flag
}

/**
 * @brief Send a string via UART5
 * @param pstr Pointer to the data buffer.
 * @param strlen Length of data to send.
 * @details Handles RS485 enable/disable switching around the transmission.
 */
void UART5_SendStr(u8 *pstr,u8 strlen)
{
    if((NULL == pstr)||(0 == strlen))
    {
        return;
    }
    RS485_TX_EN=1; // Enable RS485 Driver (Transmit Mode)
    while(strlen--)
    {
        UART5_Sendbyte(*pstr);
        pstr++;
    }
    RS485_TX_EN=0; // Disable RS485 Driver (Receive Mode)
}

/**
 * @brief UART5 Receive Interrupt Service Routine
 * @details Reads received byte and stores it in the circular buffer.
 */
void UART5_RX_ISR_PC(void)    interrupt 14
{
    // Check if Receive Interrupt Flag is set
    if((SCON3R&0x01)==0x01)
    {
        u8 res = SBUF3_RX;          // Read received data
        Rx_Buffer[Rx_Head] = res;   // Store in buffer
        Rx_Head = (Rx_Head + 1) & 0x1F; // Increment head with wrap-around (Mod 32)
        SCON3R&=0xFE;               // Clear Receive Interrupt Flag
    }
}
