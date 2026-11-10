#include "PluginEditor.h"
#include "Presets.h"

namespace
{
    // Azul: e a cor do DrikSynth (o DrikSampler e laranja).
    const juce::Colour bgColour     { 0xff10151c };
    const juce::Colour panelColour  { 0xff18212d };
    const juce::Colour accentColour { 0xff3fa7ff };
    const juce::Colour textColour   { 0xffe6e9ee };
    const juce::Colour dimColour    { 0xff8894a6 };

    juce::String utf8 (const char* s)   { return juce::String::fromUTF8 (s); }
}

//==============================================================================
Cell::Cell (juce::AudioProcessorValueTreeState& state, const juce::String& paramID, const juce::String& titleText)
{
    auto* param = state.getParameter (paramID);
    jassert (param != nullptr);

    label.setText (titleText, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, dimColour);
    label.setFont (juce::FontOptions (12.5f));
    label.setMinimumHorizontalScale (0.7f);
    addAndMakeVisible (label);

    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (param))
    {
        combo = std::make_unique<juce::ComboBox>();
        combo->addItemList (choice->choices, 1);
        combo->setColour (juce::ComboBox::backgroundColourId, bgColour);
        combo->setColour (juce::ComboBox::textColourId, textColour);
        combo->setColour (juce::ComboBox::outlineColourId, panelColour.brighter (0.4f));
        combo->setColour (juce::ComboBox::arrowColourId, accentColour);
        addAndMakeVisible (*combo);
        comboAttachment = std::make_unique<APVTS::ComboBoxAttachment> (state, paramID, *combo);
    }
    else if (dynamic_cast<juce::AudioParameterBool*> (param) != nullptr)
    {
        toggle = std::make_unique<juce::ToggleButton>();
        toggle->setColour (juce::ToggleButton::tickColourId, accentColour);
        toggle->setColour (juce::ToggleButton::tickDisabledColourId, dimColour);
        addAndMakeVisible (*toggle);
        buttonAttachment = std::make_unique<APVTS::ButtonAttachment> (state, paramID, *toggle);
    }
    else
    {
        slider = std::make_unique<juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow);
        slider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 66, 16);
        slider->setColour (juce::Slider::rotarySliderFillColourId, accentColour);
        slider->setColour (juce::Slider::rotarySliderOutlineColourId, panelColour.brighter (0.45f));
        slider->setColour (juce::Slider::thumbColourId, textColour);
        slider->setColour (juce::Slider::textBoxTextColourId, textColour);
        slider->setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        addAndMakeVisible (*slider);
        sliderAttachment = std::make_unique<APVTS::SliderAttachment> (state, paramID, *slider);
    }
}

void Cell::resized()
{
    auto area = getLocalBounds();
    label.setBounds (area.removeFromBottom (16));

    if (slider != nullptr)
        slider->setBounds (area);
    else if (combo != nullptr)
        combo->setBounds (area.withSizeKeepingCentre (juce::jmin (area.getWidth() - 4, 112), 24));
    else if (toggle != nullptr)
        toggle->setBounds (area.withSizeKeepingCentre (26, 26));
}

//==============================================================================
Section::Section (juce::AudioProcessorValueTreeState& state, const juce::String& titleText, std::initializer_list<Def> defs)
    : name (titleText)
{
    for (auto& d : defs)
        addAndMakeVisible (cells.add (new Cell (state, d.id, utf8 (d.title))));
}

void Section::paint (juce::Graphics& g)
{
    g.setColour (panelColour);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);

    g.setColour (accentColour);
    g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
    g.drawText (name, getLocalBounds().removeFromTop (22).reduced (10, 0), juce::Justification::centredLeft);
}

void Section::resized()
{
    auto area = getLocalBounds().reduced (6, 4);
    area.removeFromTop (20);

    const float unit = (float) area.getWidth() / juce::jmax (1.0f, weight());
    for (auto* c : cells)
        c->setBounds (area.removeFromLeft (juce::roundToInt (unit * c->weight())).reduced (1, 0));
}

