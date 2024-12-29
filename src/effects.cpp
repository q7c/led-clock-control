#include "effects.h"
#include <math.h>

// Массив для маппинга цифр
const uint8_t DIGITS[10][7] = {
    {0,1,1,1,1,1,1}, // 0
    {0,1,0,0,0,0,1}, // 1
    {1,1,1,0,1,1,0}, // 2
    {1,1,1,0,0,1,1}, // 3
    {1,1,0,1,0,0,1}, // 4
    {1,0,1,1,0,1,1}, // 5
    {1,0,1,1,1,1,1}, // 6
    {0,1,1,0,0,0,1}, // 7
    {1,1,1,1,1,1,1}, // 8
    {1,1,1,1,0,1,1}  // 9
};

Effects::Effects(NeoPixelBus<NeoRgbwFeature, NeoEsp8266Uart1Ws2813Method>* strip) 
    : strip(strip), currentEffect(STATIC), effectStep(0), runningPhase(0),
      lastUpdate(0), lastSparkleUpdate(0), maxBrightness(255),
      currentRed(255), currentGreen(0), currentBlue(0) {
}

float Effects::limitBrightness(float brightness) {
    float maxValue = (float)maxBrightness / 255.0f;
    return brightness < maxValue ? brightness : maxValue;
}

uint16_t Effects::getSegmentStart(uint8_t digit, uint8_t segment) {
    if (digit < 2) {
        return (digit * SEGMENTS_PER_DIGIT + segment) * LEDS_PER_SEGMENT;
    } else {
        return ((digit * SEGMENTS_PER_DIGIT + segment) * LEDS_PER_SEGMENT) + 3;
    }
}

void Effects::setSegmentColor(uint8_t digit, uint8_t segment, RgbwColor color) {
    uint16_t start = getSegmentStart(digit, segment);
    for(uint8_t i = 0; i < LEDS_PER_SEGMENT; i++) {
        strip->SetPixelColor(start + i, color);
    }
}

void Effects::showDigit(uint8_t digit, uint8_t number, RgbwColor color) {
    if(number > 9) return;
    
    setSegmentColor(digit, SEG_G, DIGITS[number][0] ? color : RgbwColor(0));
    setSegmentColor(digit, SEG_B, DIGITS[number][1] ? color : RgbwColor(0));
    setSegmentColor(digit, SEG_A, DIGITS[number][2] ? color : RgbwColor(0));
    setSegmentColor(digit, SEG_F, DIGITS[number][3] ? color : RgbwColor(0));
    setSegmentColor(digit, SEG_E, DIGITS[number][4] ? color : RgbwColor(0));
    setSegmentColor(digit, SEG_D, DIGITS[number][5] ? color : RgbwColor(0));
    setSegmentColor(digit, SEG_C, DIGITS[number][6] ? color : RgbwColor(0));
}

void Effects::showAllDigits(RgbwColor color, uint8_t hours, uint8_t minutes, bool colonVisible) {
    showDigit(0, hours / 10, color);
    showDigit(1, hours % 10, color);
    
    if (colonVisible) {
        for(uint8_t i = 0; i < DISPLAY3_LEDS; i++) {
            strip->SetPixelColor(DISPLAY3_START + i, color);
        }
    } else {
        for(uint8_t i = 0; i < DISPLAY3_LEDS; i++) {
            strip->SetPixelColor(DISPLAY3_START + i, RgbwColor(0));
        }
    }
    
    strip->SetPixelColor(DISPLAY4_START, RgbwColor(0));
    
    showDigit(2, minutes / 10, color);
    showDigit(3, minutes % 10, color);
    
    strip->Show();
}

void Effects::update(uint8_t currentHours, uint8_t currentMinutes, bool colonVisible) {
    unsigned long currentMillis = millis();
    
    if (currentMillis - lastUpdate >= 50) {
        lastUpdate = currentMillis;
        
        switch (currentEffect) {
            case STATIC:
                staticEffect(currentHours, currentMinutes, colonVisible);
                break;
            case RAINBOW:
                rainbowEffect(currentHours, currentMinutes, colonVisible);
                break;
            case BREATHING:
                breathingEffect(currentHours, currentMinutes, colonVisible);
                break;
            case RUNNING:
                runningEffect(currentHours, currentMinutes, colonVisible);
                break;
            case SPARKLE:
                sparkleEffect(currentHours, currentMinutes, colonVisible);
                break;
            default:
                staticEffect(currentHours, currentMinutes, colonVisible);
                break;
        }
    }
}

void Effects::staticEffect(uint8_t hours, uint8_t minutes, bool colonVisible) {
    RgbwColor color(
        (currentGreen * maxBrightness) / 255,
        (currentRed * maxBrightness) / 255,
        (currentBlue * maxBrightness) / 255,
        0
    );
    showAllDigits(color, hours, minutes, colonVisible);
}

