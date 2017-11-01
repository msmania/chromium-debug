!IF "$(PLATFORM)"=="X64" || "$(PLATFORM)"=="x64"
ARCH=amd64
!ELSE
ARCH=x86
!ENDIF

OUTDIR=..\$(ARCH)
OBJDIR=obj\$(ARCH)

CC=cl
RD=rd /s /q
RM=del /q
LINKER=link
TARGET=cr.dll
DEF=cr.def

OBJS=\
	$(OBJDIR)\dllmain.obj\
	$(OBJDIR)\object.obj\
	$(OBJDIR)\peimage.obj\
	$(OBJDIR)\blink.obj\

LIBS=\

# warning C4100: unreferenced formal parameter
CFLAGS=\
	/nologo\
	/Zi\
	/c\
	/Fo"$(OBJDIR)\\"\
	/Fd"$(OBJDIR)\\"\
	/O2\
	/EHsc\
	/W4\
	/wd4100\

LFLAGS=\
	/NOLOGO\
	/DEBUG\
	/SUBSYSTEM:WINDOWS\
	/DLL\
	/DEF:$(DEF)\

all: $(OUTDIR)\$(TARGET)

clean:
	@if exist $(OBJDIR) $(RD) $(OBJDIR)
	@if exist $(OUTDIR)\$(TARGET) $(RM) $(OUTDIR)\$(TARGET)
	@if exist $(OUTDIR)\$(TARGET:dll=ilk) $(RM) $(OUTDIR)\$(TARGET:dll=ilk)
	@if exist $(OUTDIR)\$(TARGET:dll=pdb) $(RM) $(OUTDIR)\$(TARGET:dll=pdb)
	@if exist $(OUTDIR)\$(TARGET:dll=exp) $(RM) $(OUTDIR)\$(TARGET:dll=exp)
	@if exist $(OUTDIR)\$(TARGET:dll=lib) $(RM) $(OUTDIR)\$(TARGET:dll=lib)

$(OUTDIR)\$(TARGET): $(OBJS)
	@if not exist $(OUTDIR) mkdir $(OUTDIR)
	$(LINKER) $(LFLAGS) $(LIBS) /PDB:"$(@R).pdb" /OUT:"$@" $**

.cpp{$(OBJDIR)}.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	$(CC) $(CFLAGS) $<
