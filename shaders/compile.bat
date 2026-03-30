@echo off
pushd "%~dp0"
"%VULKAN_SDK%\Bin\glslc.exe" shader.vert -o vert.spv
if errorlevel 1 goto :end
"%VULKAN_SDK%\Bin\glslc.exe" shader.frag -o frag.spv
"%VULKAN_SDK%\Bin\glslc.exe" pathtrace.comp -o pathtrace.comp.spv
:end
popd
