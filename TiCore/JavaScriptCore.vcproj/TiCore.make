!IF !defined(BUILDSTYLE)
BUILDSTYLE=Release
!ELSEIF "$(BUILDSTYLE)"=="DEBUG"
BUILDSTYLE=Debug_All
!ENDIF

install:
    set PRODUCTION=1
    set WebKitLibrariesDir=$(SRCROOT)\AppleInternal
    set WebKitOutputDir=$(OBJROOT)
!IF "$(BUILDSTYLE)"=="Release"
    devenv "TiCoreSubmit.sln" /rebuild Release_PGOInstrument
    set PATH=$(SYSTEMDRIVE)\cygwin\bin;$(PATH)
    xcopy "$(SRCROOT)\AppleInternal\tests\SunSpider\*" "$(OBJROOT)\tests\SunSpider" /e/v/i/h/y
    cd "$(OBJROOT)\tests\SunSpider"
    perl sunspider --shell ../../bin/jsc.exe --runs 3
    del "$(OBJROOT)\bin\TiCore.dll"
    cd "$(SRCROOT)\TiCore.vcproj"
    devenv "TiCoreSubmit.sln" /build Release_PGOOptimize
!ELSE
    devenv "TiCoreSubmit.sln" /rebuild $(BUILDSTYLE)
!ENDIF
    -xcopy "$(OBJROOT)\bin\TiCore.dll" "$(DSTROOT)\AppleInternal\bin\" /e/v/i/h/y
    -xcopy "$(OBJROOT)\bin\TiCore_debug.dll" "$(DSTROOT)\AppleInternal\bin\" /e/v/i/h/y
    -xcopy "$(OBJROOT)\bin\TiCore.pdb" "$(DSTROOT)\AppleInternal\bin\" /e/v/i/h/y
    -xcopy "$(OBJROOT)\bin\TiCore_debug.pdb" "$(DSTROOT)\AppleInternal\bin\" /e/v/i/h/y
    -xcopy "$(OBJROOT)\bin\jsc.exe" "$(DSTROOT)\AppleInternal\bin\" /e/v/i/h/y
    -xcopy "$(OBJROOT)\bin\jsc_debug.exe" "$(DSTROOT)\AppleInternal\bin\" /e/v/i/h/y
    -xcopy "$(OBJROOT)\bin\jsc.pdb" "$(DSTROOT)\AppleInternal\bin\" /e/v/i/h/y
    -xcopy "$(OBJROOT)\bin\jsc_debug.pdb" "$(DSTROOT)\AppleInternal\bin\" /e/v/i/h/y
    xcopy "$(OBJROOT)\include\*" "$(DSTROOT)\AppleInternal\include\" /e/v/i/h/y    
    xcopy "$(OBJROOT)\lib\*" "$(DSTROOT)\AppleInternal\lib\" /e/v/i/h/y    
    xcopy "$(OBJROOT)\bin\TiCore.resources\*" "$(DSTROOT)\AppleInternal\bin\TiCore.resources" /e/v/i/h/y
