/**
 * @file sys.h
 * @brief System Header File.
 * @details Contains type definitions, system macros, structure definitions,
 *          and function prototypes for the system module.
 */

#ifndef __SYS_H__
#define __SYS_H__

#include "t5los8051.h" // Include the hardware register definitions

// --- Type Definitions for Cross-Platform Compatibility ---
typedef unsigned char   u8;     // 8-bit unsigned integer
typedef unsigned short  u16;    // 16-bit unsigned integer
typedef unsigned long   u32;    // 32-bit unsigned integer
typedef char            s8;     // 8-bit signed integer
typedef short           s16;    // 16-bit signed integer
typedef long            s32;    // 32-bit signed integer

// --- System Macros ---
#define WDT_ON()    MUX_SEL |= 0x02     /**< Enable Watchdog */
#define WDT_OFF()   MUX_SEL &= 0xFD     /**< Disable Watchdog */
#define WDT_RST()   MUX_SEL |= 0x01     /**< Reset Watchdog (Feed) */

// --- System Constants ---
// Oscillator Frequency (T5L Core frequency approx 206 MHz)
#define FOSC     206438400UL 
// Timer Reload Value for 1ms interrupt (12 clocks per machine cycle)
#define T1MS    (65536-FOSC/12/1000)
#define NULL ((void *)0)

// --- Structures ---
/**
 * @brief Real-Time Clock Time Structure
 * @details Holds the components of the current date and time.
 *          Aligned to match the DGUS RTC register format.
 */
typedef struct _dev_time
{
    u8 year;    // Year offset from 2000 (e.g., 24 = 2024)
    u8 month;   // Month (1-12)
    u8 day;     // Day of month (1-31)
    u8 week;    // Day of week (0-6)
    u8 hour;    // Hour (0-23)
    u8 min;     // Minute (0-59)
    u8 sec;     // Second (0-59)
    u8 res;     // Reserved byte (for alignment/padding)
} rtc_time;

// --- Global External Variables ---
extern rtc_time real_time;      // Global RTC instance
extern volatile u16 data Wait_Count;     // System tick counter (Volatile for ISR access)

// --- Function Prototypes ---

/**
 * @brief Initialize CPU Core
 */
void INIT_CPU(void);

/**
 * @brief Initialize GPIO Ports
 */
void PORT_Init(void);

/**
 * @brief Initialize Real-Time Clock
 */
void RTC_Init(void);

/**
 * @brief Initialize Timer 0
 */
void T0_Init(void);

/**
 * @brief Initialize Timer 1
 */
void T1_Init(void);

/**
 * @brief Initialize Timer 2
 */
void T2_Init(void);

/**
 * @brief Read from DGUS Variable Pointer (VP) memory
 * @param addr 16-bit Word Address
 * @param buf Buffer pointer
 * @param len Word count
 */
void read_dgus_vp(u32 addr,void* buf,u16 len);

/**
 * @brief Write to DGUS Variable Pointer (VP) memory
 * @param addr 16-bit Word Address
 * @param buf Data pointer
 * @param len Word count
 */
void write_dgus_vp(u32 addr,void* buf,u16 len);

/**
 * @brief Calculate Day of Week
 */
u8 RTC_Get_Week(u8 year,u8 month,u8 day);

/**
 * @brief Routine to update time logic and display
 */
void Time_Update(void);

/**
 * @brief Millisecond delay (blocking)
 * @param n Milliseconds to wait
 */
void delay_ms(u16 n);

#endif
