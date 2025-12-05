/**
 * @file DWIN_PERIPHERALS.C
 * @brief Wrapper funkcije za kontrolu DWIN T5L periferija (PWM, ADC, LED, RTC).
 * @details Koristi implementacije DGUS VP pristupa (read_dgus_vp/write_dgus_vp) iz sys.c/sys.h.
 * Svi upisi i citanja se vrse u 'Word' (u16) formatu.
 */

#include "sys.h" // Ukljucuje i T5LOS8051.h
#include "DWIN_GUI_VP.H"

// =========================================================================
// 1. PWM FUNKCIJE (Pulse Width Modulation)
// =========================================================================

/**
 * @brief Konfiguriše frekvenciju i preciznost PWM0.
 * @details Koristi VP_PWM0_SET (0x0086). Word D3: 0x5A za enablovanje postavljanja.
 * @param div_coeff: Frekventni koeficijent podele (Frequency Division Coefficient), D2.
 * @param precision: Preciznost / Gornja granica brojanja (D1:D0).
 * @return u8 (0 - OK)
 */
u8 PWM0_Set_Config(u8 div_coeff, u16 precision) {
    u8 datas[4]; // Koristimo u8 niz (bajtove), ne u16!

    // Tačno kako si napisao:
    datas[0] = 0x5A;                    // D3: Enable byte
    datas[1] = div_coeff;               // D2: Frequency division (npr. 0x01)
    datas[2] = (u8)(precision >> 8);    // D1: Precision High Byte (npr. 0x20)
    datas[3] = (u8)(precision);         // D0: Precision Low Byte  (npr. 0x42)

    // Pišemo 4 bajta na adresu 0x86
    write_dgus_vp(VP_PWM0_SET, datas, 4);

    return 0;
}
/**
 * @brief Postavlja radni ciklus (Duty Cycle) za PWM0.
 * @details Koristi VP_PWM0_OUT (0x0092) za postavljanje širine visokog nivoa.
 * @param duty_value: Vrednost širine visokog nivoa (0x0000 do Max Precision).
 * @return u8 (0 - OK)
 */
u8 PWM0_Set_Duty(u16 duty_value) {
    // I ovdje možemo bajt po bajt da budemo sigurni
    u8 datas[2];
    datas[0] = (u8)(duty_value >> 8);
    datas[1] = (u8)(duty_value);
    
    write_dgus_vp(VP_PWM0_OUT, datas, 2);
    return 0;
}

/**
 * @brief Konfiguriše frekvenciju i preciznost PWM1.
 * @details Koristi VP_PWM1_SET (0x0088). Word D3: 0x5A za enablovanje postavljanja.
 * @param div_coeff: Frekventni koeficijent podele (Frequency Division Coefficient), D2.
 * @param precision: Preciznost / Gornja granica brojanja (D1:D0).
 * @return u8 (0 - OK)
 */
u8 PWM1_Set_Config(u8 div_coeff, u16 precision) {
    u8 datas[4];

    datas[0] = 0x5A;                    // D3: Enable byte
    datas[1] = div_coeff;               // D2: Frequency division
    datas[2] = (u8)(precision >> 8);    // D1: Precision High Byte
    datas[3] = (u8)(precision);         // D0: Precision Low Byte

    // Pišemo 4 bajta na adresu 0x88
    write_dgus_vp(VP_PWM1_SET, datas, 4);

    return 0;
}
/**
 * @brief Postavlja radni ciklus (Duty Cycle) za PWM1.
 * @details Koristi VP_PWM1_OUT (0x0093) za postavljanje širine visokog nivoa.
 * @param duty_value: Vrednost širine visokog nivoa (0x0000 do Max Precision).
 * @return u8 (0 - OK)
 */
u8 PWM1_Set_Duty(u16 duty_value) {
    u8 datas[2];
    datas[0] = (u8)(duty_value >> 8);
    datas[1] = (u8)(duty_value);
    
    write_dgus_vp(VP_PWM1_OUT, datas, 2);
    return 0;
}


// =========================================================================
// 2. ADC FUNKCIJE (Analog-to-Digital Converter)
// =========================================================================

/**
 * @brief Čita trenutnu 16-bitnu sirovu AD vrednost sa specifičnog kanala (AD0-AD7).
 * @details Koristi VP_ADC_INSTANT (0x0032). Svaki kanal zauzima 1 Word (2 bajta).
 * @param channel: Broj ADC kanala (0 do 7).
 * @param raw_value_ptr: Pointer na u16 varijablu gde ce biti smestena ocitana vrednost.
 * @return u8 (0 - OK, 1 - Greska)
 */
u8 ADC_Read_Raw(u8 channel, u16* raw_value_ptr) {
    u32 address;

    if (channel > 7 || raw_value_ptr == NULL) {
        return 1; 
    }
    
    // Adresa = VP_ADC_INSTANT (0x0032) + channel * 2 (Word adresa)
    address = VP_ADC_INSTANT + (u32)channel * 2;
    
    // Citamo 1 Word (2 bajta)
    read_dgus_vp(address, raw_value_ptr, 2);
    
    return 0;
}

// =========================================================================
// 3. LED/BACKLIGHT FUNKCIJE
// =========================================================================

/**
 * @brief Postavlja trenutnu osvetljenost pozadinskog svetla.
 * @details Koristi VP_LED_CONFIG (0x0082). Pise se u Low Byte (D0).
 * @param brightness: Vrednost osvetljenosti (0x00 do 0x64, tj. 0% do 100%).
 * @return u8 (0 - OK)
 */
u8 LED_Set_Brightness_Now(u8 brightness) {

    write_dgus_vp(VP_LED_CONFIG, &brightness, 1);
    return 0;
}

/**
 * @brief TEST KOMANDA: Postavlja osvetljenost pozadinskog svetla na 50% (0x32).
 * @details Koristi VP_LED_CONFIG (0x0082).
 * @return void
 */
void LED_Test_Set_50_Percent(void) {
    u8 brightness = 0x32; 
    LED_Set_Brightness_Now(brightness);
}

// =========================================================================
// 4. RTC FUNKCIJE (Real-Time Clock)
// =========================================================================

/**
 * @brief Pisanje datuma i vremena u RTC DGUS registre.
 * @details Koristi VP_RTC (0x0010). Upisuje 8 bajtova (4 Worda)
 * koji sadrže vreme.
 * @param rtc_data: Struktura koja sadrzi sve komponente vremena/datuma.
 * @return u8 (0 - OK)
 */
u8 RTC_Set_Time_DGUS(rtc_time rtc_data) {
    // VP_RTC (0x0010) je 4 Worda (8 bajta)
    // Struktura rtc_time je vec alinjirana (8 bajtova)
    // 
    // Format upisa (u bajtovima): Year, Month, Day, Week, Hour, Min, Sec, Res
    // Write 4 Word-a (8 bajtova)
    write_dgus_vp(VP_RTC, &rtc_data, 8); 
    return 0;
}

/**
 * @brief Čitanje datuma i vremena iz RTC DGUS registara.
 * @details Koristi VP_RTC (0x0010). Čita 8 bajtova (4 Worda).
 * @param rtc_data_ptr: Pointer na strukturu gde ce biti smesteni podaci.
 * @return u8 (0 - OK)
 */
u8 RTC_Read_Time_DGUS(rtc_time* rtc_data_ptr) {
    // Citamo 4 Word-a (8 bajtova)
    read_dgus_vp(VP_RTC, rtc_data_ptr, 8);
    return 0;
}
