#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <NeoPixelBus.h>
#include <EEPROM.h>
#include <time.h>

// Создаем объект ленты в зависимости от типа
NeoPixelBus<NeoRgbwFeature, NeoEsp8266Uart1Ws2813Method>* strip = nullptr;

// Изменим константы для 7-сегментного дисплея
const uint8_t LEDS_PER_SEGMENT = 3;    // количество светодиодов в одном сегменте
const uint8_t SEGMENTS_PER_DIGIT = 7;   // количество сегментов в одной цифре
const uint8_t DIGIT_COUNT = 4;          // количество цифр
const uint16_t TOTAL_SEGMENT_LEDS = DIGIT_COUNT * SEGMENTS_PER_DIGIT * LEDS_PER_SEGMENT;  // 84 светодиода для цифр

// Порядок сегментов в ленте (g,b,a,f,e,d,c)
const uint8_t SEG_G = 0;
const uint8_t SEG_B = 1;
const uint8_t SEG_A = 2;
const uint8_t SEG_F = 3;
const uint8_t SEG_E = 4;
const uint8_t SEG_D = 5;
const uint8_t SEG_C = 6;

// Массив для маппинга цветов на сегменты (0-9)
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

// Функция для получения индекса начала сегмента
uint16_t getSegmentStart(uint8_t digit, uint8_t segment) {
    if (digit < 2) {
        // Первые два дисплея остаются как есть (0-41)
        return (digit * SEGMENTS_PER_DIGIT + segment) * LEDS_PER_SEGMENT;
    } else {
        // Для третьего и четвертого дисплея добавляем смещение в 3 светодиода
        // После индексов 42,43,44 идут следующие дисплеи
        return ((digit * SEGMENTS_PER_DIGIT + segment) * LEDS_PER_SEGMENT) + 3;
    }
}

// Функция для установки цвета сегмента
void setSegmentColor(uint8_t digit, uint8_t segment, RgbwColor color) {
    uint16_t start = getSegmentStart(digit, segment);
    for(uint8_t i = 0; i < LEDS_PER_SEGMENT; i++) {
        strip->SetPixelColor(start + i, color);
    }
}

// Функция для отображения одной цифры
void showDigit(uint8_t digit, uint8_t number, RgbwColor color) {
    if(number > 9) return;
    
    // Устанавливаем цвет для каждого сегмента согласно маппингу
    setSegmentColor(digit, SEG_G, DIGITS[number][0] ? color : RgbwColor(0));
    setSegmentColor(digit, SEG_B, DIGITS[number][1] ? color : RgbwColor(0));
    setSegmentColor(digit, SEG_A, DIGITS[number][2] ? color : RgbwColor(0));
    setSegmentColor(digit, SEG_F, DIGITS[number][3] ? color : RgbwColor(0));
    setSegmentColor(digit, SEG_E, DIGITS[number][4] ? color : RgbwColor(0));
    setSegmentColor(digit, SEG_D, DIGITS[number][5] ? color : RgbwColor(0));
    setSegmentColor(digit, SEG_C, DIGITS[number][6] ? color : RgbwColor(0));
}

// Константы для дополнительных дисплеев
const uint16_t DISPLAY3_START = 42;     // Начальный индекс для доп. дисплея (2 диода)
const uint16_t DISPLAY3_LEDS = 2;       // Количество диодов
const uint16_t DISPLAY4_START = 44;     // Начальный индекс для доп. дисплея (1 диод)
const uint16_t DISPLAY4_LEDS = 1;       // Количество диодов

// Изменим объявление PixelCount, учитывая сдвиг
const uint16_t PixelCount = 90;         // 21 + 21 + 2 + 1 + 21 + 21 + 3 = 90 светодиодов всего

// Функция для установки цвета дополнительных дисплеев
void setExtraDisplays(RgbwColor color) {
    // Дополнительный дисплей 1 (2 диода)
    for(uint16_t i = 0; i < DISPLAY3_LEDS; i++) {
        strip->SetPixelColor(DISPLAY3_START + i, color);
    }
    
    // Дополнительный дисплей 2 (1 диод)
    strip->SetPixelColor(DISPLAY4_START, color);
}

