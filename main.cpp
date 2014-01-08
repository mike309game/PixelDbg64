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

#include "main.h"

//
// Static helper functions
//
namespace
{
	const char* intToString(int i) // Base 10
	{
		static char s_buff[16];
		memset(s_buff, 0, sizeof(s_buff));
		snprintf(s_buff, sizeof(s_buff)-1, "%d", i);
		return s_buff;
	}
	
	template <typename T>
	T clampValue(T i, T l, T h)
	{
		return std::min(std::max(i, l), h);
	}
	
	int randomInt(int _min, int _max)
	{
		return (_min + (rand() % (_max - _min + 1)));
	}

	const char* formatString(const char* text, ...)
	{
		static char buff[1024];
		memset(buff, 0, sizeof(buff));

		va_list arglist;
		va_start(arglist, text);
		vsnprintf(buff, sizeof(buff)-1, text, arglist);
		va_end(arglist);

		return buff;
	}
};


// Entry point:
int main(int argc, char** argv)
{
	#if defined _DEBUG && defined _MSC_VER
	_CrtMemState memState;
	_CrtMemCheckpoint(&memState);
	#endif

	int ret;
	{
		char buff[32];
		memset(buff, 0, sizeof(buff));
		snprintf(buff, sizeof(buff)-1, "PixelDbg %u.%u", PixelDbgWnd::kVersionMajor, PixelDbgWnd::kVersionMinor);

		PixelDbgWnd window(buff);
		window.end();
		window.show(argc, argv);
	
		ret = Fl::run();
	}

	#if defined _DEBUG && defined _MSC_VER
	// FLTK seems to hold something at this point (probably static vars)
	_CrtMemDumpAllObjectsSince(&memState);
	#endif

	return ret;
}

//
// PixelDbgWnd
//
void PixelDbgWnd::draw()
{
	int w = this->w();
	int h = this->h();
	
	if(m_windowSize.x != w || m_windowSize.y != h)
	{
		// Resize happend (adjust dimension fields and scroll control)
		if(isDimValid())
		{
			int imagew = getImageWidth();
			int imageh = getImageHeight();
			int deltaw = w - m_windowSize.x;
			int deltah = h - m_windowSize.y;
			m_windowSize.set(w, h);
			
			imagew = clampValue(imagew + deltaw, 1, (int)kMaxDim);
			m_width.value(intToString(imagew));
			
			imageh = clampValue(imageh + deltah, 1, (int)kMaxDim);
			m_height.value(intToString(imageh));

			updateScrollbar(m_imageScroll->value(), true);

			m_leftArea.size(220, h);

			// Redraw image
			// Note: Passing <this> for widget indicates that we currently resize the window and can't flush!
			UpdateCallback(this, this);
		}
	}
	
	Fl_Double_Window::draw();
}
	
int PixelDbgWnd::handle(int event)
{
	u32 w = (u32)getImageWidth();
	u32 h = (u32)getImageHeight();
	u32 x = (u32)Fl::event_x();
	u32 y = (u32)Fl::event_y();
	u32 r = w + x;
    u32 b = h + y;

	bool insideImage = x >= getImageBox().x() && y >= getImageBox().y() && x <= r && y <= b;
	bool ctrlDown = false;

	if(Fl::event_state() == FL_CTRL)
	{
		// Check if we have a valid image
		if(getImageBox().w() > 0 && getImageBox().h() > 0 && isDimValid())
		{
			if(insideImage)
			{
				ctrlDown = true;

				// Change cursor
				if(!m_cursorChanged)
				{
					cursor(FL_CURSOR_CROSS);
					m_cursorChanged = true;
					
					m_offset.color(FL_YELLOW);
					m_offset.redraw();
				}

				m_imageScroll->deactivate();
			}
		}
	}
	else
	{
		if(m_cursorChanged)
		{
			// Change cursor back to default
			cursor(FL_CURSOR_DEFAULT);
			m_cursorChanged = false;
			
			m_offset.color(FL_WHITE);
			m_offset.redraw();

			m_imageScroll->activate();
			
			// Reload file if desired
			if(m_autoReload.value() != 0)
			{
				ButtonCallback(&m_openButton, this);
			}
		}
	}
	
	if(event == FL_MOVE && ctrlDown && insideImage)
	{
		// Calculate offset on mouse move if CTRL is pressed and we are inside the image box
		x -= getImageBox().x();
		y -= getImageBox().y();
		
		// Take flipping ops into account
		if(m_flipV.value() != 0)
		{
			y = h - y;
		}
		if(m_flipH.value() != 0)
		{
			x = w - x;
		}
		
		// Calculate offset in bytes
		u32 ps = (u32)getPixelSize();
		u32 offset = y * w * ps + x * ps;
		
		// Check for DXT mode (6:1 or 4:1 compression ratio)
		if(isDXTMode())
		{
			int bx = x / 4;
			int by = y / 4;
			int numbx = w / 4;
			int numby = h / 4;
			int block = by * numby + bx;
			
			switch(m_DXTType.value())
			{
			case 0:
				offset = block * 8;
				break;
			case 1:
			case 2:
				offset = block * 16;			
				break;
			}
		}

		memset(m_offsetText, 0, sizeof(m_offsetText));
		snprintf(m_offsetText, sizeof(m_offsetText)-1, "%s", intToString(m_accumOffset + offset));
		m_offset.value(m_offsetText);
	}
	
	return Fl_Double_Window::handle(event);
}

bool PixelDbgWnd::isFormatValid() const
{
	int bitMask[4];
	int rgbaChannels[4];
	int rgbaBits[4];
	int pixelSize;
	
	return getPixelFormat(bitMask, rgbaChannels, rgbaBits, pixelSize);
}

int PixelDbgWnd::getRedBits() const
{
	const char* bits = m_rgbaBits.value();
	const char* r = strchr(bits, '.');
	if(!r)
	{
		return 0;
	}
	
	char num[4] = { 0, 0, 0, 0 };
	strncpy(num, bits, r - bits);
	return atoi(num);
}

