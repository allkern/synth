$SDL2_DIR   = "C:\Users\Lisandro\Desktop\Development\Libraries\SDL2"
$IMPLOT_DIR = "C:\Users\Lisandro\Desktop\Development\Libraries\implot"
$IMGUI_DIR  = "C:\Users\Lisandro\Desktop\Development\Libraries\imgui-1.83"

c++ -I"`"$($IMGUI_DIR)`"" `
    -I"`"$($IMGUI_DIR)\backends`"" `
    -I"`"$($IMGUI_DIR)\examples\libs\gl3w`"" `
    -I"`"$($SDL2_DIR)\include\SDL2`"" `
    -I"`"$($SDL2_DIR)\include`"" `
    -I"`"$($IMPLOT_DIR)`"" `
    "*.o" `
    main2.cpp `
    -o main.exe `
    -L"`"$($SDL2_DIR)\lib\x64`"" `
    -DIMGUI_IMPL_OPENGL_LOADER_GL3W `
    -limm32 -mbmi2 -lSDL2main -lSDL2 -lopengl32 -g