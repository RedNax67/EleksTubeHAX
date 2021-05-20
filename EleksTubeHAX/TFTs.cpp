#include "TFTs.h"

char oldDigit[6];

void TFTs::begin() {
  // Start with all displays selected.
  chip_select.begin();
  chip_select.setAll();

  // Turn power on to displays.
  pinMode(TFT_ENABLE_PIN, OUTPUT);
  enableAllDisplays();

  // Initialize the super class.
  init();

  // Set SPIFFS ready
  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS initialization failed!");
  }
}

void TFTs::setDigit(uint8_t digit, uint8_t value, show_t show) {
  uint8_t old_value = digits[digit];
  digits[digit] = value;
  //Serial.println(value);
  if (show != no && (old_value != value || show == force)) {
    showDigit(digit);
  }
}

/*
   Where the rubber meets the road.  Displays the bitmap for the value to the given digit.
*/
void TFTs::showDigit(uint8_t digit) {
  chip_select.setDigit(digit);

  // Filenames are no bigger than "255.bmp\0"
  char file_name[10];
  sprintf(file_name, "/%d.bmp", digits[digit]);
  //Serial.print(file_name);
  drawBmp(file_name, 0, 0);
}

void TFTs::printString(char *tftString, show_t show)
{
  String fileName;

  if (show == force) {
    oldDigit[0] = 0xff;
    oldDigit[1] = 0xff;
    oldDigit[2] = 0xff;
    oldDigit[3] = 0xff;
    oldDigit[4] = 0xff;
    oldDigit[5] = 0xff;
  }


  digitalWrite(latchPin, LOW);                                               // Setup to write the first screen
  shiftOut(dataPin, clockPin, MSBFIRST, 0xfe);                               //
  digitalWrite(latchPin, HIGH);                                              //
  digitalWrite(dataPin, HIGH);                                               //
  digitalWrite(clockPin, LOW);                                               //

  for (int i = 0; i < 6; i++) {
    if ( tftString[i] != oldDigit[i]) {                                      //Timedigit changed?
      fileName = String("/images/") + tftString[i] + String(".bmp");         //Build the image filename
      //Serial.println(fileName);
      oldDigit[i] = tftString[i];                                            //Store current digit
      drawBmp(fileName.c_str(), 0, 0);                                       //Draw the bitmap on current screen
    }
    digitalWrite(latchPin, LOW);                                             // Clock in the next screen
    digitalWrite(clockPin, HIGH);                                            // Should be faster than clocking in the entire byte every time
    digitalWrite(clockPin, LOW);                                             //
    digitalWrite(latchPin, HIGH);                                            //
  }
}

// These BMP functions are stolen directly from the TFT_SPIFFS_BMP example in the TFT_eSPI library.
// Unfortunately, they aren't part of the library itself, so I had to copy them.
// I've modified drawBmp to buffer the whole image at once instead of doing it line-by-line.

//// BEGIN STOLEN CODE
// Too big to fit on the stack.
uint16_t TFTs::output_buffer[TFT_HEIGHT][TFT_WIDTH];

bool TFTs::drawBmp(const char *filename, int16_t x, int16_t y) {

  // Nothing to do.
  if ((x >= width()) || (y >= height())) return (true);

  fs::File bmpFS;

  // Open requested file on SD card
  bmpFS = SPIFFS.open(filename, "r");

  if (!bmpFS)
  {
    Serial.println("File not found");
    return (false);
  }

  uint32_t seekOffset, cmp;
  int16_t w, h, row, col, p, bpp;
  uint8_t  r, g, b;

  uint32_t startTime = millis();

  uint16_t magic = read16(bmpFS);
  if (magic == 0xFFFF) {
    Serial.println("File not found. Make sure you upload the SPIFFs image with BMPs.");
    bmpFS.close();
    return (false);
  }

  if (magic != 0x4D42) {
    Serial.print("File not a BMP. Magic: ");
    Serial.println(magic);
    bmpFS.close();
    return (false);
  }

  read32(bmpFS); // The size of the BMP file in bytes
  read32(bmpFS); // Reserved; actual value depends on the application that creates the image, if created manually can be 0
  seekOffset = read32(bmpFS); //The offset, i.e. starting address, of the byte where the bitmap image data (pixel array) can be found.
  read32(bmpFS); // The size of this header, in bytes (40)
  w = read32(bmpFS); //the bitmap width in pixels (signed integer)
  h = read32(bmpFS); //the bitmap height in pixels (signed integer)
  p = read16(bmpFS); // the number of color planes (must be 1)
  bpp = read16(bmpFS); // the number of bits per pixel, which is the color depth of the image. Typical values are 1, 4, 8, 16, 24 and 32.
  cmp = read32(bmpFS); //the compression method being used. See the next table for a list of possible values

  if ((p != 1) || (bpp != 24) || (cmp != 0)) {
    Serial.println("BMP format not recognized.");
    bmpFS.close();
    return (false);
  }
  bool oldSwapBytes = getSwapBytes();
  setSwapBytes(true);
  bmpFS.seek(seekOffset);

  uint16_t padding = (4 - ((w * 3) & 3)) & 3;
  uint8_t lineBuffer[w * 3 + padding];

  // row is decremented as the BMP image is drawn bottom up
  for (row = h - 1; row >= 0; row--) {

    bmpFS.read(lineBuffer, sizeof(lineBuffer));
    uint8_t*  bptr = lineBuffer;

    // Convert 24 to 16 bit colours
    for (uint16_t col = 0; col < w; col++)
    {
      b = *bptr++;
      g = *bptr++;
      r = *bptr++;
      output_buffer[row][col] = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }
  }

  pushImage(x, y, w, h, (uint16_t *)output_buffer);
  setSwapBytes(oldSwapBytes);
  bmpFS.close();
  return (true);
}

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t TFTs::read16(fs::File & f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t TFTs::read32(fs::File & f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}
//// END STOLEN CODE