int PixelDbgWnd::getGreenBits() const
{
	const char* bits = m_rgbaBits.value();
	const char* g = strchr(bits, '.');		
	if(!g)
	{
		return 0;
	}
	bits = g + 1;
	g = strchr(bits, '.');
	if(!g)
	{
		return 0;
	}
	
	char num[4] = { 0, 0, 0, 0 };
	strncpy(num, bits, g - bits);
	return atoi(num);
}

int PixelDbgWnd::getBlueBits() const
{
	const char* bits = m_rgbaBits.value();
	const char* b = strchr(bits, '.');		
	if(!b)
	{
		return 0;
	}
	bits = b + 1;
	b = strchr(bits, '.');
	if(!b)
	{
		return 0;
	}
	bits = b + 1;
	b = strchr(bits, '.');
	if(!b)
	{
		return 0;
	}
	
	char num[4] = { 0, 0, 0, 0 };
	strncpy(num, bits, b - bits);
	return atoi(num);
}

int PixelDbgWnd::getAlphaBits() const
{
	const char* bits = m_rgbaBits.value();
	const char* a = strchr(bits, '.');		
	if(!a)
	{
		return 0;
	}
	bits = a + 1;
	a = strchr(bits, '.');
	if(!a)
	{
		return 0;
	}
	bits = a + 1;
	a = strchr(bits, '.');
	if(!a)
	{
		return 0;
	}
	
	char num[4] = { 0, 0, 0, 0 };
	strcpy(num, a + 1);
	return atoi(num);
}

bool PixelDbgWnd::getPixelFormat(int bitMask[4], int rgbaChannels[4], int rgbaBits[4], int& pixelSize) const
{
	rgbaBits[0] = getRedBits();
	rgbaBits[1] = getGreenBits();
	rgbaBits[2] = getBlueBits();
	rgbaBits[3] = getAlphaBits();
	
	if(rgbaBits[0] <= 0 && rgbaBits[1] <= 0 && rgbaBits[2] <= 0 && rgbaBits[3] <= 0)
	{
		return false;
	}

	// Do not allow channel depth larger then 8 until proper handling is implemented
	if(rgbaBits[0] > 8 || rgbaBits[1] > 8 || rgbaBits[2] > 8 || rgbaBits[3] > 8)
	{
		return false;
	}
	
	rgbaChannels[0] = atoi(m_redChannel.value()) - 1;
	rgbaChannels[1] = atoi(m_greenChannel.value()) - 1;
	rgbaChannels[2] = atoi(m_blueChannel.value()) - 1;
	rgbaChannels[3] = atoi(m_alphaChannel.value()) - 1;
	
	bitMask[0] = bitMask[1] = bitMask[2] = bitMask[3] = 0;
	
	for(int i=0; i<4; ++i)
	{
		if(rgbaChannels[i] == -1)
		{
			return false;
		}
		
		if(rgbaChannels[i] > 3)
		{
			return false;
		}	
		
		// No duplicate channels allowed (i.e. R=1, G=1)
		for(int j=0; j<4; ++j)
		{
			if(i != j && rgbaChannels[i] == rgbaChannels[j])
			{
				return false;
			}
		}
		
		if(rgbaChannels[i] >= 0)
		{
			bitMask[rgbaChannels[i]] = ~(((u32)-1) << rgbaBits[i]);
		}
	}
	
	int bpp = rgbaBits[0] + rgbaBits[1] + rgbaBits[2] + rgbaBits[3];
	pixelSize = bpp / 8;

	return pixelSize != 0 && (bpp % 8) == 0 && bpp <= 32;
}

bool PixelDbgWnd::getRGBABits(int rgbaBits[4])
{
	int bitMask[4];
	int rgbaChannels[4];
	int pixelSize;

	if(getPixelFormat(bitMask, rgbaChannels, rgbaBits, pixelSize))
	{
		return true;
	}
	
	return false;
}

int PixelDbgWnd::getPixelSize() const
{
	int bitMask[4];
	int rgbaChannels[4];
	int rgbaBits[4];
	int pixelSize;

	if(isPaletteMode())
	{
		return 1;
	}

	if(getPixelFormat(bitMask, rgbaChannels, rgbaBits, pixelSize))
	{
		return pixelSize;
	}

	return 0;
}

bool PixelDbgWnd::updatePixelFormat(bool startup /* false */)
{
	int bitMask[4];
	int rgbaChannels[4];
	int rgbaBits[4];
	int pixelSize;
	
	// Check rgba bits for errors
	const char* bits = m_rgbaBits.value();
	int numDots = 0;
	while(bits[0] != 0)
	{
		if(*bits++ == '.')
		{
			++numDots;
		}
	}

	if(numDots == 3)
	{	
		if(getPixelFormat(bitMask, rgbaChannels, rgbaBits, pixelSize))
		{
			int r = rgbaChannels[0];
			int g = rgbaChannels[1];
			int b = rgbaChannels[2];
			int a = rgbaChannels[3];
			
			const char* fmt = formatString("%d bpp - %.2X %.2X %.2X %.2X", pixelSize * 8, bitMask[r], bitMask[g], bitMask[b], bitMask[a]);
			m_formatInfo.copy_label(fmt);
			m_formatGroup.color(FL_DARK1);
			this->redraw();

			return true;
		}
	}
	
	m_formatInfo.label("Invalid format");
	m_formatGroup.color(FL_RED);
	this->redraw();
	
	return false;
}

void PixelDbgWnd::updateScrollbar(u32 pos, bool resize)
{
	if(resize)
	{
		if(m_currentFileSize > 0)
		{
			u32 pixelSize = (u32)getPixelSize();
			u32 offset = (u32)getOffset();
			u32 totalBytes = m_currentFileSize;
			u32 numVisibleBytes = std::min(getNumVisibleBytes(), totalBytes);

			m_imageScroll->resize(w() - m_imageScroll->w(), 0, m_imageScroll->w(), h());
			m_imageScroll->value(offset, 0, 0, totalBytes - numVisibleBytes);

			m_imageScroll->linesize(numVisibleBytes / 4);
			m_imageScroll->slider_size(float(numVisibleBytes) / float(totalBytes));
		}
		else
		{
			m_imageScroll->resize(w() - m_imageScroll->w(), 1, m_imageScroll->w(), h());
			m_imageScroll->slider_size(1.0f);
		}
	}

	if(pos != (u32)m_imageScroll->value())
	{
		m_imageScroll->value((int)pos);
	}
}

