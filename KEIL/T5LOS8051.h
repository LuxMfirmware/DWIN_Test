/**
 * @file T5LOS8051.h
 * @brief DWIN T5L ASIC Header File for Keil C51.
 * @details Defines Special Function Registers (SFRs) and bit definitions for the 
 *          T5L 8051 core. Based on "Development-Guide-of-T5L-ASIC20220413".
 *          This file maps the hardware registers to C variables.
 */

#ifndef __T5LOS8051_H__
#define __T5LOS8051_H__

// =========================================================================
// 1. SPECIAL FUNCTION REGISTERS (SFR) - ADDRESS DEFINITIONS
// =========================================================================

// --- Standard 8051 Core SFRs ---
sfr P0          = 0x80; // Port 0 (Bit-addressable)
sfr SP          = 0x81; // Stack Pointer
sfr DPL         = 0x82; // Data Pointer Low
sfr DPH         = 0x83; // Data Pointer High
sfr PCON        = 0x87; // Power Control (See PDF Pg 27)
sfr TCON        = 0x88; // Timer/Counter Control (Bit-addressable)
sfr TMOD        = 0x89; // Timer/Counter Mode
sfr TL0         = 0x8A; // Timer 0 Low Byte
sfr TL1         = 0x8B; // Timer 1 Low Byte
sfr TH0         = 0x8C; // Timer 0 High Byte
sfr TH1         = 0x8D; // Timer 1 High Byte
sfr P1          = 0x90; // Port 1 (Bit-addressable)
sfr P2          = 0xA0; // Port 2 (Bit-addressable)
sfr IEN0        = 0xA8; // Interrupt Enable 0 (Bit-addressable) (See PDF Pg 33)
sfr IP0         = 0xA9; // Interrupt Priority 0 (See PDF Pg 33)
sfr P3          = 0xB0; // Port 3 (Bit-addressable)
sfr IEN1        = 0xB8; // Interrupt Enable 1 (Bit-addressable) (See PDF Pg 33)
sfr IP1         = 0xB9; // Interrupt Priority 1 (See PDF Pg 33)
sfr IRCON       = 0xC0; // Interrupt Request Control (Bit-addressable) (Used for TF2)
sfr T2CON       = 0xC8; // Timer 2 Control (Bit-addressable) (See PDF Pg 22)
sfr PSW         = 0xD0; // Program Status Word (Bit-addressable)
sfr ACC         = 0xE0; // Accumulator (Bit-addressable)
sfr B           = 0xF0; // B Register (Bit-addressable)

// --- System & Memory Configuration (See PDF Pg 14, 18) ---
sfr CKCON       = 0x8E; // Clock Control
sfr DPC         = 0x93; // Data Pointer Control
sfr PAGESEL     = 0x94; // Code Memory Page Select
sfr D_PAGESEL   = 0x95; // Data Memory Page Select
sfr MUX_SEL     = 0xC9; // Peripheral Multiplexing (Not bit-addressable)
sfr PORTDRV     = 0xF9; // Port Drive Strength (Not bit-addressable)

// --- Timer 2 Extensions (See PDF Pg 22) ---
sfr TRL2L       = 0xCA; // Timer 2 Reload Low
sfr TRL2H       = 0xCB; // Timer 2 Reload High
sfr TL2         = 0xCC; // Timer 2 Count Low
sfr TH2         = 0xCD; // Timer 2 Count High

// --- UART2 (8051 Standard UART) (See PDF Pg 27) ---
sfr SCON0       = 0x98; // UART2 Control (Bit-addressable)
sfr SBUF0       = 0x99; // UART2 Data Buffer
sfr SREL0L      = 0xAA; // UART2 Baud Rate Reload Low
sfr SREL0H      = 0xBA; // UART2 Baud Rate Reload High
sfr ADCON       = 0xD8; // Baud Rate Generator Select (Bit-addressable)

// --- UART3 (See PDF Pg 28) ---
// Note: Registers at 0x9B are NOT bit-addressable in 8051 architecture
sfr SCON1       = 0x9B; // UART3 Control
sfr SBUF1       = 0x9C; // UART3 Data Buffer
sfr SREL1L      = 0x9D; // UART3 Baud Rate Reload Low
sfr SREL1H      = 0xBB; // UART3 Baud Rate Reload High
sfr IEN2        = 0x9A; // Interrupt Enable 2 (Contains ES1)

