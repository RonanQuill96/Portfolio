@echo off

set GLSL_COMPILER=%VULKAN_SDK%/Bin32/glslc.exe
set COMPILE_OPTIONS=-g -o

:: Remove old .spv file
for /r %%i in (*.spv) do del "%%i"

:: Compile shaders
setlocal enabledelayedexpansion
for /r %%i in (*.vert *.frag *.comp *.geom) do (
    %GLSL_COMPILER%  "%%i" %COMPILE_OPTIONS%  "%%i.spv"
    if !errorlevel! neq 0 exit /b !errorlevel!
)