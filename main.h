/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2013-2014 Nikita Kindt (n.kindt.pdbg@gmail.com)         *
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

#define NOMINMAX

#include <stdarg.h>
#include <assert.h>
#include <iostream>
#include <vector>
#include <set>
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Input_Choice.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Scrollbar.H>
#include <FL/Fl_BMP_Image.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/fl_message.H>
#ifdef _WIN32
#include <windows.h>
#endif

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

class PixelDbgWnd : public Fl_Double_Window
{
public:
	static const u32 kMaxDim = 1024;
	static const u32 kMaxBufferSize = kMaxDim * kMaxDim * 4;
	static const u32 kMaxImageSize = kMaxDim * kMaxDim * 3;
	static const u32 kVersionMajor = 0;
	static const u32 kVersionMinor = 8;
	
	#define RECT_RIGHT(__wdg__) __wdg__.x() + __wdg__.w()
	#define RECT_BOTTOM(__wdg__) __wdg__.y() + __wdg__.h()

	enum ConvertFlags
	{
		CF_IgnoreChannelOrder = (1<<0),
		CF_IgnoreTiles = (1<<1),
		CF_IgnoreRedChannel = (1<<2),
		CF_IgnoreGreenChannel = (1<<3),
		CF_IgnoreBlueChannel = (1<<4),
		CF_IgnoreAlphaChannel = (1<<5)
	};

	struct BitwiseOp
	{
		enum Op
		{
			OP_NOP = 0,
			OP_AND,
			OP_OR,
			OP_XOR,
			OP_SHL,
			OP_SHR,
			OP_ROL,
			OP_ROR
		};

		Op op;
		u8 r, g, b; // bits
	};

