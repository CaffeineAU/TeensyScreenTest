/*
 Name:		TeensyScreenTest.ino
 Created:	11/16/2016 11:33:00 PM
 Author:	Liam O
*/

// the setup function runs once when you press reset or power the board


//#include <hsv2rgb.h>
#include <SysCall.h>
#include <SdFatConfig.h>
#include <MinimumSerial.h>
#include <FreeStack.h>
#include <BlockDriver.h>
//#include <SdFatmainpage.h>
#include "SdFat.h"

#include <ILI9341_t3.h>
#include <font_ArialBold.h>
#include <font_Arial.h>

#define TFT_RST 8
#define TFT_DC  9
#define TFT_CS 10

ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC, TFT_RST);
#define RGB(r,g,b) (r<<11|g<<5|b)  //  R = 5 bits 0 to 31, G = 6 bits 0 to 63, B = 5 bits 0 to 31

byte colourbyte = 0;

SdFatSdioEX sd;
SdFile file;


void setup() {
	delay(200);

	tft.begin();
	tft.fillScreen(ILI9341_BLUE);

	tft.setFont(Arial_12_Bold);

	Serial.print(F("Initializing SD card..."));
	tft.println(F("Init SD card..."));
	tft.setTextColor(ILI9341_BLACK);

	//if (!(SD.begin(BUILTIN_SDCARD))) {
	//	while (1) {
	//		Serial.println("Unable to access the SD card");
	//		delay(500);
	//	}
	//}
	sd.begin();

	Serial.println("OK!");
	


}
char filename[255];

// the loop function runs over and over again until power down or reset
void loop() {
	//CHSV colour = CHSV(colourbyte++, 255, 255);
	//CRGB colourRGB = CRGB(colour);

	//CHSV BGcolour = CHSV(colourbyte+128, 255, 255);
	//CRGB BGcolourRGB = CRGB(BGcolour);

	//tft.setTextColor(CL(colourRGB.red, colourRGB.green, colourRGB.blue));
	//tft.fillRoundRect(20, 20, 200, 200, 15, CL(BGcolourRGB.red, BGcolourRGB.green, BGcolourRGB.blue));
	//tft.
	//delay(20);
	sd.vwd()->rewind();

	while (file.openNext(sd.vwd(), O_READ)) {
		file.getName(filename, 255);
		Serial.println(filename);
		if (bmpDraw(filename, 0, 0))
		{
			delay(5000);
			//file.printName(&tft);
		}
		file.close();
	}

	//bmpDraw("flowers.bmp", 0, 0);
	//delay(1500);
	//bmpDraw("cat.bmp", 0, 0);
	//delay(1500);
	//bmpDraw("spitfire.bmp", 0, 0);
	//delay(1500);
	//bmpDraw("spit2.bmp", 0, 0);
	//delay(1500);
}

#define ROWSPERDRAW 80
#define BUFFPIXEL 240