// Функция для отображения числа
void showNumber(int number, RgbwColor color) {
    // Ограничиваем число четырьмя разрядами
    number = constrain(number, 0, 9999);
    
    // Разбиваем число на цифры
    uint8_t digits[4];
    digits[0] = number / 1000;          // Тысячи
    digits[1] = (number / 100) % 10;    // Сотни
    digits[2] = (number / 10) % 10;     // Десятки
    digits[3] = number % 10;            // Единицы
    
    // Первые две цифры на первый и второй дисплей (индексы 0-41)
    showDigit(0, digits[0], color);  // Первая цифра (тысячи)
    showDigit(1, digits[1], color);  // Вторая цифра (сотни)
    
    // Показываем дополнительные дисплеи (индексы 42-44)
    setExtraDisplays(color);
    
    // Последние две цифры на пятый и шестой дисплей (индексы 45-86)
    showDigit(2, digits[2], color);  // Третья цифра (десятки)
    showDigit(3, digits[3], color);  // Четвертая цифра (единицы)
    
    strip->Show();
}

// В начале файла добавим определения типов лент
enum StripType {
    SK6812_RGBW,
    WS2812_RGB,
    WS2812B_RGB
};

// Добавим перечисление для эффектов
enum Effect {
    STATIC,
    RAINBOW,
    BREATHING,
    RUNNING,
    SPARKLE,
    EFFECT_COUNT  // Используем для проверки границ
};

// Глобальные переменные
const uint8_t PixelPin = 2;  // Фиксированный пин GPIO2 (D4)
StripType currentStripType = SK6812_RGBW;

// Адреса в EEPROM
const int TYPE_ADDRESS = 0;
const int BRIGHTNESS_LIMIT_ADDRESS = 2;
const int RED_ADDRESS = 3;
const int GREEN_ADDRESS = 4;
const int BLUE_ADDRESS = 5;
const int EFFECT_ADDRESS = 6;

// Добавим глобальные переменные для анимации
Effect currentEffect = STATIC;  // Начальный эффект не важен, т.к. время всегда отображается
unsigned long lastUpdate = 0;
uint8_t effectStep = 0;

// Добавим глобальные переменные для хранения времени
uint8_t currentHours = 0;
uint8_t currentMinutes = 0;
bool colonVisible = true;  // Для мигания разделителя

// Добавим глобальную переменную для отслеживания времени мигания
unsigned long lastColonUpdate = 0;

// Добавим глобальную переменную для отслеживания времени бегущего огня
unsigned long lastRunningUpdate = 0;

// Добавим глобальную переменную для отслеживания времени мерцания
unsigned long lastSparkleUpdate = 0;

// Добавим глобальную переменную для фазы бегущего огня
uint8_t runningPhase = 0;

// Добавим прототип функции перед updateEffect
void showAllDigits(RgbwColor color);

// Функция для отображения времени
void showTime(RgbwColor color) {
    // Отображаем часы на первых двух дисплеях
    showDigit(0, currentHours / 10, color);  // Десятки часов
    showDigit(1, currentHours % 10, color);  // Единицы часов
    
    // Мигающий разделитель только на третьем дисплее (2 диода)
    if (colonVisible) {
        // Третий дисплей мигает
        for(uint16_t i = 0; i < DISPLAY3_LEDS; i++) {
            strip->SetPixelColor(DISPLAY3_START + i, color);
        }
    } else {
        // Выключаем третий дисплей
        for(uint16_t i = 0; i < DISPLAY3_LEDS; i++) {
            strip->SetPixelColor(DISPLAY3_START + i, RgbwColor(0));
        }
    }
    
    // Чтвертый дисплей всегда выключен
    strip->SetPixelColor(DISPLAY4_START, RgbwColor(0));
    
    // Отображаем минуты на последних двух дисплеях
    showDigit(2, currentMinutes / 10, color);  // Десятки минут
    showDigit(3, currentMinutes % 10, color);  // Единицы минут
    
    strip->Show();
}

