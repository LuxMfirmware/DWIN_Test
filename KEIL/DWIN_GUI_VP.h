/**
 * @file DWIN_GUI_VP.H
 * @brief DWIN T5L GUI System Variable (VP) Address Definitions.
 * @details Based on T5L_DGUSII Application Development Guide (Chapter 5, 
 * addresses 0x0000 - 0x0FFF). These addresses are fixed within the DGUS RAM.
 */

#ifndef __DWIN_GUI_VP_H__
#define __DWIN_GUI_VP_H__


// =========================================================================
// 1. DWIN GUI SYSTEM VARIABLE ADDRESSES (VP) (0x0000 - 0x0FFF)
// =========================================================================

// --- Fiksne adrese za Hardver/Sistem ---

// Sistemska kontrola / Reset
#define VP_SYS_RESET                0x0004  // System Reset (W) [cite: 1635]
#define VP_OS_UPDATE_CMD            0x0006  // OS Update Command (W) [cite: 1635]
#define VP_NOR_FLASH_RW_CMD         0x0008  // NOR FLASH Read/Write Command (W) [cite: 1635]

// Komunikacija
#define VP_UART2_CONFIG             0x000C  // UART2 Configuration (W) (Baud rate, CRC) [cite: 1640]
#define VP_VERSION_NUMBER           0x000F  // GUI and DWIN OS Version (R) [cite: 1646]
#define VP_RTC                      0x0010  // RTC Date/Time (R/W) [cite: 1646]
#define VP_PIC_NOW                  0x0014  // Current Page ID (R) [cite: 1646]
#define VP_GUI_STATUS               0x0015  // GUI Status (R) [cite: 1646]
#define VP_TP_STATUS                0x0016  // Touch Panel Status/Coordinates (R/W) [cite: 1647]

// Osvetljenje / ADC (Analogni ulazi)
#define VP_LED_NOW                  0x0031  // Backlight Brightness (R) [cite: 1652]
#define VP_ADC_INSTANT              0x0032  // AD0-AD7 Instantaneous value (R) [cite: 1652]

// Sistemska konfiguracija (Preklapa se sa CFG fajlom)
#define VP_LCD_HOR_RES              0x007A  // Horizontal Resolution (R) [cite: 1653]
#define VP_LCD_VER_RES              0x007B  // Vertical Resolution (R) [cite: 1653]
#define VP_SYSTEM_CONFIG            0x0080  // System Config Flags (R/W) [cite: 1654]
#define VP_LED_CONFIG               0x0082  // LED Config (Standby/Brightness) (R/W) [cite: 1660]
#define VP_PIC_SET                  0x0084  // Page Switch/Picture Display (R/W) [cite: 1660]

// PWM Kontrola (Pulsno širinska modulacija)
#define VP_PWM0_SET                 0x0086  // PWM0 Frequency/Precision Setting (R/W) [cite: 1667]
#define VP_PWM1_SET                 0x0088  // PWM1 Frequency/Precision Setting (R/W) [cite: 1670]
#define VP_PWM0_OUT                 0x0092  // PWM0 Duty Cycle (R/W) [cite: 1700]
#define VP_PWM1_OUT                 0x0093  // PWM1 Duty Cycle (R/W) [cite: 1703]

// --- Hardverske periferije i interfejsi ---
#define VP_FSK_INTERFACE_START      0x0100  // FSK Bus Interface Start Address [cite: 2191]
#define VP_CURVE_STATUS_START       0x0300  // Dynamic Curve Status Feedback [cite: 2185]
#define VP_CURVE_CONFIG_START       0x0380  // Dynamic Curve Read/Config Start [cite: 2148]
#define VP_NETWORK_INTERFACE_START  0x0400  // Network Interface Start Address [cite: 2230]

// =========================================================================
// 2. KORISNIČKE VARIJABLE (VP)
// =========================================================================

// Preporučeni početak za varijable ako se ne koriste krive
#define VP_USER_START_NO_CURVE      0x1000  // Korisničke VP adrese (124KB) [cite: 576]

// Preporučeni početak za varijable ako se koriste krive (0x1000-0x4FFF je bafer za 8 kanala)
#define VP_USER_START_WITH_CURVE    0x5000  // Korisničke VP adrese (Preporučeno) [cite: 574]

// Takt iz dokumentacije (825.7536 MHz)
#define PWM_BASE_CLOCK 825753600UL

#endif // __DWIN_GUI_VP_H__
