#pragma warning(disable : 4244)     // type conversion warning.
#define INCLUDELIBS

#ifdef NeXT
#include <libc.h>
#endif

//#include <conio.h>//getch();
#include "bmp2spr.h"

#define MAX_BUFFER_SIZE		0xF00000
#define MAX_FRAMES			1024

dsprite_t		sprite;
byte			*byteimage, *lbmpalette;
int				byteimagewidth, byteimageheight;
byte			*lumpbuffer = NULL, *plump;
//char			spritedir[1024];
//char			spriteoutname[1024];
int				framesmaxs[2];
int				framecount;

qboolean do16bit = true;

typedef struct
{
	spriteframetype_t	type;		// single frame or group of frames
	void				*pdata;		// either a dspriteframe_t or group info
	float				interval;	// only used for frames in groups
	int					numgroupframes;	// only used by group headers
} spritepackage_t;

spritepackage_t	frames[MAX_FRAMES];

void WriteFrame(FILE *spriteouthandle, int framenum)
{
	dspriteframe_t	*pframe;
	dspriteframe_t	frametemp;

	pframe = (dspriteframe_t *)frames[framenum].pdata;
	frametemp.origin[0] = LittleLong (pframe->origin[0]);
	frametemp.origin[1] = LittleLong (pframe->origin[1]);
	frametemp.width = LittleLong (pframe->width);
	frametemp.height = LittleLong (pframe->height);

	SafeWrite(spriteouthandle, &frametemp, sizeof (frametemp));
	SafeWrite(spriteouthandle, (byte *)(pframe + 1), pframe->height * pframe->width);
}

void WriteSprite(FILE *spriteouthandle)
{
	int			i, groupframe, curframe;
	dsprite_t	spritetemp;

	sprite.boundingradius = sqrt (((framesmaxs[0] >> 1) *
								   (framesmaxs[0] >> 1)) +
								  ((framesmaxs[1] >> 1) *
								   (framesmaxs[1] >> 1)));
// write out the sprite header
	spritetemp.type = LittleLong (sprite.type);
	spritetemp.texFormat = LittleLong (sprite.texFormat);
	spritetemp.boundingradius = LittleFloat (sprite.boundingradius);
	spritetemp.width = LittleLong (framesmaxs[0]);
	spritetemp.height = LittleLong (framesmaxs[1]);
	spritetemp.numframes = LittleLong (sprite.numframes);
	spritetemp.beamlength = LittleFloat (sprite.beamlength);
	spritetemp.synctype = LittleFloat (sprite.synctype);
	spritetemp.version = LittleLong (SPRITE_VERSION);
	spritetemp.ident = LittleLong (IDSPRITEHEADER);

	SafeWrite (spriteouthandle, &spritetemp, sizeof(spritetemp));

	if(do16bit)
	{
		// Write out palette in 16bit mode
		short cnt = 256;
		SafeWrite(spriteouthandle, (void *) &cnt, sizeof(cnt));
		SafeWrite(spriteouthandle, lbmpalette, cnt * 3);
	}

// write out the frames
	curframe = 0;
	for (i=0 ; i<sprite.numframes ; i++)
	{
		SafeWrite (spriteouthandle, &frames[curframe].type, sizeof(frames[curframe].type));
		if (frames[curframe].type == SPR_SINGLE)
		{
		// single (non-grouped) frame
			WriteFrame (spriteouthandle, curframe);
			curframe++;
		}
		else
		{
			int					j, numframes;
			dspritegroup_t		dsgroup;
			float				totinterval;

			groupframe = curframe;
			curframe++;
			numframes = frames[groupframe].numgroupframes;
		// set and write the group header
			dsgroup.numframes = LittleLong (numframes);
			SafeWrite (spriteouthandle, &dsgroup, sizeof(dsgroup));

		// write the interval array
			totinterval = 0.0;

			for (j=0 ; j<numframes ; j++)
			{
				dspriteinterval_t	temp;

				totinterval += frames[groupframe+1+j].interval;
				temp.interval = LittleFloat (totinterval);
				SafeWrite(spriteouthandle, &temp, sizeof(temp));
			}

			for (j=0 ; j<numframes ; j++)
			{
				WriteFrame(spriteouthandle, curframe);
				curframe++;
			}
		}
	}
}

