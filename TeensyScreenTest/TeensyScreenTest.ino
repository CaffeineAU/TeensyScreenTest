/*
 Name:		TeensyScreenTest.ino
 Created:	11/16/2016 11:33:00 PM
 Author:	Liam O
*/

// the setup function runs once when you press reset or power the board


//#include <hsv2rgb.h>
#include "font_Michroma.h"
#include "font_Oxygen-Bold.h"

#include <SysCall.h>
#include <SdFatConfig.h>
#include <MinimumSerial.h>
#include <BlockDriver.h>
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

#define ROWSPERDRAW 80
uint16_t awColors[240 * ROWSPERDRAW];  // hold colors for one row at a time...


void setup() {
	delay(200);

	tft.begin();
	tft.fillScreen(ILI9341_WHITE);

	tft.setFont(Oxygen_14_Bold);

	Serial.print(F("Initializing SD card..."));
	//tft.println(F("Init SD card..."));
	tft.setTextColor(ILI9341_BLACK);

	//if (!(SD.begin(BUILTIN_SDCARD))) {
	//	while (1) {
	//		Serial.println("Unable to access the SD card");
	//		delay(500);
	//	}
	//}
	sd.begin();

	Serial.println("OK!");

	//bmpDrawScale("triangle.bmp", 0, 0,2);
	int border = 5;
	tft.fillRect((tft.width() / 4) - border, (tft.height() / 4) - border, 120 + (border * 2), 160 + (border * 2), ILI9341_BLACK);

}

uint32_t FreeRam() { // for Teensy 3.0
	uint32_t stackTop;
	uint32_t heapTop;

	// current position of the stack.
	stackTop = (uint32_t)&stackTop;

	// current position of heap.
	void* hTop = malloc(1);
	heapTop = (uint32_t)hTop;
	free(hTop);

	// The difference is the free, available ram.
	return stackTop - heapTop;
}

char filename[255];
int xoff = 1;
int yoff = 0;
int drawscale = 4;

