#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

//==============================================================================
void SharedState::bind (juce::AudioProcessorValueTreeState& state)
{
    apvts = &state;
    o1wave = p ("o1wave");  o1oct = p ("o1oct");
    o2wave = p ("o2wave");  o2semi = p ("o2semi");  o2fine = p ("o2fine");  sync = p ("sync");
    pw = p ("pw");          pwm = p ("pwm");
    mix1 = p ("mix1");      mix2 = p ("mix2");      sub = p ("sub");        noise = p ("noise");
    unison = p ("unison");  detune = p ("detune");  spread = p ("spread");
    glide = p ("glide");    mono = p ("mono");
    cutoff = p ("cutoff");  reso = p ("reso");      fmode = p ("fmode");
    envamt = p ("envamt");  keytrack = p ("keytrack"); lfocut = p ("lfocut");
    fa = p ("fa"); fd = p ("fd"); fs = p ("fs"); fr = p ("fr");
    aa = p ("aa"); ad = p ("ad"); as = p ("as"); ar = p ("ar");
    lfopitch = p ("lfopitch");
    velsens = p ("velsens");
}

//==============================================================================
void SynthVoice::setCurrentPlaybackSampleRate (double newRate)
{
    juce::SynthesiserVoice::setCurrentPlaybackSampleRate (newRate);

    if (newRate > 0.0)
    {
        ampEnv.setSampleRate (newRate);
        filterEnv.setSampleRate (newRate);
    }
}

