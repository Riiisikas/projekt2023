const int arraySize = 5;
unsigned long feedingTimes[arraySize] = {};
int rawHours[arraySize] = {};
int rawMinutes[arraySize] = {};

void setup() {
  Serial.begin(57600);
  rawHours[0] = 8;
  rawMinutes[0] = 0;

  rawHours[1] = 11;
  rawMinutes[1] = 30;

  rawHours[2] = 15;
  rawMinutes[2] = 45;

  rawHours[3] = 18;
  rawMinutes[3] = 15;

  rawHours[4] = 22;
  rawMinutes[4] = 0;

  for (int i = 0; i < arraySize; i++) {
    createFeedingTime(rawHours[i], rawMinutes[i], i);
  }

  Serial.println("Generated feedingTimes array:");
  for (int i = 0; i < arraySize; i++) {
    Serial.print("Index ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(feedingTimes[i]);
  }
}

void loop() {
  // Your main loop code here
}

void createFeedingTime(int hours, int minutes, int index) {
  unsigned long timeInSeconds = hours * 3600UL + minutes * 60UL;

  Serial.print("Raw Hours: ");
  Serial.println(hours);
  Serial.print("Raw Minutes: ");
  Serial.println(minutes);
  Serial.print("Calculated Time in Seconds: ");
  Serial.println(timeInSeconds);

  feedingTimes[index] = timeInSeconds;
}
