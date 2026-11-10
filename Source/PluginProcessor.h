#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "Dsp.h"

//==============================================================================
/** Valores dos parametros lidos pela thread de audio, mais o estado comum a todas as vozes. */
struct SharedState
{
    std::atomic<float>* p (const char* id) const   { return apvts->getRawParameterValue (id); }

    juce::AudioProcessorValueTreeState* apvts = nullptr;

    std::atomic<float> *o1wave, *o1oct, *o2wave, *o2semi, *o2fine, *sync, *pw, *pwm,
                       *mix1, *mix2, *sub, *noise, *unison, *detune, *spread, *glide, *mono,
                       *cutoff, *reso, *fmode, *envamt, *keytrack, *lfocut,
                       *fa, *fd, *fs, *fr, *aa, *ad, *as, *ar, *lfopitch, *velsens;

    const float* lfo = nullptr;     // valor do LFO (-1..1) para cada sample do bloco atual
    float modWheel = 0.0f;          // 0..1
    float lastNote = -1.0f;         // ultima nota tocada, para o glide

    void bind (juce::AudioProcessorValueTreeState&);
};

struct SynthSound final : public juce::SynthesiserSound
{
    bool appliesToNote (int) override      { return true; }
    bool appliesToChannel (int) override   { return true; }
};

//==============================================================================
class SynthVoice final : public juce::SynthesiserVoice
{
public:
    SynthVoice (SharedState& s, int index) : shared (s), voiceIndex (index) {}

    bool canPlaySound (juce::SynthesiserSound* s) override   { return dynamic_cast<SynthSound*> (s) != nullptr; }
    void setCurrentPlaybackSampleRate (double) override;
    void startNote (int midiNote, float velocity, juce::SynthesiserSound*, int pitchWheel) override;
    void stopNote (float, bool allowTailOff) override;
    void pitchWheelMoved (int) override;
    void controllerMoved (int, int) override {}
    void renderNextBlock (juce::AudioBuffer<float>&, int startSample, int numSamples) override;

private:
    static constexpr int maxUnison = 4;

    SharedState& shared;
    const int voiceIndex;

    drik::Envelope ampEnv, filterEnv;
    drik::LadderFilter ladder[2];
    drik::StateVariableFilter svf[2];

    float phase1[maxUnison] {}, phase2[maxUnison] {}, phaseSub[maxUnison] {};
    float ratio[maxUnison] {}, gainL[maxUnison] {}, gainR[maxUnison] {};
    int numUnison = 1;

    float targetNote = 60.0f, currentNote = 60.0f, bendSemis = 0.0f, velocityGain = 1.0f;
    juce::Random random;
};

/** Sintetizador com modo mono: ao tocar uma nota nova, solta as anteriores. */
class SynthEngine final : public juce::Synthesiser
{
public:
    bool mono = false;

    void noteOn (int channel, int note, float velocity) override
    {
        if (mono)
            for (auto* v : voices)
                if (v->isVoiceActive())
                    stopVoice (v, 1.0f, true);

        juce::Synthesiser::noteOn (channel, note, velocity);
    }
};

//==============================================================================
class DrikSynthProcessor final : public juce::AudioProcessor
{
public:
    DrikSynthProcessor();
    ~DrikSynthProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                          { return true; }

    const juce::String getName() const override              { return "DrikSynth"; }
    bool acceptsMidi() const override                        { return true; }
    bool producesMidi() const override                       { return false; }
    bool isMidiEffect() const override                       { return false; }
    double getTailLengthSeconds() const override             { return 8.0; }

    int getNumPrograms() override                            { return 1; }
    int getCurrentProgram() override                         { return 0; }
    void setCurrentProgram (int) override                    {}
    const juce::String getProgramName (int) override         { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    /** Aplica um preset da biblioteca (indice em drik::presets()). */
    void applyPreset (int index);

    /** Presets do usuario: um arquivo por som, em Documentos/DrikSynth/Presets. */
    static juce::File userPresetFolder();
    static juce::Array<juce::File> userPresetFiles();
    bool savePresetTo (const juce::File&);
    bool loadPresetFrom (const juce::File&);

    /** Nome do som em uso (de fabrica ou do usuario), para a tela mostrar. */
    juce::String getPresetName() const;

    /** Segurar: com isso ligado, cada nota tocada fica soando ate ser tocada de novo.
        Serve para deixar um acorde preso e mexer nos botoes com o mouse. */
    void setHold (bool shouldHold)          { hold = shouldHold; }
    bool getHold() const                    { return hold.load(); }
    bool isNoteHeld (int note) const        { return note >= 0 && note < 128 && held[note].load(); }

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    SharedState shared;
    SynthEngine synth;

    std::vector<float> lfoBuffer;
    double lfoPhase = 0.0;
    float lfoRandom = 0.0f, lfoRandomSmoothed = 0.0f;
    juce::Random random;

    drik::Chorus chorus;

    // Drive: distorcao sobre a soma das vozes (como um amplificador), com 4x de sobreamostragem.
    void applyDrive (float* left, float* right, int n);
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    drik::StateVariableFilter driveTone[2];
    juce::SmoothedValue<float> driveSmoothed;
    float hpState[2] { 0.0f, 0.0f };
    juce::SmoothedValue<float> volume;

    std::atomic<float> *lfoRate = nullptr, *lfoWave = nullptr, *hpf = nullptr,
                       *chorusMode = nullptr, *chorusMix = nullptr, *volumeDb = nullptr,
                       *drive = nullptr, *driveToneHz = nullptr;
    void applyHold (juce::MidiBuffer&);
    std::atomic<bool> hold { false };
    std::atomic<bool> held[128];
    bool holdWasOn = false;
    juce::MidiBuffer holdScratch;

    void setPresetName (const juce::String&);
    juce::CriticalSection nameLock;
    juce::String presetName;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrikSynthProcessor)
};
