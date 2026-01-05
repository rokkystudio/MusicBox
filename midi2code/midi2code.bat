@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem ================================================================
rem  MIDI -> NoteEvent converter (drag&drop .mid/.midi onto this .bat)
rem  Requires:
rem    - python in PATH
rem    - mid2code.py next to this .bat
rem    - pip install mido
rem ================================================================

if "%~1"=="" (
	echo Drag and drop a .mid/.midi file onto this .bat
	pause
	exit /b 1
)

set "IN=%~1"
set "IN_EXT=%~x1"

if /I not "%IN_EXT%"==".mid" if /I not "%IN_EXT%"==".midi" (
	echo Not a .mid/.midi file: "%IN%"
	pause
	exit /b 2
)

rem Script is expected to be in the same folder as this .bat
set "SCRIPT=%~dp0mid2code.py"
if not exist "%SCRIPT%" (
	echo Missing script: "%SCRIPT%"
	echo Put mid2code.py next to this .bat
	pause
	exit /b 3
)

rem Output file next to MIDI with same base name
set "OUT=%~dp1%~n1.txt"

rem Use base name as C array name (sanitize via PowerShell, unicode-safe)
for /f "usebackq delims=" %%A in (`
	powershell -NoProfile -Command ^
	"$n='%~n1';" ^
	"$s=($n -replace '[^A-Za-z0-9_]', '_');" ^
	"if($s -match '^[0-9]'){ $s='song_'+$s }" ^
	"if($s -match '^_+$'){ $s='song0' }" ^
	"if([string]::IsNullOrEmpty($s)){ $s='song0' }" ^
	"$s"
`) do set "NAME=%%A"

echo Input : "%IN%"
echo Output: "%OUT%"
echo Name  : "%NAME%"
echo.

rem Новый скрипт сам:
rem  - берёт ноты из всех треков
rem  - делает моно highest
rem  - квантует по 1/16
rem  - вставляет TEMPO из set_tempo
python "%SCRIPT%" "%IN%" --name "%NAME%" > "%OUT%"
if errorlevel 1 (
	echo.
	echo ERROR: conversion failed.
	echo Tip: run this to inspect tracks/channels:
	echo   python "%SCRIPT%" "%IN%" --inspect
	pause
	exit /b 4
)

echo Done.
pause
exit /b 0
