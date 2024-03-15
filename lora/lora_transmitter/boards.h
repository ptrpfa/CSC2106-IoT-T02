#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Ticker.h>
#include "utilities.h"

#ifdef HAS_SDCARD
#include <SD.h>
#include <FS.h>
#endif
SPIClass SDSPI(HSPI);
#if  defined(HAS_DISPLAY) && !defined(EDP_DISPLAY)
#include <U8g2lib.h>

#ifndef DISPLAY_MODEL
#define DISPLAY_MODEL U8G2_SSD1306_128X64_NONAME_F_HW_I2C
#endif

DISPLAY_MODEL *u8g2 = nullptr;
#elif defined(EDP_DISPLAY)
#include "Adafruit_GFX.h"
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <GxEPD.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxDEPG0213BN/GxDEPG0213BN.h> // 2.13" b/w  form DKE GROUP

#include GxEPD_BitmapExamples
GxIO_Class io(SDSPI,  EDP_CS_PIN, EDP_DC_PIN,  EDP_RSET_PIN);
GxEPD_Class display(io, EDP_RSET_PIN, EDP_BUSY_PIN);
#endif

Ticker ledTicker;
#if defined(LILYGO_TBeam_V1_X)
#include <axp20x.h>
AXP20X_Class PMU;

bool initPMU()
{
    if (PMU.begin(Wire, AXP192_SLAVE_ADDRESS) == AXP_FAIL) {
        return false;
    }
    /*
     * The charging indicator can be turned on or off
     * * * */
    // PMU.setChgLEDMode(LED_BLINK_4HZ);

    /*
    * The default ESP32 power supply has been turned on,
    * no need to set, please do not set it, if it is turned off,
    * it will not be able to program
    *
    *   PMU.setDCDC3Voltage(3300);
    *   PMU.setPowerOutPut(AXP192_DCDC3, AXP202_ON);
    *
    * * * */

    /*
     *   Turn off unused power sources to save power
     * **/

    PMU.setPowerOutPut(AXP192_DCDC1, AXP202_OFF);
    PMU.setPowerOutPut(AXP192_DCDC2, AXP202_OFF);
    PMU.setPowerOutPut(AXP192_LDO2, AXP202_OFF);
    PMU.setPowerOutPut(AXP192_LDO3, AXP202_OFF);
    PMU.setPowerOutPut(AXP192_EXTEN, AXP202_OFF);

    /*
     * Set the power of LoRa and GPS module to 3.3V
     **/
    PMU.setLDO2Voltage(3300);   //LoRa VDD
    PMU.setLDO3Voltage(3300);   //GPS  VDD
    PMU.setDCDC1Voltage(3300);  //3.3V Pin next to 21 and 22 is controlled by DCDC1

    PMU.setPowerOutPut(AXP192_DCDC1, AXP202_ON);
    PMU.setPowerOutPut(AXP192_LDO2, AXP202_ON);
    PMU.setPowerOutPut(AXP192_LDO3, AXP202_ON);

    pinMode(PMU_IRQ, INPUT_PULLUP);
    attachInterrupt(PMU_IRQ, [] {
        // pmu_irq = true;
    }, FALLING);

    PMU.adc1Enable(AXP202_VBUS_VOL_ADC1 |
                   AXP202_VBUS_CUR_ADC1 |
                   AXP202_BATT_CUR_ADC1 |
                   AXP202_BATT_VOL_ADC1,
                   AXP202_ON);

    PMU.enableIRQ(AXP202_VBUS_REMOVED_IRQ |
                  AXP202_VBUS_CONNECT_IRQ |
                  AXP202_BATT_REMOVED_IRQ |
                  AXP202_BATT_CONNECT_IRQ,
                  AXP202_ON);
    PMU.clearIRQ();

    return true;
}

void disablePeripherals()
{
    PMU.setPowerOutPut(AXP192_DCDC1, AXP202_OFF);
    PMU.setPowerOutPut(AXP192_LDO2, AXP202_OFF);
    PMU.setPowerOutPut(AXP192_LDO3, AXP202_OFF);
}
#else
#define initPMU()
#define disablePeripherals()
#endif




