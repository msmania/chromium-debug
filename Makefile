!IF "$(PLATFORM)"=="X64" || "$(PLATFORM)"=="x64"
ARCH=amd64
!ELSE
ARCH=x86
!ENDIF

OUTDIR=bin\$(ARCH)
OBJDIR=obj\$(ARCH)
SRCDIR=src

CC=cl
RD=rd /s /q
RM=del /q
LINKER=link
TARGET=cr.dll

OBJS=\
	$(OBJDIR)\common.obj\
	$(OBJDIR)\dllmain.obj\
#	$(OBJDIR)\dom.obj\
#	$(OBJDIR)\layout.obj\
#	$(OBJDIR)\object.obj\
#	$(OBJDIR)\oilpan.obj\
#	$(OBJDIR)\partitions.obj\
	$(OBJDIR)\peimage.obj\
	$(OBJDIR)\symbol_manager.obj\
#	$(OBJDIR)\threadstate.obj\
#	$(OBJDIR)\typeinfo.obj\
	$(OBJDIR)\vtable_manager.obj\

LIBS=\
	dbgeng.lib\

# warning C4100: unreferenced formal parameter
CFLAGS=\
	/nologo\
	/Zi\
	/c\
	/Fo"$(OBJDIR)\\"\
	/Fd"$(OBJDIR)\\"\
	/Od\
	/EHsc\
	/W4\
	/wd4100\
	/D_CRT_SECURE_NO_WARNINGS\
	/DUNICODE\

LFLAGS=\
	/NOLOGO\
	/DEBUG\
	/SUBSYSTEM:WINDOWS\
	/DLL\
	/DEF:$(SRCDIR)\cr.def\

all: $(OUTDIR)\$(TARGET)

$(OUTDIR)\$(TARGET): $(OBJS)
	@if not exist $(OUTDIR) mkdir $(OUTDIR)
	$(LINKER) $(LFLAGS) $(LIBS) /PDB:"$(@R).pdb" /OUT:"$@" $**

{$(SRCDIR)}.cpp{$(OBJDIR)}.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	$(CC) $(CFLAGS) $<

clean:
	@if exist $(OBJDIR) $(RD) $(OBJDIR)
	@if exist $(OUTDIR)\$(TARGET) $(RM) $(OUTDIR)\$(TARGET)
	@if exist $(OUTDIR)\$(TARGET:dll=pdb) $(RM) $(OUTDIR)\$(TARGET:dll=pdb)
	@if exist $(OUTDIR)\$(TARGET:dll=lib) $(RM) $(OUTDIR)\$(TARGET:dll=lib)
	@if exist $(OUTDIR)\$(TARGET:dll=exp) $(RM) $(OUTDIR)\$(TARGET:dll=exp)
	@if exist $(OUTDIR)\$(TARGET:dll=ilk) $(RM) $(OUTDIR)\$(TARGET:dll=ilk)