// Функции для работы с EEPROM
void saveStripConfig(StripType type, uint8_t brightness, uint8_t red, uint8_t green, uint8_t blue, Effect effect) {
    EEPROM.begin(512);
    EEPROM.write(TYPE_ADDRESS, (uint8_t)type);
    EEPROM.write(BRIGHTNESS_LIMIT_ADDRESS, brightness);
    EEPROM.write(RED_ADDRESS, red);
    EEPROM.write(GREEN_ADDRESS, green);
    EEPROM.write(BLUE_ADDRESS, blue);
    EEPROM.write(EFFECT_ADDRESS, (uint8_t)effect);
    EEPROM.commit();
}

// Настройки WiFi
const char* ssid = "q7c";
const char* password = "11111111";

ESP8266WebServer server(80);

// Сначала объявим все глобальные переменные
uint8_t currentRed = 0;
uint8_t currentGreen = 0;
uint8_t currentBlue = 0;
uint8_t currentWhite = 0;
uint8_t currentBrightness = 255;
uint8_t maxBrightness = 255;  // Объявляем перед использованием в HTML

// HTML шаблон без значения яркости
const char* serverIndex = R"(
<!DOCTYPE html>
<html lang="ru">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>LED Clock Control</title>
    <style>
        :root {
            --primary: #00ff88;
            --primary-dark: #00cc66;
            --bg-dark: #121212;
            --bg-panel: #1e1e1e;
            --border: #2a2a2a;
            --text: #ffffff;
        }
        
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: 'Segoe UI', Arial, sans-serif;
            background: var(--bg-dark);
            color: var(--text);
            line-height: 1.6;
            padding: 20px;
            min-height: 100vh;
        }
        
        .container {
            max-width: 800px;
            margin: 0 auto;
        }
        
        .header {
            text-align: center;
            margin-bottom: 40px;
        }
        
        .header h1 {
            color: var(--primary);
            font-size: 2.5em;
            margin-bottom: 10px;
            text-shadow: 0 0 10px rgba(0,255,136,0.3);
        }
        
        .header p {
            color: var(--primary-dark);
            font-size: 1.1em;
        }
        
        .panel {
            background: var(--bg-panel);
            border-radius: 15px;
            padding: 25px;
            margin-bottom: 20px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
            border: 1px solid var(--border);
        }
        
        .panel h2 {
            color: var(--primary);
            margin-bottom: 20px;
            font-size: 1.5em;
        }
        
        .control-group {
            margin-bottom: 15px;
        }
        
        .control-group label {
            display: block;
            margin-bottom: 8px;
            color: var(--primary-dark);
        }
        
        .color-picker {
            width: 100%;
            height: 60px;
            border: none;
            border-radius: 10px;
            cursor: pointer;
            background: var(--bg-dark);
            margin-bottom: 15px;
        }
        
        .slider {
            -webkit-appearance: none;
            width: 100%;
            height: 10px;
            border-radius: 5px;
            background: var(--bg-dark);
            outline: none;
            margin-bottom: 15px;
        }
        
        .slider::-webkit-slider-thumb {
            -webkit-appearance: none;
            width: 20px;
            height: 20px;
            border-radius: 50%;
            background: var(--primary);
            cursor: pointer;
            transition: background 0.2s;
        }
        
        .slider::-webkit-slider-thumb:hover {
            background: var(--primary-dark);
        }
        
        .select {
            width: 100%;
            padding: 12px;
            border-radius: 10px;
            background: var(--bg-dark);
            border: 1px solid var(--border);
            color: var(--text);
            font-size: 16px;
            cursor: pointer;
            margin-bottom: 15px;
        }
        
        .select option {
            background: var(--bg-dark);
            color: var(--text);
            padding: 12px;
        }
        
        .btn {
            width: 100%;
            padding: 12px;
            border: none;
            border-radius: 10px;
            background: var(--primary);
            color: var(--bg-dark);
            font-size: 16px;
            font-weight: bold;
            cursor: pointer;
            transition: all 0.2s;
        }
        
        .btn:hover {
            background: var(--primary-dark);
            transform: translateY(-2px);
        }
        
        .file-input {
            width: 100%;
            padding: 12px;
            background: var(--bg-dark);
            border: 1px solid var(--border);
            border-radius: 10px;
            color: var(--text);
            margin-bottom: 15px;
        }
        
        @media (max-width: 600px) {
            .container {
                padding: 10px;
            }
            
            .panel {
                padding: 15px;
            }
        }
    </style>
    <script>
    function applyEffect() {
        const effect = document.getElementById('effectSelect').value;
        fetch('/effect?value=' + effect)
            .then(response => {
                if (!response.ok) throw new Error('Network response was not ok');
                console.log('Effect changed to:', effect);
            })
            .catch(error => {
                console.error('Error:', error);
            });
        return false;
    }

    function applyColor() {
        const color = document.getElementById('colorPicker').value;
        fetch('/update-strip?color=' + encodeURIComponent(color))
            .then(response => {
                if (!response.ok) throw new Error('Network response was not ok');
                console.log('Color changed to:', color);
            })
            .catch(error => {
                console.error('Error:', error);
            });
        return false;
    }

    function applyBrightness() {
        const brightness = document.getElementById('brightnessSlider').value;
        fetch('/brightness?value=' + brightness)
            .then(response => {
                if (!response.ok) throw new Error('Network response was not ok');
                console.log('Brightness changed to:', brightness);
            })
            .catch(error => {
                console.error('Error:', error);
            });
        return false;
    }
    </script>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>LED Clock Control</h1>
            <p>Управление LED часами</p>
        </div>

        <div class="panel">
            <h2>Цвет подсветки</h2>
            <form onsubmit='return applyColor()'>
                <div class="control-group">
                    <label>Выберите цвет:</label>
                    <input type="color" id="colorPicker" name="color" value="#ff0000" class="color-picker">
                </div>
                <button type="submit" class="btn">Применить цвет</button>
            </form>
        </div>

        <div class="panel">
            <h2>Яркость</h2>
            <form onsubmit='return applyBrightness()'>
                <div class="control-group">
                    <label>Уровень яркости:</label>
                    <input type="range" id="brightnessSlider" name="value" min="0" max="255" value="255" class="slider">
                </div>
                <button type="submit" class="btn">Установить яркость</button>
            </form>
        </div>

        <div class="panel">
            <h2>Эффекты</h2>
            <form onsubmit='return applyEffect()'>
                <div class="control-group">
                    <label>Выберите эффект:</label>
                    <select id="effectSelect" name="value" class="select">
                        <option value="0">Статический режим</option>
                        <option value="1">Радуга</option>
                        <option value="2">Дыхание</option>
                        <option value="3">Бегущий огонь</option>
                        <option value="4">Мерцание</option>
                    </select>
                </div>
                <button type="submit" class="btn">Применить эффект</button>
            </form>
        </div>

        <div class="panel">
            <h2>Обновление прошивки</h2>
            <form method="POST" action="/update" enctype="multipart/form-data">
                <div class="control-group">
                    <label>Выберите файл прошивки:</label>
                    <input type="file" name="update" accept=".bin" class="file-input">
                </div>
                <button type="submit" class="btn">Обновить прошивку</button>
            </form>
        </div>
    </div>
