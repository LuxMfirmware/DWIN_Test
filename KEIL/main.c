/**
 * @file main.c
 * @brief Main application file for the DWIN T5L Demo Project.
 * @details This file contains the main entry point and the primary control loop.
 *          It handles initialization of CPU, timers, UART, and RTC.
 *          The main loop processes time updates, sends keep-alive messages via UART,
 *          handles received UART data to update display variables, and processes
 *          simulated button inputs from the display.
 */

#include "sys.h"
#include "uart.h"

extern u8 ADC_Read_Raw(u8 channel, u16* raw_value_ptr);
extern u8 LED_Set_Brightness_Now(u8 brightness);

// Global variables
/** @brief Counter variable incremented by button press. */
u16 my_variable = 0;
/** @brief Variable to store the button state read from DGUS VP. */
u16 button_val = 0;
/** @brief Timestamp for the last keep-alive message sent. */
u16 last_keep_alive = 0;
/** @brief Timestamp for P1 update. */
u16 last_p1_update = 0;
/** @brief Counter for Port 1. */
u8 p1_cnt = 0;
/** @brief ADC Value storage */
u16 adc0_raw_val = 0;
u32 adc0_mv_val = 0; // Koristimo 32-bit za racunicu da izbegnemo overflow


void Update_GUI_RTC(void)
{
    u8 rtc_buffer[8];
   

    // Priprema podataka za VP 0x0010 (Sistemski RTC prikaz)
    // Format prema dokumentaciji (strana 50): 
    // Y M D W H M S (7 bajtova + 1 dummy)
    
    rtc_buffer[0] = 0x19;   // Godina (npr. 23 za 2023)
    rtc_buffer[1] = 0x0B;  // Mjesec
    rtc_buffer[2] = 0x09;    // Dan
    rtc_buffer[3] = 0x02;   // Sedmica (0-6)
    rtc_buffer[4] = 0x10;   // Sat
    rtc_buffer[5] = 0x0D;    // Minuta
    rtc_buffer[6] = 0x00;    // Sekunda
    rtc_buffer[7] = 0x00;             // Nije definisano (dummy)

    // DIREKTAN upis na 0x0010
    // Ovo radi samo kad NEMA hardverskog RTC-a koji bi se takmicio sa vama
    write_dgus_vp(0x0010, rtc_buffer, 8); // Pišemo 4 word-a (8 bajtova)

}

// Funkcija za "Samouništenje" (Flash Overwrite Test)
void Self_Destruct_Test(void)
{
    // Staticka varijabla da osigura da se ovo desi samo jednom
    static bit is_triggered = 0;
    
    // Bufferi za komande
    u8 update_cmd[4];
    u8 reset_cmd[4];

    // Provjera tajmera: Da li je prošlo 20.000 ms (20 sekundi)?
    // Wait_Count se inkrementira u sys.c svakih 1ms
    if (!is_triggered && Wait_Count >= 20000)
    {
        is_triggered = 1; // Blokiraj ponovno izvršavanje

        // --- KORAK 1: Priprema komande za Update Koda (VP 0x06) ---
        // D3 = 0x5A: Enable / Trigger
        // D2 = 0xA5: Mode 0xA5 (Update User 8051 Code, 64KB block)
        // D1 = 0x10: Source Address High Byte (VP 0x1000)
        // D0 = 0x00: Source Address Low Byte
        update_cmd[0] = 0x5A;
        update_cmd[1] = 0xA5;
        update_cmd[2] = 0x10; 
        update_cmd[3] = 0x00;

        // Opcionalno: Pošalji poruku na UART da znamo da pocinje kraj
        // UART5_SendStr("Bye Bye! Flashing garbage...\r\n", 28);

        // --- KORAK 2: Izvrši Flash Update ---
        // U ovom trenutku hardver pauzira CPU, briše Flash i upisuje 
        // sadržaj sa adrese RAM 0x1000 u Code Flash.
        write_dgus_vp(0x0006, update_cmd, 4);

        // --- KORAK 3: Sigurnosno cekanje ---
        // Iako je CPU pauziran hardverski, dodajemo delay da budemo sigurni
        // da se ništa ne desi prije nego što je Flash stabilan.
        delay_ms(1000);

        // --- KORAK 4: Sistemski Reset (VP 0x04) ---
        // D3-D0 = 0x55, 0xAA, 0x5A, 0xA5
        reset_cmd[0] = 0x55;
        reset_cmd[1] = 0xAA;
        reset_cmd[2] = 0x5A;
        reset_cmd[3] = 0xA5;
        
        write_dgus_vp(0x0004, reset_cmd, 4);
        
        // Nakon ovoga, uredaj se resetuje. 
        // Hardver kopira NOVI (vjerovatno neispravan) sadržaj iz Flasha u RAM.
        // Uredaj se više nece upaliti kako treba.
    }
}
// Strukura za 0x0084 komandu (2 Worda = 4 Bajta)
typedef struct {
    u16 enable_mode; // 0x5A01
    u16 pic_id;      // 0x0000 ili 0x0001
} pic_set_cmd;