void LoadScreen(char *name)
{
	static byte origpalette[256*3];
	int iError;

	printf("grabbing from %s...\n",name);
	iError = LoadBMP(name, &byteimage, &lbmpalette);
	if (iError)
		Error("Unable to load file \"%s\"\n", name);

	byteimagewidth = bmhd.w;
	byteimageheight = bmhd.h;

	if (sprite.numframes == 0)
		memcpy(origpalette, lbmpalette, sizeof( origpalette ));
	else if (memcmp(origpalette, lbmpalette, sizeof( origpalette )) != 0)
		Error("bitmap \"%s\" doesn't share a pallette with the previous bitmap\n", name);
}

int SPR_Type(char *str)
{
	int type = SPR_VP_PARALLEL;
	if (str != NULL && *str != '\0')
	{
		if (!strcmp (str, "vp_parallel"))
			type = SPR_VP_PARALLEL;
		else if (!strcmp (str, "vp_parallel_upright"))
			type = SPR_VP_PARALLEL_UPRIGHT;
		else if (!strcmp (str, "facing_upright"))
			type = SPR_FACING_UPRIGHT;
		else if (!strcmp (str, "oriented"))
			type = SPR_ORIENTED;
		else if (!strcmp (str, "vp_parallel_oriented"))
			type = SPR_VP_PARALLEL_ORIENTED;
		else
			printf("SPR_Type: Bad sprite type '%s'! Using vp_parallel.\n", str);
	}
	return type;
}

int SPR_Texture(char *str)
{
	int texFormat = SPR_ADDITIVE;
	if (str != NULL && *str != '\0')
	{
		if (!strcmp (str, "additive"))
			texFormat = SPR_ADDITIVE;
		else if (!strcmp (str, "normal"))
			texFormat = SPR_NORMAL;
		else if (!strcmp (str, "indexalpha"))
			texFormat = SPR_INDEXALPHA;
		else if (!strcmp (str, "alphatest"))
			texFormat = SPR_ALPHTEST;
		else
			printf("SPR_Texture: Bad sprite texture type '%s'! Using additive.\n", str);
	}
	return texFormat;
}

void FinishSprite (char *spriteoutname)
{
	FILE	*spriteouthandle = NULL;

	if (sprite.numframes == 0)
		Error("no frames\n");

	if (!strlen(spriteoutname))
		Error("Didn't name sprite file");

	if ((plump - lumpbuffer) > MAX_BUFFER_SIZE)
		Error("Sprite package too big; increase MAX_BUFFER_SIZE");

	spriteouthandle = SafeOpenWrite(spriteoutname);
	printf("--------\nsaving in %s\n", spriteoutname);
	WriteSprite(spriteouthandle);
	fclose (spriteouthandle);
	
	printf("--------\n%d frame(s)\n", sprite.numframes);
	printf("%d ungrouped frame(s), including group headers\n", framecount);

//	spriteoutname[0] = 0;		// clear for a new sprite
}

