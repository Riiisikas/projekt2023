#include <Servo.h>
#include <WiFiNINA.h>
#include <SPI.h>
#include <WiFiUdp.h>
#include <HX711_ADC.h>

void enable_WiFi();
void connect_WiFi();
void printWifiStatus();
void printWEB(WiFiClient client);
unsigned long getCurrentRealWorldTime();
void sendNTPpacket();
void calibrate();
void processRequest(String request);
void createFeedingTime(int hours, int minutes, int index);
void startFeedingCycle();

const int HX711_dout = 4; //mcu > HX711 dout pin
const int HX711_sck = 5; //mcu > HX711 sck pin

boolean _tare = false;

HX711_ADC LoadCell(HX711_dout, HX711_sck);

const int calVal_eepromAdress = 0;
unsigned long t = 0;
int tareValue = 0;

char ssid[] = "vhkwifi"; //"NOKIA-0823"
char pass[] = "teretibukenekabujalakene"; //"pfQ4H8NFay"
int status = WL_IDLE_STATUS;

IPAddress timeServerIP;
const char* ntpServerName = "time.nist.gov";
const int NTP_PACKET_SIZE = 48;
byte packetBuffer[NTP_PACKET_SIZE];
WiFiUDP udp;

WiFiServer server(80);

WiFiClient client = server.available();


const int arraySize = 5; //tõenäoliselt muutub 3-ks
const int servoPin = 6;
unsigned long feedingTimes[arraySize] = {"a", "a", "a", "a", "a"}; //algsed suurused 0-id
int arrayIndex = 0;

Servo myServo;

unsigned long realWorldTime = 0; //päris aja placeholder
unsigned long scaleVariable = 0; // kaalu placeholder
const unsigned long portionSize = 500; //portsioni suurus

void setup() {
  Serial.begin(57600);
  myServo.attach(servoPin);
  enable_WiFi();
  connect_WiFi();

  server.begin();
  printWifiStatus();

  LoadCell.begin();
}

void loop() {

  WiFiClient client = server.available();
  if (client){
    printWEB(client);
  }

  Serial.println(feedingTimes[0]);
  Serial.println(realWorldTime);
  //hetkel placeholder
  realWorldTime = getCurrentRealWorldTime();

  //kellaaegade checkimine
  for (int i = 0; i < arraySize; i++) {
    if (abs(realWorldTime - feedingTimes[i]) <= 20) {
      //kausi täitmise alustamine
      startFeedingCycle();
    }
  }
  if (_tare == true){
    LoadCell.start(2000, _tare);
    /*if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag()) {
      Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    }
    else {
      LoadCell.setCalFactor(1.0); // user set calibration value (float), initial value 1.0 may be used for this sketch
      Serial.println("Startup is complete");
    }*/
    while (!LoadCell.update());
      calibrate(); //start calibration procedure
  }


  static boolean newDataReady = 0;
  const int serialPrintInterval = 0;
  if (LoadCell.update()) newDataReady = true;

  if (newDataReady) {
    if (millis() > t + serialPrintInterval) {
      float i = LoadCell.getData();
      Serial.print("Load_cell output val: ");
      Serial.println(i);
      newDataReady = 0;
      t = millis();
    }
  }

  if (LoadCell.getTareStatus() == true) {
    Serial.println("Tare complete");
  }

}



void printWifiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  //boardi IP aadress:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");

  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
}

void enable_WiFi() {
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < "1.0.0") {
    Serial.println("Please upgrade the firmware");
  }
}

void connect_WiFi() {
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(10000);
  }
}