void Test_Image_Switch(void)
{
    // Staticke varijable pamte stanje izmedu poziva funkcije
    static u16 last_img_time = 0; // Tajmer za sliku (5s)
    static u16 last_dnd_time = 0; // Tajmer za DND (400ms)
    static u16 last_hmd_time = 0; // Tajmer za HMD (900ms)
    
    static u16 current_image_id = 0; // Trenutna slika (0 ili 1)
    static u16 val_dnd = 0;          // Vrijednost za VP 0x1030
    static u16 val_hmd = 0;          // Vrijednost za VP 0x1040

    pic_set_cmd command;

    // --- 1. Logika za promjenu slike (Svakih 5000ms) ---
    // Koristimo Wait_Count koji se inkrementira u sys.c (T0_ISR)
    if((u16)(Wait_Count - last_img_time) >= 5000)
    {
        last_img_time = Wait_Count; // Resetuj tajmer za sliku

        // Prebaci ID slike: 0 -> 1 -> 0
        if(current_image_id == 0) current_image_id = 1;
        else current_image_id = 0;

        // Pošalji komandu za promjenu slike (VP 0x0084)
        command.enable_mode = 0x5A01; 
        command.pic_id = current_image_id;
        write_dgus_vp(0x0084, &command, 4);
    }

    // --- 2. Logika za ikonice (Samo ako je slika 00 aktivna) ---
    if(current_image_id == 0)
    {
        // A) iconDND na VP 0x1030 (Svakih 400ms)
        if((u16)(Wait_Count - last_dnd_time) >= 400)
        {
            last_dnd_time = Wait_Count;
            
            // Toggle vrijednost 0 <-> 1
            if(val_dnd == 0) val_dnd = 1; else val_dnd = 0;
            
            // Upis na VP 0x1030 (DND Icon)
            write_dgus_vp(0x1030, &val_dnd, 2);
        }

        // B) iconHMD na VP 0x1040 (Svakih 900ms)
        if((u16)(Wait_Count - last_hmd_time) >= 900)
        {
            last_hmd_time = Wait_Count;
            
            // Toggle vrijednost 0 <-> 1
            if(val_hmd == 0) val_hmd = 1; else val_hmd = 0;
            
            // Upis na VP 0x1040 (HMD Icon)
            write_dgus_vp(0x1040, &val_hmd, 2);
        }
    }
}
/**
 * @brief Main Entry Point
 * @details Initializes system peripherals and enters the infinite control loop.
 */
