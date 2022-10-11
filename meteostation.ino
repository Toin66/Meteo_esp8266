#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>

#define pressAdd 650 //добавление к значению давления
#define getDataInt 10000 //интервал опроса датчиков
#define writeDataInt 3600000 //10000 - 10 sec 1800000 - 30min 3600000-1hour
#define fwVersion "v. 0.0.0.1"

Adafruit_BMP280 bmp;               
const char* ssid = "anet";          
const char* password = "myoldkey66";    
WiFiServer server(80);                  
String header;

unsigned long getDataTime; // переменная таймера для опроса датчика
unsigned long writeDataTime; // переменная таймера для записи таймера
byte tempData[24] = {0}; //журнал температуры
byte pressData[24] = {0}; //журнал давления

void makeWeb() {
  byte minTempVal = tempData[23];
  byte maxTempVal = 0;
  byte minPressVal = pressData[23];
  byte maxPressVal =  0;

  for (byte i = 0; i < 24; i++) {
    if ((minTempVal > tempData[i]) & (tempData[i] != 0)) minTempVal = tempData[i];
    if (maxTempVal < tempData[i]) maxTempVal = tempData[i];
    if ((minPressVal > pressData[i]) & (pressData[i] != 0)) minPressVal = pressData[i];
    if (maxPressVal < pressData[i]) maxPressVal = pressData[i];
  }

  WiFiClient client = server.available();              
  if (client) {
    Serial.println("New Client.");                      
    String currentLine = "";                           
    while (client.connected()) {                        
      if (client.available()) {                         
        char c = client.read();                         
        Serial.write(c);                                
        header += c;
        if (c == '\n') {                                
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");          
            client.println("Content-type:text/html ");
            client.println("Connection: close");        
            client.println("Refresh: 10");              
            client.println();

            client.println("<!DOCTYPE html><html>");    
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<meta charset='UTF-8'>");   
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
  Serial.begin(115200);                                
  bool status;
  if (!bmp.begin(0x76)) {                               
    Serial.println("Could not find a valid bmp280 sensor, check wiring!"); 
    while (1);                                          
  }

  Serial.print("Connecting to ");                       
  Serial.println(ssid);                                 
  WiFi.begin(ssid, password);                           
  while (WiFi.status() != WL_CONNECTED) {               
    delay(500);                                         
    Serial.print(".");                                  
  }
  Serial.println("");                                   
  Serial.println("WiFi connected.");                    
  Serial.println("IP address: ");                       
  Serial.println(WiFi.localIP());                       
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
