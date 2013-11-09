/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2013 Nikita Kindt (n.kindt.pdbg@gmail.com)              *
 *                                                                         *
 *   File is part of PixelDbg:                                             *
 *   https://sourceforge.net/projects/pixeldbg/                            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *                                                                         *
 ***************************************************************************/

#ifndef __MAIN_H
#define __MAIN_H

#include <iostream>
#include <set>
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Input_Choice.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_BMP_Image.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/fl_message.H>

typedef signed char i8;
typedef unsigned char u8;
typedef signed short i16;
typedef unsigned short u16;
typedef signed int i32;
typedef unsigned int u32;

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#pragma pack(push, packing)
#pragma pack(1)
struct TgaHeader
{
	u8 identsize;		// Size of id field that follows 18 byte header (0 usually)
	u8 colormaptype;	// Type of color map 0 = none, 1 = has palette
	u8 imagetype;		// Type of image 0 = none,1 = indexed,2 = rgb,3 = grey, +8 = rle packed
	u16 palletestart;	// First color map entry in palette
	u16 palettelength;	// Number of colors in palette
	u8 palettebits;		// Number of bits per palette entry 15,16,24,32
	u16 xstart;			// Image x origin
	u16 ystart;			// Image y origin
	u16 width;			// Image width
	u16 height;			// Image height
	u8 bpp;				// Image bit depth
	u8 descriptor;		// Image descriptor bits (vh flip bits)
};
#pragma pack(pop)

template <typename T> class Point2D
{
public:
	Point2D(T _x = T(0), T _y = T(0)) : x(_x), y(_y) {}
	
	void set(T _x, T _y) { x = _x; y = _y; }
	
	T x;
	T y;
};

class MyWindow : public Fl_Double_Window
{
public:
	static const u32 kMaxDim = 1024;
	static const u32 kMaxBufferSize = kMaxDim * kMaxDim * 4;
	static const u32 kMaxImageSize = kMaxDim * kMaxDim * 3;
	static const float kVersion = 0.6f;
	