void printWEB(WiFiClient client) {
    if (client) {
        Serial.println("New client");
        String referer = "";
        unsigned long timeout = millis() + 5000;
        bool processed = false;

        while (client.connected() && millis() < timeout) {
            if (client.available()) {
                char c = client.read();
                Serial.write(c);
                referer += c;
                if (c == '\n' && referer.startsWith("GET /submit")) {
                    int hourIndex = referer.indexOf("hour=");
                    int minuteIndex = referer.indexOf("&minute=");
                    int feedingTimeIndex = referer.indexOf("&feedingTime=");
                    if (hourIndex != -1 && minuteIndex != -1 && feedingTimeIndex != -1) {
                        String hourStr = referer.substring(hourIndex + 5, minuteIndex);
                        String minuteStr = referer.substring(minuteIndex + 8);
                        String feedingTimeStr = referer.substring(feedingTimeIndex + 13);
                        int selectedFeedingTimeIndex = feedingTimeStr.toInt();
                        Serial.print("Selected feeding time index: ");
                        Serial.println(selectedFeedingTimeIndex);
                        processRequest("hour=" + hourStr + "&minute=" + minuteStr + "&feedingTime=" + String(selectedFeedingTimeIndex));
                        processed = true;
                    }
                }

                else if (c == '\n' && referer.startsWith("GET /tare")) {
                  _tare = true;
                }
                else if (c == '\n' && referer.startsWith("GET /calculate")) {
                  int massIndex = referer.indexOf("mass=");
                    if (massIndex != -1) {
                        String massStr = referer.substring(massIndex + 5);
                        tareValue = massStr.toInt();
                    }
                }
                if (c == '\n') {
                    referer = "";
                }
            }
        }

        client.println("HTTP/1.1 200 OK");
        client.println("Content-type: text/html");
        client.println();
        client.println("<!DOCTYPE html>");
        client.println("<html>");
        client.println("<head>");
        client.println("<title>add time</title>");
        client.println("</head>");
        client.println("<body>");
        client.println("<h2>Massi arvutamine:</h2>");
        client.println("<form method='GET' action='/tare'>"); // Form for tare button
        client.println("<input type='submit' value='Alusta'>"); // Button for tare
        client.println("</form>");
        if (_tare) {
            client.println("<h3>Asetage objekt alusele ja sisestage selle mass siia:</h3>");
            client.println("<form method='GET' action='/calculate'>");
            client.println("Mass: <input type='text' name='mass' pattern='[0-9]+' title='Only numbers are allowed' required><br>");
            client.println("<input type='submit' value='OK'>");
            client.println("</form>");
        }
        client.println("<h2>Lisa kellaaeg:</h2>");
        client.println("<form method='GET' action='/submit'>");
        client.println("Tund: <input type='number' name='hour' min='0' max='23' /><br>");
        client.println("Minut: <input type='number' name='minute' min='0' max='59' /><br>");
        client.println("Vali kellaaja koht (0-4): <input type='number' name='feedingTime' min='0' max='4' /><br><br>");
        client.println("<input type='submit' value='OK'>");
        client.println("</form>");

        client.println("<h3>Praegused kellaajad:</h3>");
        unsigned long currentSeconds = getCurrentRealWorldTime();
        int currentHour = currentSeconds / 3600;
        int currentMinute = (currentSeconds % 3600) / 60;
        client.print("Praegune aeg: ");
        if (currentHour < 10) client.print("0");
        client.print(currentHour);
        client.print(":");
        if (currentMinute < 10) client.print("0");
        client.println(currentMinute);
        for (int i = 0; i < arraySize; i++) {
            client.print("Kellaaeg ");
            client.print(i);
            client.print(": ");
            int hour = feedingTimes[i] / 3600;
            int minute = (feedingTimes[i] % 3600) / 60;
            if (hour < 10) client.print("0");
            client.print(hour);
            client.print(":");
            if (minute < 10) client.print("0");
            client.println(minute);
        }

        client.println("</body>");
        client.println("</html>");
        client.println();

        client.stop();
        Serial.println("Client disconnected");

        if (processed) {
            processed = false;
        }
    }
}


void processRequest(String request) {
    int hourIndex = request.indexOf("hour=");
    int minuteIndex = request.indexOf("&minute=");
    int feedingTimeIndex = request.indexOf("&feedingTime=");
    
    if (hourIndex != -1 && minuteIndex != -1 && feedingTimeIndex != -1) {
        String hourStr = request.substring(hourIndex + 5, minuteIndex);
        String minuteStr = request.substring(minuteIndex + 8);
        String feedingTimeStr = request.substring(feedingTimeIndex + 13);

        int hour = hourStr.toInt();
        int minute = minuteStr.toInt();
        int index = feedingTimeStr.toInt();

        createFeedingTime(hour, minute, index);
        Serial.print("Added feeding time: ");
        Serial.print(hour);
        Serial.print(":");
        Serial.print(minute);
        Serial.print(" to index ");
        Serial.println(index);
    }
}


//createFeedingTime funktsioon (genereerib sekundites kellaajad)
void createFeedingTime(int hours, int minutes, int index) {
  unsigned long timeInSeconds = hours * 3600UL + minutes * 60UL;

  //kellaaegade hoiustamine array-sse
  feedingTimes[index] = timeInSeconds;
}

unsigned long getCurrentRealWorldTime() {
  unsigned long epoch = 0;

  udp.begin(2390);
  sendNTPpacket();

  delay(1000);
  int cb = udp.parsePacket();
  if (cb) {
    udp.read(packetBuffer, NTP_PACKET_SIZE);

    unsigned long secsSince1900 = 0;
    for (int i = 0; i < 4; i++) {
      secsSince1900 = (secsSince1900 << 8) | packetBuffer[40 + i];
    }

    const unsigned long seventyYears = 2208988800UL;
    epoch = secsSince1900 - seventyYears;

    epoch += 7200;
    epoch %= 86400;
  }

  udp.stop();
  return epoch;
}


void sendNTPpacket() {
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;
  packetBuffer[1] = 0;
  packetBuffer[2] = 6;
  packetBuffer[3] = 0xEC;
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  udp.beginPacket(ntpServerName, 123);
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

void calibrate(){
  float known_mass = tareValue;
  LoadCell.refreshDataSet();
  float newCalibrationValue = LoadCell.getNewCalibration(known_mass);
}


//toitmise alustamise funktsioon
void startFeedingCycle() {
  Serial.println("kausi täitmine in progress");
  
  while (scaleVariable < portionSize) {
    myServo.write(10);
  }
  myServo.write(0); //pärast 90-ne peale
  Serial.println("kausi täitmine on lõppenud");
}


//https://github.com/arduino/ArduinoCore-megaavr/issues/27
//https://forum.arduino.cc/t/system-time-on-wifi-rev2-is-on-steroids/1179917
//https://www.nonscio.com/blog/installing-esp8266-libraries-to-the-arduino-ide
//https://docs.arduino.cc/tutorials/uno-wifi-rev2/uno-wifi-r2-hosting-a-webserver/
//https://sentry.io/answers/html-text-input-allow-only-numeric-input/
//https://docs.arduino.cc/tutorials/uno-wifi-rev2/uno-wifi-r2-web-server-ap-mode/
//https://forum.arduino.cc/t/client-print-f-not-displaying-web-page/490152
//https://stackoverflow.com/posts/13952761/edit
