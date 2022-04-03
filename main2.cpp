#include "SDL.h"
#include "SDL_audio.h"

#include "synth.hpp"

#include <vector>

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"

#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
// About Desktop OpenGL function loaders:
//  Modern desktop OpenGL doesn't have a standard portable header file to load OpenGL function pointers.
//  Helper libraries are often used for this purpose! Here we are supporting a few common ones (gl3w, glew, glad).
//  You may use another loader/header of your choice (glext, glLoadGen, etc.), or chose to manually implement your own.
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
#include <GL/gl3w.h>            // Initialize with gl3wInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
#include <GL/glew.h>            // Initialize with glewInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
#include <glad/glad.h>          // Initialize with gladLoadGL()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD2)
#include <glad/gl.h>            // Initialize with gladLoadGL(...) or gladLoaderLoadGL()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING2)
#define GLFW_INCLUDE_NONE       // GLFW including OpenGL headers causes ambiguity or multiple definition errors.
#include <glbinding/Binding.h>  // Initialize with glbinding::Binding::initialize()
#include <glbinding/gl/gl.h>
using namespace gl;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING3)
#define GLFW_INCLUDE_NONE       // GLFW including OpenGL headers causes ambiguity or multiple definition errors.
#include <glbinding/glbinding.h>// Initialize with glbinding::initialize()
#include <glbinding/gl/gl.h>
using namespace gl;
#else
#include IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#endif

void init_audio() {
    SDL_AudioSpec desired;

    desired.freq     = sample_rate;
    desired.format   = AUDIO_S16SYS;
    desired.channels = 1;
    desired.samples  = 255;
    desired.callback = audio_request_callback;
    desired.userdata = nullptr;

    SDL_AudioSpec obtained;

    // you might want to look for errors here
    SDL_AudioDeviceID dev = SDL_OpenAudioDevice(
        nullptr,
        0,
        &desired, &obtained,
        SDL_AUDIO_ALLOW_FREQUENCY_CHANGE
    );

    std::cout <<
        "obtained.freq     = " << (int)obtained.freq << std::endl <<
        "obtained.format   = " << (int)obtained.format << std::endl <<
        "obtained.channels = " << (int)obtained.channels << std::endl <<
        "obtained.samples  = " << (int)obtained.samples << std::endl <<
        "obtained.callback = " <<      obtained.callback << std::endl <<
        "obtained.userdata = " <<      obtained.userdata << std::endl;

    // start play audio
    SDL_PauseAudioDevice(dev, 0);
}

#undef main

std::vector <uint32_t> buf;

int counter = 10;

int main() {
    SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    SDL_Window* window = SDL_CreateWindow(
        "Synth",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        1280, 720,
        SDL_WINDOW_OPENGL
    );

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_PRESENTVSYNC
    );

    SDL_Texture* texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,
        500, 500
    );

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);

    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
    int err = gl3wInit();
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
    bool err = glewInit() != GLEW_OK;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
    bool err = gladLoadGL() == 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD2)
    bool err = gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress) == 0; // glad2 recommend using the windowing library loader instead of the (optionally) bundled one.
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING2)
    bool err = false;
    glbinding::Binding::initialize();
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING3)
    bool err = false;
    glbinding::initialize([](const char* name) { return (glbinding::ProcAddress)SDL_GL_GetProcAddress(name); });
#else
    bool err = false; // If you use IMGUI_IMPL_OPENGL_LOADER_CUSTOM, your loader is likely to requires some form of initialization.
