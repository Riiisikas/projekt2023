const int arraySize = 5; //tõenäoliselt muutub 3-ks
unsigned long feedingTimes[arraySize] = {}; //algsed suurused 0-id
int rawHours[arraySize] = {}; //hiljem ühindub appiga
int rawMinutes[arraySize] = {}; //hiljem ühindub appiga

unsigned long realWorldTime = 0; //päris aja placeholder
unsigned long scaleVariable = 0; // kaalu placeholder
const unsigned long portionSize = 500; //portsioni suurus

void setup() {
  Serial.begin(57600);

  //rawHours ja rawMinutes lisamine (tuleb pärast appist)
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

  //feedingTimes array-sse aegade lisamine
  for (int i = 0; i < arraySize; i++) {
    createFeedingTime(rawHours[i], rawMinutes[i], i);
  }

  //feedingTimes array printimine
  Serial.println("feedingTimes array:");
  for (int i = 0; i < arraySize; i++) {
    Serial.print("Index ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(feedingTimes[i]);
  }
}

void loop() {
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

//createFeedingTime funktsioon (genereerib sekundites kellaajad)
void createFeedingTime(int hours, int minutes, int index) {
  unsigned long timeInSeconds = hours * 3600UL + minutes * 60UL;

  //kellaaegade hoiustamine array-sse
  feedingTimes[index] = timeInSeconds;
}

//placeholder
unsigned long getCurrentRealWorldTime() {
  return 0;
}

//toitmise alustamise funktsioon
void startFeedingCycle() {
  Serial.println("kausi täitmine in progress");
  
  while (scaleVariable < portionSize) {
    //servo kood (hetkel placeholder)
    scaleVariable += 10;
    delay(100);
  }

  Serial.println("kausi täitmine on lõppenud");
}