// --- UART4 (See PDF Pg 29) ---
// Note: Registers at 0x96, 0x97 are NOT bit-addressable
sfr SCON2T      = 0x96; // UART4 Transmit Control
sfr SCON2R      = 0x97; // UART4 Receive Control
sfr SBUF2_TX    = 0x9E; // UART4 Transmit Buffer
sfr SBUF2_RX    = 0x9F; // UART4 Receive Buffer
sfr BODE2_DIV_L = 0xD7; // UART4 Baud Rate Div Low
sfr BODE2_DIV_H = 0xD9; // UART4 Baud Rate Div High

// --- UART5 (See PDF Pg 30) ---
// Note: Registers at 0xA7, 0xAB are NOT bit-addressable
sfr SCON3T      = 0xA7; // UART5 Transmit Control
sfr SCON3R      = 0xAB; // UART5 Receive Control
sfr SBUF3_TX    = 0xAC; // UART5 Transmit Buffer
sfr SBUF3_RX    = 0xAD; // UART5 Receive Buffer
sfr BODE3_DIV_H = 0xAE; // UART5 Baud Rate Div High
sfr BODE3_DIV_L = 0xAF; // UART5 Baud Rate Div Low

// --- CAN Interface (See PDF Pg 31) ---
// Note: 0x8F, 0x91 are NOT bit-addressable
sfr CAN_CR      = 0x8F; // CAN Control Register
sfr CAN_IR      = 0x91; // CAN Interrupt Status
sfr CAN_ET      = 0xE8; // CAN Error Type (Bit-addressable!)

// --- GPIO Output Configuration (See PDF Pg 25) ---
sfr P0MDOUT     = 0xB7; // Port 0 Output Mode (0=Open Drain, 1=Push Pull)
sfr P1MDOUT     = 0xBC; // Port 1 Output Mode
sfr P2MDOUT     = 0xBD; // Port 2 Output Mode
sfr P3MDOUT     = 0xBE; // Port 3 Output Mode

// --- Mathematical Unit (MDU) (See PDF Pg 20) ---
sfr MAC_CN      = 0xE5; // MDU Control
sfr DIV_CN      = 0xE6; // Divider Control

// --- Interrupts Extended (See PDF Pg 33) ---
sfr IEN3        = 0xD1; // Must write 0x00

// --- DGUS Variable Memory Access (See PDF Pg 16) ---
sfr ADR_H       = 0xF1; // DGUS RAM Address High
sfr ADR_M       = 0xF2; // DGUS RAM Address Mid
sfr ADR_L       = 0xF3; // DGUS RAM Address Low
sfr ADR_INC     = 0xF4; // Address Increment Step
sfr RAMMODE     = 0xF8; // Access Mode Control (Bit-addressable)
sfr DATA3       = 0xFA; // Data Byte 3 (MSB)
sfr DATA2       = 0xFB; // Data Byte 2
sfr DATA1       = 0xFC; // Data Byte 1
sfr DATA0       = 0xFD; // Data Byte 0 (LSB)

// --- Extended SFR Access (See PDF Pg 19) ---
sfr EXADR       = 0xFE; // Extended SFR Address
sfr EXDATA      = 0xFF; // Extended SFR Data


// =========================================================================
// 2. BIT DEFINITIONS (SBIT)
// Only allowed for SFRs at addresses: 0x80, 0x88, 0x90, 0x98, 0xA0... 0xF8
// =========================================================================

// --- P0 (0x80) ---
sbit P0_0 = P0^0;
sbit P0_1 = P0^1;
sbit P0_2 = P0^2;
sbit P0_3 = P0^3;
sbit P0_4 = P0^4;
sbit P0_5 = P0^5;
sbit P0_6 = P0^6;
sbit P0_7 = P0^7;

// --- TCON (0x88) - Timer Control (See PDF Pg 22) ---
sbit TF1 = TCON^7; // Timer 1 Overflow Flag
sbit TR1 = TCON^6; // Timer 1 Run Control
sbit TF0 = TCON^5; // Timer 0 Overflow Flag
sbit TR0 = TCON^4; // Timer 0 Run Control
sbit IE1 = TCON^3; // External Interrupt 1 Edge Flag
sbit IT1 = TCON^2; // External Interrupt 1 Type Control
sbit IE0 = TCON^1; // External Interrupt 0 Edge Flag
sbit IT0 = TCON^0; // External Interrupt 0 Type Control

