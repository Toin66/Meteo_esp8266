#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>

#define pressAdd 650 //добавление к значению давления
#define getDataInt 10000 //интервал опроса датчиков
#define writeDataInt 3600000 //10000 - 10 sec 1800000 - 30min 3600000-1hour
#define fwVersion "v. 0.0.0.1"

Adafruit_BMP280 bmp;               // Установка связи по интерфейсу I2C
const char* ssid = "anet";          // Название WiFi сети
const char* password = "myoldkey66";     // Пароль от  WiFi сети
WiFiServer server(80);                   // порт Web-сервера
String header;

unsigned long getDataTime; // переменная таймера для опроса датчика
unsigned long writeDataTime; // переменная таймера для записи таймера
byte tempData[24] = {0}; //журнал температуры
byte pressData[24] = {0}; //журнал давления

void makeWeb() {
  byte minTempVal = tempData[23];
  byte maxTempVal = tempData[23];
  byte minPressVal =  pressData[23];
  byte maxPressVal =  pressData[23];
  for (byte i = 0; i < 24; i++) {
    if (minTempVal > tempData[i]) minTempVal = tempData[i];
    if (maxTempVal < tempData[i]) maxTempVal = tempData[i];
    if (minPressVal > pressData[i]) minPressVal = pressData[i];
    if (maxPressVal < pressData[i]) maxPressVal = pressData[i];
  }

  WiFiClient client = server.available();               // Получаем данные, посылаемые клиентом
  if (client) {
    Serial.println("New Client.");                      // Отправка "Новый клиент"
    String currentLine = "";                            // Создаем строку для хранения входящих данных от клиента
    while (client.connected()) {                        // Пока есть соединение с клиентом
      if (client.available()) {                         // Если клиент активен
        char c = client.read();                         // Считываем посылаемую информацию в переменную "с"
        Serial.write(c);                                // Отправка в Serial port
        header += c;
        if (c == '\n') {                                // Вывод HTML страницы
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");          // Стандартный заголовок HT
            client.println("Content-type:text/html ");
            client.println("Connection: close");        // Соединение будет закрыто после завершения ответа
            client.println("Refresh: 10");              // Автоматическое обновление каждые 10 сек
            client.println();

            client.println("<!DOCTYPE html><html>");    // Веб-страница создается с использованием HTML
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<meta charset='UTF-8'>");   // Делаем русскую кодировку
            client.println("<link rel=\"icon\" href=\"data:,\">");

            client.println("<style>body { text-align: center; font-family: \"Trebuchet MS\", Arial;}");
            client.println("table { border-collapse: collapse; width:40%; margin-left:auto; margin-right:auto; }");
            client.println("th { padding: 12px; background-color: #0043af; color: white; }");
            client.println("tr { border: 1px solid #ddd; padding: 12px; }");
            client.println("tr:hover { background-color: #bcbcbc; }");
            client.println("td { border: none; padding: 12px; }");
            client.println(".sensor { color:white; font-weight: bold; background-color: #bcbcbc; padding: 1px; }");
            client.println("</style></head><body><h1>Метеостанция на bmp280 и ESP8266</h1>");
            client.println("<table><tr><th>Параметр</th><th>Показания</th></tr>");
            client.println("<tr><td>Температура</td><td><span class=\"sensor\">");
            client.println(round(bmp.readTemperature()), 0);
            client.println(" *C</span></td></tr>");
            client.println("<tr><td>Давление</td><td><span class=\"sensor\">");
            client.println(round((bmp.readPressure() / 100.0F ) * 0.75), 0);
            client.println(" mm.Hg.</span></td></tr>");
            client.println("<tr><td>MinTemp</td><td><span class=\"sensor\">");
            client.println(minTempVal);
            client.println(" *C</span></td></tr>");
            client.println("<tr><td>MaxTemp</td><td><span class=\"sensor\">");
            client.println(maxTempVal);
            client.println(" *C</span></td></tr>");

            client.println("<tr><td>MinPress</td><td><span class=\"sensor\">");
            client.println(minPressVal + pressAdd);
            client.println(" mm.Hg.</span></td></tr>");
            client.println("<tr><td>MaxPress</td><td><span class=\"sensor\">");
            client.println(maxPressVal + pressAdd);
            client.println(" mm.Hg.</span></td></tr>");

            client.println("</table><table>");
            client.println("<tr><td>Статистика температуры</td></tr>");

            client.println("<tr>");
            for (byte i = 0; i < 24; i++) {
              client.println("<td><span class=\"sensor\">");
              if (tempData[i] != 0) {
                client.println(tempData[i]);
                client.println(" *C</span></td>");
              }
              else {
                client.println("N/A");
                client.println("</span></td>");
              }
            }
            client.println("</tr>");

            client.println("<tr><td>Статистика давления</td></tr>");

            client.println("<tr>");
            for (byte i = 0; i < 24; i++) {
              client.println("<td><span class=\"sensor\">");
              if (pressData[i] != 0) {
                client.println(pressData[i] + pressAdd);
                client.println(" mm.Hg.</span></td>");
              }
              else {
                client.println("N/A");
                client.println("</span></td>");
              }
            }

            client.println("</tr></table>");
            client.println("</body></html>");
            client.println();
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    header = "";
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}

void getMeteoData() {

  if ((millis() - writeDataTime > writeDataInt) || tempData[23] == 0) {
    writeDataTime = millis();
    for (byte i = 0; i < 24; i++) {
      if (i != 23) {
        tempData[i] = tempData[i + 1];
        pressData[i] = pressData[i + 1];
      }
      if (i == 23) {
        tempData[23] = round(bmp.readTemperature());
        pressData[23] =  round((bmp.readPressure() / 100.0F ) * 0.75) - pressAdd;
      }
    }
  }


}

void setup() {
  Serial.begin(115200);                                 // Скорость передачи 115200
  bool status;
  if (!bmp.begin(0x76)) {                               // Проверка инициализации датчика
    Serial.println("Could not find a valid bmp280 sensor, check wiring!"); // Печать, об ошибки инициализации.
    while (1);                                          // Зацикливаем
  }

  Serial.print("Connecting to ");                       // Отправка в Serial port
  Serial.println(ssid);                                 // Отправка в Serial port
  WiFi.begin(ssid, password);                           // Подключение к WiFi Сети
  while (WiFi.status() != WL_CONNECTED) {               // Проверка подключения к WiFi сети
    delay(500);                                         // Пауза
    Serial.print(".");                                  // Отправка в Serial port
  }
  Serial.println("");                                   // Отправка в Serial port
  Serial.println("WiFi connected.");                    // Отправка в Serial port
  Serial.println("IP address: ");                       // Отправка в Serial port
  Serial.println(WiFi.localIP());                       // Отправка в Serial port
  server.begin();

  getDataTime = millis();
  writeDataTime = millis();
}

void loop() {
  if (millis() - getDataTime > getDataInt) {
    getDataTime = millis();
    getMeteoData();
  }

  makeWeb();
}
