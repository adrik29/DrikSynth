#pragma once
// Blocos de DSP do DrikSynth, sem dependencia do JUCE (da pra testar isolado).
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace drik
{
constexpr double kPi = 3.14159265358979323846;

inline float midiToHz (float note)   { return 440.0f * std::pow (2.0f, (note - 69.0f) / 12.0f); }

//==============================================================================
// Osciladores com polyBLEP (arredonda os degraus da onda para reduzir aliasing).
inline float polyBlep (float t, float dt)
{
    if (t < dt)         { t /= dt;               return t + t - t * t - 1.0f; }
    if (t > 1.0f - dt)  { t = (t - 1.0f) / dt;   return t * t + t + t + 1.0f; }
    return 0.0f;
}

enum Wave { waveSaw = 0, wavePulse, waveTriangle, waveSine };

/** Valor da onda na fase t (0..1), com incremento dt por sample. */
inline float oscValue (int wave, float t, float dt, float pw)
{
    switch (wave)
    {
        case waveSaw:
            return 2.0f * t - 1.0f - polyBlep (t, dt);

        case wavePulse:
        {
            float t2 = t + 1.0f - pw;
            if (t2 >= 1.0f) t2 -= 1.0f;
            float v = (t < pw ? 1.0f : -1.0f) + polyBlep (t, dt) - polyBlep (t2, dt);
            return v - (2.0f * pw - 1.0f);          // tira o componente continuo
        }

        case waveTriangle:
            return 2.0f * std::fabs (2.0f * t - 1.0f) - 1.0f;

        default:
            return std::sin (2.0f * (float) kPi * t);
    }
}

//==============================================================================
/** Envelope ADSR com curvas exponenciais, como nos analogicos. */
class Envelope
{
public:
    void setSampleRate (double sr)   { sampleRate = sr; }

    void set (float attack, float decay, float sustainLevel, float release)
    {
        aCoef = coef (attack / 1.466f);     // mira em 1.3 e corta em 1.0: ataque quase reto
        dCoef = coef (decay / 4.0f);
        rCoef = coef (release / 4.0f);
        sustain = sustainLevel;
    }

    void noteOn()    { stage = attackStage; }
    void noteOff()   { if (stage != idle) stage = releaseStage; }
    void reset()     { stage = idle; level = 0.0f; }
    bool isActive() const   { return stage != idle; }
    float value() const     { return level; }

    float next()
    {
        switch (stage)
        {
            case attackStage:
                level += (1.3f - level) * aCoef;
                if (level >= 1.0f) { level = 1.0f; stage = decayStage; }
                break;
            case decayStage:
                level += (sustain - level) * dCoef;
                break;
            case releaseStage:
                level += (0.0f - level) * rCoef;
                if (level < 1.0e-4f) { level = 0.0f; stage = idle; }
                break;
            default:
                break;
        }
        return level;
    }

private:
    float coef (float tau) const
    {
        const double samples = std::max (1.0, (double) tau * sampleRate);
        return (float) (1.0 - std::exp (-1.0 / samples));
    }

    enum Stage { idle, attackStage, decayStage, releaseStage };
    Stage stage = idle;
    double sampleRate = 44100.0;
    float level = 0.0f, sustain = 1.0f, aCoef = 0.1f, dCoef = 0.001f, rCoef = 0.001f;
};

//==============================================================================
/** Filtro passa-baixa de 24 dB/oitava em escada (familia Moog/Roland), sem atraso na realimentacao. */
class LadderFilter
{
public:
    void reset()   { s[0] = s[1] = s[2] = s[3] = 0.0f; }

    /** resonance 0..1 (1 = beira da auto-oscilacao). */
    void set (float cutoffHz, float resonance, double sampleRate)
    {
        const float g = (float) std::tan (kPi * (double) cutoffHz / sampleRate);
        G = g / (1.0f + g);
        k = 3.9f * resonance;
        comp = 1.0f + 0.5f * k;            // compensa a perda de volume que a ressonancia causa
    }

    float process (float x)
    {
        const float oneMinusG = 1.0f - G;
        const float G2 = G * G;
        const float S = (G2 * G * s[0] + G2 * s[1] + G * s[2] + s[3]) * oneMinusG;
        float u = (x * comp - k * S) / (1.0f + k * G2 * G2);
        u = std::tanh (u);                 // saturacao suave na entrada, como no circuito

        for (auto& st : s)
        {
            const float v = (u - st) * G;
            const float y = v + st;
            st = y + v;
            u = y;
        }
        return u;
    }

private:
    float s[4] = { 0, 0, 0, 0 };
    float G = 0.5f, k = 0.0f, comp = 1.0f;
};

/** Filtro passa-baixa de 12 dB/oitava de variavel de estado (familia Oberheim). */
class StateVariableFilter
{
public:
    void reset()   { ic1 = ic2 = 0.0f; }

    void set (float cutoffHz, float resonance, double sampleRate)
    {
        const float g = (float) std::tan (kPi * (double) cutoffHz / sampleRate);
        const float q = 0.5f * std::pow (40.0f, resonance);      // 0.5 .. 20
        const float kk = 1.0f / q;
        a1 = 1.0f / (1.0f + g * (g + kk));
        a2 = g * a1;
        a3 = g * a2;
    }

    float process (float x)
    {
        const float v3 = x - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        return v2;
    }

private:
    float ic1 = 0.0f, ic2 = 0.0f, a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;
};

//==============================================================================
/** Chorus estereo com tres personalidades:
    1 e 2 = chorus de duas linhas em oposicao (familia Juno), lento e medio;
    3 = ensemble de tres linhas com modulacao lenta + rapida (string machines). */
class Chorus
{
public:
    void prepare (double sr)
    {
        sampleRate = sr;
        size = 1;
        while (size < (int) (sr * 0.03) + 8) size <<= 1;
        buffer.assign ((size_t) size, 0.0f);
        writePos = 0;
        slow = fast = 0.0;
        lp = 0.0f;
        lpCoef = (float) (1.0 - std::exp (-2.0 * kPi * 9000.0 / sr));
    }

    void process (float* left, float* right, int n, int mode, float mix)
    {
        if (mode <= 0 || mix <= 0.0f || buffer.empty())
        {
            // Continua alimentando a linha para nao estalar ao ligar.
            for (int i = 0; i < n; ++i) write (0.5f * (left[i] + right[i]));
            return;
        }

        const double slowRate = mode == 1 ? 0.513 : mode == 2 ? 0.863 : 0.6;
        const double fastRate = 6.0;
        const double slowInc = slowRate / sampleRate, fastInc = fastRate / sampleRate;
        const float ms = (float) (sampleRate * 0.001);

        for (int i = 0; i < n; ++i)
        {
            write (0.5f * (left[i] + right[i]));

            if (mode < 3)
            {
                const float tri = 4.0f * (float) std::fabs (slow - 0.5) - 1.0f;        // -1..1
                const float a = read ((3.5f + 1.85f * tri) * ms);
                const float b = read ((3.5f - 1.85f * tri) * ms);
                left[i]  = left[i]  * (1.0f - 0.3f * mix) + a * 0.9f * mix;
                right[i] = right[i] * (1.0f - 0.3f * mix) + b * 0.9f * mix;
            }
            else
            {
                float w[3];
                for (int k = 0; k < 3; ++k)
                {
                    const double ph = (double) k / 3.0;
                    const float d = 6.0f + 1.0f  * (float) std::sin (2.0 * kPi * (slow + ph))
                                         + 0.12f * (float) std::sin (2.0 * kPi * (fast + ph));
                    w[k] = read (d * ms);
                }
                left[i]  = left[i]  * (1.0f - mix) + (w[0] + 0.6f * w[2]) * 0.75f * mix;
                right[i] = right[i] * (1.0f - mix) + (w[1] + 0.6f * w[2]) * 0.75f * mix;
            }

            slow += slowInc; if (slow >= 1.0) slow -= 1.0;
            fast += fastInc; if (fast >= 1.0) fast -= 1.0;
        }
    }

private:
    void write (float x)
    {
        lp += (x - lp) * lpCoef;                    // as linhas de atraso analogicas cortam os agudos
        buffer[(size_t) writePos] = lp;
        writePos = (writePos + 1) & (size - 1);
    }

    float read (float delaySamples) const
    {
        const float pos = (float) writePos - 1.0f - delaySamples + (float) size;
        const int i0 = (int) pos;
        const float frac = pos - (float) i0;
        const float a = buffer[(size_t) (i0 & (size - 1))];
        const float b = buffer[(size_t) ((i0 + 1) & (size - 1))];
        return a + frac * (b - a);
    }

    std::vector<float> buffer;
    int size = 0, writePos = 0;
    double sampleRate = 44100.0, slow = 0.0, fast = 0.0;
    float lp = 0.0f, lpCoef = 1.0f;
};

/** Limitador suave na saida: nao mexe abaixo de 0.7 e arredonda acima disso. */
inline float softClip (float x)
{
    const float a = std::fabs (x);
    if (a <= 0.7f) return x;
    const float y = 0.7f + 0.3f * std::tanh ((a - 0.7f) / 0.3f);
    return x < 0.0f ? -y : y;
}

} // namespace drik