// --- P1 (0x90) ---
sbit P1_0 = P1^0;
sbit P1_1 = P1^1;
sbit P1_2 = P1^2;
sbit P1_3 = P1^3;
sbit P1_4 = P1^4;
sbit P1_5 = P1^5;
sbit P1_6 = P1^6;
sbit P1_7 = P1^7;

// --- SCON0 (0x98) - UART2 Control (See PDF Pg 27) ---
sbit SM0_0 = SCON0^7; // Serial Mode Bit 0
sbit SM1_0 = SCON0^6; // Serial Mode Bit 1
sbit SM2_0 = SCON0^5; // Multiprocessor Comm Enable
sbit REN_0 = SCON0^4; // Receive Enable
sbit TB8_0 = SCON0^3; // 9th Bit to Transmit
sbit RB8_0 = SCON0^2; // 9th Bit Received
sbit TI0   = SCON0^1; // Transmit Interrupt Flag
sbit RI0   = SCON0^0; // Receive Interrupt Flag

// --- P2 (0xA0) ---
sbit P2_0 = P2^0;
sbit P2_1 = P2^1;
sbit P2_2 = P2^2;
sbit P2_3 = P2^3;
sbit P2_4 = P2^4;
sbit P2_5 = P2^5;
sbit P2_6 = P2^6;
sbit P2_7 = P2^7;

// --- IEN0 (0xA8) - Interrupt Enable 0 (See PDF Pg 33) ---
sbit EA  = IEN0^7; // Global Interrupt Enable
// Bit 6 Reserved (0)
sbit ET2 = IEN0^5; // Timer 2 Interrupt Enable
sbit ES0 = IEN0^4; // UART2 Interrupt Enable
sbit ET1 = IEN0^3; // Timer 1 Interrupt Enable
sbit EX1 = IEN0^2; // External Interrupt 1 Enable
sbit ET0 = IEN0^1; // Timer 0 Interrupt Enable
sbit EX0 = IEN0^0; // External Interrupt 0 Enable

// --- P3 (0xB0) ---
sbit P3_0 = P3^0;
sbit P3_1 = P3^1;
sbit P3_2 = P3^2;
sbit P3_3 = P3^3;
// P3.4 - P3.7 are not available on T5L OS CPU IO (see PDF Pg 25)

// --- IEN1 (0xB8) - Interrupt Enable 1 (See PDF Pg 33) ---
// Bit 7-6 Reserved (0)
sbit ES3R = IEN1^5; // UART5 Receive Interrupt Enable
sbit ES3T = IEN1^4; // UART5 Send Interrupt Enable
sbit ES2R = IEN1^3; // UART4 Receive Interrupt Enable
sbit ES2T = IEN1^2; // UART4 Send Interrupt Enable
sbit ECAN = IEN1^1; // CAN Interrupt Enable
// Bit 0 Reserved (0)

// --- IRCON (0xC0) - Interrupt Request (See PDF Pg 22) ---
// Note: PDF Table on Pg 22 says TF2 is TCON.6, but also lists T2CON at 0xC8.
// Standard 8052 and DWIN INC files place TF2 at 0xC0.6 (IRCON/T2CON overlap logic).
sbit TF2 = IRCON^6; // Timer 2 Overflow Flag

// --- T2CON (0xC8) - Timer 2 Control (See PDF Pg 22) ---
sbit T2_CLK_DIV = T2CON^7; // 0=CPU/12, 1=CPU/24
// Bits 6-4 must write 1
// Bits 3-1 must write 0
sbit TR2        = T2CON^0; // Timer 2 Run Control

// --- PSW (0xD0) - Program Status Word ---
sbit CY  = PSW^7; // Carry Flag
sbit AC  = PSW^6; // Auxiliary Carry Flag
sbit F0  = PSW^5; // User Flag 0
sbit RS1 = PSW^4; // Register Bank Select 1
sbit RS0 = PSW^3; // Register Bank Select 0
sbit OV  = PSW^2; // Overflow Flag
sbit F1  = PSW^1; // User Flag 1
sbit P   = PSW^0; // Parity Flag

// --- ADCON (0xD8) - Baud Rate Gen (See PDF Pg 27) ---
sbit SMOD = ADCON^7; // Baud rate doubler

// --- CAN_ET (0xE8) - CAN Error Type (See PDF Pg 31) ---
sbit NODE_SUS    = CAN_ET^7; // Node Suspended
sbit ACTIVE_ER   = CAN_ET^6; // Active Error
sbit PASSIVE_ER  = CAN_ET^5; // Passive Error
sbit CRC_ER      = CAN_ET^4; // CRC Error
sbit FORMAT_ER   = CAN_ET^3; // Format Error
sbit BIT_FILL_ER = CAN_ET^2; // Bit Filling Error
sbit BIT_ER      = CAN_ET^1; // Bit Error

