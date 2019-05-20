#include <SPI.h>
#include <SD.h>
#include <LiquidCrystal.h>

/* Arduino SD Card MIDI Song Player - by Eric Rangell - Release 1.00 - 2012-JUN-04
 * Adapted from open-source Arduino MIDI code.  Published in the public domain.
 *
 * Use DIP switches to select song 001.MID to 255.MID in the root of the SD card.  Reset button plays song.
 * Song 000 does note off 0-127 on channels 0-15.
 * Set debugSong = true to debug songs that don't play.
 * For Type 1 Midi Files, program will attempt to play data from track #2.
 * It does not mix down tracks in Type 1 Midi Files, so they should be converted to Type 0.
 * 
 * Modification History:
 * 2012-MAY-28: fixed bug with long songs that didn't play
 * 2012-JUN-02: changed pin assignments: 8->9 9->10
 * 2012-JUN-07: updated for Arduino 1.0
 */

#define HEADER_CHUNK_ID 0x4D546864  // MThd
#define TRACK_CHUNK_ID 0x4D54726B   // MTrk
#define SD_BUFFER_SIZE 512
#define DELTA_TIME_VALUE_MASK 0x7F
#define DELTA_TIME_END_MASK 0x80
#define DELTA_TIME_END_VALUE 0x80
#define EVENT_TYPE_MASK 0xF0
#define EVENT_CHANNEL_MASK 0x0F
#define NOTE_OFF_EVENT_TYPE 0x80
#define NOTE_ON_EVENT_TYPE 0x90
#define KEY_AFTERTOUCH_EVENT_TYPE 0xA0
#define CONTROL_CHANGE_EVENT_TYPE 0xB0
#define PROGRAM_CHANGE_EVENT_TYPE 0xC0
#define CHANNEL_AFTERTOUCH_EVENT_TYPE 0xD0
#define PITCH_WHEEL_CHANGE_EVENT_TYPE 0xE0
#define META_EVENT_TYPE 0xFF
#define SYSTEM_EVENT_TYPE 0xF0
#define META_SEQ_COMMAND 0x00
#define META_TEXT_COMMAND 0x01
#define META_COPYRIGHT_COMMAND 0x02
#define META_TRACK_NAME_COMMAND 0x03
#define META_INST_NAME_COMMAND 0x04
#define META_LYRICS_COMMAND 0x05
#define META_MARKER_COMMAND 0x06
#define META_CUE_POINT_COMMAND 0x07
#define META_CHANNEL_PREFIX_COMMAND 0x20
#define META_END_OF_TRACK_COMMAND 0x2F
#define META_SET_TEMPO_COMMAND 0x51
#define META_SMPTE_OFFSET_COMMAND 0x54
#define META_TIME_SIG_COMMAND 0x58
#define META_KEY_SIG_COMMAND 0x59
#define META_SEQ_SPECIFIC_COMMAND 0x7F

/* PINS */

/* Charlieplexed LEDs */
int pinA = 2;
int pinB = 3;
int pinC = 5;  // 3 pins can control 6 LEDs
int pinD = 6;  // 4 pins can control 12 LEDs (use for testing duty cycles)
int pinE = 7;  // 5 pins can control 20 LEDs
int pinF = 8;  // 6 pins can control 30 LEDs  (3x3x3 cube)
int pinG = 9;  // 7 pins can control 42 LEDs
int pinH = 10;  // 8 pins 8can control 56 LEDs
int pinI = 11; // 9 pins can control 72 LEDs (4x4x4 cube)
int pinJ = 12;  // 10 pins can control 90 Charlieplexed LED's (Piano display with 2 pedals)
int pinMaxUsed = pinG;

const int b7 = 35;     // the numbers of the DIP switch pins
const int b6 = 37;
const int b5 = 39;
const int b4 = 41;
const int b3 = 43;
const int b2 = 45;
const int b1 = 47;
const int b0 = 49;

// change this to match your SD shield or module;
// Arduino Ethernet shield: pin 4
// Adafruit SD shields and modules: pin 10
// Sparkfun SD shield: pin 8
 const int chipSelect = 4;  
 const int ssPin = 53;  // must be set to output for SPI use

