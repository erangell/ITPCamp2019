#include <SdFat.h>
#include <SdFatUtil.h>
#include <SPI.h>

/* Arduino SD Card MIDI Song Player - by Eric Rangell - Release 1.00 - 2012-JUN-04
 * Adapted from open-source Arduino MIDI code.  Published in the public domain.
 *
 * Use DIP switches to select song 001.MID to 255.MID in the root of the SD card.  Reset button plays song.d
 * Song 000 does note off 0-127 on channels 0-15.
 * Set debugSong = true to debug songs that don't play.
 * For Type 1 Midi Files, program will attempt to play data from track #2.
 * It does not mix down tracks in Type 1 Midi Files, so they should be converted to Type 0.
 * 
 * Modification History:
 * 2012-MAY-28: fixed bug with long songs that didn't play
 * 2012-JUN-02: changed pin assignments: 8->9 9->10
 * 2013-JAN-20: continuous playback incrementing song number until file not found
 * 2013-MAY-31: if song # > 127, get directory number from lower 7 bits and start playing from song 000
 * 2013-JUN-08: use analog a0-a5 to select subdirectory.
 * 2014-NOV-28: testing tempo change issues with Standards songs
 * 2016-DEC-28: Fix for small resistance on analog switches 3 and 4
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

boolean waiting = true;
boolean get = false;
boolean post = false;
boolean boundary = false;

boolean file_opened = false;
boolean last_block = false;
boolean file_closed = false;
boolean logging = true;
char currentSong[13];
char TheSubDir[13];
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

const int b0 = 2;     // the numbers of the switch pins
const int b1 = 3;
const int b2 = 4;
const int b3 = 5;
const int b4 = 6;
const int b5 = 7;
const int b6 = 9;
const int b7 = 10;
const int b8 = 11;
const int b9 = 12;

//use analog pins as digital inputs
const int a0 = 0;
const int a1 = 1;
const int a2 = 2;
const int a3 = 3;
const int a4 = 4;
const int a5 = 5;

int bs0 = 0;
int bs1 = 0;
int bs2 = 0;
int bs3 = 0;
int bs4 = 0;
int bs5 = 0;
int bs6 = 0;
int bs7 = 0;

int as0 = 0;
int as1 = 0;
int as2 = 0;
int as3 = 0;
int as4 = 0;
int as5 = 0;

int inbyte = 0; 
int analog = 0; 

boolean debugSong = false;

/************ SDCARD STUFF ************/
Sd2Card card;
SdVolume volume;
SdFile root;
SdFile subdir;
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

void resetGlobalVars()
{  
  waiting = true;
  get = false;
  post = false;
  boundary = false;

  file_opened = false;
  last_block = false;
  file_closed = false;
  logging = true;
  bufsiz=SD_BUFFER_SIZE;
  bytesread1 = 0;
  bufIndex = 0;
  currentByte = 0;
  previousByte = 0;

  format = 0;
  trackCount = 0;
  timeDivision = 0;

  deltaTime = 0;
  eventType = 0;
  eventChannel = 0;
  parameter1 = 0;
  parameter2 = 0;

  microseconds = 500000;
  index = 0;
  accDelta = 0;

  firstNote = true;
  currFreq = -1;
  lastMillis = 0;
}

