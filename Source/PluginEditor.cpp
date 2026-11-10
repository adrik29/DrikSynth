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

    // Explicacao de cada controle, mostrada quando o mouse para em cima dele.
    const char* tipFor (const juce::String& id)
    {
        static const std::pair<const char*, const char*> tips[] = {
            { "o1wave", "Formato da onda do oscilador 1. Serra: brilhante e cheia (cordas, metais). Pulso: oca, tipo clarinete; muda com a Largura. Tri\xc3\xa2" "ngulo: suave. Seno: pura, sem brilho." },
            { "o1oct", "Sobe ou desce o oscilador 1 em oitavas inteiras. -1 ou -2 para baixos." },
            { "o2wave", "Formato da onda do oscilador 2. S\xc3\xb3" " se ouve se o bot\xc3\xa3" "o Osc 2 do MIXER estiver acima de zero." },
            { "o2semi", "Afina\xc3\xa7\xc3\xa3" "o do oscilador 2 em semitons em rela\xc3\xa7\xc3\xa3" "o \xc3\xa0" " tecla. +12 dobra uma oitava acima (brilho); -12 dobra uma oitava abaixo (peso); +7 d\xc3\xa1" " uma quinta." },
            { "o2fine", "Desafina um pouco o oscilador 2. Entre 5 e 15 cents os dois osciladores batem um contra o outro e o som engorda e se move devagar." },
            { "sync", "Prende o oscilador 2 ao ciclo do oscilador 1. Com o oscilador 2 afinado bem acima, d\xc3\xa1" " um timbre rasgado e met\xc3\xa1" "lico de solo." },
            { "pw", "Largura do pulso (s\xc3\xb3" " vale para a onda Pulso). 50% \xc3\xa9" " a onda quadrada, oca. Perto de 10% ou 90% fica fina e anasalada." },
            { "pwm", "Faz o LFO balan\xc3\xa7" "ar a largura do pulso. O som fica se mexendo como se fossem v\xc3\xa1" "rios instrumentos. A Velocidade do LFO manda no ritmo." },
            { "mix1", "Volume do oscilador 1." },
            { "mix2", "Volume do oscilador 2. Em zero ele n\xc3\xa3" "o soa." },
            { "sub", "Sub-oscilador: uma onda quadrada uma oitava abaixo do oscilador 1. \xc3\x89" " o grav\xc3\xa3" "o por baixo do som." },
            { "noise", "Ru\xc3\xad" "do branco misturado ao som: sopro, chiado, vento." },
            { "unison", "Quantas c\xc3\xb3" "pias de cada nota tocam juntas (1 a 4). Mais c\xc3\xb3" "pias, som mais largo e gordo." },
            { "detune", "Quanto as c\xc3\xb3" "pias do unison s\xc3\xa3" "o desafinadas entre si. Pouco: engorda. Muito: vira um enxame." },
            { "spread", "Abertura no est\xc3\xa9" "reo. Em 0% tudo fica no centro; em 100% as vozes se espalham entre esquerda e direita." },
            { "glide", "Tempo que a afina\xc3\xa7\xc3\xa3" "o leva para escorregar de uma nota at\xc3\xa9" " a seguinte. Zero: troca na hora." },
            { "mono", "Uma nota de cada vez, como nos sintetizadores de solo e de baixo. A nota nova corta a anterior." },
            { "fmode", "Inclina\xc3\xa7\xc3\xa3" "o do filtro. 24 dB corta os agudos com for\xc3\xa7" "a (Moog, Juno, Jupiter). 12 dB corta mais leve e deixa mais brilho (Oberheim)." },
            { "hpf", "Passa-alta: tira os graves abaixo dessa frequ\xc3\xaa" "ncia. Deixa o som mais fino, como nas string machines." },
            { "cutoff", "Corte do filtro: o bot\xc3\xa3" "o mais importante do sintetizador. Tudo acima dessa frequ\xc3\xaa" "ncia \xc3\xa9" " abafado. Aberto: brilhante e \xc3\xa1" "spero. Fechado: escuro e liso." },
            { "reso", "Resson\xc3\xa2" "ncia: real\xc3\xa7" "a a regi\xc3\xa3" "o bem em cima do corte. Pouca: som nasal. Muita: assobio, o miado dos sons de sintetizador." },
            { "envamt", "Quanto o envelope do filtro abre (positivo) ou fecha (negativo) o corte a cada nota. \xc3\x89" " o que faz o 'uau' dos metais e o estalo dos baixos." },
            { "keytrack", "Faz o corte acompanhar a altura da tecla. Em 100% as notas agudas ficam t\xc3\xa3" "o brilhantes quanto as graves; em 0% as agudas ficam abafadas." },
            { "lfocut", "Faz o LFO balan\xc3\xa7" "ar o corte do filtro: o som abre e fecha sozinho, no ritmo da Velocidade do LFO." },
            { "fa", "Ataque do envelope do filtro: tempo para o filtro abrir depois que a tecla desce." },
            { "fd", "Decay do envelope do filtro: tempo para o filtro voltar a fechar at\xc3\xa9" " o n\xc3\xad" "vel do Sustain." },
            { "fs", "Sustain do envelope do filtro: onde o filtro fica enquanto a tecla est\xc3\xa1" " presa." },
            { "fr", "Release do envelope do filtro: tempo para o filtro fechar depois que a tecla sobe." },
            { "aa", "Ataque do volume: tempo para o som chegar ao m\xc3\xa1" "ximo. Curto: nota seca, de piano ou baixo. Longo: o som entra devagar, como cordas." },
            { "ad", "Decay do volume: tempo para o som cair do m\xc3\xa1" "ximo at\xc3\xa9" " o n\xc3\xad" "vel do Sustain." },
            { "as", "Sustain do volume: n\xc3\xad" "vel em que o som fica enquanto a tecla est\xc3\xa1" " presa. Em 0% a nota morre sozinha, como corda dedilhada." },
            { "ar", "Release do volume: a cauda. Tempo que o som leva para sumir depois que a tecla sobe." },
            { "lfowave", "Formato do balan\xc3\xa7" "o do LFO. Tri\xc3\xa2" "ngulo e Seno: vaiv\xc3\xa9" "m suave. Quadrada: pula entre dois valores. Aleat\xc3\xb3" "rio: um valor novo a cada ciclo." },
            { "lforate", "Velocidade do LFO, o oscilador lento que balan\xc3\xa7" "a outras coisas (Vibrato, PWM, LFO do filtro). Sozinho n\xc3\xa3" "o faz som." },
            { "lfopitch", "Vibrato: quanto o LFO balan\xc3\xa7" "a a afina\xc3\xa7\xc3\xa3" "o. Poucos cents: vibrato de cantor. Muito: sirene. A roda de modula\xc3\xa7\xc3\xa3" "o do teclado soma vibrato." },
            { "drive", "Distor\xc3\xa7\xc3\xa3" "o sobre o som inteiro, como um amplificador saturando. Em acordes o som rasga, porque as notas se misturam dentro da distor\xc3\xa7\xc3\xa3" "o." },
            { "drivetone", "Corta os agudos depois da distor\xc3\xa7\xc3\xa3" "o. S\xc3\xb3" " age quando o Drive est\xc3\xa1" " acima de zero." },
            { "chorus", "Efeito que dobra o som com c\xc3\xb3" "pias que oscilam de leve. Chorus I: lento e largo. Chorus II: mais r\xc3\xa1" "pido. Ensemble: o tremor t\xc3\xad" "pico das string machines." },
            { "chomix", "Quanto do efeito de chorus entra no som. Em 0% o efeito some." },
            { "velsens", "Quanto a for\xc3\xa7" "a da tecla muda o volume. Em 0% toda nota sai no volume cheio. Os presets n\xc3\xa3" "o mexem neste bot\xc3\xa3" "o." },
            { "volume", "Volume de sa\xc3\xad" "da. Os presets n\xc3\xa3" "o mexem neste bot\xc3\xa3" "o." },
        };
        for (auto& t : tips)
            if (id == t.first)
                return t.second;
        return "";
    }
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
    label.setTooltip (utf8 (tipFor (paramID)));
    addAndMakeVisible (label);

    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (param))
    {
        combo = std::make_unique<juce::ComboBox>();
        combo->addItemList (choice->choices, 1);
        combo->setColour (juce::ComboBox::backgroundColourId, bgColour);
        combo->setColour (juce::ComboBox::textColourId, textColour);
        combo->setColour (juce::ComboBox::outlineColourId, panelColour.brighter (0.4f));
        combo->setColour (juce::ComboBox::arrowColourId, accentColour);
        combo->setTooltip (utf8 (tipFor (paramID)));
        addAndMakeVisible (*combo);
        comboAttachment = std::make_unique<APVTS::ComboBoxAttachment> (state, paramID, *combo);
    }
    else if (dynamic_cast<juce::AudioParameterBool*> (param) != nullptr)
    {
        toggle = std::make_unique<juce::ToggleButton>();
        toggle->setColour (juce::ToggleButton::tickColourId, accentColour);
        toggle->setColour (juce::ToggleButton::tickDisabledColourId, dimColour);
        toggle->setTooltip (utf8 (tipFor (paramID)));
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
        slider->setTooltip (utf8 (tipFor (paramID)));
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
      keyboard (p, accentColour)
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

    holdButton.setButtonText ("Segurar");
    holdButton.setTooltip (utf8 ("Ligado, cada nota que voc\xc3\xaa toca fica soando at\xc3\xa9 ser tocada de novo. "
                                 "Serve para prender um acorde e mexer nos bot\xc3\xb5" "es com o mouse. Desligar solta tudo."));
    holdButton.setColour (juce::ToggleButton::textColourId, textColour);
    holdButton.setColour (juce::ToggleButton::tickColourId, accentColour);
    holdButton.setColour (juce::ToggleButton::tickDisabledColourId, dimColour);
    holdButton.setToggleState (processor.getHold(), juce::dontSendNotification);
    holdButton.onClick = [this] { processor.setHold (holdButton.getToggleState()); };
    addAndMakeVisible (holdButton);

    octaveLabel.setFont (juce::FontOptions (13.0f));
    octaveLabel.setColour (juce::Label::textColourId, dimColour);
    octaveLabel.setTooltip (utf8 ("Clique no teclado da tela e toque pelo teclado do computador: A S D F G H J K s\xc3\xa3" "o as teclas brancas, "
                                  "W E T Y U as pretas. Z desce uma oitava e X sobe."));
    addAndMakeVisible (octaveLabel);
    keyboard.onOctaveChange = [this] { showOctave(); };
    showOctave();

    startTimerHz (15);
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
    // Redesenha o teclado quando muda o conjunto de notas presas pelo Segurar.
    juce::uint64 nowHeld[2] { 0, 0 };
    for (int note = 0; note < 128; ++note)
        if (processor.isNoteHeld (note))
            nowHeld[note / 64] |= (juce::uint64) 1 << (note % 64);
    if (nowHeld[0] != shownHeld[0] || nowHeld[1] != shownHeld[1])
    {
        shownHeld[0] = nowHeld[0]; shownHeld[1] = nowHeld[1];
        keyboard.repaint();
    }

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

void DrikSynthEditor::showOctave()
{
    // A tecla A do computador toca o Do desta oitava (Do central = C4).
    octaveLabel.setText (utf8 ("Teclado do PC: A = D\xc3\xb3 ") + "C" + juce::String (keyboard.getOctave() - 1) + "   (Z / X mudam a oitava)",
                         juce::dontSendNotification);
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
    holdButton.setBounds (top.removeFromLeft (100));
    octaveLabel.setBounds (top.removeFromLeft (300).withTrimmedTop (4));
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
