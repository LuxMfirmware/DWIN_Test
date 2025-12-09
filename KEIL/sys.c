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
#include <intrins.h>

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
 *          Includes robust defaults for Memory Pointers and Watchdog from factory demo.
 */
void INIT_CPU(void)
{
    EA = 0;           
    RS0 = 0;          
    RS1 = 0;
    
    // --- Clock & Memory Configuration (Factory Defaults) ---
    CKCON = 0x00;           // CPU Clock Control (Default)
    DPC = 0x00;             // Data Pointer Control (Default)
    PAGESEL = 0x01;         // Code Memory Page Select
    D_PAGESEL = 0x02;       // Data Memory Page Select (RAM 0x8000-0xFFFF)

    // --- Peripheral Multiplexing & Watchdog ---
    // MUX_SEL: Bit 6=UART2 En, Bit 5=UART3 En, Bit 1=WDT En, Bit 0=WDT Feed
    // Ensure Watchdog is OFF during init (Factory Default: 0x60 enables UART2/3)
    WDT_OFF(); 

    PORTDRV = 0x01;   // Drive Strength +/- 8mA
    IEN0 = 0x00;      
    IEN1 = 0x00;
    IEN2 = 0x00;
    IP0 = 0x00;       
    IP1 = 0x00;
    
    // Initialize Ports to Input Mode (High Impedance/Weak Pull-up equivalent)
    P0 = 0xFF; P1 = 0xFF; P2 = 0xFF; P3 = 0xFF;
    
    // Configure Output Modes (1=Push-Pull) [cite: 1986]
    // Note: P0.4 (UART2 TX) set to Push-Pull (0x10). 
    // Specific project pins handled in PORT_Init().
    P0MDOUT = 0x10; 
    P1MDOUT = 0x00; 
    P2MDOUT = 0x00; 
    P3MDOUT = 0x00;

    // --- UART0 (Standard 8051 UART / UART2 in DWIN) Configuration ---
    // 115200 8N1 @ ~206MHz
    ADCON = 0x80;     
    SCON0 = 0x50;     
    SREL0H = 0x03;    
    SREL0L = 0xE4;    
    
    // --- UART3 Configuration (Optional/Factory Default) ---
    /*
    SCON1 = 0x50;
    SREL1H = 0x03; SREL1L = 0xC8;
    */

    // --- UART4 Configuration ---
    SCON2T = 0x80;    
    SCON2R = 0x80;    
    BODE2_DIV_H = 0x00; 
    BODE2_DIV_L = 0xE0;

    // --- UART5 Configuration ---
    // (See UART5_Init in uart.c for active configuration)
    
    // Timer Configuration
    TMOD = 0x11;      
    TH0 = 0x00; TL0 = 0x00; TR0 = 0x00; 
    TH1 = 0x00; TL1 = 0x00; TR1 = 0x00; 
    TCON = 0x05; // External Interrupt Edge Trigger (Factory Default)
    
    // Timer 2 Configuration (16-bit Auto-reload) [cite: 1925]
    T2CON = 0x70; 
    TH2 = 0x00; TL2 = 0x00; 
    TRL2H = 0xBC; TRL2L = 0xCD; // 1ms Reload Value
}

/**
 * @brief Initialize GPIO Port Directions.
 * @details Configures specific pins on P0, P1, and P2 as push-pull outputs.
 */
