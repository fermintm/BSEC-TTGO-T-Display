#define FS_NO_GLOBALS
#include <FS.h>

#include "Arduino.h"
#include <SPI.h>
#include <TFT_eSPI.h> // Hardware-specific library
#include "bsec.h"
#include <Wire.h>
#include "SPIFFS.h"
#include <Pangodream_18650_CL.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "driver/adc.h"

#define GFXFF 1
#define FF18 &FreeSans12pt7b
#define CF_OL24 &Orbitron_Light_24
#define CF_OL32 &Orbitron_Light_32
#define CF_RT24 &Roboto_Thin_24
#define CF_S24  &Satisfy_24
#define CF_Y32  &Yellowtail_32
#define DB_DB10 &Dialog_bold_10
#define DB_DB12 &Dialog_bold_12
#define DB_DB14 &Dialog_bold_14
#define DB_DB16 &Dialog_bold_16
#define DP_DP10 &Dialog_plain_10
#define DP_DP12 &Dialog_plain_12
#define DP_DP14 &Dialog_plain_14
#define DP_DP16 &Dialog_plain_16
#define CF_OM10 &Orbitron_Medium_10
#define CF_OM12 &Orbitron_Medium_12
#define CF_OM16 &Orbitron_Medium_16
#define CF_OM18 &Orbitron_Medium_18

#define TFT_BLACK       0x0000      /*   0,   0,   0 */
#define TFT_NAVY        0x000F      /*   0,   0, 128 */
#define TFT_DARKGREEN   0x03E0      /*   0, 128,   0 */
#define TFT_DARKCYAN    0x03EF      /*   0, 128, 128 */
#define TFT_MAROON      0x400      /* 128,   0,   0 */
#define TFT_PURPLE      0x780F      /* 128,   0, 128 */
#define TFT_OLIVE       0x7BE0      /* 128, 128,   0 */
#define TFT_LIGHTGREY   0xD69A      /* 211, 211, 211 */
#define TFT_LIGHTGREY2  0xC618
#define TFT_GREY        0x5AEB
#define TFT_DARKGREY    0x7BEF      /* 128, 128, 128 */
#define TFT_BLUE        0x001F      /*   0,   0, 255 */
#define TFT_GREEN       0x07E0      /*   0, 255,   0 */
#define TFT_DARKGREEN   0x0120
#define TFT_CYAN        0x07FF      /*   0, 255, 255 */
#define TFT_RED         0xF800      /* 255,   0,   0 */
#define TFT_DARKRED     0x7804
#define TFT_MAGENTA     0xF81F      /* 255,   0, 255 */
#define TFT_YELLOW      0xFFE0      /* 255, 255,   0 */
#define TFT_WHITE       0xFFFF      /* 255, 255, 255 */
#define TFT_ORANGE      0xFDA0      /* 255, 180,   0 */
#define TFT_ORANGE2     0xFCA0
#define TFT_GREENYELLOW 0xB7E0      /* 180, 255,   0 */
#define TFT_GREENYELLOW2 0x5D1C
#define TFT_PINK2       0xFE19      /* 255, 192, 203 */ //Lighter pink, was 0xFC9F 
#define TFT_PINK        0xF97F
#define TFT_BROWN       0x9A60      /* 150,  75,   0 */
#define TFT_BROWN2      0x8200
#define TFT_GOLD        0xFEA0      /* 255, 215,   0 */
#define TFT_GOLD2       0xA508
#define TFT_SILVER      0xC618      /* 192, 192, 192 */
#define TFT_SILVER2     0xA510
#define TFT_SKYBLUE     0x867D      /* 135, 206, 235 */
#define TFT_VIOLET      0x915C      /* 180,  46, 226 */
#define TFT_VIOLET2     0x9199
#define TFT_LIME        0x87E0
#define TFT_LIME2       0x04FF

const int pwmFreq = 5000;
const int pwmResolution = 8;
const int pwmLedChannelTFT = 0;

int backlight[8] = {0, 30, 60, 100, 120, 150, 180, 220 };
byte b = 1;

#define MIN_USB_VOL 4.3
#define ADC_PIN 34
#define CONV_FACTOR 1.81
#define READS 20

Pangodream_18650_CL BL(ADC_PIN, CONV_FACTOR, READS);

Bsec iaqSensor; // Create an object of the class Bsec

#define WIDTH  240
#define HEIGHT 135

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);

