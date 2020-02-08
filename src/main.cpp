/*
Simple media player using Arduino Nanon, DFPlayer and 2 ky-040 rotary encoders.
Continously play mp3s from one of the mp3 folders on an SD card in arbitrary order.
Switch folders using one of the rotary encoders.
Note that this DFPlayer board seems to introduce noise and pops, according to some fora it was not very well designed.

2020/01/27 Jeroen Veen
Inspired by John Potter (techno-womble)

See .platformio\lib\DFRobotDFPlayerMini_ID1308\doc for datasheet of DFPlayer

Audio data sorted by folder, supports up to 100 folders, every folder can hold up to 255 songs
Files are stored in directories named 01, 02, etc. The directory names must be two digits long with a leading 'zero' i.e. 01 up to a maximum of 99.
Within each directory the audio files must be named 001.mp3, 002.mp3 up to 255.mp3. Each file name is three digits long with leading 'zeros' and an mp3 file extension.

01 - classical
02 - classical instrumental
03 - chanson
04 - 

Normalize files:  mp3gain.exe /r /c D:\01\*.* 
Rename files using a rename python script, something like:
  import glob, os

  data_path = os.path.sep.join(['D:', '02'])
  p = os.path.sep.join([data_path, '*.mp3'])
  file_list = [f for f in glob.iglob(p, recursive=True) if (os.path.isfile(f))]

  for (i,filename) in enumerate(file_list):
      title, ext = os.path.splitext(os.path.basename(filename))
      os.rename(filename, os.path.join(data_path, '{0:03d}'.format(i+1) + ext ))    
*/
#include <Arduino.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <MyRotaryEncoder.h>

boolean serialDebug = true;     // enable/disable troubleshooting messages
boolean randomTrackPlay = true; // randomise the track order

SoftwareSerial mySoftwareSerial(10, 11); // RX, TX (with 1k series resistor)
DFRobotDFPlayerMini myDFPlayer;
RotaryEncoder rotaryEncoder2(A0, A1, A2); // PIN_A (ky-040 clk), PIN_B (ky-040 dt), BUTTON (ky-040 sw)
RotaryEncoder rotaryEncoder1(A3, A4, A5); // PIN_A (ky-040 clk), PIN_B (ky-040 dt), BUTTON (ky-040 sw)
int folderCount = 0, currentFolder = 0;
const uint8_t mutePin = 13;

void playArbitraryTrack(uint8_t folder);
void printDetail(uint8_t type, int value);

void setup()
{
  pinMode(mutePin, OUTPUT);
  digitalWrite(mutePin, true);
  pinMode(9, OUTPUT);
  digitalWrite(9, false);
  pinMode(12, OUTPUT);
  digitalWrite(12, false);
  mySoftwareSerial.begin(9600);
  Serial.begin(115200);
  randomSeed(analogRead(7));

  if (serialDebug)
  {
    Serial.println();
    Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
  }

  if (!myDFPlayer.begin(mySoftwareSerial, false, true))
  { //Use softwareSerial to communicate with mp3.
    if (serialDebug)
    {
      Serial.println(F("Unable to begin:"));
      Serial.println(F("1.Please recheck the connection!"));
      Serial.println(F("2.Please insert the SD card!"));
    }
    while (true)
    {
      delay(0); // Code to compatible with ESP8266 watch dog.
    }
  }
  else if (serialDebug)
    Serial.println(F("DFPlayer Mini online."));

  myDFPlayer.setTimeOut(500); //Set serial communictaion time out 500ms

  // Set different EQ
  myDFPlayer.EQ(DFPLAYER_EQ_NORMAL); // DFPLAYER_EQ_CLASSIC etc

  // Set device we use SD as default
  myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);

  myDFPlayer.volume(1); //Set volume value. From 0 to 30

  // Find the number of folders
  myDFPlayer.waitAvailable(100);
  int fileCount = myDFPlayer.readFileCounts(); //read all file counts in SD card
  myDFPlayer.waitAvailable(100);
  folderCount = myDFPlayer.readFolderCounts() - 1;
  if ((fileCount == -1) || (folderCount == -1))
  {
    if (serialDebug)
    {
      Serial.println(F("No files or folders found"));
      Serial.println(fileCount);
      Serial.println(folderCount);
    }
  }
  else
  {
    // Random start
    currentFolder = random(1, folderCount);
    playArbitraryTrack(currentFolder);
    digitalWrite(mutePin, false);
    if (serialDebug)
    {
      Serial.print(fileCount);
      Serial.print(" found in ");
      Serial.print(folderCount);
      Serial.println(" folders");
    }
  }
}

