#include <Wire.h>
#include "wiring_private.h" // pinPeripheral() function
TwoWire myWire(&sercom5, 13, 12);
#include "Adafruit_TCS34725_2nd_i2c.h"
#include <Adafruit_TCS34725.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1331.h>
#include <SPI.h>
#include <Adafruit_VS1053.h>
#include <SD.h>

using namespace std;

//Color Sensor
//#define SDApin 46
//#define SCLpin 47
//#define SDApin2 44
//#define SCLpin2 45


// See http://www.vlsi.fi/fileadmin/datasheets/vs1053.pdf Pg 31
#define VS1053_BANK_DEFAULT 0x00
#define VS1053_BANK_DRUMS1 0x78
#define VS1053_BANK_DRUMS2 0x7F
#define VS1053_BANK_MELODY 0x79

// See http://www.vlsi.fi/fileadmin/datasheets/vs1053.pdf Pg 32 for more!
#define VS1053_GM1_OCARINA 80

#define MIDI_NOTE_ON  0x90
#define MIDI_NOTE_OFF 0x80
#define MIDI_CHAN_MSG 0xB0
#define MIDI_CHAN_BANK 0x00
#define MIDI_CHAN_VOLUME 0x07
#define MIDI_CHAN_PROGRAM 0xC0


#if defined(__AVR_ATmega32U4__) || defined(ARDUINO_SAMD_FEATHER_M0) || defined(TEENSYDUINO) || defined(ARDUINO_STM32_FEATHER)
  #define VS1053_MIDI Serial1
#elif defined(ESP32)
  HardwareSerial Serial1(2);
  #define VS1053_MIDI Serial1
#elif defined(ESP8266)
  #define VS1053_MIDI Serial
#else
  #define VS1053_MIDI Serial1
#endif


//OLED
#define sclk SCLK
#define mosi MOSI
#define cs   A2
#define rst A3
#define dc  A4

#define commonAnode true

// Color definitions
#define  BLACK           0x0000
//#define WHITE           0xFFFF
#define RED             0xF800
#define ORANGE          0xFBE0
#define YELLOW          0xFFE0
#define GREEN           0x07E0
#define CYAN            0x07FF
#define BLUE            0x001F
#define PURPLE          0x780F
#define PINK            0xFB78//F81F

uint16_t colors[] = {RED, ORANGE, YELLOW, GREEN, CYAN, BLUE, PURPLE, PINK};
String letters[] = {"C", "D", "E", "F", "G", "A", "B", "C"};
int notes[] = {60, 62, 64, 65, 67,69, 71, 72};

// our RGB -> eye-recognized gamma color
byte gammatable[256];


uint16_t clear, red, green, blue;

String prevNote1 = "";
String prevNote2 = "";

Adafruit_SSD1331 display = Adafruit_SSD1331(cs, dc, rst);
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);
//Adafruit_TCS34725_2nd_i2c tcs2 = Adafruit_TCS34725_2nd_i2c(TCS34725_INTEGRATIONTIME_50MS2, TCS34725_GAIN_4X2);

 int delayTime = 500;
 
void setup() {
  //SPI Chaining from http://www.gammon.com.au/forum/?id=10892&reply=1#reply1

  Serial.begin(9600);
  
  ////////////////////////////
  //Display
  ////////////////////////////
  display.begin();
  Serial.println("Display started");
  display.fillScreen(BLACK);

  ////////////////////////////
  //Color Sensors
  ////////////////////////////
   pinPeripheral(13, PIO_SERCOM_ALT);
  pinPeripheral(12, PIO_SERCOM_ALT);
//  if (tcs2.begin()) {
//  if (1) {
//    Serial.println("Found TCS2 sensor");
//  } else {
//    Serial.println("No TCS34725 found ... check your connections");
//    while (1); // halt!
//  }
  if (tcs.begin()) {
    Serial.println("Found TCS1 sensor");
  } else {
    Serial.println("No TCS34725 found ... check your connections");
    while (1); // halt!
  }

  for (int i=0; i<256; i++) {
    float x = i;
    x /= 255;
    x = pow(x, 2.5);
    x *= 255;
      
    if (commonAnode) {
      gammatable[i] = 255 - x;
    } else {
      gammatable[i] = x;      
    }
    //Serial.println(gammatable[i]);
  }
  
  ////////////////////////////
  //Audio Breakout
  ////////////////////////////
  Serial.println("VS1053 MIDI test");

  VS1053_MIDI.begin(31250); // MIDI uses a 'strange baud rate'
  Serial.println("VS1053 began");
  pinMode(5, OUTPUT);
  digitalWrite(5, LOW);
  delay(10);
  digitalWrite(5, HIGH);
  delay(10);
  
  midiSetChannelBank(0, VS1053_BANK_MELODY);
  Serial.println("Channel bank completed");

  midiSetChannelVolume(0, 170);
  Serial.println("Channel volume initialized");

  midiSetInstrument(0, VS1053_GM1_OCARINA);

  Serial.println("Setup completed!");
}

