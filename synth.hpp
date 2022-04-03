#pragma once

#include <iostream>
#include <cstdint>
#include <vector>
#include <cmath>
#include <algorithm>

const double musical_notes[] = {
/*    C       C#      D       Eb      E       F       F#      G       G#      A       Bb      B   */
/*0*/ 16.35,  17.32,  18.35,  19.45,  20.60,  21.83,  23.12,  24.50,  25.96,  27.50,  29.14,  30.87,
/*1*/ 32.70,  34.65,  36.71,  38.89,  41.20,  43.65,  46.25,  49.00,  51.91,  55.00,  58.27,  61.74,
/*2*/ 65.41,  69.30,  73.42,  77.78,  82.41,  87.31,  92.50,  98.00,  103.8,  110.0,  116.5,  123.5,
/*3*/ 130.8,  138.6,  146.8,  155.6,  164.8,  174.6,  185.0,  196.0,  207.7,  220.0,  233.1,  246.9,
/*4*/ 261.6,  277.2,  293.7,  311.1,  329.6,  349.2,  370.0,  392.0,  415.3,  440.0,  466.2,  493.9,
/*5*/ 523.3,  554.4,  587.3,  622.3,  659.3,  698.5,  740.0,  784.0,  830.6,  880.0,  932.3,  987.8,
/*6*/ 1047,   1109,   1175,   1245,   1319,   1397,   1480,   1568,   1661,   1760,   1865,   1976,
/*7*/ 2093,   2217,   2349,   2489,   2637,   2794,   2960,   3136,   3322,   3520,   3729,   3951,
/*8*/ 4186,   4435,   4699,   4978,   5274,   5588,   5920,   6272,   6645,   7040,   7459,   7902
};

// #define C   0
// #define CS  1
// #define D   2
// #define DS  3
// #define E   4
// #define F   5
// #define FS  6
// #define G   7
// #define GS  8
// #define A   9
// #define AS  10
// #define B   11

inline double get_note_frequency(int note, int octave) {
    return musical_notes[(octave * 12) + note];
}

const int sample_rate = 96000;

class source_t {
public:
    virtual int16_t get_sample(double) = 0;
};

struct sine_t : public source_t {
    double a, f;

    int16_t get_sample(double t) override {
        double sample = std::sin(t * f * M_PI / sample_rate);

        return sample * (a * 0x7fff);
    }
};

struct square_t : public source_t {
    double a, f;

    int16_t get_sample(double t) override {
        double sample = std::sin(t * f * M_PI / sample_rate);

        return sample > 0.0 ? (a * 0x7fff) : sample < 0.0 ? ((-a) * 0x7fff) : 0.0;
    }
};

struct saw_t : public source_t {
    double a, f;

    int16_t get_sample(double t) override {
         double sample = -((2 * a) / M_PI) * std::atan(1 / std::tan((t * M_PI) / f));

         return sample * (a * 0x7fff);
    }
};

struct fm_t : public source_t {
    struct fm_operator_t {
        int base_note, semi_offset, octave;
        double fine_offset;

        double a, f;

        enum adsr_state_t : int {
            AS_NONE,
            AS_ATTACK,
            AS_DECAY,
            AS_SUSTAIN,
            AS_RELEASE,
            AS_DONE
        };

        struct adsr_t {
            int m_samples = 0;
            double m_step = 0.0;

            adsr_state_t state = AS_NONE;

            double a = 0.0,
                   d = 0.0,
                   s = 0.0,
                   r = 0.0;

            double peak_level = 0.0,
                   sustain_level = 0.0,
                   base_level = 0.0;

            bool enabled = false;
        } adsr;

        bool enabled = false;

