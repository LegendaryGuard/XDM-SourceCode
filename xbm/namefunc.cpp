#include "extdll.h"
#include "util.h"
#include "enginecallback.h"
#include "cbase.h"
#include "bot.h"

//#undef FUNCTION_FROM_NAME
//#undef NAME_FOR_FUNCTION

extern HINSTANCE h_Library;

#define MAX_LIBRARY_EXPORTS			4096/* max exported symbols */
#define SIZEOF_SHORT_NAME			8
#define NUMBER_OF_DIRECTORY_ENTRIES	16

#define DOS_SIGNATURE	0x5A4D		/* MZ */
#define NT_SIGNATURE	0x00004550	/* PE00 */

// globals
WORD *p_Ordinals = NULL;
DWORD *p_Functions = NULL;
DWORD *p_Names = NULL;
//char *g_FunctionNames = NULL;// TODO: "aaaa\tbbbbbb\tccccccc\0"
char *p_FunctionNames[MAX_LIBRARY_EXPORTS];
DWORD num_ordinals = 0;
unsigned long base_offset = 0;


typedef struct {                        // DOS .EXE header
	WORD	e_magic;                     // Magic number
	WORD	e_cblp;                      // Bytes on last page of file
	WORD	e_cp;                        // Pages in file
	WORD	e_crlc;                      // Relocations
	WORD	e_cparhdr;                   // Size of header in paragraphs
	WORD	e_minalloc;                  // Minimum extra paragraphs needed
	WORD	e_maxalloc;                  // Maximum extra paragraphs needed
	WORD	e_ss;                        // Initial (relative) SS value
	WORD	e_sp;                        // Initial SP value
	WORD	e_csum;                      // Checksum
	WORD	e_ip;                        // Initial IP value
	WORD	e_cs;                        // Initial (relative) CS value
	WORD	e_lfarlc;                    // File address of relocation table
	WORD	e_ovno;                      // Overlay number
	WORD	e_res[4];                    // Reserved words
	WORD	e_oemid;                     // OEM identifier (for e_oeminfo)
	WORD	e_oeminfo;                   // OEM information; e_oemid specific
	WORD	e_res2[10];                  // Reserved words
	LONG	e_lfanew;                    // File address of new exe header
} DOS_HEADER, *P_DOS_HEADER;

typedef struct {
	WORD	Machine;
	WORD	NumberOfSections;
	DWORD	TimeDateStamp;
	DWORD	PointerToSymbolTable;
	DWORD	NumberOfSymbols;
	WORD	SizeOfOptionalHeader;
	WORD	Characteristics;
} PE_HEADER, *P_PE_HEADER;

#define SIZEOF_SHORT_NAME              8

typedef struct {
	BYTE	Name[SIZEOF_SHORT_NAME];
	union {
		DWORD	PhysicalAddress;
		DWORD	VirtualSize;
	} Misc;
	DWORD	VirtualAddress;
	DWORD	SizeOfRawData;
	DWORD	PointerToRawData;
	DWORD	PointerToRelocations;
	DWORD	PointerToLinenumbers;
	WORD	NumberOfRelocations;
	WORD	NumberOfLinenumbers;
	DWORD	Characteristics;
} SECTION_HEADER, *P_SECTION_HEADER;

typedef struct {
	DWORD	VirtualAddress;
	DWORD	Size;
} DATA_DIRECTORY, *P_DATA_DIRECTORY;

#define NUMBEROF_DIRECTORY_ENTRIES    16

typedef struct {
	WORD	Magic;
	BYTE	MajorLinkerVersion;
	BYTE	MinorLinkerVersion;
	DWORD	SizeOfCode;
	DWORD	SizeOfInitializedData;
	DWORD	SizeOfUninitializedData;
	DWORD	AddressOfEntryPoint;
	DWORD	BaseOfCode;
	DWORD	BaseOfData;
	DWORD	ImageBase;
	DWORD	SectionAlignment;
	DWORD	FileAlignment;
	WORD	MajorOperatingSystemVersion;
	WORD	MinorOperatingSystemVersion;
	WORD	MajorImageVersion;
	WORD	MinorImageVersion;
	WORD	MajorSubsystemVersion;
	WORD	MinorSubsystemVersion;
	DWORD	Win32VersionValue;
	DWORD	SizeOfImage;
	DWORD	SizeOfHeaders;
	DWORD	CheckSum;
	WORD	Subsystem;
	WORD	DllCharacteristics;
	DWORD	SizeOfStackReserve;
	DWORD	SizeOfStackCommit;
	DWORD	SizeOfHeapReserve;
	DWORD	SizeOfHeapCommit;
	DWORD	LoaderFlags;
	DWORD	NumberOfRvaAndSizes;
	DATA_DIRECTORY DataDirectory[NUMBEROF_DIRECTORY_ENTRIES];
} OPTIONAL_HEADER, *P_OPTIONAL_HEADER;

