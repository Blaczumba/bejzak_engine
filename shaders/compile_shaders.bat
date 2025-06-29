set "SCRIPT_DIR=%~dp0"

glslc.exe -fshader-stage=vertex "%SCRIPT_DIR%\shader_blinn_phong.vert.glsl" -O -o "%SCRIPT_DIR%\shader_blinn_phong.vert.spv"
glslc.exe -fshader-stage=tesscontrol "%SCRIPT_DIR%\shader_blinn_phong.tsc.glsl" -O -o "%SCRIPT_DIR%\shader_blinn_phong.tsc.spv"
glslc.exe -fshader-stage=tesseval "%SCRIPT_DIR%\shader_blinn_phong.tse.glsl" -O -o "%SCRIPT_DIR%\shader_blinn_phong.tse.spv"
glslc.exe -fshader-stage=fragment "%SCRIPT_DIR%\shader_blinn_phong.frag.glsl" -O -o "%SCRIPT_DIR%\shader_blinn_phong.frag.spv"

glslc.exe -fshader-stage=fragment "%SCRIPT_DIR%\offscreen.frag.glsl" -O -o "%SCRIPT_DIR%\off.frag.spv"

glslc.exe -fshader-stage=vertex "%SCRIPT_DIR%\skybox.vert.glsl" -O -o "%SCRIPT_DIR%\skybox.vert.spv"
glslc.exe -fshader-stage=fragment "%SCRIPT_DIR%\skybox.frag.glsl" -O -o "%SCRIPT_DIR%\skybox.frag.spv"

glslc.exe -fshader-stage=fragment "%SCRIPT_DIR%\skybox_offscreen.frag.glsl" -O -o "%SCRIPT_DIR%\skybox_offscreen.frag.spv"

glslc.exe -fshader-stage=vertex "%SCRIPT_DIR%\shadow.vert.glsl" -O -o "%SCRIPT_DIR%\shadow.vert.spv"
glslc.exe -fshader-stage=fragment "%SCRIPT_DIR%\shadow.frag.glsl" -O -o "%SCRIPT_DIR%\shadow.frag.spv"

glslc.exe -I "%SCRIPT_DIR%\bindless.glsl" -fshader-stage=vertex "%SCRIPT_DIR%\shader_pbr.vert.glsl" -O -o "%SCRIPT_DIR%\shader_pbr.vert.spv"
glslc.exe -fshader-stage=vertex "%SCRIPT_DIR%\shader_pbr_tesselation.vert.glsl" -O -o "%SCRIPT_DIR%\shader_pbr_tesselation.vert.spv"
glslc.exe -fshader-stage=tesscontrol "%SCRIPT_DIR%\shader_pbr_tesselation.tsc.glsl" -O -o "%SCRIPT_DIR%\shader_pbr_tesselation.tsc.spv"
glslc.exe -fshader-stage=tesseval "%SCRIPT_DIR%\shader_pbr_tesselation.tse.glsl" -O -o "%SCRIPT_DIR%\shader_pbr_tesselation.tse.spv"
glslc.exe -fshader-stage=frag "%SCRIPT_DIR%\shader_pbr_tesselation.frag.glsl" -O -o "%SCRIPT_DIR%\shader_pbr_tesselation.frag.spv"

glslc.exe -I "%SCRIPT_DIR%\bindless.glsl"  -fshader-stage=frag "%SCRIPT_DIR%\shader_pbr.frag.glsl" -O -o "%SCRIPT_DIR%\shader_pbr.frag.spv"

glslc.exe -fshader-stage=frag "%SCRIPT_DIR%\offscreen_shader_pbr.frag.glsl" -O -o "%SCRIPT_DIR%\offscreen_shader_pbr.frag.spv"