void PixelDbgWnd::convertData(const u8* data, u32 size, u8* rgbOut, u32 flags /* 0 */, u32 tileX /* 0xffff */, u32 tileY /* 0xffff */, u8* palette /* NULL */)
{	
	int bitMask[4];
	int rgbaChannels[4];
	int rgbaBits[4];
	int ps;
	
	if(!isValid() || !getPixelFormat(bitMask, rgbaChannels, rgbaBits, ps))
	{
		return;
	}

	if((flags & CF_IgnoreChannelOrder) != 0)
	{
		int rgbaMask[4] = { bitMask[rgbaChannels[0]], bitMask[rgbaChannels[1]], bitMask[rgbaChannels[2]], bitMask[rgbaChannels[3]] };
		bitMask[0] = rgbaMask[0];
		bitMask[1] = rgbaMask[1];
		bitMask[2] = rgbaMask[2];
		bitMask[3] = rgbaMask[3];

		rgbaChannels[0] = 0;
		rgbaChannels[1] = 1;
		rgbaChannels[2] = 2;
		rgbaChannels[3] = 3;
	}

	// In palette mode each pixel is 1 byte wide indexing a total of 255 colors
	if(palette)
	{
		ps = 1;
	}

	int bitCount[4];
	bitCount[rgbaChannels[0]] = rgbaBits[0];
	bitCount[rgbaChannels[1]] = rgbaBits[1];
	bitCount[rgbaChannels[2]] = rgbaBits[2];
	bitCount[rgbaChannels[3]] = rgbaBits[3];
	
	// Shift pixels up by the difference between 8bit per channel and the interpreted data.
	// This will avoid darker images for lower precision formats (i.e. 5551 -> 8888 = 1.1111 -> 0001.1111).
	int rdiff = 8 - bitCount[rgbaChannels[0]];
	int gdiff = 8 - bitCount[rgbaChannels[1]];
	int bdiff = 8 - bitCount[rgbaChannels[2]];
	int adiff = 8 - bitCount[rgbaChannels[3]];
	
	// Align backwards
	while(size % ps != 0)
	{
		--size;
	}
	
	// Convert
	bool redMasked = (flags & CF_IgnoreRedChannel) != 0;
	bool greenMasked = (flags & CF_IgnoreGreenChannel) != 0;
	bool blueMasked = (flags & CF_IgnoreBlueChannel) != 0;
	bool alphaOnly = redMasked && greenMasked && blueMasked;
	if(alphaOnly)
	{
		rdiff = gdiff = bdiff = adiff;
	}

	u32 width = (u32)getImageWidth();
	u32 height = (u32)getImageHeight();
	u32 xTiles = 1;
	u32 yTiles = 1;

	if((flags & CF_IgnoreTiles) == 0 && tileX < width && tileY < height)
	{
		xTiles = width / tileX;
		yTiles = height / tileY;
	}
	
	u32 totalPixels = size / ps;
	u32 stride = width * ps;
	u32 dest = 0;
	u32 pixels = 0;
	
	for(u32 ty=0; ty<yTiles; ++ty)
	{
		u32 by = ty * tileY;
		
		for(u32 tx=0; tx<xTiles; ++tx)
		{
			u32 bx = tx * tileX;
			
			for(u32 y=0; y<tileY; ++y)
			{
				for(u32 x=0; x<tileX; ++x, dest+=3, ++pixels)
				{
					u32 i = (by + y) * stride + (bx + x) * ps;
					
					if(pixels >= totalPixels)
					{
						return;
					}
					
					// Read pixel byte-wise
					u32 pixel = 0;
					for(u32 j=0; j<ps; ++j)
					{
						pixel |= data[i+j] << (j * 8);
					}
					
					// Final channel values
					u8 r = 0, g = 0, b = 0;

					if(!palette)
					{
						if(alphaOnly)
						{
							if(rgbaChannels[3] >= 0 && rgbaBits[3] != 0)
							{
								int start = 0;
								for(int j=0; j<rgbaChannels[3]; ++j)
								{
									start += bitCount[j];
								}
								r = pixel >> start;
								r &= bitMask[rgbaChannels[3]];
								g = b = r;
							}
						}
						else
						{
							if(!redMasked && rgbaChannels[0] >= 0 && rgbaBits[0] != 0)
							{
								int start = 0;
								for(int j=0; j<rgbaChannels[0]; ++j)
								{
									start += bitCount[j];
								}
								r = pixel >> start;
								r &= bitMask[rgbaChannels[0]];
							}
					
							if(!greenMasked && rgbaChannels[1] >= 0 && rgbaBits[1] != 0)
							{
								int start = 0;
								for(int j=0; j<rgbaChannels[1]; ++j)
								{
									start += bitCount[j];
								}
								g = pixel >> start;
								g &= bitMask[rgbaChannels[1]];
							}
					
							if(!blueMasked && rgbaChannels[2] >= 0 && rgbaBits[2] != 0)
							{
								int start = 0;
								for(int j=0; j<rgbaChannels[2]; ++j)
								{
									start += bitCount[j];
								}
								b = pixel >> start;
								b &= bitMask[rgbaChannels[2]];
							}
						}

						rgbOut[dest+0] = r << rdiff;
						rgbOut[dest+1] = g << gdiff;
						rgbOut[dest+2] = b << bdiff;
					}
					else
					{
						rgbOut[dest+0] = redMasked ? 0 : palette[pixel * 3 + rgbaChannels[0]];
						rgbOut[dest+1] = greenMasked ? 0 : palette[pixel * 3 + rgbaChannels[1]];
						rgbOut[dest+2] = blueMasked ? 0 : palette[pixel * 3 + rgbaChannels[2]];
					}
				}
			}
		}
	}
}