	PixelDbgWnd(const char* text) :
		Fl_Double_Window(881, 536, text),
		m_leftArea(0, 0, 220, h()),
		m_dimGroup(5, 1, 195, 91),
		m_fileGroup(5, 95, 195, 80),
		m_formatGroup(5, 178, 195, 124),
		m_paletteGroup(5, 305, 195, 108),
		m_bitwiseGroup(5, 416, 195, 119),
		m_opsGroup(5, 538, 195, 116),
		m_width(120, 5, 70, 20, "Width [1, 1024]:"),
		m_height(120, 27, 70, 20, "Height [1, 1024]:"),
		m_data(50, 49, 140, 21, "Data:"),
		m_backwardButton(145, 70, 23, 20, "@<"),
		m_forwardButton(168, 70, 23, 20, "@>"),
		m_byteCount(5, 70, 140, 20),
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
		m_redMask(11, 252, 31, 20, "R"),
		m_greenMask(42, 252, 31, 20, "G"),
		m_blueMask(73, 252, 31, 20, "B"),
		m_alphaMask(104, 252, 31, 20, "A"),
		m_tile(11, 275, 47, 20, "Tile"),
		m_tileX(78, 275, 45, 20, "X:"),
		m_tileY(145, 275, 45, 20, "Y:"),
		m_paletteMode(11, m_paletteGroup.y() + 4, 70, 20, "Palette"),
		m_paletteIndices(91, m_paletteGroup.y() + 4, 105, 20, ""),
		m_paletteOffset(110, RECT_BOTTOM(m_paletteIndices) + 2, 80, 20, "From offset:"),
		m_loadPalette(15, RECT_BOTTOM(m_paletteOffset) + 5, 175, 25, "Load palette (offset / file)"),
		m_savePalette(15, RECT_BOTTOM(m_loadPalette) + 2, 175, 25, "Save palette"),
		m_bitwiseStage1(30, m_bitwiseGroup.y() + 4, 60, 20, "1."),
		m_bitwiseStage1Bits(95, m_bitwiseGroup.y() + 4, 95, 20),
		m_bitwiseStage2(30, RECT_BOTTOM(m_bitwiseStage1) + 2, 60, 20, "2."),
		m_bitwiseStage2Bits(95, RECT_BOTTOM(m_bitwiseStage1) + 2, 95, 20),
		m_bitwiseStage3(30, RECT_BOTTOM(m_bitwiseStage2) + 2, 60, 20, "3."),
		m_bitwiseStage3Bits(95, RECT_BOTTOM(m_bitwiseStage2) + 2, 95, 20),
		m_bitwiseStage4(30, RECT_BOTTOM(m_bitwiseStage3) + 2, 60, 20, "4."),
		m_bitwiseStage4Bits(95, RECT_BOTTOM(m_bitwiseStage3) + 2, 95, 20),
		m_bitwiseStage5(30, RECT_BOTTOM(m_bitwiseStage4) + 2, 60, 20, "5."),
		m_bitwiseStage5Bits(95, RECT_BOTTOM(m_bitwiseStage4) + 2, 95, 20),
		m_DXTMode(11, m_opsGroup.y() + 4, 100, 20, "Interpret as: "),
		m_DXTType(113, m_opsGroup.y() + 4, 75, 20),
		m_RLEMode(11, RECT_BOTTOM(m_DXTMode) + 2, 100, 20, "Interpret as: "),
		m_RLEType(113, RECT_BOTTOM(m_DXTMode) + 2, 75, 20),
		m_flipV(11, RECT_BOTTOM(m_RLEMode) + 2, 110, 20, "Flip vertically"),
		m_flipH(11, RECT_BOTTOM(m_flipV) + 2, 125, 20, "Flip horizontally"),
		m_colorCount(11, RECT_BOTTOM(m_flipH) + 2, 150, 20, "Count colors"),
		m_aboutButton(5, RECT_BOTTOM(m_opsGroup) + 4, 195, 23, "About"),
		m_windowSize(w(), h()),
		m_cursorChanged(false),
		m_accumOffset(0),
		m_offsetChanged(false),
		m_currentFileSize(0)
	{
		// Limit window size on resize (1x70 as minimum image)
		size_range(242, 93, 1265, 1075);

		m_leftArea.end();
		m_leftArea.type(Fl_Scroll::VERTICAL_ALWAYS);
		
		m_dimGroup.box(FL_ENGRAVED_BOX);
		m_dimGroup.color(FL_DARK1);
		m_fileGroup.box(FL_ENGRAVED_BOX);
		m_fileGroup.color(FL_DARK1);
		m_formatGroup.box(FL_ENGRAVED_BOX);
		m_formatGroup.color(FL_DARK1);
		m_bitwiseGroup.box(FL_ENGRAVED_BOX);
		m_bitwiseGroup.color(FL_DARK1);
		m_paletteGroup.box(FL_ENGRAVED_BOX);
		m_paletteGroup.color(FL_DARK1);
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
		m_height.insert("534");
		m_height.type(FL_INT_INPUT);
		m_height.textfont(FL_COURIER);
		m_height.textsize(12);
		m_height.when(FL_WHEN_CHANGED);
		m_height.callback(DimCallback, this);
		m_height.tooltip("Image height to display loaded data.");
		
		m_data.maximum_size(kMaxBufferSize);
		m_data.textfont(FL_COURIER);
		m_data.textsize(12);
		m_data.when(FL_WHEN_CHANGED);
		m_data.callback(RedrawCallback, this);
		m_data.tooltip("Data in bytes that is displayed as the actual image. Using up/down keys will reload file from previous/next offset.");

		m_backwardButton.box(FL_NO_BOX);
		m_backwardButton.when(FL_WHEN_RELEASE);
		m_backwardButton.callback(DimCallback, this);
		m_backwardButton.tooltip("Move offset 1 byte backwards (clamped to 0).");

		m_forwardButton.box(FL_NO_BOX);
		m_forwardButton.when(FL_WHEN_RELEASE);
		m_forwardButton.callback(DimCallback, this);
		m_forwardButton.tooltip("Move offset 1 byte forward (clamped to file size).");

		m_byteCount.labelsize(11);
		m_byteCount.label("Visible: 0.00 %");
		
		m_offset.maximum_size(10);
		m_offset.insert("0");
		m_offset.type(FL_INT_INPUT);
		m_offset.textfont(FL_COURIER);
		m_offset.textsize(12);
		m_offset.when(FL_WHEN_ENTER_KEY_ALWAYS);
		m_offset.callback(OffsetCallback, this);
		m_offset.tooltip("File offset to read from when opening file. Offset can be picked from the image by holding CTRL. Using up/down keys will reload file from previous/next offset.");

		m_saveButton.box(FL_THIN_UP_BOX);
		m_saveButton.when(FL_WHEN_RELEASE);
		m_saveButton.callback(ButtonCallback, this);
		m_openButton.box(FL_THIN_UP_BOX);
		m_openButton.when(FL_WHEN_RELEASE);
		m_openButton.callback(ButtonCallback, this);
		
		m_autoReload.down_box(FL_DIAMOND_DOWN_BOX);
		m_autoReload.when(FL_WHEN_CHANGED);
		m_autoReload.callback(OffsetCallback, this);
		m_autoReload.tooltip("If checked, auto-reload current file after each offset pick (CTRL + mouse move). On a compressed data stream (i.e. S3TC/RLE) picking might be not very accurate.");
		
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

		m_redMask.value(1);
		m_redMask.down_box(FL_DIAMOND_DOWN_BOX);
		m_redMask.when(FL_WHEN_CHANGED);
		m_redMask.callback(ChannelCallback, this);
		m_redMask.tooltip("Enable or disable red channel.");

		m_greenMask.value(1);
		m_greenMask.down_box(FL_DIAMOND_DOWN_BOX);
		m_greenMask.when(FL_WHEN_CHANGED);
		m_greenMask.callback(ChannelCallback, this);
		m_greenMask.tooltip("Enable or disable green channel.");

		m_blueMask.value(1);
		m_blueMask.down_box(FL_DIAMOND_DOWN_BOX);
		m_blueMask.when(FL_WHEN_CHANGED);
		m_blueMask.callback(ChannelCallback, this);
		m_blueMask.tooltip("Enable or disable blue channel.");

		m_alphaMask.value(0);
		m_alphaMask.down_box(FL_DIAMOND_DOWN_BOX);
		m_alphaMask.when(FL_WHEN_CHANGED);
		m_alphaMask.callback(ChannelCallback, this);
		m_alphaMask.tooltip("Enable or disable alpha channel. Alpha channel is replicated in RGB channels and can use same operations as normal RGB data.");

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
	
		m_paletteMode.when(FL_WHEN_CHANGED);
		m_paletteMode.down_box(FL_DIAMOND_DOWN_BOX);
		m_paletteMode.callback(RedrawCallback, this);
		m_paletteMode.tooltip("If checked, use palette to display image. Every byte acts as a look-up index (0-255) into current palette. Palette consists of 256 pixels with the specified pixel format.");
		m_paletteMode.when(FL_WHEN_CHANGED);
		m_paletteMode.callback(PaletteCallback, this);

		m_paletteIndices.label("Used: 0-0");
		m_paletteIndices.deactivate();

		m_paletteOffset.maximum_size(m_offset.maximum_size());
		m_paletteOffset.insert("0");
		m_paletteOffset.type(FL_INT_INPUT);
		m_paletteOffset.textfont(FL_COURIER);
		m_paletteOffset.textsize(12);
		m_paletteOffset.when(FL_WHEN_ENTER_KEY_ALWAYS);
		m_paletteOffset.callback(PaletteCallback, this);
		m_paletteOffset.deactivate();
		m_paletteOffset.tooltip("Specifies the offset from where a palette should be loaded. A value of 0 will load new palette from another file. A value greater 0 will load a palette from current file. Using up/down keys will reload palette from previous/next offset.");
		
		m_loadPalette.box(FL_THIN_UP_BOX);
		m_loadPalette.when(FL_WHEN_RELEASE);
		m_loadPalette.callback(PaletteCallback, this);
		m_loadPalette.deactivate();
		m_loadPalette.tooltip("Load palette from specified offset in current file or from another file if palette offset is 0. If file type is tga or bmp, the actual pixels are used as palette colors and file header is ignored.");
		
		m_savePalette.box(FL_THIN_UP_BOX);
		m_savePalette.when(FL_WHEN_RELEASE);
		m_savePalette.callback(PaletteCallback, this);
		m_savePalette.deactivate();
		m_savePalette.tooltip("Save current palette as 24bpp bitmap file.");

		const char* bwTooltip = "Operation to be performed on incoming pixel.\n\nNOP - do nothing\n"
							    "AND - bitwise AND (0111 AND 0101 = 0101)\n"
								"OR  - bitwise OR (0111 OR 0101 = 0111)\n"
								"XOR - bitwise eXclusive OR (0111 XOR 0101 = 0010)\n"
								"SHL - SHift Left by given number (0011 SHL 1 = 0110)\n"
								"SHR - SHift Right by given number (0011 SHR 1 = 0001)\n"
								"ROL - ROtate Left by given number (1001 ROL 1 = 0011)\n"
								"ROR - ROtate Right by given number (1001 ROR 1 = 1100)";

		m_bitwiseStage1.textfont(FL_COURIER);
		m_bitwiseStage1.textsize(12);
		m_bitwiseStage1.add("NOP");
		m_bitwiseStage1.add("AND");
		m_bitwiseStage1.add("OR");
		m_bitwiseStage1.add("XOR");
		m_bitwiseStage1.add("SHL");
		m_bitwiseStage1.add("SHR");
		m_bitwiseStage1.add("ROL");
		m_bitwiseStage1.add("ROR");
		m_bitwiseStage1.value(0);
		m_bitwiseStage1.when(FL_WHEN_CHANGED);
		m_bitwiseStage1.callback(BitwiseCallback, this);
		m_bitwiseStage1.tooltip(bwTooltip);
		m_bitwiseStage1Bits.maximum_size(8);
		m_bitwiseStage1Bits.textfont(FL_COURIER);
		m_bitwiseStage1Bits.textsize(12);
		m_bitwiseStage1Bits.deactivate();
		m_bitwiseStage1Bits.value("ff.ff.ff");
		m_bitwiseStage1Bits.when(FL_WHEN_CHANGED);
		m_bitwiseStage1Bits.callback(BitwiseCallback, this);
		m_bitwiseStage1Bits.tooltip("R.G.B bits in hexadecimal representation used by selected operation on incoming pixel.");

		m_bitwiseStage2.textfont(FL_COURIER);
		m_bitwiseStage2.textsize(12);
		m_bitwiseStage2.add("NOP");
		m_bitwiseStage2.add("AND");
		m_bitwiseStage2.add("OR");
		m_bitwiseStage2.add("XOR");
		m_bitwiseStage2.add("SHL");
		m_bitwiseStage2.add("SHR");
		m_bitwiseStage2.add("ROL");
		m_bitwiseStage2.add("ROR");
		m_bitwiseStage2.value(0);
		m_bitwiseStage2.when(FL_WHEN_CHANGED);
		m_bitwiseStage2.callback(BitwiseCallback, this);
		m_bitwiseStage2.tooltip(bwTooltip);
		m_bitwiseStage2Bits.maximum_size(8);
		m_bitwiseStage2Bits.textfont(FL_COURIER);
		m_bitwiseStage2Bits.textsize(12);
		m_bitwiseStage2Bits.deactivate();
		m_bitwiseStage2Bits.value("ff.ff.ff");
		m_bitwiseStage2Bits.when(FL_WHEN_CHANGED);
		m_bitwiseStage2Bits.callback(BitwiseCallback, this);
		m_bitwiseStage2Bits.tooltip("R.G.B bits in hexadecimal representation used by selected operation on incoming pixel.");

		m_bitwiseStage3.textfont(FL_COURIER);
		m_bitwiseStage3.textsize(12);
		m_bitwiseStage3.add("NOP");
		m_bitwiseStage3.add("AND");
		m_bitwiseStage3.add("OR");
		m_bitwiseStage3.add("XOR");
		m_bitwiseStage3.add("SHL");
		m_bitwiseStage3.add("SHR");
		m_bitwiseStage3.add("ROL");
		m_bitwiseStage3.add("ROR");
		m_bitwiseStage3.value(0);
		m_bitwiseStage3.when(FL_WHEN_CHANGED);
		m_bitwiseStage3.callback(BitwiseCallback, this);
		m_bitwiseStage3.tooltip(bwTooltip);
		m_bitwiseStage3Bits.maximum_size(8);
		m_bitwiseStage3Bits.textfont(FL_COURIER);
		m_bitwiseStage3Bits.textsize(12);
		m_bitwiseStage3Bits.deactivate();
		m_bitwiseStage3Bits.value("ff.ff.ff");
		m_bitwiseStage3Bits.when(FL_WHEN_CHANGED);
		m_bitwiseStage3Bits.callback(BitwiseCallback, this);
		m_bitwiseStage3Bits.tooltip("R.G.B bits in hexadecimal representation used by selected operation on incoming pixel.");

		m_bitwiseStage4.textfont(FL_COURIER);
		m_bitwiseStage4.textsize(12);
		m_bitwiseStage4.add("NOP");
		m_bitwiseStage4.add("AND");
		m_bitwiseStage4.add("OR");
		m_bitwiseStage4.add("XOR");
		m_bitwiseStage4.add("SHL");
		m_bitwiseStage4.add("SHR");
		m_bitwiseStage4.add("ROL");
		m_bitwiseStage4.add("ROR");
		m_bitwiseStage4.value(0);
		m_bitwiseStage4.when(FL_WHEN_CHANGED);
		m_bitwiseStage4.callback(BitwiseCallback, this);
		m_bitwiseStage4.tooltip(bwTooltip);
		m_bitwiseStage4Bits.maximum_size(8);
		m_bitwiseStage4Bits.textfont(FL_COURIER);
		m_bitwiseStage4Bits.textsize(12);
		m_bitwiseStage4Bits.deactivate();
		m_bitwiseStage4Bits.value("ff.ff.ff");
		m_bitwiseStage4Bits.when(FL_WHEN_CHANGED);
		m_bitwiseStage4Bits.callback(BitwiseCallback, this);
		m_bitwiseStage4Bits.tooltip("R.G.B bits in hexadecimal representation used by selected operation on incoming pixel.");

		m_bitwiseStage5.textfont(FL_COURIER);
		m_bitwiseStage5.textsize(12);
		m_bitwiseStage5.add("NOP");
		m_bitwiseStage5.add("AND");
		m_bitwiseStage5.add("OR");
		m_bitwiseStage5.add("XOR");
		m_bitwiseStage5.add("SHL");
		m_bitwiseStage5.add("SHR");
		m_bitwiseStage5.add("ROL");
		m_bitwiseStage5.add("ROR");
		m_bitwiseStage5.value(0);
		m_bitwiseStage5.when(FL_WHEN_CHANGED);
		m_bitwiseStage5.callback(BitwiseCallback, this);
		m_bitwiseStage5.tooltip(bwTooltip);
		m_bitwiseStage5Bits.maximum_size(8);
		m_bitwiseStage5Bits.textfont(FL_COURIER);
		m_bitwiseStage5Bits.textsize(12);
		m_bitwiseStage5Bits.deactivate();
		m_bitwiseStage5Bits.value("ff.ff.ff");
		m_bitwiseStage5Bits.when(FL_WHEN_CHANGED);
		m_bitwiseStage5Bits.callback(BitwiseCallback, this);
		m_bitwiseStage5Bits.tooltip("R.G.B bits in hexadecimal representation used by selected operation on incoming pixel.");

		m_DXTMode.when(FL_WHEN_CHANGED);
		m_DXTMode.down_box(FL_DIAMOND_DOWN_BOX);
		m_DXTMode.callback(DXTCallback, this);
		m_DXTMode.tooltip("If checked, interpret data stream as selected S3TC DXT variant. RGBA bits can be either 5.6.5.0 or 5.5.5.1, other RGBA bits are ignored.");
		
		m_DXTType.textfont(FL_COURIER);
		m_DXTType.textsize(12);
		m_DXTType.add("DXT1");
		m_DXTType.add("DXT3");
		m_DXTType.add("DXT5");
		m_DXTType.value(0);
		m_DXTType.when(FL_WHEN_CHANGED);
		m_DXTType.callback(DXTCallback, this);
		m_DXTType.deactivate();
		m_DXTType.tooltip("Supported DXT modes. Alpha is ignored on DXT3 and DXT5. DXT1 can be used with 5.5.5.1 to show 1-bit alpha.");

		m_RLEMode.when(FL_WHEN_CHANGED);
		m_RLEMode.down_box(FL_DIAMOND_DOWN_BOX);
		m_RLEMode.callback(RLECallback, this);
		m_RLEMode.tooltip("If checked, interpret data stream as RLE compressed. Last or first byte always represents the run length and following or preceding bytes hold the pixel color as described by the pixel format."
			              " RLE compressed stream doesn't work well with picking and scroll bar length.");
		
		m_RLEType.textfont(FL_COURIER);
		m_RLEType.textsize(12);
		m_RLEType.add("RLE");
		m_RLEType.add("RLE (MSB)");
		m_RLEType.add("RLE (TGA)");
		m_RLEType.value(0);
		m_RLEType.when(FL_WHEN_CHANGED);
		m_RLEType.callback(RLECallback, this);
		m_RLEType.deactivate();
		m_RLEType.tooltip("Supported RLE modes. If MSB is set run length is read from most significant byte otherwise from least significant byte. TGA mode always removes the most significant bit from the run length byte.");

		m_flipV.when(FL_WHEN_CHANGED);
		m_flipV.down_box(FL_DIAMOND_DOWN_BOX);
		m_flipV.callback(RedrawCallback, this);
		m_flipV.tooltip("If checked, flip vertically on each redraw. Top/left image origin will be located at bottom/left instead (affects picking mode).");
		
		m_flipH.when(FL_WHEN_CHANGED);
		m_flipH.down_box(FL_DIAMOND_DOWN_BOX);
		m_flipH.callback(RedrawCallback, this);
		m_flipH.tooltip("If checked, flip horizontally on each redraw. Top/left image origin will be located at top/right instead (affects picking mode).");
		
		m_colorCount.when(FL_WHEN_CHANGED);
		m_colorCount.down_box(FL_DIAMOND_DOWN_BOX);
		m_colorCount.callback(OpsCallback, this);
		m_colorCount.tooltip("If checked, count unique colors every time the image changes.");

		m_aboutButton.box(FL_THIN_UP_BOX);
		m_aboutButton.when(FL_WHEN_RELEASE);
		m_aboutButton.callback(ButtonCallback, this);

		// Don't know how to get those controls into extra box without allocating memory (need to end left area scroll before calling right area ctor)
		m_rightArea = new Fl_Box(m_leftArea.w() + 5, 0, w() - m_leftArea.w(), h());
		m_imageScroll = new Fl_Scrollbar(w() - 15, 0, 15, h());
		m_imageBox = new Fl_Box(m_leftArea.w(), 2, w() - 15 - m_leftArea.w(), h() - 2);

		m_imageScroll->type(FL_VERTICAL);
		m_imageScroll->align(FL_ALIGN_RIGHT);
		m_imageScroll->value(0, 1, 0, 1);
		m_imageScroll->when(FL_WHEN_CHANGED);
		m_imageScroll->callback(ScrollbarCallback, this);
		m_imageScroll->deactivate();
		
		m_pixels = new u8[kMaxImageSize]; // BGR as pixels in window
		m_text = new char[kMaxBufferSize]; // BGRA as text in editbox
		m_image = 0;
		
		// Show current pixel format
		updatePixelFormat(true);

		memset(m_currentFile, 0, sizeof(m_currentFile));
		
		// Fill palette (use green tint at startup)
		for(int i=0; i<256; ++i)
		{
			int lum = 255 - i;
			m_palette[i*3+0] = lum / 32;
			m_palette[i*3+1] = lum;
			m_palette[i*3+2] = lum / 64;
			m_rawPalette[i*3+0] = m_palette[i*3+0];
			m_rawPalette[i*3+1] = m_palette[i*3+1];
			m_rawPalette[i*3+2] = m_palette[i*3+2];
			m_rawPalette[i*3+3] = 0;
		}

		// Create a pixel pipeline with 5 bitwise ops
		m_bitwiseOpVec.reserve(5);
		m_bitwiseOpVec.push_back(BitwiseOp());
		m_bitwiseOpVec.push_back(BitwiseOp());
		m_bitwiseOpVec.push_back(BitwiseOp());
		m_bitwiseOpVec.push_back(BitwiseOp());
		m_bitwiseOpVec.push_back(BitwiseOp());
	}
	