void SynthVoice::startNote (int midiNote, float velocity, juce::SynthesiserSound*, int pitchWheel)
{
    const bool wasSounding = ampEnv.isActive();

    targetNote = (float) midiNote;
    const bool gliding = shared.glide->load() > 0.001f && shared.lastNote >= 0.0f;
    currentNote = gliding ? shared.lastNote : targetNote;
    shared.lastNote = targetNote;

    bendSemis = ((float) pitchWheel - 8192.0f) / 8192.0f * 2.0f;

    const float sens = juce::jlimit (0.0f, 1.0f, shared.velsens->load());
    velocityGain = 1.0f - sens + sens * velocity;

    // Unison: de 1 a 4 copias da voz, desafinadas e espalhadas no estereo.
    numUnison = juce::jlimit (1, maxUnison, juce::roundToInt (shared.unison->load()));
    const float detuneCents = shared.detune->load();
    const float spread = shared.mono->load() > 0.5f && numUnison == 1 ? 0.0f : shared.spread->load();

    // Sem unison, cada voz do acorde ganha um lugar fixo no estereo (como nos Oberheim).
    static const float voicePan[8] = { -0.5f, 0.5f, -0.2f, 0.2f, -0.8f, 0.8f, -0.35f, 0.35f };

    for (int u = 0; u < numUnison; ++u)
    {
        const float pos = numUnison == 1 ? 0.0f : 2.0f * (float) u / (float) (numUnison - 1) - 1.0f;
        ratio[u] = std::pow (2.0f, pos * detuneCents / 1200.0f);

        const float pan = numUnison == 1 ? voicePan[voiceIndex % 8] * spread : pos * spread;   // -1..1
        const float angle = (pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi;
        gainL[u] = std::cos (angle) * 1.41421356f;
        gainR[u] = std::sin (angle) * 1.41421356f;

        if (! wasSounding)
        {
            // Osciladores analogicos correm soltos: cada nota comeca numa fase qualquer.
            phase1[u] = random.nextFloat();
            phase2[u] = random.nextFloat();
            phaseSub[u] = random.nextFloat();
        }
    }

    if (! wasSounding)
    {
        for (int c = 0; c < 2; ++c) { ladder[c].reset(); svf[c].reset(); }
        ampEnv.reset();
        filterEnv.reset();
    }

    ampEnv.set (shared.aa->load(), shared.ad->load(), shared.as->load(), shared.ar->load());
    filterEnv.set (shared.fa->load(), shared.fd->load(), shared.fs->load(), shared.fr->load());
    ampEnv.noteOn();
    filterEnv.noteOn();
}

void SynthVoice::stopNote (float, bool allowTailOff)
{
    if (allowTailOff)
    {
        ampEnv.noteOff();
        filterEnv.noteOff();
    }
    else
    {
        ampEnv.reset();
        filterEnv.reset();
        clearCurrentNote();
    }
}

void SynthVoice::pitchWheelMoved (int newValue)
{
    bendSemis = ((float) newValue - 8192.0f) / 8192.0f * 2.0f;      // +/- 2 semitons
}

void SynthVoice::renderNextBlock (juce::AudioBuffer<float>& out, int startSample, int numSamples)
{
    if (! ampEnv.isActive())
        return;

    const float sr = (float) getSampleRate();
    if (sr <= 0.0f || shared.lfo == nullptr)
        return;

    float* outL = out.getWritePointer (0, startSample);
    float* outR = out.getNumChannels() > 1 ? out.getWritePointer (1, startSample) : nullptr;

    // Parametros lidos uma vez por bloco.
    const int wave1 = juce::roundToInt (shared.o1wave->load());
    const int wave2 = juce::roundToInt (shared.o2wave->load());
    const float oct1 = std::round (shared.o1oct->load()) * 12.0f;
    const float semi2 = std::round (shared.o2semi->load()) + shared.o2fine->load() * 0.01f;
    const bool sync = shared.sync->load() > 0.5f;
    const float pw = shared.pw->load(), pwm = shared.pwm->load();
    const float mix1 = shared.mix1->load(), mix2 = shared.mix2->load();
    const float sub = shared.sub->load(), noise = shared.noise->load();
    const float glideTime = shared.glide->load();
    const float cutoffBase = shared.cutoff->load(), reso = shared.reso->load();
    const bool ladderMode = shared.fmode->load() < 0.5f;
    const float envAmt = shared.envamt->load(), keytrack = shared.keytrack->load(), lfoCut = shared.lfocut->load();
    const float vibratoCents = shared.lfopitch->load() + shared.modWheel * 50.0f;
    // Em mono so ha uma voz soando: ganha o dobro para nao ficar baixo perto dos acordes.
    const float norm = (shared.mono->load() > 0.5f ? 0.5f : 0.25f) / std::sqrt ((float) numUnison);

    ampEnv.set (shared.aa->load(), shared.ad->load(), shared.as->load(), shared.ar->load());
    filterEnv.set (shared.fa->load(), shared.fd->load(), shared.fs->load(), shared.fr->load());

    const int chunk = 16;      // altura, largura de pulso e corte do filtro mudam a cada 16 samples

    for (int done = 0; done < numSamples; done += chunk)
    {
        const int n = juce::jmin (chunk, numSamples - done);
        const float lfo = shared.lfo[startSample + done];

        if (glideTime > 0.001f)
            currentNote += (targetNote - currentNote) * (1.0f - std::exp (-(float) n / (glideTime * sr * 0.33f)));
        else
            currentNote = targetNote;

        const float pitchMod = bendSemis + lfo * vibratoCents * 0.01f;
        const float dt1 = juce::jmin (0.45f, drik::midiToHz (currentNote + oct1 + pitchMod) / sr);
        const float dt2 = juce::jmin (0.45f, drik::midiToHz (currentNote + semi2 + pitchMod) / sr);
        const float pwNow = juce::jlimit (0.05f, 0.95f, pw + pwm * 0.45f * lfo);

        const float octaves = envAmt * 7.0f * filterEnv.value()
                            + keytrack * (currentNote - 60.0f) / 12.0f
                            + lfoCut * 3.0f * lfo;
        const float cutoff = juce::jlimit (20.0f, sr * 0.45f, cutoffBase * std::pow (2.0f, octaves));

        for (int c = 0; c < 2; ++c)
        {
            if (ladderMode) ladder[c].set (cutoff, reso, sr);
            else            svf[c].set (cutoff, reso, sr);
        }

        for (int i = 0; i < n; ++i)
        {
            float l = 0.0f, r = 0.0f;

            for (int u = 0; u < numUnison; ++u)
            {
                const float d1 = juce::jmin (0.49f, dt1 * ratio[u]);
                const float d2 = juce::jmin (0.49f, dt2 * ratio[u]);

                float v = 0.0f;
                if (mix1 > 0.0f) v += mix1 * drik::oscValue (wave1, phase1[u], d1, pwNow);
                if (mix2 > 0.0f) v += mix2 * drik::oscValue (wave2, phase2[u], d2, pwNow);
                if (sub > 0.0f)  v += sub * drik::oscValue (drik::wavePulse, phaseSub[u], d1 * 0.5f, 0.5f);

                phase1[u] += d1;
                phase2[u] += d2;
                phaseSub[u] += d1 * 0.5f;

                if (phase1[u] >= 1.0f)
                {
                    phase1[u] -= 1.0f;
                    if (sync)
                        phase2[u] = phase1[u] * (d2 / d1);      // osc 2 recomeca junto com o osc 1
                }
                if (phase2[u] >= 1.0f)   phase2[u] -= std::floor (phase2[u]);
                if (phaseSub[u] >= 1.0f) phaseSub[u] -= 1.0f;

                l += v * gainL[u];
                r += v * gainR[u];
            }

            if (noise > 0.0f)
            {
                const float nz = (random.nextFloat() * 2.0f - 1.0f) * noise * 0.7f * std::sqrt ((float) numUnison);
                l += nz;
                r += nz;
            }

            l *= norm;
            r *= norm;

            if (ladderMode) { l = ladder[0].process (l); r = ladder[1].process (r); }
            else            { l = svf[0].process (l);    r = svf[1].process (r); }

            filterEnv.next();
            const float amp = ampEnv.next() * velocityGain;
            l *= amp;
            r *= amp;

            if (outR != nullptr) { outL[done + i] += l; outR[done + i] += r; }
            else                 { outL[done + i] += 0.5f * (l + r); }
        }

        if (! ampEnv.isActive())
        {
            clearCurrentNote();
            break;
        }
    }
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout DrikSynthProcessor::createLayout()
{
    using Range  = juce::NormalisableRange<float>;
    using Float  = juce::AudioParameterFloat;
    using Int    = juce::AudioParameterInt;
    using Bool   = juce::AudioParameterBool;
    using Choice = juce::AudioParameterChoice;
    using Attr   = juce::AudioParameterFloatAttributes;

    auto id = [] (const char* s) { return juce::ParameterID { s, 1 }; };
    auto skewed = [] (float lo, float hi, float centre) { Range r (lo, hi, 0.0f); r.setSkewForCentre (centre); return r; };

    auto percent = Attr().withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; });
    auto seconds = Attr().withStringFromValueFunction ([] (float v, int)
    {
        return v < 1.0f ? juce::String (juce::roundToInt (v * 1000.0f)) + " ms" : juce::String (v, 2) + " s";
    });
    auto hertz = Attr().withStringFromValueFunction ([] (float v, int)
    {
        return v < 1000.0f ? juce::String (juce::roundToInt (v)) + " Hz" : juce::String (v / 1000.0f, 1) + " kHz";
    });
    auto cents = Attr().withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v)) + " ct"; });
    auto signedPercent = Attr().withStringFromValueFunction ([] (float v, int)
    {
        const int p = juce::roundToInt (v * 100.0f);
        return juce::String (p > 0 ? "+" : "") + juce::String (p) + " %";
    });

    const juce::StringArray waves { "Serra", "Pulso", juce::String::fromUTF8 ("Tri\xc3\xa2ngulo"), "Seno" };

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Osciladores
    layout.add (std::make_unique<Choice> (id ("o1wave"), "Osc 1 Wave", waves, 0));
    layout.add (std::make_unique<Int>    (id ("o1oct"),  "Osc 1 Octave", -2, 2, 0));
    layout.add (std::make_unique<Choice> (id ("o2wave"), "Osc 2 Wave", waves, 0));
    layout.add (std::make_unique<Int>    (id ("o2semi"), "Osc 2 Semitones", -24, 24, 0));
    layout.add (std::make_unique<Float>  (id ("o2fine"), "Osc 2 Fine", Range (-50.0f, 50.0f, 0.1f), 0.0f, cents));
    layout.add (std::make_unique<Bool>   (id ("sync"),   "Osc Sync", false));
    layout.add (std::make_unique<Float>  (id ("pw"),     "Pulse Width", Range (0.05f, 0.95f, 0.0f), 0.5f, percent));
    layout.add (std::make_unique<Float>  (id ("pwm"),    "PWM Depth", Range (0.0f, 1.0f, 0.0f), 0.0f, percent));

    // Mixer
    layout.add (std::make_unique<Float> (id ("mix1"),  "Osc 1 Level", Range (0.0f, 1.0f, 0.0f), 1.0f, percent));
    layout.add (std::make_unique<Float> (id ("mix2"),  "Osc 2 Level", Range (0.0f, 1.0f, 0.0f), 0.0f, percent));
    layout.add (std::make_unique<Float> (id ("sub"),   "Sub Level",   Range (0.0f, 1.0f, 0.0f), 0.0f, percent));
    layout.add (std::make_unique<Float> (id ("noise"), "Noise Level", Range (0.0f, 1.0f, 0.0f), 0.0f, percent));

    // Unison e voz
    layout.add (std::make_unique<Int>   (id ("unison"), "Unison Voices", 1, 4, 1));
    layout.add (std::make_unique<Float> (id ("detune"), "Unison Detune", Range (0.0f, 50.0f, 0.1f), 12.0f, cents));
    layout.add (std::make_unique<Float> (id ("spread"), "Stereo Spread", Range (0.0f, 1.0f, 0.0f), 0.5f, percent));
    layout.add (std::make_unique<Float> (id ("glide"),  "Glide", skewed (0.0f, 2.0f, 0.2f), 0.0f, seconds));
    layout.add (std::make_unique<Bool>  (id ("mono"),   "Mono", false));

    // Filtro
    layout.add (std::make_unique<Choice> (id ("fmode"),  "Filter Type", juce::StringArray { "24 dB", "12 dB" }, 0));
    layout.add (std::make_unique<Float>  (id ("hpf"),    "High Pass", skewed (20.0f, 2000.0f, 200.0f), 20.0f, hertz));
    layout.add (std::make_unique<Float>  (id ("cutoff"), "Cutoff", skewed (20.0f, 20000.0f, 1000.0f), 20000.0f, hertz));
    layout.add (std::make_unique<Float>  (id ("reso"),   "Resonance", Range (0.0f, 1.0f, 0.0f), 0.0f, percent));
    layout.add (std::make_unique<Float>  (id ("envamt"), "Filter Env Amount", Range (-1.0f, 1.0f, 0.0f), 0.0f, signedPercent));
    layout.add (std::make_unique<Float>  (id ("keytrack"), "Key Tracking", Range (0.0f, 1.0f, 0.0f), 0.5f, percent));
    layout.add (std::make_unique<Float>  (id ("lfocut"), "LFO to Cutoff", Range (0.0f, 1.0f, 0.0f), 0.0f, percent));

    // Envelopes
    layout.add (std::make_unique<Float> (id ("fa"), "Filter Attack",  skewed (0.001f, 8.0f, 0.3f), 0.005f, seconds));
    layout.add (std::make_unique<Float> (id ("fd"), "Filter Decay",   skewed (0.005f, 10.0f, 0.5f), 0.3f, seconds));
    layout.add (std::make_unique<Float> (id ("fs"), "Filter Sustain", Range (0.0f, 1.0f, 0.0f), 0.5f, percent));
    layout.add (std::make_unique<Float> (id ("fr"), "Filter Release", skewed (0.005f, 10.0f, 0.5f), 0.3f, seconds));
    layout.add (std::make_unique<Float> (id ("aa"), "Amp Attack",  skewed (0.001f, 8.0f, 0.3f), 0.005f, seconds));
    layout.add (std::make_unique<Float> (id ("ad"), "Amp Decay",   skewed (0.005f, 10.0f, 0.5f), 0.3f, seconds));
    layout.add (std::make_unique<Float> (id ("as"), "Amp Sustain", Range (0.0f, 1.0f, 0.0f), 1.0f, percent));
    layout.add (std::make_unique<Float> (id ("ar"), "Amp Release", skewed (0.005f, 10.0f, 0.5f), 0.25f, seconds));

    // LFO
    layout.add (std::make_unique<Choice> (id ("lfowave"), "LFO Wave",
                                          juce::StringArray { juce::String::fromUTF8 ("Tri\xc3\xa2ngulo"), "Seno", "Quadrada", juce::String::fromUTF8 ("Aleat\xc3\xb3rio") }, 0));
    layout.add (std::make_unique<Float>  (id ("lforate"), "LFO Rate", skewed (0.05f, 20.0f, 3.0f), 5.0f,
                                          Attr().withStringFromValueFunction ([] (float v, int) { return juce::String (v, 2) + " Hz"; })));
    layout.add (std::make_unique<Float>  (id ("lfopitch"), "Vibrato", skewed (0.0f, 1200.0f, 50.0f), 0.0f, cents));

    // Drive
    layout.add (std::make_unique<Float> (id ("drive"), "Drive", Range (0.0f, 1.0f, 0.0f), 0.0f, percent));
    layout.add (std::make_unique<Float> (id ("drivetone"), "Drive Tone", skewed (800.0f, 16000.0f, 4000.0f), 8000.0f, hertz));

    // Chorus e saida
    layout.add (std::make_unique<Choice> (id ("chorus"), "Chorus Type", juce::StringArray { "Desligado", "Chorus I", "Chorus II", "Ensemble" }, 0));
    layout.add (std::make_unique<Float>  (id ("chomix"), "Chorus Mix", Range (0.0f, 1.0f, 0.0f), 0.5f, percent));
    layout.add (std::make_unique<Float>  (id ("velsens"), "Velocity Sensitivity", Range (0.0f, 1.0f, 0.0f), 0.5f, percent));
    layout.add (std::make_unique<Float>  (id ("volume"), "Volume", Range (-36.0f, 6.0f, 0.1f), -6.0f,
                                          Attr().withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1) + " dB"; })));
    return layout;
}

