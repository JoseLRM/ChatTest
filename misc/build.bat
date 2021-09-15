@echo off

pushd %~dp0\..
set origin_path=%cd%
popd

pushd %origin_path%

SET base=0
SET server=0

:arg_loop
IF "%1"=="" GOTO end_arg_loop

IF "%1"=="base" SET base=1
IF "%1"=="server" SET server=1

SHIFT
GOTO arg_loop
:end_arg_loop

SET name=Networking

SET common_compiler_flags= /std:c++17 /EHsc -wd4047 -wd4311 -wd4312 /Zi -nologo

SET defines= -DSV_PLATFORM_WINDOWS=1 -DSV_SLOW=1

SET out= 

IF "%server%"=="1" (
  SET defines= %defines% -DSERVER
  SET out= build_server
) ELSE (
  SET defines= %defines%
  SET out= build_client
)

SET include_dirs= /I w:\ /I %VULKAN_SDK%\Include\ /I w:\%name%\src\

SET libs= %VULKAN_SDK%\Lib\vulkan-1.lib w:\Hosebase\external\sprv\sprv.lib

SET link=
SET src=
SET src= %src% w:\%name%\src\main.c
SET src= %src% w:\%name%\src\networking.c

IF "%base%"=="1" (
   SET src= %src% w:\Hosebase\build_unit.c w:\Hosebase\src\vulkan\graphics_vulkan.cpp w:\Hosebase\src\vulkan\graphics_vulkan_pipeline.cpp
) ELSE (
  SET link= %link% build_unit.obj graphics_vulkan.obj graphics_vulkan_pipeline.obj
)

IF NOT EXIST %out% mkdir %out%

pushd %out%\

IF NOT EXIST %name%\res mkdir %name%\res

CALL cl %src% %libs% %common_compiler_flags% %include_dirs% %defines% /link /out:%name%\%name%.exe /PDB:%name%.pdb /incremental:no /LTCG %link%

XCOPY w:\Hosebase\res\* %name%\ /I /Q /Y > NUL
XCOPY ..\res\* %name%\res /I /Q /Y /s > NUL

popd
popd

