@echo off
setlocal
set GLSLC=C:\VulkanSDK\1.3.296.0\Bin\glslc.exe
set SHADER_DIR=%~dp0
set SHADER_DIR=%SHADER_DIR:~0,-1%

cd /d "%SHADER_DIR%"

"%GLSLC%" -I "%SHADER_DIR%" triangle.vert -o triangle.vert.spv
"%GLSLC%" -I "%SHADER_DIR%" triangle.frag -o triangle.frag.spv
"%GLSLC%" -I "%SHADER_DIR%" textured.vert -o textured.vert.spv
"%GLSLC%" -I "%SHADER_DIR%" textured.frag -o textured.frag.spv
"%GLSLC%" -I "%SHADER_DIR%" lighting_composite.vert -o lighting_composite.vert.spv
"%GLSLC%" -I "%SHADER_DIR%" lighting_composite.frag -o lighting_composite.frag.spv

endlocal