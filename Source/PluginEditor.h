#pragma once

#include "PluginProcessor.h"

//==============================================================================
/** Um controle da tela: botao giratorio, lista de opcoes ou chave, conforme o tipo do parametro. */
class Cell final : public juce::Component
{
public:
    Cell (juce::AudioProcessorValueTreeState&, const juce::String& paramID, const juce::String& title);
    void resized() override;
    float weight() const   { return combo != nullptr ? 1.6f : 1.0f; }    // lista de opcoes precisa de mais largura

private:
    using APVTS = juce::AudioProcessorValueTreeState;

    juce::Label label;
    std::unique_ptr<juce::Slider> slider;
    std::unique_ptr<juce::ComboBox> combo;
    std::unique_ptr<juce::ToggleButton> toggle;
    std::unique_ptr<APVTS::SliderAttachment> sliderAttachment;
    std::unique_ptr<APVTS::ComboBoxAttachment> comboAttachment;
    std::unique_ptr<APVTS::ButtonAttachment> buttonAttachment;
};

/** Um bloco da tela (OSC 1, FILTRO...), com titulo e seus controles lado a lado. */
class Section final : public juce::Component
{
public:
    struct Def { const char* id; const char* title; };

    Section (juce::AudioProcessorValueTreeState&, const juce::String& title, std::initializer_list<Def>);
    float weight() const   { float w = 0.0f; for (auto* c : cells) w += c->weight(); return w; }
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::String name;
    juce::OwnedArray<Cell> cells;
};

//==============================================================================
class DrikSynthEditor final : public juce::AudioProcessorEditor,
                              private juce::Timer
{
public:
    explicit DrikSynthEditor (DrikSynthProcessor&);
    ~DrikSynthEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void stepPreset (int delta);

    DrikSynthProcessor& processor;

    juce::Label title, versionLabel;
    juce::ComboBox presetBox;
    juce::TextButton prevButton { "<" }, nextButton { ">" };
    std::vector<std::vector<Section*>> rows;
    juce::OwnedArray<Section> sections;
    juce::MidiKeyboardComponent keyboard;
    int shownPreset = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrikSynthEditor)
};