//===========================================================
// Try Draw using writeRect
bool bmpDraw(const char *filename, uint8_t x, uint16_t y) {

	File     bmpFile;
	int      bmpWidth, bmpHeight;   // W+H in pixels
	uint8_t  bmpDepth;              // Bit depth (currently must be 24)
	uint32_t bmpImageoffset;        // Start of image data in file
	uint32_t rowSize;               // Not always = bmpWidth; may have padding
	uint8_t  sdbuffer[3 * BUFFPIXEL*ROWSPERDRAW]; // pixel buffer (R+G+B per pixel)
	uint16_t buffidx = sizeof(sdbuffer); // Current position in sdbuffer
	boolean  goodBmp = false;       // Set to true on valid header parse
	boolean  flip = true;        // BMP is stored bottom-to-top
	int      w, h, row, col;
	uint8_t  r, g, b;
	uint32_t pos = 0, startTime = millis();

	uint16_t awColors[240* ROWSPERDRAW];  // hold colors for one row at a time...

	if ((x >= tft.width()) || (y >= tft.height())) return false;

	Serial.println();
	Serial.print(F("Loading image '"));
	Serial.print(filename);
	Serial.println('\'');

	// Open requested file on SD card
	if (!(bmpFile.open(filename))) {
		Serial.print(F("File not found"));
		return false;
	}

	// Parse BMP header
	if (read16(bmpFile) == 0x4D42) { // BMP signature
		Serial.print(F("File size: ")); Serial.println(read32(bmpFile));
		(void)read32(bmpFile); // Read & ignore creator bytes
		bmpImageoffset = read32(bmpFile); // Start of image data
		Serial.print(F("Image Offset: ")); Serial.println(bmpImageoffset, DEC);
		// Read DIB header
		Serial.print(F("Header size: ")); Serial.println(read32(bmpFile));
		bmpWidth = read32(bmpFile);
		bmpHeight = read32(bmpFile);
		if (read16(bmpFile) == 1) { // # planes -- must be '1'
			bmpDepth = read16(bmpFile); // bits per pixel
			Serial.print(F("Bit Depth: ")); Serial.println(bmpDepth);
			if ((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

				goodBmp = true; // Supported BMP format -- proceed!
				Serial.print(F("Image size: "));
				Serial.print(bmpWidth);
				Serial.print('x');
				Serial.println(bmpHeight);

				// BMP rows are padded (if needed) to 4-byte boundary
				rowSize = (bmpWidth * 3 + 3) & ~3;

				// If bmpHeight is negative, image is in top-down order.
				// This is not canon but has been observed in the wild.
				if (bmpHeight < 0) {
					bmpHeight = -bmpHeight;
					flip = false;
				}

				// Crop area to be loaded
				w = bmpWidth;
				h = bmpHeight;
				if ((x + w - 1) >= tft.width())  w = tft.width() - x;
				if ((y + h - 1) >= tft.height()) h = tft.height() - y;


				for (row = 0; row < h; row+= ROWSPERDRAW) { // For each scanline...

											  // Seek to start of scan line.  It might seem labor-
											  // intensive to be doing this on every line, but this
											  // method covers a lot of gritty details like cropping
											  // and scanline padding.  Also, the seek only takes
											  // place if the file position actually needs to change
											  // (avoids a lot of cluster math in SD library).
					if (flip) // Bitmap is stored bottom-to-top order (normal BMP)
						pos = bmpImageoffset + (bmpHeight - ROWSPERDRAW - row) * rowSize;
					else     // Bitmap is stored top-to-bottom
						pos = bmpImageoffset + row * rowSize;
					if (bmpFile.position() != pos) { // Need seek?
						bmpFile.seek(pos);
						buffidx = sizeof(sdbuffer); // Force buffer reload
					}

					for (int i = ROWSPERDRAW; i > 0; i--)
					{
						for (col = 0; col < (w); col++) { // For each pixel...
													  // Time to read more pixel data?
							if (buffidx >= sizeof(sdbuffer)) { // Indeed
								bmpFile.read(sdbuffer, sizeof(sdbuffer));
								buffidx = 0; // Set index to beginning
							}

							// Convert pixel from BMP to TFT format, push to display
							b = sdbuffer[buffidx++];
							g = sdbuffer[buffidx++];
							r = sdbuffer[buffidx++];
							awColors[col+((i-1)*w)] = tft.color565(r, g, b);
						} // end pixel
					}
					tft.writeRect(0, row, w, ROWSPERDRAW, awColors);
				} // end scanline
				Serial.print(F("Loaded in "));
				Serial.print(millis() - startTime);
				Serial.println(" ms");
			} // end goodBmp
		}
	}
	else
	{
		return false;
	}
	bmpFile.close();
	if (!goodBmp) Serial.println(F("BMP format not recognized."));
	tft.setCursor(10, 10);
	tft.print(filename);
	tft.print(" : ");
	tft.print(millis() - startTime);
	tft.println(" ms");
	return true;
}



// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(File &f) {
	uint16_t result;
	((uint8_t *)&result)[0] = f.read(); // LSB
	((uint8_t *)&result)[1] = f.read(); // MSB
	return result;
}

uint32_t read32(File &f) {
	uint32_t result;
	((uint8_t *)&result)[0] = f.read(); // LSB
	((uint8_t *)&result)[1] = f.read();
	((uint8_t *)&result)[2] = f.read();
	((uint8_t *)&result)[3] = f.read(); // MSB
	return result;
}