        double get_sample(double t, bool carrier = false, double fm = 0.0) {
            if (adsr.enabled) {
                if (!adsr.m_samples) {
                    switch (adsr.state) {
                        case AS_NONE: {
                            adsr.state = AS_ATTACK;

                            a = adsr.base_level;

                            adsr.m_samples = sample_rate * (adsr.a / 1000.0);
                            adsr.m_step    = (adsr.peak_level - adsr.base_level) / (double)adsr.m_samples;
                        } break;
                        case AS_ATTACK: {
                            adsr.state = AS_DECAY;

                            a = adsr.peak_level;

                            adsr.m_samples = sample_rate * (adsr.d / 1000.0);
                            adsr.m_step    = (adsr.sustain_level - adsr.peak_level) / (double)adsr.m_samples;
                        } break;
                        case AS_DECAY: {
                            adsr.state = AS_SUSTAIN;

                            a = adsr.sustain_level;

                            adsr.m_samples = sample_rate * (adsr.s / 1000.0);
                            adsr.m_step    = 0;
                        } break;
                        case AS_SUSTAIN: {
                            adsr.state = AS_RELEASE;

                            adsr.m_samples = sample_rate * (adsr.r / 1000.0);
                            adsr.m_step    = (adsr.base_level - adsr.sustain_level) / (double)adsr.m_samples;

                            a = adsr.sustain_level;
                        } break;
                        case AS_RELEASE: {
                            adsr.state = AS_DONE;

                            a = 0.0;

                            adsr.m_samples = 0;
                            adsr.m_step = 0;
                        } break;
                    }
                }

                a += adsr.m_step;

                if (adsr.m_samples) adsr.m_samples--;
            }

            double phase = carrier ? fm : 0.0;
            double amp = carrier ? a : 1.0;

            return enabled ? (std::sin(phase + t * f * M_PI / sample_rate) * amp) : 0.0;
        }
    };

    fm_operator_t operators[4];

    std::vector <int> notes;

    void push_note(int note, int octave) {
        notes.push_back(note + (octave * 12));

        operators[0].adsr.state = fm_operator_t::AS_NONE;
        operators[1].adsr.state = fm_operator_t::AS_NONE;
        operators[2].adsr.state = fm_operator_t::AS_NONE;
        operators[3].adsr.state = fm_operator_t::AS_NONE;
        
        operators[0].adsr.m_samples = 0;
        operators[1].adsr.m_samples = 0;
        operators[2].adsr.m_samples = 0;
        operators[3].adsr.m_samples = 0;
    }

    void release_note(int note, int octave) {
        auto it = std::find(notes.begin(), notes.end(), note + (octave * 12));

        notes.erase(it);
    }

    int16_t get_sample(double t) override {
        double v[4];

        double sample = 0.0;

        for (int i = 0; i < notes.size(); i++) {
            operators[0].f = get_note_frequency(notes[i] + operators[0].semi_offset, operators[0].octave) + operators[0].fine_offset;
            operators[1].f = get_note_frequency(notes[i] + operators[1].semi_offset, operators[1].octave) + operators[1].fine_offset;
            operators[2].f = get_note_frequency(notes[i] + operators[2].semi_offset, operators[2].octave) + operators[2].fine_offset;
            operators[3].f = get_note_frequency(notes[i] + operators[3].semi_offset, operators[3].octave) + operators[3].fine_offset;

            v[0] = operators[0].get_sample(t);
            v[1] = operators[1].get_sample(t, true, v[0]);
            v[2] = operators[2].get_sample(t, true, v[1]);
            v[3] = operators[3].get_sample(t, true, v[2]);

            sample += v[3];
        }

        return (sample / notes.size()) * 0x7fff;
    }
};

namespace synthesizer {
    double t = 0.0;

    std::vector <source_t*> sources;

    int16_t get_sample() {
        if (!sources.size()) return 0.0;

        double sample = 0.0;

        for (source_t* src : sources) {
            sample += src->get_sample(t);
        }

        t += 1.0;

        return sample / sources.size();
    }
}

void audio_request_callback(void* userdata, uint8_t* stream, int len) {
    for (int i = 0; i < (len >> 1); i++) {
        reinterpret_cast<int16_t*>(stream)[i] = synthesizer::get_sample();
    }
}