int calcColor(int side){
 // Determine RGB values
  uint32_t sum = clear;
  float r, g, b;
  r = red; r /= sum;
  g = green; g /= sum;
  b = blue; b /= sum;
  r *= 256; g *= 256; b *= 256;

  // RGB to HSL conversion from Arduino library https://github.com/ratkins/RGBConverter
  double rd = (double) r/255;
  double gd = (double) g/255;
  double bd = (double) b/255;
  double maxGdBd = max(gd, bd);
  double maxRGB = max(rd, maxGdBd);
  double minGdBd = min(gd, bd);
  double minRGB = min(rd, minGdBd);
  double h, s, l = (maxRGB + minRGB) / 2;

  if (maxRGB == minRGB) {
        h = s = 0; // achromatic
  } else {
        double d = maxRGB - minRGB;
        s = l > 0.5 ? d / (2 - maxRGB - minRGB) : d / (maxRGB + minRGB);
        if (maxRGB == rd) {
            h = (gd - bd) / d + (gd < bd ? 6 : 0);
        } else if (maxRGB == gd) {
            h = (bd - rd) / d + 2;
        } else if (maxRGB == bd) {
            h = (rd - gd) / d + 4;
        }
        h /= 6;
  }

//  Serial.print("\t");
//  // Serial.print((int)r, HEX); Serial.print((int)g, HEX); Serial.print((int)b, HEX);
//  Serial.print("\tH:\t"); Serial.print(h);
//  Serial.print("\tS:\t"); Serial.print(s);
//  Serial.print("\tL:\t"); Serial.print(l);
//  Serial.println();

  h *= 360;
  s *= 100;
  l *= 100;

  Serial.print("\t");
  // Serial.print((int)r, HEX); Serial.print((int)g, HEX); Serial.print((int)b, HEX);
  Serial.print("\tH:\t"); Serial.print(h);
  Serial.print("\tS:\t"); Serial.print(s);
  Serial.print("\tL:\t"); Serial.print(l);
  Serial.println();


  // Map colors to notes
  if (s > 12 && l > 25 && l < 70) {
    // Color range

    //delay(100);

    if ((h >= 0 && h < 21)) {
      // C4: RED
      Serial.println("C4 Red");
      return 0;
    } else if (h >= 21 && h < 45) {
      // D: ORANGE
      Serial.println("D Orange");
      return 1;
    } else if (h >= 45 && h < 76) {
      // E: YELLOW
      Serial.println("E Yellow");
      return 2;
    } else if (h >= 76 && h < 159) {
      // F: GREEN
      Serial.println("F green");
      return 3;
    } else if (h >= 159 && h < 200) {
      // G: CYAN
      Serial.println("G cyan");
      return 4;
    } else if (h >= 200 && h < 230) {
      // A: BLUE
      Serial.println("A blue");
      return 5;
    } else if (h >= 230 && h < 293) {
      // B: PURPLE
      Serial.println("B purple");
      return 6;
    } else if (h >= 293 && h < 361) {
      // C5: PINK
      Serial.println("C pink");
      return 7;
    } else {
      clearSide(side);
      return -1;
    }
  } else {
    clearSide(side);
    return -1;
  }
}
void playNote(int sideA, int sideB){

    if(sideA != -1 && sideB != -1){
        midiNoteOn(0, notes[sideA], 127);
        midiNoteOn(0, notes[sideB], 127);
        delay(500);
        midiNoteOff(0, notes[sideA], 127); 
        midiNoteOff(0, notes[sideB], 127); 
      
      
     }else if(sideA != -1 && sideB == -1){
          midiNoteOn(0, notes[sideA], 127);
        delay(500);
        midiNoteOff(0, notes[sideA], 127); 
        
     }else if(sideB != -1 && sideA == -1){
        midiNoteOn(0, notes[sideB], 127);
        delay(500);
        midiNoteOff(0, notes[sideB], 127); 
      
     }
  
    
}
void displayLetter(int arrNum, int side) {

    if(side == 0){
//      if(prevNote1 != letters[arrNum]){
          clearSide(0);
          display.setCursor(0, 1);
          display.setTextColor(colors[arrNum]);
          display.setTextSize(7);
          if(letters[arrNum] == "C5"){
              display.println("C");
            }else{
              display.println(letters[arrNum]);
            }
          
//          prevNote1 = letters[arrNum];
//        }
    }else{
//      if(prevNote2 != letters[arrNum]){
          clearSide(1);
          display.setCursor(display.width()/2 + 1, 1);
          display.setTextColor(colors[arrNum]);
          display.setTextSize(7);
          if(letters[arrNum] == "C5"){
              display.println("C");
            }else{
              display.println(letters[arrNum]);
            }
//          prevNote2 = letters[arrNum];
//        }
        
      }
      
}




