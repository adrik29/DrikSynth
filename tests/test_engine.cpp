// Teste do motor do DrikSynth sem interface: afinacao, silencio, estabilidade e todos os presets.
#include "../Source/PluginProcessor.h"
#include "../Source/Presets.h"
#include "PitchDetect.h"
#include <chrono>
#include <cstdio>

static int fails = 0;
static void check (bool ok, const juce::String& what)
{
    if (! ok) { std::printf ("FALHA  %s\n", what.toRawUTF8()); ++fails; }
}

struct Rendered { std::vector<float> mono; float peak = 0.0f; bool finite = true; double rms = 0.0; };

static Rendered render (DrikSynthProcessor& p, juce::MidiBuffer first, double seconds, double sr, int block, int channels = 2)
{
    Rendered out;
    juce::AudioBuffer<float> buf (channels, block);
    double sum = 0.0;
    for (int done = 0; done < (int) (seconds * sr); done += block)
    {
        p.processBlock (buf, first);
        first.clear();
        for (int i = 0; i < block; ++i)
        {
            float m = 0.0f;
            for (int c = 0; c < channels; ++c)
            {
                const float v = buf.getSample (c, i);
                out.finite = out.finite && std::isfinite (v);
                out.peak = std::max (out.peak, std::fabs (v));
                m += v / (float) channels;
            }
            out.mono.push_back (m);
            sum += (double) m * m;
        }
    }
    out.rms = std::sqrt (sum / std::max<size_t> (1, out.mono.size()));
    return out;
}

static juce::MidiBuffer notesOn (std::initializer_list<int> notes, float vel)
{
    juce::MidiBuffer m;
    for (int n : notes) m.addEvent (juce::MidiMessage::noteOn (1, n, vel), 0);
    return m;
}

static juce::MidiBuffer notesOff (std::initializer_list<int> notes)
{
    juce::MidiBuffer m;
    for (int n : notes) m.addEvent (juce::MidiMessage::noteOff (1, n), 0);
    return m;
}

static void setP (DrikSynthProcessor& p, const char* id, float v)
{
    auto* prm = p.apvts.getParameter (id);
    prm->setValueNotifyingHost (prm->convertTo0to1 (v));
}