//LCD:
 const int LCDE = 38;
 const int LCDRS = 40;
 const int LCDD7 = 42;
 const int LCDD6 = 44;
 const int LCDD5 = 46;
 const int LCDD4 = 48;
 
 LiquidCrystal lcd(LCDRS, LCDE, LCDD4, LCDD5, LCDD6, LCDD7);


boolean waiting = true;
boolean get = false;
boolean post = false;
boolean boundary = false;

boolean file_opened = false;
boolean last_block = false;
boolean file_closed = false;
boolean logging = true;
char currentSong[13];
uint16_t bufsiz=SD_BUFFER_SIZE;
uint8_t buf1[SD_BUFFER_SIZE];
uint16_t bytesread1;
uint16_t bufIndex;
uint8_t currentByte;
uint8_t previousByte;

int format;
int trackCount;
int timeDivision;

unsigned long deltaTime;
int eventType;
int eventChannel;
int parameter1;
int parameter2;

// The number of microseconds per quarter note (i.e. the current tempo)
long microseconds = 500000;
int index = 0;
unsigned long accDelta = 0;

boolean firstNote = true;
int currFreq = -1;
unsigned long lastMillis;

int bs0 = 0;
int bs1 = 0;
int bs2 = 0;
int bs3 = 0;
int bs4 = 0;
int bs5 = 0;
int bs6 = 0;
int bs7 = 0;
int inbyte = 0;

boolean debugSong = false;
boolean debugCharlie = true;

long charlieDisplayPattern= 0; 
long charlieDisplayPattern2 = 0; 
long charlieDisplayPattern3 = 0;

int charlieVelocity = 255; // brightness of current LED
int charlieNoteBottom = 41;
int charlieNoteTop = 83;

//8 observable LED light levels.  Consider logarithmic assignment of MIDI velocity*volume to LED light levels.
int vel7 = 16*16-1;
int vel6 = 13*16-1;
int vel5 = 10*16-1;
int vel4 = 7*16-1;
int vel3 = 5*16-1;
int vel2 = 3*16-1;
int vel1 = 1*16-1;


/************ SDCARD STUFF ************/
Sd2Card card;
SdVolume volume;
SdFile root;
SdFile file;

// store error strings in flash to save RAM
#define error(s) error_P(PSTR(s))

void error_P(const char* str) {
 PgmPrint("error: ");
 SerialPrintln_P(str);
 if (card.errorCode()) {
   PgmPrint("SD error: ");
   Serial.print(card.errorCode(), HEX);
   Serial.print(',');
   Serial.println(card.errorData(), HEX);
 }
 while(1);
}
/************ SDCARD STUFF ************/

void setPinMode (int pin, int firstpin, int secondpin)
{
    if ((pin == firstpin) || (pin == secondpin))
    {
      pinMode(pin, OUTPUT);           
    }
    else
    {
      pinMode(pin, INPUT);
    }

}
void Charlie(int firstpin, int secondpin, int dly)
{
  setPinMode(pinA, firstpin, secondpin);
  setPinMode(pinB, firstpin, secondpin);
  setPinMode(pinC, firstpin, secondpin);
  setPinMode(pinD, firstpin, secondpin);
  setPinMode(pinE, firstpin, secondpin);
  setPinMode(pinF, firstpin, secondpin);
  setPinMode(pinG, firstpin, secondpin);

  analogWrite(firstpin, charlieVelocity);  //velocity must be 0-255 - this sets the PWM rate or LED brightness 
  digitalWrite(secondpin, LOW);
  if (dly > 0)
  {
    delay(dly);
  }
  digitalWrite(firstpin, LOW);
  digitalWrite(secondpin, LOW);  
}

