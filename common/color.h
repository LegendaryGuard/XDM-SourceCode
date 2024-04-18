/*====================================================================

 Purpose: RGBA Color class, 4 bytes and all conversions.
 Compatible with uint32 save/restore mechanism.
 Also, all color-related utilities
 Copyright (c) 2011-2016 Xawari
 Header is C and C++ compliant

//==================================================================*/
#ifndef COLOR_H
#define COLOR_H
#ifdef _WIN32
#if !defined (__MINGW32__)
#pragma once
#endif /* !__MINGW32__ */
#endif

//#include <limits.h>


#define HSL_H_MIN		0
#define HSL_H_MAX		360
#define HSL_H_RANGE		(HSL_H_MAX - HSL_H_MIN)

// unsigned long == unsigned int == uint32 == __uint32
#define RGBA2INT(r,g,b,a)	((unsigned long)(((byte)(r)|((WORD)((byte)(g))<<CHAR_BIT))|(((DWORD)(byte)(b))<<(2*CHAR_BIT))|(((DWORD)(byte)(a))<<(3*CHAR_BIT))))
#define RGB2INT(r,g,b)		((unsigned long)(((byte)(r)|((WORD)((byte)(g))<<CHAR_BIT))|(((DWORD)(byte)(b))<<(2*CHAR_BIT))))
#define RGB2R(rgb)			((unsigned char)(rgb))
#define RGB2G(rgb)			((unsigned char)(((unsigned short)(rgb)) >> CHAR_BIT))
#define RGB2B(rgb)			((unsigned char)((rgb) >> (2*CHAR_BIT)))
#define RGB2A(rgba)			((unsigned char)((rgba) >> (3*CHAR_BIT)))
#define RGBA2A(rgba)		((unsigned char)((rgba) >> (3*CHAR_BIT)))

/*struct color24bit_s
{
	byte r;
	byte g;
	byte b;
?	byte a;
};

union color24bit
{
	color24bit_s bytes;
	byte array[4];
	uint32 integer;// ABGR
};*/

#ifdef __cplusplus

// RGBA 4 Bytes
class Color
{
public:
	union
	{
		uint32 integer;// ABGR
		byte array[4];
		struct
		{
			byte r;
			byte g;
			byte b;
			byte a;
		};
	};

	Color()
	{
//#if defined (_DEBUG)
//		Set(0xFF000000);// opaque, makes mistakes visible?
//#else
		Set(0x00000000);
//#endif
	}
	Color(const uint32 &color4b)
	{
		Set(color4b);
	}
	Color(byte r, byte g, byte b)// bytes are [0...255]. Assuming alpha = 255 for use with RGBA functions
	{
		Set(r, g, b, 255);
	}
	Color(int r, int g, int b)// integers are [0...255]. Assuming alpha = 255 for use with RGBA functions
	{
		Set((byte)r, (byte)g, (byte)b, 255);
	}
	Color(byte r, byte g, byte b, byte a)// bytes are [0...255]
	{
		Set(r, g, b, a);
	}
	Color(int r, int g, int b, int a)// integers are [0...255]
	{
		Set((byte)r, (byte)g, (byte)b, (byte)a);
	}
	Color(byte array[])// bytes are [0...255]
	{
		Set(array[0], array[1], array[2], array[3]);
	}

