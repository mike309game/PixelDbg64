/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2013 Nikita Kindt (n.kindt.pdbg@gmail.com)    		   *
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
// Entry point:
//
int main(int argc, char** argv)
{
	char buff[32];
	snprintf(buff, sizeof(buff), "PixelDbg %.2f", MyWindow::kVersion);

	MyWindow window(buff);
	window.end();
	window.show(argc, argv);
	
	return Fl::run();
}

//
// MyWindow
//
void MyWindow::draw()
{
	int w = this->w();
	int h = this->h();
	
	if(m_windowSize.x != w || m_windowSize.y != h)
	{
		// Resize happend (adjust dimension fields)
		int imagew = getImageWidth();
		int imageh = getImageHeight();
		int deltaw = w - m_windowSize.x;
		int deltah = h - m_windowSize.y;
		m_windowSize.set(w, h);
		
		char num[16];
		imagew = std::min(std::max(imagew + deltaw, 1), (int)kMaxDim);
		m_width.value(itoa(imagew, num, 10));
		imageh = std::min(std::max(imageh + deltah, 1), (int)kMaxDim);
		m_height.value(itoa(imageh, num, 10));
        
		// Redraw image
		// Note: Passing <this> for widget indicates that we currently resize the window and can't flush!
		UpdateCallback(this, this);
	}
	
	Fl_Double_Window::draw();
}
	
int MyWindow::handle(int event)
{
    bool ctrlDown = false;
	bool insideImage = false;
	u32 w, h, x, y, r, b;
	
	if(Fl::event_state() == FL_CTRL)
	{
        // Check if we have a valid image
		if(m_imageBox.w() > 0 && m_imageBox.h() > 0)
		{
			w = static_cast<u32>(getImageWidth());
			h = static_cast<u32>(getImageHeight());
			x = static_cast<u32>(Fl::event_x());
			y = static_cast<u32>(Fl::event_y());
			r = w + x;
			b = h + y;
			
			if(x >= m_imageBox.x() && y >= m_imageBox.y() && x <= r && y <= b)
			{
                ctrlDown = true;
				insideImage = true;
            
				// Change cursor
				if(!m_cursorChanged)
				{
					cursor(FL_CURSOR_CROSS);
					m_cursorChanged = true;
                    
                    m_offset.color(FL_YELLOW);
                    m_offset.redraw();
				}
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
            
            // Reload file if desired (argument == 255 won't open file dialog window)
            if(m_autoReload.value() != 0)
            {
                ButtonCallback(&m_openButton, this);
            }
		}
	}
	
	if(event == FL_MOVE && ctrlDown && insideImage)
	{
		// Calculate offset on mouse move if CTRL is pressed and we are inside the image box
		x -= m_imageBox.x();
		y -= m_imageBox.y();
		
		u32 s = static_cast<u32>(getPixelSize());
		u32 offset = y * w * s + x * s;
		
		memset(m_offsetText, 0, sizeof(m_offsetText));
		itoa(m_accumOffset + offset, m_offsetText, 10);
		m_offset.value(m_offsetText);
	}
	
	return Fl_Double_Window::handle(event);
}

int MyWindow::getRedBits() const
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

int MyWindow::getGreenBits() const
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

int MyWindow::getBlueBits() const
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

int MyWindow::getAlphaBits() const
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

bool MyWindow::getPixelFormat(int bitMask[4], int rgbaChannels[4], int rgbaBits[4], int& pixelSize) const
{
	rgbaBits[0] = getRedBits();
	rgbaBits[1] = getGreenBits();
	rgbaBits[2] = getBlueBits();
	rgbaBits[3] = getAlphaBits();
	
	if(rgbaBits[0] <= 0 && rgbaBits[1] <= 0 && rgbaBits[2] <= 0 && rgbaBits[3] <= 0)
	{
		return false;
	}
	
	rgbaChannels[0] = atoi(m_redChannel.value()) - 1;
	rgbaChannels[1] = atoi(m_greenChannel.value()) - 1;
	rgbaChannels[2] = atoi(m_blueChannel.value()) - 1;
	rgbaChannels[3] = atoi(m_alphaChannel.value()) - 1;
	
	for(int i=0; i<4; ++i)
	{
		if(rgbaChannels[i] < 0 && rgbaBits[i] != 0)
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
	}
	
	bitMask[rgbaChannels[0]] = ~(((u32)-1) << rgbaBits[0]);
	bitMask[rgbaChannels[1]] = ~(((u32)-1) << rgbaBits[1]);
	bitMask[rgbaChannels[2]] = ~(((u32)-1) << rgbaBits[2]);
	bitMask[rgbaChannels[3]] = ~(((u32)-1) << rgbaBits[3]);
	
	int bpp = rgbaBits[0] + rgbaBits[1] + rgbaBits[2] + rgbaBits[3];
	pixelSize = bpp / 8;

	return pixelSize != 0 && (bpp % 8) == 0 && bpp <= 32;
}

int MyWindow::getPixelSize() const
{
	int bitMask[4];
	int rgbaChannels[4];
	int rgbaBits[4];
	int pixelSize;

	if(getPixelFormat(bitMask, rgbaChannels, rgbaBits, pixelSize))
	{
		return pixelSize;
	}
	
	return 0;
}

bool MyWindow::updatePixelFormat(bool startup /* false */)
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
			
			char buff[64];
			snprintf(buff, sizeof(buff), "%d bpp - %.2X %.2X %.2X %.2X",
				pixelSize * 8, bitMask[r], bitMask[g], bitMask[b], bitMask[a]);
						
			m_formatInfo.copy_label(buff);
			m_formatGroup.color(FL_DARK1);
			if(!startup)
            {
                flush();
            }

			return true;
		}
	}
	
	m_formatGroup.color(FL_RED);
	m_formatInfo.label("Invalid format");
    m_formatInfo.redraw();
    if(!startup)
    {
        flush();
    }
	
	return false;
}