//==============================================================================
DrikSynthProcessor::DrikSynthProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "DrikSynth", createLayout())
{
    shared.bind (apvts);
    lfoRate    = apvts.getRawParameterValue ("lforate");
    lfoWave    = apvts.getRawParameterValue ("lfowave");
    hpf        = apvts.getRawParameterValue ("hpf");
    chorusMode = apvts.getRawParameterValue ("chorus");
    chorusMix  = apvts.getRawParameterValue ("chomix");
    volumeDb   = apvts.getRawParameterValue ("volume");
    drive       = apvts.getRawParameterValue ("drive");
    driveToneHz = apvts.getRawParameterValue ("drivetone");
    oversampling = std::make_unique<juce::dsp::Oversampling<float>> (2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);

    for (int i = 0; i < 16; ++i)
        synth.addVoice (new SynthVoice (shared, i));
    synth.addSound (new SynthSound());

    applyPreset (0);
}

void DrikSynthProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);
    keyboardState.reset();
    lfoBuffer.assign ((size_t) juce::jmax (samplesPerBlock, 64) * 2, 0.0f);
    chorus.prepare (sampleRate);
    oversampling->initProcessing ((size_t) juce::jmax (samplesPerBlock, 64));
    oversampling->reset();
    for (auto& f : driveTone) f.reset();
    driveSmoothed.reset (sampleRate, 0.05);
    driveSmoothed.setCurrentAndTargetValue (drive->load());
    hpState[0] = hpState[1] = 0.0f;
    volume.reset (sampleRate, 0.03);
    volume.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (volumeDb->load()));
    shared.lastNote = -1.0f;
}