typedef struct {
	DWORD	Characteristics;
	DWORD	TimeDateStamp;
	WORD	MajorVersion;
	WORD	MinorVersion;
	DWORD	Name;
	DWORD	Base;
	DWORD	NumberOfFunctions;
	DWORD	NumberOfNames;
	DWORD	AddressOfFunctions;		// RVA from base of image
	DWORD	AddressOfNames;			// RVA from base of image
	DWORD	AddressOfNameOrdinals;	// RVA from base of image
} EXPORT_DIRECTORY, *P_EXPORT_DIRECTORY;


//-----------------------------------------------------------------------------
// Purpose: unknown
// Input  : *str - 
//			*bfp - 
//-----------------------------------------------------------------------------
/*void FgetString(char *str, FILE *bfp)
{
	char ch;
//	size_t l = 0;
	while ((ch = fgetc(bfp)) != EOF)
	{
		*str++ = ch;
//		++l;
		if (ch == 0)
			break;
	}
//	return l;
}*/

//-----------------------------------------------------------------------------
// Purpose: Memory deallocation
//-----------------------------------------------------------------------------
void FreeNameFuncGlobals(void)
{
	if (p_Ordinals)
	{
		free(p_Ordinals);
		p_Ordinals = NULL;
	}
	if (p_Functions)
	{
		free(p_Functions);
		p_Functions = NULL;
	}
	if (p_Names)
	{
		free(p_Names);
		p_Names = NULL;
	}
/*	if (g_FunctionNames)
	{
		free(g_FunctionNames);
		g_FunctionNames = NULL;
	}*/

	for (DWORD i=0; i < num_ordinals; ++i)
	{
		if (p_FunctionNames[i])
			free(p_FunctionNames[i]);
	}
	num_ordinals = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Strip C++ function names off some garbage
// Input  : *out_name - "ManagerThink@CMultiManager"
//			*in_name - "?ManagerThink@CMultiManager@@QAEXXZ"
//-----------------------------------------------------------------------------
void getMSVCName(char *out_name, const char *in_name)
{
	const char *pos;
	if (in_name[0] == '?')  // is this a MSVC C++ mangled name?
	{
		if ((pos = strstr(in_name, "@@")) != NULL)
		{
			size_t len = pos - in_name;
			strncpy(out_name, &in_name[1], len);  // strip off the leading '?'
			out_name[len-1] = 0;  // terminate string at the "@@"
			return;
		}
	}
	strcpy(out_name, in_name);
}

#define FUNCTION_NAME_MAX_LEN		256

//-----------------------------------------------------------------------------
// Purpose: Load exported symbols from a dynamic-link library
// Input  : *filename - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool LoadSymbols(const char *filename)
{
	if (filename == NULL || *filename == 0)
		return false;

	conprintf(1, "XBM: Loading symbols for %s\n", filename);

	FILE *bfp;
	if ((bfp=fopen(filename, "rb")) == NULL)
	{
		ALERT(at_error, "DLL file %s not found!", filename);
		return false;
	}

	long name_offset;
	long exports_offset;
	long ordinal_offset;
	long function_offset;
	size_t i, index;
	long rdata_delta;
	char function_name[FUNCTION_NAME_MAX_LEN];
	DOS_HEADER dos_header;
	LONG nt_signature;
	PE_HEADER pe_header;
	SECTION_HEADER section_header;
	OPTIONAL_HEADER optional_header;
	EXPORT_DIRECTORY export_directory;
	bool rdata_found;

	if (fread(&dos_header, sizeof(dos_header), 1, bfp) != 1)
	{
		ALERT(at_error, "%s is NOT a valid DLL file!\n", filename);
		return false;
	}
	if (dos_header.e_magic != DOS_SIGNATURE)
	{
		ALERT(at_error, "file does not have a valid DLL signature!\n");
		return false;
	}
	if (fseek(bfp, dos_header.e_lfanew, SEEK_SET) != 0)
	{
		ALERT(at_error, "error seeking to new exe header!\n");
		return false;
	}
	if (fread(&nt_signature, sizeof(nt_signature), 1, bfp) != 1)
	{
		ALERT(at_error, "file does not have a valid NT signature!\n");
		return false;
	}
	if (nt_signature != NT_SIGNATURE)
	{
		ALERT(at_error, "file does not have a valid NT signature!\n");
		return false;
	}
	if (fread(&pe_header, sizeof(pe_header), 1, bfp) != 1)
	{
		ALERT(at_error, "file does not have a valid PE header!\n");
		return false;
	}
	if (pe_header.SizeOfOptionalHeader == 0)
	{
		ALERT(at_error, "file does not have an optional header!\n");
		return false;
	}
	if (pe_header.NumberOfSections == 0)
	{
		ALERT(at_error, "file does not have sections!\n");
		return false;
	}
	memset(&optional_header, 0, sizeof(OPTIONAL_HEADER));
	if (fread(&optional_header, sizeof(OPTIONAL_HEADER), 1, bfp) != 1)
	{
		ALERT(at_error, "file does not have a valid optional header!\n");
		return false;
	}
	memset(&section_header, 0, sizeof(SECTION_HEADER));// XDM3038: just so the compiler may shut up
	// XaeroX
	rdata_found = false;
	for (i = 0; i < pe_header.NumberOfSections; ++i)
	{
		if (fread(&section_header, sizeof(SECTION_HEADER), 1, bfp) != 1)
		{
			ALERT(at_error, "Error reading section %d header!\n", i);
			return false;
		}

		if ((optional_header.DataDirectory[0].VirtualAddress >= section_header.VirtualAddress) && 
			(optional_header.DataDirectory[0].VirtualAddress < (section_header.VirtualAddress + section_header.Misc.VirtualSize)))
		{
			rdata_found = true;
			break;
		}
	}

	if (rdata_found)
		rdata_delta = section_header.VirtualAddress - section_header.PointerToRawData; 
	else
		rdata_delta = 0;

	exports_offset = optional_header.DataDirectory[0].VirtualAddress - rdata_delta;

	if (fseek(bfp, exports_offset, SEEK_SET) != 0)
	{
		ALERT(at_error, "file does not have a valid exports section!\n");
		return false;
	}

	memset(&export_directory, 0, sizeof(EXPORT_DIRECTORY));
	if (fread(&export_directory, sizeof(EXPORT_DIRECTORY), 1, bfp) != 1)
	{
		ALERT(at_error, "file does not have a valid optional header!");
		return false;
	}

	num_ordinals = export_directory.NumberOfNames;  // also number of ordinals
	if (num_ordinals > MAX_LIBRARY_EXPORTS)
	{
		ALERT(at_error, "Too many symbols to process! Header may be invalid!\n");
		return false;
	}

	for (i=0; i < num_ordinals; ++i)// XDM: lol, moved here
		p_FunctionNames[i] = NULL;

	ordinal_offset = export_directory.AddressOfNameOrdinals - rdata_delta;

	if (fseek(bfp, ordinal_offset, SEEK_SET) != 0)
	{
		ALERT(at_error, "file does not have a valid ordinals section!\n");
		return false;
	}

	if ((p_Ordinals = (WORD *)malloc(num_ordinals * sizeof(WORD))) == NULL)
	{
		ALERT(at_error, "error allocating memory for ordinals section!\n");
		return false;
	}

	if (fread(p_Ordinals, num_ordinals * sizeof(WORD), 1, bfp) != 1)
	{
		FreeNameFuncGlobals();
		ALERT(at_error, "error reading ordinals table!\n");
		return false;
	}

	function_offset = export_directory.AddressOfFunctions - rdata_delta;

	if (fseek(bfp, function_offset, SEEK_SET) != 0)
	{
		ALERT(at_error, "file does not have a valid export address section!\n");
		return false;
	}

	if ((p_Functions = (DWORD *)malloc(num_ordinals * sizeof(DWORD))) == NULL)
	{
		ALERT(at_error, "error allocating memory for export address section!\n");
		return false;
	}

	if (fread(p_Functions, num_ordinals * sizeof(DWORD), 1, bfp) != 1)
	{
		FreeNameFuncGlobals();
		ALERT(at_error, "error reading export address section!\n");
		return false;
	}

	name_offset = export_directory.AddressOfNames - rdata_delta;

	if (fseek(bfp, name_offset, SEEK_SET) != 0)
	{
		FreeNameFuncGlobals();
		ALERT(at_error, "file does not have a valid names section!\n");
		return false;
	}

	if ((p_Names = (DWORD *)malloc(num_ordinals * sizeof(DWORD))) == NULL)
	{
		FreeNameFuncGlobals();
		ALERT(at_error, "error allocating memory for names section!\n");
		return false;
	}

	if (fread(p_Names, num_ordinals * sizeof(DWORD), 1, bfp) != 1)
	{
		FreeNameFuncGlobals();
		ALERT(at_error, "error reading names table!\n");
		return false;
	}

	// XDM3037a: put everythin into one 1D array
/*	TODO UNDONE
	size_t mem_size = 0;
	name_offset = p_Names[num_ordinals-1] - rdata_delta;
	if (name_offset != 0)
	{
		mem_size = name_offset;
		if (fseek(bfp, name_offset, SEEK_SET) != 0)
		{
			char ch;
			while ((ch = fgetc(bfp)) != EOF)
			{
				++mem_size;
				if (ch == 0)
					break;
			}
		}
	}
	if (mem_size == 0)
	{
		FreeNameFuncGlobals();
		ALERT(at_error, "error!\n");
		return false;
	}
	mem_size += num_ordinals*sizeof(char);// delimiters
	g_FunctionNames = (char *)malloc(mem_size);
	if (g_FunctionNames == NULL)
	{
		FreeNameFuncGlobals();
		ALERT(at_error, "error allocating memory form ordinal names!\n");
		return false;
	}

	size_t fnames_offset = 0;// never reset this!
	bool bCPPname;
	bool bGotOneAt;
	char ch;
	for (i = 0; i < num_ordinals; ++i)
	{
		name_offset = p_Names[i] - rdata_delta;
		if (name_offset != 0)
		{
			if (fseek(bfp, name_offset, SEEK_SET) != 0)// seek to next name in header
			{
				bCPPname = false;
				bGotOneAt = false;
				while ((ch = fgetc(bfp)) != EOF)// read name char by char
				{
					if (!bCPPname && ch == '?')// CPP name detected
					{
						bCPPname = true;
						continue;
					}
					else if (bCPPname)
					{
						if (ch == '@')
						{
							if (bGotOneAt)// this is second '@'
							{
								g_FunctionNames[fnames_offset-1] = '\t';// end of name
								break;
							}
							else// got first '@'
							{
								bGotOneAt = true;
							}
						}
						else
							bGotOneAt = false;
					}
					g_FunctionNames[fnames_offset] = ch;
					++fnames_offset;
					if (ch == 0)
						break;
				}
//				FgetString(function_name, bfp);
//				getMSVCName(p_FunctionNames[i], function_name);
			}
			else break;
		}
	}
	g_FunctionNames[fnames_offset] = 0;*/

	for (i=0; i < num_ordinals; ++i)
	{
		name_offset = p_Names[i] - rdata_delta;
		if (name_offset != 0)
		{
			if (fseek(bfp, name_offset, SEEK_SET) == 0)
			{
//				FgetString(function_name, bfp);
				if (fgets(function_name, FUNCTION_NAME_MAX_LEN, bfp))
				{
					p_FunctionNames[i] = (char *)malloc(strlen(function_name)+1);
					getMSVCName(p_FunctionNames[i], function_name);
				}
				else
					break;
			}
			else
				break;
		}
	}

	if (i != num_ordinals)
	{
		FreeNameFuncGlobals();
		ALERT(at_error, "error while loading names section!");
		return false;
	}

	fclose(bfp);
	bfp = NULL;

	// Now, find the base for all offsets
	void *game_GiveFnptrsToDll = NULL;
	for (i=0; i < num_ordinals; ++i)
	{
		if (strcmp("GiveFnptrsToDll", p_FunctionNames[i]) == 0)
		{
			index = p_Ordinals[i];
			game_GiveFnptrsToDll = (void *)GetProcAddress(h_Library, "GiveFnptrsToDll");
			base_offset = (unsigned long)(game_GiveFnptrsToDll) - p_Functions[index];
			break;
		}
	}

	if (p_Names)// we don't need those anymore
	{
		free(p_Names);
		p_Names = NULL;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Restore function pointer (offset) by name
// Warning: Used by CRestore::ReadField()
// I guess GetProcAddress would be fine, but we need opposite function too.
// Input  : *pName - 
// Output : unsigned long
//-----------------------------------------------------------------------------
unsigned long FUNCTION_FROM_NAME(const char *pName)
{
	DWORD i, index;
	for (i=0; i < num_ordinals; ++i)
	{
		if (strcmp(pName, p_FunctionNames[i]) == 0)
		{
			index = p_Ordinals[i];
			return p_Functions[index] + base_offset;
		}
	}
	return 0L;  // couldn't find the function name to return address
}

//-----------------------------------------------------------------------------
// Purpose: Get printable/readable name of a function
// Warning: Used by CSave::WriteFunction()
// Input  : function - 
// Output : const char
//-----------------------------------------------------------------------------
const char *NAME_FOR_FUNCTION(unsigned long function)
{
	DWORD i, index;
	for (i=0; i < num_ordinals; ++i)
	{
		index = p_Ordinals[i];
		if ((function - base_offset) == p_Functions[index])
			return p_FunctionNames[i];
	}
	return NULL;  // couldn't find the function address to return name
}