void Effects::rainbowEffect(uint8_t hours, uint8_t minutes, bool colonVisible) {
    // Обновляем фазу эффекта
    effectStep = (effectStep + 1) % 256;

    // Вычисляем цвет на основе текущей фазы
    uint8_t red = (sin((effectStep * 0.02) + 0) * 127 + 128) * maxBrightness / 255;
    uint8_t green = (sin((effectStep * 0.02) + 2) * 127 + 128) * maxBrightness / 255;
    uint8_t blue = (sin((effectStep * 0.02) + 4) * 127 + 128) * maxBrightness / 255;

    RgbwColor color(red, green, blue, 0); // Устанавливаем цвет без белого канала

    // Отображаем все цифры с цветом радуги
    showAllDigits(color, hours, minutes, colonVisible);
}

void Effects::breathingEffect(uint8_t hours, uint8_t minutes, bool colonVisible) {
    effectStep = (effectStep + 2) % 256;
    float brightness = sin(effectStep * PI / 128) * 0.4 + 0.6;
    brightness = limitBrightness(brightness);
    
    RgbwColor color(
        (currentGreen * maxBrightness * brightness) / 255,
        (currentRed * maxBrightness * brightness) / 255,
        (currentBlue * maxBrightness * brightness) / 255,
        0
    );
    showAllDigits(color, hours, minutes, colonVisible);
}

void Effects::runningEffect(uint8_t hours, uint8_t minutes, bool colonVisible) {
    runningPhase = (runningPhase + 2) % 256;
    
    const float baseBrightness = 0.3f;
    uint8_t activeDisplay = (runningPhase / 64) % 4;
    uint8_t transitionPhase = runningPhase % 64;
    
    RgbwColor baseColor(
        (currentGreen * maxBrightness * baseBrightness) / 255,
        (currentRed * maxBrightness * baseBrightness) / 255,
        (currentBlue * maxBrightness * baseBrightness) / 255,
        0
    );
    
    for(uint8_t i = 0; i < 4; i++) {
        showDigit(i, i < 2 ? 
            (i == 0 ? hours / 10 : hours % 10) :
            (i == 2 ? minutes / 10 : minutes % 10),
            baseColor);
    }
    
    float activePhase = (float)transitionPhase / 63.0f;
    float activeBrightness = baseBrightness + (1.0f - baseBrightness) * sin(activePhase * PI);
    activeBrightness = limitBrightness(activeBrightness);
    
    RgbwColor activeColor(
        (currentGreen * maxBrightness * activeBrightness) / 255,
        (currentRed * maxBrightness * activeBrightness) / 255,
        (currentBlue * maxBrightness * activeBrightness) / 255,
        0
    );
    
    showDigit(activeDisplay, activeDisplay < 2 ? 
        (activeDisplay == 0 ? hours / 10 : hours % 10) :
        (activeDisplay == 2 ? minutes / 10 : minutes % 10),
        activeColor);
    
    if (colonVisible) {
        for(uint8_t i = 0; i < DISPLAY3_LEDS; i++) {
            strip->SetPixelColor(DISPLAY3_START + i, baseColor);
        }
    } else {
        for(uint8_t i = 0; i < DISPLAY3_LEDS; i++) {
            strip->SetPixelColor(DISPLAY3_START + i, RgbwColor(0));
        }
    }
    
    strip->SetPixelColor(DISPLAY4_START, RgbwColor(0));
    strip->Show();
}

void Effects::sparkleEffect(uint8_t hours, uint8_t minutes, bool colonVisible) {
    unsigned long currentMillis = millis();
    if (currentMillis - lastSparkleUpdate >= 200) {
        lastSparkleUpdate = currentMillis;
        
        for(uint8_t digit = 0; digit < 4; digit++) {
            float brightness = random(100) < 30 ? 0.8f : 1.0f;
            brightness = limitBrightness(brightness);
            
            RgbwColor color(
                (currentGreen * maxBrightness * brightness) / 255,
                (currentRed * maxBrightness * brightness) / 255,
                (currentBlue * maxBrightness * brightness) / 255,
                0
            );
            
            showDigit(digit, digit < 2 ? 
                (digit == 0 ? hours / 10 : hours % 10) :
                (digit == 2 ? minutes / 10 : minutes % 10),
                color);
        }
        
        if (colonVisible) {
            RgbwColor color(
                (currentGreen * maxBrightness) / 255,
                (currentRed * maxBrightness) / 255,
                (currentBlue * maxBrightness) / 255,
                0
            );
            for(uint8_t i = 0; i < DISPLAY3_LEDS; i++) {
                strip->SetPixelColor(DISPLAY3_START + i, color);
            }
        } else {
            for(uint8_t i = 0; i < DISPLAY3_LEDS; i++) {
                strip->SetPixelColor(DISPLAY3_START + i, RgbwColor(0));
            }
        }
        
        strip->SetPixelColor(DISPLAY4_START, RgbwColor(0));
        strip->Show();
    }
}