void PixelDbgWnd::convertDXT(const u8* data, u32 size, u8* rgbOut, u32 flags, int DXTType, bool oneBitAlpha /* false */)
{
	int bitMask[4];
	int rgbaChannels[4];
	int rgbaBits[4];
	int ps;
	
	if(!isValid() || !getPixelFormat(bitMask, rgbaChannels, rgbaBits, ps))
	{
		return;
	}

	bool redMasked = (flags & CF_IgnoreRedChannel) != 0;
	bool greenMasked = (flags & CF_IgnoreGreenChannel) != 0;
	bool blueMasked = (flags & CF_IgnoreBlueChannel) != 0;
	bool alphaOnly = redMasked && greenMasked && blueMasked;
	
	u32 width = static_cast<u32>(getImageWidth());
	u32 height = static_cast<u32>(getImageHeight());
	u32 stride = width * 3;
	u32 xTiles = width / 4;
	u32 yTiles = height / 4;
	const u8* end = data + size;
	u8 codes[16];
	
	for(u32 ty=0; ty<=yTiles; ++ty)
	{
		u32 by = ty * 4;
		
		for(u32 tx=0; tx<xTiles; ++tx)
		{
			// We iterate block by block, make sure we don't read more then is given
			if(data + 8 > end)
			{
				return;
			}
			
			u32 bx = tx * 4;
			
			// Skip alpha in DXT3/DXT5
			if(DXTType > 1)
			{
				data += 8;
			}
			
			u32 colors = *((u32*)data);
			u32 clrlut = *((u32*)data + 1);
			data += 8;
			
			// Extract two 16bit colors
			u16 rgb0 = colors & 0x0000ffff;
			u16 rgb1 = colors >> 16;
			
			// Decode colors to 888 format
			int lut[4 * 5];
			lut[0]  = ((rgb0 >> 11) << 3);
			lut[1]  = oneBitAlpha ? ((rgb0 >> 6) << 3) & 0xff : ((rgb0 >> 5) << 2) & 0xff;
			lut[2]  = oneBitAlpha ? ((rgb0 >> 1) << 3) & 0xff : ((rgb0 << 3) & 0xff);
			lut[3]  = ((rgb1 >> 11) << 3);
			lut[4]  = oneBitAlpha ? ((rgb1 >> 6) << 3) & 0xff : ((rgb1 >> 5) << 2) & 0xff;
			lut[5]  = oneBitAlpha ? ((rgb1 >> 1) << 3) & 0xff : ((rgb1 << 3) & 0xff);
			lut[6]  = (lut[0] * 2 + lut[3]) / 3;
			lut[7]  = (lut[1] * 2 + lut[4]) / 3;
			lut[8]  = (lut[2] * 2 + lut[5]) / 3;
			lut[9]  = (lut[0] + lut[3] * 2) / 3;
			lut[10] = (lut[1] + lut[4] * 2) / 3;
			lut[11] = (lut[2] + lut[5] * 2) / 3;
			lut[12] = (lut[0] + lut[3]) / 2;
			lut[13] = (lut[1] + lut[4]) / 2;
			lut[14] = (lut[2] + lut[5]) / 2;
			lut[15] = 0;
			lut[16] = 0;
			lut[17] = 0;
			
			// Read color codes
			for(u32 c=0; c<16; ++c)
			{
				codes[c] = (clrlut >> c * 2) & 3;
			}
			
			// Decode 16 pixels
			for(u32 y=0; y<4; ++y)
			{
				for(u32 x=0; x<4; ++x)
				{
					u32 code = codes[y * 4 + x];
					u32 i = (by + y) * stride + (bx + x) * 3;
					
					switch(code)
					{
					case 0:
					case 1:
						code *= 3;
						break;
					case 2:
						code = (DXTType == 1 && rgb0 < rgb1) ? 12 : 6;
						break;
					case 3:
						code = (DXTType == 1 && rgb0 < rgb1) ? 15 : 9;
						break;
					};
					
					rgbOut[i+0] = lut[code + rgbaChannels[2]];
					rgbOut[i+1] = lut[code + rgbaChannels[1]];
					rgbOut[i+2] = lut[code + rgbaChannels[0]];
				}
			}
		}
	}
}

void PixelDbgWnd::convertPalette(const u8* data, u32 size, u8* rgbOut)
{
	int bitMask[4];
	int rgbaChannels[4];
	int rgbaBits[4];
	int pixelSize;

	if(getPixelFormat(bitMask, rgbaChannels, rgbaBits, pixelSize))
	{
		memset(m_palette, 0, sizeof(m_palette));
		convertData(data, (u32)(256 * pixelSize), rgbOut, CF_IgnoreChannelOrder | CF_IgnoreTiles);
	}
}

void PixelDbgWnd::flipVertically(int w, int h, void* data)
{
	if(w > 0 && h > 0 && data)
	{
		int stride = w * 3; // Always 24bpp

		for(int y=0; y<h/2; ++y)
		{
			u8* src = ((u8*)data) + y * stride;
			u8* dest = ((u8*)data) + (h - y - 1) * stride;
			
			for(int x=0; x<w; ++x, src+=3, dest+=3)
			{
				std::swap(src[0], dest[0]);
				std::swap(src[1], dest[1]);
				std::swap(src[2], dest[2]);
			}
		}
	}
}

u32 PixelDbgWnd::readFile(const char* name, void* out, u32 size, u32 offset /* 0 */)
{
	FILE* f = fopen(name, "rb");
	if(f)
	{
		fseek(f, 0, SEEK_END);
		u32 filesize = (u32)ftell(f);
		fseek(f, offset, SEEK_SET);

		size = std::min(size + offset, filesize) - offset;
		u32 count = (u32)fread(out, size, 1, f);
		fclose(f);

		return size;
	}

	return 0;
}

