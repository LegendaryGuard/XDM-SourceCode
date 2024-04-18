REM This script launches IMDISK to create a virtual RAM drive and format it to FAT32
REM Used for temporary (intermediate) files during build

REM imdisk -a -s 1G -m R: -p "/fs:FAT32 /Q /V:RAMDRIVE /Y"
runas /noprofile /user:Administrator "imdisk -a -s 1G -m R: -p \"/fs:FAT32 /Q /V:RAMDRIVE /Y\""

pause
cls
