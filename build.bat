@echo off
REM Build script for MSVC (run from anywhere). Sets up vcvars64 then compiles.
setlocal
pushd %~dp0
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 ( echo vcvars64 failed & popd & exit /b 1 )

cl /nologo /EHsc /MT /O2 /std:c++17 ^
   /Ilib\include ^
   main.cpp msvc_shim.cpp ^
   /Fe:game.exe /Fo:build\ ^
   /link /LIBPATH:lib\lib /LTCG ^
   freeglut.lib opengl32.lib glu32.lib ^
   legacy_stdio_definitions.lib ^
   user32.lib gdi32.lib winmm.lib advapi32.lib ^
   /SUBSYSTEM:CONSOLE
set ERR=%ERRORLEVEL%
popd
exit /b %ERR%
