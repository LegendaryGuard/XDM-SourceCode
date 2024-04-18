//====================================================================
//
// Purpose: Implementation of all color utilities
//
//====================================================================
//#include "platform.h"
#include "vector.h"
#include "util_vector.h"
#include "const.h"
#include "color.h"

//-----------------------------------------------------------------------------
// Purpose: Extract color channels
// Input  : rgb - 24 bits, 8 bits per channel
// Output : &r g b - separate channels
//-----------------------------------------------------------------------------
void Int2RGB(int rgb, int &r, int &g, int &b)
{
	r = RGB2R(rgb);
	g = RGB2G(rgb);
	b = RGB2B(rgb);
}

//-----------------------------------------------------------------------------
// Purpose: Extract color channels
// Input  : rgb - 24 bits, 8 bits per channel
// Output : &r g b - separate channels
//-----------------------------------------------------------------------------
void Int2RGB(uint32 rgb, byte &r, byte &g, byte &b)
{
	r = RGB2R(rgb);
	g = RGB2G(rgb);
	b = RGB2B(rgb);
}

//-----------------------------------------------------------------------------
// Purpose: Extract color channels
// Input  : rgb - 24 bits, 8 bits per channel
// Output : &r g b - separate channels
//-----------------------------------------------------------------------------
void Int2RGBA(uint32 rgb, byte &r, byte &g, byte &b)
{
	r = RGB2R(rgb);
	g = RGB2G(rgb);
	b = RGB2B(rgb);
}

