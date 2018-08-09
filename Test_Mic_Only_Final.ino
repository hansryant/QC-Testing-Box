#include <SD.h>
#include <SPI.h>

//Pin Assignment on Teensy 3.5
const int Button1Pin = 2;
const int Button2Pin = 3;
const int Test1Pin_Green = 9;
const int Test1Pin_Red = 10;
const int Test2Pin_Green = 11;
const int Test2Pin_Red = 12;
const int micPin = 15;
const int LED1 = 24;
const int LED2 = 25;

File myFile;

//Variables Declaration for Microphone
int micOutput = 0;
int micOut;
const long sampleTime = 10;
const int micThreshold = 35;

//Initialize Counter
int ctr = 0;
int line = 0;
int sum = 0;
int def = 0;


//Variables for Find Peak-to-Peak Algorithm
long time = 0;
long debounce = 50;
long startTime = 0;
long endTime = 0;

//Variables for CPU Timer
long testTime = 0;
long testStartTime = 0;
long testEndTime = 0;
long testEndTime2 = 0;

//State for Mic Output
int micTest = 0;

//Variables Setup for Buttons
int Button1State = 1;
int Button1Reading;           // the current reading from the input pin
int Button1Previous = 0;
int Button2State = 1;
int Button2Reading;
int Button2Previous = 0;