//==============================================================================
DrikSynthEditor::DrikSynthEditor (DrikSynthProcessor& p)
    : AudioProcessorEditor (p),
      processor (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    title.setText ("DrikSynth", juce::dontSendNotification);
    title.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    title.setColour (juce::Label::textColourId, accentColour);
    addAndMakeVisible (title);

    versionLabel.setText (juce::String ("v") + DRIK_VERSION + "  (" + juce::Time::getCompilationDate().formatted ("%d/%m/%Y") + ")",
                          juce::dontSendNotification);
    versionLabel.setFont (juce::FontOptions (13.0f));
    versionLabel.setColour (juce::Label::textColourId, dimColour);
    addAndMakeVisible (versionLabel);

    rebuildPresetList();
    presetBox.setColour (juce::ComboBox::backgroundColourId, panelColour);
    presetBox.setColour (juce::ComboBox::textColourId, textColour);
    presetBox.setColour (juce::ComboBox::outlineColourId, accentColour.withAlpha (0.6f));
    presetBox.setColour (juce::ComboBox::arrowColourId, accentColour);
    presetBox.onChange = [this] { choosePreset (presetBox.getSelectedId()); };
    addAndMakeVisible (presetBox);

    for (auto* b : { &prevButton, &nextButton, &saveButton })
    {
        b->setColour (juce::TextButton::buttonColourId, panelColour);
        b->setColour (juce::TextButton::textColourOffId, textColour);
        addAndMakeVisible (*b);
    }
    prevButton.onClick = [this] { stepPreset (-1); };
    nextButton.onClick = [this] { stepPreset (1); };
    saveButton.onClick = [this] { askNameAndSave(); };

    auto& s = processor.apvts;
    auto add = [this, &s] (int row, const char* name, std::initializer_list<Section::Def> defs)
    {
        auto* sec = sections.add (new Section (s, utf8 (name), defs));
        addAndMakeVisible (sec);
        if ((int) rows.size() <= row) rows.resize ((size_t) row + 1);
        rows[(size_t) row].push_back (sec);
    };

    add (0, "OSC 1",   { { "o1wave", "Onda" }, { "o1oct", "Oitava" } });
    add (0, "OSC 2",   { { "o2wave", "Onda" }, { "o2semi", "Semitons" }, { "o2fine", "Fino" }, { "sync", "Sync" } });
    add (0, "PULSO",   { { "pw", "Largura" }, { "pwm", "PWM" } });
    add (0, "MIXER",   { { "mix1", "Osc 1" }, { "mix2", "Osc 2" }, { "sub", "Sub" }, { "noise", "Ru\xc3\xad" "do" } });
    add (0, "UNISON",  { { "unison", "Vozes" }, { "detune", "Desafino" }, { "spread", "Est\xc3\xa9reo" } });

    add (1, "FILTRO",  { { "fmode", "Tipo" }, { "hpf", "Passa-alta" }, { "cutoff", "Corte" }, { "reso", "Resson\xc3\xa2ncia" },
                         { "envamt", "Envelope" }, { "keytrack", "Teclado" }, { "lfocut", "LFO" } });
    add (1, "ENVELOPE DO FILTRO", { { "fa", "Ataque" }, { "fd", "Decay" }, { "fs", "Sustain" }, { "fr", "Release" } });

    add (2, "ENVELOPE DO VOLUME", { { "aa", "Ataque" }, { "ad", "Decay" }, { "as", "Sustain" }, { "ar", "Release" } });
    add (2, "LFO",     { { "lfowave", "Onda" }, { "lforate", "Velocidade" }, { "lfopitch", "Vibrato" } });
    add (2, "DRIVE",   { { "drive", "Drive" }, { "drivetone", "Tom" } });
    add (2, "CHORUS",  { { "chorus", "Tipo" }, { "chomix", "Mistura" } });
    add (2, "VOZ",     { { "glide", "Glide" }, { "mono", "Mono" } });
    add (2, "SA\xc3\x8d" "DA", { { "velsens", "Sensibilidade" }, { "volume", "Volume" } });

    keyboard.setAvailableRange (12, 120);
    keyboard.setOctaveForMiddleC (4);
    keyboard.setVelocity (1.0f, false);
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, accentColour);
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, accentColour.withAlpha (0.4f));
    addAndMakeVisible (keyboard);

    startTimerHz (10);
    setSize (1280, 690);
}

DrikSynthEditor::~DrikSynthEditor()
{
    stopTimer();
}

// Itens da lista: 1..N sao os de fabrica; de 1001 em diante, os arquivos do usuario.
static constexpr int userIdBase = 1001;