void CharlieDisplay(long testNum, long testNum2, long testNum3, int msecdelay)  //needs to be configured based on hardware connections
{
      if (testNum % 2 == 1)
      {
        Charlie(pinB,pinA,msecdelay); //1
      }
      testNum = testNum / 2;
      if (testNum % 2 == 1)
      {
        Charlie(pinA,pinB,msecdelay); //2
      }
      testNum = testNum / 2;
      if (testNum % 2 == 1)
      {      
        Charlie(pinC,pinA,msecdelay); //3
      }
      testNum = testNum / 2;
      if (testNum % 2 == 1)
      {
        Charlie(pinC,pinB,msecdelay); //4
      }
      testNum = testNum / 2;
      if (testNum % 2 == 1)
      {
        Charlie(pinB,pinC,msecdelay); //5
      }
      testNum = testNum / 2;
      if (testNum % 2 == 1)
      {
        Charlie(pinA,pinC,msecdelay); //6
      }
      testNum = testNum / 2;
      if (testNum % 2 == 1)
      {
        Charlie(pinA,pinD,msecdelay); //7  
      }
      testNum = testNum / 2;
      if (testNum % 2 == 1)
      {
        Charlie(pinD,pinA,msecdelay); //8       
      }      
      testNum = testNum / 2;
      if (testNum % 2 == 1)
      {
        Charlie(pinD,pinB,msecdelay); //9 
      }      
      testNum = testNum / 2;
      if (testNum % 2 == 1)
      {
        Charlie(pinB,pinD,msecdelay); //10       
      }      
      testNum = testNum / 2;
      if (testNum % 2 == 1)
      {
        Charlie(pinD,pinC,msecdelay); //11 
      }      
      testNum = testNum / 2;
      if (testNum % 2 == 1)
      {
        Charlie(pinC,pinD,msecdelay); //12       
      }
      
      testNum = testNum / 2;
      if (testNum % 2 == 1)
      {
        Charlie(pinE ,pinA ,msecdelay); //13
      }
      testNum = testNum / 2;
      if (testNum % 2 == 1)
      {
        Charlie(pinE ,pinB ,msecdelay); //14    
      }
      testNum = testNum / 2;
      if (testNum % 2 == 1)
      {
        Charlie(pinE ,pinC ,msecdelay); //15    
      }
      testNum = testNum / 2;
      if (testNum % 2 == 1)
      {
        Charlie(pinE ,pinD ,msecdelay); //16    
      }
      testNum = testNum / 2;
      if (testNum % 2 == 1)
      {
        Charlie(pinA ,pinE ,msecdelay); //17    
      }
      testNum = testNum / 2;
      if (testNum % 2 == 1)
      {
        Charlie(pinB ,pinE ,msecdelay); //18    
      }
      testNum = testNum / 2;
      if (testNum % 2 == 1)
      {
        Charlie(pinC ,pinE ,msecdelay); //19    
      }
      testNum = testNum / 2;
      if (testNum % 2 == 1)
      {
        Charlie(pinD ,pinE ,msecdelay); //20    
      }
      testNum = testNum / 2;
      if (testNum % 2 == 1)
      {
        Charlie(pinE ,pinF ,msecdelay); //21    
      }
      testNum = testNum / 2;
      if (testNum % 2 == 1)
      {
        Charlie(pinF ,pinE ,msecdelay); //22    
      }
      testNum = testNum / 2;
      if (testNum % 2 == 1)
      {
        Charlie(pinF ,pinA ,msecdelay); //23    
      }
      testNum = testNum / 2;
      if (testNum % 2 == 1)
      {
        Charlie(pinF ,pinB ,msecdelay); //24    
      }      
      testNum = testNum / 2;
      if (testNum % 2 == 1)
      {
        Charlie(pinF ,pinC ,msecdelay); //25    
      }
      testNum = testNum / 2;
      if (testNum % 2 == 1)
      {
        Charlie(pinF ,pinD ,msecdelay); //26    
      }
      testNum = testNum / 2;
      if (testNum % 2 == 1)
      {
        Charlie(pinA ,pinF ,msecdelay); //27    
      }
      testNum = testNum / 2;
      if (testNum % 2 == 1)
      {
        Charlie(pinB ,pinF ,msecdelay); //28    
      }
      testNum = testNum / 2;
      if (testNum % 2 == 1)
      {
        Charlie(pinC ,pinF ,msecdelay); //29    
      }
      testNum = testNum / 2;
      if (testNum % 2 == 1)
      {
        Charlie(pinD ,pinF ,msecdelay); //30    
      }


      if (testNum2 % 2 == 1)
      {
        Charlie(pinG ,pinA ,msecdelay); //31    
      }
      testNum2 = testNum2 / 2;
      if (testNum2 % 2 == 1)
      {
        Charlie(pinG ,pinB ,msecdelay); //32    
      }      
      testNum2 = testNum2 / 2;
      if (testNum2 % 2 == 1)
      {
        Charlie(pinG ,pinC ,msecdelay); //33    
      }
      testNum2 = testNum2 / 2;
      if (testNum2 % 2 == 1)
      {
        Charlie(pinG ,pinD ,msecdelay); //34    
      }
      testNum2 = testNum2 / 2;
      if (testNum2 % 2 == 1)
      {
        Charlie(pinG ,pinE ,msecdelay); //35    
      }
      testNum2 = testNum2 / 2;
      if (testNum2 % 2 == 1)
      {
        Charlie(pinG ,pinF ,msecdelay); //36    
      }
      testNum2 = testNum2 / 2;
      if (testNum2 % 2 == 1)
      {
        Charlie(pinA ,pinG ,msecdelay); //37    
      }
      testNum2 = testNum2 / 2;
      if (testNum2 % 2 == 1)
      {
        Charlie(pinB ,pinG ,msecdelay); //38    
      }
      testNum2 = testNum2 / 2;
      if (testNum2 % 2 == 1)
      {
        Charlie(pinC ,pinG ,msecdelay); //39    
      }
      testNum2 = testNum2 / 2;
      if (testNum2 % 2 == 1)
      {
        Charlie(pinD ,pinG ,msecdelay); //40    
      }
      testNum2 = testNum2 / 2;
      if (testNum2 % 2 == 1)
      {
        Charlie(pinE ,pinG ,msecdelay); //41    
      }
      testNum2 = testNum2 / 2;
      if (testNum2 % 2 == 1)
      {
        Charlie(pinF ,pinG ,msecdelay); //42    
      }
      
}