	// Set
	void Set(const uint32 &color4b)
	{
		integer = color4b;
	}
	void Set(const byte &br, const byte &bg, const byte &bb, const byte &ba)
	{
		r = br;
		g = bg;
		b = bb;
		a = ba;
	}
	void Set(const byte &br, const byte &bg, const byte &bb)// does NOT change alpha value
	{
		r = br;
		g = bg;
		b = bb;
	}
	void Set3b(const byte *brgb)// Requires byte[3] array // does NOT change alpha value
	{
		r = brgb[0];
		g = brgb[1];
		b = brgb[2];
	}
	void Set4b(const byte *brgba)// Requires byte[4] array
	{
		r = brgba[0];
		g = brgba[1];
		b = brgba[2];
		a = brgba[3];
	}
	void Set3f(const float &fr, const float &fg, const float &fb)// floats are [0...1] // does NOT change alpha value
	{
		r = (byte)(fr*255.0f);// should really use round()
		g = (byte)(fg*255.0f);
		b = (byte)(fb*255.0f);
	}
	void Set3f(const float *frgb)// Requires float[3] array // floats are [0...1] // does NOT change alpha value
	{
		r = (byte)(frgb[0]*255.0f);// should really use round()
		g = (byte)(frgb[1]*255.0f);
		b = (byte)(frgb[2]*255.0f);
	}
	void Set4f(const float &fr, const float &fg, const float &fb, const float &fa)// floats are [0...1]
	{
		r = (byte)(fr*255.0f);// should really use round()
		g = (byte)(fg*255.0f);
		b = (byte)(fb*255.0f);
		a = (byte)(fa*255.0f);
	}
	void Set4f(const float *frgba)// Requires float[4] array // floats are [0...1]
	{
		r = (byte)(frgba[0]*255.0f);// should really use round()
		g = (byte)(frgba[1]*255.0f);
		b = (byte)(frgba[2]*255.0f);
		a = (byte)(frgba[3]*255.0f);
	}
	// Get
	void Get(byte &br, byte &bg, byte &bb, byte &ba) const
	{
		br = r;
		bg = g;
		bb = b;
		ba = a;
	}
	void Get(int &ir, int &ig, int &ib, int &ia) const
	{
		ir = r;
		ig = g;
		ib = b;
		ia = a;
	}
	void Get(int &color4b) const
	{
		color4b = integer;
	}
	void Get4f(float &fr, float &fg, float &fb, float &fa)// floats are [0...1]
	{
		fr = ((float)r/255.0f);
		fg = ((float)g/255.0f);
		fb = ((float)b/255.0f);
		fa = ((float)a/255.0f);
	}

	// Type conversion
	operator uint32()
	{
		return integer;
	}

	byte &operator[](size_t index)
	{
		//ASSERTD(index <= 3);
		if (index <= 3)
			return array[index];
		//return *((unsigned char *)(c.integer)+(sizeof(byte))*index);
		return a;
	}

	// Equality operators
	bool operator == (Color &rhs) const
	{
		return (integer == rhs.integer);
	}
	bool operator != (Color &rhs) const
	{
		return (integer != rhs.integer);
	}

	// Assignment operators
	inline Color& operator-=(const Color &c);
	inline Color& operator+=(const Color &c);
	inline Color& operator*=(const float &f);
	inline Color& operator/=(const float &f);
	inline Color& operator*=(const Color &c);// a[i]*b[i]
	inline Color& operator-=(const byte &c);
	inline Color& operator+=(const byte &c);

	//inline Color& operator=(const color24 &c);// HL compatibility

	// Unary operators
	Color operator-() const { return Color(255-r, 255-g, 255-b, 255-a); }// does this make sence?
	Color operator+() const { return *this; }

	// Binary operators
	inline Color operator-(const Color&) const;
	inline Color operator+(const Color&) const;
	inline Color operator/(const Color&) const;
    inline Color operator*(const Color&) const;
	inline Color operator^(const Color&) const;// CROSS PRODUCT

	// Misc
	void SetBlack(void) { Set(0x000000FF); }
	void SetWhite(void) { Set(0xFFFFFFFF); }
};

// Assignment operators
inline Color& Color::operator-=(const Color &c)
{
	r -= c.r;
	g -= c.g;
	b -= c.b;
	a -= c.a;
	return *this;
}

inline Color& Color::operator+=(const Color &c)
{
	r += c.r;
	g += c.g;
	b += c.b;
	a += c.a;
	return *this;
}

inline Color& Color::operator*=(const float &f)
{
	r *= (int)f;
	g *= (int)f;
	b *= (int)f;
	a *= (int)f;
	return *this;
}

inline Color& Color::operator/=(const float &f)
{
	r /= (int)f;
	g /= (int)f;
	b /= (int)f;
	a /= (int)f;
	return *this; 
}

inline Color& Color::operator*=(const Color &c)
{
	r *= c.r;
	g *= c.g;
	b *= c.b;
	a *= c.a;
	return *this; 
}

inline Color& Color::operator-=(const byte &v)
{
	r -= v;
	g -= v;
	b -= v;
	a -= v;
	return *this;
}

