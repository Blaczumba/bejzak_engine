set "SCRIPT_DIR=%~dp0"

glslc.exe -I "%SCRIPT_DIR%\32bit_push_constants.glsl" -fshader-stage=vertex "%SCRIPT_DIR%\shader_blinn_phong.vert.glsl" -O -o "%SCRIPT_DIR%\shader_blinn_phong.vert.spv"
glslc.exe -fshader-stage=tesscontrol "%SCRIPT_DIR%\shader_blinn_phong.tsc.glsl" -O -o "%SCRIPT_DIR%\shader_blinn_phong.tsc.spv"
glslc.exe -I "%SCRIPT_DIR%\bindless.glsl" -I "%SCRIPT_DIR%\32bit_push_constants.glsl" -fshader-stage=tesseval "%SCRIPT_DIR%\shader_blinn_phong.tse.glsl" -O -o "%SCRIPT_DIR%\shader_blinn_phong.tse.spv"
glslc.exe -I "%SCRIPT_DIR%\bindless.glsl" -I "%SCRIPT_DIR%\32bit_push_constants.glsl" -fshader-stage=fragment "%SCRIPT_DIR%\shader_blinn_phong.frag.glsl" -O -o "%SCRIPT_DIR%\shader_blinn_phong.frag.spv"

glslc.exe -fshader-stage=vertex "%SCRIPT_DIR%\skybox.vert.glsl" -O -o "%SCRIPT_DIR%\skybox.vert.spv"
glslc.exe -I "%SCRIPT_DIR%\bindless.glsl" -fshader-stage=fragment "%SCRIPT_DIR%\skybox.frag.glsl" -O -o "%SCRIPT_DIR%\skybox.frag.spv"

glslc.exe -fshader-stage=vertex "%SCRIPT_DIR%\shadow.vert.glsl" -O -o "%SCRIPT_DIR%\shadow.vert.spv"
glslc.exe -fshader-stage=fragment "%SCRIPT_DIR%\shadow.frag.glsl" -O -o "%SCRIPT_DIR%\shadow.frag.spv"

glslc.exe -I "%SCRIPT_DIR%\bindless.glsl" -I "%SCRIPT_DIR%\32bit_push_constants.glsl" -I "%SCRIPT_DIR%\16bit_push_constants.glsl" -fshader-stage=vertex "%SCRIPT_DIR%\env_mapping_phong.vert.glsl" -O -o "%SCRIPT_DIR%\env_mapping_phong.vert.spv"
glslc.exe -I "%SCRIPT_DIR%\bindless.glsl" -I "%SCRIPT_DIR%\32bit_push_constants.glsl" -I "%SCRIPT_DIR%\16bit_push_constants.glsl" -fshader-stage=frag "%SCRIPT_DIR%\env_mapping_phong.frag.glsl" -O -o "%SCRIPT_DIR%\env_mapping_phong.frag.spv"

glslc.exe -I "%SCRIPT_DIR%\bindless.glsl" -I "%SCRIPT_DIR%\32bit_push_constants.glsl" -I "%SCRIPT_DIR%\16bit_push_constants.glsl" -fshader-stage=vertex "%SCRIPT_DIR%\env_mapping_phong_multiview.vert.glsl" -O -o "%SCRIPT_DIR%\env_mapping_phong_multiview.vert.spv"
glslc.exe -I "%SCRIPT_DIR%\bindless.glsl" -I "%SCRIPT_DIR%\32bit_push_constants.glsl" -I "%SCRIPT_DIR%\16bit_push_constants.glsl" -fshader-stage=frag "%SCRIPT_DIR%\env_mapping_phong_multiview.frag.glsl" -O -o "%SCRIPT_DIR%\env_mapping_phong_multiview.frag.spv"

glslc.exe -I "%SCRIPT_DIR%\bindless.glsl" -I "%SCRIPT_DIR%\32bit_push_constants.glsl" -I "%SCRIPT_DIR%\16bit_push_constants.glsl" -fshader-stage=vertex "%SCRIPT_DIR%\pbr_env_mapping.vert.glsl" -O -o "%SCRIPT_DIR%\pbr_env_mapping.vert.spv"

glslc.exe -I "%SCRIPT_DIR%\bindless.glsl" -I "%SCRIPT_DIR%\32bit_push_constants.glsl" -I "%SCRIPT_DIR%\16bit_push_constants.glsl" -fshader-stage=vertex "%SCRIPT_DIR%\shader_pbr.vert.glsl" -O -o "%SCRIPT_DIR%\shader_pbr.vert.spv"
glslc.exe -I "%SCRIPT_DIR%\bindless.glsl" -I "%SCRIPT_DIR%\32bit_push_constants.glsl" -I "%SCRIPT_DIR%\16bit_push_constants.glsl" -fshader-stage=vertex "%SCRIPT_DIR%\shader_pbr_multiview.vert.glsl" -O -o "%SCRIPT_DIR%\shader_pbr_multiview.vert.spv"
glslc.exe -I "%SCRIPT_DIR%\32bit_push_constants.glsl" -fshader-stage=vertex "%SCRIPT_DIR%\shader_pbr_tesselation.vert.glsl" -O -o "%SCRIPT_DIR%\shader_pbr_tesselation.vert.spv"
glslc.exe -fshader-stage=tesscontrol "%SCRIPT_DIR%\shader_pbr_tesselation.tsc.glsl" -O -o "%SCRIPT_DIR%\shader_pbr_tesselation.tsc.spv"
glslc.exe -I "%SCRIPT_DIR%\bindless.glsl" -I "%SCRIPT_DIR%\32bit_push_constants.glsl" -fshader-stage=tesseval "%SCRIPT_DIR%\shader_pbr_tesselation.tse.glsl" -O -o "%SCRIPT_DIR%\shader_pbr_tesselation.tse.spv"
glslc.exe -I "%SCRIPT_DIR%\bindless.glsl" -I "%SCRIPT_DIR%\32bit_push_constants.glsl" -fshader-stage=frag "%SCRIPT_DIR%\shader_pbr_tesselation.frag.glsl" -O -o "%SCRIPT_DIR%\shader_pbr_tesselation.frag.spv"

glslc.exe -I "%SCRIPT_DIR%\bindless.glsl" -I "%SCRIPT_DIR%\32bit_push_constants.glsl" -I "%SCRIPT_DIR%\16bit_push_constants.glsl" -fshader-stage=frag "%SCRIPT_DIR%\shader_pbr.frag.glsl" -O -o "%SCRIPT_DIR%\shader_pbr.frag.spv"

glslc.exe -fshader-stage=compute "%SCRIPT_DIR%\fov_fragment_shading_rate.comp.glsl" -O -o "%SCRIPT_DIR%\fov_fragment_shading_rate.comp.spv"
glslc.exe -fshader-stage=compute "%SCRIPT_DIR%\fov_fragment_density_map.comp.glsl" -O -o "%SCRIPT_DIR%\fov_fragment_density_map.comp.spv"