	~PixelDbgWnd()
	{
		delete [] m_text;
		delete [] m_pixels;

		delete m_imageScroll;
		delete m_imageBox;
		delete m_rightArea;

		if(m_image)
		{
			m_image->~Fl_RGB_Image();
		}
	}
	
	virtual void draw();
	virtual int handle(int event);

	bool isFormatValid() const;
	int getRedBits() const;
	int getGreenBits() const;
	int getBlueBits() const;
	int getAlphaBits() const;
	bool getPixelFormat(int bitMask[4], int rgbaChannels[4], int rgbaBits[4], int& pixelSize) const;
	bool getRGBABits(int rgbaBits[4]);
	bool getRGBABitsFromHexString(const char* rgb, int* r = NULL, int* g = NULL, int* b = NULL);
	int getPixelSize() const;
	bool updatePixelFormat(bool startup = false);
	bool updateBitwiseOps();
	void updateScrollbar(u32 pos, bool resize);
	void convertRaw(const u8* data, u32 size, u8* rgbOut, u32 flags = 0, const std::vector<BitwiseOp>* bwOps = NULL, u32 tileX = 0xffff, u32 tileY = 0xffff, u8* palette = NULL);
	void convertDXT(const u8* data, u32 size, u8* rgbOut, u32 flags, int DXTType, bool oneBitAlpha = false);
	void convertRLE(const u8* data, u32 size, u8* rgbOut, u32 flags, u32 RLmask, bool RLmsb, const std::vector<BitwiseOp>* bwOps = NULL);
	void convertPalette(const u8* data, u32 size, u8* rgbOut);
	void flipVertically(int w, int h, void* data);
	u32 readFile(const char* name, void* out, u32 size, u32 offset = 0);
	bool writeBitmap(const char* filename, int width, int height, void* data);
	bool writeTga(const char* filename);
	
