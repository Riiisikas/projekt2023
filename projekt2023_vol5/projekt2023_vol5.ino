const int arraySize = 5;
unsigned long feedingTimes[arraySize] = {};
int rawHours[arraySize] = {};
int rawMinutes[arraySize] = {};

unsigned long realWorldTime = 0;
unsigned long scaleVariable = 0;
const unsigned long portionSize = 500;

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
  realWorldTime = getCurrentRealWorldTime();

  for (int i = 0; i < arraySize; i++) {
    if (realWorldTime == feedingTimes[i]) {
      startFeedingCycle();
    }
  }
}

void createFeedingTime(int hours, int minutes, int index) {
  unsigned long timeInSeconds = hours * 3600UL + minutes * 60UL;
  feedingTimes[index] = timeInSeconds;
}

unsigned long getCurrentRealWorldTime() {
  return 0;
}

void startFeedingCycle() {
  Serial.println("Feeding cycle started!");
  while (scaleVariable < portionSize) {
    scaleVariable += 10;
    delay(100);
  }

  Serial.println("Feeding cycle completed!");
}