void MyWindow::convertData(const u8* data, u8* rgbOut, u32 size, u32 tileX, u32 tileY)
{
	int bitMask[4];
	int rgbaChannels[4];
	int rgbaBits[4];
	int ps;
	
	if(!getPixelFormat(bitMask, rgbaChannels, rgbaBits, ps))
	{
		return;
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

	// Align backwards
	while(size % ps != 0)
	{
		--size;
	}

	// Convert
	u32 width = static_cast<u32>(getImageWidth());
	u32 height = static_cast<u32>(getImageHeight());
	u32 xTiles = width / tileX;
	u32 yTiles = height / tileY;
	u32 totalPixels = width * height;
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
					if(i >= size)
					{
						break;
					}
					
					// Read pixel byte-wise
					u32 pixel = 0;
					for(u32 j=0; j<ps; ++j)
					{
						pixel |= data[i+j] << (j * 8);
					}
					
					// Final channel values
					u8 r = 0;
					u8 g = 0;
					u8 b = 0;
					
					if(rgbaChannels[0] >= 0 && rgbaBits[0] != 0)
					{
						int start = 0;
						for(int j=0; j<rgbaChannels[0]; ++j)
						{
							start += bitCount[j];
						}
						r = pixel >> start;
						r &= bitMask[rgbaChannels[0]];
					}
					
					if(rgbaChannels[1] >= 0 && rgbaBits[1] != 0)
					{
						int start = 0;
						for(int j=0; j<rgbaChannels[1]; ++j)
						{
							start += bitCount[j];
						}
						g = pixel >> start;
						g &= bitMask[rgbaChannels[1]];
					}
					
					if(rgbaChannels[2] >= 0 && rgbaBits[2] != 0)
					{
						int start = 0;
						for(int j=0; j<rgbaChannels[2]; ++j)
						{
							start += bitCount[j];
						}
						b = pixel >> start;
						b &= bitMask[rgbaChannels[2]];
					}
					
					if(pixels >= totalPixels)
					{
						break;
					}
					
					// Check for alpha-only mode explictly and replicate alpha in all channels
					if(rgbaBits[0] == 0 && rgbaBits[1] == 0 && rgbaBits[2] == 0)
					{
						rgbOut[dest+0] = pixel;
						rgbOut[dest+1] = pixel;
						rgbOut[dest+2] = pixel;
						continue;
					}
					
					rgbOut[dest+0] = r << rdiff;
					rgbOut[dest+1] = g << gdiff;
					rgbOut[dest+2] = b << bdiff;
				}
			}
		}
	}
}

