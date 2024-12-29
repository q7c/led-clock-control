#ifndef EFFECTS_H
#define EFFECTS_H

#include <NeoPixelBus.h>

// Константы для 7-сегментного дисплея
const uint8_t LEDS_PER_SEGMENT = 3;    // количество светодиодов в одном сегменте
const uint8_t SEGMENTS_PER_DIGIT = 7;   // количество сегментов в одной цифре
const uint8_t DIGIT_COUNT = 4;          // количество цифр

// Константы для дополнительных дисплеев
const uint16_t DISPLAY3_START = 42;     // Начальный индекс для доп. дисплея (2 диода)
const uint16_t DISPLAY3_LEDS = 2;       // Количество диодов
const uint16_t DISPLAY4_START = 44;     // Начальный индекс для доп. дисплея (1 диод)
const uint16_t DISPLAY4_LEDS = 1;       // Количество диодов

// Порядок сегментов
const uint8_t SEG_G = 0;
const uint8_t SEG_B = 1;
const uint8_t SEG_A = 2;
const uint8_t SEG_F = 3;
const uint8_t SEG_E = 4;
const uint8_t SEG_D = 5;
const uint8_t SEG_C = 6;

// Перечисление для эффектов
enum Effect {
    STATIC,
    RAINBOW,
    BREATHING,
    RUNNING,
    SPARKLE,
    EFFECT_COUNT
};

class Effects {
public:
    Effects(NeoPixelBus<NeoRgbwFeature, NeoEsp8266Uart1Ws2813Method>* strip);
    void update(uint8_t currentHours, uint8_t currentMinutes, bool colonVisible);
    void setEffect(Effect effect) { currentEffect = effect; }
    void setColor(uint8_t r, uint8_t g, uint8_t b) { 
        currentRed = r; 
        currentGreen = g; 
        currentBlue = b; 
    }
    void setBrightness(uint8_t brightness) { maxBrightness = brightness; }
    Effect getCurrentEffect() { return currentEffect; }

private:
    NeoPixelBus<NeoRgbwFeature, NeoEsp8266Uart1Ws2813Method>* strip;
    Effect currentEffect;
    uint8_t currentRed, currentGreen, currentBlue;
    uint8_t maxBrightness;
    uint8_t effectStep;
    uint8_t runningPhase;
    unsigned long lastUpdate;
    unsigned long lastSparkleUpdate;

    void setSegmentColor(uint8_t digit, uint8_t segment, RgbwColor color);
    uint16_t getSegmentStart(uint8_t digit, uint8_t segment);
    void showDigit(uint8_t digit, uint8_t number, RgbwColor color);
    void showAllDigits(RgbwColor color, uint8_t hours, uint8_t minutes, bool colonVisible);
    float limitBrightness(float brightness);

    void staticEffect(uint8_t hours, uint8_t minutes, bool colonVisible);
    void rainbowEffect(uint8_t hours, uint8_t minutes, bool colonVisible);
    void breathingEffect(uint8_t hours, uint8_t minutes, bool colonVisible);
    void runningEffect(uint8_t hours, uint8_t minutes, bool colonVisible);
    void sparkleEffect(uint8_t hours, uint8_t minutes, bool colonVisible);
};

#endif