// --- RAMMODE (0xF8) - DGUS Access (See PDF Pg 16) ---
sbit APP_REQ = RAMMODE^7; // Request Access
sbit APP_EN  = RAMMODE^6; // Start Read/Write
sbit APP_RW  = RAMMODE^5; // 1=Read, 0=Write
sbit APP_ACK = RAMMODE^4; // Access Granted Flag
// Bits 3-0: Write enables for DATA3-DATA0

// =========================================================================
// 3. BIT MASKS (For Non-Bit-Addressable SFRs)
// These registers cannot use sbit. Use logical operators (&, |, ^)
// =========================================================================

// MUX_SEL (0xC9) (See PDF Pg 26)
#define MUX_CAN_EN      0x80 // .7: 1=P0.2/P0.3 are CAN
#define MUX_UART2_EN    0x40 // .6: 1=P0.4/P0.5 are UART2
#define MUX_UART3_EN    0x20 // .5: 1=P0.6/P0.7 are UART3
#define MUX_WDT_EN      0x02 // .1: 1=WDT Open
#define MUX_WDT_RST     0x01 // .0: Write 1 to feed dog

// IEN2 (0x9A) (See PDF Pg 33)
#define MASK_ES1        0x01 // .0: UART3 Interrupt Enable

// SCON1 (0x9B) - UART3 (See PDF Pg 28)
#define SCON1_M0        0x80 // .7: Mode (0=9bit, 1=8bit)
#define SCON1_SM2       0x20 // .5: Multi-processor
#define SCON1_REN       0x10 // .4: Receive Enable
#define SCON1_TB8       0x08 // .3: 9th bit transmit
#define SCON1_RB8       0x04 // .2: 9th bit receive
#define SCON1_TI        0x02 // .1: Transmit Interrupt Flag
#define SCON1_RI        0x01 // .0: Receive Interrupt Flag

// SCON2T (0x96) - UART4 Transmit (See PDF Pg 29)
#define SCON2T_EN       0x80 // .7: Enable
#define SCON2T_MOD      0x40 // .6: 0=8bit, 1=9bit
#define SCON2T_TB8      0x20 // .5: 9th bit
#define SCON2T_TI       0x01 // .0: Transmit Flag

// SCON2R (0x97) - UART4 Receive (See PDF Pg 29)
#define SCON2R_EN       0x80 // .7: Enable
#define SCON2R_RB8      0x20 // .5: 9th bit
#define SCON2R_RI       0x01 // .0: Receive Flag

// SCON3T (0xA7) - UART5 Transmit (See PDF Pg 30)
#define SCON3T_EN       0x80 // .7: Enable
#define SCON3T_MOD      0x40 // .6: 0=8bit, 1=9bit
#define SCON3T_TB8      0x20 // .5: 9th bit
#define SCON3T_TI       0x01 // .0: Transmit Flag

// SCON3R (0xAB) - UART5 Receive (See PDF Pg 30)
#define SCON3R_EN       0x80 // .7: Enable
#define SCON3R_RB8      0x20 // .5: 9th bit (PDF says TB8, usually RB8 for RX)
#define SCON3R_RI       0x01 // .0: Receive Flag

// CAN_CR (0x8F) (See PDF Pg 31)
#define CAN_CR_EN       0x80 // .7: Enable
#define CAN_CR_RST      0x40 // .6: 1=Reset
#define CAN_CR_CFG      0x20 // .5: Configure
#define CAN_CR_SPD      0x10 // .4: Speed (1=1samp, 0=3samp)
#define CAN_CR_FILT     0x08 // .3: Filter (1=Dual, 0=Single)
#define CAN_CR_TX       0x04 // .2: Send Request

// CAN_IR (0x91) (See PDF Pg 31)
#define CAN_IR_RF       0x80 // .7: Remote Frame Flag
#define CAN_IR_RX       0x40 // .6: Receive Flag
#define CAN_IR_TX       0x20 // .5: Transmit Flag
#define CAN_IR_OV       0x10 // .4: Overflow Flag
#define CAN_IR_ERR      0x08 // .3: Error Flag
#define CAN_IR_ARB      0x04 // .2: Arbitration Fail Flag

#endif