// Toca um preset do DrikSynth e grava 6 segundos em WAV.
//   DrikRender saida.wav <indice do preset> [parametro=valor ...] [nota MIDI ...]
// Exemplo: DrikRender teste.wav 6 cutoff=8000 drive=0.1 45 57 64 68 71
#include "../Source/PluginProcessor.h"

int main (int argc, char** argv)
{
    if (argc < 3)
        return 1;

    juce::ScopedJuceInitialiser_GUI init;
    const double sr = 44100.0;
    DrikSynthProcessor p;
    p.setPlayConfigDetails (0, 2, sr, 512);
    p.prepareToPlay (sr, 512);
    p.applyPreset (juce::String (argv[2]).getIntValue());

    juce::MidiBuffer m;
    for (int i = 3; i < argc; ++i)
    {
        const juce::String a (argv[i]);
        if (a.contains ("="))
        {
            if (auto* prm = p.apvts.getParameter (a.upToFirstOccurrenceOf ("=", false, false)))
                prm->setValueNotifyingHost (prm->convertTo0to1 (a.fromFirstOccurrenceOf ("=", false, false).getFloatValue()));
        }
        else
        {
            m.addEvent (juce::MidiMessage::noteOn (1, a.getIntValue(), 0.9f), 0);
        }
    }

    juce::AudioBuffer<float> all (2, (int) (sr * 6.0)), buf (2, 512);
    for (int pos = 0; pos + 512 <= all.getNumSamples(); pos += 512)
    {
        p.processBlock (buf, m);
        m.clear();
        all.copyFrom (0, pos, buf, 0, 0, 512);
        all.copyFrom (1, pos, buf, 1, 0, 512);
    }

    juce::File out (argv[1]);
    out.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::OutputStream> os (out.createOutputStream());
    auto w = wav.createWriterFor (os, juce::AudioFormatWriterOptions().withSampleRate (sr).withNumChannels (2).withBitsPerSample (16));
    return w != nullptr && w->writeFromAudioSampleBuffer (all, 0, all.getNumSamples()) ? 0 : 1;
}