	MyWindow(const char* text) :
		Fl_Double_Window(855, 515, text),
		m_dimGroup(5, 2, 195, 90),
		m_fileGroup(5, 95, 195, 80),
		m_formatGroup(5, 178, 195, 106),
        m_opsGroup(5, 287, 195, 65),
		m_width(120, 5, 70, 20, "Width [1, 1024]:"),
		m_height(120, 27, 70, 20, "Height [1, 1024]:"),
		m_data(60, 49, 130, 22, "Data:"),
		m_byteCount(5, 70, 200, 22),
		m_offset(110, 145, 80, 22, "Offset:"),
		m_saveButton(15, 100, 90, 40, "Save image"),
		m_openButton(110, 100, 80, 40, "Open file"),
		m_autoReload(11, 145, 50, 22, "Auto"),
		m_redChannel(30, 182, 24, 20, "R:"),
		m_greenChannel(75, 182, 24, 20, "G:"),
		m_blueChannel(120, 182, 24, 20, "B:"),
		m_alphaChannel(165, 182, 24, 20, "A:"),
		m_rgbaBits(85, 205, 105, 20, "RGBA bits:"),
		m_formatInfo(10, 230, 180, 22),
		m_tile(11, 257, 47, 20, "Tile"),
		m_tileX(78, 257, 45, 20, "X:"),
		m_tileY(145, 257, 45, 20, "Y:"),
		m_flipV(11, 291, 185, 20, "Flip vertically"),
		m_flipH(11, 309, 185, 20, "Flip horizontally"),
		m_colorCount(11, 327, 185, 20, "Count colors"),
		m_imageBox(210, 2, 0, 0),
		m_aboutButton(5, 357, 195, 23, "About"),
		m_windowSize(this->w(), this->h()),
		m_cursorChanged(false),
		m_accumOffset(0),
		m_offsetChanged(false)
	{	
		// Limit window size on resize (1x352 as minimum image)
		size_range(216, 355, 1239, 1045);
		
		m_dimGroup.box(FL_ENGRAVED_BOX);
		m_dimGroup.color(FL_DARK1);
		m_fileGroup.box(FL_ENGRAVED_BOX);
		m_fileGroup.color(FL_DARK1);
		m_formatGroup.box(FL_ENGRAVED_BOX);
		m_formatGroup.color(FL_DARK1);
		m_opsGroup.box(FL_ENGRAVED_BOX);
		m_opsGroup.color(FL_DARK1);
		
		m_width.maximum_size(4);
		m_width.insert("640");
		m_width.type(FL_INT_INPUT);
		m_width.textfont(FL_COURIER);
		m_width.textsize(12);
		m_width.when(FL_WHEN_CHANGED);
		m_width.callback(DimCallback, this);
		m_width.tooltip("Image width to display loaded data.");

		m_height.maximum_size(4);
		m_height.insert("512");
		m_height.type(FL_INT_INPUT);
		m_height.textfont(FL_COURIER);
		m_height.textsize(12);
		m_height.when(FL_WHEN_CHANGED);
		m_height.callback(DimCallback, this);
		m_height.tooltip("Image height to display loaded data.");
		
		m_data.maximum_size(kMaxDim * kMaxDim * 4);
		m_data.textfont(FL_COURIER);
		m_data.textsize(12);
		m_data.when(FL_WHEN_CHANGED);
		m_data.callback(UpdateCallback, this);
		m_data.tooltip("Data that is displayed as the actual image. Different data can be inserted or deleted here like normal text.");
		
		char buff[64];
		snprintf(buff, sizeof(buff), "0/%u Bytes", kMaxDim * kMaxDim * 4);
		m_byteCount.copy_label(buff);
		
		m_offset.maximum_size(10);
		m_offset.insert("0");
		m_offset.type(FL_INT_INPUT);
		m_offset.textfont(FL_COURIER);
		m_offset.textsize(12);
		m_offset.callback(OffsetCallback, this);
		m_offset.tooltip("File offset to read from when opening file. Offset can be picked from the image by holding CTRL. "
                         "File offset is not adjusted automatically if data is inserted or deleted manually in Data editbox!");

		m_saveButton.box(FL_THIN_UP_BOX);
		m_saveButton.when(FL_WHEN_RELEASE);
		m_saveButton.callback(ButtonCallback, this);
		m_openButton.box(FL_THIN_UP_BOX);
		m_openButton.when(FL_WHEN_RELEASE);
		m_openButton.callback(ButtonCallback, this);
		
		m_autoReload.down_box(FL_DIAMOND_DOWN_BOX);
		m_autoReload.when(FL_WHEN_CHANGED);
		m_autoReload.callback(OffsetCallback, this);
		m_autoReload.tooltip("When checked, auto-reload current file after each offset pick (CTRL + mouse move).");
		
		m_redChannel.maximum_size(1);
		m_redChannel.insert("3");
		m_redChannel.type(FL_INT_INPUT);
		m_redChannel.textfont(FL_COURIER);
		m_redChannel.textsize(12);
		m_redChannel.when(FL_WHEN_CHANGED);
		m_redChannel.callback(ChannelCallback, this);
		m_redChannel.tooltip("Red channel order in data stream (can be 0 if red bits are 0).");
		
		m_greenChannel.maximum_size(1);
		m_greenChannel.insert("2");
		m_greenChannel.type(FL_INT_INPUT);
		m_greenChannel.textfont(FL_COURIER);
		m_greenChannel.textsize(12);
		m_greenChannel.when(FL_WHEN_CHANGED);
		m_greenChannel.callback(ChannelCallback, this);
		m_greenChannel.tooltip("Green channel order in data stream (can be 0 if green bits are 0).");
		
		m_blueChannel.maximum_size(1);
		m_blueChannel.insert("1");
		m_blueChannel.type(FL_INT_INPUT);
		m_blueChannel.textfont(FL_COURIER);
		m_blueChannel.textsize(12);
		m_blueChannel.when(FL_WHEN_CHANGED);
		m_blueChannel.callback(ChannelCallback, this);
		m_blueChannel.tooltip("Blue channel order in data stream (can be 0 if blue bits are 0).");
		
		m_alphaChannel.maximum_size(1);
		m_alphaChannel.insert("4");
		m_alphaChannel.type(FL_INT_INPUT);
		m_alphaChannel.textfont(FL_COURIER);
		m_alphaChannel.textsize(12);
		m_alphaChannel.when(FL_WHEN_CHANGED);
		m_alphaChannel.callback(ChannelCallback, this);
		m_alphaChannel.tooltip("Alpha channel order in data stream (can be 0 if alpha bits are 0).");
		
		m_rgbaBits.maximum_size(7);
		m_rgbaBits.insert("8.8.8.0");
		m_rgbaBits.textfont(FL_COURIER);
		m_rgbaBits.textsize(12);
		m_rgbaBits.when(FL_WHEN_CHANGED);
		m_rgbaBits.callback(ChannelCallback, this);
		m_rgbaBits.tooltip("Pixel and channel size used to interpret the data stream. Can only be 8, 16, 24 or 32 bpp.");
				
		m_tile.when(FL_WHEN_CHANGED);
		m_tile.down_box(FL_DIAMOND_DOWN_BOX);
		m_tile.callback(TileCallback, this);
		m_tile.tooltip("If checked, displayed data stream is tiled in X and Y. I.e. some game consoles use tiled memory.");
		
		m_tileX.maximum_size(m_width.maximum_size());
		m_tileX.insert("32");
		m_tileX.type(FL_INT_INPUT);
		m_tileX.textfont(FL_COURIER);
		m_tileX.textsize(12);
		m_tileX.when(FL_WHEN_CHANGED);
		m_tileX.deactivate();
		m_tileX.callback(TileCallback, this);
		m_tileX.tooltip("Tile image in X. Number of tiles is calculated as (width / tileX).");
		
		m_tileY.maximum_size(m_height.maximum_size());
		m_tileY.insert("32");
		m_tileY.type(FL_INT_INPUT);
		m_tileY.textfont(FL_COURIER);
		m_tileY.textsize(12);
		m_tileY.when(FL_WHEN_CHANGED);
		m_tileY.deactivate();
		m_tileY.callback(TileCallback, this);
		m_tileY.tooltip("Tile image in Y. Number of tiles is calculated as (height / tileY).");
		
		m_flipV.when(FL_WHEN_CHANGED);
		m_flipV.down_box(FL_DIAMOND_DOWN_BOX);
		m_flipV.callback(UpdateCallback, this);
		m_flipV.tooltip("If checked, flip vertically on each redraw.");
		
		m_flipH.when(FL_WHEN_CHANGED);
		m_flipH.down_box(FL_DIAMOND_DOWN_BOX);
		m_flipH.callback(UpdateCallback, this);
		m_flipH.tooltip("If checked, flip horizontally on each redraw.");
		
		m_colorCount.when(FL_WHEN_CHANGED);
		m_colorCount.down_box(FL_DIAMOND_DOWN_BOX);
		m_colorCount.callback(OpsCallback, this);
		m_colorCount.tooltip("If checked, count unique colors every time the image changes.");
		
		m_aboutButton.box(FL_THIN_UP_BOX);
		m_aboutButton.when(FL_WHEN_RELEASE);
		m_aboutButton.callback(ButtonCallback, this);
		
		m_pixels = new u8[kMaxDim * kMaxDim * 3]; // BGR as pixels in window
		m_text = new char[kMaxDim * kMaxDim * 4]; // BGRA as text in editbox
		m_image = 0;
		
		// Show current pixel format
		updatePixelFormat(true);
	}
	
