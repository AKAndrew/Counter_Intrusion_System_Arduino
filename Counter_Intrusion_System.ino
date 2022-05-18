#include "RTClib.h"
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
using namespace Adafruit_LittleFS_Namespace;

RTC_DS1307 rtc;
File file(InternalFS);
#define filename "motionlogs.txt"
int buzzer = 13; // buzzer PIN
int R = A0, G = A1, B = A2; // R G B led pins
int motionDetector = 12; // PIR Out pin 
int motionDetected = 0; // PIR status
String systemStates[] = {
  "Disarmed",
  "Starting",
  "Armed",
  "Triggered"
};
int state = 0;
int stateToggler = 7;
int stateTimer = millis();
int lastPIRState;
bool prevPIRState, currentPIRState;
String logging;
String s; char c;

void setup() {
  Serial.begin(115200);
  InternalFS.begin();
  pinMode(buzzer, OUTPUT);
  noTone(buzzer); digitalWrite(buzzer,LOW);
  pinMode(R, OUTPUT);
  pinMode(G, OUTPUT);
  pinMode(B, OUTPUT);
  pinMode(motionDetector, INPUT);
  pinMode(stateToggler, INPUT_PULLUP);

  while (!Serial && millis()-stateTimer < 2000); //waiting 2sec for serial to start
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }
  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
  
  attachInterrupt(stateToggler, stateChangeManual, FALLING);
  Serial.println("Serial ready!");
  Serial.print("System State: ");
  Serial.println(systemStates[state]);
}

void setRGB(int r, int g, int b) {
  analogWrite(R, r);
  analogWrite(G, g);
  analogWrite(B, b);
}

void breatheLED(bool r, bool g, bool b) {
  int val = 1, asc = 1;
  while (val += asc) {
    setRGB(val * r, val * g, val * b);
    if (val == 255) asc = asc * (-1);
    delay(3);
  }
}

String getTimeStamp(){
    DateTime now = rtc.now();
    String datestr, timestr, datetime;
    datestr += now.day(); datestr += '/'; 
    datestr += now.month(); datestr += '/'; 
    datestr += now.year();
    timestr += now.hour(); timestr += ':'; 
    timestr += now.minute(); timestr += ':'; 
    timestr += now.second();
    datetime = datestr + ' ' + timestr;
    return datetime;
}

void printTimeStamp(){
  Serial.println(getTimeStamp());
}

void stateChange() {
    ++state %= 4;
    Serial.print("System State changed to "); Serial.println(systemStates[state]);
    logging = '\n' + getTimeStamp() + " System State changed to " + (String)systemStates[state];
    writeToFlash(logging);
    stateTimer = millis();
}

void stateChangeManual() {
  if (millis() - stateTimer > 1000)
    switch (state) {
    case 0:
      stateChange();
      break;
    default:
      state = 0;
      Serial.print("System State changed manually to "); Serial.println(systemStates[state]);
      logging = '\n' + getTimeStamp() + " System State changed manually to " + (String)systemStates[state];
      writeToFlash(logging);
      stateTimer = millis();
      break;
    }
  else Serial.println("State flood!");
}

bool onemillis(int lastmillis){
  return millis() - lastmillis > 1;
}

void writeToFlash(String logging){ //variable logging is the content to write
  char contents[64];
  int i;
  if (file.open(filename, FILE_O_WRITE)) { //making sure file exists/created and we have write access to file
    strcpy(contents,""); //copy an empty string to contents
    for(i=0; i<=logging.length(); i++) contents[i] = logging[i]; //put each characters of logging into contents
    contents[i] = '\r'; //last character of contents is carrage return to mark the end
    file.write(contents, strlen(contents)); //writing to file contents
    file.close(); //closing file
    //Serial.println(); Serial.print("Written to "); Serial.print(filename); Serial.print(" '"); Serial.print(contents); Serial.println("'"); 
  } else Serial.println("FAILED!");
}

void downloadFile(){
  file.open(filename, FILE_O_READ);
  if(file){
    Serial.println("Download will begin in 3 seconds.\n\tDON'T CLOSE THIS WINDOW!\n");
    stateTimer = millis();
    while(millis() - stateTimer < 3000); //wait for 3 sec to pass
    Serial.println("##### DOWNLOAD BEGIN #####");
    uint32_t readlen=1;
    char buffer[256] = { 0 };
    while(readlen > 0){        
      readlen = file.read(buffer, sizeof(buffer));
      buffer[readlen] = 0;
      Serial.print(buffer);
    }
    file.close();
    Serial.println("\n##### END OF DOWNLOAD #####");
    Serial.flush(); Serial.end();
  }else Serial.println("File doesn't exist!");
}

void loop() {
    switch (state) {
    case 0: //disarmed
      breatheLED(0, 1, 0); //green
      //Serial.println((char)Serial.read());
      while(Serial.available()){
        c = Serial.read();
        if (c != '\n') s+=c;
        else if(s == "download") { downloadFile(); s=""; }
        else { Serial.print("Received: '"); Serial.print(s); Serial.println("'"); s=""; }
      }
      break;
    case 1: //starting
      if (millis() - stateTimer > 600){ // waits 60 seconds
        prevPIRState=0, currentPIRState=0, lastPIRState=millis(); //reset detection variables before arming
        stateChange(); //change state to armed
      }
      breatheLED(0, 0, 1); //blue LED while starting
      break;
    case 2: //armed
      currentPIRState = digitalRead(motionDetector); //constant motion checking
      if(prevPIRState != currentPIRState){ //mark change of state from motion to no-motion and vice-versa
        //Serial.print(' '); Serial.print(millis()-lastPIRState); Serial.println(curr);
        String logging = " " + (String)(millis()-lastPIRState) + "\n" + getTimeStamp() + " " + (String)currentPIRState;
        //place detection variable into string 'logging' to be written to flash
        Serial.print(logging); //output to serial monitor the contents of logging
        writeToFlash(logging); //write to flash value of logging
        lastPIRState = millis(); //timer for how long current state happened for
        prevPIRState = currentPIRState; //previous PIR state becomes current state untill it changes again
      }
      if(currentPIRState==1 && millis()-lastPIRState > 5000) stateChange();
      //if it's been detecting motion for over 5sec continuously, change state to triggered
      break;
    case 3: //alarm triggered
      tone(buzzer,1000); // 1kHz buzzing
      setRGB(255,0,0); // red LED
      delay(500);
      noTone(buzzer); // stop buzzing
      setRGB(0,0,255); // blue LED
      delay(500);
      break;
    default: //other unknown states
      stateChange();
      break;
  }
}
