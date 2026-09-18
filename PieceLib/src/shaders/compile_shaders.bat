@echo off
setlocal
set GLSLC=C:\VulkanSDK\1.3.296.0\Bin\glslc.exe
set SHADER_DIR=%~dp0
set SHADER_DIR=%SHADER_DIR:~0,-1%

cd /d "%SHADER_DIR%"

"%GLSLC%" -I "%SHADER_DIR%" textured.vert -o textured.vert.spv
"%GLSLC%" -I "%SHADER_DIR%" textured.frag -o textured.frag.spv
"%GLSLC%" -I "%SHADER_DIR%" lighting_composite.vert -o lighting_composite.vert.spv
"%GLSLC%" -I "%SHADER_DIR%" lighting_composite.frag -o lighting_composite.frag.spv
"%GLSLC%" -I "%SHADER_DIR%" lighting_composite_msaa.frag -o lighting_composite_msaa.frag.spv
"%GLSLC%" -I "%SHADER_DIR%" present.frag -o present.frag.spv
"%GLSLC%" -I "%SHADER_DIR%" bloom_extract.frag -o bloom_extract.frag.spv
"%GLSLC%" -I "%SHADER_DIR%" bloom_blur.frag -o bloom_blur.frag.spv
"%GLSLC%" -I "%SHADER_DIR%" bloom_blur_vertical.frag -o bloom_blur_vertical.frag.spv
"%GLSLC%" -I "%SHADER_DIR%" billboard.vert -o billboard.vert.spv
"%GLSLC%" -I "%SHADER_DIR%" billboard.frag -o billboard.frag.spv

endlocal