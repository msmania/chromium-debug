!IF "$(PLATFORM)"=="X64" || "$(PLATFORM)"=="x64"
ARCH=amd64
!ELSE
ARCH=x86
!ENDIF

all:
	@pushd src & nmake /nologo & popd

clean:
	@pushd src & nmake /nologo clean & popd