bool MyWindow::writeBitmap(const char* filename)
{
	if(m_data.size() <= 0 || !isValid())
	{
		return false;
	}
	
	FILE* f = fopen(filename, "wb");
	if(f)
	{
		u32 w = static_cast<u32>(getImageWidth());
		u32 h = static_cast<u32>(getImageHeight());
		u32 size = w * h * 3;
		
		// Create bitmap on hard disk
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
		fwrite(m_pixels, size, 1, f);
		fclose(f);
		
		return true;
	}
	
	return false;
}

bool MyWindow::writeTga(const char* filename)
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
void MyWindow::ButtonCallback(Fl_Widget* widget, void* param)
{
	if(!param)
	{
		return;
	}
	
	MyWindow* p = static_cast<MyWindow*>(param);
	char buff[128];
	
	if(widget == &p->m_saveButton)
	{
		int w = p->getImageWidth();
		int h = p->getImageHeight();
		int o = p->getOffset();
		snprintf(buff, sizeof(buff), "Pixels_%dx%d_%d.tga", w, h, o);
		
		if(!p->writeTga(buff))
		{
			fl_message("Saving failed. Either format is invalid or no data exists."); 
		}
	}
	else if(widget == &p->m_openButton)
	{
		u32 offset = static_cast<u32>(std::max(atoi(p->m_offset.value()), 0));
        Fl_Native_File_Chooser browser;
        const char* filename = p->m_currentFile;
		
		// Show file dialog only if we are not in auto-reload mode
        if(p->m_autoReload.value() == 0)
        {
            snprintf(buff, sizeof(buff), "Open file from offset: %d Bytes", offset);
            
            browser.title(buff);
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
                u32 size = static_cast<u32>(ftell(f));
                fseek(f, 0, SEEK_SET);
                
                if(offset >= size)
                {
                	// If auto-reload is on do nothing when pointing out of bounds
                	if(p->m_autoReload.value() != 0)
                	{
                		fclose(f);
                		return;
                	}
                	
                    fl_message("Offset %d larger then file size (%d Bytes). Reading from offset 0 instead.", offset, size);
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
                
                size = std::min(size - offset, kMaxImageSize);
                p->m_accumOffset = offset;
                
                memset(p->m_text, 0, kMaxBufferSize);
                fseek(f, offset, SEEK_CUR);
                fread(p->m_text, size, 1, f);
                fclose(f);
                
                // Store current file for the accumulated offset
                if(strcmp(filename, p->m_currentFile) != 0)
                {
                    memset(p->m_currentFile, 0, sizeof(p->m_currentFile));
                    snprintf(p->m_currentFile, sizeof(p->m_currentFile), "%s", filename);
                }
                
                // Set data and update image view
                p->m_data.static_value(reinterpret_cast<char*>(p->m_text), size);
                UpdateCallback(&p->m_data, param);
                
                // Move data cursor to data start
                p->m_data.position(0, 0);

				// Assign new accumulation offset
                char num[32];
                p->m_offset.value(itoa(p->m_accumOffset, num, 10));
            }
        }
	}
	else if(widget == &p->m_aboutButton)
	{
		#if defined _WIN32 && defined __GNUC__
		snprintf(buff, sizeof(buff), "PixelDbg %.2f\nNikita Kindt (n.kindt.pdbg<at>gmail.com)\nCompiled on %s with TDM-GCC %d.%d.%d", 
				 kVersion, __DATE__, __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
		#else
		#error Not there yet
		#endif

		fl_message(buff);
	}
}

void MyWindow::OffsetCallback(Fl_Widget* widget, void* param)
{
	if(!param)
	{
		return;
	}
	
	MyWindow* p = static_cast<MyWindow*>(param);		
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
}

void MyWindow::ChannelCallback(Fl_Widget* widget, void* param)
{
	if(!param)
	{
		return;
	}
	
	MyWindow* p = static_cast<MyWindow*>(param);
			
	if(p->updatePixelFormat())
	{
		UpdateCallback(widget, param);
	}
}

void MyWindow::DimCallback(Fl_Widget* widget, void* param)
{
	if(!param)
	{
		return;
	}
	
	MyWindow* p = static_cast<MyWindow*>(param);
    
	int w = p->getImageWidth();
	int h = p->getImageHeight();
	if(w < 1 || w > kMaxDim || h < 1 || h > kMaxDim)
	{ 
		w = std::min(std::max(1, w), (int)kMaxDim);
		h = std::min(std::max(1, h), (int)kMaxDim);
		
		char num[8];
		p->m_width.value(itoa(w, num, 10));
		p->m_height.value(itoa(h, num, 10));
	}
	
	if(p->updatePixelFormat())
	{
		UpdateCallback(widget, param);
	}
}

void MyWindow::TileCallback(Fl_Widget* widget, void* param)
{
	if(!param)
	{
		return;
	}
	MyWindow* p = static_cast<MyWindow*>(param);
	char num[16];

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
	else if(widget == &p->m_tileX)
	{
		int w = p->getImageWidth();
		int t = atoi(p->m_tileX.value());

		if(t < 1 || t > w)
		{
			t = std::min(std::max(t, 1), w);
			p->m_tileX.value(itoa(t, num, 10));
		}
		
		UpdateCallback(widget, param);
	}
	else if(widget == &p->m_tileY)
	{
		int h = p->getImageHeight();
		int t = atoi(p->m_tileY.value());

		if(t < 1 || t > h)
		{
			t = std::min(std::max(t, 1), h);
			p->m_tileY.value(itoa(t, num, 10));
		}
		
		UpdateCallback(widget, param);
	}
}

void MyWindow::UpdateCallback(Fl_Widget* widget, void* param) // Callback to update image
{
	MyWindow* p = static_cast<MyWindow*>(param);

	if(!p || !p->isValid())
	{
		return;
	}
	
	const u8* text = reinterpret_cast<const u8*>(p->m_data.value());
	u32 length = static_cast<u32>(p->m_data.size());
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
	char buff[64];
	snprintf(buff, sizeof(buff), "%u/%u Bytes", length, kMaxBufferSize);
	p->m_byteCount.copy_label(buff);

	if(!text || length == 0 || ps <= 0)
	{
		return;
	}
	
	// Wipe old data
	u32 size = u32(w) * u32(h) * 3;
	memset(p->m_pixels, 0, size);
	
	// Convert new data
	length = std::min(size, length);
	p->convertData(text, p->m_pixels, length, u32(tx), u32(ty));
	
	// Show new image
	if(p->m_image)
	{
		p->m_image->Fl_RGB_Image::~Fl_RGB_Image();
	}
	p->m_image = new (p->m_rawMemoryFlRGBImage) Fl_RGB_Image(p->m_pixels, w, h, 3);
	p->m_imageBox.image(p->m_image);
	p->m_imageBox.resize(p->m_imageBox.x(), p->m_imageBox.y(), std::min(w, p->w() - 5), h);
	
	if(widget != p)
	{
		// Flush main window but only if we are not resizing.
		// With flush the image will be visible but vanish when we resize.
		// Without flush the image won't be visible until we resize.
		// Seems to be a problem with FLTK so this workaround is needed.
		p->flush();
	}
}