bool DrikSynthProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void DrikSynthProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const double sr = getSampleRate();
    buffer.clear();

    if (n <= 0 || sr <= 0.0)
        return;

    keyboardState.processNextMidiBuffer (midi, 0, n, true);

    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (m.isController() && m.getControllerNumber() == 1)
            shared.modWheel = (float) m.getControllerValue() / 127.0f;
    }

    // LFO unico para todas as vozes, calculado para o bloco inteiro.
    if ((int) lfoBuffer.size() < n)
        lfoBuffer.resize ((size_t) n);

    const int wave = juce::roundToInt (lfoWave->load());
    const double inc = (double) lfoRate->load() / sr;
    const float smooth = (float) (1.0 - std::exp (-1.0 / (0.004 * sr)));

    for (int i = 0; i < n; ++i)
    {
        float v;
        switch (wave)
        {
            case 0:  v = 4.0f * (float) std::fabs (lfoPhase - 0.5) - 1.0f; break;
            case 1:  v = (float) std::sin (2.0 * juce::MathConstants<double>::pi * lfoPhase); break;
            case 2:  v = lfoPhase < 0.5 ? 1.0f : -1.0f; break;
            default: lfoRandomSmoothed += (lfoRandom - lfoRandomSmoothed) * smooth; v = lfoRandomSmoothed; break;
        }
        lfoBuffer[(size_t) i] = v;

        lfoPhase += inc;
        if (lfoPhase >= 1.0)
        {
            lfoPhase -= 1.0;
            lfoRandom = random.nextFloat() * 2.0f - 1.0f;
        }
    }
    shared.lfo = lfoBuffer.data();

    synth.mono = shared.mono->load() > 0.5f;
    synth.renderNextBlock (buffer, midi, 0, n);

    // Cadeia de saida: passa-alta, chorus, volume, limitador suave.
    float* left = buffer.getWritePointer (0);
    float* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

    std::vector<float>& scratch = lfoBuffer;    // so para o caso mono, depois de usado pelas vozes
    if (right == nullptr)
    {
        std::copy (left, left + n, scratch.begin());
        right = scratch.data();
    }

    applyDrive (left, right, n);

    const float hpCoef = (float) (1.0 - std::exp (-2.0 * juce::MathConstants<double>::pi * (double) hpf->load() / sr));
    if (hpf->load() > 21.0f)
    {
        for (int i = 0; i < n; ++i)
        {
            hpState[0] += (left[i] - hpState[0]) * hpCoef;   left[i] -= hpState[0];
            hpState[1] += (right[i] - hpState[1]) * hpCoef;  right[i] -= hpState[1];
        }
    }

    chorus.process (left, right, n, juce::roundToInt (chorusMode->load()), chorusMix->load());

    volume.setTargetValue (juce::Decibels::decibelsToGain (volumeDb->load()));
    for (int i = 0; i < n; ++i)
    {
        const float g = volume.getNextValue();
        left[i] = drik::softClip (left[i] * g);
        right[i] = drik::softClip (right[i] * g);
    }

    if (buffer.getNumChannels() == 1)
        for (int i = 0; i < n; ++i)
            left[i] = 0.5f * (left[i] + right[i]);
}