void DrikSynthEditor::rebuildPresetList()
{
    presetBox.clear (juce::dontSendNotification);

    userFiles = DrikSynthProcessor::userPresetFiles();
    if (! userFiles.isEmpty())
    {
        presetBox.addSectionHeading ("Meus sons");
        for (int i = 0; i < userFiles.size(); ++i)
            presetBox.addItem (userFiles[i].getFileNameWithoutExtension(), userIdBase + i);
    }

    const auto& all = drik::presets();
    juce::String group;
    for (int i = 0; i < (int) all.size(); ++i)
    {
        if (group != all[(size_t) i].group)
        {
            group = all[(size_t) i].group;
            presetBox.addSectionHeading (utf8 (all[(size_t) i].group));
        }
        presetBox.addItem (utf8 (all[(size_t) i].name), i + 1);
    }

    shownName = {};
    timerCallback();
}

void DrikSynthEditor::choosePreset (int itemId)
{
    if (itemId >= userIdBase)
    {
        const int i = itemId - userIdBase;
        if (i < userFiles.size())
            processor.loadPresetFrom (userFiles[i]);
    }
    else if (itemId >= 1)
    {
        processor.applyPreset (itemId - 1);
    }

    shownName = processor.getPresetName();
}

void DrikSynthEditor::timerCallback()
{
    // Mantem a lista mostrando o som em uso, inclusive quando ele muda por fora (projeto reaberto).
    const auto name = processor.getPresetName();
    if (name == shownName)
        return;

    shownName = name;
    for (int i = 0; i < presetBox.getNumItems(); ++i)
    {
        if (presetBox.getItemText (i) == name)
        {
            presetBox.setSelectedItemIndex (i, juce::dontSendNotification);
            return;
        }
    }
    presetBox.setText (name, juce::dontSendNotification);
}

void DrikSynthEditor::stepPreset (int delta)
{
    const int count = presetBox.getNumItems();
    if (count == 0)
        return;

    const int current = juce::jmax (0, presetBox.getSelectedItemIndex());
    presetBox.setSelectedItemIndex ((current + delta + count) % count, juce::sendNotificationSync);
}

void DrikSynthEditor::askNameAndSave()
{
    auto* w = new juce::AlertWindow ("Salvar som", "Nome do som:", juce::MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("name", {});
    w->addButton ("Salvar", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancelar", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    juce::Component::SafePointer<DrikSynthEditor> safe (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([safe, w] (int result)
    {
        if (safe == nullptr || result != 1)
            return;

        const auto name = juce::File::createLegalFileName (w->getTextEditorContents ("name").trim());
        if (name.isEmpty())
            return;

        const auto file = DrikSynthProcessor::userPresetFolder().getChildFile (name + ".drikpreset");
        if (safe->processor.savePresetTo (file))
            safe->rebuildPresetList();
        else
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Salvar som",
                                                    utf8 ("N\xc3\xa3" "o consegui gravar em:\n") + file.getFullPathName());
    }), true);
}

void DrikSynthEditor::paint (juce::Graphics& g)
{
    g.fillAll (bgColour);
}

void DrikSynthEditor::resized()
{
    auto area = getLocalBounds().reduced (14);

    auto top = area.removeFromTop (34);
    title.setBounds (top.removeFromLeft (125));
    versionLabel.setBounds (top.removeFromLeft (170).withTrimmedTop (5));
    saveButton.setBounds (top.removeFromRight (84).reduced (0, 3));
    top.removeFromRight (12);
    nextButton.setBounds (top.removeFromRight (34).reduced (0, 3));
    top.removeFromRight (6);
    presetBox.setBounds (top.removeFromRight (300).reduced (0, 3));
    top.removeFromRight (6);
    prevButton.setBounds (top.removeFromRight (34).reduced (0, 3));

    area.removeFromTop (10);
    keyboard.setBounds (area.removeFromBottom (80));
    keyboard.setKeyWidth ((float) keyboard.getWidth() / 64.0f);
    area.removeFromBottom (10);

    const int gap = 8;
    const int rowHeight = (area.getHeight() - gap * ((int) rows.size() - 1)) / juce::jmax (1, (int) rows.size());

    for (auto& row : rows)
    {
        auto rowArea = area.removeFromTop (rowHeight);
        area.removeFromTop (gap);

        float totalWeight = 0.0f;
        for (auto* sec : row) totalWeight += sec->weight();

        const int usable = rowArea.getWidth() - gap * ((int) row.size() - 1);
        int x = rowArea.getX();

        for (size_t i = 0; i < row.size(); ++i)
        {
            const int w = i + 1 == row.size() ? rowArea.getRight() - x
                                              : juce::roundToInt ((float) usable * row[i]->weight() / juce::jmax (1.0f, totalWeight));
            row[i]->setBounds (x, rowArea.getY(), w, rowArea.getHeight());
            x += w + gap;
        }
    }
}