void setup() {
  lcd.begin(16,2);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(" # #  # # # ");
  lcd.setCursor(0,1);
  lcd.print("C D EF G A B");

  charlieVelocity = 255;
  
  if (debugCharlie)
  {
    int spdly = 1;    
    for (int mm=1; mm <= 4; mm++)
    {    
      charlieDisplayPattern = 1;
      charlieDisplayPattern2 = 0;
      charlieDisplayPattern3 = 0;
      for (int pow = 1; pow <= 30; pow++)
      {
            CharlieDisplay(charlieDisplayPattern,charlieDisplayPattern2,charlieDisplayPattern3,spdly);
            charlieDisplayPattern *= 2;
      }    
      long saveCharlieDisplayPattern = charlieDisplayPattern;
      charlieDisplayPattern = 0;
      charlieDisplayPattern2 = 1;
      charlieDisplayPattern3 = 0;
      for (int pow = 1; pow <= 12; pow++)
      {
            CharlieDisplay(charlieDisplayPattern,charlieDisplayPattern2,charlieDisplayPattern3,spdly);
            charlieDisplayPattern2 *= 2;
      }
      charlieDisplayPattern2 /= 2;
      for (int pow = 1; pow <= 11; pow++)
      {
            CharlieDisplay(charlieDisplayPattern,charlieDisplayPattern2,charlieDisplayPattern3,spdly);
            charlieDisplayPattern2 /= 2;
      }  
      charlieDisplayPattern = saveCharlieDisplayPattern / 2;
      charlieDisplayPattern2 = 0;    
      charlieDisplayPattern3 = 0;
      for (int pow = 1; pow <= 30; pow++)
      {
            CharlieDisplay(charlieDisplayPattern,charlieDisplayPattern2,charlieDisplayPattern3,spdly);
            charlieDisplayPattern /= 2;
      }
    }
  }

  lcd.clear();  
  
  pinMode(b0, INPUT);     
  pinMode(b1, INPUT);     
  pinMode(b2, INPUT);     
  pinMode(b3, INPUT);     
  pinMode(b4, INPUT);     
  pinMode(b5, INPUT);     
  pinMode(b6, INPUT);     
  pinMode(b7, INPUT);     
  
  bs0 = digitalRead(b0);
  bs1 = digitalRead(b1);
  bs2 = digitalRead(b2);
  bs3 = digitalRead(b3);
  bs4 = digitalRead(b4);
  bs5 = digitalRead(b5);
  bs6 = digitalRead(b6);
  bs7 = digitalRead(b7);
  
  //for testing 
  //bs0 = 1; bs1 = 0; bs2 = 1; bs3 = 0; bs4 = 0; bs5 = 1; bs6 = 0; bs7 = 0;
   
  if (debugSong)
 {
   Serial.begin(9600);
   Serial1.begin(31250);
 }
 else
 {
   Serial1.begin(31250);
 }

 // initialize the SD card at SPI_HALF_SPEED to avoid bus errors with
 // breadboards.  use SPI_FULL_SPEED for better performance.
 pinMode(ssPin, OUTPUT);                       // set the SS pin as an output (necessary!)
 //digitalWrite(ssPin, HIGH);                    // but turn off the W5100 chip!

 SD.begin(chipSelect);  // select SD card on Ethernet Shield
 
 if (!card.init(SPI_HALF_SPEED, chipSelect)) error("card.init failed!");
 // initialize a FAT volume
 if (!volume.init(card)) error("vol.init failed!");

 if (debugSong)
 {
   PgmPrint("Volume is FAT");
   Serial.println(volume.fatType(),DEC);
   Serial.println();
 }
 
 if (!root.openRoot(volume)) error("openRoot failed");

  inbyte=0;
  if (bs0 == HIGH) {     
    inbyte += 1;
  } 
  if (bs1 == HIGH) {     
    inbyte += 2;
  } 
  if (bs2 == HIGH) {     
    inbyte += 4;
  } 
  if (bs3 == HIGH) {     
    inbyte += 8;
  } 
  if (bs4 == HIGH) {     
    inbyte += 16;
  } 
  if (bs5 == HIGH) {     
    inbyte += 32;
  } 
  if (bs6 == HIGH) {     
    inbyte += 64;
  } 
  if (bs7 == HIGH) {     
    inbyte += 128;
  } 
  
 SendAllNotesOff();
 int currentSongNum = inbyte;
 if (currentSongNum > 0)
 {
     currentSong[2] = currentSongNum%10 + '0';
     currentSongNum = currentSongNum / 10;
     currentSong[1] = currentSongNum%10 + '0';
     currentSongNum = currentSongNum / 10;
     currentSong[0] = currentSongNum%10 + '0';
     currentSong[3] = '.';
     currentSong[4] = 'M';
     currentSong[5] = 'I';
     currentSong[6] = 'D';
     currentSong[7] = '\0';
     
     if (debugSong)
     {
       Serial.print("Current Song #: ");
       Serial.println(currentSong);
     }
     
     //Phase processing
     while (!file_closed)
     {
        processChunk(); // header chunk
        
        if(getFileFormat() == 0) {
          logs("MIDI file format = 0");
          int trackCount = getTrackCount();
          logi("Track Count=",trackCount);
          
          /*
          if (debugSong)
          {
            file_closed = true;
            for (int i=0; i<512; i++)
            {
              int b=buf1[i];
              Serial.print(b,HEX);
              Serial.print(" ");
              if (((i+1)/16)==((i+1)%16))
              {
                Serial.println();
              }
            }
          }
          else
          {
            */
            for(int i = 0; i < getTrackCount(); i++) {
              processChunk();
            }
            if (debugSong)
            {
              file_closed = true;
            }
            /*
          }   */       
        }
        else {
          logs("MIDI file not format 0.");
        }
     }
   }
 }