//==============================================================================
void DrikSynthProcessor::applyDrive (float* left, float* right, int n)
{
    driveSmoothed.setTargetValue (drive->load());

    if (driveSmoothed.getTargetValue() <= 0.001f && ! driveSmoothed.isSmoothing())
    {
        driveSmoothed.skip (n);
        return;
    }

    const float amount = driveSmoothed.skip (n);

    // Ganho de 0 a +36 dB antes do limitador; quanto mais ganho, mais o acorde "rasga",
    // porque as notas se misturam dentro da distorcao (como num amplificador de guitarra).
    const float gain = std::pow (10.0f, amount * 36.0f / 20.0f);
    const float bias = 0.08f * amount;                       // assimetria: acrescenta harmonicos pares
    const float offset = std::tanh (gain * bias);
    const float makeup = 1.0f / (1.0f + 3.5f * amount);      // segura o volume, que sobe com a distorcao

    float* channels[2] = { left, right };
    juce::dsp::AudioBlock<float> block (channels, 2, (size_t) n);

    {
        auto up = oversampling->processSamplesUp (block);
        for (size_t c = 0; c < 2; ++c)
        {
            float* d = up.getChannelPointer (c);
            for (size_t i = 0; i < up.getNumSamples(); ++i)
                d[i] = (std::tanh (gain * (d[i] + bias)) - offset) * makeup;
        }
        oversampling->processSamplesDown (block);
    }

    // Tom: passa-baixa depois da distorcao (o "alto-falante").
    const float tone = juce::jlimit (200.0f, (float) getSampleRate() * 0.45f, driveToneHz->load());
    for (int c = 0; c < 2; ++c)
    {
        driveTone[c].set (tone, 0.1f, getSampleRate());
        for (int i = 0; i < n; ++i)
            channels[c][i] = driveTone[c].process (channels[c][i]);
    }
}

