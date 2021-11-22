@echo off
setlocal enabledelayedexpansion

for  %%S in (*.c) do (
	set ori=%%S
	echo !ori:.c=.o! : %%S
	echo		cl /c %%S /Fo!ori:.c=.o!
)