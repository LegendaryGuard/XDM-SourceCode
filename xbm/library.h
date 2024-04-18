/*
Copyright:
Botman, XaeroX, Uncle Mike, Xawari
*/
#ifndef LIBRARY_H
#define LIBRARY_H
#ifdef _WIN32
#ifndef __MINGW32__
#pragma once
#endif /* !__MINGW32__ */
#endif


#define DOS_SIGNATURE	0x5A4D		/* MZ */
#define NT_SIGNATURE	0x00004550	/* PE00 */

#define SIZEOF_SHORT_NAME			8
#define NUMBER_OF_DIRECTORY_ENTRIES	16
#define MAX_LIBRARY_EXPORTS			4096/* max exported symbols */


// DOS executable header
typedef struct
{	
	WORD	e_magic;	// Magic number
	WORD	e_cblp;		// Bytes on last page of file
	WORD	e_cp;		// Pages in file
	WORD	e_crlc;		// Relocations
	WORD	e_cparhdr;	// Size of header in paragraphs
	WORD	e_minalloc;	// Minimum extra paragraphs needed
	WORD	e_maxalloc;	// Maximum extra paragraphs needed
	WORD	e_ss;		// Initial (relative) SS value
	WORD	e_sp;		// Initial SP value
	WORD	e_csum;		// Checksum
	WORD	e_ip;		// Initial IP value
	WORD	e_cs;		// Initial (relative) CS value
	WORD	e_lfarlc;	// File address of relocation table
	WORD	e_ovno;		// Overlay number
	WORD	e_res[4];	// Reserved words
	WORD	e_oemid;	// OEM identifier (for e_oeminfo)
	WORD	e_oeminfo;	// OEM information; e_oemid specific
	WORD	e_res2[10];	// Reserved words
	long	e_lfanew;	// File address of new exe header
} DOS_HEADER;//, *P_DOS_HEADER;

// Windows executable header
typedef struct
{	
	WORD	Machine;
	WORD	NumberOfSections;
	DWORD	TimeDateStamp;
	DWORD	PointerToSymbolTable;
	DWORD	NumberOfSymbols;
	WORD	SizeOfOptionalHeader;
	WORD	Characteristics;
} PE_HEADER;//, *P_PE_HEADER;

typedef struct
{
	byte	Name[SIZEOF_SHORT_NAME];
	union
	{
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
} SECTION_HEADER;//, *P_SECTION_HEADER;

typedef struct
{
	DWORD	VirtualAddress;
	DWORD	Size;
} DATA_DIRECTORY;//, *P_DATA_DIRECTORY;

typedef struct
{
	WORD	Magic;
	byte	MajorLinkerVersion;
	byte	MinorLinkerVersion;
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

	DATA_DIRECTORY	DataDirectory[NUMBER_OF_DIRECTORY_ENTRIES];
} OPTIONAL_HEADER;//, *P_OPTIONAL_HEADER;

typedef struct
{
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
} EXPORT_DIRECTORY;//, *P_EXPORT_DIRECTORY;

typedef struct dll_user_s
{
	void	*hInstance;		// to avoid possible hacks
	int		custom_loader;	// a bit who indicated loader type
	int		encrypted;		// dll is crypted (some client.dll in HL, CS etc)
	char	dllName[32];	// for debug messages
	string	fullPath, shortPath;// actual dll paths
	// ordinals stuff
	word	*ordinals;
	dword	*funcs;
	char	*names[MAX_LIBRARY_EXPORTS];// max 4096 exports supported
	int	num_ordinals;		// actual exports count
	dword	funcBase;			// base offset
} dll_user_t;


void *Com_LoadLibrary(const char *dllname, int build_ordinals_table);
void *Com_LoadLibraryExt(const char *dllname, int build_ordinals_table, qboolean directpath);
void *Com_GetProcAddress(void *hInstance, const char *name);
const char *Com_NameForFunction(void *hInstance, DWORD function);
DWORD Com_FunctionFromName(void *hInstance, const char *pName);
void Com_FreeLibrary(void *hInstance);


#endif//LIBRARY_H
