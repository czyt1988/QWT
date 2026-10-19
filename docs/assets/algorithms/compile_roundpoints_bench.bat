@echo off
rem Build roundpoints_bench.cpp against the repository build tree (MSVC x64).
rem Adjust QT_DIR and the Visual Studio path for the local installation.
setlocal
set ROOT=%~dp0..\..\..
if "%QT_DIR%"=="" set QT_DIR=D:\Qt\6.7.3\msvc2019_64
if "%QWT_BUILD_DIR%"=="" set QWT_BUILD_DIR=%ROOT%\build

call "D:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 (
    echo Could not initialize the MSVC environment - edit the vcvars64.bat path in this file.
    exit /b 1
)

cl /nologo /O2 /EHsc /std:c++17 /Zc:__cplusplus /MD /permissive- ^
   /I"%ROOT%\src" /I"%ROOT%\src\core" /I"%ROOT%\src\plot" ^
   /I"%QT_DIR%\include" /I"%QT_DIR%\include\QtCore" /I"%QT_DIR%\include\QtGui" ^
   /I"%QT_DIR%\include\QtWidgets" /I"%QT_DIR%\include\QtConcurrent" ^
   /I"%QT_DIR%\include\QtPrintSupport" ^
   "%~dp0roundpoints_bench.cpp" /Fe:"%~dp0roundpoints_bench.exe" ^
   /link /LIBPATH:"%QWT_BUILD_DIR%\lib\Release" /LIBPATH:"%QT_DIR%\lib" ^
   qwtplot.lib qwtcore.lib Qt6Widgets.lib Qt6Gui.lib Qt6Core.lib ^
   Qt6PrintSupport.lib Qt6Concurrent.lib opengl32.lib
endlocal