void PORT_Init(void)
{
    // Specific GPIO setup for the project
    P0MDOUT |= 0x02; // P0.1 (RS485 EN)
    
    // P1: Set to Push-Pull for LED/Counter demo
    // We configure ALL P1 pins as Output to ensure the main loop counter works on all 8 bits.
    // Note: If AD1 is physically P1.1, reading it while driving it as Output might return the Output state.
    // However, per user request, we treat them as separate/independent.
    P1MDOUT = 0xFF; 
    // P1 |= 0xEA; // Removed Input High-Z forcing

    // P2: Set P2.0 to Push-Pull for PWM (1kHz)
    P2MDOUT |= 0x03; // P2.0 (PWM) + P2.1 (Original)
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

/**
 * @brief Initialize Timer 2.
 * @details Configures Timer 2 for 500us interrupts to generate 1kHz PWM on P2.0.
 *          500us High + 500us Low = 1ms Period = 1kHz.
 */
void T2_Init(void)
{
    T2CON = 0x70;       // 16-bit Auto-reload
    TH2 = 0x00;
    TL2 = 0x00;
    
    // Reload value for 500us (17.2032 MHz clock)
    // 65536 - (17203200 * 0.0005) = 56934 = 0xDE66
    TRL2H = 0xDE;       
    TRL2L = 0x66;       
    
    IEN0 |= 0x20;       // Enable Timer 2 Interrupt (ET2)
    TR2 = 1;            // Start Timer 2
}

/**
 * @brief Timer 2 Interrupt Service Routine.
 * @details Toggles P2.0 every 500us to create a 1kHz square wave (50% Duty Cycle).
 */
void T2_ISR_PC(void) interrupt 5
{
    TF2 = 0;        // Clear Overflow Flag (Hardware should do this in auto-reload, but safe to ensure)
    P2_0 = !P2_0;   // Toggle P2.0
}

// =============================================================================
//  UNIVERSAL DGUS MEMORY ACCESS FUNCTIONS (OPTIMIZED)
// =============================================================================

/**
 * @brief Write data to DGUS Variable Pointer (VP) memory.
 * @details Optimized for DWIN T5L. Handles atomic 32-bit accesses, odd/even alignment,
 *          and supports multi-byte buffers.
 *          CRITICAL: Disables Global Interrupts (EA) during hardware access to prevent corruption.
 * @param addr 16-bit VP Address
 * @param vbuf Pointer to source buffer
 * @param len Length of data in bytes
 */
void write_dgus_vp(u32 addr, void* vbuf, u16 len)
{
    u8* buf = (u8*)vbuf;
    u32 OS_addr = addr >> 1;
    u8 is_odd = addr & 0x01;
    u8 mask;
    
    EA = 0; // Disable Interrupts for Atomic Access

    // 1. Set Initial Address
    ADR_H = (u8)(OS_addr >> 16);
    ADR_M = (u8)(OS_addr >> 8);
    ADR_L = (u8)OS_addr;
    ADR_INC = 0x01; // Enable Auto-Increment for bulk writes

    // 2. Handle Start Alignment (Odd Address Case)
    if(is_odd && len > 0)
    {
        // For Odd Address, we write to the Lower Half (DATA1/DATA0)
        // We temporarily Disable Auto-Increment to stay on the current word for this partial write
        ADR_INC = 0x00; 
        
        mask = 0x00;
        if(len > 0) { DATA1 = *buf++; mask |= 0x02; len--; }
        if(len > 0) { DATA0 = *buf++; mask |= 0x01; len--; }
        
        if(mask)
        {
            RAMMODE = 0x80 | mask; // Write Request + Byte Enables
            APP_EN = 1; while(APP_EN); // Trigger & Wait
        }

        // Move to next OS Word (Even) manually since we disabled Auto-Inc
        // Or just re-enable Auto-Inc and dummy write? No, cleaner to just increment OS_addr.
        // Actually, simpler: Re-enable Auto-Inc for the main loop.
        // Since we wrote to the "Lower" half of the current address, the next write MUST be to the "Next" address.
        // We need to manually increment the hardware address registers for the loop.
        // (OS_addr + 1)
        OS_addr++;
        ADR_H = (u8)(OS_addr >> 16);
        ADR_M = (u8)(OS_addr >> 8);
        ADR_L = (u8)OS_addr;
        ADR_INC = 0x01; 
    }

    // 3. Main Loop - Write Full Words (4 Bytes)
    while(len >= 4)
    {
        // Optimize: Use Full Write (0x8F) for speed
        RAMMODE = 0x8F; 
        DATA3 = *buf++;
        DATA2 = *buf++;
        DATA1 = *buf++;
        DATA0 = *buf++;
        APP_EN = 1; while(APP_EN);
        len -= 4;
    }

    // 4. Handle Remaining Bytes (1-3 bytes)
    if(len > 0)
    {
        // Auto-Increment is ON from loop. The hardware address is pointing to the NEXT word.
        // We just load data and mask correctly.
        mask = 0x00;
        if(len > 0) { DATA3 = *buf++; mask |= 0x08; }
        if(len > 1) { DATA2 = *buf++; mask |= 0x04; }
        if(len > 2) { DATA1 = *buf++; mask |= 0x02; }
        
        RAMMODE = 0x80 | mask;
        APP_EN = 1; while(APP_EN);
    }

    RAMMODE = 0x00; // Release Access
    EA = 1;         // Restore Interrupts
}

/**
 * @brief Read data from DGUS Variable Pointer (VP) memory.
 * @details Handles atomic 32-bit accesses, odd/even alignment, and supports multi-byte buffers.
 *          CRITICAL: Disables Global Interrupts (EA) during hardware access.
 * @param addr 16-bit VP Address
 * @param vbuf Pointer to destination buffer
 * @param len Length of data in bytes
 */
void read_dgus_vp(u32 addr, void* vbuf, u16 len)
{
    u8* buf = (u8*)vbuf;
    u32 OS_addr = addr >> 1;
    u8 is_odd = addr & 0x01;
    
    EA = 0; // Disable Interrupts

    // 1. Set Initial Address
    ADR_H = (u8)(OS_addr >> 16);
    ADR_M = (u8)(OS_addr >> 8);
    ADR_L = (u8)OS_addr;
    ADR_INC = 0x01; // Enable Auto-Increment

    // 2. Handle Start Alignment (Odd Address)
    if(is_odd && len > 0)
    {
        ADR_INC = 0x00; // Disable Auto-Inc for partial read
        
        // Read Mode
        RAMMODE = 0xAF; 
        APP_EN = 1; while(APP_EN);

        if(len > 0) { *buf++ = DATA1; len--; }
        if(len > 0) { *buf++ = DATA0; len--; }
        
        // Move to next OS Word
        OS_addr++;
        ADR_H = (u8)(OS_addr >> 16);
        ADR_M = (u8)(OS_addr >> 8);
        ADR_L = (u8)OS_addr;
        ADR_INC = 0x01;
    }

    // 3. Main Loop - Read Full Words (4 Bytes)
    while(len >= 4)
    {
        RAMMODE = 0xAF;
        APP_EN = 1; while(APP_EN);
        
        *buf++ = DATA3;
        *buf++ = DATA2;
        *buf++ = DATA1;
        *buf++ = DATA0;
        len -= 4;
    }

    // 4. Handle Remaining Bytes
    if(len > 0)
    {
        RAMMODE = 0xAF;
        APP_EN = 1; while(APP_EN);

        if(len > 0) *buf++ = DATA3;
        if(len > 1) *buf++ = DATA2;
        if(len > 2) *buf++ = DATA1;
    }

    RAMMODE = 0x00;
    EA = 1; // Restore Interrupts
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
void delay_ms(u16 n)
{
    SysTick = n;
    while(SysTick);
}
/**
 * @brief Upisuje cijeli 16.icl fajl (256KB) koristeci podatke sa RAM adrese 0x1000.
 * @details Ponavlja upis istog 32KB RAM buffera 8 puta na uzastopne Flash adrese.
 * Koristi 'delay_ms' da osigura da GUI jezgro stigne obraditi svaki blok.
 */
void Test_Flash_Write_Full_16ICL(void)
{
    u8 i;
    u8 cmd_buffer[12];
    
    // Proracun pocetnog bloka za ID 16:
    // Svaki ID = 256KB. Komanda piše 32KB.
    // 256 / 32 = 8 blokova po ID-u.
    // Pocetni blok = 16 * 8 = 128 (0x0080).
    u16 start_block_addr = 0x0080; 

    // Petlja od 0 do 7 (ukupno 8 blokova)
    for(i = 0; i < 8; i++)
    {
        // --- 1. Priprema komande za VP 0x00AA ---

        // D11:D10 - Enable (0x5A) & Mode (0x02 - Write 32KB)
        cmd_buffer[0] = 0x5A;
        cmd_buffer[1] = 0x02;

        // D9:D8 - Flash Block Address
        // U prvoj iteraciji 0x0080, u drugoj 0x0081, itd...
        cmd_buffer[2] = (u8)((start_block_addr + i) >> 8); 
        cmd_buffer[3] = (u8)(start_block_addr + i);        

        // D7:D6 - Source RAM Address
        // Uvijek uzimamo isti uzorak sa 0x1000 kako ste tražili
        cmd_buffer[4] = 0x10;
        cmd_buffer[5] = 0x00;

        // D5:D4 - Delay/Safety Wait (Parametar za GUI jezgro)
        // Kažemo GUI jezgru da priceka 100ms nakon upisa
        cmd_buffer[6] = 0x00;
        cmd_buffer[7] = 0x64; 

        // D3:D0 - Reserved (0x00)
        cmd_buffer[8] = 0x00;
        cmd_buffer[9] = 0x00;
        cmd_buffer[10] = 0x00;
        cmd_buffer[11] = 0x00;

        // --- 2. Slanje komande ---
        write_dgus_vp(0x00AA, cmd_buffer, 12);

        // --- 3. Obavezno cekanje ---
        // Moramo pauzirati OS jezgro da ne pregazimo komandu dok GUI jezgro piše u Flash.
        // 200ms je sigurna margina (32KB upis traje neko vrijeme).
        delay_ms(200); 
    }
}

