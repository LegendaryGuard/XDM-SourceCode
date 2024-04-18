#define _NOENUMQBOOL
//  winspool.lib comdlg32.lib shell32.lib ole32.lib 
#define  WIN32_LEAN_AND_MEAN

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <ctype.h>

//#include "../../common/platform.h"

typedef unsigned char 		byte;

typedef enum
{
at_notice,
at_console,		// same as at_notice, but forces a ConPrintf, not a message box
at_aiconsole,	// same as at_console, but only shown if developer level is 2!
at_warning,
at_error,
at_logged		// Server print to console ( only in multiplayer games ).
} ALERT_TYPE;

void ALERT(ALERT_TYPE atype, char *szFmt, ...)
{
	va_list argptr;
	static char string[1024];
	va_start(argptr, szFmt);
	vsprintf(string, szFmt, argptr);
	va_end(argptr);

	printf(string);
}


// TODO
LOAD_FILE_FOR_ME
FREE_FILE


//-----------------------------------------------------------------------------
// Purpose: same as fopen, but no need to include gamedir
// Input  : *name - 'filename.ext'
//			*mode - 'r'
// Output : FILE
//-----------------------------------------------------------------------------
FILE *LoadFile(const char *name, const char *mode)
{
	FILE *f = NULL;
/*	char gamedir[MAX_PATH];
	GET_GAME_DIR(gamedir);
	char file[MAX_PATH];
//	sprintf(file, "%s%s%s", gamedir, PATHSEPARATOR, name);
#ifdef	WIN32
	sprintf(file, "%s\\%s", gamedir, name);
#else	// UNIX/LINUX
	sprintf(file, "%s/%s", gamedir, name);
#endif	// WIN32
*/
	f = fopen(name, mode);
	if (f == NULL)
	{
		ALERT(at_console, "ERROR: Unable to load file '%s'!\n", name);
//wtf		fclose(f);
		return NULL;
	}
	return f;
}


//-----------------------------------------------------------------------------
// COM_
//-----------------------------------------------------------------------------
static char com_token[1500];

// for external use
char *COM_Token(void)
{
	return com_token;
}

/*
==============
COM_Parse

Parse a token out of a string
==============
*/
char *COM_Parse(char *data)
{
	int		c;
	int		len;
	
	len = 0;
	com_token[0] = 0;

	if (!data)
		return NULL;

// skip whitespace
skipwhite:
	while ((c = *data) <= ' ')
	{
		if (c == 0)
			return NULL;			// end of file;
		data++;
	}

// skip // comments
	if (c=='/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}

// XDM3035c: skip /* */ comments
/* TESTME: NEED TO BE TESTED!
	if (c=='/' && data[1] == '*')
	{
		while (*data)
		{
			if (*data == '*')
			{
				data++;
				if (*data == 0 || *data == '/')
					break;
			}
			data++;
		}
		goto skipwhite;
	}*/

// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				return data;
			}
			com_token[len] = c;
			len++;
		}
	}

// parse single characters
	if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c == ',' )
	{
		com_token[len] = c;
		len++;
		com_token[len] = 0;
		return data+1;
	}

// parse a regular word
	do
	{
		com_token[len] = c;
		data++;
		len++;
		c = *data;
	if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c == ',' )
			break;
	} while (c>32);

	com_token[len] = 0;
	return data;
}

/*
==============
COM_TokenWaiting

Returns 1 if additional data is waiting to be processed on this line
==============
*/
int COM_TokenWaiting(char *buffer)
{
	char *p = buffer;
	while (*p && *p!='\n')
	{
		if (!isspace(*p) || isalnum(*p))
			return 1;

		p++;
	}
	return 0;
}



//-----------------------------------------------------------------------------
// "Valve script" parsing routines
//-----------------------------------------------------------------------------
#define SCRIPT_VERSION				1.0f
#ifdef CLIENT_DLL
#define SCRIPT_DESCRIPTION_TYPE		"INFO_OPTIONS"
#else
#define SCRIPT_DESCRIPTION_TYPE		"SERVER_OPTIONS"
#endif