void initBoard()
{
    Serial.begin(115200);
    Serial.println("initBoard");
    SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN);

#if defined(HAS_DISPLAY)
    Wire.begin(I2C_SDA, I2C_SCL);
#endif


#ifdef HAS_GPS
    Serial1.begin(GPS_BAUD_RATE, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
#endif

#if OLED_RST
    pinMode(OLED_RST, OUTPUT);
    digitalWrite(OLED_RST, HIGH); delay(20);
    digitalWrite(OLED_RST, LOW);  delay(20);
    digitalWrite(OLED_RST, HIGH); delay(20);
#endif

    initPMU();


#ifdef BOARD_LED
    /*
    * T-BeamV1.0, V1.1 LED defaults to low level as trun on,
    * so it needs to be forced to pull up
    * * * * */
#if LED_ON == LOW
    gpio_hold_dis(GPIO_NUM_4);
#endif
    pinMode(BOARD_LED, OUTPUT);
    ledTicker.attach_ms(500, []() {
        static bool level;
        digitalWrite(BOARD_LED, level);
        level = !level;
    });
#endif


#ifdef HAS_DISPLAY
    Wire.beginTransmission(0x3C);
    if (Wire.endTransmission() == 0) {
        Serial.println("Started OLED");
        u8g2 = new U8G2_SSD1306_128X64_NONAME_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE);
        u8g2->begin();
        u8g2->clearBuffer();
        u8g2->setFlipMode(0);
        u8g2->setFontMode(1); // Transparent
        u8g2->setDrawColor(1);
        u8g2->setFontDirection(0);
        u8g2->firstPage();
        do {
            u8g2->setFont(u8g2_font_inb19_mr);
            u8g2->drawStr(0, 30, "LilyGo");
            u8g2->drawHLine(2, 35, 47);
            u8g2->drawHLine(3, 36, 47);
            u8g2->drawVLine(45, 32, 12);
            u8g2->drawVLine(46, 33, 12);
            u8g2->setFont(u8g2_font_inb19_mf);
            u8g2->drawStr(58, 60, "LoRa");
        } while ( u8g2->nextPage() );
        u8g2->sendBuffer();
        u8g2->setFont(u8g2_font_fur11_tf);
        delay(3000);
    }
#endif



#ifdef HAS_SDCARD

#ifdef HAS_DISPLAY
    if (u8g2) {
        u8g2->setFont(u8g2_font_ncenB08_tr);
    }
#endif
    pinMode(SDCARD_MISO, INPUT_PULLUP);
    SDSPI.begin(SDCARD_SCLK, SDCARD_MISO, SDCARD_MOSI, SDCARD_CS);
#ifdef EDP_DISPLAY
    display.init();
    display.setTextColor(GxEPD_BLACK);
    delay(10);
    display.setRotation(0);
    delay(10);
    display.fillScreen(GxEPD_WHITE);
    delay(10);
    display.drawExampleBitmap(BitmapExample1, 0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, GxEPD_BLACK);
    display.update();
    Serial.println("EDP_MODE");
#endif
#ifdef HAS_DISPLAY
    if (u8g2) {
        u8g2->clearBuffer();
    }
#endif
    if (!SD.begin(SDCARD_CS, SDSPI)) {

        Serial.println("setupSDCard FAIL");
#ifdef HAS_DISPLAY
        if (u8g2) {
            do {
                u8g2->setCursor(0, 16);
                u8g2->println( "SDCard FAILED");;
            } while ( u8g2->nextPage() );
        }
#endif
    } else {
        uint32_t cardSize = SD.cardSize() / (1024 * 1024);
        Serial.print("setupSDCard PASS . SIZE = ");
        Serial.print(cardSize / 1024.0);
        Serial.println(" GB");
#ifdef HAS_DISPLAY 
        if (u8g2) {
            do {
                u8g2->setCursor(0, 16);
                u8g2->print( "SDCard:");;
                u8g2->print(cardSize / 1024.0);;
                u8g2->println(" GB");;
            } while ( u8g2->nextPage() );
        }
#endif        
    }
#ifdef HAS_DISPLAY     
    if (u8g2) {
        u8g2->sendBuffer();
    }
#endif
    delay(3000);
#endif
}


