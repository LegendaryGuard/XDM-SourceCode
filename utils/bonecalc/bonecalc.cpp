#include <stdio.h>
#include <string.h>

// some temporary program I wrote and forgot about
void main(int argc, char **argv)
{
	printf("\n - Bone calculator 1.0 by xawari -\n BUILD %s\n", __DATE__);
	if (argc == 1)
	{
		printf("Usage: bonecalc <filename> [outfile]\n");
		return;
	}

	FILE *pFile = fopen(argv[1], "r");

	if (!pFile)
	{
		printf("Unable to open %s\n", argv[1]);
		return;
	}
	else
		printf("Parsing %s\n", argv[1]);

	char str[256];
	int flag = 0;
	while (fgets(str, 256, pFile))
	{
		if (!strnicmp(str, "time 0", 5))
		{
			flag = 1;
			break;
		}
	}

	if (flag <= 0)
	{
		printf("No 'time 0' section found\n");
		fclose(pFile);
		return;
	}

	flag = 0;
	int bones = 0;
	float pos[3];
	float ang[3];

	float finalpos[3];
	float finalang[3];
	finalpos[0] = 0;
	finalpos[1] = 0;
	finalpos[2] = 0;
	finalang[0] = 0;
	finalang[1] = 0;
	finalang[2] = 0;

	while (fgets(str, 256, pFile))
	{
		if (!strnicmp(str, "end", 3))
			break;
		else
		{
			sscanf(str, "%d %f %f %f %f %f %f", &bones, &pos[0], &pos[1], &pos[2], &ang[0], &ang[1], &ang[2]);
			printf("Bone %d (%f %f %f) (%f %f %f)\n", bones, pos[0], pos[1], pos[2], ang[0], ang[1], ang[2]);
			finalpos[0] += pos[0];
			finalpos[1] += pos[1];
			finalpos[2] += pos[2];
			finalang[0] += ang[0];
			finalang[1] += ang[1];
			finalang[2] += ang[2];
			printf("Accumulated: %f %f %f %f %f %f\n", finalpos[0], finalpos[1], finalpos[2], finalang[0], finalang[1], finalang[2]);
		}
	}
	printf("Final: %f %f %f %f %f %f\n", finalpos[0], finalpos[1], finalpos[2], finalang[0], finalang[1], finalang[2]);
	fclose(pFile);

	pFile = fopen(argv[2], "w");

	if (pFile)
	{
		printf("Using %s for output\n", argv[2]);
		fprintf(pFile, "%f %f %f %f %f %f\n", finalpos[0], finalpos[1], finalpos[2], finalang[0], finalang[1], finalang[2]);
		fclose(pFile);
	}
}