static int pitchOf (const Rendered& r, double sr, double fromSeconds)
{
    const int from = (int) (fromSeconds * sr);
    return drik::hzToMidiNote (drik::detectPitchHz (r.mono.data() + from, (int) r.mono.size() - from, sr));
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    const auto& presets = drik::presets();
    const int block = 512;
    int initIndex = -1;
    for (int i = 0; i < (int) presets.size(); ++i)
        if (juce::String (presets[(size_t) i].name).startsWith ("Init")) initIndex = i;
    check (initIndex >= 0, "existe o preset Init");

    // ---- afinacao em tres taxas de amostragem
    for (double sr : { 44100.0, 48000.0, 96000.0 })
    {
        DrikSynthProcessor p;
        p.setPlayConfigDetails (0, 2, sr, block);
        p.prepareToPlay (sr, block);
        p.applyPreset (initIndex);
        check (render (p, {}, 0.2, sr, block).peak == 0.0f, "sem nota: silencio");

        for (int note : { 33, 45, 57, 69, 81, 93 })
        {
            auto a = render (p, notesOn ({ note }, 1.0f), 0.6, sr, block);
            check (a.finite && a.peak > 0.02f, "nota produz som");
            check (pitchOf (a, sr, 0.1) == note, "afinacao da nota " + juce::String (note) + " a " + juce::String (sr));
            auto t = render (p, notesOff ({ note }), 3.0, sr, block);
            check (t.finite, "cauda finita");
            check (render (p, {}, 0.2, sr, block).peak == 0.0f, "depois do release: silencio");
        }

        // Osc 2 uma oitava acima sozinho; oitava do osc 1; pitch bend.
        setP (p, "mix1", 0.0f); setP (p, "mix2", 1.0f); setP (p, "o2semi", 12.0f);
        auto o2 = render (p, notesOn ({ 57 }, 1.0f), 0.5, sr, block);
        check (pitchOf (o2, sr, 0.1) == 69, "osc 2 +12 semitons");
        render (p, notesOff ({ 57 }), 2.0, sr, block);
        setP (p, "mix1", 1.0f); setP (p, "mix2", 0.0f); setP (p, "o1oct", -1.0f);
        auto oc = render (p, notesOn ({ 69 }, 1.0f), 0.5, sr, block);
        check (pitchOf (oc, sr, 0.1) == 57, "oitava -1 do osc 1");
        juce::MidiBuffer bend;
        bend.addEvent (juce::MidiMessage::pitchWheel (1, 16383), 0);
        auto bd = render (p, bend, 0.5, sr, block);
        check (pitchOf (bd, sr, 0.1) == 59, "pitch bend sobe 2 semitons");
        render (p, notesOff ({ 69 }), 2.0, sr, block);
    }

    // ---- todos os presets: acorde, cauda, nivel
    {
        const double sr = 48000.0;
        std::printf ("\n%-26s %7s %7s\n", "preset", "pico", "rms dB");
        for (int i = 0; i < (int) presets.size(); ++i)
        {
            DrikSynthProcessor p;
            p.setPlayConfigDetails (0, 2, sr, block);
            p.prepareToPlay (sr, block);
            p.applyPreset (i);
            setP (p, "volume", -6.0f);
            const juce::String name (presets[(size_t) i].name);
            const bool mono = p.apvts.getRawParameterValue ("mono")->load() > 0.5f;

            for (const auto& kv : presets[(size_t) i].values)
                check (p.apvts.getParameter (kv.first) != nullptr, name + ": parametro desconhecido " + kv.first);

            auto a = mono ? render (p, notesOn ({ 45 }, 0.8f), 2.5, sr, block)
                          : render (p, notesOn ({ 48, 55, 60, 64 }, 0.8f), 2.5, sr, block);
            check (a.finite, name + ": saida finita");
            check (a.peak <= 1.0f, name + ": nao passa de 0 dB");
            check (a.rms > 0.004, name + ": nao esta mudo");
            std::printf ("%-26s %7.2f %7.1f\n", presets[(size_t) i].name, a.peak, 20.0 * std::log10 (a.rms + 1e-9));

            auto t = render (p, notesOff ({ 45, 48, 55, 60, 64 }), 14.0, sr, block);
            check (t.finite, name + ": cauda finita");
            check (render (p, {}, 0.3, sr, block).peak < 1.0e-4f, name + ": silencia depois de soltar");
        }
    }

    // ---- modo mono: a nota nova assume
    {
        const double sr = 48000.0;
        DrikSynthProcessor p;
        p.setPlayConfigDetails (0, 2, sr, block);
        p.prepareToPlay (sr, block);
        p.applyPreset (initIndex);
        setP (p, "mono", 1.0f); setP (p, "ar", 0.02f);
        render (p, notesOn ({ 57 }, 1.0f), 0.4, sr, block);
        auto second = render (p, notesOn ({ 69 }, 1.0f), 0.6, sr, block);
        check (pitchOf (second, sr, 0.3) == 69, "mono: a nota nova assume");
        setP (p, "glide", 0.3f);
        auto slide = render (p, notesOn ({ 81 }, 1.0f), 2.5, sr, block);
        check (pitchOf (slide, sr, 2.0) == 81, "glide: chega na nota de destino");
    }

    // ---- Segurar: a nota fica presa ate ser tocada de novo
    {
        const double sr = 48000.0;
        DrikSynthProcessor p;
        p.setPlayConfigDetails (0, 2, sr, block);
        p.prepareToPlay (sr, block);
        p.applyPreset (initIndex);
        setP (p, "ar", 0.02f);
        p.setHold (true);
        render (p, notesOn ({ 57, 64 }, 1.0f), 0.2, sr, block);
        auto afterOff = render (p, notesOff ({ 57, 64 }), 1.0, sr, block);
        check (afterOff.peak > 0.02f && p.isNoteHeld (57) && p.isNoteHeld (64), "segurar: soltar a tecla nao solta a nota");
        render (p, notesOn ({ 64 }, 1.0f), 0.5, sr, block);
        auto one = render (p, notesOff ({ 64 }), 0.5, sr, block);
        check (! p.isNoteHeld (64) && p.isNoteHeld (57) && pitchOf (one, sr, 0.1) == 57, "segurar: tocar de novo solta so aquela nota");
        p.setHold (false);
        render (p, {}, 0.5, sr, block);
        check (render (p, {}, 0.2, sr, block).peak == 0.0f && ! p.isNoteHeld (57), "segurar: desligar solta tudo");
        auto normal = render (p, notesOn ({ 60 }, 1.0f), 0.2, sr, block);
        render (p, notesOff ({ 60 }), 0.5, sr, block);
        check (normal.peak > 0.02f && render (p, {}, 0.2, sr, block).peak == 0.0f, "segurar desligado: teclado volta ao normal");
    }

    // ---- estado salvo e restaurado
    {
        const double sr = 48000.0;
        DrikSynthProcessor p, q;
        p.applyPreset (7);
        setP (p, "cutoff", 1234.0f);
        juce::MemoryBlock state;
        p.getStateInformation (state);
        q.setStateInformation (state.getData(), (int) state.getSize());
        check (q.getPresetName() == p.getPresetName() && p.getPresetName().isNotEmpty(), "estado: nome do som lembrado");
        bool same = true;
        for (auto* prm : p.getParameters())
            same = same && std::abs (prm->getValue() - q.getParameters()[prm->getParameterIndex()]->getValue()) < 1.0e-4f;
        check (same, "estado: todos os botoes iguais");
        juce::ignoreUnused (sr);
    }

    // ---- sons do usuario: salvar em arquivo e carregar de volta
    {
        DrikSynthProcessor p, q;
        p.applyPreset (12);
        setP (p, "cutoff", 777.0f); setP (p, "o2semi", 7.0f); setP (p, "chorus", 3.0f); setP (p, "mono", 1.0f);
        setP (q, "volume", -20.0f);
        auto file = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("Ultravox New Europeans.drikpreset");
        file.deleteFile();
        check (p.savePresetTo (file) && file.existsAsFile(), "salva o som em arquivo");
        check (p.getPresetName() == "Ultravox New Europeans", "o som salvo passa a ter o nome dado");
        check (q.loadPresetFrom (file), "carrega o som salvo");
        bool same = true;
        for (auto* prm : p.getParameters())
        {
            const auto id = dynamic_cast<juce::RangedAudioParameter*> (prm)->getParameterID();
            if (id != "volume" && id != "velsens")
                same = same && std::abs (prm->getValue() - q.getParameters()[prm->getParameterIndex()]->getValue()) < 1.0e-4f;
        }
        check (same, "som carregado: todos os botoes iguais ao salvo");
        check (std::abs (q.apvts.getRawParameterValue ("volume")->load() + 20.0f) < 0.01f, "carregar um som nao mexe no volume");
        file.replaceWithText ("isto nao e um preset");
        check (! q.loadPresetFrom (file), "arquivo invalido: recusa sem travar");
        file.deleteFile();
    }

    // ---- saida mono
    {
        const double sr = 48000.0;
        DrikSynthProcessor p;
        p.setPlayConfigDetails (0, 1, sr, block);
        p.prepareToPlay (sr, block);
        auto a = render (p, notesOn ({ 60, 64, 67 }, 1.0f), 1.0, sr, block, 1);
        check (a.finite && a.rms > 0.004 && a.peak <= 1.0f, "saida mono funciona");
    }

    // ---- botoes em posicoes aleatorias: nunca pode dar NaN nem estourar
    {
        const double sr = 44100.0;
        DrikSynthProcessor p;
        p.setPlayConfigDetails (0, 2, sr, 256);
        p.prepareToPlay (sr, 256);
        juce::Random rnd (1234);
        bool ok = true;
        for (int round = 0; round < 400 && ok; ++round)
        {
            for (auto* prm : p.getParameters())
                prm->setValueNotifyingHost (rnd.nextFloat());
            juce::MidiBuffer m;
            for (int k = 0; k < 6; ++k)
                m.addEvent (rnd.nextBool() ? juce::MidiMessage::noteOn (1, 24 + rnd.nextInt (84), rnd.nextFloat())
                                           : juce::MidiMessage::noteOff (1, 24 + rnd.nextInt (84)), rnd.nextInt (256));
            auto r = render (p, m, 0.15, sr, 256);
            ok = r.finite && r.peak <= 1.0f;
        }
        check (ok, "400 rodadas de botoes aleatorios: sem NaN e sem passar de 0 dB");
    }

    // ---- custo de processamento no pior caso
    {
        const double sr = 48000.0;
        DrikSynthProcessor p;
        p.setPlayConfigDetails (0, 2, sr, block);
        p.prepareToPlay (sr, block);
        p.applyPreset (initIndex);
        setP (p, "unison", 4.0f); setP (p, "mix2", 1.0f); setP (p, "sub", 1.0f); setP (p, "chorus", 3.0f);
        juce::MidiBuffer m;
        for (int n = 40; n < 56; ++n) m.addEvent (juce::MidiMessage::noteOn (1, n, 1.0f), 0);
        const auto t0 = std::chrono::steady_clock::now();
        render (p, m, 5.0, sr, block);
        const double secs = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
        std::printf ("\npior caso (16 vozes x 4 unison, 3 osciladores, ensemble): %.1f%% de um nucleo\n", 100.0 * secs / 5.0);
        check (secs < 2.5, "pior caso cabe em tempo real com folga");
    }

    std::printf (fails == 0 ? "\ntudo certo\n" : "\n%d falhas\n", fails);
    return fails;
}
