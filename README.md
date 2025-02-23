# LED Clock Control

Этот проект представляет собой управление LED-часами с использованием ESP8266 и библиотеки NeoPixelBus. Часы отображают текущее время и поддерживают различные эффекты подсветки.

## Описание

Проект включает в себя:
- 4 семисегментных дисплея для отображения времени (часы и минуты).
- 2 дополнительных дисплея для отображения дополнительных эффектов.
- Поддержка различных эффектов подсветки, таких как статический цвет, радуга, дыхание, бегущий огонь и мерцание.

## Установка

1. **Клонируйте репозиторий:**
   ```bash
   git clone https://github.com/ваш_пользователь/led-clock-control.git
   cd led-clock-control
   ```

2. **Установите PlatformIO:**
   Убедитесь, что у вас установлен [PlatformIO](https://platformio.org/). Вы можете установить его как плагин для Visual Studio Code или использовать его в командной строке.

3. **Соберите проект:**
   В командной строке выполните:
   ```bash
   pio run
   ```

4. **Загрузите проект на ESP8266:**
   Подключите вашу плату ESP8266 и выполните:
   ```bash
   pio run --target upload
   ```

## Использование

После загрузки проекта на плату, подключитесь к Wi-Fi сети, указанной в `main.cpp`. Вы можете управлять цветом, яркостью и эффектами через веб-интерфейс, доступный по IP-адресу вашей платы.

### Эффекты

- **Статический режим**: Отображает выбранный цвет.
- **Радуга**: Плавно меняет цвета.
- **Дыхание**: Плавное нарастание и затухание яркости.
- **Бегущий огонь**: Эффект, имитирующий пламя.
- **Мерцание**: Эффект случайного мерцания.

## Зависимости

- [NeoPixelBus](https://github.com/Makuna/NeoPixelBus): Библиотека для управления адресными светодиодами.

## Лицензия

Этот проект лицензирован под MIT License. Пожалуйста, смотрите файл [LICENSE](LICENSE) для получения дополнительной информации.

## Контрибьюция

Если вы хотите внести изменения в проект, пожалуйста, создайте форк репозитория и отправьте пулл-запрос с вашими изменениями.

## Контакты

Если у вас есть вопросы или предложения, вы можете связаться со мной через GitHub.