bool PixelDbgWnd::writeBitmap(const char* filename, int width, int height, void* data)
{
	if(!data || width <= 0 || height <= 0 || !isValid())
	{
		return false;
	}
	
	FILE* f = fopen(filename, "wb");
	if(f)
	{
		u32 w = (u32)width;
		u32 h = (u32)height;
		u32 size = w * h * 3;
		u8* pixels = (u8*)data;

		flipVertically(width, height, pixels);

		// Swap before saving (we have Fl_RGB_Image but need BGR order)
		for(u32 i=0; i<size; i+=3)
		{
			std::swap(pixels[i+0], pixels[i+2]);
		}
		
		unsigned char header[54];
		memset(header, 0, sizeof(header));

		*reinterpret_cast<u16*>(&header[0]) = 0x4D42;
		*reinterpret_cast<u32*>(&header[2]) = size + 54;
		*reinterpret_cast<u32*>(&header[10]) = 54;
		*reinterpret_cast<u32*>(&header[14]) = 40;
		*reinterpret_cast<u32*>(&header[18]) = w;
		*reinterpret_cast<u32*>(&header[22]) = h;
		*reinterpret_cast<u16*>(&header[26]) = 1;
		*reinterpret_cast<u16*>(&header[28]) = 24;
		*reinterpret_cast<u32*>(&header[34]) = size;
		
		fwrite(header, sizeof(header), 1, f);
		fwrite(pixels, size, 1, f);
		fclose(f);

		// Swap back
		for(u32 i=0; i<size; i+=3)
		{
			std::swap(pixels[i+0], pixels[i+2]);
		}

		flipVertically(width, height, data);
		
		return true;
	}
	
	return false;
}

bool PixelDbgWnd::writeTga(const char* filename)
{
	if(m_data.size() <= 0 || !isValid())
	{
		return false;
	}
	
	FILE* f = fopen(filename, "wb");
	if(f)
	{
		u16 w = static_cast<u16>(getImageWidth());
		u16 h = static_cast<u16>(getImageHeight());
		u8 bd = 24;
		TgaHeader header = { 0, 0, 2, 0, 0, 0, 0, 0, w, h, bd, 32 };
		
		// Swap before saving (we have Fl_RGB_Image but need BGR order)
		u32 size = w * h * 3;
		for(u32 i=0; i<size; i+=3)
		{
			std::swap(m_pixels[i+0], m_pixels[i+2]);
		}

		fwrite(&header, sizeof(header), 1, f);
		fwrite(m_pixels, size, 1, f);
		fclose(f);
		
		// Swap back
		for(u32 i=0; i<size; i+=3)
		{
			std::swap(m_pixels[i+0], m_pixels[i+2]);
		}
		
		return true;
	}
	
	return false;
}


