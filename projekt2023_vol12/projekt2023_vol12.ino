#include <Servo.h>
#include <WiFiNINA.h>
#include <SPI.h>
#include <WiFiUdp.h>

char ssid[] = "NOKIA-0823";
char pass[] = "pfQ4H8NFay";
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
unsigned long feedingTimes[arraySize] = {}; //algsed suurused 0-id
//int rawHours[arraySize] = {}; //hiljem ühindub appiga
//int rawMinutes[arraySize] = {}; //hiljem ühindub appiga
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

  //rawHours ja rawMinutes lisamine (tuleb pärast appist)
  /*rawHours[0] = 8;
  rawMinutes[0] = 0;

  rawHours[1] = 11;
  rawMinutes[1] = 30;

  rawHours[2] = 15;
  rawMinutes[2] = 45;

  rawHours[3] = 18;
  rawMinutes[3] = 15;

  rawHours[4] = 22;
  rawMinutes[4] = 0;
  */
  //feedingTimes array-sse aegade lisamine

  //for (int i = 0; i < arraySize; i++) {
    //createFeedingTime(rawHours[i], rawMinutes[i], i);
  //}

  //feedingTimes array printimine
  /*Serial.println("feedingTimes array:");
  for (int i = 0; i < arraySize; i++) {
    Serial.print("Index ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(feedingTimes[i]);
  }*/
}

void loop() {

  WiFiClient client = server.available();
  if (client){
    printWEB(client);
  }

  //hetkel placeholder
  realWorldTime = getCurrentRealWorldTime();


  //kellaaegade checkimine
  for (int i = 0; i < arraySize; i++) {
    if (realWorldTime == feedingTimes[i]) {
      //kausi täitmise alustamine
      startFeedingCycle();
    }
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
                    if (hourIndex != -1 && minuteIndex != -1) {
                        String hourStr = referer.substring(hourIndex + 5, minuteIndex);
                        String minuteStr = referer.substring(minuteIndex + 8);
                        processRequest("hour=" + hourStr + "&minute=" + minuteStr);
                        processed = true;
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
        client.println("<h2>Lisa kellaaeg:</h2>");
        client.println("<form method='GET' action='/submit'>");
        client.println("Tund: <input type='number' name='hour' min='0' max='23' /><br>");
        client.println("Minut: <input type='number' name='minute' min='0' max='59' /><br><br>");
        client.println("<input type='submit' value='OK'>");
        client.println("</form>");
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
    
    if (hourIndex != -1 && minuteIndex != -1) {
        String hourStr = request.substring(hourIndex + 5, minuteIndex);
        String minuteStr = request.substring(minuteIndex + 8);

        int hour = hourStr.toInt();
        int minute = minuteStr.toInt();

        createFeedingTime(hour, minute, arrayIndex);
        Serial.print("Added feeding time: ");
        Serial.print(hour);
        Serial.print(":");
        Serial.println(minute);
        Serial.print("feedingTimes[");
        Serial.print(arrayIndex);
        Serial.print("]: ");
        Serial.println(feedingTimes[arrayIndex]);
        arrayIndex++;
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

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    unsigned long secsSince1900 = highWord << 16 | lowWord;

    const unsigned long seventyYears = 2208988800UL;
    epoch = secsSince1900 - seventyYears;
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




//toitmise alustamise funktsioon
void startFeedingCycle() {
  Serial.println("kausi täitmine in progress");
  
  while (scaleVariable < portionSize) {
    myServo.write(15);
  }
  myServo.write(15); //pärast 90-ne peale
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