//==============================================================================
static bool presetLeavesAlone (const juce::String& id)
{
    return id == "volume" || id == "velsens";       // ajustes do usuario: preset nao mexe
}

void DrikSynthProcessor::applyPreset (int index)
{
    const auto& all = drik::presets();
    if (index < 0 || index >= (int) all.size())
        return;

    const auto& preset = all[(size_t) index];

    for (auto* param : getParameters())
    {
        auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param);
        if (ranged == nullptr || presetLeavesAlone (ranged->getParameterID()))
            continue;

        float value = ranged->convertFrom0to1 (ranged->getDefaultValue());
        for (const auto& kv : preset.values)
            if (ranged->getParameterID() == kv.first)
                value = kv.second;

        ranged->setValueNotifyingHost (ranged->convertTo0to1 (value));
    }

    setPresetName (juce::String::fromUTF8 (preset.name));
}

//==============================================================================
juce::File DrikSynthProcessor::userPresetFolder()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
               .getChildFile ("DrikSynth").getChildFile ("Presets");
}

juce::Array<juce::File> DrikSynthProcessor::userPresetFiles()
{
    auto files = userPresetFolder().findChildFiles (juce::File::findFiles, false, "*.drikpreset");
    files.sort();
    return files;
}

bool DrikSynthProcessor::savePresetTo (const juce::File& file)
{
    juce::XmlElement xml ("DrikSynthPreset");
    xml.setAttribute ("version", 1);

    for (auto* param : getParameters())
    {
        auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param);
        if (ranged == nullptr || presetLeavesAlone (ranged->getParameterID()))
            continue;

        auto* e = xml.createNewChildElement ("PARAM");
        e->setAttribute ("id", ranged->getParameterID());
        e->setAttribute ("value", (double) ranged->convertFrom0to1 (ranged->getValue()));
    }

    if (! file.getParentDirectory().createDirectory() || ! xml.writeTo (file))
        return false;

    setPresetName (file.getFileNameWithoutExtension());
    return true;
}

