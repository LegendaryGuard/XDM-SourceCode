#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>

typedef unsigned char byte;

typedef struct color_s
{
	byte r;
	byte g;
	byte b;
} color_t;

color_t colortable[256];

void ReadPalFile(char *pFileName);
void WritePalFile(char *pFileName, bool bReversed);
void WriteBmpFile(char *pFileName);
void SortColors(void);
void FindIdentical(void);
int comp_red(const void *e1, const void *e2);
int comp_grn(const void *e1, const void *e2);
int comp_blu(const void *e1, const void *e2);

int main(int argc, char **argv)
{
	printf("----- xawari's JASC Palette converter v1.1 ----\n");

	if (argc-1 <= 0)
	{
		printf("Usage: <exe file name> <pal file name>\nOr just drag-and-drop palette file onto exe file.\nWARNING! Palette file fill be overwritten!\n");
		return -1;
	}

	ReadPalFile(argv[argc-1]);
//	SortColors();
//	char OutFile[32];
//	sprintf(OutFile, "%s_new", argv[argc-1]);
//	WriteBmpFile(OutFile);
//	WritePalFile(OutFile, true);
	WritePalFile(argv[argc-1], true);// write to the same file. just for now.
	printf("Show identical colors? (y)\n");

	if (getch() == 'y')
	{
		FindIdentical();
		getch();
	}

	return 0;
}

	// 1) check 1st line to be "JASC-PAL" (file header).
	// 2) skip all strings before "256".
	// 3) load 3rd line to int variable (wor what? IDK.).
	// 4) process file string by string and fill the colortable.
	// -) create a bitmap 16x16 with all colors!
	// 5) recombine colors (It's so easy, isn't it???:) ).
	// 6) write new palette file.
void ReadPalFile(char *pFileName)
{
	FILE *palfile;
	char cur_str[16];

	printf("Input file name: %s\n", pFileName);

	if ((palfile = fopen(pFileName, "r")) == NULL)
		return;

	if (fgets(cur_str, 16, palfile) == NULL)
		printf("fgets error!\n");
	else if(strcmp(cur_str, "JASC-PAL\n") != 0)
		printf("Invalid palette type: %s\n", cur_str);

	while (strcmp(cur_str, "256\n") != 0)
	{
//		printf("current string: %s", cur_str);
		fgets(cur_str, 16, palfile);
	}

	int i = 0;
	while (fgets(cur_str, 16, palfile))
	{
		printf("reading color: %s", cur_str);// don't use \n! fgets already did it!
		if (sscanf(cur_str, "%d %d %d\n", &colortable[i].r, &colortable[i].g, &colortable[i].b) != 3)
			printf("error reading color entry!\n");

		i ++;// Sleep(100L);
	}

	fclose(palfile);
}

void WritePalFile(char *pFileName, bool bReversed)
{
	FILE *palfile;
	int i;

	printf("Output file name: %s\n", pFileName);
	palfile = fopen(pFileName, "w");
	fprintf(palfile,"JASC-PAL\n0100\n256\n");// write the header

	if (bReversed)
	{
		for(i=255; i > -1; i--)
			fprintf(palfile,"%d %d %d\n", colortable[i].r, colortable[i].g, colortable[i].b);
	}
	else
	{
		for(i=0; i < 256; i++)
			fprintf(palfile,"%d %d %d\n", colortable[i].r, colortable[i].g, colortable[i].b);
	}

	fclose(palfile);
}

void FindIdentical(void)
{
	for (int i = 0; i < 256; i++)
	{
		for (int j = 0; j < 256; j++)
		{
			if (colortable[i].r == colortable[j].r &&
				colortable[i].g == colortable[j].g &&
				colortable[i].b == colortable[j].b && i != j)
			{
				printf("> Identical: %d and %d\n", i, j);
			}
		}
	}
}

/*
void SortColors(void)
{
	int i, ir, ig, ib;
	ir = ig = ib = 0;

	for(i=0; i < 256; i++)
	{
		if((colortable[i].r > colortable[i].g)&&(colortable[i].r > colortable[i].b))
		{
			red[ir]=colortable[i];
			ir++;
		}
		else if((colortable[i].g > colortable[i].r)&&(colortable[i].g > colortable[i].b))
		{
			grn[ig]=colortable[i];
			ig++;
		}
		else
		{
			blu[ib]=colortable[i];
			ib++;
		}
	}

	char OutFile[32];
	sprintf(OutFile, "unsorted.pal");
	WritePalFile(OutFile, false);

//qsort( void *base, size_t num, size_t width, int (__cdecl *compare )(const void *elem1, const void *elem2 ) );
	qsort(red, 256, sizeof(color_t), comp_red);
	qsort(grn, 256, sizeof(color_t), comp_grn);
	qsort(blu, 256, sizeof(color_t), comp_blu);

	for(i=0 ; i < ir; i++)
		colortable[i] = red[i];

	for(i=0 ; i < ig; i++)
		colortable[i+ir] = grn[i];

	for(i=0 ; i < ib; i++)
		colortable[i+ir+ig] = blu[i];
}

int comp_red(const void *e1, const void *e2)
{
	if(((color_t*)e1)->r < ((color_t*)e2)->r)
		return -1;
	if(((color_t*)e1)->r == ((color_t*)e2)->r)
		return 0;
	else
		return 1;
}

int comp_grn(const void *e1, const void *e2)
{
	if(((color_t*)e1)->g < ((color_t*)e2)->g)
		return -1;
	if(((color_t*)e1)->g == ((color_t*)e2)->g)
		return 0;
	else
		return 1;
}

int comp_blu(const void *e1, const void *e2)
{
	if(((color_t*)e1)->b < ((color_t*)e2)->b)
		return -1;
	if(((color_t*)e1)->b == ((color_t*)e2)->b)
		return 0;
	else
		return 1;
}



void CreateRGBPalette(void)
{
	long Cnt;
	BYTE *tPal = IDXpal;

	RGBPalette = (PALETTEENTRY *)malloc(sizeof(PALETTEENTRY) * paletteSize);
	for (Cnt = 0; Cnt < paletteSize; Cnt++)
	{
		RGBPalette[Cnt].peRed	= *tPal++;
		RGBPalette[Cnt].peGreen = *tPal++;
		RGBPalette[Cnt].peBlue	= *tPal++;
		RGBPalette[Cnt].peFlags = PC_NOCOLLAPSE;
	}
}

void CreateIDXPalette(void)
{
	long Cnt;
	BYTE *tPal = IDXpal;

	RGBPalette = (PALETTEENTRY *)malloc(sizeof(PALETTEENTRY) * paletteSize);
	for (Cnt = 0; Cnt < paletteSize; Cnt++)
	{
		RGBPalette[Cnt].peRed	= *tPal++;
		RGBPalette[Cnt].peGreen = *tPal++;
		RGBPalette[Cnt].peBlue	= *tPal++;
		RGBPalette[Cnt].peFlags = PC_NOCOLLAPSE;
	}
}

void WriteBmpFile(char *pFileName)
{
//	printf("Output file name: %s\n", pFileName);
//	WriteBMPfile(pFileName, data, 16, 16, theSprite->palette);
}
*/