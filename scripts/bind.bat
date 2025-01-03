@echo off

REM Store the output of usbipd list into a temporary file
usbipd list > temp.txt

REM Find the line containing "COM" and extract the busid
for /f "tokens=1,2,*" %%a in ('findstr "COM" temp.txt') do (
    set "busid=%%a"
    echo Found COM port device with busid: %%a
    goto bind_device
)

:bind_device
if defined busid (
    echo Binding device %busid%...
    usbipd bind --busid %busid%
    echo Attaching device to WSL...
    usbipd attach --wsl --busid %busid%
) else (
    echo No COM port device found!
)

REM Clean up
del temp.txt

pause