</body>
</html>
)";

// Объявим функцию updateEffect перед setup()
void updateEffect();

// Изменим функцию limitBrightness
float limitBrightness(float brightness) {
    float maxValue = (float)maxBrightness / 255.0f;
    return brightness < maxValue ? brightness : maxValue;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Запуск");
  
  // Чтние конфигурации из EEPROM
  EEPROM.begin(512);
  StripType savedType = (StripType)EEPROM.read(TYPE_ADDRESS);
  
  if (savedType <= WS2812B_RGB) {
      currentStripType = savedType;
  }
  
  // Инциализация ленты
  if (strip != nullptr) {
    delete strip;
  }
  strip = new NeoPixelBus<NeoRgbwFeature, NeoEsp8266Uart1Ws2813Method>(PixelCount, PixelPin);
  strip->Begin();
  
  // Устанавливаем начальный расный цвет
  currentRed = 255;
  currentGreen = 0;
  currentBlue = 0;
  currentWhite = 0;
  currentBrightness = 255;
  
  // После чтения других параметров из EEPROM
  uint8_t savedBrightness = EEPROM.read(BRIGHTNESS_LIMIT_ADDRESS);
  if (savedBrightness > 0 && savedBrightness <= 255) {
      maxBrightness = savedBrightness;
  }

  // Читаем сохраненный цвет
  uint8_t savedRed = EEPROM.read(RED_ADDRESS);
  uint8_t savedGreen = EEPROM.read(GREEN_ADDRESS);
  uint8_t savedBlue = EEPROM.read(BLUE_ADDRESS);
  
  // Прменяем сохраненный цвет, если он валиден
  if (savedRed != 255 || savedGreen != 255 || savedBlue != 255) {
      currentRed = savedRed;
      currentGreen = savedGreen;
      currentBlue = savedBlue;
  } else {
      // Наче используем красный по умолчанию
      currentRed = 255;
      currentGreen = 0;
      currentBlue = 0;
  }

  // Обновляем цвет ленты с учетом всех параметров
  RgbwColor color(
      (currentGreen * maxBrightness) / 255,
      (currentRed * maxBrightness) / 255,
      (currentBlue * maxBrightness) / 255,
      currentWhite
  );
  
  for(uint16_t i = 0; i < PixelCount; i++) {
      strip->SetPixelColor(i, color);
  }
  strip->Show();

  // Подключение к WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Ошибка подключения! Перезагрузка...");
    delay(5000);
    ESP.restart();
  }

  // Настройка OTA
  ArduinoOTA.setHostname("esp8266-ota"); // Задайте своё имя устройства
  
  ArduinoOTA.onStart([]() {
    Serial.println("Начало OTA обновления");
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\nавешено");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Прогресс: %u%%\r", (progress / (total / 100)));
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Ошибка[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Ошибка аутентификации");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Ошибка начала OTA");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Ошибка соединения");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Ошибк получения данных");
    else if (error == OTA_END_ERROR) Serial.println("Ошибка завершения");
  });

  ArduinoOTA.begin();

  // Настройка веб-сервера
  server.on("/", HTTP_GET, []() {
    String html = String(serverIndex);
    html.replace("value='255'", "value='" + String(maxBrightness) + "'");
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", html);
  });
  
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "Ошибка" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Обновление: %s\n", upload.filename.c_str());
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      if (!Update.begin(maxSketchSpace)) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) {
        Serial.printf("Обновление успешно: %u\nПерезагрузка...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
    yield();
  });

  // Обновляем обработчики с захватом переменных
  server.on("/color", HTTP_GET, [&]() {
    String hexColor = server.arg("hex");
    long number = strtol(hexColor.c_str(), NULL, 16);
    currentRed = number >> 16;
    currentGreen = (number >> 8) & 0xFF;
    currentBlue = number & 0xFF;
    
    RgbwColor color(
      currentRed,
      currentGreen,
      currentBlue,
      currentWhite
    );
    
    for(uint16_t i = 0; i < PixelCount; i++) {
      strip->SetPixelColor(i, color);
    }
    strip->Show();
    server.send(200, "text/plain", "OK");
  });

  server.on("/brightness", HTTP_GET, [&]() {
    String value = server.arg("value");
    maxBrightness = value.toInt();
    
    // Сохраняем яркость в EEPROM
    EEPROM.begin(512);
    EEPROM.write(BRIGHTNESS_LIMIT_ADDRESS, maxBrightness);
    EEPROM.commit();
    
    // Перенаправляем обратно на главную страницу
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
  });

  server.on("/white", HTTP_GET, [&]() {
    currentWhite = server.arg("value").toInt();
    RgbwColor color(
      currentRed,
      currentGreen,
      currentBlue,
      currentWhite
    );
    
    for(uint16_t i = 0; i < PixelCount; i++) {
      strip->SetPixelColor(i, color);
    }
    strip->Show();
    server.send(200, "text/plain", "OK");
  });

  // Добавляем новый обработчик для одновременного обновления всех параметров
  server.on("/update-strip", HTTP_GET, [&]() {
    String color = server.arg("color");
    // Убираем символ # из начала строки, если он есть
    if (color.startsWith("#")) {
        color = color.substring(1);
    }
    
    // Преобразуем HEX в RGB
    long number = strtol(color.c_str(), NULL, 16);
    currentRed = (number >> 16) & 0xFF;
    currentGreen = (number >> 8) & 0xFF;
    currentBlue = number & 0xFF;
    currentWhite = 0;
    
    // Сохраняем в EEPROM
    EEPROM.begin(512);
    EEPROM.write(RED_ADDRESS, currentRed);
    EEPROM.write(GREEN_ADDRESS, currentGreen);
    EEPROM.write(BLUE_ADDRESS, currentBlue);
    EEPROM.commit();
    
    // Применяем цвет
    RgbwColor rgbw(
        (currentGreen * maxBrightness) / 255,
        (currentRed * maxBrightness) / 255,
        (currentBlue * maxBrightness) / 255,
        0
    );
    
    // Обновляем дисплеи
    showAllDigits(rgbw);
    
    // Перенаправляем обратно на главную страницу
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
  });

  // Добавляем новый обработчик для конфигурации ленты
  server.on("/strip-config", HTTP_GET, [&]() {
    uint16_t newCount = server.arg("count").toInt();
    StripType newType = (StripType)server.arg("type").toInt();
    uint8_t newBrightness = server.arg("brightness").toInt();
    
    if (newCount > 0 && newCount <= 1000 &&
        newType <= WS2812B_RGB &&
        newBrightness > 0 && newBrightness <= 255) {
        
        saveStripConfig(newType, newBrightness, currentRed, currentGreen, currentBlue, currentEffect);
        server.send(200, "text/plain", "OK");
        ESP.restart();
    } else {
        server.send(400, "text/plain", "Invalid parameters");
    }
  });

  // В setup() после чтения других параметров
  uint8_t savedEffect = EEPROM.read(EFFECT_ADDRESS);
  if(savedEffect < EFFECT_COUNT) {
      currentEffect = (Effect)savedEffect;
  }

  // Добавим обработчик изменения эффекта (перед server.begin())
  server.on("/effect", HTTP_GET, [&]() {
      int effect = server.arg("value").toInt();
      if(effect >= 0 && effect < EFFECT_COUNT) {
          currentEffect = (Effect)effect;
          effectStep = 0;
          
          // Схраняем эффект в EEPROM
          EEPROM.begin(512);
          EEPROM.write(EFFECT_ADDRESS, (uint8_t)currentEffect);
          EEPROM.commit();
          
          server.send(200, "text/plain", "OK");
      } else {
          server.send(400, "text/plain", "Invalid effect");
      }
  });

  // Настройка времени
  configTime(3 * 3600, 0, "ru.pool.ntp.org", "europe.pool.ntp.org", "ntp1.stratum2.ru"); // GMT+3, без летнего времени
  Serial.println("Ожидание синхронизации времени...");
  while (time(nullptr) < 10000) {
      delay(100);
  }

  Serial.println("Подключение к NTP...");
  time_t now = time(nullptr);
  while (now < 24 * 3600) {
      Serial.print(".");
      delay(100);
      now = time(nullptr);
  }
  Serial.println();
  Serial.println("Время синхронизировано");

  server.begin();
  
  Serial.println("OTA и веб-сервер готовы");
  Serial.print("IP адрес: ");
  Serial.println(WiFi.localIP());

  // Добавим обработчик установки времени
  server.on("/set-time", HTTP_GET, [&]() {
      int hours = server.arg("hours").toInt();
      int minutes = server.arg("minutes").toInt();
      
      if(hours >= 0 && hours <= 23 && minutes >= 0 && minutes <= 59) {
          // Устанавливаем время
          currentHours = hours;
          currentMinutes = minutes;
          
          // Сразу отображаем новое время
          RgbwColor color(
              (currentGreen * maxBrightness) / 255,
              (currentRed * maxBrightness) / 255,
              (currentBlue * maxBrightness) / 255,
              0
          );
          showTime(color);
          
          server.send(200, "text/plain", "OK");
      } else {
          server.send(400, "text/plain", "Invalid time");
      }
  });

  // Добавим обработчик для получения времени
  server.on("/get-time", HTTP_GET, [&]() {
      time_t now = time(nullptr);
      struct tm* timeinfo = localtime(&now);
      char timeString[6];
      sprintf(timeString, "%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min);
      server.send(200, "text/plain", timeString);
  });
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();
  updateEffect();
}