	// Inline
	const char* getCurrentFileName() const
	{
		if(m_currentFile[0] != 0)
		{
			#ifdef _WIN32
			const char* s = strrchr(m_currentFile, '\\');
			#else
			const char* s = strrchr(m_currentFile, '/');
			#endif

			return s ? s + 1 : NULL;
		}

		return NULL;
	}

	bool isDimValid() const
	{
		return getImageWidth() != -1 && getImageHeight() != -1;
	}
	
	bool isValid() const
	{
		return isDimValid() && isFormatValid();
	}
	
	int getImageWidth() const
	{
		const char* value = m_width.value();
		return value && value[0] != 0 ? atoi(value) : -1;
	}
	
	int getImageHeight() const
	{
		const char* value = m_height.value();
		return value && value[0] != 0 ? atoi(value) : -1;
	}
	
	int getOffset() const
	{
		return atoi(m_offset.value());
	}

	int getPaletteOffset() const
	{
		return atoi(m_paletteOffset.value());
	}

	u32 getNumVisibleBytes() const
	{
		u32 w = (u32)getImageWidth();
		u32 h = (u32)getImageHeight();
		u32 s = (u32)getPixelSize();
		u32 b = w * h * s;

		if(isDXTMode())
		{
			switch(m_DXTType.value())
			{
			case 0: b /= 6; break;
			case 1:
			case 2: b /= 4; break;
			}
		}

		return b;
	}