#endif
    if (err) {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return 1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

// Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImGuiIO& cio = ImGui::GetIO();
            
    cio.Fonts->Clear();
    cio.Fonts->AddFontDefault();

    cio.Fonts->AddFontFromFileTTF("ubuntu-mono.ttf", 16.0f);

    buf.resize(500 * 500);

    init_audio();

    bool open = true;

    int carrier_octave = 5,
        modulator_octave = 5;
    
    double fm_amount = 15.0;

    bool lctrl = false;

    double min_freq = 10.0,
           zero = 0.0,
           minus_100 = -100.0,
           plus_100 = 100.0,
           max_freq = (double)sample_rate / 2.0;

    fm_t dummy_fm;

    int operator_octaves[4] = { 4 };

    bool link_peak_sustain = false;

    fm_t* fm = new fm_t;

    synthesizer::sources.push_back(fm);

    while (open) {
        SDL_Event event;

        SDL_RenderClear(renderer);
        
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);

        ImGui::NewFrame();

        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);

        ImGui::ShowDemoWindow();

        if (ImGui::Begin("FM Channel 0")) {
            for (int i = 0; i < 4; i++) {
                char label[50];

                if (i != 3) {
                    sprintf(label, "Operator %u", i + 1);
                } else {
                    sprintf(label, "Main");
                }

                if (ImGui::TreeNode(label, label, i + 1)) {
                    ImGui::Checkbox("Enabled", &fm->operators[i].enabled);

                    ImGui::SliderInt("Octave", &fm->operators[i].octave, 0, 8);
                    ImGui::SliderInt("Semitone", &fm->operators[i].semi_offset, 0, 11);
                    ImGui::DragScalar(
                        "Fine",
                        ImGuiDataType_Double,
                        &fm->operators[i].fine_offset,
                        0.1f,
                        &minus_100, &plus_100,
                        "%.3f"
                    );

                    ImGui::Checkbox("ADSR Enabled", &fm->operators[i].adsr.enabled);

                    if (!fm->operators[i].adsr.enabled)
                        ImGui::DragScalar(
                            "Amplitude",
                            ImGuiDataType_Double,
                            &fm->operators[i].a,
                            0.1f,
                            &zero, NULL,
                            "%.3f"
                        );

                    if (fm->operators[i].adsr.enabled) {
                        float adsr[4];

                        adsr[0] = (float)fm->operators[i].adsr.a;
                        adsr[1] = (float)fm->operators[i].adsr.d;
                        adsr[2] = (float)fm->operators[i].adsr.s;
                        adsr[3] = (float)fm->operators[i].adsr.r;

                        ImGui::DragFloat4("ADSR Control", adsr, 0.1f, 0.0f, 1000000.0f);

                        fm->operators[i].adsr.a = (double)adsr[0];
                        fm->operators[i].adsr.d = (double)adsr[1];
                        fm->operators[i].adsr.s = (double)adsr[2];
                        fm->operators[i].adsr.r = (double)adsr[3];

                        ImGui::Checkbox("Link Peak Sustain", &link_peak_sustain);

                        if (link_peak_sustain) {
                            ImGui::DragScalar(
                                "Peak/Sustain",
                                ImGuiDataType_Double,
                                &fm->operators[i].adsr.peak_level,
                                0.1f,
                                &zero, NULL,
                                "%.3f"
                            );

                            fm->operators[i].adsr.sustain_level = fm->operators[i].adsr.peak_level;
                        } else {
                            ImGui::DragScalar(
                                "Peak",
                                ImGuiDataType_Double,
                                &fm->operators[i].adsr.peak_level,
                                0.1f,
                                &zero, NULL,
                                "%.3f"
                            );

                            ImGui::DragScalar(
                                "Sustain",
                                ImGuiDataType_Double,
                                &fm->operators[i].adsr.sustain_level,
                                0.1f,
                                &zero, NULL,
                                "%.3f"
                            );
                        }

                        ImGui::DragScalar(
                            "Base",
                            ImGuiDataType_Double,
                            &fm->operators[i].adsr.base_level,
                            0.1f,
                            &zero, NULL,
                            "%.3f"
                        );

                        const char* items[] = {
                            "Operator 1",
                            "Operator 2",
                            "Operator 3",
                            "Main"
                        };

                        static int item_current = 0;

                        ImGui::Combo("Copy from", &item_current, items, 4);

                        if (ImGui::Button("Copy")) {
                            fm->operators[i].adsr = fm->operators[item_current].adsr;
                        }

                        ImGui::SameLine();

                        if (ImGui::Button("Clear")) {
                            fm->operators[i].adsr = { .enabled = true };
                        }
                    }

                    ImGui::TreePop();
                }
            }

            ImGui::End();
        }

        ImGui::PopFont();

        ImGui::Render();

        glClearColor(0.1, 0.1, 0.1, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);

        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);

            switch (event.type) {
                case SDL_QUIT: { open = false; } break;
                case SDL_KEYDOWN: {
                    if (event.key.repeat) break;

                    switch (event.key.keysym.sym) {
                        case SDLK_a        : { fm->push_note(0, 0); } break;
                        case SDLK_w        : { fm->push_note(1, 0); } break;
                        case SDLK_s        : { fm->push_note(2, 0); } break;
                        case SDLK_e        : { fm->push_note(3, 0); } break;
                        case SDLK_d        : { fm->push_note(4, 0); } break;
                        case SDLK_f        : { fm->push_note(5, 0); } break;
                        case SDLK_t        : { fm->push_note(6, 0); } break;
                        case SDLK_g        : { fm->push_note(7, 0); } break;
                        case SDLK_y        : { fm->push_note(8, 0); } break;
                        case SDLK_h        : { fm->push_note(9, 0); } break;
                        case SDLK_u        : { fm->push_note(10, 0); } break;
                        case SDLK_j        : { fm->push_note(11, 0); } break;
                        case SDLK_k        : { fm->push_note(12, 0); } break;
                        case SDLK_o        : { fm->push_note(13, 0); } break;
                        case SDLK_l        : { fm->push_note(14, 0); } break;
                        case SDLK_p        : { fm->push_note(15, 0); } break;
                        case SDLK_SEMICOLON: { fm->push_note(16, 0); } break;
                        case SDLK_QUOTE    : { fm->push_note(17, 0); } break;
                    }
                } break;
                case SDL_KEYUP: {
                    switch (event.key.keysym.sym) {
                        case SDLK_a        : { fm->release_note(0, 0); } break;
                        case SDLK_w        : { fm->release_note(1, 0); } break;
                        case SDLK_s        : { fm->release_note(2, 0); } break;
                        case SDLK_e        : { fm->release_note(3, 0); } break;
                        case SDLK_d        : { fm->release_note(4, 0); } break;
                        case SDLK_f        : { fm->release_note(5, 0); } break;
                        case SDLK_t        : { fm->release_note(6, 0); } break;
                        case SDLK_g        : { fm->release_note(7, 0); } break;
                        case SDLK_y        : { fm->release_note(8, 0); } break;
                        case SDLK_h        : { fm->release_note(9, 0); } break;
                        case SDLK_u        : { fm->release_note(10, 0); } break;
                        case SDLK_j        : { fm->release_note(11, 0); } break;
                        case SDLK_k        : { fm->release_note(12, 0); } break;
                        case SDLK_o        : { fm->release_note(13, 0); } break;
                        case SDLK_l        : { fm->release_note(14, 0); } break;
                        case SDLK_p        : { fm->release_note(15, 0); } break;
                        case SDLK_SEMICOLON: { fm->release_note(16, 0); } break;
                        case SDLK_QUOTE    : { fm->release_note(17, 0); } break;
                    }
                } break;
            }
        }
    }
}