void setup() {

  Wire.begin();
  Wire.setClock(400000);

  SPIFFS.begin();

  adc_power_off();
  WiFi.disconnect(true);  // Disconnect from the network
  WiFi.mode(WIFI_OFF);    // Switch WiFi off

  setCpuFrequencyMhz(80);

  pinMode(0, INPUT_PULLUP);
  pinMode(35, INPUT);
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);

  spr.setColorDepth(8);
  spr.createSprite(WIDTH, HEIGHT);

  ledcSetup(pwmLedChannelTFT, pwmFreq, pwmResolution);
  ledcAttachPin(TFT_BL, pwmLedChannelTFT);
  ledcWrite(pwmLedChannelTFT, backlight[b]);

  iaqSensor.begin(BME680_I2C_ADDR_SECONDARY, Wire);

  bsec_virtual_sensor_t sensorList[10] = {
    BSEC_OUTPUT_RAW_TEMPERATURE,
    BSEC_OUTPUT_RAW_PRESSURE,
    BSEC_OUTPUT_RAW_HUMIDITY,
    BSEC_OUTPUT_RAW_GAS,
    BSEC_OUTPUT_IAQ,
    BSEC_OUTPUT_STATIC_IAQ,
    BSEC_OUTPUT_CO2_EQUIVALENT,
    BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
  };

  iaqSensor.updateSubscription(sensorList, 10, BSEC_SAMPLE_RATE_LP);
}

int press1 = 0;
int press2 = 0;