	u32 getRGBAIgnoreMask() const
	{
		u32 flags = (m_redMask.value() == 0) ? CF_IgnoreRedChannel : 0;
		flags |= (m_greenMask.value() == 0) ? CF_IgnoreGreenChannel : 0;
		flags |= (m_blueMask.value() == 0) ? CF_IgnoreBlueChannel : 0;
		flags |= (m_alphaMask.value() == 0) ? CF_IgnoreAlphaChannel : 0;
		return flags;
	}

	bool isBitwiseOpMode() const
	{
		bool stage1 = m_bitwiseStage1.value() != 0;
		bool stage2 = m_bitwiseStage2.value() != 0;
		bool stage3 = m_bitwiseStage3.value() != 0;
		bool stage4 = m_bitwiseStage4.value() != 0;
		bool stage5 = m_bitwiseStage5.value() != 0;
		return stage1 || stage2 || stage3 || stage4 || stage5;
	}

	bool isPaletteMode() const
	{
		return m_paletteMode.value() != 0;
	}

	bool isDXTMode() const
	{
		return m_DXTMode.value() != 0;
	}

	bool isRLEMode() const
	{
		return m_RLEMode.value() != 0;
	}

	Fl_Box& getImageBox() const
	{
		return *m_imageBox;
	}

private:
	static void ButtonCallback(Fl_Widget* widget, void* param);
	static void OffsetCallback(Fl_Widget* widget, void* param);
	static void ChannelCallback(Fl_Widget* widget, void* param);
	static void DimCallback(Fl_Widget* widget, void* param);
	static void TileCallback(Fl_Widget* widget, void* param);
	static void BitwiseCallback(Fl_Widget* widget, void* param);
	static void PaletteCallback(Fl_Widget* widget, void* param);
	static void DXTCallback(Fl_Widget* widget, void* param);
	static void RLECallback(Fl_Widget* widget, void* param);
	static void OpsCallback(Fl_Widget* widget, void* param);
	static void ScrollbarCallback(Fl_Widget* widget, void* param);
	static void RedrawCallback(Fl_Widget* widget, void* param);

