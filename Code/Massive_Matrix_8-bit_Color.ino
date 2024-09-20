// Include necessary libraries
#include <Adafruit_GFX.h>
#include <FastLED_NeoMatrix.h>
#include <FastLED.h>
#include <HardwareSerial.h>

//--------------- Change as Needed ---------------------------------------
// Define constants for the LED matrix
#define PIN 21
#define BRIGHTNESS 32      // Brightness of the LED matrix (out of 255)
#define mw 48             // Matrix width
#define mh 32             // Matrix height
#define NUM_MATRIX (mw*mh) // Total number of LEDs in the matrix

// Number of panels in the LED matrix
uint8_t NUM_PANELS = 3;

//--------------- Change as Needed ---------------------------------------

uint8_t width = mw, height = (mh * NUM_PANELS);

// Array to hold the LED color data
CRGB leds[NUM_MATRIX];

// Create a new NeoMatrix object
FastLED_NeoMatrix *matrix = new FastLED_NeoMatrix(leds, 16, 16, 3, 2, 
  NEO_MATRIX_TOP     + NEO_MATRIX_LEFT +
  NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG + 
  NEO_TILE_BOTTOM + NEO_TILE_LEFT +  NEO_TILE_COLUMNS + NEO_TILE_ZIGZAG);

// Arrays to hold the draw data and pass data
uint16_t DrawData[NUM_MATRIX]; 
uint8_t PassData[NUM_MATRIX * (3 * 2)]; // NUM_PANELS * 2
uint8_t * RawData8 = NULL;
uint16_t * RawData16 = NULL;

void setup() {
  // Add LEDs to the FastLED library
  FastLED.addLeds<WS2812B, PIN, GRB>(leds, NUM_MATRIX);
  // Initialize serial communication
  Serial.begin(2000000);
  Serial1.setPins(18, 17);
  Serial1.begin(2000000);
  // Set the brightness of the LED matrix
  matrix->setBrightness(BRIGHTNESS);
  delay(500);
}

void GetTheData8(){
  // Read enough data for NUM_PANELS
  Serial.readBytes(PassData, NUM_MATRIX * (NUM_PANELS));  
  // Write the header the next panel
  Serial1.write(0x80 | (NUM_PANELS - 1));      
  // Write the data for all but one panel           
  Serial1.write(PassData, NUM_MATRIX * (NUM_PANELS - 1)); 
  // Now we draw our frame
  DrawTheFrame8(); 
  // Send an acknowledgement
  Serial.write(0x06); 
}

void GetTheData16(){
  // Read enough data for NUM_PANELS
  Serial.readBytes(PassData, NUM_MATRIX * (NUM_PANELS) * 2);  
  // Write the header the next panel
  Serial1.write(0xC0 | (NUM_PANELS - 1));      
  // Write the data for all but one panel           
  Serial1.write(PassData, NUM_MATRIX * (NUM_PANELS - 1) * 2); 
  // Now we draw our frame
  DrawTheFrame16(); 
  // Send an acknowledgement
  Serial.write(0x06); 
}

void DrawTheFrame8(){
  // Get the address of the frame to draw
  RawData8 = &PassData[(NUM_PANELS - 1) * NUM_MATRIX];
  // Map the 8-bit color to RGB 565
  for (int i = 0; i < NUM_MATRIX; i++) {
    DrawData[i] = ((RawData8[i] & 0xE0) << 8)   // Red
                | ((RawData8[i] & 0x1C) << 6)  // Green
                | ((RawData8[i] & 0x03) << 3);  // Blue
    // Fix blue
    DrawData[i] |= (RawData8[i] & 0x03) < 2 ? 0 : 4; 
  }

  // Draw the bitmap on the LED matrix
  matrix->drawRGBBitmap(0, 0, DrawData, 48, 32);

  // Show the LEDs
  FastLED.show();
}

void DrawTheFrame16(){
  // Get the address of the frame to draw
  RawData16 = (uint16_t *) &PassData[(NUM_PANELS - 1) * (NUM_MATRIX * 2)];

  // Swap every two bytes in the draw data
  for (int i = 0; i < NUM_MATRIX; i++) {
    // Swap the byte
    DrawData[i] = ((RawData16[i] & 0xFF) << 8) | ((RawData16[i] & 0xFF00) >> 8);
  }

  // Draw the bitmap on the LED matrix
  matrix->drawRGBBitmap(0, 0, DrawData, 48, 32);

  // Show the LEDs
  FastLED.show();
}

void loop() {
  // Read header from the serial port
  uint8_t header = Serial.read();
  if (header == 0x05){
    // If the header is 0x05, print the width and height of the LED matrix
    Serial.println(width);
    Serial.println(height);
  }
  else if (header == 0x42){
    // Read the data from the serial port
    GetTheData16();
  }
  else if (header == 0x43){
    // Read the data from the serial port
    GetTheData8();
  }
  else if (header == 0xC3 || header == 0xC2 || header == 0xC1){
    // Set the number of panels and read the data from the serial port
    NUM_PANELS = header & 0x0F;
    GetTheData16();
  }
  else if (header == 0x83 || header == 0x82 || header == 0x81){
    // Set the number of panels and read the data from the serial port
    NUM_PANELS = header & 0x0F;
    GetTheData8();
  }
}