enum objtype_t
{
	O_BADTYPE,
	O_BOOL,
	O_NUMBER,
	O_LIST,
	O_STRING
};

typedef struct
{
	objtype_t type;
	char szDescription[32];
} objtypedesc_t;

objtypedesc_t objtypes[] =
{
	{ O_BOOL  , "BOOL" }, 
	{ O_NUMBER, "NUMBER" }, 
	{ O_LIST  , "LIST" }, 
	{ O_STRING, "STRING" }
};

//-----------------------------------------------------------------------------
// Purpose: Recognize block type by its text description
// Input  : *pszType - "LIST"
// Output : objtype_t
//-----------------------------------------------------------------------------
objtype_t CONFIG_ParseScriptObjectType(char *pszType)
{
	int nTypes = sizeof(objtypes)/sizeof(objtypedesc_t);
	for (int i = 0; i < nTypes; ++i)
	{
		if (!stricmp(objtypes[i].szDescription, pszType))
			return objtypes[i].type;
	}
	return O_BADTYPE;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pBuffer - file buffer
//			*pName - output pointer
//			*pValue - output pointer
//			*pDescription - output pointer
//			&handled - output 1 = success, 0 = failure
// Output : char * - remaining file buffer
//-----------------------------------------------------------------------------
char *CONFIG_ParseScriptObject(char *pBuffer, char *pName, char *pValue, char *pDescription, byte &handled)
{
//	pOutput->fHandled = false;
	handled = 0;
	pBuffer = COM_Parse(pBuffer);
	char *token = COM_Token();
	if (strlen(token) <= 0)
		return pBuffer;

//	pOutput->szKeyName = token;// pointer must last outside
	strcpy(pName, token);

	// Parse the {
	pBuffer = COM_Parse(pBuffer);
	token = COM_Token();
	if (strlen(token) <= 0)
		return pBuffer;

	if (strcmp(token, "{"))
	{
		ALERT(at_console, "Expecting '{', got '%s'", token);
		return false;
	}

	// Parse the Prompt
	pBuffer = COM_Parse(pBuffer);
	token = COM_Token();
	if (strlen(token) <= 0)
		return pBuffer;

//	pOutput->szClassName = token;
	strcpy(pDescription, token);

	// Parse the next {
	pBuffer = COM_Parse(pBuffer);
	token = COM_Token();
	if (strlen(token) <= 0)
		return pBuffer;

	if (strcmp(token, "{"))
	{
		ALERT(at_console, "Expecting '{', got '%s'", token);
		return false;
	}

	// Now parse the type:
	pBuffer = COM_Parse(pBuffer);
	token = COM_Token();
	if (strlen(token) <= 0)
		return pBuffer;

	objtype_t type = CONFIG_ParseScriptObjectType(token);
	if (type == O_BADTYPE)
	{
		ALERT(at_console, "Type '%s' unknown", token);
		return false;
	}

	switch (type)
	{
	case O_BOOL:
		// Parse the next {
		pBuffer = COM_Parse(pBuffer);
		token = COM_Token();
		if (strlen(token) <= 0)
			return pBuffer;

		if (strcmp(token, "}"))
		{
			ALERT(at_console, "Expecting '{', got '%s'", token);
			return false;
		}
		break;
	case O_NUMBER:
		// Parse the Min
		pBuffer = COM_Parse(pBuffer);
		token = COM_Token();
		if (strlen(token) <= 0)
			return pBuffer;

//		fMin = (float)atof(token);
		// Parse the Max
		pBuffer = COM_Parse(pBuffer);
		token = COM_Token();
		if (strlen(token) <= 0)
			return pBuffer;

//		fMax = (float)atof(token);
		// Parse the next {
		pBuffer = COM_Parse(pBuffer);
		token = COM_Token();
		if (strlen(token) <= 0)
			return pBuffer;

		if (strcmp(token, "}"))
		{
			ALERT(at_console, "Expecting '{', got '%s'", token);
			return false;
		}
		break;
	case O_STRING:
		// Parse the next {
		pBuffer = COM_Parse(pBuffer);
		token = COM_Token();
		if (strlen(token) <= 0)
			return pBuffer;

		if (strcmp(token, "}"))
		{
			ALERT(at_console, "Expecting '{', got '%s'", token);
			return false;
		}
		break;
	case O_LIST:
		while (1)// Parse items until we get the }
		{
			// Parse the next {
			pBuffer = COM_Parse(pBuffer);
			token = COM_Token();
			if (strlen(token) <= 0)
				return pBuffer;

			if (!strcmp(token, "}"))
				break;

			// Add the item to a list somewhere
			// AddItem( token )
/*			char strItem[128];
			char strValue[128];
			strcpy(strItem, token);*/

			// Parse the value
			pBuffer = COM_Parse(pBuffer);
			token = COM_Token();
			if (strlen(token) <= 0)
				return pBuffer;

//			strcpy(strValue, token);
//			CScriptListItem *pItem = new CScriptListItem(strItem, strValue);
//			AddItem(pItem);
		}
		break;
	}

	// Now read in the default value

	// Parse the {
	pBuffer = COM_Parse(pBuffer);
	token = COM_Token();
	if (strlen(token) <= 0)
		return pBuffer;

	if (strcmp(token, "{"))
	{
		ALERT(at_console, "Expecting '{', got '%s'", token);
		return false;
	}

	// Parse the default
	pBuffer = COM_Parse(pBuffer);
	token = COM_Token();
//	if (strlen(token) <= 0)
//		return pBuffer;

	// Set the values
//	pOutput->szValue = token;// pointer lasts outside
	strcpy(pValue, token);
//	fdefValue = (float)atof(token);

	// Parse the }
	pBuffer = COM_Parse(pBuffer);
	token = COM_Token();
	if (strlen(token) <= 0)
		return pBuffer;

	if (strcmp(token, "}"))
	{
		ALERT(at_console, "Expecting '{', got '%s'", token);
		return false;
	}

	// Parse the final }
	pBuffer = COM_Parse(pBuffer);
	token = COM_Token();
	if (strlen(token) <= 0)
		return pBuffer;

	if (!stricmp(token, "SetInfo"))
	{
//		bSetInfo = true;
		// Parse the final }
		pBuffer = COM_Parse(pBuffer);
		token = COM_Token();
		if (strlen(token) <= 0)
			return pBuffer;
	}

	if (strcmp(token, "}"))
	{
		ALERT(at_console, "Expecting '{', got '%s'", token);
		return pBuffer;
	}
	handled = 1;
//	pOutput->fHandled = true;
	return pBuffer;
}


//-----------------------------------------------------------------------------
// Purpose: Generate config file by cvars in "script" (valve {{}} fmt)
// Input  : *templatefilename - settings.scr
//			*configfilename - settings.cfg
// Output : int 0 = failure
//-----------------------------------------------------------------------------
int CONFIG_GenerateFromTemplate(const char *templatefilename, const char *configfilename)
{
	int length = 0;
	char *pData;
	char *aFileStart = pData = (char *)LOAD_FILE_FOR_ME((char *)templatefilename, &length);
	FILE *cfg = NULL;
	float fVer = 0.0f;
	int ret = 0;

	// Get the first token.
	pData = COM_Parse(pData);
	char *token = COM_Token();
	if (strlen(token) <= 0)
		goto finish;

	// Read VERSION #
	if (stricmp(token, "VERSION"))
	{
		ALERT(at_console, "Expecting 'VERSION', got '%s' in %s\n", token, templatefilename);
		goto finish;
	}

	// Parse in the version #
	// Get the first token.
	pData = COM_Parse(pData);
	token = COM_Token();
	if (strlen(token) <= 0)
	{
		ALERT(at_console, "Expecting version #");
		goto finish;
	}

	fVer = (float)atof(token);
	if (fVer != SCRIPT_VERSION)
	{
		ALERT(at_console, "Version mismatch, expecting %f, got %f in %s\n", SCRIPT_VERSION, fVer, templatefilename);
		goto finish;
	}

	// Get the "DESCRIPTION"
	pData = COM_Parse(pData);
	token = COM_Token();
	if (strlen(token) <= 0)
		goto finish;

	// Read DESCRIPTION
	if (stricmp(token, "DESCRIPTION"))
	{
		ALERT(at_console, "Expecting 'DESCRIPTION', got '%s' in %s\n", token, templatefilename);
		goto finish;
	}

	// Parse in the description type
	pData = COM_Parse(pData);
	token = COM_Token();
	if (strlen(token) <= 0)
	{
		ALERT(at_console, "Expecting '%s'", SCRIPT_DESCRIPTION_TYPE);
		goto finish;
	}

	if (stricmp(token, SCRIPT_DESCRIPTION_TYPE))
	{
		ALERT(at_console, "Expecting %s, got %s in %s\n", SCRIPT_DESCRIPTION_TYPE, token, templatefilename);
		goto finish;
	}

	// Parse the {
	pData = COM_Parse(pData);
	token = COM_Token();
	if (strlen(token) <= 0)
		goto finish;

	if (strcmp(token, "{"))
	{
		ALERT(at_console, "Expecting '{', got '%s'", token);
		goto finish;
	}

	cfg = LoadFile(configfilename, "wt");
	if (cfg == NULL)
	{
		ALERT(at_console, "Unable to load profile config file %s for writing!\n", configfilename);
		goto finish;
	}

	char *pStart;
	byte cvHandled;
	char cvName[64];
	char cvValue[192];
	char cvDescription[256];
	while (1)
	{
		pStart = pData;// remember start of the block

		pData = COM_Parse(pData);
		token = COM_Token();
		if (strlen(token) <= 0)
			goto finish;

		if (!stricmp(token, "}"))// EOF
			break;

		// Create a new object
		pData = CONFIG_ParseScriptObject(pStart, cvName, cvValue, cvDescription, cvHandled);
		// Get value and write it to the config file
		if (cvHandled)//KVD.fHandled)
		{
			//fprintf(cfg, "%s \"%s\"// %s (%s)\n", cvName, CVAR_GET_STRING(cvName), cvDescription, cvValue);
			fprintf(cfg, "%s \"%s\"// %s\n", cvName, cvValue, cvDescription);
			++ret;
		}
		else// there was some error
		{
			ALERT(at_console, "Some cvar(s) left unread in %s!\n", templatefilename);
			goto finish;// since we cannot possibly find the end of this block or anythin else now
		}
#ifdef _DEBUG
		if (ret >= 1000)
			ALERT(at_logged, "Possible endless loop in %s!\n", templatefilename);
#endif
	}

finish:
	if (aFileStart)
		FREE_FILE(aFileStart);
	if (cfg)
		fclose(cfg);// don't forget to close previous file

	return ret;
}


int main(int argc, char *argv[])
{
	int		i;
	char	*inputfile = NULL;//[ 256 ] = "Lucida Console";
	char	*outputfile = NULL;

	printf("srcipt2config Version 1.0, (build %s)\n", __DATE__ );

	for (i=1 ; i<argc ; ++i)
	{
		if (!strcmp(argv[i],"-version"))
		{
			printf("Version 1.0, build %s\n", __DATE__ );
			exit(0);
		}
		else if (!strcmp(argv[i],"-output"))
		{
			++i;
			if (i < argc)// next argument does exist
			{
				if (outputfile == NULL)
					outputfile = argv[i];
			}
		}
		else if ( argv[i][0] == '-' )
		{
			printf("Unknown option \"%s\"", argv[i]);
		}
		else
		{
			if (inputfile == NULL)
				inputfile = argv[i];
			else
				printf("WARNING: More than one input file specified! Ignoring.\n");
		}
	}

	if (i != argc - 1)
	{
		printf("usage: srcipt2config <file.scr> [-output <filename.cfg>]");
		exit(1);
	}

	if (inputfile == NULL)
	{
		printf("No input file name specified!\n");
		exit(1);
	}

	if (outputfile == NULL)
		outputfile = "settings.cfg";


	int n = CONFIG_GenerateFromTemplate(inputfile, outputfile);

	printf("Generated %d entries.\n", n);

//	Sleep(1000);
	return 0;
}
