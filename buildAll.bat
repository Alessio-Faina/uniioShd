cd sys
build -cewZ
cd ..\testExe
build -cewZ


cd ..\sys\objchk_win7_x86\i386
copy uniioctl.sys ..\..\..\testExe\objchk_win7_x86\i386
cd ..\..\..
PAUSE