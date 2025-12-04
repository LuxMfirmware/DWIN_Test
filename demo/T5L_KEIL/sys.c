/**
 * @file sys.c
 * @brief System Initialization and Utility Functions for DWIN T5L.
 * @details Implements core system setup, reliable DGUS memory access (Read/Write),
 * and Software RTC management.
 * Based on "Development-Guide-of-T5L-ASIC20220413".
 */

#include "sys.h"
#include "uart.h"
#include "string.h"

// --- Lookup Tables for Calendar Calculation ---
const u8 code table_week[12] = {0,3,3,6,1,4,6,2,5,0,3,5};
const u8 code mon_table[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
const u8 code time_set_init[6] = {19,5,1,12,00,00};

// --- System Global Variables ---
/** @brief System tick counter, incremented every ms by Timer 0 ISR. Volatile for safe ISR access. */
volatile u16 data Wait_Count = 0;          
/** @brief Internal counter for the RTC tick, used to measure 1-second intervals. */
static u16 data SysTick_RTC = 0;  
/** @brief Countdown variable for the `delay_ms` function. */
static u16 data SysTick = 0;      
/** @brief Global structure holding the current real-time clock time. */
rtc_time real_time;             
/** @brief Flag set by the RTC ISR every second to signal the main loop to update the display. */
volatile u16 Second_Updata_Flag = 0;       
/** @brief Buffer to hold time values for display purposes. */
u16 time_display[7] = {0};       

/**
 * @brief Initialize CPU Core Registers
 * @details Sets up Interrupts, GPIO modes, UARTs, and Timers according to T5L specs.
 */
void INIT_CPU(void)
{
    EA = 0;           
    RS0 = 0;          
    RS1 = 0;
    PORTDRV = 0x01;   
    IEN0 = 0x00;      
    IEN1 = 0x00;
    IEN2 = 0x00;
    IP0 = 0x00;       
    IP1 = 0x00;
    
    // Disable Watchdog (WDT) [cite: 1810]
    MUX_SEL &= 0xFD;  

    // Initialize Ports to Input Mode (High Impedance/Weak Pull-up equivalent)
    P0 = 0xFF; P1 = 0xFF; P2 = 0xFF; P3 = 0xFF;
    
    // Configure Output Modes (1=Push-Pull) [cite: 1986]
    P0MDOUT = 0x10; 
    P1MDOUT = 0x00; 
    P2MDOUT = 0x00; 
    P3MDOUT = 0x00;

    // UART0 (Standard 8051 UART) Configuration
    ADCON = 0x80;     
    SCON0 = 0x50;     
    SREL0H = 0x03;    
    SREL0L = 0xE4;    
    
    // UART4 Configuration
    SCON2T = 0x80;    
    SCON2R = 0x80;    
    BODE2_DIV_H = 0x00; 
    BODE2_DIV_L = 0xE0;
    
    // Timer Configuration
    TMOD = 0x11;      
    TH0 = 0x00; TL0 = 0x00; TR0 = 0x00; 
    TH1 = 0x00; TL1 = 0x00; TR1 = 0x00; 
    
    // Timer 2 Configuration (16-bit Auto-reload) [cite: 1925]
    T2CON = 0x70; 
    TH2 = 0x00; TL2 = 0x00; 
    TRL2H = 0xBC; TRL2L = 0xCD;
}

/**
 * @brief Initialize GPIO Port Directions.
 * @details Configures specific pins on P0, P1, and P2 as push-pull outputs.
 */
void PORT_Init(void)
{
    // Specific GPIO setup for the project
    P0MDOUT |= 0x02; 
    P1MDOUT |= 0x1E; 
    P2MDOUT |= 0x02; 
}

/**
 * @brief Initialize the software Real-Time Clock.
 * @details Loads the `real_time` structure with a default compile-time date and time.
 */
void RTC_Init(void)
{
    // Load initial time values
    real_time.year = time_set_init[0];
    real_time.month = time_set_init[1];
    real_time.day = time_set_init[2];
    real_time.hour = time_set_init[3];
    real_time.min = time_set_init[4];
    real_time.sec = time_set_init[5];
}

/**
 * @brief Initialize Timer 0 for System Tick.
 * @details Configures Timer 0 to generate an interrupt every 1ms, which serves as the main system tick.
 */
void T0_Init(void)
{
    // System Tick Timer
    TMOD |= 0x01;         
    TH0 = T1MS >> 8;        
    TL0 = T1MS;           
    ET0 = 1;              
    EA = 1;               
    TR0 = 1;              
}

/**
 * @brief Initialize Timer 1 for RTC Tick.
 * @details Configures Timer 1 to generate a 1ms interrupt used for the software RTC counting.
 */
void T1_Init(void)
{
    // RTC Tick Timer
    TMOD |= 0x10;        
    TH1 = T1MS >> 8;        
    TL1 = T1MS;           
    ET1 = 1;              
    EA = 1;               
    TR1 = 1;              
}

// =============================================================================
//  UNIVERSAL DGUS MEMORY ACCESS FUNCTIONS (FIXED)
// =============================================================================

/**
 * @brief Write data to DGUS Variable Pointer (VP) memory.
 * @details Handles 16-bit to 32-bit address mapping and prevents overlap using RAMMODE masking.
 * Waits for hardware completion signal (APP_EN = 0).
 * @param addr 16-bit VP Address (e.g., 0x2010)
 * @param vbuf Pointer to data buffer
 * @param len Length of data in bytes (1, 2, or 4)
 */
void write_dgus_vp(u32 addr, void* vbuf, u16 len)
{
    u8* buf = (u8*)vbuf;
    u32 OS_addr = addr >> 1; // Convert 16-bit VP addr to 32-bit OS slot addr
    u8 is_odd = addr & 0x01; // Check if VP address is Odd
    u8 mask = 0x00;          // Mask for RAMMODE to select specific bytes

    // 1. Set OS CPU Address Registers 
    ADR_H = (u8)(OS_addr >> 16);
    ADR_M = (u8)(OS_addr >> 8);
    ADR_L = (u8)OS_addr;
    ADR_INC = 0x00; // Disable auto-increment for single atomic writes

    // 2. Load Data registers and build Mask
    // DATA3/DATA2 = Even VP Address (High Word of OS Slot)
    // DATA1/DATA0 = Odd VP Address (Low Word of OS Slot)
    
    if(!is_odd) // --- EVEN ADDRESS (e.g. 0x2010, 0x2020) ---
    {
        if(len > 0) { DATA3 = *buf++; mask |= 0x08; } // Bit 3 Enable
        if(len > 1) { DATA2 = *buf++; mask |= 0x04; } // Bit 2 Enable
        // If writing 4 bytes (Long Int) to an Even Address, fill the rest
        if(len > 2) { DATA1 = *buf++; mask |= 0x02; } // Bit 1 Enable
        if(len > 3) { DATA0 = *buf++; mask |= 0x01; } // Bit 0 Enable
    }
    else // --- ODD ADDRESS (e.g. 0x2011) ---
    {
        // Odd address starts at lower half of the 32-bit slot
        if(len > 0) { DATA1 = *buf++; mask |= 0x02; } // Bit 1 Enable
        if(len > 1) { DATA0 = *buf++; mask |= 0x01; } // Bit 0 Enable
    }

    // 3. Execute Write Command
    if(mask != 0)
    {
        // RAMMODE: Bit 7 (1=Request) | Mask (Bits 3-0 for Byte Enables) 
        RAMMODE = 0x80 | mask; 
        
        APP_EN = 1;         // Trigger Hardware Access 
        while(APP_EN);      // WAIT for Hardware to clear APP_EN (Done) 
        
        RAMMODE = 0x00;     // Release Access
    }
}

/**
 * @brief Read data from DGUS Variable Pointer (VP) memory.
 * @details Reads the full 32-bit slot and extracts requested bytes based on alignment.
 * @param addr 16-bit VP Address
 * @param vbuf Pointer to destination buffer
 * @param len Length of data in bytes
 */
void read_dgus_vp(u32 addr, void* vbuf, u16 len)
{
    u8* buf = (u8*)vbuf;
    u32 OS_addr = addr >> 1;
    u8 is_odd = addr & 0x01;

    // 1. Set OS CPU Address Registers
    ADR_H = (u8)(OS_addr >> 16);
    ADR_M = (u8)(OS_addr >> 8);
    ADR_L = (u8)OS_addr;
    ADR_INC = 0x00;

    // 2. Configure for Read
    // RAMMODE: Bit 7 (Request) | Bit 5 (Read Mode=1) | 0x0F (Enable all bytes for reading)
    // 0x80 | 0x20 | 0x0F = 0xAF
    RAMMODE = 0xAF; 

    // 3. Execute Read Command
    APP_EN = 1;         // Trigger Hardware Access 
    while(APP_EN);      // WAIT for Hardware to clear APP_EN (Done) 

    // 4. Extract Data from Registers
    if(!is_odd) // --- EVEN ADDRESS ---
    {
        if(len > 0) *buf++ = DATA3;
        if(len > 1) *buf++ = DATA2;
        if(len > 2) *buf++ = DATA1;
        if(len > 3) *buf++ = DATA0;
    }
    else // --- ODD ADDRESS ---
    {
        if(len > 0) *buf++ = DATA1;
        if(len > 1) *buf++ = DATA0;
    }
    
    RAMMODE = 0x00; // Release Access
}

// --- Interrupt Service Routines & Logic ---

/**
 * @brief Timer 0 Interrupt Service Routine.
 * @details This ISR is triggered every 1ms. It reloads the timer, increments the main system
 *          `Wait_Count`, and decrements the `SysTick` counter for `delay_ms`.
 */
void T0_ISR_PC(void) interrupt 1
{
    TH0 = T1MS >> 8;    
    TL0 = T1MS;       
    Wait_Count++;   
    if(SysTick > 0) SysTick--;
}

/**
 * @brief Calculates the day of the week from a given date.
 * @param year The year (e.g., 24 for 2024).
 * @param month The month (1-12).
 * @param day The day of the month (1-31).
 * @return The day of the week, where 0 is Monday and 6 is Sunday.
 * @details This function implements a variation of Zeller's congruence to determine
 *          the day of the week. It uses a lookup table for month offsets.
 */
u8 RTC_Get_Week(u8 year, u8 month, u8 day)
{
    u16 temp;
    u16 year_real = (u16)year + 2000;
    u8 yearH = year_real / 100;
    u8 yearL = year_real % 100;
    
    if (yearH > 19) yearL += 100;
    
    temp = yearL + yearL / 4;
    temp = temp % 7;
    temp = temp + day + table_week[month - 1];
    
    if (yearL % 4 == 0 && month < 3) temp--;
    
    temp %= 7;
    if(temp == 0) return 6;
    else return temp - 1;
}

/**
 * @brief Updates RTC logic and synchronizes with DGUS Display.
 * @details Called from main loop. Uses non-overlapping addresses 
 * (0x2010, 0x2020, 0x2030) as confirmed working.
 */
void Time_Update(void)
{
    u16 hour_val, min_val, sec_val;

    MUX_SEL |= 0x01; // Feed Watchdog (Reset WDT) [cite: 1810]
    
    if(Second_Updata_Flag == 1)
    {
        real_time.week = RTC_Get_Week(real_time.year, real_time.month, real_time.day);
        
        // Prepare local variables (u16)
        hour_val = real_time.hour;
        min_val = real_time.min;
        sec_val = real_time.sec;

        // --- WRITE TO DGUS VP ---
        // Using strict Even addresses spaced out to ensure no overlap.
        // Function write_dgus_vp will handle the 2-byte write safely.
        
        write_dgus_vp(0x2010, &hour_val, 2); 
        // No delay needed here with the correct 'while(APP_EN)' check in write_dgus_vp, 
        // but keeping small delay is safe for bus stability if desired.
        
        write_dgus_vp(0x2020, &min_val, 2);  
        
        write_dgus_vp(0x2030, &sec_val, 2);  
        
        Second_Updata_Flag = 0;
    }
}

/**
 * @brief Timer 1 Interrupt Service Routine.
 * @details This ISR is triggered every 1ms to drive the software RTC. It increments a counter
 *          and, upon reaching 1000ms (1 second), it updates the `real_time` structure and
 *          sets a flag for the main loop to update the display.
 */
void T1_ISR_PC(void) interrupt 3
{
    TH1 = T1MS >> 8;
    TL1 = T1MS;
    SysTick_RTC++;
    
    // 1 Second Heartbeat
    if(SysTick_RTC >= 1000)
    {
        SysTick_RTC = 0;
        real_time.sec++;
        if(real_time.sec > 59)
        {
            real_time.sec = 0;
            real_time.min++;
            if(real_time.min > 59)
            {
                real_time.min = 0;
                real_time.hour++;
                if(real_time.hour > 23)
                {
                    real_time.hour = 0;
                    real_time.day++;
                    // Basic Month/Day logic (simplified)
                    if(real_time.day > 28) { 
                        // Full calendar logic should go here if needed
                    } 
                }
            }
        }
        Second_Updata_Flag = 1; // Flag to update display in main loop
    }
}

/**
 * @brief Provides a blocking delay for a specified number of milliseconds.
 * @details This function uses a global counter `SysTick` which is decremented by the Timer 0 ISR.
 *          It is a "busy-wait" or "cooperative" delay, not a true sleep.
 * @param n The number of milliseconds to delay.
 */
//void delay_ms(u16 n)
//{
//    SysTick = n;
//    while(SysTick);
//}