void SendAllNotesOff() 
{
  for (int channel = 0; channel < 16; channel++)
  {
    for (int note = 00; note < 128; note++) 
    {
      midiShortMsg(0x90+channel, note, 0x00);   
    }
  }
}

void logs(char* string) {
  if(!debugSong)
    return;
  
  Serial.println(string);
}

void logi(char* label, int data) {
  if(!debugSong)
    return;
  
  Serial.print(label);
  Serial.print(": ");
  Serial.println(data);
}

void logl(char* label, long data) {
  if(!debugSong)
    return;
  
  Serial.print(label);
  Serial.print(": ");
  Serial.println(data,HEX);
}


void logDivision(boolean major) {
  if(!debugSong)
    return;
  
  if(major) {
    Serial.println("===========================");    
  }
  else {
    Serial.println("----------------------");
  }
}


int readInt()
{
  return readByte() << 8 | readByte();
}


long readLong() {
  return (long) readByte() << 24 | (long) readByte() << 16 | (long)readByte() << 8 | (long)readByte();
}


byte getLastByte()
{
  return currentByte;
}

byte readByte()
{
  ReadMidiByte();
  return currentByte;
}


void ReadMidiByte()
{
  if (!file_opened)
  {
     if (file.open(root, currentSong, O_READ)) 
     {
        logs("Opened file");
        file_opened = true;
        ReadNextBlock();
     }
  }
  previousByte = currentByte;
  currentByte = buf1[bufIndex];
  bufIndex++;
  if (bufIndex >= bytesread1)
  {
       if (last_block)
       {
          file.close();   
          file_closed = true;
          logs("\nDone");         
       }
       else
       {
           ReadNextBlock();
       }     
  }
}