char prevfilename[255] = "";

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
		//Serial.println(filename);
		//Serial.print("x:");
		//Serial.print(xoff);
		//Serial.print(" y:");
		//Serial.println(yoff);
		if (bmpDrawScale(filename, 60, 80, 2))
		{
			//if (bmpDrawScale(filename, xoff % 240 - 1, yoff, drawscale))
			//{
			//	//delay(1500);
			//	//file.printName(&tft);
			//	xoff += (240 / drawscale);
			//	//if (xoff >= 240)
			//	//{
			//	//	xoff = 0;
			//	//}
			//	yoff = (xoff / 240) * (320 / drawscale);
			//	if (yoff >= 320)
			//	{
			//		yoff = 0;
			//		xoff = 1;
			//	}
			//}
			char filenametoprint[255];
			
			SubStringBeforeChar(filename, filenametoprint, '.');

			tft.setCursor(60, 250);
			tft.setTextColor(ILI9341_WHITE);
			//Serial.print("writing white ");
			//Serial.print(prevfilename);
			tft.println(prevfilename);
			tft.setTextColor(ILI9341_BLACK);
			tft.setCursor(60, 250);
			tft.println(filenametoprint);
			//Serial.print(" writing black ");
			//Serial.println(filenametoprint);
			strcpy(prevfilename, filenametoprint);
			delay(2000);
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

void SubStringBeforeChar(char *in, char *out, char delimiter)
{
	int i = 0;
	while (in[i] != delimiter)
	{
		out[i] = in[i];
		i++;
	}

	out[i] = 0;

}

#define BUFFPIXEL 240


bool bmpDrawScale(const char *filename, uint8_t x, uint16_t y, int scale) {

	File     bmpFile;
	int      bmpWidth, bmpHeight;   // W+H in pixels
	uint8_t  bmpDepth;              // Bit depth (currently must be 24)
	uint32_t bmpImageoffset;        // Start of image data in file
	uint32_t rowSize;               // Not always = bmpWidth; may have padding
	uint8_t  sdbuffer[3 * BUFFPIXEL*ROWSPERDRAW]; // pixel buffer (R+G+B per pixel)
	uint32_t buffidx = sizeof(sdbuffer); // Current position in sdbuffer
	boolean  goodBmp = false;       // Set to true on valid header parse
	boolean  flip = true;        // BMP is stored bottom-to-top
	int      row, col;
	uint8_t  r, g, b;
	uint32_t pos = 0, startTime = millis();

	if ((x >= tft.width()) || (y >= tft.height())) return false;

	Serial.println();
	Serial.print("Filename: ");
	Serial.println(filename);
	//Serial.println('\'');

	//Serial.print("x:");
	//Serial.print(x);
	//Serial.print(" y:");
	//Serial.println(y);


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
			//Serial.print(F("Bit Depth: ")); Serial.println(bmpDepth);
			if ((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

				goodBmp = true; // Supported BMP format -- proceed!
				//Serial.print(F("Image size: "));
				//Serial.print(bmpWidth);
				//Serial.print('x');
				//Serial.println(bmpHeight);

				// BMP rows are padded (if needed) to 4-byte boundary
				rowSize = (bmpWidth * 3 + 3) & ~3;

				// If bmpHeight is negative, image is in top-down order.
				// This is not canon but has been observed in the wild.
				if (bmpHeight < 0) {
					bmpHeight = -bmpHeight;
					flip = false;
				}

				// Crop area to be loaded
				//w = bmpWidth;
				//h = bmpHeight;
				//if ((x + w - 1) >= tft.width())  w = tft.width() - x;
				//if ((y + h - 1) >= tft.height()) h = tft.height() - y;


				for (row = 0; row < bmpHeight; row += ROWSPERDRAW) { // For each scanline...

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
						//Serial.print("Seeking to ");
						//Serial.println(pos);
						buffidx = sizeof(sdbuffer); // Force buffer reload
					}
					for (int i = ROWSPERDRAW; i > 0; i--)
					{
						int index = 0;

						for (col = 0; col < (bmpWidth); col++) { // For each pixel...
																		// Time to read more pixel data?
							if (buffidx >= sizeof(sdbuffer)) { // Indeed
								bmpFile.read(sdbuffer, sizeof(sdbuffer));
								buffidx = 0; // Set index to beginning
							}

							b = sdbuffer[buffidx++];
							g = sdbuffer[buffidx++];
							r = sdbuffer[buffidx++];
							// Convert pixel from BMP to TFT format, push to display
							if (i%scale == 0)
							{
								if (index++%scale == 0)
								{
									awColors[(index / scale) + (((i / scale) - 1)*bmpWidth / scale)] = tft.color565(r, g, b);
								}
								//Serial.print("col : ");
								//Serial.print(col);
								//Serial.print(" buffidx : ");
								//Serial.print(buffidx);
								//Serial.print(" row : ");
								//Serial.print(i);
								//Serial.print(" output bufferindex : ");
								//Serial.println((index / scale) + (((i / scale) - 1)*(w/scale)));
							} // end pixel
						}
					}
					tft.writeRect(x, y + (row / scale), bmpWidth / scale, ROWSPERDRAW / scale, awColors);
				} // end scanline
				Serial.print(F("Loaded in ")); Serial.print(millis() - startTime); Serial.println(" ms");
				//Serial.print(F("Free Stack :"));
				//Serial.println(FreeRam());
			} // end goodBmp
		}
	}
	else
	{
		return false;
	}
	bmpFile.close();
	if (!goodBmp) Serial.println(F("BMP format not recognized."));
	return true;
}



// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(File &f) {
	uint8_t readValues[2];
	f.read(readValues, 2);
	uint16_t *result = (uint16_t*)readValues;
	return result[0];
}

uint32_t read32(File &f) {
	uint8_t readValues[4];
	f.read(readValues, 4);
	uint32_t *result = (uint32_t*)readValues;
	return result[0];
}