void loop() {

  spr.fillSprite(0);

  iaqSensor.run(); // If new data is available

  float RT = (iaqSensor.rawTemperature);
  float PRE = (iaqSensor.pressure);
  float RH = (iaqSensor.rawHumidity);
  float GAS = (iaqSensor.gasResistance);
  float IAQ = (iaqSensor.iaq);
  float IAQAC = (iaqSensor.iaqAccuracy);
  float TEMP = (iaqSensor.temperature);
  float HUM = (iaqSensor.humidity);
  float STATIAQ = (iaqSensor.staticIaq);
  float CO2EQ = (iaqSensor.co2Equivalent);
  float BREATHEQ = (iaqSensor.breathVocEquivalent);

  spr.drawRoundRect(0, 0, 240, 135, 4, TFT_CYAN);
  spr.drawLine(173, 0, 173, 135, TFT_CYAN);

  spr.setFreeFont(CF_OM12);
  if (BL.getBatteryVolts() >= MIN_USB_VOL) {
    spr.setCursor(190, 118);
    spr.setTextColor(TFT_GREEN, TFT_BLACK);
    spr.print("CHG");
    drawBmp("/battery_05.bmp", 188, 84);
  }

  else {
    int batteryLevel = BL.getBatteryChargeLevel();
    spr.setCursor(186, 116);
    spr.setTextColor(TFT_WHITE, TFT_BLACK);
    spr.print(BL.getBatteryVolts(), 1);
    spr.print(" V");
    spr.setCursor(186, 130);
    spr.print(BL.getBatteryChargeLevel());
    spr.print(" %");
    if (batteryLevel >= 80) {
      drawBmp("/battery_04.bmp", 188, 84);
    } else if (batteryLevel < 80 && batteryLevel >= 50 ) {
      drawBmp("/battery_03.bmp", 188, 84);
    } else if (batteryLevel < 50 && batteryLevel >= 20 ) {
      drawBmp("/battery_02.bmp", 188, 84);
    } else if (batteryLevel < 20 ) {
      drawBmp("/battery_01.bmp", 188, 84);
    }
  }

  spr.setTextFont(2);
  spr.setCursor(7, 5);
  spr.setTextColor(TFT_WHITE, TFT_BLACK);
  spr.print(TEMP, 1); spr.setCursor(60, 5);
  spr.println("TEMPERATURA");
  spr.setCursor(7, 20);
  spr.setTextColor(TFT_WHITE, TFT_BLACK);
  spr.print(HUM, 1); spr.setCursor(60, 20);
  spr.println("HUMEDAD");

  spr.setTextColor(TFT_WHITE, TFT_BLACK);

  spr.setCursor(7, 35);
  spr.print(PRE / 100, 0);
  spr.setCursor(60, 35); spr.println("MILIBAR");

  spr.setCursor(7, 60);

  if (STATIAQ >= 0 && STATIAQ < 50.9) {
    spr.setTextColor(TFT_GREEN, TFT_BLACK);
  }
  if (STATIAQ >= 51 && STATIAQ < 100.9) {
    spr.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
  }
  if (STATIAQ >= 101 && STATIAQ < 150.9) {
    spr.setTextColor(TFT_YELLOW, TFT_BLACK);
  }
  if (STATIAQ >= 151 && STATIAQ < 200.9) {
    spr.setTextColor(TFT_ORANGE, TFT_BLACK);
  }
  if (STATIAQ >= 201 && STATIAQ < 250.9) {
    spr.setTextColor(TFT_RED, TFT_BLACK);
  }
  if (STATIAQ >= 251 && STATIAQ < 350.9) {
    spr.setTextColor(TFT_MAGENTA, TFT_BLACK);
  }
  if (iaqSensor.staticIaq > 351) {
    spr.setTextColor(TFT_BROWN, TFT_BLACK);
  }

  spr.print(STATIAQ, 0);
  spr.setCursor(60, 52);
  spr.println("CALIDAD AIRE");
  spr.setCursor(7, 85);
  spr.setTextColor(TFT_WHITE, TFT_BLACK);
  spr.print(CO2EQ, 0);
  spr.setCursor(60, 85);
  spr.println("PPM CO2 EQUIV.");
  spr.setCursor(7, 100);
  spr.print(BREATHEQ);
  spr.setCursor(60, 100);
  spr.println("PPM VOC EQUIV.");
  spr.setCursor(7, 115);
  spr.print(IAQAC, 0);
  spr.setCursor(60, 115);
  spr.print("PRECISION IAQ ");

  if (STATIAQ >= 0 && STATIAQ < 50.9) {
    spr.fillRoundRect(180, 5, 55, 70, 4, TFT_GREEN);
    //spr.fillRect(112, 168, 206, 25, TFT_BLACK);
    spr.setCursor(60, 68);
    spr.setTextColor(TFT_GREEN);
    spr.print("EXCELENTE");
  }

  if (STATIAQ >= 51 && STATIAQ < 100.9) {
    spr.fillRoundRect(180, 5, 55, 70, 4, TFT_GREENYELLOW);
    //spr.fillRect(112, 168, 206, 25, TFT_BLACK);
    spr.setCursor(60, 68);
    spr.setTextColor(TFT_GREENYELLOW);
    spr.print("BUENA");
  }

  if (STATIAQ >= 101 && STATIAQ < 150.9) {
    spr.fillRoundRect(180, 5, 55, 70, 4, TFT_YELLOW);
    //spr.fillRect(112, 168, 206, 25, TFT_BLACK);
    spr.setCursor(60, 68);
    spr.setTextColor(TFT_YELLOW);
    spr.print("POCO MALA");
  }

  if (STATIAQ >= 151 && STATIAQ < 200.9) {
    spr.fillRoundRect(180, 5, 55, 70, 4, TFT_ORANGE);
    //spr.fillRect(112, 168, 206, 25, TFT_BLACK);
    spr.setCursor(60, 68);
    spr.setTextColor(TFT_ORANGE);
    spr.print("MALA");
  }

  if (STATIAQ >= 201 && STATIAQ < 250.9) {
    spr.fillRoundRect(180, 5, 55, 70, 4, TFT_RED);
    //spr.fillRect(112, 168, 206, 25, TFT_BLACK);
    spr.setCursor(60, 68);
    spr.setTextColor(TFT_RED);
    spr.print("BASTANTE MALA");
  }

  if (STATIAQ >= 251 && STATIAQ < 350.9) {
    spr.fillRoundRect(180, 5, 55, 70, 4, TFT_MAGENTA);
    //spr.fillRect(112, 168, 206, 25, TFT_BLACK);
    spr.setCursor(60, 68);
    spr.setTextColor(TFT_MAGENTA);
    spr.print("MUY MALA");
  }

  if (iaqSensor.staticIaq > 351) {
    spr.fillRoundRect(180, 5, 55, 70, 4, TFT_BROWN);
    //spr.fillRect(112, 168, 206, 25, TFT_BLACK);
    spr.setCursor(60, 68);
    spr.setTextColor(TFT_BROWN);
    spr.print("EXTREMA");
  }

  if (digitalRead(35) == 0) {
    if (press2 == 0)
    { press2 = 1;

      b++;
      if (b >= 8)
        b = 0;

      ledcWrite(pwmLedChannelTFT, backlight[b]);
    }
  } else press2 = 0;

  if (digitalRead(0) == 0) {
    spr.fillSprite(0);
    drawBmp("/STD.bmp", 0, 15);
    spr.setTextSize(2);
    spr.setCursor(105, 52);
    spr.setTextColor(TFT_GREEN);
    spr.print("APAGANDO");
    spr.pushSprite(0, 0);
    delay(5000);
    esp_deep_sleep_start();
  }

  spr.pushSprite(0, 0);
}