void ReadNextBlock()
{
        bytesread1 = file.read(buf1,bufsiz);
        bufIndex = 0;
        if (bytesread1 < bufsiz)
        {
           logs("Last Block");
           last_block = true;
        }
        else
        {
          logi("BUF1 Bytes read",bytesread1);
        }
}

void processByte(uint8_t b)
{
  if (debugSong)
  {
    Serial.print(b,HEX);
    Serial.print(" ");
  }
}

void processChunk() {
  boolean valid = true;
  
  long chunkID = readLong();
  long size = readLong();
  
  logDivision(true);
  logi("Chunk ID", chunkID);
  logl("Chunk Size", size);
  
  if(chunkID == HEADER_CHUNK_ID) {
    processHeader(size);
    
    logi("File format", getFileFormat());
    logi("Track count", getTrackCount());
    logi("Time division", getTimeDivision());
  }
  else if(chunkID == TRACK_CHUNK_ID) {
    processTrack(size);
  }
}


/*
 * Parses useful information out of the MIDI file header.
 */
void processHeader(long size) {
  // size should always be 6
  // do we want to bother checking?
  
  format = readInt();
  trackCount = readInt();
  timeDivision = readInt();
  
  //logs("Processed header info.");
}

int getFileFormat() {
  return format;
}

int getTrackCount() {
  return trackCount;
}

int getTimeDivision() {
  return timeDivision;
}

/*
 * Loops through a track of the MIDI file, processing the data as it goes.
 */

void processTrack(long size) {
  long counter = 0;

  while(counter < size) {
    //logl("Track counter", counter);
    counter += processEvent();
  }
  
  
}


/*
 * Reads an event type code from the currently open file, and handles it accordingly.
 */
int processEvent() {
  //logDivision(false);
  
  int counter = 0;
  deltaTime = 0;
  
  int b;
  
  do {
    b = readByte();
    counter++;
    
    deltaTime = (deltaTime << 7) | (b & DELTA_TIME_VALUE_MASK);
  } while((b & DELTA_TIME_END_MASK) == DELTA_TIME_END_VALUE);
  
  //logi("Delta", deltaTime);
  
  b = readByte();
    counter++;

  boolean runningMode = true;
  // New events will always have a most significant bit of 1
  // Otherwise, we operate in 'running mode', whereby the last
  // event command is assumed and only the parameters follow
  if(b >= 128) {
    eventType = (b & EVENT_TYPE_MASK) >> 4;
    eventChannel = b & EVENT_CHANNEL_MASK;
    runningMode = false;
  }
  
  //logi("Event type", eventType);
  //logi("Event channel", eventChannel);
  
  // handle meta-events and track events separately
  if(eventType == (META_EVENT_TYPE & EVENT_TYPE_MASK) >> 4
     && eventChannel == (META_EVENT_TYPE & EVENT_CHANNEL_MASK)) {
    counter += processMetaEvent();
  }
  else {
    counter += processTrackEvent(runningMode, b);
  }
  
  return counter;
}

/*
 * Reads a meta-event command and size from the file, performing the appropriate action
 * for the command.
 *
 * NB: currently, only tempo changes are handled - all else is useless for our organ.
 */
int processMetaEvent() {
  int command = readByte();
  int size = readByte();
  
  //logi("Meta event length", size);
  
  for(int i = 0; i < size; i++) {
    byte data = readByte();
    
    switch(command) {
      case META_SET_TEMPO_COMMAND:
        processTempoEvent(i, data);
    }
  }
  
  return size + 2;
}

/*
 * Reads a track event from the file, either as a full event or in running mode (to
 * be determined automatically), and takes appropriate playback action.
 */