void setup() {

  pinMode(b0, INPUT);     
  pinMode(b1, INPUT);     
  pinMode(b2, INPUT);     
  pinMode(b3, INPUT);     
  pinMode(b4, INPUT);     
  pinMode(b5, INPUT);     
  pinMode(b6, INPUT);     
  pinMode(b7, INPUT);     

  pinMode(a0, INPUT);     
  pinMode(a1, INPUT);     
  pinMode(a2, INPUT);     
  pinMode(a3, INPUT);     
  pinMode(a4, INPUT);     
  pinMode(a5, INPUT);     
  
  bs0 = digitalRead(b0);
  bs1 = digitalRead(b1);
  bs2 = digitalRead(b2);
  bs3 = digitalRead(b3);
  bs4 = digitalRead(b4);
  bs5 = digitalRead(b5);
  bs6 = digitalRead(b6);
  bs7 = digitalRead(b7);

  as0 = analogRead(a0);
  as1 = analogRead(a1);
  as2 = analogRead(a2);
  as3 = analogRead(a3);
  as4 = analogRead(a4);
  as5 = analogRead(a5);
  
  if (debugSong)
 {
   Serial.begin(9600);
 }
 else
 {
   Serial.begin(31250);
 }
 // initialize the SD card at SPI_HALF_SPEED to avoid bus errors with
 // breadboards.  use SPI_FULL_SPEED for better performance.
 pinMode(10, OUTPUT);                       // set the SS pin as an output (necessary!)
 digitalWrite(10, HIGH);                    // but turn off the W5100 chip!
 if (!card.init()) error("card.init failed!");
 // initialize a FAT volume
 if (!volume.init(card)) error("vol.init failed!");
 //PgmPrint("Volume is FAT");
 //Serial.println(volume.fatType(),DEC);
 //Serial.println();
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

  //2016-12-28: fix for resistance on switches as3 and as4 - threshold level = 1000
  analog=0;
  if (as0 > 1000) {     
    analog += 1;
  } 
  if (as1 > 1000 ) {     
    analog += 2;
  } 
  if (as2 > 1000) {     
    analog += 4;
  } 
  if (as3 > 1000) {     
    analog += 8;
  } 
  if (as4 > 1000) {     
    analog += 16;
  } 
  if (as5 > 1000) {     
    analog += 32;
  } 

  if (debugSong)
  {
       Serial.println();
       Serial.print("as0= ");
       Serial.println(as0);
       Serial.print("as1= ");
       Serial.println(as1);
       Serial.print("as2= ");
       Serial.println(as2);
       Serial.print("as3= ");
       Serial.println(as3);
       Serial.print("as4= ");
       Serial.println(as4);
       Serial.print("as5= ");
       Serial.println(as5);
  }
    
  if (analog > 0)
  {
    int currentSubdir = analog;
    TheSubDir[2] = currentSubdir%10 + '0';
    currentSubdir = currentSubdir / 10;
    TheSubDir[1] = currentSubdir%10 + '0';
    currentSubdir = currentSubdir / 10;
    TheSubDir[0] = currentSubdir%10 + '0';
    currentSong[3] = '\0';
     
    if (debugSong)
    {
       Serial.println();
       Serial.print("Current Subdir: ");
       Serial.println(TheSubDir);
    }

    // test for existence of subdir
    if (!subdir.open(root, TheSubDir, O_READ)) 
    {
       if (debugSong)
       {
           Serial.println("SUBDIR NOT FOUND ");       
       }
       while (1==1) {
           // halt condition - use missing file number to signal end of a set
       }
    }
  }
  else
  {
	subdir.openRoot(volume);
  }
}

void SendAllNotesOff() 
{
  for (int channel = 0; channel < 16; channel++)
  {
    for (int note = 00; note < 128; note++) 
    {
      midiOutShortMsg(0x90+channel, note, 0x00);   
    }
  }
}
void midiOutShortMsg(int cmd, int pitch, int velocity) {
  if (!debugSong)
  {
    Serial.print(cmd, BYTE);
    Serial.print(pitch, BYTE);
    Serial.print(velocity, BYTE);
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
  if (file_closed)
  {
    currentByte = 0;
  }
  else
  {
    if (!file_opened)
    {
       if (file.open(subdir, currentSong, O_READ)) 
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
  if (debugSong)
  {
    Serial.println("=====");
    Serial.print("TEMPO:");
    Serial.println(microseconds);
  }
}
  
unsigned long getMicrosecondsPerQuarterNote() {
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
    
    if(currMillis < lastMillis + deltaMillis)
    {
      if (!debugSong)
      {
        delay(lastMillis - currMillis + deltaMillis);
      }
    }
  }

  if (velocity < 0)
  {
      midi2ByteMsg (channel, note);
  }
  else
  {  
      midiShortMsg (channel, note, velocity);
  } 
  lastMillis = millis();
}

void midiShortMsg(int cmd, int pitch, int velocity) {
  if (!debugSong)
  {  
    Serial.print(cmd, BYTE);
    //Serial.print(" ");
    Serial.print(pitch, BYTE);
    //Serial.print(" ");
    Serial.print(velocity, BYTE);
    //Serial.println();
}
}

void midi2ByteMsg(int cmd, int value) {
  if (!debugSong)
  {
    Serial.print(cmd, BYTE);
    //Serial.print(" ");
    Serial.print(value, BYTE);
    //Serial.println();
  }
}

void loop()
{
  if (debugSong)
   {
     Serial.print("TOP OF LOOP: inbyte=");
     Serial.println(inbyte);
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
       Serial.println();
       Serial.print("Current Song #: ");
       Serial.println(currentSong);
     }

     // test for existence of file - if it opens, close it - it will be reopened when chunks processed
     if (file.open(subdir, currentSong, O_READ)) 
     {
       file.close();
     }
     else
     {
       if (debugSong)
       {
           Serial.println("FILE NOT FOUND ");       
       }
       inbyte=0; // force loop back to song 001
       file_closed = true;
     }
     
     //Phase processing
     while (!file_closed)
     {
        processChunk(); // header chunk
        
        if(getFileFormat() == 0) {
          logs("MIDI file format = 0");
          int trackCount = getTrackCount();
          logi("Track Count=",trackCount);
          
          for(int i = 0; i < getTrackCount(); i++) {
              processChunk();
          }
          file_closed = true;
        }
        else {
          logs("MIDI file not format 0.");
        }
     }
   }
   inbyte++;
   resetGlobalVars();
   if (debugSong)
   {
     while (1==1) { inbyte = 0; }
   }
}

