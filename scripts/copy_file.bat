@echo off
setlocal enabledelayedexpansion

set local_path=%~dp0
set src_path=%local_path%..\resources
set dest_path=%local_path%..\output\bin\Release\

echo "copy file"
del %dest_path%\duilib.dll
del %dest_path%\yuv.lib
del %dest_path%\SDL2.dll
rd /s /Q %dest_path%resources
md %dest_path%resources

xcopy %src_path% %dest_path%resources /s /e
xcopy %local_path%..\third\bin\duilib.dll %dest_path%
xcopy %local_path%..\third\bin\libyuv.dll %dest_path%
xcopy %local_path%..\third\bin\yuv.lib %dest_path%
xcopy %local_path%..\third\bin\SDL2.dll %dest_path%