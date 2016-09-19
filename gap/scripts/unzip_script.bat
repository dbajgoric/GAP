@echo off
if %1.==. GOTO ERR1
if %2.==. GOTO ERR2
"C:\Program Files\7-Zip\7z.exe" e -o%2 -y %1
GOTO END

:ERR1
	ECHO Error: zip filepath not provided
	exit /b 1

:ERR2
	ECHO Error: output folder path not provided
	exit /b 1

:END
exit /b %errorlevel%