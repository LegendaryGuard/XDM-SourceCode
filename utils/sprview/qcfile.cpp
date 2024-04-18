#include "stdafx.h"
#include <commdlg.h>
#include "sprview.h"

void SendQCHeader(FILE *qcFile, char *spritePath, TSpriteData *theSprite)
{
	if (qcFile)
	{
		char *tPos;

		fprintf(qcFile, "//\n");
		fprintf(qcFile, "// %s\n", spritePath);
		fprintf(qcFile, "//\n");
		fprintf(qcFile, "// Half-Life sprite script file \n");
		fprintf(qcFile, "//\n");							   

		tPos = spritePath;
		while (strchr(tPos, '\\'))
		{
			tPos = strchr(tPos, '\\') + 1;
		}
		fprintf(qcFile, "$spritename    %s\n", tPos);
		
		fprintf(qcFile, "$type          ");
		switch (theSprite->header.type)
		{
			case SPR_VP_PARALLEL_UPRIGHT:
				fprintf(qcFile, "vp_parallel_upright\n");
				break;
			case SPR_FACING_UPRIGHT:
				fprintf(qcFile, "facing_upright\n");
				break;
			case SPR_VP_PARALLEL:
				fprintf(qcFile, "vp_parallel\n");
				break;
			case SPR_ORIENTED:
				fprintf(qcFile, "oriented\n");
				break;
			case SPR_VP_PARALLEL_ORIENTED:
				fprintf(qcFile, "vp_parallel_oriented\n");
				break;
		}
		
		fprintf(qcFile, "$texture		");
		switch (theSprite->header.texFormat)
		{
			case SPR_NORMAL:
				fprintf(qcFile, "normal\n");
				break;
			case SPR_ADDITIVE:
				fprintf(qcFile, "additive\n");
				break;
			case SPR_INDEXALPHA:
				fprintf(qcFile, "indexalpha\n");
				break;
			case SPR_ALPHTEST:
				fprintf(qcFile, "alphatest\n");
				break;
		}
	}
}

void SendQCFrame(FILE *qcFile, char *fileName, TSpriteFrame *theFrame)
{
	if (qcFile)
	{
		char tLine[256];
		char *pPos, *tPos;
		
		strcpy(tLine, fileName);
		tPos = tLine;		
		while (strchr(tPos, '\\'))
		{
			tPos = strchr(tPos, '\\') + 1;
		}
		pPos = strchr(tPos, '.');
		if (pPos)
			*pPos = 0;

		fprintf(qcFile, "$load		%s\n", tPos);
		fprintf(qcFile, "$frame		0   0   %d   %d\n",
			theFrame->frame.width, theFrame->frame.height);
	}
}