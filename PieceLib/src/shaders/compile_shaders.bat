@echo off
setlocal
set GLSLC=C:\VulkanSDK\1.3.296.0\Bin\glslc.exe
set SHADER_DIR=%~dp0

cd /d "%SHADER_DIR%"

"%GLSLC%" triangle.vert -o triangle.vert.spv
"%GLSLC%" triangle.frag -o triangle.frag.spv
"%GLSLC%" textured.vert -o textured.vert.spv
"%GLSLC%" textured.frag -o textured.frag.spv

endlocal