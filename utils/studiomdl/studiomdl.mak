# Microsoft Developer Studio Generated NMAKE File, Based on studiomdl.dsp
!IF "$(CFG)" == ""
CFG=studiomdl - Win32 Release
!MESSAGE No configuration specified. Defaulting to studiomdl - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "studiomdl - Win32 Release" && "$(CFG)" != "studiomdl - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "studiomdl.mak" CFG="studiomdl - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "studiomdl - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "studiomdl - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "studiomdl - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\studiomdl.exe" "$(OUTDIR)\studiomdl.bsc"


CLEAN :
	-@erase "$(INTDIR)\cmdlib.obj"
	-@erase "$(INTDIR)\cmdlib.sbr"
	-@erase "$(INTDIR)\lbmlib.obj"
	-@erase "$(INTDIR)\lbmlib.sbr"
	-@erase "$(INTDIR)\mathlib.obj"
	-@erase "$(INTDIR)\mathlib.sbr"
	-@erase "$(INTDIR)\scriplib.obj"
	-@erase "$(INTDIR)\scriplib.sbr"
	-@erase "$(INTDIR)\studiomdl.obj"
	-@erase "$(INTDIR)\studiomdl.res"
	-@erase "$(INTDIR)\studiomdl.sbr"
	-@erase "$(INTDIR)\trilib.obj"
	-@erase "$(INTDIR)\trilib.sbr"
	-@erase "$(INTDIR)\tristrip.obj"
	-@erase "$(INTDIR)\tristrip.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\write.obj"
	-@erase "$(INTDIR)\write.sbr"
	-@erase "$(OUTDIR)\studiomdl.bsc"
	-@erase "$(OUTDIR)\studiomdl.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O2 /I "../../common" /I "../../dlls" /I "../../engine" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\studiomdl.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\studiomdl.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\studiomdl.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\cmdlib.sbr" \
	"$(INTDIR)\lbmlib.sbr" \
	"$(INTDIR)\mathlib.sbr" \
	"$(INTDIR)\scriplib.sbr" \
	"$(INTDIR)\studiomdl.sbr" \
	"$(INTDIR)\trilib.sbr" \
	"$(INTDIR)\tristrip.sbr" \
	"$(INTDIR)\write.sbr"

"$(OUTDIR)\studiomdl.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\studiomdl.pdb" /machine:I386 /out:"$(OUTDIR)\studiomdl.exe" 
LINK32_OBJS= \
	"$(INTDIR)\cmdlib.obj" \
	"$(INTDIR)\lbmlib.obj" \
	"$(INTDIR)\mathlib.obj" \
	"$(INTDIR)\scriplib.obj" \
	"$(INTDIR)\studiomdl.obj" \
	"$(INTDIR)\trilib.obj" \
	"$(INTDIR)\tristrip.obj" \
	"$(INTDIR)\write.obj" \
	"$(INTDIR)\studiomdl.res"

"$(OUTDIR)\studiomdl.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "studiomdl - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\studiomdl.exe" "$(OUTDIR)\studiomdl.bsc"


CLEAN :
	-@erase "$(INTDIR)\cmdlib.obj"
	-@erase "$(INTDIR)\cmdlib.sbr"
	-@erase "$(INTDIR)\lbmlib.obj"
	-@erase "$(INTDIR)\lbmlib.sbr"
	-@erase "$(INTDIR)\mathlib.obj"
	-@erase "$(INTDIR)\mathlib.sbr"
	-@erase "$(INTDIR)\scriplib.obj"
	-@erase "$(INTDIR)\scriplib.sbr"
	-@erase "$(INTDIR)\studiomdl.obj"
	-@erase "$(INTDIR)\studiomdl.res"
	-@erase "$(INTDIR)\studiomdl.sbr"
	-@erase "$(INTDIR)\trilib.obj"
	-@erase "$(INTDIR)\trilib.sbr"
	-@erase "$(INTDIR)\tristrip.obj"
	-@erase "$(INTDIR)\tristrip.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\write.obj"
	-@erase "$(INTDIR)\write.sbr"
	-@erase "$(OUTDIR)\studiomdl.bsc"
	-@erase "$(OUTDIR)\studiomdl.exe"
	-@erase "$(OUTDIR)\studiomdl.ilk"
	-@erase "$(OUTDIR)\studiomdl.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../common" /I "../../dlls" /I "../../engine" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\studiomdl.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\studiomdl.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\studiomdl.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\cmdlib.sbr" \
	"$(INTDIR)\lbmlib.sbr" \
	"$(INTDIR)\mathlib.sbr" \
	"$(INTDIR)\scriplib.sbr" \
	"$(INTDIR)\studiomdl.sbr" \
	"$(INTDIR)\trilib.sbr" \
	"$(INTDIR)\tristrip.sbr" \
	"$(INTDIR)\write.sbr"

"$(OUTDIR)\studiomdl.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\studiomdl.pdb" /debug /machine:I386 /out:"$(OUTDIR)\studiomdl.exe" 
LINK32_OBJS= \
	"$(INTDIR)\cmdlib.obj" \
	"$(INTDIR)\lbmlib.obj" \
	"$(INTDIR)\mathlib.obj" \
	"$(INTDIR)\scriplib.obj" \
	"$(INTDIR)\studiomdl.obj" \
	"$(INTDIR)\trilib.obj" \
	"$(INTDIR)\tristrip.obj" \
	"$(INTDIR)\write.obj" \
	"$(INTDIR)\studiomdl.res"

"$(OUTDIR)\studiomdl.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("studiomdl.dep")
!INCLUDE "studiomdl.dep"
!ELSE 
!MESSAGE Warning: cannot find "studiomdl.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "studiomdl - Win32 Release" || "$(CFG)" == "studiomdl - Win32 Debug"
SOURCE=.\cmdlib.c

"$(INTDIR)\cmdlib.obj"	"$(INTDIR)\cmdlib.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\lbmlib.c

"$(INTDIR)\lbmlib.obj"	"$(INTDIR)\lbmlib.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\mathlib.c

"$(INTDIR)\mathlib.obj"	"$(INTDIR)\mathlib.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\scriplib.c

"$(INTDIR)\scriplib.obj"	"$(INTDIR)\scriplib.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\studiomdl.c

"$(INTDIR)\studiomdl.obj"	"$(INTDIR)\studiomdl.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\trilib.c

"$(INTDIR)\trilib.obj"	"$(INTDIR)\trilib.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\tristrip.c

"$(INTDIR)\tristrip.obj"	"$(INTDIR)\tristrip.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\write.c

"$(INTDIR)\write.obj"	"$(INTDIR)\write.sbr" : $(SOURCE) "$(INTDIR)"


SOURCE=.\studiomdl.rc

"$(INTDIR)\studiomdl.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)



!ENDIF 

