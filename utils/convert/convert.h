#ifndef CONVERT_H
#define CONVERT_H

typedef struct color_s
{
	int	r;
	int g;
	int b;
} color_t;

color_t colortable[256];
//color_t red[255];
//color_t grn[255];
//color_t blu[255];

void ReadPalFile(char *pFileName);
void WritePalFile(char *pFileName, bool bReversed);
void WriteBmpFile(char *pFileName);
void SortColors(void);
void FindIdentical(void);
int comp_red(const void *e1, const void *e2);
int comp_grn(const void *e1, const void *e2);
int comp_blu(const void *e1, const void *e2);

#endif CONVERT_H