// В функции updateEffect изменим структуру:
void updateEffect() {
    static unsigned long lastTimeUpdate = 0;
    static unsigned long lastEffectUpdate = 0;
    unsigned long currentMillis = millis();

    // Объявляем все переменные в начале функции
    RgbwColor color;
    float brightness = 0;
    RgbwColor dimColor;
    RgbwColor brightColor;
    RgbwColor baseColor;

    // Обновляем время каждую секунду
    if (currentMillis - lastTimeUpdate >= 1000) {
        lastTimeUpdate = currentMillis;
        time_t now = time(nullptr);
        struct tm* timeinfo = localtime(&now);
        currentHours = timeinfo->tm_hour;
        currentMinutes = timeinfo->tm_min;
    }

    // Обновляем состояние разделителя каждые 500мс
    if(currentMillis - lastColonUpdate >= 500) {
        lastColonUpdate = currentMillis;
        colonVisible = !colonVisible;
    }

    // Обновляем эффекты каждые 50мс
    if (currentMillis - lastEffectUpdate >= 50) {
        lastEffectUpdate = currentMillis;

        switch (currentEffect) {
            case STATIC:
            default:
                color = RgbwColor(
                    (currentGreen * maxBrightness) / 255,
                    (currentRed * maxBrightness) / 255,
                    (currentBlue * maxBrightness) / 255,
                    0
                );
                break;

            case RAINBOW:
                effectStep = (effectStep + 1) % 256;
                if(effectStep < 85) {
                    color = RgbwColor(
                        (255 - effectStep * 3) * maxBrightness / 255,
                        (effectStep * 3) * maxBrightness / 255,
                        0,
                        0
                    );
                } else if(effectStep < 170) {
                    uint8_t step = effectStep - 85;
                    color = RgbwColor(
                        0,
                        (255 - step * 3) * maxBrightness / 255,
                        (step * 3) * maxBrightness / 255,
                        0
                    );
                } else {
                    uint8_t step = effectStep - 170;
                    color = RgbwColor(
                        (step * 3) * maxBrightness / 255,
                        0,
                        (255 - step * 3) * maxBrightness / 255,
                        0
                    );
                }
                break;

            case BREATHING:
                effectStep = (effectStep + 2) % 256;
                brightness = sin(effectStep * PI / 128) * 0.4 + 0.6;
                color = RgbwColor(
                    (currentGreen * maxBrightness * brightness) / 255,
                    (currentRed * maxBrightness * brightness) / 255,
                    (currentBlue * maxBrightness * brightness) / 255,
                    0
                );
                break;

            case RUNNING:
                {
                    // Обновляем фазу анимации (0-255)
                    runningPhase = (runningPhase + 2) % 256;  // Увеличили шаг для более быстрой анимации
                    
                    // Базовая яркость 30%
                    const float baseBrightness = 0.3f;
                    
                    // Вычисляем текущий активный дисплей (0-3)
                    uint8_t activeDisplay = (runningPhase / 64) % 4;  // Делим на 64 для 4 фаз
                    
                    // Вычисляем фазу перехода (0-63)
                    uint8_t transitionPhase = runningPhase % 64;
                    
                    // Создаем базовый цвет для неактивных дисплеев
                    RgbwColor baseColor(
                        (currentGreen * maxBrightness * baseBrightness) / 255,
                        (currentRed * maxBrightness * baseBrightness) / 255,
                        (currentBlue * maxBrightness * baseBrightness) / 255,
                        0
                    );
                    
                    // Отображаем все дисплеи с базовой яркостью
                    for(uint8_t i = 0; i < 4; i++) {
                        showDigit(i, i < 2 ? 
                            (i == 0 ? currentHours / 10 : currentHours % 10) :
                            (i == 2 ? currentMinutes / 10 : currentMinutes % 10),
                            baseColor);
                    }
                    
                    // Вычисляем яркость для активного дисплея (плавное нарастание и затухание)
                    float activePhase = (float)transitionPhase / 63.0f;  // 0.0 - 1.0
                    float activeBrightness = baseBrightness + (1.0f - baseBrightness) * sin(activePhase * PI);
                    
                    // Создаем цвет для активного дисплея
                    RgbwColor activeColor(
                        (currentGreen * maxBrightness * activeBrightness) / 255,
                        (currentRed * maxBrightness * activeBrightness) / 255,
                        (currentBlue * maxBrightness * activeBrightness) / 255,
                        0
                    );
                    
                    // Обновляем активный дисплей
                    showDigit(activeDisplay, activeDisplay < 2 ? 
                        (activeDisplay == 0 ? currentHours / 10 : currentHours % 10) :
                        (activeDisplay == 2 ? currentMinutes / 10 : currentMinutes % 10),
                        activeColor);
                    
                    // Разделитель мигает с базовой яркостью
                    if (colonVisible) {
                        for(uint8_t i = 0; i < DISPLAY3_LEDS; i++) {
                            strip->SetPixelColor(DISPLAY3_START + i, baseColor);
                        }
                    } else {
                        for(uint8_t i = 0; i < DISPLAY3_LEDS; i++) {
                            strip->SetPixelColor(DISPLAY3_START + i, RgbwColor(0));
                        }
                    }
                    
                    // Четвертый дисплей всегда выключен
                    strip->SetPixelColor(DISPLAY4_START, RgbwColor(0));
                    
                    strip->Show();
                    return;
                }

            case SPARKLE:
                // Обновляем мерцание каждые 200мс
                if (currentMillis - lastSparkleUpdate >= 200) {
                    lastSparkleUpdate = currentMillis;
                    
                    for(uint8_t digit = 0; digit < 4; digit++) {
                        if(random(100) < 30) { // 30% вероятность уменьшения яркости
                            // Уменьшаем яркость на 20% от базовой
                            color = RgbwColor(
                                (currentGreen * maxBrightness * 0.8) / 255,  // -20% от яркости
                                (currentRed * maxBrightness * 0.8) / 255,    // -20% от яркости
                                (currentBlue * maxBrightness * 0.8) / 255,   // -20% от яркости
                                0
                            );
                        } else {
                            // Базовая яркость
                            color = RgbwColor(
                                (currentGreen * maxBrightness) / 255,
                                (currentRed * maxBrightness) / 255,
                                (currentBlue * maxBrightness) / 255,
                                0
                            );
                        }
                        showDigit(digit, digit < 2 ? 
                            (digit == 0 ? currentHours / 10 : currentHours % 10) :
                            (digit == 2 ? currentMinutes / 10 : currentMinutes % 10),
                            color);
                    }
                    
                    // Разделитель с базовой яркостью
                    if (colonVisible) {
                        color = RgbwColor(
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
                    
                    // Четвертый дисплей всегда выключен
                    strip->SetPixelColor(DISPLAY4_START, RgbwColor(0));
                    
                    strip->Show();
                }
                return; // Выходим, так как уже обновили дисплей
        }

        // Общий код для отображения
        showAllDigits(color);
    }
}

// Реализация showAllDigits после updateEffect
void showAllDigits(RgbwColor color) {
    // Отображаем время
    showDigit(0, currentHours / 10, color);
    showDigit(1, currentHours % 10, color);

    // Мигающий разделитель
    if (colonVisible) {
        for(uint8_t i = 0; i < DISPLAY3_LEDS; i++) {
            strip->SetPixelColor(DISPLAY3_START + i, color);
        }
    } else {
        for(uint8_t i = 0; i < DISPLAY3_LEDS; i++) {
            strip->SetPixelColor(DISPLAY3_START + i, RgbwColor(0));
        }
    }

    // Четвертый дисплей всегда выключен
    strip->SetPixelColor(DISPLAY4_START, RgbwColor(0));

    // Отображаем минуты
    showDigit(2, currentMinutes / 10, color);
    showDigit(3, currentMinutes % 10, color);

    strip->Show();
} 
