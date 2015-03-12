#include <Adafruit_NeoPixel.h>
#include <avr/power.h>

#define MIC_PIN   1
#define LED_PIN   0
#define BTN_PIN   3
#define DC_OFFSET 0
#define NOISE     100
#define SAMPLES   60
#define PIXELS    125
#define TOP       (PIXELS + 1)

byte
  peak      = 0,      // Used for falling dot
  dotCount  = 0,      // Frame counter for delaying dot-falling speed
  volCount  = 0;      // Frame counter for storing past volume data

int
vol[SAMPLES],       // Collection of prior volume samples
   lvl       = 10,     // Current "dampened" audio level
   minLvlAvg = 0,      // For dynamic adjustment of graph low & high
   maxLvlAvg = 512,
   mode      = 0;

Adafruit_NeoPixel cube = Adafruit_NeoPixel(PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  // This is the auto-speed doubler line, keep it in, it will
  // automatically double the speed when 16Mhz is selected!
  if (F_CPU == 16000000) clock_prescale_set(clock_div_1);

  pinMode(BTN_PIN, INPUT); // Define the mode button

  memset(vol, 0, sizeof(vol));

  cube.show(); // Initialize all pixels to 'off'
}

void loop() {

  if (digitalRead(BTN_PIN) == LOW) {
    mode = mode + 1;
  }

  if (mode > 2 || mode < 0) {
    mode = 0;
  }

  if (mode == 0) {
    rainbow(20);
  }

  if (mode == 1) {
    audioColors();
  }

  if (mode == 2) {
    theaterChaseRainbow(20);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for (j = 0; j < 256; j++) {
    for (i = 0; i < cube.numPixels(); i++) {
      cube.setPixelColor(i, Wheel((i + j) & 255));
    }
    cube.show();
    delay(wait);
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j = 0; j < 256; j++) {   // cycle all 256 colors in the wheel
    for (int q = 0; q < 3; q++) {
      for (int i = 0; i < cube.numPixels(); i = i + 3) {
        cube.setPixelColor(i + q, Wheel( (i + j) % 255)); //turn every third pixel on
      }
      cube.show();

      delay(wait);

      for (int i = 0; i < cube.numPixels(); i = i + 3) {
        cube.setPixelColor(i + q, 0);      //turn every third pixel off
      }
    }
  }
}

void audioColors() {
  uint8_t i;
  uint16_t minLvl, maxLvl;

  int  n, height;
  n   = analogRead(MIC_PIN);                 // Raw reading from mic
  n   = abs(n - 512 - DC_OFFSET);            // Center on zero
  n   = (n <= NOISE) ? 0 : (n - NOISE);      // Remove noise/hum
  lvl = ((lvl * 7) + n) >> 3;    // "Dampened" reading (else looks twitchy)

  // Calculate bar height based on dynamic min/max levels (fixed point):
  height = TOP * (lvl - minLvlAvg) / (long)(maxLvlAvg - minLvlAvg);

  if (height < 0L)       height = 0;     // Clip output
  else if (height > TOP) height = TOP;
  if (height > peak)     peak   = height; // Keep 'peak' dot at top

  // Color pixels based on rainbow gradient
  for (i = 0; i < PIXELS; i++) {
    if (i >= height) {
      cube.setPixelColor(i,   0,   0, 0);
    } else {
      cube.setPixelColor(i, Wheel(map(i, 0, cube.numPixels() - 1, 30, 150)));
    }
  }

  cube.show(); // Update cube

  vol[volCount] = n;                      // Save sample for dynamic leveling
  if (++volCount >= SAMPLES) volCount = 0; // Advance/rollover sample counter

  // Get volume range of prior frames
  minLvl = maxLvl = vol[0];
  for (i = 1; i < SAMPLES; i++) {
    if (vol[i] < minLvl)      minLvl = vol[i];
    else if (vol[i] > maxLvl) maxLvl = vol[i];
  }
  // minLvl and maxLvl indicate the volume range over prior frames, used
  // for vertically scaling the output graph (so it looks interesting
  // regardless of volume level).  If they're too close together though
  // (e.g. at very low volume levels) the graph becomes super coarse
  // and 'jumpy'...so keep some minimum distance between them (this
  // also lets the graph go to zero when no sound is playing):
  if ((maxLvl - minLvl) < TOP) maxLvl = minLvl + TOP;
  minLvlAvg = (minLvlAvg * 63 + minLvl) >> 6; // Dampen min/max levels
  maxLvlAvg = (maxLvlAvg * 63 + maxLvl) >> 6; // (fake rolling average)
}

// Input a value 0 to 255 to get a color value.
// The colors are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if (WheelPos < 85) {
    return cube.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    return cube.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
    WheelPos -= 170;
    return cube.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}