// static:
void PixelDbgWnd::ButtonCallback(Fl_Widget* widget, void* param)
{
	if(!param)
	{
		return;
	}
	
	PixelDbgWnd* p = static_cast<PixelDbgWnd*>(param);
	
	if(widget == &p->m_saveButton && p->isDimValid())
	{
		int w = p->getImageWidth();
		int h = p->getImageHeight();
		int o = p->getOffset();
		const char* name = p->getCurrentFileName();
		const char* filename = formatString("%s_%dx%d_%d.bmp", name ? name : "", w, h, o);
		
		if(!p->writeBitmap(filename, w, h, p->m_pixels))
		{
			fl_message("Saving failed. Either format is invalid or no data exists."); 
		}
	}
	else if(widget == &p->m_openButton)
	{
		u32 offset = (u32)std::max(atoi(p->m_offset.value()), 0);
		Fl_Native_File_Chooser browser;
		const char* filename = p->m_currentFile;
		
		// Show file dialog only if we are not in auto-reload mode
		if(p->m_autoReload.value() == 0)
		{
			const char* title = formatString("Open file from offset: %d Bytes", offset);

			browser.title(title);
			browser.type(Fl_Native_File_Chooser::BROWSE_FILE);
			browser.filter("Any file\t*.*\n");
			
			if(browser.show() == 0)
			{
				filename = browser.filename();
			}
			else
			{
				return;
			}
		}
		
		if(filename && filename[0] != 0)
		{
			FILE* f = fopen(filename, "rb");
			if(f)
			{
			    fseek(f, 0, SEEK_END);
			    p->m_currentFileSize = (u32)ftell(f);
			    
			    if(offset >= p->m_currentFileSize)
			    {
					// If auto-reload is on do nothing when pointing out of bounds
					if(p->m_autoReload.value() != 0)
					{
						fclose(f);
						return;
					}
					
					fl_message("Offset %d larger then file size (%d Bytes). Reading from offset 0 instead.", offset, p->m_currentFileSize);
					offset = 0;
					p->m_accumOffset = 0;
			    }
			    
			    // Reset accumulated offset if we opened a different file or changed it manually
			    if(strcmp(filename, p->m_currentFile) != 0)
			    {
			        p->m_accumOffset = 0;
			    }
			    else
			    {
			        if(p->m_offsetChanged)
			        {
			            p->m_accumOffset = 0;
			            p->m_offsetChanged = false;
			        }
			    }
			    
			    u32 size = std::min(p->m_currentFileSize - offset, kMaxImageSize);
			    p->m_accumOffset = offset;
			    
			    memset(p->m_text, 0, kMaxBufferSize);
			    fseek(f, offset, SEEK_SET);
			    fread(p->m_text, size, 1, f);
			    fclose(f);
			    
			    // Store current file for the accumulated offset
			    if(strcmp(filename, p->m_currentFile) != 0)
			    {
			        memset(p->m_currentFile, 0, sizeof(p->m_currentFile));
			        snprintf(p->m_currentFile, sizeof(p->m_currentFile)-1, "%s", filename);
			    }
			    
			    // Set data and update image view
			    p->m_data.static_value(reinterpret_cast<char*>(p->m_text), size);
			    UpdateCallback(&p->m_data, param);
			    
			    // Move data cursor to data start
			    p->m_data.position(0, 0);
				
				// Assign new accumulation offset
				p->m_offset.value(intToString(p->m_accumOffset));

				// Update scroll bar on valid file
				p->m_imageScroll->activate();
				p->updateScrollbar(p->m_accumOffset, true);
				
				// Adjust window title
				const char* title = formatString("PixelDbg %u.%u  -  %s (%u Bytes)", 
					PixelDbgWnd::kVersionMajor, PixelDbgWnd::kVersionMinor, p->getCurrentFileName(), p->m_currentFileSize);
				p->copy_label(title);
			}
		}
	}
	else if(widget == &p->m_aboutButton)
	{
		#if defined __GNUC__
		#if defined _WIN32
		const char* about = formatString("PixelDbg %u.%u\n\nNikita Kindt (n.kindt.pdbg<at>gmail.com)\nCompiled on %s with g++ %d.%d.%d (mingw/tdm-gcc)", 
				 kVersionMajor, kVersionMinor, __DATE__, __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
		#else
		const char* about = formatString("PixelDbg %u.%u\n\nNikita Kindt (n.kindt.pdbg<at>gmail.com)\nCompiled on %s with g++ %d.%d.%d", 
				 kVersionMajor, kVersionMinor, __DATE__, __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
		#endif
		#elif defined _MSC_VER
		const char* about = formatString("PixelDbg %u.%u\n\nNikita Kindt (n.kindt.pdbg<at>gmail.com)\nCompiled on %s with MSVC %d", 
				 kVersionMajor, kVersionMinor, __DATE__, _MSC_VER);
		#else
		#error Platform not supported!
		#endif

		fl_message(about);
	}
}

void PixelDbgWnd::OffsetCallback(Fl_Widget* widget, void* param)
{
	if(!param)
	{
		return;
	}
	
	PixelDbgWnd* p = static_cast<PixelDbgWnd*>(param);
	p->m_offsetChanged = true;
	
	if(widget == &p->m_autoReload)
	{
		if(p->m_autoReload.value() != 0)
		{
			p->m_openButton.deactivate();
		}
		else
		{
			p->m_openButton.activate();
		}
	}
	else if(widget == &p->m_offset)
	{
		ButtonCallback(&p->m_openButton, p);
	}
}

void PixelDbgWnd::ChannelCallback(Fl_Widget* widget, void* param)
{
	if(!param)
	{
		return;
	}
	
	PixelDbgWnd* p = static_cast<PixelDbgWnd*>(param);
	
	// Make sure we have a valid channel range
	if(widget == &p->m_redChannel || widget == &p->m_greenChannel ||
	   widget == &p->m_blueChannel || widget == &p->m_alphaChannel)
	{
		const char* value = ((Fl_Input*)widget)->value();
		if(value && value[0] != 0)
		{
			int i = atoi(value);
			if(i < 1 || i > 4)
			{
				i = clampValue(i, 1, 4);
				((Fl_Input*)widget)->value(intToString(i));
			}
		}
	}

	// Make sure that at least one channel is always enabled
	if(widget == &p->m_redMask || widget == &p->m_greenMask || widget == &p->m_blueMask)
	{
		bool r = p->m_redMask.value() != 0;
		bool g = p->m_greenMask.value() != 0;
		bool b = p->m_blueMask.value() != 0;
		bool a = p->m_alphaMask.value() != 0;

		if(!r && !g && !b && !a)
		{
			if(!p->m_alphaMask.active())
			{
				// Keep single channel checked
				static_cast<Fl_Check_Button*>(widget)->value(1);
			}
			else
			{
				// Activate alpha only if RGB disabled
				p->m_alphaMask.value(1);
			}
		}
	}
	else if(widget == &p->m_alphaMask)
	{
		bool r = p->m_redMask.value() != 0;
		bool g = p->m_greenMask.value() != 0;
		bool b = p->m_blueMask.value() != 0;
		bool a = p->m_alphaMask.value() != 0;

		if(!r && !g && !b && !a)
		{
			p->m_redMask.value(1);
			p->m_greenMask.value(1);
			p->m_blueMask.value(1);
		}
	}

	if(p->updatePixelFormat() && p->isValid())
	{
		// If in palette mode make sure to convert
		if(p->isPaletteMode())
		{
			int bitMask[4];
			int rgbaChannels[4];
			int rgbaBits[4];
			int pixelSize;
			
			p->convertPalette(p->m_rawPalette, sizeof(p->m_rawPalette), p->m_palette);
		}

		// Depending on the pixelformat conversion ratio the knob size can change
		if(widget == &p->m_rgbaBits)
		{
			p->updateScrollbar(p->m_imageScroll->value(), true);
		}

		UpdateCallback(widget, param);
	}
}

void PixelDbgWnd::DimCallback(Fl_Widget* widget, void* param)
{
	if(!param)
	{
		return;
	}
	
	PixelDbgWnd* p = static_cast<PixelDbgWnd*>(param);

	if(widget == &p->m_backwardButton || widget == &p->m_forwardButton)
	{
		u32 offset = p->m_accumOffset;

		if(widget == &p->m_backwardButton && offset > 0)
		{
			--offset;
		}
		else if(widget == &p->m_forwardButton && offset < p->m_currentFileSize)
		{
			++offset;
		}

		if(offset != p->m_accumOffset)
		{
			p->m_imageScroll->value((int)offset);

			memset(p->m_pixels, 0, kMaxImageSize);

 			if(u32 size = p->readFile(p->m_currentFile, p->m_text, kMaxBufferSize, offset))
			{
				p->m_accumOffset = offset;
				p->m_offset.value(intToString(offset));
				p->m_data.static_value(reinterpret_cast<char*>(p->m_text), size);
				p->m_data.position(0, 0);

				ScrollbarCallback(p->m_imageScroll, p);	

				UpdateCallback(widget, param);
			}
		}
	}
	else if(widget == &p->m_width || widget == &p->m_height)
	{
		int w = p->getImageWidth();
		if(w != -1)
		{
			int maxw = p->w() - p->getImageBox().x() - p->m_imageScroll->w() - 6;
			if(w < 1 || w > maxw)
			{
				w = clampValue(w, 1, maxw);
				p->m_width.value(intToString(w));
			}
		}
	
		int h = p->getImageHeight();
		if(h != -1)
		{
			int maxh = p->h() - p->getImageBox().y();
			if(h < 1 || h > maxh)
			{
				h = clampValue(h, 1, maxh);
				p->m_height.value(intToString(h));
			}
		}

		if(w != -1 && h != -1)
		{
			p->m_dimGroup.color(FL_DARK1);
			p->redraw();

			if(p->updatePixelFormat())
			{
				UpdateCallback(widget, param);
			}
		}
		else
		{
			p->m_dimGroup.color(FL_RED);
			p->redraw();
		}
	}
}

void PixelDbgWnd::TileCallback(Fl_Widget* widget, void* param)
{
	if(!param)
	{
		return;
	}
	PixelDbgWnd* p = static_cast<PixelDbgWnd*>(param);

	if(widget == &p->m_tile)
	{
		if(p->m_tile.value() == 0)
		{
			p->m_tileX.deactivate();
			p->m_tileY.deactivate();
		}
		else
		{
			p->m_tileX.activate();
			p->m_tileY.activate();
		}
		
		UpdateCallback(widget, param);
	}
	else if(widget == &p->m_tileX && p->isDimValid())
	{
		int w = p->getImageWidth();
		int t = atoi(p->m_tileX.value());

		if(t < 1 || t > w)
		{
			t = clampValue(t, 1, w);
			p->m_tileX.value(intToString(t));
		}
		
		UpdateCallback(widget, param);
	}
	else if(widget == &p->m_tileY && p->isDimValid())
	{
		int h = p->getImageHeight();
		int t = atoi(p->m_tileY.value());

		if(t < 1 || t > h)
		{
			t = clampValue(t, 1, h);
			p->m_tileY.value(intToString(t));
		}
		
		UpdateCallback(widget, param);
	}
}

void PixelDbgWnd::PaletteCallback(Fl_Widget* widget, void* param)
{
	if(!param)
	{
		return;
	}
	PixelDbgWnd* p = static_cast<PixelDbgWnd*>(param);
	
	if(widget == &p->m_paletteMode)
	{
		if(p->m_paletteMode.value() == 0)
		{
			p->m_alphaMask.activate();
			p->m_paletteIndices.deactivate();
			p->m_paletteOffset.deactivate();
			p->m_loadPalette.deactivate();
			p->m_savePalette.deactivate();

			p->m_paletteIndices.label("Used: 0-0");
		}
		else
		{
			p->m_alphaMask.deactivate();
			p->m_paletteIndices.activate();
			p->m_paletteOffset.activate();
			p->m_loadPalette.activate();
			p->m_savePalette.activate();
		}
		
		p->updateScrollbar(p->m_imageScroll->value(), true);
		UpdateCallback(widget, param);
	}
	else if(widget == &p->m_loadPalette || widget == &p->m_paletteOffset)
	{
		u32 offset = (u32)atoi(p->m_paletteOffset.value());
		const char* filename;
		Fl_Native_File_Chooser browser;

		// Load from file if offset 0 otherwise read palette from current file
		if(offset != 0)
		{
			filename = (const char*)p->m_currentFile;
		}
		else
		{
			browser.title("Load palette from file (first 768 bytes)");
			browser.type(Fl_Native_File_Chooser::BROWSE_FILE);
			browser.filter("Any file\t*.*\n");
		
			if(browser.show() == 1)
			{
				return;
			}
			
			filename = browser.filename();

			// Wipe old palette
			memset(p->m_palette, 0, sizeof(p->m_palette));
				
			// Check for known image files
			const char* dot = strrchr(filename, '.');
			if(dot)
			{
				u32 len = strlen(dot);
				if(len < 5)
				{
					char ext[8];
					strcpy(ext, dot);
					for(u32 j=1; j<len; ++j)
					{
						ext[j] = tolower(ext[j]);
					}
					
					if(strstr(ext, ".bmp") == ext)
					{
						offset = 54;
					}
					else if(strstr(ext, ".tga") == ext)
					{
						offset = 12;
					}
				}
			}
		}

		// Put caret at offset end
		p->m_paletteOffset.position(25, 25);

		// Read palette from given offset and convert to specified format
		memset(p->m_rawPalette, 0, sizeof(p->m_rawPalette));
		if(p->readFile(filename, p->m_rawPalette, sizeof(p->m_rawPalette), offset) != 0)
		{
			p->convertPalette(p->m_rawPalette, sizeof(p->m_rawPalette), p->m_palette);

			UpdateCallback(widget, param);
		}
	}
	else if(widget == &p->m_savePalette)
	{
		const char* name = p->getCurrentFileName();
		int offset = p->getPaletteOffset();

		const char* filename = formatString("%s_palette_%d.bmp", name, offset);
		p->writeBitmap(filename, 32, 8, p->m_palette);
	}
}

void PixelDbgWnd::DXTCallback(Fl_Widget* widget, void* param)
{
	if(!param)
	{
		return;
	}
	PixelDbgWnd* p = static_cast<PixelDbgWnd*>(param);
	
	if(widget == &p->m_DXTMode)
	{
		if(!p->isDXTMode())
		{
			p->m_alphaMask.activate();
			p->m_tile.activate();
			p->m_tileX.activate();
			p->m_tileY.activate();
			p->m_DXTType.deactivate();
			p->m_paletteMode.activate();

			if(p->isPaletteMode())
			{
				p->m_paletteIndices.activate();
				p->m_paletteOffset.activate();
				p->m_loadPalette.activate();
				p->m_savePalette.activate();
			}
		}
		else
		{
			p->m_alphaMask.deactivate();
			p->m_tile.deactivate();
			p->m_tileX.deactivate();
			p->m_tileY.deactivate();
			p->m_DXTType.activate();
			p->m_paletteMode.deactivate();
			p->m_paletteIndices.deactivate();
			p->m_paletteOffset.deactivate();
			p->m_loadPalette.deactivate();
			p->m_savePalette.deactivate();

			// Set appropriate pixel format for DXT
			p->m_rgbaBits.value("5.6.5.0");
			p->updatePixelFormat();
		}
		
		p->updateScrollbar(p->m_imageScroll->value(), true);

		UpdateCallback(widget, param);
	}
	else if(widget == &p->m_DXTType)
	{
		// Set appropriate pixel format for DXT
		p->m_rgbaBits.value("5.6.5.0");
		p->updatePixelFormat();
		p->updateScrollbar(p->m_imageScroll->value(), true);
		
		UpdateCallback(widget, param);
	}
}

void PixelDbgWnd::OpsCallback(Fl_Widget* widget, void* param)
{
	if(!param)
	{
		return;
	}
	PixelDbgWnd* p = static_cast<PixelDbgWnd*>(param);
	
	if(widget == &p->m_colorCount)
	{
		if(p->m_colorCount.value() == 0)
		{
			// Set back original label
			p->m_colorCount.label("Count colors");
		}
		else
		{
			UpdateCallback(widget, param);
		}
	}
}

void PixelDbgWnd::ScrollbarCallback(Fl_Widget* widget, void* param)
{
	PixelDbgWnd* p = static_cast<PixelDbgWnd*>(param);
	
	if(!p || !p->isValid())
	{
		return;
	}

	if(widget == p->m_imageScroll)
	{
		u32 pos = (u32)p->m_imageScroll->value();
		u32 offset = (u32)p->getOffset();
		u32 numVisibleBytes = p->getNumVisibleBytes();

		if(numVisibleBytes > p->m_currentFileSize || pos == p->m_currentFileSize || pos == offset)
		{
			return;
		}

		memset(p->m_pixels, 0, kMaxImageSize);

 		if(u32 size = p->readFile(p->m_currentFile, p->m_text, kMaxBufferSize, pos))
		{
			p->m_accumOffset = pos;
			p->m_offset.value(intToString(pos));
			p->m_data.static_value(reinterpret_cast<char*>(p->m_text), size);
			p->m_data.position(0, 0);

			p->updateScrollbar(pos, true);

			UpdateCallback(widget, param);
		}
	}
}

void PixelDbgWnd::UpdateCallback(Fl_Widget* widget, void* param) // Callback to update image
{
	PixelDbgWnd* p = static_cast<PixelDbgWnd*>(param);

	if(!p || !p->isValid())
	{
		return;
	}
	
	const u8* text = reinterpret_cast<const u8*>(p->m_data.value());
	u32 length = (u32)p->m_data.size();
	int w = p->getImageWidth();
	int h = p->getImageHeight();
	int ps = p->getPixelSize();
	
	// Handle memory tiling
	int tx = w;
	int ty = h;
	if(p->m_tile.value() != 0)
	{
		tx = atoi(p->m_tileX.value());
		ty = atoi(p->m_tileY.value());
	}
	
	// Print byte count
	u32 maxVisible = std::min(p->m_currentFileSize - p->m_accumOffset, p->getNumVisibleBytes());
	const char* byteCount = formatString("Visible: %.2f %%", float(maxVisible) / float(p->getNumVisibleBytes()) * 100.0f);
	p->m_byteCount.copy_label(byteCount);

	if(!text || length == 0 || ps <= 0)
	{
		return;
	}
	
	// Wipe old data
	u32 size = u32(w) * u32(h) * 3;
	u32 stride = u32(w) * 3;
	memset(p->m_pixels, 0, size);
	
	// Convert data
	u32 flags = p->getRGBAIgnoreMask();
	if(p->isDXTMode())
	{
		int rgbaBits[4];
		if(p->getRGBABits(rgbaBits))
		{
			int type = 1;
			switch(p->m_DXTType.value())
			{
			case 0: type = 1; break;
			case 1: type = 3; break;
			case 2: type = 5; break;
			}
			
			bool oneBitAlpha = rgbaBits[3] == 1;
			p->convertDXT(text, length, p->m_pixels, flags, type, oneBitAlpha);
		}
	}
	else
	{
		length = std::min(kMaxBufferSize, length);
		p->convertData(text, length, p->m_pixels, flags, u32(tx), u32(ty), p->isPaletteMode() ? p->m_palette : NULL);
	}
	
	//
	// See if we have other ops to perform on the data (we could combine some of them for performance but maybe later):
	//

	// Vertical flip ?
	if(p->m_flipV.value() != 0)
	{
		for(u32 y=0; y<u32(h/2); ++y)
		{
			u8* src = p->m_pixels + y * stride;
			u8* dest = p->m_pixels + (u32(h) - y - 1) * stride;
			
			for(u32 x=0; x<u32(w); ++x, src+=3, dest+=3)
			{
				std::swap(src[0], dest[0]);
				std::swap(src[1], dest[1]);
				std::swap(src[2], dest[2]);
			}
		}
	}
	
	// Horizontal flip ?
	if(p->m_flipH.value() != 0)
	{
		for(u32 y=0; y<u32(h); ++y)
		{
			u8* line = p->m_pixels + y * stride;
			
			for(u32 x=0; x<u32(w/2); ++x)
			{
				u32 rx = u32(w - 1) - x;
				
				std::swap(line[x*3+0], line[rx*3+0]);
				std::swap(line[x*3+1], line[rx*3+1]);
				std::swap(line[x*3+2], line[rx*3+2]);
			}
		}
	}
	
	// Count colors ?
	if(p->m_colorCount.value() != 0)
	{
		for(u32 y=0; y<u32(h); ++y)
		{
			u8* line = p->m_pixels + y * stride;
			
			for(u32 x=0; x<u32(w); ++x, line+=3)
			{
				u32 color = line[0] | (line[1] << 8) | (line[2] << 16);
				p->m_colorSet.insert(color);
			}
		}
	
		const char* colorCount = formatString("Colors: %u", p->m_colorSet.size());
		p->m_colorCount.copy_label(colorCount);
		p->m_colorSet.clear();
	}

	// Recalculate used min/max indices in palette mode
	if(p->isPaletteMode())
	{
		u8 inmin = 255, inmax = 0;
		int w = p->getImageWidth();
		int h = p->getImageHeight();
		const u8* data = reinterpret_cast<const u8*>(p->m_data.value());

		for(u32 i=0; i<(u32)p->m_data.size(); ++i)
		{
			u8 index = data[i];
			if(index < inmin) inmin = index;
			else if(index > inmax) inmax = index;
		}

		p->m_paletteIndices.copy_label(formatString("Used: %u-%u", inmin, inmax));
	}
	
	// Show new image
	if(p->m_image)
	{
		p->m_image->Fl_RGB_Image::~Fl_RGB_Image();
	}
	p->m_image = new (p->m_rawMemoryFlRGBImage) Fl_RGB_Image(p->m_pixels, w, h, 3);
	p->getImageBox().image(p->m_image);
	
	int newWidth = std::min(p->w() - p->m_imageScroll->w() - 5, p->w() - p->m_imageScroll->w() - 5);
	p->getImageBox().resize(p->m_leftArea.w(), 2, p->w() - 15 - p->m_leftArea.w(), p->h() - 2);

	if(widget != p)
	{
		// Flush main window but only if we are not resizing.
		// With flush the image will be visible but vanish when we resize.
		// Without flush the image won't be visible until we resize.
		// Seems to be a problem with FLTK so this workaround is needed.
		p->flush();
	}
}