int processTrackEvent(boolean runningMode, int lastByte) {
  int count = 0;
  
  if(runningMode) {
    parameter1 = getLastByte();
  }
  else {
    parameter1 = readByte(); 
    count++;
  }
  
  //logi("Parameter 1", parameter1);
  
  int eventShift = eventType << 4;

  parameter2 = -2;  
  switch(eventShift) {
    case NOTE_OFF_EVENT_TYPE:
    case NOTE_ON_EVENT_TYPE:
    case KEY_AFTERTOUCH_EVENT_TYPE:
    case CONTROL_CHANGE_EVENT_TYPE:
    case PITCH_WHEEL_CHANGE_EVENT_TYPE:
    default:
      parameter2 = readByte();
      count++;

      //logi("Parameter 2", parameter2);
      
      break;
    case PROGRAM_CHANGE_EVENT_TYPE:
    case CHANNEL_AFTERTOUCH_EVENT_TYPE:
      parameter2 = -1;
      break;
  }
  
  if (parameter2 >= 0)
  {
    processMidiEvent(deltaTime, eventType*16+eventChannel, parameter1, parameter2);
  }
  else if (parameter2 == -1)
  {
    process2ByteMidiEvent(deltaTime, eventType*16+eventChannel, parameter1);
  }
  else {
    addDelta(deltaTime);
  }
  
  return count;
}


/*
 * Handles a tempo event with the given values.
 */
void processTempoEvent(int paramIndex, byte param) {
  byte bits = 16 - 8*paramIndex;
  microseconds = (microseconds & ~((long) 0xFF << bits)) | ((long) param << bits);
  
  //lcd.setCursor(11,0);
  //lcd.print("     ");
  //lcd.setCursor(11,0);
  //lcd.print(microseconds,HEX);

  //Serial.print("TEMPO:");
  //Serial.println(microseconds);
}
  
long getMicrosecondsPerQuarterNote() {
  return microseconds;
}

void addDelta(unsigned long delta) {
  accDelta = accDelta + delta;
}

void resetDelta() {
  accDelta = 0;
}

void processMidiEvent(unsigned long delta, int channel, int note, int velocity) {
  addDelta(delta);
  
  playback(channel, note, velocity, accDelta);
  index++;
  
  resetDelta();
}

void process2ByteMidiEvent(unsigned long delta, int channel, int value) {
  addDelta(delta);
  
  playback(channel, value, -1, accDelta);
  index++;
  
  resetDelta();
}