inline Color& Color::operator+=(const byte &v)
{
	r += v;
	g += v;
	b += v;
	a += v;
	return *this;
}


// Binary operators
inline Color Color::operator-(const Color &c) const
{
	return (Color(	r - c.r,
					g - c.g,
					b - c.b));
}

inline Color Color::operator+(const Color &c) const
{
	return (Color(	r + c.r,
					g + c.g,
					b + c.b));
}

inline Color Color::operator/(const Color &c) const
{
	return (Color(	r / c.r,
					g / c.g,
					b / c.b));
}

inline Color Color::operator*(const Color &c) const
{
	return (Color(	r * c.r,
					g * c.g,
					b * c.b));
}

// Stand-alone operators
inline Color operator*(float f, const Color &c)
{
	return Color((byte)(c.r*f), (byte)(c.g*f), (byte)(c.b*f));//, (byte)(c.a*f));
}

inline Color operator*(const Color &c, float f)
{
	return Color((byte)(c.r*f), (byte)(c.g*f), (byte)(c.b*f));//, (byte)(c.a*f));
}

inline Color operator/(const Color &c, float f)
{
	return Color((byte)(c.r/f), (byte)(c.g/f), (byte)(c.b/f));//, (byte)(c.a/f));
}

inline Color operator+(const Color &c, byte f)
{
	return Color(c.r+f, c.g+f, c.b+f);//, c.a+f);
}

inline Color operator+(byte f, const Color &c)
{
	return Color(c.r+f, c.g+f, c.b+f);//, c.a+f);
}

inline Color operator-(const Color &c, byte f)
{
	return Color(c.r-f, c.g-f, c.b-f);//, c.a-f);
}

inline Color operator-(byte f, const Color &c)
{
	return Color(f-c.r, f-c.g, f-c.b);//, f-c.a);
}

// Some constants
//#define ColorRed		Color(255,0,0,255);
//#define ColorGreen		Color(0,255,0,255);
//#define ColorBlue		Color(0,0,255,255);

#else /* __cplusplus */

typedef struct Color_s
{
	color24bit c;
} Color;

#endif /* __cplusplus */




void Int2RGB(int rgb, int &r, int &g, int &b);
void Int2RGB(uint32 rgb, byte &r, byte &g, byte &b);
void Int2RGBA(uint32 rgb, byte &r, byte &g, byte &b, byte &a);

bool StringToColor(const char *str, class Color &c);
bool StringToRGB(const char *str, byte &r, byte &g, byte &b);
bool StringToRGBA(const char *str, byte &r, byte &g, byte &b, byte &a);
bool StringToRGBA(const char *str, float &r, float &g, float &b, float &a);

// XDM3035
void RGB2HSL(const float &r, const float &g, const float &b, float &h, float &s, float &l);// r,g,b 0...1; h,s,l must be != 0 to be calculated
void RGB2HSL(const byte &rb, const byte &gb, const byte &bb, float &h, float &s, float &l);// h,s,l 0...255; must be != 0 to be calculated
void HSL2RGB(float h, float s, float l, float &r, float &g, float &b);// r,g,b 0...1
void HSL2RGB(float h, float s, float l, byte &rb, byte &gb, byte &bb);// r,g,b 0...255
short RGB2colormap(const byte &rb, const byte &gb, const byte &bb);// XDM3037a
void colormap2RGB(short colormap, bool bottomcolor, byte &r, byte &g, byte &b);
short colormap(byte topcolor, byte bottomcolor);
byte colormap2topcolor(short colormap);
byte colormap2bottomcolor(short colormap);

//void rgb2hsv(float R, float G, float B, float &H, float &S, float &V);
//void hsv2rgb(float H, float S, float V, float &R, float &G, float &B);

void ScaleColorsF(int &r, int &g, int &b, const float &a);
void ScaleColors(int &r, int &g, int &b, const int &a);
void ScaleColorsF(byte &r, byte &g, byte &b, const float &a);
void ScaleColors(byte &r, byte &g, byte &b, const byte &a);


extern DLL_GLOBAL Color g_Palette[256];

#endif /* COLOR_H */