int main (int argc, char **argv)
{
	qboolean firsttime = true;
	dspriteframe_t *pframe = NULL;
	byte *screen_p, *source;
	int i = 0, x=0, y=0, pix=0, j = 0;
	int spr_texture = SPR_ADDITIVE, spr_type = SPR_VP_PARALLEL, spr_beamlength = 16;
	char outname[256];

	printf("bmp2spr.exe v 2.2 (build %s)\n", __DATE__);
	printf("----- Easy Sprite Gen ----\n\n");
	Q_getwd(outname);
	printf("Working dir: %s\n", outname);

	if (argc < 2)
		Error ("usage: <progname> [-texture $] [-type $] [-beamlength #] [-archive $] [-no16bit] <flile1.bmp> [file2.bmp] ...");

	memset(outname, 0, sizeof(outname));// !
	for(i=1; i<argc; i++)
	{
//		printf("%d:\t%s\n", i, argv[i]);
//		getch();
		if(*argv[i] == '-')
		{
			if(!strcmp(argv[i], "-spritename"))// force output name
			{
				i++;
				strcpy(outname, argv[i]);// copy even if it wasn't empty
			}
			else if(!strcmp(argv[i], "-archive"))
			{
				archive = true;
				i++;
				strcpy(archivedir, argv[i]);
				printf("Archiving source to: %s\n", archivedir);
			}
			else if(!strcmp( argv[i], "-16bit"))
				do16bit = true;
			else if(!strcmp( argv[i], "-no16bit"))
				do16bit = false;
			else if(!strcmp( argv[i], "-texture"))
			{
				i++;
				spr_texture = SPR_Texture(argv[i]);
			}
			else if(!strcmp( argv[i], "-type"))
			{
				i++;
				spr_type = SPR_Type(argv[i]);
			}
			else if(!strcmp( argv[i], "-beamlength"))
			{
				i++;
				spr_beamlength = atof(argv[i]);
			}
			else
				printf("Unsupported command line flag: '%s'", argv[i]);

		}
		else
		{
//			ExtractFilePath(argv[i], spritedir);
			// Cmd_Spritename
			if (firsttime)
			{
				memset(&sprite, 0, sizeof(sprite));
				framecount = 0;
				framesmaxs[0] = -9999999;
				framesmaxs[1] = -9999999;

				if (!lumpbuffer)
					lumpbuffer = malloc(MAX_BUFFER_SIZE * 2);// *2 for padding

				if (!lumpbuffer)
					Error("Couldn't get buffer memory");

				plump = lumpbuffer;
				sprite.synctype = ST_RAND;// default
				sprite.texFormat = spr_texture;
				sprite.type = spr_type;
				sprite.beamlength = spr_beamlength;

				if (outname[0] == 0)// may be already set by filename/command
				{
//					ExtractFileBase(argv[i], outname);
					j = strlen(argv[i]) - 1;
					while (j > 0 && argv[i][j] != '.' && !PATHSEPARATOR(argv[i][j]))
						j--;
					if (j > 0)
					{
						strncpy(outname, argv[i], j);
						strcat(outname, ".spr\0");
					}
				}
				firsttime = false;
			}
			// Cmd_Load
			LoadScreen(ExpandPathAndArchive(argv[i]));
			// Cmd_Frame
			pframe = (dspriteframe_t *)plump;
			frames[framecount].pdata = pframe;
			frames[framecount].type = SPR_SINGLE;
			frames[framecount].interval = (float)0.1;
			pframe->origin[0] = -(byteimagewidth >> 1);
			pframe->origin[1] = byteimageheight >> 1;
			pframe->width = byteimagewidth;
			pframe->height = byteimageheight;
			if (byteimagewidth > framesmaxs[0])
				framesmaxs[0] = byteimagewidth;
			
			if (byteimageheight > framesmaxs[1])
				framesmaxs[1] = byteimageheight;
			
			plump = (byte *)(pframe + 1);
			screen_p = byteimage;
			source = plump;
			for (y=0 ; y < byteimagewidth ; y++)
			{
				for (x=0 ; x < byteimageheight ; x++)
				{
					pix = *screen_p;
					*screen_p++ = 0;
					*plump++ = pix;
				}
			}
			framecount++;
			if (framecount >= MAX_FRAMES)
				Error("Too many frames; increase MAX_FRAMES\n");

			sprite.numframes++;
		}
	}
	if (outname[0] == 0)
		sprintf(outname, "sprite.spr\0");// default

//	_chdir(ExtractFilePath);
	printf("Output file name: %s\n", outname);
//	getch();
	FinishSprite(outname);
//	getch();
	return 0;
}