void playback(int channel, int note, int velocity, unsigned long delta) {
  unsigned long deltaMillis = (delta * getMicrosecondsPerQuarterNote()) / (((long) getTimeDivision()) * 1000);
  
  if(firstNote) {
    firstNote = false;
  }
  else {
    unsigned long currMillis = millis();
    
    int vel2use = 255;
    
    while(currMillis < lastMillis + deltaMillis)
    {
      //delay(lastMillis - currMillis + deltaMillis);
      int dly2use = 1;
      
      //Prevent display from interfering with timing of music
      if (lastMillis + deltaMillis - currMillis < 12)
      {
        dly2use= 0;
      }
      charlieVelocity = vel2use;
      CharlieDisplay(charlieDisplayPattern,charlieDisplayPattern2,charlieDisplayPattern3,dly2use);  
      currMillis = millis();  
    }      
  }

  if (velocity < 0)
  {
      midi2ByteMsg (channel, note);
  }
  else
  {  
      midiShortMsg (channel, note, velocity);

      //Update CharlieDisplayPattern based on note events
      if ((channel >=0x80) && (channel < 0x9F))
      {
        if (debugSong)
        {
          Serial.print("NOTE:");
          Serial.print(channel,HEX);
          Serial.print(note,HEX);
          Serial.println(velocity,HEX);
        }

        //midi channel filtering would go here
        int pianoNote = note % 12; //c=0,c#=1,...b=11 -> determines LED to light/turn off
        bool turnOn = false;
        if (channel >= 0x90)
        {
          if (velocity > 0)
          {
            turnOn = true;
          }
        }
        
        switch(pianoNote)
        {
          case 0:
          {
            lcd.setCursor(0,1);
            if (turnOn)  lcd.print("C"); else lcd.print(" ");
            break;
          }
          case 1:
          {
            lcd.setCursor(1,0);
            if (turnOn)  lcd.print("d"); else lcd.print(" ");
            break;
          }
          case 2:
          {
            lcd.setCursor(2,1);
            if (turnOn)  lcd.print("D"); else lcd.print(" ");
            break;
          }
          case 3:
          {
            lcd.setCursor(3,0);
            if (turnOn)  lcd.print("e"); else lcd.print(" ");
            break;
          }
          case 4:
          {
            lcd.setCursor(4,1);
            if (turnOn)  lcd.print("E"); else lcd.print(" ");
            break;
          }
          case 5:
          {
            lcd.setCursor(5,1);
            if (turnOn)  lcd.print("F"); else lcd.print(" ");
            break;
          }
          case 6:
          {
            lcd.setCursor(6,0);
            if (turnOn)  lcd.print("g"); else lcd.print(" ");
            break;
          }
          case 7:
          {
            lcd.setCursor(7,1);
            if (turnOn)  lcd.print("G"); else lcd.print(" ");
            break;
          }
          case 8:
          {
            lcd.setCursor(8,0);
            if (turnOn)  lcd.print("a"); else lcd.print(" ");
            break;
          }
          case 9:
          {
            lcd.setCursor(9,1);
            if (turnOn)  lcd.print("A"); else lcd.print(" ");
            break;
          }
          case 10:
          {
            lcd.setCursor(10,0);
            if (turnOn)  lcd.print("b"); else lcd.print(" ");
            break;
          }
          case 11:
          {
            lcd.setCursor(11,1);
            if (turnOn)  lcd.print("B"); else lcd.print(" ");            
            break;
          }          
        }
        
        long bitMask = 0;
        long bitMask2 = 0;
        if ((note >= charlieNoteTop - 29) && (note <=charlieNoteTop))
        {
          bitMask = 1;
          bitMask2 = 0;
          for (int b=0;b<(charlieNoteTop-note);b++)
          {
            bitMask *= 2;
          }
        }  
        else if ((note >= charlieNoteBottom ) && (note <= charlieNoteTop - 30))
        {
          bitMask = 0;
          bitMask2 = 1;
          for (int b=0;b<(charlieNoteTop-30-note);b++)
          {
            bitMask2 *= 2;
          }
        }
       
        if (debugSong)
        {
          if (turnOn)
          {
            Serial.print("ON ");
          }
          else
          {
            Serial.print("OFF ");
          }
          Serial.print(pianoNote,DEC);
          Serial.print(" mask=");
          Serial.print(bitMask,HEX);
          Serial.print(" mask2=");
          Serial.print(bitMask2,HEX);
        }
        
        if ((note >= charlieNoteBottom) && (note <=charlieNoteTop))
        {        
          if (turnOn)
          {
            charlieVelocity = velocity * 2;  //0-127 mapped to 0-254
            charlieDisplayPattern |= bitMask;  
            charlieDisplayPattern2 |= bitMask2;              
            if (debugSong)
            {
              Serial.print(" charlieDisplayPattern=");
              Serial.println(charlieDisplayPattern,HEX);              
              Serial.print(" charlieDisplayPattern2=");
              Serial.println(charlieDisplayPattern2,HEX);              
            }
          }
          else
          {
            bitMask = 0x3FFFFFFF & bitMask;
            bitMask2 = 0x3FFFFFFF & bitMask2;
            charlieDisplayPattern &= bitMask;
            charlieDisplayPattern2 &= bitMask2;
            if (debugSong)
            {
              Serial.print(" charlieDisplayPattern=");
              Serial.println(charlieDisplayPattern,HEX);              
              Serial.print(" charlieDisplayPattern2=");
              Serial.println(charlieDisplayPattern2,HEX);              
            }
          }        
        }
      }
      
  } 
  lastMillis = millis();
}

void midiShortMsg(int cmd, int pitch, int velocity) {  
  Serial1.write(cmd);
  Serial1.write(pitch);
  Serial1.write(velocity);
}

void midi2ByteMsg(int cmd, int value) {
  Serial1.write(cmd);
  Serial1.write(value);
}

void loop()
{
     if (waiting) {
           //Serial.print("Waiting for connection ... FreeRam=");
           //Serial.println(FreeRam());
           waiting = false;
     }
}