//-----------------------------------------------------------------------------
// Purpose: Parse RGBA from string to Color
// Input  : *str - "<R> <G> <B> [A]" string, where each channel is 0...255
//			&c - class Color
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool StringToColor(const char *str, ::Color &c)
{
	unsigned short ir, ig, ib, ia = 255;// XDM3037a: 'a' needs default value
	if (str && sscanf(str, "%hu %hu %hu %hu", &ir, &ig, &ib, &ia) >= 3)// 'short' is the shortest value it can read
	{
		c.r = ir;
		c.g = ig;
		c.b = ib;
		c.a = ia;// may not be scanned
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Parse RGB from string
// Input  : *str - "255 255 255"
//			&r &g &b - output
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool StringToRGB(const char *str, byte &r, byte &g, byte &b)
{
	unsigned short ir, ig, ib;
	if (str && sscanf(str, "%hu %hu %hu", &ir, &ig, &ib) == 3)// 'short' is the shortest value it can read
	{
		r = ir;
		g = ig;
		b = ib;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Parse RGBA from string
// Input  : *str - "255 255 255 255"
//			&r &g &b &a - output
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool StringToRGBA(const char *str, byte &r, byte &g, byte &b, byte &a)
{
	int ir, ig, ib, ia;
	if (str && sscanf(str, "%d %d %d %d", &ir, &ig, &ib, &ia) == 4)// scanf will probably write 4 bytes for %d
	{
		r = ir;
		g = ig;
		b = ib;
		a = ia;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Parse RGBA from string
// Input  : *str - "1 1 1 1"
//			&r &g &b &a - output
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool StringToRGBA(const char *str, float &r, float &g, float &b, float &a)
{
	//float cr,cg,cb,ca;
	//if (str && sscanf(str, "%f %f %f %f", &cr, &cg, &cb, &ca) == 4)
	if (str && sscanf(str, "%f %f %f %f", &r, &g, &b, &a) == 4)
	{
		//r = cr; g = cg; b = cb; a = ca;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: RGB2HSL. h,s,l must be non-zero to be calculated
// Warning: S cannot be calculated without L, L==0 will be ignored
// Input  : &r g b - 0...1
// Output :
//			&h - Hue is 0...360
//			&s - Saturation is 0...1
//			&l - Lightness is 0...1
//-----------------------------------------------------------------------------
void RGB2HSL(const float &r, const float &g, const float &b, float &h, float &s, float &l)
{
	float delta;
	const float *pmin, *pmax;

	pmin = &min(r,min(g,b));
	pmax = &max(r,max(g,b));
	delta = *pmax - *pmin;

	if (h)// H requested
	{
		h = 0.0f;
		if (delta > 0.0f)
		{
			if (pmax == &r && pmax != &g)
				h += (g - b) / delta;
			if (pmax == &g && pmax != &b)
				h += (2 + (b - r) / delta);
			if (pmax == &b && pmax != &r)
				h += (4 + (r - g) / delta);

			h *= 60.0f;
		}
	}

	if (l > 0 || s > 0)// L or S requested
	{
		l = (*pmin + *pmax) / 2.0f;
		if (s > 0)// S requested
		{
			if (l > 0.0f && l < 1.0f)
				s = delta / (l < 0.5f ? (2.0f*l) : (2.0f - 2.0f*l));
			else
				s = 0.0f;
		}
	}

	/*
	/double/ float themin,themax,delta;
	themin = min(r,min(g,b));
	themax = max(r,max(g,b));
	delta = themax - themin;

	if (oh)// H requested
	{
		h = 0;
		if (delta > 0.0f)
		{
			if (themax == r && themax != g)
				h += (g - b) / delta;
			if (themax == g && themax != b)
				h += (2 + (b - r) / delta);
			if (themax == b && themax != r)
				h += (4 + (r - g) / delta);

			h *= 60.0f;
		}
	}

	if (os && ol)// SL requested
	{
		s = 0;
		l = (themin + themax) / 2.0f;

		if (l > 0.0f && l < 1.0f)
			s = delta / (l < 0.5f ? (2.0f*l) : (2.0f - 2.0f*l));
	}*/
}

//-----------------------------------------------------------------------------
// Purpose: RGB2HSL. h,s,l must be non-zero to be calculated
// Input  : &r g b - 0...255
//			&h,s,l - 0 = ignore/1 = calculate
// Output :
//			&h - Hue is 0...360
//			&s - Saturation is 0...1
//			&l - Lightness is 0...1
//-----------------------------------------------------------------------------
void RGB2HSL(const byte &rb, const byte &gb, const byte &bb, float &h, float &s, float &l)
{
	const float r = (const float)rb/255.0f;
	const float g = (const float)gb/255.0f;
	const float b = (const float)bb/255.0f;
	RGB2HSL(r,g,b, h,s,l);// try rewriting this algorithm for bytes
}

//-----------------------------------------------------------------------------
// Purpose: Color space conversion
// Input  : h - Hue is in degrees 0...360
//			s - Saturation is 0...1
//			l - Lightness is 0...1
//			&r &g &b - RGB is 0...1
//-----------------------------------------------------------------------------
void HSL2RGB(float h, float s, float l, float &r, float &g, float &b)
{
	if (s == 0.0f)
	{
		r = l; g = l; b = l;
	}
	else
	{
		NormalizeAngle360(&h);// XDM3037a
		//h *= 6.0f; 0...1 -> 0...6
		h /= 60.0f;// 0...360 -> 0...6
		int i = (int)h; // the integer part of H
		float f = h - i;
		float p = l * (1.0f - s);
		float q = l * (1.0f -(s * f));
		float t = l * (1.0f -(s * (1 - f)));
		switch (i)
		{
		case 0: r = l; g = t; b = p; break;
		case 1: r = q; g = l; b = p; break;
		case 2: r = p; g = l; b = t; break;
		case 3: r = p; g = q; b = l; break;
		case 4: r = t; g = p; b = l; break;
		case 5: r = l; g = p; b = q; break;
		default: ASSERTSZ(i == 0, "HSL2RGB: bad Hue specified!\n"); break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Color space conversion
// Input  : h - Hue is in degrees 0...360
//			s - Saturation is 0...1
//			l - Lightness is 0...1
//			&rb &gb &bb - RGB is 0...255
//-----------------------------------------------------------------------------
void HSL2RGB(float h, float s, float l, byte &rb, byte &gb, byte &bb)
{
	float r;
	float g;
	float b;
	HSL2RGB(h,s,l, r,g,b);
	rb = (byte)(r*255.0f);
	gb = (byte)(g*255.0f);
	bb = (byte)(b*255.0f);
}

//-----------------------------------------------------------------------------
// Purpose: Convert RGB color directly to colormap
// Note   : Adds 15 degrees H to bottom color (looks nicer)
// Warning: Returns only 2 bytes, so use RGB2colormap(r,g,b) & 0x0000FFFF with 4-byte integers!
// Input  : &rb &gb &bb - RGB is 0...255
// Output : int lowbyte topcolor, highbyte bottomcolor, in HL Hue = 0...255
//-----------------------------------------------------------------------------
short RGB2colormap(const byte &rb, const byte &gb, const byte &bb)
{
	float h=1.0f, s=0.0f, l=0.0f;
	RGB2HSL(rb, gb, bb, h,s,l);
	return ((short)(h*255.0f/360.0f)) + (((short)(NormalizeAngle360r(h+15.0f)*255.0f/360.0f)) << CHAR_BIT);// shift 1 byte to the left
}

//-----------------------------------------------------------------------------
// Purpose: Convert player top or bottom color into RGB color
// Input  : colormap - 2 bytes: lowbyte topcolor, highbyte bottomcolor (Hue)
//			bottomcolor - Which color to extract: 0=topcolor, 1=bottomcolor
//			&r &g &b - output
//-----------------------------------------------------------------------------
void colormap2RGB(short colormap, bool bottomcolor, byte &r, byte &g, byte &b)
{
	short hcolor = bottomcolor?((colormap & 0xFF00) >> CHAR_BIT):(colormap & 0x00FF);
	HSL2RGB(((float)hcolor/255.0f)*360.0f, 1.0f, 1.0f, r,g,b);
}

//-----------------------------------------------------------------------------
// Purpose: Make entvars_t 2-byte colormap
// Input  : topcolor - byte
//			bottomcolor - byte
// Output : Returns usable entvars_t colormap (2 actual bytes)
//-----------------------------------------------------------------------------
short colormap(byte topcolor, byte bottomcolor)
{
	// long version return (int)((topcolor & 0x000000FF) | ((bottomcolor << CHAR_BIT) & 0x0000FF00));// shift and cleanup bits
	return (short)((topcolor & 0x00FF) | ((bottomcolor << CHAR_BIT) & 0xFF00));// shift and cleanup bits
}

//-----------------------------------------------------------------------------
// Purpose: Extract "top" color (hue in byte format)
// Input  : colormap - 2 byte entvars_t colormap
// Output : Returns 1 byte
//-----------------------------------------------------------------------------
byte colormap2topcolor(short colormap)
{
	return (colormap & 0x00FF);
}

//-----------------------------------------------------------------------------
// Purpose: Extract "bottom" color (hue in byte format)
// Input  : colormap - 2 byte entvars_t colormap
// Output : Returns 1 byte
//-----------------------------------------------------------------------------
byte colormap2bottomcolor(short colormap)
{
	return (colormap & 0xFF00) >> CHAR_BIT;
}

//-----------------------------------------------------------------------------
// Purpose: Scale RGB by a/255
// Input  : &r &g &b - 0...255
//			&a - 0...1
//-----------------------------------------------------------------------------
void ScaleColorsF(int &r, int &g, int &b, const float &a)
{
	r = (int)((float)r * a);
	g = (int)((float)g * a);
	b = (int)((float)b * a);
}

//-----------------------------------------------------------------------------
// Purpose: Scale RGB by a/255
// Input  : &r &g &b - 0...255
//			&a - 0...255
//-----------------------------------------------------------------------------
void ScaleColors(int &r, int &g, int &b, const int &a)
{
	ScaleColorsF(r,g,b, (float)a/255.0f);
	/*float x = (float)a / 255.0f;
	r = (int)((float)r * x);
	g = (int)((float)g * x);
	b = (int)((float)b * x);*/
}

//-----------------------------------------------------------------------------
// Purpose: Scale RGB by a/255
// Input  : &r &g &b - 0...255
//			&a - 0...1
//-----------------------------------------------------------------------------
void ScaleColorsF(byte &r, byte &g, byte &b, const float &a)
{
	r = (byte)((float)r * a);
	g = (byte)((float)g * a);
	b = (byte)((float)b * a);
}

//-----------------------------------------------------------------------------
// Purpose: Scale RGB by a/255
// Input  : &r &g &b - 0...255
//			&a - 0...255
//-----------------------------------------------------------------------------
void ScaleColors(byte &r, byte &g, byte &b, const byte &a)
{
	ScaleColorsF(r,g,b, (float)a/255.0f);
	/*float x = (float)a / 255.0f;
	r = (byte)((float)r * x);
	g = (byte)((float)g * x);
	b = (byte)((float)b * x);*/
}



// Find the maximum among three values.
/*float max_of3(float first, float second, float third)
{
	float tmp = max( first, second );
	return max( tmp, third );
}

// Find the minimum among three values.
float min_of3(float first, float second, float third)
{
	float tmp = min( first, second );
	return min( tmp, third );
}

void rgb2hsv(float R, float G, float B, float& H, float& S, float& V)
{
	float maxV = max_of3(R, G, B);
	float minV = min_of3(R, G, B);
	float diffV = maxV - minV;
	V = maxV;

	if (maxV != 0.0f)
		S = diffV / maxV;
	else
		S = 0.0f;
	
	if (S == 0.0f)
		H = -1.0f;
	else
	{
		if (R == maxV)
			H = (G - B)/diffV;
		else if (G == maxV)
			H = 2.0f + (B - R)/diffV;
		else
			H = 4.0f + (R - G)/diffV;
		H /= 6.0f;

		if (H < 0.0f) H += 1.0f;
	}
}

void hsv2rgb(float H, float S, float V, float &R, float &G, float &B)
{
	if (S == 0.0f)
	{
		R = V; G = V; B = V;
	}
	else
	{
		if (H == 1.0f)
			H = 0.0f;

		H *= 6.0f;
		int i = (int)H; //the integer part of H
		float f = H - i;
		float p = V * (1.0f - S);
		float q = V * (1.0f - (S * f));
		float t = V * (1.0f - (S * (1 - f)));
		switch (i)
		{
		case 0: R = V; G = t; B = p; break;
		case 1: R = q; G = V; B = p; break;
		case 2: R = p; G = V; B = t; break;
		case 3: R = p; G = q; B = V; break;
		case 4: R = t; G = p; B = V; break;
		case 5: R = V; G = p; B = q; break;
		}
	}
}*/
