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

/** Teclado da tela que tambem acende as notas presas pelo botao Segurar. */
class HoldKeyboard final : public juce::MidiKeyboardComponent
{
public:
    HoldKeyboard (DrikSynthProcessor& p, juce::Colour heldColour)
        : juce::MidiKeyboardComponent (p.keyboardState, horizontalKeyboard), processor (p), colour (heldColour) {}

protected:
    void drawWhiteNote (int note, juce::Graphics& g, juce::Rectangle<float> area, bool isDown, bool isOver,
                        juce::Colour lineColour, juce::Colour textColour) override
    {
        juce::MidiKeyboardComponent::drawWhiteNote (note, g, area, isDown, isOver, lineColour, textColour);
        if (processor.isNoteHeld (note) && ! isDown)
        {
            g.setColour (colour.withAlpha (0.75f));
            g.fillRect (area.reduced (1.0f, 0.0f));
        }
    }

    void drawBlackNote (int note, juce::Graphics& g, juce::Rectangle<float> area, bool isDown, bool isOver,
                        juce::Colour noteFillColour) override
    {
        juce::MidiKeyboardComponent::drawBlackNote (note, g, area, isDown, isOver,
                                                    processor.isNoteHeld (note) ? colour : noteFillColour);
    }

private:
    DrikSynthProcessor& processor;
    juce::Colour colour;
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
    void rebuildPresetList();
    void choosePreset (int itemId);
    void askNameAndSave();

    DrikSynthProcessor& processor;

    juce::Label title, versionLabel;
    juce::ComboBox presetBox;
    juce::TextButton prevButton { "<" }, nextButton { ">" }, saveButton { "Salvar..." };
    juce::Array<juce::File> userFiles;
    juce::String shownName;
    std::vector<std::vector<Section*>> rows;
    juce::OwnedArray<Section> sections;
    HoldKeyboard keyboard;
    juce::ToggleButton holdButton;
    juce::TooltipWindow tooltips { this, 600 };
    juce::uint64 shownHeld[2] { 0, 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrikSynthEditor)
};