void loop()
{
  static bool mute = false;

  // Read inputs
  uint8_t type = myDFPlayer.readType();
  uint8_t val = myDFPlayer.read();
  rotaryEncoder1.read();
  rotaryEncoder2.read();

  if (serialDebug)
    printDetail(type, val); //Print the detail message from DFPlayer to handle different errors and states.

  // Play next mp3?
  if ((type == DFPlayerPlayFinished) || (type == DFPlayerError) || (type == DFPlayerCardInserted))
  {
    // myDFPlayer.stop();
    // delay(150);
    playArbitraryTrack(currentFolder);
  }
  int8_t folderButton = rotaryEncoder1.buttonChange();
  if (folderButton > 0) // button pressed
  {
    Serial.println("next");
    playArbitraryTrack(currentFolder);
  }

  // Switch to another mp3 folder?
  int16_t folderChange = rotaryEncoder1.positionChange();
  if (folderChange != 0)
  {
    if (folderChange > 0)
      currentFolder++;
    else
      currentFolder--;
    if (currentFolder < 1)
      currentFolder = folderCount;
    if (currentFolder > folderCount)
      currentFolder = 1;
    playArbitraryTrack(currentFolder);
  }

  // Change volume?
  int16_t volChange = rotaryEncoder2.positionChange();
  if (volChange != 0)
  {
    if (volChange > 0)
    {
      myDFPlayer.volumeUp(); // Volume Up
      if (serialDebug)
        Serial.println("Volume up");
    }
    else
    {
      myDFPlayer.volumeDown(); // Volume Down
      if (serialDebug)
        Serial.println("Volume down");
    }
  }

  // Mute?
  int8_t volButton = rotaryEncoder2.buttonChange();
  if (volButton > 0) // button pressed
  {
    mute = !mute;
    if (mute)
    {
      digitalWrite(mutePin, true);
      // myDFPlayer.sleep(); // Sleep
      // myDFPlayer.disableDAC(); // Disable On-chip DAC
      if (serialDebug)
        Serial.println("muted");
    }
    else
    {
      // myDFPlayer.reset(); // Reset the module
      // myDFPlayer.enableDAC(); // Enable On-chip DAC
      delay(100);
      digitalWrite(mutePin, false);
      if (serialDebug)
        Serial.println("unmuted");
    }
  }
}

void playArbitraryTrack(uint8_t folder)
{
  // myDFPlayer.waitAvailable(100);

  uint8_t trackCount = myDFPlayer.readFileCountsInFolder(folder); // number of tracks in folder
  uint8_t currentTrack = (uint8_t)random(1, trackCount);

  if (serialDebug)
  {
    Serial.print("Playing track ");
    Serial.print(currentTrack);
    Serial.print(" in folder ");
    Serial.println(currentFolder);
  }
  // myDFPlayer.stop();
  myDFPlayer.playFolder(folder, currentTrack); //play specific mp3 in SD:/15/004.mp3; Folder Name(1~99); File Name(1~255)
  // myDFPlayer.start();
}

void printDetail(uint8_t type, int value)
{
  switch (type)
  {
  case TimeOut:
    Serial.println(F("Time Out!"));
    break;
  case WrongStack:
    Serial.println(F("Stack Wrong!"));
    break;
  case DFPlayerCardInserted:
    Serial.println(F("Card Inserted!"));
    break;
  case DFPlayerCardRemoved:
    Serial.println(F("Card Removed!"));
    break;
  case DFPlayerCardOnline:
    Serial.println(F("Card Online!"));
    break;
  case DFPlayerUSBInserted:
    Serial.println("USB Inserted!");
    break;
  case DFPlayerUSBRemoved:
    Serial.println("USB Removed!");
    break;
  case DFPlayerPlayFinished:
    Serial.print(F("Number:"));
    Serial.print(value);
    Serial.println(F(" Play Finished!"));
    break;
  case DFPlayerError:
    Serial.print(F("DFPlayerError:"));
    switch (value)
    {
    case Busy:
      Serial.println(F("Card not found"));
      break;
    case Sleeping:
      Serial.println(F("Sleeping"));
      break;
    case SerialWrongStack:
      Serial.println(F("Get Wrong Stack"));
      break;
    case CheckSumNotMatch:
      Serial.println(F("Check Sum Not Match"));
      break;
    case FileIndexOut:
      Serial.println(F("File Index Out of Bound"));
      break;
    case FileMismatch:
      Serial.println(F("Cannot Find File"));
      break;
    case Advertise:
      Serial.println(F("In Advertise"));
      break;
    default:
      break;
    }
    break;
  default:
    break;
  }
}
