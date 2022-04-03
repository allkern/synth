
#include "SDL.h"
#include "SDL_audio.h"

#include "synth.hpp"

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
        "obtained.freq     = " << obtained.freq << std::endl <<
        "obtained.format   = " << obtained.format << std::endl <<
        "obtained.channels = " << obtained.channels << std::endl <<
        "obtained.samples  = " << obtained.samples << std::endl <<
        "obtained.callback = " << obtained.callback << std::endl <<
        "obtained.userdata = " << obtained.userdata << std::endl;

    // start play audio
    SDL_PauseAudioDevice(dev, 0);
}

#undef main

int main() {
    SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    SDL_Window* window = SDL_CreateWindow(
        "Synth",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        500, 500,
        SDL_WINDOW_VULKAN
    );

    init_audio();

    bool open = true;

    while (open) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT: { open = false; } break;
                case SDL_KEYDOWN: {
                    if (event.key.repeat) break;

                    switch (event.key.keysym.sym) {
                        case SDLK_a: {
                            saw_t* s = new saw_t();

                            s->f = C5;
                            s->a = 1.0;

                            synthesizer::sources.push_back(s);
                        } break;
                        
                        case SDLK_w: {
                            square_t* s = new square_t();

                            s->f = CS5;
                            s->a = 1.0;

                            synthesizer::sources.push_back(s);
                        } break;

                        case SDLK_s: {
                            square_t* s = new square_t();

                            s->f = D5;
                            s->a = 1.0;

                            synthesizer::sources.push_back(s);
                        } break;
                        case SDLK_d: {
                            square_t* s = new square_t();

                            s->f = E5;
                            s->a = 1.0;

                            synthesizer::sources.push_back(s);
                        } break;
                        case SDLK_f: {
                            square_t* s = new square_t();

                            s->f = F5;
                            s->a = 1.0;

                            synthesizer::sources.push_back(s);
                        } break;
                        case SDLK_g: {
                            square_t* s = new square_t();

                            s->f = G5;
                            s->a = 1.0;

                            synthesizer::sources.push_back(s);
                        } break;
                        case SDLK_h: {
                            square_t* s = new square_t();

                            s->f = A5;
                            s->a = 1.0;

                            synthesizer::sources.push_back(s);
                        } break;
                        case SDLK_j: {
                            square_t* s = new square_t();

                            s->f = B5;
                            s->a = 1.0;

                            synthesizer::sources.push_back(s);
                        } break;
                        case SDLK_k: {
                            square_t* s = new square_t();

                            s->f = C6;
                            s->a = 1.0;

                            synthesizer::sources.push_back(s);
                        } break;
                        default: break;
                    }
                } break;
                case SDL_KEYUP: {
                    synthesizer::sources.erase(std::end(synthesizer::sources) - 1);
                } break;
            }
        }
    }
}