	// UI controls
	Fl_Scroll m_leftArea;
	Fl_Box m_dimGroup;
	Fl_Box m_fileGroup;
	Fl_Box m_formatGroup;
	Fl_Box m_paletteGroup;
	Fl_Box m_bitwiseGroup;
	Fl_Box m_opsGroup;
	Fl_Input m_width;
	Fl_Input m_height;
	Fl_Output m_data;
	Fl_Button m_backwardButton;
	Fl_Button m_forwardButton;
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
	Fl_Check_Button m_redMask;
	Fl_Check_Button m_greenMask;
	Fl_Check_Button m_blueMask;
	Fl_Check_Button m_alphaMask;
	Fl_Check_Button m_tile;
	Fl_Input m_tileX;
	Fl_Input m_tileY;
	Fl_Check_Button m_paletteMode;
	Fl_Box m_paletteIndices;
	Fl_Input m_paletteOffset;
	Fl_Button m_loadPalette;
	Fl_Button m_savePalette;
	Fl_Choice m_bitwiseStage1;
	Fl_Input m_bitwiseStage1Bits;
	Fl_Choice m_bitwiseStage2;
	Fl_Input m_bitwiseStage2Bits;
	Fl_Choice m_bitwiseStage3;
	Fl_Input m_bitwiseStage3Bits;
	Fl_Choice m_bitwiseStage4;
	Fl_Input m_bitwiseStage4Bits;
	Fl_Choice m_bitwiseStage5;
	Fl_Input m_bitwiseStage5Bits;
	Fl_Check_Button m_DXTMode;
	Fl_Choice m_DXTType;
	Fl_Check_Button m_RLEMode;
	Fl_Choice m_RLEType;
	Fl_Check_Button m_flipV;
	Fl_Check_Button m_flipH;
	Fl_Check_Button m_colorCount;
	Fl_Button m_aboutButton;
	Fl_Box* m_rightArea;
	Fl_Box* m_imageBox;
	Fl_Scrollbar* m_imageScroll;
	Fl_RGB_Image* m_image;
	
	// Data
	Point2D<int> m_windowSize; // Cached size for resize checks
	u8* m_pixels; // Displayed pixel data in main window
	char* m_text; // Data text full size (to avoid memory reallocs)
	bool m_cursorChanged;
	u32 m_accumOffset;
	bool m_offsetChanged;
	char m_offsetText[32]; // To avoid memory allocs
	char m_currentFile[260];
	u32 m_currentFileSize;
	char m_rawMemoryFlRGBImage[sizeof(Fl_RGB_Image)];
	u8 m_palette[256 * 3];
	u8 m_rawPalette[256 * 4];
	std::vector<BitwiseOp> m_bitwiseOpVec;
	std::set<u32> m_colorSet;
};

#endif