void setup() {
  // put your setup code here, to run once:
  pinMode(Button1Pin, INPUT);
  pinMode(Button2Pin, INPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(Test1Pin_Green, OUTPUT);
  pinMode(Test1Pin_Red, OUTPUT);
  pinMode(Test2Pin_Green, OUTPUT);
  pinMode(Test2Pin_Red, OUTPUT);
  pinMode(13, OUTPUT);

  micTest = 0;

  //UNCOMMENT THESE TWO LINES FOR TEENSY AUDIO BOARD:
  SPI.setMOSI(7);  // Audio shield has MOSI on pin 7
  SPI.setSCK(14);  // Audio shield has SCK on pin 14

  Serial.begin(2000000);
  /*
    while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
    }
  */
  Serial.print("Initializing SD Card...");
  if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");

  myFile = SD.open("datalog.txt");
  readSD();
}

void loop() {
  digitalWrite(13, HIGH);
  Button1Reading = digitalRead(Button1Pin);
  Button2Reading = digitalRead(Button2Pin);

  if (Button2Reading == 1 && Button2Previous == 0 && millis() - time > debounce) {

    if (Button2State == 1)
      Button2State = 0;
    else
      Button2State = 1;

    time = millis();
  }

  if (Button2State == 0) {
    noiseDebug();
    digitalWrite(LED1, HIGH);
    digitalWrite(LED2, LOW);
    digitalWrite(Test2Pin_Red, LOW);
    digitalWrite(Test2Pin_Green, LOW);
  } else if (Button2State == 1) {
    if (Button1Reading == 1) {
      digitalWrite(LED1, LOW);
      digitalWrite(LED2, LOW);
      digitalWrite(Test1Pin_Red, LOW);
      digitalWrite(Test1Pin_Green, LOW);
      digitalWrite(Test2Pin_Red, LOW);
      digitalWrite(Test2Pin_Green, LOW);
      testStartTime = millis();
      micTest = 0;
    } else {
      testMode();
      digitalWrite(LED1, LOW);
      digitalWrite(Test1Pin_Red, LOW);
      digitalWrite(Test1Pin_Green, LOW);
    }
    Button1Previous = Button1Reading;
  }
  Button2Previous = Button2Reading;
}

//TEST MODE: to run the test
int testMode() {
  //micTest = 0;

  digitalWrite(LED2, HIGH);
  testTime = millis();
  testEndTime = testStartTime + 3000;
  testEndTime2 = testStartTime + 3100;

  noiseTest();

  if (testTime == testEndTime) {
    //digitalWrite(LED1, HIGH);
    if (micTest != 1) {
      digitalWrite(Test2Pin_Red, LOW);
      digitalWrite(Test2Pin_Green, HIGH);
      myFile = SD.open("datalog.txt", FILE_WRITE) ;
      writeSDpassed();
    } else if (micTest == 1) {
      digitalWrite(Test2Pin_Red, HIGH);
      digitalWrite(Test2Pin_Green, LOW);
      myFile = SD.open("datalog.txt", FILE_WRITE) ;
      writeSDdefective();
    }
    //digitalWrite(LED1, HIGH);
  }
  //Serial.println("Mic : " + String(micOutput) + ", Start Time : " + String(testStartTime) + ", Test Time : " + String(testTime) + ", End Time : " + String(testEndTime) + ", Mic Test : " + String(micTest));
}

void writeSDpassed() {

  // if the file opened okay, write to it:
  if (myFile) {
    sum++;
    ctr = sum;
    
    Serial.print("Mic Test = Passed   , Counter:");
    Serial.println(ctr);
    myFile.print("Mic Test = Passed   , Counter:");
    myFile.println(ctr);
    // close the file:
    myFile.close();
    //Serial.println("(Serial) done.");
  } else {
    // if the file didn't open, print an error:
    Serial.println("(Serial) error opening test.txt");
  }

}

void writeSDdefective() {

  // if the file opened okay, write to it:
  if (myFile) {
    sum++;
    ctr = sum;

    Serial.print("Mic Test = Defective, Counter:");
    Serial.println(ctr);
    myFile.print("Mic Test = Defective, Counter:");
    myFile.println(ctr);
    // close the file:
    myFile.close();
    //Serial.println("(Serial) done.");
  } else {
    // if the file didn't open, print an error:
    Serial.println("(Serial) error opening test.txt");
  }

}

void readSD() {
  if (myFile) {
    int test = myFile.available();
    while (myFile.available()) {
      int x = myFile.parseInt(':');
      if (x != 0) {
        line = x;
      }
    }
    sum = line;
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }
}

void noiseTest() {
  micOutput = findPTPAmp();
  if (micOutput >= micThreshold && testTime <= testEndTime) {
    micTest = 1;
  }
}

void noiseDebug() {
  micSensor();
  if (micOutput >= micThreshold) {
    digitalWrite(Test1Pin_Red, HIGH);
    digitalWrite(Test1Pin_Green, LOW);
  } else {
    digitalWrite(Test1Pin_Green, HIGH);
    digitalWrite(Test1Pin_Red, LOW);
  }
}

void micSensor() {
  micOutput = findPTPAmp();
  Serial.println("Microphone: " + String(micOutput));
  //Serial.println("Microphone: " + String(analogRead(micPin)));
}

int findPTPAmp() {
  // Time variables to find the peak-to-peak amplitude
  unsigned long startTime = millis(); // Start of sample window
  unsigned int PTPAmp = 0;

  // Signal variables to find the peak-to-peak amplitude
  unsigned int maxAmp = 0;
  unsigned int minAmp = 1024;

  // Find the max and min of the mic output within the 50 ms timeframe
  while (millis() - startTime < sampleTime)
  {
    micOut = analogRead(micPin);
    if ( micOut < 1024) //prevent erroneous readings
    {
      if (micOut > maxAmp)
      {
        maxAmp = micOut; //save only the max reading
      }
      else if (micOut < minAmp)
      {
        minAmp = micOut; //save only the min reading
      }
    }
  }

  PTPAmp = maxAmp - minAmp; // (max amp) - (min amp) = peak-to-peak amplitude
  //double micOut_Volts = (PTPAmp * 3.3) / 1024; // Convert ADC into voltage

  //Uncomment this line for help debugging (be sure to also comment out the VUMeter function)
  //Serial.println(PTPAmp);

  //Return the PTP amplitude to use in the soundLevel function.
  // You can also return the micOut_Volts if you prefer to use the voltage level.
  return PTPAmp;
}