bool DrikSynthProcessor::loadPresetFrom (const juce::File& file)
{
    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr || ! xml->hasTagName ("DrikSynthPreset"))
        return false;

    // Parametro que nao estiver no arquivo (preset salvo numa versao mais antiga) volta ao padrao.
    for (auto* param : getParameters())
    {
        auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param);
        if (ranged == nullptr || presetLeavesAlone (ranged->getParameterID()))
            continue;

        float value = ranged->convertFrom0to1 (ranged->getDefaultValue());
        for (auto* e : xml->getChildWithTagNameIterator ("PARAM"))
            if (e->getStringAttribute ("id") == ranged->getParameterID())
                value = (float) e->getDoubleAttribute ("value", value);

        ranged->setValueNotifyingHost (ranged->convertTo0to1 (value));
    }

    setPresetName (file.getFileNameWithoutExtension());
    return true;
}

juce::String DrikSynthProcessor::getPresetName() const
{
    const juce::ScopedLock sl (nameLock);
    return presetName;
}

void DrikSynthProcessor::setPresetName (const juce::String& name)
{
    const juce::ScopedLock sl (nameLock);
    presetName = name;
}

//==============================================================================
void DrikSynthProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("presetName", getPresetName(), nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, dest);
}

void DrikSynthProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        auto state = juce::ValueTree::fromXml (*xml);
        if (! state.isValid())
            return;

        setPresetName (state.getProperty ("presetName").toString());
        apvts.replaceState (state);
    }
}

juce::AudioProcessorEditor* DrikSynthProcessor::createEditor()
{
    return new DrikSynthEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DrikSynthProcessor();
}
