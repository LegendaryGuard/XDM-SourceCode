#include "cmdlib.h"
#include "mathlib.h"
#include "bspfile.h"

void main (int argc, char **argv)
{
	int			i;
	char		source[1024];
	int			size;
	FILE		*f;

	printf ("---- ENT ----\n" );

	if (argc == 1)
		Error("usage: ent <filename> ...");
		
	for (i=1 ; i<argc ; i++)
	{
		strcpy(source, argv[i]);
		DefaultExtension(source, ".ent");
		f = fopen (source, "rb");
		if (f)
		{
			size = filelength (f);
			fclose (f);
		}
		else
			size = 0;

		LoadENTFile(source);
	}
}