void main(void)
{
    // --- Initialization Phase ---
    INIT_CPU();     // Initialize CPU core registers and GPIO directions
    T0_Init();      // Initialize Timer 0 (System Tick)
    T1_Init();      // Initialize Timer 1 (RTC Tick)
    T2_Init();      // Initialize Timer 2 (1kHz PWM on P2.0)
    UART5_Init();   // Initialize UART5 for communication
    RTC_Init();     // Initialize Real Time Clock
    PORT_Init();    // Initialize Port IO specific configurations
    
    
    // Send startup message
    UART5_SendStr("Demo Started\r\n", 14);
    Update_GUI_RTC();
    // Initialize the keep-alive timer
    last_keep_alive = Wait_Count;
    last_p1_update = Wait_Count;
    // --- Main Control Loop ---
    while(1)
    {
        // Update RTC and synchronize with Display VP if needed
        Time_Update();
        
        Test_Image_Switch();
        //Self_Destruct_Test();
        // --- P1 Update (100ms) ---
        if((u16)(Wait_Count - last_p1_update) >= 100)
        {
            last_p1_update = Wait_Count;
            P1 = p1_cnt;
            p1_cnt++;
            LED_Set_Brightness_Now(p1_cnt/3);
            
        }

        // --- Keep Alive Mechanism ---
        // Check if 5 seconds (5000 ticks) have passed since the last keep-alive
        if((u16)(Wait_Count - last_keep_alive) >= 5000)
        {
            last_keep_alive = Wait_Count; // Reset timestamp

            // --- ISPIS TACNOG VREMENA ---
            UART5_SendStr("Time: ", 6);
            UART5_Sendbyte((real_time.hour / 10) + '0');
            UART5_Sendbyte((real_time.hour % 10) + '0');
            UART5_Sendbyte(':');
            UART5_Sendbyte((real_time.min / 10) + '0');
            UART5_Sendbyte((real_time.min % 10) + '0');
            UART5_Sendbyte(':');
            UART5_Sendbyte((real_time.sec / 10) + '0');
            UART5_Sendbyte((real_time.sec % 10) + '0');
            UART5_SendStr(" | ", 3);
            
            // --- 2. NOVO: CITANJE ADC0 I SLANJE ---
            // Citanje kanala 0 (ADC0)
            if(ADC_Read_Raw(1, &adc0_raw_val) == 0) // Promenjeno na kanal 1
            {
                UART5_SendStr(" | ADC1 Raw: ", 13);
                
                // Ispis sirove vrednosti (0 - 65535)
                // Posto je vrednost > 4095, ovo je sigurno 16-bitni podatak
                UART5_Sendbyte((adc0_raw_val / 10000) + '0');
                UART5_Sendbyte(((adc0_raw_val % 10000) / 1000) + '0');
                UART5_Sendbyte(((adc0_raw_val % 1000) / 100) + '0');
                UART5_Sendbyte(((adc0_raw_val % 100) / 10) + '0');
                UART5_Sendbyte((adc0_raw_val % 10) + '0');
                
                // --- ISPRAVKA FORMULE ---
                // Buduci da dobijate vrednost 7834 (sto je vece od 4095), hardver radi u 16-bitnom rezimu.
                // Moramo deliti sa 65535 umesto sa 4095.
                // Formula: Napon(mV) = (Raw * 3300) / 65535
                
                adc0_mv_val = ((u32)adc0_raw_val * 3300) / 65535;
                
                UART5_SendStr(" (", 2);
                // Ispis napona (do 3300 mV, tj. 3.30V)
                UART5_Sendbyte(((adc0_mv_val / 1000) % 10) + '0');
                UART5_Sendbyte('.');
                UART5_Sendbyte(((adc0_mv_val / 100) % 10) + '0');
                UART5_Sendbyte(((adc0_mv_val / 10) % 10) + '0');
                UART5_Sendbyte('V');
                UART5_Sendbyte(')');
            }
            else
            {
                UART5_SendStr(" | ADC1 Error", 13);
            }
            UART5_SendStr("\r\n", 2);
        }

        // --- UART RX Handling ---
        // Check if there is new data in the UART receive buffer
        if(Rx_Head != Rx_Tail)
        {
            // Read one byte from the circular buffer
            u8 c = Rx_Buffer[Rx_Tail];
            Rx_Tail = (Rx_Tail + 1) & 0x1F; // Increment tail with wrap-around

            // Process Numeric Commands
            // Check if the received character is a digit '0'-'9'
            if(c >= '0' && c <= '9')
            {
                u16 val = (u16)(c - '0'); // Convert ASCII to integer
                // Write the value to DGUS Variable Pointer (VP) address 0x1100
                write_dgus_vp(0x2040, &val, 2);

                // Echo back valid input
                UART5_SendStr("written number: ", 16);
                UART5_Sendbyte(c);
                UART5_SendStr("\r\n", 2);
            }
            else
            {
                // Echo back invalid input
                UART5_SendStr("out of limit\r\n", 14);
            }
        }

        // --- Button Handling ---
        // Read the status of the button at VP address 0x1200.
        // The display is expected to write '1' to this address when the button is pressed.
        read_dgus_vp(0x1200, &button_val, 2);

        if(button_val == 1)
        {
            my_variable++; // Increment the counter

            // Send debug information via UART
            UART5_SendStr("Variable updated [new value:", 28);

            // Convert the 3-digit counter value to ASCII and send character by character
            UART5_Sendbyte(((my_variable / 100) % 10) + '0'); // Hundreds digit
            UART5_Sendbyte(((my_variable / 10) % 10) + '0');  // Tens digit
            UART5_Sendbyte((my_variable % 10) + '0');         // Units digit

            UART5_SendStr("]\r\n", 3); // End of line


            button_val = 0;
            write_dgus_vp(0x1200, &button_val, 2);
            // FORCE RESET LOOP: Keep writing 0 until the hardware acknowledges it
            // This prevents the "machine gun" effect if a single write is missed.
//            do
//            {
//                write_dgus_vp(0x1200, &button_val, 2);
//                delay_ms(10); // Small wait for hardware
//                read_dgus_vp(0x1200, &button_val, 2); // Verify
//            } while(button_val == 1); // Retry if still 1

            // Simple cooldown delay to prevent multiple triggers during a single press
//            delay_ms(200);
        }
    }
}