//turns a half of the screen black
//pass in 0 for left side, and 1 for right side
void clearSide(int side) {
   if(side == 0){
      display.fillRect(0, 0, display.width()/2, display.height(), BLACK);
    }
    else{
      display.fillRect(display.width()/2, 0, display.width()/2, display.height(), BLACK);  
   }
   

}
void loop() {

//  tcs.setInterrupt(false);      // turn on LED
//  tcs2.setInterrupt(false);
    
  tcs.getRawData(&red, &green, &blue, &clear);

  tcs.setInterrupt(true);  // turn off LED
  int sideA = calcColor(0);
  
  if(sideA != -1){
      displayLetter(sideA, 0);
    }
//  tcs2.getRawData(&red, &green, &blue, &clear);
//
//  tcs2.setInterrupt(true);  // turn off LED
//  int sideB = calcColor(1);
//
//  
//  if(sideB != -1){
//    displayLetter(sideB, 1);
//  }
  playNote(sideA, -1);
  

  
}

void midiSetInstrument(uint8_t chan, uint8_t inst) {
  if (chan > 15) return;
  inst --; // page 32 has instruments starting with 1 not 0 :(
  if (inst > 127) return;
  
  VS1053_MIDI.write(MIDI_CHAN_PROGRAM | chan);  
  delay(10);
  VS1053_MIDI.write(inst);
  delay(10);
}


void midiSetChannelVolume(uint8_t chan, uint8_t vol) {
  if (chan > 15) return;
  if (vol > 127) return;
  
  VS1053_MIDI.write(MIDI_CHAN_MSG | chan);
  VS1053_MIDI.write(MIDI_CHAN_VOLUME);
  VS1053_MIDI.write(vol);
}

void midiSetChannelBank(uint8_t chan, uint8_t bank) {
  if (chan > 15) return;
  if (bank > 127) return;
  
  VS1053_MIDI.write(MIDI_CHAN_MSG | chan);
  VS1053_MIDI.write((uint8_t)MIDI_CHAN_BANK);
  VS1053_MIDI.write(bank);
}

void midiNoteOn(uint8_t chan, uint8_t n, uint8_t vel) {
  if (chan > 15) return;
  if (n > 127) return;
  if (vel > 127) return;
  
  VS1053_MIDI.write(MIDI_NOTE_ON | chan);
  VS1053_MIDI.write(n);
  VS1053_MIDI.write(vel);
}

void midiNoteOff(uint8_t chan, uint8_t n, uint8_t vel) {
  if (chan > 15) return;
  if (n > 127) return;
  if (vel > 127) return;
  
  VS1053_MIDI.write(MIDI_NOTE_OFF | chan);
  VS1053_MIDI.write(n);
  VS1053_MIDI.write(vel);
}
void testlines(uint16_t color) {
   display.fillScreen(BLACK);
   for (int16_t x=0; x < display.width()-1; x+=6) {
     display.drawLine(0, 0, x, display.height()-1, color);
   }
   for (int16_t y=0; y < display.height()-1; y+=6) {
     display.drawLine(0, 0, display.width()-1, y, color);
   }
   
   display.fillScreen(BLACK);
   for (int16_t x=0; x < display.width()-1; x+=6) {
     display.drawLine(display.width()-1, 0, x, display.height()-1, color);
   }
   for (int16_t y=0; y < display.height()-1; y+=6) {
     display.drawLine(display.width()-1, 0, 0, y, color);
   }

   // To avoid ESP8266 watchdog timer resets when not using the hardware SPI pins
   delay(0); 

   display.fillScreen(BLACK);
   for (int16_t x=0; x < display.width()-1; x+=6) {
     display.drawLine(0, display.height()-1, x, 0, color);
   }
   for (int16_t y=0; y < display.height()-1; y+=6) {
     display.drawLine(0, display.height()-1, display.width()-1, y, color);
   }

   display.fillScreen(BLACK);
   for (int16_t x=0; x < display.width()-1; x+=6) {
     display.drawLine(display.width()-1, display.height()-1, x, 0, color);
   }
   for (int16_t y=0; y < display.height()-1; y+=6) {
     display.drawLine(display.width()-1, display.height()-1, 0, y, color);
   }
   
}