	~MyWindow()
	{
		delete [] m_text;
		delete [] m_pixels;
	}
	
	virtual void draw();
	virtual int handle(int event);

	int getRedBits() const;
	int getGreenBits() const;
	int getBlueBits() const;
	int getAlphaBits() const;
	bool getPixelFormat(int bitMask[4], int rgbaChannels[4], int rgbaBits[4], int& pixelSize) const;
	int getPixelSize() const;
	bool updatePixelFormat(bool startup = false);
	void convertData(const u8* data, u8* rgbOut, u32 size, u32 tileX, u32 tileY);
	bool writeBitmap(const char* filename);
	bool writeTga(const char* filename);
	
	// Inline
	bool isValid() const
	{
		return m_formatGroup.color() != FL_RED;
	}
	
	int getImageWidth() const
	{
		return atoi(m_width.value());
	}
	
	int getImageHeight() const
	{
		return atoi(m_height.value());
	}
	
	int getOffset() const
	{
		return atoi(m_offset.value());
	}
	
private:
	static void ButtonCallback(Fl_Widget* widget, void* param);
	static void OffsetCallback(Fl_Widget* widget, void* param);
	static void ChannelCallback(Fl_Widget* widget, void* param);
	static void DimCallback(Fl_Widget* widget, void* param);
	static void TileCallback(Fl_Widget* widget, void* param);
	static void OpsCallback(Fl_Widget* widget, void* param);
	static void UpdateCallback(Fl_Widget* widget, void* param);

public:
	Fl_Box m_dimGroup;
	Fl_Box m_fileGroup;
	Fl_Box m_formatGroup;
    Fl_Box m_opsGroup;
	Fl_Input m_width;
	Fl_Input m_height;
	Fl_Input m_data;
	Fl_Box m_byteCount;
	Fl_Input m_offset;
	Fl_Button m_saveButton;
	Fl_Button m_openButton;
	Fl_Check_Button m_autoReload;
	Fl_Input m_redChannel;
	Fl_Input m_greenChannel;
	Fl_Input m_blueChannel;
	Fl_Input m_alphaChannel;
	Fl_Input m_rgbaBits;
	Fl_Box m_formatInfo;
	Fl_Check_Button m_tile;
	Fl_Input m_tileX;
	Fl_Input m_tileY;
	Fl_Check_Button m_flipV;
	Fl_Check_Button m_flipH;
	Fl_Check_Button m_colorCount;
	Fl_Box m_imageBox;
	Fl_Button m_aboutButton;
	Fl_RGB_Image* m_image;
	
	u8* m_pixels; // Displayed pixel data in main window
	char* m_text; // Data text full size (to avoid memory reallocs)
	Point2D<int> m_windowSize; // Cached size for resize checks
	bool m_cursorChanged;
	u32 m_accumOffset;
	bool m_offsetChanged;
	char m_offsetText[32]; // To avoid memory allocs
	char m_currentFile[260];
	char m_rawMemoryFlRGBImage[sizeof(Fl_RGB_Image)];
};

#endif

