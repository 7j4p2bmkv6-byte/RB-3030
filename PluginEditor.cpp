#include "PluginEditor.h"

using namespace tbui;

namespace
{
    const char* noteNames[13] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B","C'" };
    constexpr float twoPi = juce::MathConstants<float>::twoPi;

    // rotary ranges (angles clockwise from 12 o'clock)
    constexpr float knobStart = twoPi - 2.40f, knobEnd = twoPi + 2.40f;
    constexpr float groupStart = twoPi - 2.25f, groupEnd = twoPi + 2.05f;
    constexpr float modeStart = 0.80f, modeEnd = 2.40f;

    bool samePattern (const tb303::Pattern& a, const tb303::Pattern& b)
    {
        if (a.length != b.length || a.triplet != b.triplet) return false;
        for (int i = 0; i < 16; ++i)
        {
            const auto& x = a.steps[i]; const auto& y = b.steps[i];
            if (x.note != y.note || x.up != y.up || x.down != y.down || x.accent != y.accent
                || x.slide != y.slide || x.time != y.time) return false;
        }
        return true;
    }

    void text (juce::Graphics& g, const juce::String& s, float x, float y, float w, float h, float size,
               juce::Colour c = col::ink, juce::Justification j = juce::Justification::centred, bool bold = true)
    {
        g.setColour (c);
        g.setFont (font (size, bold));
        g.drawText (s, juce::Rectangle<float> (x, y, w, h), j, false);
    }
    void textC (juce::Graphics& g, const juce::String& s, float cx, float cy, float size, juce::Colour c = col::ink, bool bold = true)
    {
        text (g, s, cx - 150.0f, cy - size, 300.0f, size * 2.0f, size, c, juce::Justification::centred, bold);
    }
    void triangleUp (juce::Graphics& g, float cx, float cy, float s, juce::Colour c = col::ink)
    {
        juce::Path p; p.addTriangle (cx - s, cy + s * 0.8f, cx + s, cy + s * 0.8f, cx, cy - s * 0.9f);
        g.setColour (c); g.fillPath (p);
    }
    // small eighth note glyph
    void noteGlyph (juce::Graphics& g, float x, float y, bool filled, juce::Colour c)
    {
        g.setColour (c);
        auto head = juce::Rectangle<float> (x, y + 9, 9, 7);
        if (filled) g.fillEllipse (head); else g.drawEllipse (head, 1.4f);
        g.fillRect (x + 7.6f, y - 4.0f, 1.6f, 15.0f);
        juce::Path flag; flag.startNewSubPath (x + 9, y - 4); flag.quadraticTo (x + 15, y + 1, x + 12, y + 7);
        g.strokePath (flag, juce::PathStrokeType (1.6f));
    }
}

//==============================================================================
// StepStrip
//==============================================================================
juce::Rectangle<float> StepStrip::cellBounds (int i) const
{
    auto area = getLocalBounds().toFloat().withTrimmedTop (24.0f).reduced (2.0f, 2.0f);
    const float w = area.getWidth() / 16.0f;
    return { area.getX() + w * (float) i, area.getY(), w, area.getHeight() };
}

int StepStrip::cellAt (juce::Point<int> p) const
{
    for (int i = 0; i < 16; ++i) if (cellBounds (i).contains (p.toFloat())) return i;
    return -1;
}

bool StepStrip::refresh()
{
    int g, idx;
    auto p = proc.getShownPattern (g, idx);
    const int playing = proc.getPlayingStep();
    const int cursor = proc.getCursorStep();
    auto status = proc.statusLine();
    if (samePattern (p, shown) && g == shownGroup && idx == shownIndex && playing == lastPlaying
        && cursor == lastCursor && status == lastStatus)
        return false;
    shown = p; shownGroup = g; shownIndex = idx; lastPlaying = playing; lastCursor = cursor; lastStatus = status;
    repaint();
    return true;
}

void StepStrip::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff111111));
    g.fillRoundedRectangle (b, 5.0f);

    // status line
    g.setColour (juce::Colour (0xffe8913a));
    g.setFont (juce::Font (juce::FontOptions ("Consolas", 15.0f, juce::Font::bold)));
    g.drawText (lastStatus, b.withHeight (24.0f).reduced (10.0f, 0.0f), juce::Justification::centredLeft, true);
    g.setColour (juce::Colour (0xff8a8a85));
    g.setFont (font (11.0f, false));
    g.drawText ("click = select step   right-click = edit   wheel = note   double-click = note/rest",
                b.withHeight (24.0f).reduced (10.0f, 0.0f), juce::Justification::centredRight, true);

    const int cursor = proc.getCursorStep();
    const bool writeMode = proc.getMode() == 3;
    for (int i = 0; i < 16; ++i)
    {
        auto c = cellBounds (i).reduced (2.0f);
        const auto& s = shown.steps[i];
        const bool active = i < shown.length;
        const bool isPlaying = i == lastPlaying;
        g.setColour (isPlaying ? juce::Colour (0xff6b1a12) : (active ? juce::Colour (0xff2c2c2e) : juce::Colour (0xff1b1b1c)));
        g.fillRoundedRectangle (c, 3.0f);
        if (i % 4 == 0 && active)
        {
            g.setColour (juce::Colour (0xff444446));
            g.fillRect (c.getX(), c.getY(), c.getWidth(), 3.0f);
        }
        if (i == cursor)
        {
            g.setColour (writeMode ? juce::Colours::white : juce::Colour (0xff8a8a85));
            g.drawRoundedRectangle (c, 3.0f, writeMode ? 2.0f : 1.0f);
        }
        const float alpha = active ? 1.0f : 0.30f;
        g.setColour (juce::Colour (0xff9a9a95).withAlpha (alpha));
        g.setFont (font (10.0f, false));
        g.drawText (juce::String (i + 1), c.withHeight (14.0f).reduced (3.0f, 0.0f), juce::Justification::centredLeft, false);
        if (isPlaying)
        {
            g.setColour (col::red);
            g.fillEllipse (c.getRight() - 11.0f, c.getY() + 4.0f, 7.0f, 7.0f);
        }

        juce::String main;
        if (s.time == tb303::TimeType::Rest) main = juce::String::fromUTF8 ("\xc2\xb7");
        else if (s.time == tb303::TimeType::Tie) main = "---";
        else main = noteNames[s.note];
        g.setColour (juce::Colour (s.time == tb303::TimeType::Note ? 0xfff0f0ea : 0xff8a8a85).withAlpha (alpha));
        g.setFont (font (s.time == tb303::TimeType::Rest ? 26.0f : 17.0f));
        g.drawText (main, c.withTrimmedTop (12.0f).withHeight (24.0f), juce::Justification::centred, false);

        // flags row
        auto row = c.withTrimmedTop (c.getHeight() - 17.0f).reduced (3.0f, 2.0f);
        const float fw = row.getWidth() / 4.0f;
        auto flag = [&] (int k, const char* t, bool on, juce::Colour onCol)
        {
            auto r = juce::Rectangle<float> (row.getX() + fw * (float) k, row.getY(), fw - 1.0f, row.getHeight());
            g.setColour (on ? onCol.withAlpha (alpha) : juce::Colour (0xff3a3a3c).withAlpha (alpha));
            g.fillRoundedRectangle (r, 2.0f);
            g.setColour (on ? juce::Colours::black : juce::Colour (0xff6a6a68));
            g.setFont (font (9.0f));
            g.drawText (t, r, juce::Justification::centred, false);
        };
        flag (0, "D", s.down, juce::Colour (0xff6ab0ff));
        flag (1, "U", s.up, juce::Colour (0xff6ab0ff));
        flag (2, "A", s.accent, juce::Colour (0xffff5a40));
        flag (3, "S", s.slide, juce::Colour (0xffffc040));
    }
}

void StepStrip::mouseDown (const juce::MouseEvent& e)
{
    int i = cellAt (e.getPosition());
    if (i < 0) return;
    proc.setCursorStep (i);
    if (e.mods.isPopupMenu()) showStepMenu (i);
    refresh();
}

void StepStrip::mouseDoubleClick (const juce::MouseEvent& e)
{
    int i = cellAt (e.getPosition());
    if (i < 0 || e.mods.isPopupMenu()) return;
    proc.editShownPattern ([i] (tb303::Pattern& p)
    {
        auto& s = p.steps[i];
        s.time = (s.time == tb303::TimeType::Rest) ? tb303::TimeType::Note : tb303::TimeType::Rest;
    });
    refresh();
}

void StepStrip::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    int i = cellAt (e.getPosition());
    if (i < 0 || w.deltaY == 0.0f) return;
    const int d = w.deltaY > 0 ? 1 : -1;
    proc.editShownPattern ([i, d] (tb303::Pattern& p)
    {
        auto& s = p.steps[i];
        s.time = tb303::TimeType::Note;
        s.note = (uint8_t) juce::jlimit (0, 12, (int) s.note + d);
    });
    refresh();
}

void StepStrip::showStepMenu (int i)
{
    const auto& s = shown.steps[i];
    juce::PopupMenu notes;
    for (int k = 0; k < 13; ++k)
        notes.addItem (100 + k, noteNames[k], true, s.time == tb303::TimeType::Note && s.note == k);
    juce::PopupMenu m;
    m.addSectionHeader ("Step " + juce::String (i + 1));
    m.addSubMenu ("Note", notes);
    m.addItem (1, "Up (+1 octave)", true, s.up);
    m.addItem (2, "Down (-1 octave)", true, s.down);
    m.addItem (3, "Accent", true, s.accent);
    m.addItem (4, "Slide", true, s.slide);
    m.addSeparator();
    m.addItem (5, "Note", true, s.time == tb303::TimeType::Note);
    m.addItem (6, "Tie", true, s.time == tb303::TimeType::Tie);
    m.addItem (7, "Rest", true, s.time == tb303::TimeType::Rest);
    m.addSeparator();
    m.addItem (8, "Pattern ends at this step");
    m.addItem (9, "Full length (16 steps)");
    m.addItem (10, "Triplet timing", true, shown.triplet);

    juce::Component::SafePointer<StepStrip> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [safe, i] (int r)
    {
        if (safe == nullptr || r == 0) return;
        safe->proc.editShownPattern ([i, r] (tb303::Pattern& p)
        {
            auto& s = p.steps[i];
            if (r >= 100) { s.note = (uint8_t) (r - 100); s.time = tb303::TimeType::Note; }
            else switch (r)
            {
                case 1: s.up = ! s.up; if (s.up) s.down = false; break;
                case 2: s.down = ! s.down; if (s.down) s.up = false; break;
                case 3: s.accent = ! s.accent; break;
                case 4: s.slide = ! s.slide; break;
                case 5: s.time = tb303::TimeType::Note; break;
                case 6: s.time = tb303::TimeType::Tie; break;
                case 7: s.time = tb303::TimeType::Rest; break;
                case 8: p.length = i + 1; break;
                case 9: p.length = 16; break;
                case 10: p.triplet = ! p.triplet; break;
                default: break;
            }
        });
        safe->refresh();
    });
}

//==============================================================================
// PanelView
//==============================================================================
PanelView::PanelView (TB3030Processor& p) : proc (p), strip (p)
{
    setLookAndFeel (&lnf);

    makeKnob (tuning,    "tuning",    { 470, 98 }, 92, "knob");
    makeKnob (cutoff,    "cutoff",    { 585, 98 }, 92, "knob");
    makeKnob (resonance, "resonance", { 700, 98 }, 92, "knob");
    makeKnob (envmod,    "envmod",    { 815, 98 }, 92, "knob");
    makeKnob (decay,     "decay",     { 930, 98 }, 92, "knob");
    makeKnob (accent,    "accent",    { 1045, 98 }, 92, "knob");
    makeKnob (tempo,     "tempo",     { 240, 272 }, 136, "big");
    makeKnob (volume,    "volume",    { 1355, 272 }, 136, "big");
    makeKnob (drive,     "drive",     { 1250, 922 }, 64, "knob");
    makeKnob (shuffle,   "shuffle",   { 1335, 922 }, 64, "knob");
    tuning.setDoubleClickReturnValue (true, 0.0);

    // rotary switches (panel state, not automatable)
    auto setupSwitch = [this] (juce::Slider& s, double maxV, float a0, float a1, juce::Point<int> c)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setRange (0.0, maxV, 1.0);
        s.setRotaryParameters (a0, a1, true);
        s.setMouseDragSensitivity (120);
        s.getProperties().set ("style", "switch");
        s.setBounds (juce::Rectangle<int> (118, 118).withCentre (c));
        s.setMouseClickGrabsKeyboardFocus (false);
        addAndMakeVisible (s);
    };
    setupSwitch (groupSw, 6.0, groupStart, groupEnd, { 465, 286 });
    setupSwitch (modeSw, 3.0, modeStart, modeEnd, { 690, 286 });
    groupSw.setValue (proc.getGroupKnob(), juce::dontSendNotification);
    modeSw.setValue (proc.getMode(), juce::dontSendNotification);
    groupSw.onValueChange = [this] { proc.setGroupKnob ((int) groupSw.getValue()); };
    modeSw.onValueChange  = [this] { proc.setMode ((int) modeSw.getValue()); };

    waveSw.setBounds (160, 58, 52, 22);
    addAndMakeVisible (waveSw);
    buttonAtt.push_back (std::make_unique<BA> (proc.apvts, "square", waveSw));
    clockSw.setBounds (1400, 912, 56, 22);
    addAndMakeVisible (clockSw);
    buttonAtt.push_back (std::make_unique<BA> (proc.apvts, "hostsync", clockSw));
    waveSw.setTooltip ("Waveform: saw / square");
    clockSw.setTooltip ("Clock: internal tempo / follow the DAW transport");

    // ---- keys
    const int whiteN[8] = { 0, 2, 4, 5, 7, 9, 11, 12 };
    const int whiteX[8] = { 390, 475, 560, 645, 727, 812, 897, 982 };
    const int blackN[5] = { 1, 3, 6, 8, 10 };
    const int blackX[5] = { 433, 517, 687, 771, 856 };
    for (int i = 0; i < 8; ++i)
        addKey ((PanelBtn) whiteN[i], juce::Rectangle<int> (52, 38).withCentre ({ whiteX[i], 597 }), { whiteX[i], 541 });
    for (int i = 0; i < 5; ++i)
        addKey ((PanelBtn) blackN[i], juce::Rectangle<int> (44, 40).withCentre ({ blackX[i], 486 }), { blackX[i], 439 });

    addKey (PanelBtn::PatternClear, juce::Rectangle<int> (64, 32).withCentre ({ 113, 487 }), { -1, -1 });
    addKey (PanelBtn::RunStop,      { 66, 560, 100, 54 },                                   { 112, 546 });
    addKey (PanelBtn::PitchMode,    juce::Rectangle<int> (64, 32).withCentre ({ 270, 487 }), { 300, 442 });
    addKey (PanelBtn::Function,     juce::Rectangle<int> (64, 32).withCentre ({ 255, 590 }), { 318, 541 });
    addKey (PanelBtn::TimeMode,     juce::Rectangle<int> (64, 32).withCentre ({ 1318, 463 }), { 1197, 437 });
    addKey (PanelBtn::Down,         juce::Rectangle<int> (52, 38).withCentre ({ 1070, 597 }), { 1070, 541 });
    addKey (PanelBtn::Up,           juce::Rectangle<int> (52, 38).withCentre ({ 1155, 597 }), { 1155, 541 });
    addKey (PanelBtn::Accent,       juce::Rectangle<int> (52, 38).withCentre ({ 1240, 597 }), { 1240, 541 });
    addKey (PanelBtn::Slide,        juce::Rectangle<int> (52, 38).withCentre ({ 1325, 597 }), { 1325, 541 });
    addKey (PanelBtn::Back,         juce::Rectangle<int> (64, 32).withCentre ({ 1447, 488 }), { -1, -1 });
    addKey (PanelBtn::WriteNext,    { 1396, 560, 102, 54 },                                 { -1, -1 });

    // ---- memory rail
    soundLcd.setBounds (30, 794, 425, 84);
    addAndMakeVisible (soundLcd);
    strip.setBounds (30, 888, 1150, 84);
    addAndMakeVisible (strip);

    auto setupBtn = [this] (juce::TextButton& b, juce::Rectangle<int> r)
    {
        b.setBounds (r);
        b.setMouseClickGrabsKeyboardFocus (false);
        addAndMakeVisible (b);
    };
    setupBtn (bankPrev, { 475, 808, 38, 40 });
    setupBtn (bankNext, { 517, 808, 38, 40 });
    bankPrev.onClick = [this] { proc.selectBank ((proc.getBankIndex() - 1 + proc.banks.numBanks()) % proc.banks.numBanks()); };
    bankNext.onClick = [this] { proc.selectBank ((proc.getBankIndex() + 1) % proc.banks.numBanks()); };
    for (int i = 0; i < 4; ++i)
    {
        groupBtns[i].setButtonText (juce::String::charToString ((juce::juce_wchar) ('A' + i)));
        setupBtn (groupBtns[i], { 575 + i * 44, 808, 40, 40 });
        groupBtns[i].onClick = [this, i] { proc.selectSlot (i * 8 + proc.getSlot() % 8); };
    }
    for (int i = 0; i < 8; ++i)
    {
        numBtns[i].setButtonText (juce::String (i + 1));
        setupBtn (numBtns[i], { 765 + i * 44, 808, 40, 40 });
        numBtns[i].onClick = [this, i] { proc.selectSlot ((proc.getSlot() / 8) * 8 + i); };
    }
    setupBtn (slotPrev, { 1135, 808, 38, 40 });
    setupBtn (slotNext, { 1177, 808, 38, 40 });
    slotPrev.onClick = [this] { proc.selectSlot ((proc.getSlot() + 31) % 32); };
    slotNext.onClick = [this] { proc.selectSlot ((proc.getSlot() + 1) % 32); };
    setupBtn (writeBtn,    { 1233, 808, 88, 40 });
    setupBtn (bankMenuBtn, { 1331, 808, 100, 40 });
    setupBtn (pattMenuBtn, { 1441, 808, 104, 40 });
    writeBtn.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff8e2a1f));
    writeBtn.onClick    = [this] { writeDialog(); };
    bankMenuBtn.onClick = [this] { showBankMenu(); };
    pattMenuBtn.onClick = [this] { showPatternMenu(); };

    refresh();
}

PanelView::~PanelView()
{
    setLookAndFeel (nullptr);
}

void PanelView::makeKnob (juce::Slider& s, const char* paramId, juce::Point<int> centre, int size, const char* style)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    s.setRotaryParameters (knobStart, knobEnd, true);
    s.getProperties().set ("style", style);
    s.setBounds (juce::Rectangle<int> (size, size).withCentre (centre));
    s.setPopupDisplayEnabled (true, true, this);
    s.setMouseClickGrabsKeyboardFocus (false);
    addAndMakeVisible (s);
    sliderAtt.push_back (std::make_unique<SA> (proc.apvts, paramId, s));
}

void PanelView::addKey (PanelBtn id, juce::Rectangle<int> r, juce::Point<int> ledPos)
{
    KeyItem item;
    item.id = id;
    item.key = std::make_unique<PanelKey>();
    item.key->setBounds (r);
    item.key->onPress = [this, id] (bool shift) { proc.pressButton (id, shift); refresh(); };
    addAndMakeVisible (*item.key);
    if (ledPos.x >= 0)
    {
        item.led = std::make_unique<Led>();
        item.led->setBounds (juce::Rectangle<int> (22, 22).withCentre (ledPos));
        addAndMakeVisible (*item.led);
    }
    keys.push_back (std::move (item));
}

void PanelView::refresh()
{
    for (auto& k : keys)
        if (k.led != nullptr) k.led->setOn (proc.ledOn (k.id));

    if ((int) modeSw.getValue() != proc.getMode()) modeSw.setValue (proc.getMode(), juce::dontSendNotification);
    if ((int) groupSw.getValue() != proc.getGroupKnob()) groupSw.setValue (proc.getGroupKnob(), juce::dontSendNotification);

    const auto& bank = proc.banks.bank (proc.getBankIndex());
    juce::StringArray lines;
    lines.add ("BANK " + juce::String (proc.getBankIndex() + 1) + ": " + bank.name + (bank.isFactory ? "  (read-only)" : ""));
    lines.add (SoundBank::slotName (proc.getSlot()) + "  " + proc.currentSoundName() + (proc.isSoundModified() ? " *" : ""));
    lines.add (juce::String (proc.currentBpm(), 1) + " BPM" + (proc.isRunning() ? "   RUN" : "   STOP"));
    soundLcd.setLines (lines, { 14.0f, 22.0f, 13.0f });

    const int slot = proc.getSlot();
    for (int i = 0; i < 4; ++i) groupBtns[i].setToggleState (slot / 8 == i, juce::dontSendNotification);
    for (int i = 0; i < 8; ++i) numBtns[i].setToggleState (slot % 8 == i, juce::dontSendNotification);

    strip.refresh();
}

void PanelView::message (const juce::String& title, const juce::String& txt)
{
    juce::AlertWindow::showAsync (juce::MessageBoxOptions()
                                      .withIconType (juce::MessageBoxIconType::InfoIcon)
                                      .withTitle (title).withMessage (txt).withButton ("OK"),
                                  nullptr);
}

void PanelView::writeDialog()
{
    auto* aw = new juce::AlertWindow ("Write sound",
                                      proc.banks.bank (proc.getBankIndex()).isFactory
                                          ? "The factory bank is read-only - the sound will be written to User Bank 1."
                                          : "Write the current panel sound to the memory slot.",
                                      juce::MessageBoxIconType::NoIcon);
    juce::StringArray slots;
    for (int i = 0; i < SoundBank::numSlots; ++i) slots.add (SoundBank::slotName (i));
    aw->addComboBox ("slot", slots, "Slot");
    if (auto* cb = aw->getComboBoxComponent ("slot")) cb->setSelectedItemIndex (proc.getSlot());
    aw->addTextEditor ("name", proc.currentSoundName(), "Name");
    aw->addButton ("Write", 1, juce::KeyPress (juce::KeyPress::returnKey));
    aw->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    juce::Component::SafePointer<PanelView> safe (this);
    aw->enterModalState (true, juce::ModalCallbackFunction::create ([safe, aw] (int r)
    {
        if (safe == nullptr || r != 1) return;
        int slot = 0;
        if (auto* cb = aw->getComboBoxComponent ("slot")) slot = juce::jmax (0, cb->getSelectedItemIndex());
        safe->proc.writeSound (slot, aw->getTextEditorContents ("name").trim());
        safe->refresh();
    }), true);
}

void PanelView::showBankMenu()
{
    const bool factory = proc.banks.bank (proc.getBankIndex()).isFactory;
    juce::PopupMenu m;
    m.addItem (1, "New bank (copy of factory sounds)");
    m.addItem (2, "Import bank...");
    m.addItem (3, "Export this bank...");
    m.addItem (4, "Rename this bank...", ! factory);
    m.addItem (5, "Delete this bank", ! factory);
    m.addSeparator();
    m.addItem (6, "Rescan banks folder");
    m.addItem (7, "Show banks folder");

    juce::Component::SafePointer<PanelView> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&bankMenuBtn), [safe] (int r)
    {
        if (safe == nullptr || r == 0) return;
        auto& self = *safe.getComponent();
        auto& pr = self.proc;
        switch (r)
        {
            case 1: pr.newBank(); break;
            case 2:
                self.chooser = std::make_unique<juce::FileChooser> ("Import TB-3030 bank", BankManager::bankFolder(), "*.tb3bank");
                self.chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [safe] (const juce::FileChooser& fc)
                    {
                        auto f = fc.getResult();
                        if (safe == nullptr || f == juce::File()) return;
                        if (! safe->proc.importBankFile (f)) safe->message ("Import failed", "That file is not a TB-3030 bank.");
                        safe->refresh();
                    });
                break;
            case 3:
            {
                auto def = juce::File::getSpecialLocation (juce::File::userDesktopDirectory)
                               .getChildFile (juce::File::createLegalFileName (pr.banks.bank (pr.getBankIndex()).name) + ".tb3bank");
                self.chooser = std::make_unique<juce::FileChooser> ("Export TB-3030 bank", def, "*.tb3bank");
                self.chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                                               | juce::FileBrowserComponent::warnAboutOverwriting,
                    [safe] (const juce::FileChooser& fc)
                    {
                        auto f = fc.getResult();
                        if (safe == nullptr || f == juce::File()) return;
                        if (! safe->proc.exportCurrentBank (f.withFileExtension ("tb3bank")))
                            safe->message ("Export failed", "Could not write " + f.getFullPathName());
                    });
                break;
            }
            case 4:
            {
                auto* aw = new juce::AlertWindow ("Rename bank", "New name for this bank:", juce::MessageBoxIconType::NoIcon);
                aw->addTextEditor ("name", pr.banks.bank (pr.getBankIndex()).name);
                aw->addButton ("Rename", 1, juce::KeyPress (juce::KeyPress::returnKey));
                aw->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
                aw->enterModalState (true, juce::ModalCallbackFunction::create ([safe, aw] (int res)
                {
                    if (safe == nullptr || res != 1) return;
                    auto name = aw->getTextEditorContents ("name").trim();
                    if (name.isEmpty()) return;
                    auto& bk = safe->proc.banks.bank (safe->proc.getBankIndex());
                    bk.name = name; bk.save();
                    safe->refresh();
                }), true);
                break;
            }
            case 5:
            {
                auto* aw = new juce::AlertWindow ("Delete bank",
                                                  "Delete \"" + pr.banks.bank (pr.getBankIndex()).name + "\"? The file is moved to the recycle bin.",
                                                  juce::MessageBoxIconType::WarningIcon);
                aw->addButton ("Delete", 1);
                aw->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
                aw->enterModalState (true, juce::ModalCallbackFunction::create ([safe] (int res)
                {
                    if (safe == nullptr || res != 1) return;
                    safe->proc.deleteCurrentBank();
                    safe->refresh();
                }), true);
                break;
            }
            case 6:
            {
                auto name = pr.banks.bank (pr.getBankIndex()).name;
                pr.banks.rescan();
                int bi = pr.banks.indexOf (name);
                pr.selectBank (bi >= 0 ? bi : 0);
                break;
            }
            case 7:
                BankManager::bankFolder().createDirectory();
                BankManager::bankFolder().startAsProcess();
                break;
            default: break;
        }
        self.refresh();
    });
}

void PanelView::showPatternMenu()
{
    juce::PopupMenu m;
    m.addSectionHeader ("Pattern shown in the step display");
    m.addItem (1, "Copy pattern");
    m.addItem (2, "Paste pattern");
    m.addItem (3, "Randomise (acid generator)");
    m.addItem (4, "Clear pattern");
    m.addSeparator();
    m.addSectionHeader ("All patterns and tracks");
    m.addItem (5, "Save patterns & tracks...");
    m.addItem (6, "Load patterns & tracks...");
    m.addItem (7, "Restore factory patterns");

    juce::Component::SafePointer<PanelView> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&pattMenuBtn), [safe] (int r)
    {
        if (safe == nullptr || r == 0) return;
        auto& self = *safe.getComponent();
        auto& pr = self.proc;
        switch (r)
        {
            case 1: pr.copyPattern(); break;
            case 2: pr.pastePattern(); break;
            case 3: pr.randomisePattern(); break;
            case 4: pr.editShownPattern ([] (tb303::Pattern& p) { p.clear(); }); break;
            case 5:
            {
                BankManager::patternFolder().createDirectory();
                self.chooser = std::make_unique<juce::FileChooser> ("Save patterns", BankManager::patternFolder().getChildFile ("My Patterns.tb3pat"), "*.tb3pat");
                self.chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                                               | juce::FileBrowserComponent::warnAboutOverwriting,
                    [safe] (const juce::FileChooser& fc)
                    {
                        auto f = fc.getResult();
                        if (safe == nullptr || f == juce::File()) return;
                        if (! safe->proc.savePatternsToFile (f.withFileExtension ("tb3pat")))
                            safe->message ("Save failed", "Could not write " + f.getFullPathName());
                    });
                break;
            }
            case 6:
                BankManager::patternFolder().createDirectory();
                self.chooser = std::make_unique<juce::FileChooser> ("Load patterns", BankManager::patternFolder(), "*.tb3pat");
                self.chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [safe] (const juce::FileChooser& fc)
                    {
                        auto f = fc.getResult();
                        if (safe == nullptr || f == juce::File()) return;
                        if (! safe->proc.loadPatternsFromFile (f)) safe->message ("Load failed", "That file is not a TB-3030 pattern file.");
                        safe->refresh();
                    });
                break;
            case 7:
            {
                auto* aw = new juce::AlertWindow ("Restore factory patterns",
                                                  "Replace all 64 patterns and 7 tracks with the factory set?",
                                                  juce::MessageBoxIconType::WarningIcon);
                aw->addButton ("Restore", 1);
                aw->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
                aw->enterModalState (true, juce::ModalCallbackFunction::create ([safe] (int res)
                {
                    if (safe == nullptr || res != 1) return;
                    safe->proc.restoreFactoryPatterns();
                    safe->refresh();
                }), true);
                break;
            }
            default: break;
        }
        self.refresh();
    });
}

//------------------------------------------------------------------------------
void PanelView::drawRotaryLabels (juce::Graphics& g)
{
    // PATT.GROUP / TRACK positions 1..7 with group brackets
    {
        const juce::Point<float> c (465.0f, 286.0f);
        const float r = 74.0f;
        for (int i = 0; i < 7; ++i)
        {
            float a = groupStart + (groupEnd - groupStart) * (float) i / 6.0f;
            juce::Point<float> p (c.x + r * std::sin (a), c.y - r * std::cos (a));
            auto box = juce::Rectangle<float> (18.0f, 18.0f).withCentre (p);
            g.setColour (col::ink);
            g.drawRect (box, 1.2f);
            text (g, juce::String (i + 1), box.getX(), box.getY(), box.getWidth(), box.getHeight(), 13.0f);
            juce::Point<float> d (c.x + (r - 16) * std::sin (a), c.y - (r - 16) * std::cos (a));
            g.fillEllipse (juce::Rectangle<float> (4.0f, 4.0f).withCentre (d));
        }
        // group labels
        auto lab = [&] (const char* t, float a, float rr)
        {
            juce::Point<float> p (c.x + rr * std::sin (a), c.y - rr * std::cos (a));
            textC (g, t, p.x, p.y, 14.0f);
        };
        const float st = (groupEnd - groupStart) / 6.0f;
        lab ("I",   groupStart + st * 0.5f, 98.0f);
        lab ("II",  groupStart + st * 2.5f, 98.0f);
        lab ("III", groupStart + st * 4.5f, 98.0f);
        lab ("IV",  groupStart + st * 6.0f, 100.0f);
    }
    // MODE positions
    {
        const juce::Point<float> c (690.0f, 286.0f);
        const char* names[4] = { "WRITE", "PLAY", "PLAY", "WRITE" };
        for (int i = 0; i < 4; ++i)
        {
            float a = modeStart + (modeEnd - modeStart) * (float) i / 3.0f;
            juce::Point<float> d (c.x + 66.0f * std::sin (a), c.y - 66.0f * std::cos (a));
            g.setColour (col::ink);
            g.fillEllipse (juce::Rectangle<float> (5.0f, 5.0f).withCentre (d));
            juce::Point<float> p (c.x + 74.0f * std::sin (a), c.y - 74.0f * std::cos (a));
            bool boxed = i <= 1;
            auto r = juce::Rectangle<float> (p.x, p.y - 10.0f, 60.0f, 20.0f);
            if (boxed)
            {
                g.setColour (col::black);
                g.fillRect (r);
                text (g, names[i], r.getX(), r.getY(), r.getWidth(), r.getHeight(), 14.0f, juce::Colours::white);
            }
            else text (g, names[i], r.getX(), r.getY(), r.getWidth(), r.getHeight(), 14.0f);
        }
        // brackets
        g.setColour (col::ink);
        auto bracket = [&] (float y1, float y2, const char* t, bool boxed)
        {
            g.drawLine (835, y1, 845, y1, 1.4f); g.drawLine (845, y1, 845, y2, 1.4f); g.drawLine (835, y2, 845, y2, 1.4f);
            g.drawLine (845, (y1 + y2) * 0.5f, 852, (y1 + y2) * 0.5f, 1.4f);
            auto r = juce::Rectangle<float> (854, (y1 + y2) * 0.5f - 10, 72, 20);
            if (boxed) { g.setColour (col::black); g.fillRect (r); text (g, t, r.getX(), r.getY(), r.getWidth(), r.getHeight(), 13.0f, juce::Colours::white); }
            else text (g, t, r.getX(), r.getY(), r.getWidth(), r.getHeight(), 13.0f);
            g.setColour (col::ink);
        };
        juce::ignoreUnused (bracket);
    }
}

void PanelView::paint (juce::Graphics& g)
{
    // ===================== body
    g.fillAll (juce::Colour (0xff9a9a96));
    auto body = juce::Rectangle<float> (6, 6, 1548, 768);
    juce::ColourGradient metal (col::panelTop, 0, 6, col::panelBottom, 0, 774, false);
    g.setGradientFill (metal);
    g.fillRoundedRectangle (body, 8.0f);
    // brushed texture
    juce::Random rnd (303);
    for (int i = 0; i < 380; ++i)
    {
        float y = 8.0f + rnd.nextFloat() * 760.0f;
        g.setColour ((rnd.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (0.025f));
        g.drawHorizontalLine ((int) y, 10.0f, 1550.0f);
    }
    g.setColour (juce::Colour (0xff8a8a86));
    g.drawRoundedRectangle (body, 8.0f, 2.0f);
    g.setColour (juce::Colour (0xff7d7d79));
    g.drawLine (8, 152, 1552, 152, 2.0f);
    g.drawLine (8, 380, 1552, 380, 2.0f);
    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.drawLine (8, 154, 1552, 154, 1.0f);
    g.drawLine (8, 382, 1552, 382, 1.0f);

    // ===================== top strip
    g.setColour (juce::Colour (0xff7d7d79));
    g.drawLine (400, 16, 400, 146, 1.6f);
    g.drawLine (1110, 16, 1110, 146, 1.6f);

    triangleUp (g, 72, 22, 6);  textC (g, "MIX IN", 72, 38, 13.0f);
    textC (g, "WAVEFORM", 186, 38, 13.0f);
    // waveform symbols (saw / square)
    {
        g.setColour (col::ink);
        juce::Path saw; saw.startNewSubPath (152, 30); saw.lineTo (160, 18); saw.lineTo (160, 30); saw.lineTo (168, 18); saw.lineTo (168, 30);
        juce::Path sq; sq.startNewSubPath (204, 30); sq.lineTo (204, 19); sq.lineTo (211, 19); sq.lineTo (211, 30); sq.lineTo (218, 30); sq.lineTo (218, 19);
        g.strokePath (saw, juce::PathStrokeType (1.6f));
        g.strokePath (sq, juce::PathStrokeType (1.6f));
        text (g, "SAW", 150, 82, 30, 14, 10.0f); text (g, "SQR", 184, 82, 30, 14, 10.0f);
    }
    triangleUp (g, 310, 22, 6); textC (g, "SYNC IN", 310, 38, 13.0f);

    // brand logo
    {
        auto box = juce::Rectangle<float> (70, 98, 46, 40);
        g.setColour (col::ink);
        g.drawRoundedRectangle (box, 9.0f, 3.2f);
        text (g, "LA", box.getX(), box.getY(), box.getWidth(), box.getHeight(), 22.0f);
        g.setFont (juce::Font (juce::FontOptions ("Arial", 36.0f, juce::Font::bold)).withHorizontalScale (0.92f));
        g.drawText ("Landgren", juce::Rectangle<float> (124, 92, 230, 52), juce::Justification::centredLeft, false);
        text (g, "AUDIO", 290, 128, 70, 16, 11.0f, col::ink, juce::Justification::centredRight);
    }

    const char* topLabels[6] = { "TUNING", "CUT OFF FREQ", "RESONANCE", "ENV MOD", "DECAY", "ACCENT" };
    for (int i = 0; i < 6; ++i)
    {
        float x = 470.0f + 115.0f * (float) i;
        textC (g, topLabels[i], x, 30, 14.0f);
        g.setColour (col::ink);
        g.fillRect (x - 4, 44.0f, 8.0f, 5.0f);
    }

    const char* rightLabels[5] = { "CV", "GATE", "HEADPHONE", "OUTPUT", "DC 9V" };
    const float rightX[5] = { 1140, 1215, 1302, 1392, 1482 };
    for (int i = 0; i < 5; ++i) { triangleUp (g, rightX[i], 22, 6); textC (g, rightLabels[i], rightX[i], 40, 13.0f); }
    g.setColour (col::ink);
    g.setFont (juce::Font (juce::FontOptions ("Arial", 50.0f, juce::Font::bold)).withHorizontalScale (0.95f));
    g.drawText ("Bass Line", juce::Rectangle<float> (1160, 82, 330, 60), juce::Justification::centred, false);

    // ===================== middle section
    textC (g, "TEMPO", 240, 178, 17.0f);
    text (g, "SLOW", 150, 338, 70, 20, 12.0f); text (g, "FAST", 262, 338, 70, 20, 12.0f);
    {
        auto r = juce::Rectangle<float> (378, 167, 64, 22);
        g.setColour (col::black); g.fillRect (r);
        text (g, "TRACK", r.getX(), r.getY(), r.getWidth(), r.getHeight(), 14.0f, juce::Colours::white);
        text (g, "PATT. GROUP", 445, 167, 130, 22, 16.0f, col::ink, juce::Justification::centredLeft);
    }
    textC (g, "MODE", 690, 178, 17.0f);
    drawRotaryLabels (g);
    {
        // TRACK / PATTERN brackets for mode labels
        g.setColour (col::ink);
        g.drawLine (826, 232, 836, 232, 1.4f); g.drawLine (836, 232, 836, 272, 1.4f); g.drawLine (826, 272, 836, 272, 1.4f);
        g.drawLine (836, 252, 846, 252, 1.4f);
        auto r = juce::Rectangle<float> (848, 242, 70, 20);
        g.setColour (col::black); g.fillRect (r);
        text (g, "TRACK", r.getX(), r.getY(), r.getWidth(), r.getHeight(), 13.0f, juce::Colours::white);
        g.setColour (col::ink);
        g.drawLine (826, 310, 836, 310, 1.4f); g.drawLine (836, 310, 836, 350, 1.4f); g.drawLine (826, 350, 836, 350, 1.4f);
        g.drawLine (836, 330, 846, 330, 1.4f);
        text (g, "PATTERN", 848, 320, 80, 20, 13.0f, col::ink, juce::Justification::centredLeft);
    }
    // model name
    g.setColour (col::ink);
    g.setFont (juce::Font (juce::FontOptions ("Arial", 58.0f, juce::Font::plain)).withHorizontalScale (0.95f));
    g.drawText ("TB-3030", juce::Rectangle<float> (950, 222, 290, 66), juce::Justification::centredRight, false);
    g.fillRect (938.0f, 292.0f, 304.0f, 2.5f);
    g.setFont (juce::Font (juce::FontOptions ("Arial", 36.0f, juce::Font::plain)).withHorizontalScale (0.92f));
    g.drawText ("Computer Controlled", juce::Rectangle<float> (930, 296, 312, 46), juce::Justification::centred, false);

    textC (g, "VOLUME", 1355, 178, 17.0f);
    text (g, "POWER SW  OFF", 1200, 338, 140, 20, 12.0f, col::ink, juce::Justification::centredRight);
    text (g, "MAX", 1408, 338, 60, 20, 12.0f);

    // ===================== bottom (keyboard) section
    auto lower = juce::Rectangle<float> (20, 390, 1520, 356);
    g.setColour (juce::Colour (0xff9b9b97));
    g.fillRoundedRectangle (lower, 6.0f);
    g.setColour (juce::Colour (0xffc9c9c4));
    g.fillRoundedRectangle (lower.reduced (6.0f), 4.0f);

    auto boxOutline = [&] (juce::Rectangle<float> r) { g.setColour (col::ink); g.drawRect (r, 1.6f); };
    // left boxes
    boxOutline ({ 34, 400, 160, 120 });
    boxOutline ({ 34, 524, 160, 210 });
    {
        auto dc = juce::Rectangle<float> (46, 410, 30, 20);
        g.drawRect (dc, 1.2f);
        text (g, "D.C.", dc.getX(), dc.getY(), dc.getWidth(), dc.getHeight(), 12.0f, col::ink, juce::Justification::centred, false);
        auto br = juce::Rectangle<float> (80, 410, 100, 20);
        g.drawRect (br, 1.2f);
        text (g, "BAR RESET", br.getX(), br.getY(), br.getWidth(), br.getHeight(), 12.0f);
        textC (g, "PATTERN CLEAR", 113, 452, 13.0f);
        text (g, "RUN", 50, 536, 40, 20, 12.0f);
        text (g, "BATTERY", 128, 536, 64, 20, 12.0f);
        textC (g, "RUN/STOP", 113, 640, 14.0f);
    }
    // pitch mode / function
    {
        auto pm = juce::Rectangle<float> (196, 400, 152, 120);
        g.setColour (col::black); g.fillRect (pm);
        text (g, "PITCH MODE", pm.getX(), pm.getY() + 6, pm.getWidth(), 22, 16.0f, juce::Colours::white);
        boxOutline ({ 196, 524, 152, 210 });
        text (g, "FUNCTION", 200, 530, 110, 22, 16.0f);
        text (g, "NORMAL", 290, 562, 60, 16, 11.0f);
        text (g, "MODE", 290, 576, 60, 16, 11.0f);
        auto bar = juce::Rectangle<float> (226, 622, 60, 22);
        g.setColour (col::black); g.fillRect (bar);
        text (g, "BAR", bar.getX(), bar.getY(), bar.getWidth(), bar.getHeight(), 14.0f, juce::Colours::white);
    }
    // keyboard area: black header and black key columns
    {
        g.setColour (col::black);
        g.fillRect (348.0f, 400.0f, 682.0f, 26.0f);
        const float whiteX[8] = { 390, 475, 560, 645, 727, 812, 897, 982 };
        const char* whiteNames[8] = { "C", "D", "E", "F", "G", "A", "B", "C" };
        const float blackX[5] = { 433, 517, 687, 771, 856 };
        const char* blackNames[5] = { "C#", "D#", "F#", "G#", "A#" };
        for (int i = 0; i < 5; ++i)
            g.fillRect (blackX[i] - 38.0f, 400.0f, 76.0f, 140.0f);
        for (int i = 0; i < 8; ++i)
            text (g, whiteNames[i], whiteX[i] - 20, 404, 40, 20, 15.0f, juce::Colours::white);
        for (int i = 0; i < 5; ++i)
            text (g, blackNames[i], blackX[i] - 20, 404, 40, 20, 15.0f, juce::Colours::white);
        // DEL / INS tags
        auto tag = [&] (float cx, const char* t)
        {
            auto r = juce::Rectangle<float> (cx - 26, 514, 52, 20);
            g.setColour (juce::Colour (0xffd9d9d4)); g.fillRect (r);
            text (g, t, r.getX(), r.getY(), r.getWidth(), r.getHeight(), 13.0f);
        };
        tag (433, "DEL"); tag (517, "INS");
        // separators between white keys
        g.setColour (col::ink.withAlpha (0.6f));
        for (int i = 0; i < 7; ++i)
        {
            float x = (whiteX[i] + whiteX[i + 1]) * 0.5f;
            g.drawLine (x, 540, x, 620, 1.0f);
        }
        // PATTERN row
        g.setColour (col::black);
        g.fillRect (285.0f, 626.0f, 745.0f, 30.0f);
        text (g, "PATTERN", 292, 628, 90, 26, 16.0f, col::redText, juce::Justification::centredLeft);
        for (int i = 0; i < 8; ++i)
            text (g, juce::String (i + 1), whiteX[i] - 20, 628, 40, 26, 18.0f, col::redText);
        // SELECTOR row
        text (g, "SELECTOR", 240, 666, 110, 24, 16.0f, col::ink, juce::Justification::centredLeft);
        const char* sel[8] = { "1", "2", "3", "4", "5", "6", "7", "8" };
        for (int i = 0; i < 8; ++i)
        {
            auto r = juce::Rectangle<float> (whiteX[i] - 13, 668, 26, 22);
            g.setColour (col::black); g.fillRect (r);
            text (g, sel[i], r.getX(), r.getY(), r.getWidth(), r.getHeight(), 14.0f, juce::Colours::white);
        }
        auto smallTag = [&] (float cx, const char* t)
        {
            auto r = juce::Rectangle<float> (cx - 20, 690, 40, 18);
            g.setColour (col::black); g.fillRect (r);
            text (g, t, r.getX(), r.getY(), r.getWidth(), r.getHeight(), 11.0f, juce::Colours::white);
        };
        smallTag (433, "DEL"); smallTag (517, "INS");
    }
    // right: time mode / transpose / accent / slide
    {
        boxOutline ({ 1034, 400, 334, 334 });
        text (g, "TIME MODE", 1120, 404, 160, 22, 16.0f);
        noteGlyph (g, 1046, 450, true, col::ink);  g.setColour (col::ink); g.fillEllipse (1036, 462, 6, 6);
        noteGlyph (g, 1134, 450, true, col::ink);  g.drawEllipse (1122, 460, 8, 8, 1.4f);
        g.fillRect (1215.0f, 466.0f, 14.0f, 3.0f);
        text (g, "REST", 1232, 456, 40, 20, 11.0f);
        g.setColour (col::black);
        g.fillRect (1034.0f, 484.0f, 334.0f, 36.0f);
        text (g, "TRANSPOSE", 1040, 484, 150, 16, 12.0f, juce::Colours::white);
        g.setColour (juce::Colours::white);
        g.drawLine (1045, 500, 1180, 500, 1.0f);
        g.drawLine (1197, 484, 1197, 520, 1.4f);
        g.drawLine (1282, 484, 1282, 520, 1.4f);
        text (g, "DOWN", 1030, 500, 80, 20, 14.0f, juce::Colours::white);
        text (g, "UP", 1115, 500, 80, 20, 14.0f, juce::Colours::white);
        text (g, "ACCENT", 1200, 490, 80, 26, 14.0f, juce::Colours::white);
        text (g, "SLIDE", 1285, 490, 80, 26, 14.0f, juce::Colours::white);
        text (g, "STEP", 1030, 632, 80, 20, 14.0f);
        // triplet glyph
        for (int i = 0; i < 3; ++i) noteGlyph (g, 1134.0f + (float) i * 10.0f, 622.0f, true, col::ink);
        text (g, "3", 1140, 642, 30, 16, 11.0f);
        auto ps = juce::Rectangle<float> (1200, 626, 166, 40);
        g.setColour (col::black); g.fillRect (ps);
        text (g, "A", 1200, 626, 82, 20, 15.0f, col::redText);
        text (g, "B", 1284, 626, 82, 20, 15.0f, col::redText);
        g.setColour (juce::Colours::white); g.drawLine (1282, 626, 1282, 646, 1.0f);
        text (g, "PATT. SECTION", 1200, 646, 166, 20, 12.0f, col::redText);
        const char* nums[4] = { "9", "0", "100", "200" };
        const float nx[4] = { 1070, 1155, 1240, 1325 };
        for (int i = 0; i < 4; ++i)
        {
            auto r = juce::Rectangle<float> (nx[i] - 22, 676, 44, 22);
            g.setColour (col::black); g.fillRect (r);
            text (g, nums[i], r.getX(), r.getY(), r.getWidth(), r.getHeight(), 14.0f, juce::Colours::white);
        }
    }
    // far right: back / write-next
    {
        boxOutline ({ 1372, 400, 152, 120 });
        boxOutline ({ 1372, 524, 152, 210 });
        text (g, "BACK", 1372, 438, 152, 22, 15.0f);
        auto ds = juce::Rectangle<float> (1427, 532, 40, 20);
        g.setColour (col::ink); g.drawRect (ds, 1.2f);
        text (g, "D.S.", ds.getX(), ds.getY(), ds.getWidth(), ds.getHeight(), 12.0f, col::ink, juce::Justification::centred, false);
        auto wn = juce::Rectangle<float> (1392, 628, 112, 24);
        g.setColour (col::ink); g.drawRect (wn, 1.4f);
        text (g, "WRITE/NEXT", wn.getX(), wn.getY(), wn.getWidth(), wn.getHeight(), 14.0f);
        text (g, "TAP", 1372, 660, 152, 20, 13.0f);
    }

    // ===================== memory rail (TB-3030 additions)
    auto rail = juce::Rectangle<float> (0, 780, 1560, 200);
    juce::ColourGradient rg (juce::Colour (0xff323234), 0, 780, juce::Colour (0xff1f1f20), 0, 980, false);
    g.setGradientFill (rg);
    g.fillRect (rail);
    g.setColour (juce::Colour (0xff4a4a4c));
    g.drawLine (0, 781, 1560, 781, 2.0f);

    auto railLabel = [&] (const char* t, float x, float y, float w)
    { text (g, t, x, y, w, 14, 11.0f, col::railText.withAlpha (0.75f), juce::Justification::centredLeft); };
    railLabel ("BANK", 475, 792, 80);
    railLabel ("GROUP", 575, 792, 100);
    railLabel ("NUMBER", 765, 792, 100);
    railLabel ("SOUND", 1135, 792, 80);
    railLabel ("MEMORY", 1233, 792, 100);
    text (g, "Shift-click pattern keys = chain patterns   |   Space = Run/Stop   |   Keys A W S E D F T G Y H U J K play notes (Z / X = octave)",
          475, 852, 1070, 24, 12.0f, col::railText.withAlpha (0.55f), juce::Justification::centredLeft, false);
    text (g, "DRIVE", 1210, 954, 80, 16, 11.0f, col::railText);
    text (g, "SHUFFLE", 1295, 954, 80, 16, 11.0f, col::railText);
    text (g, "CLOCK", 1398, 892, 60, 16, 11.0f, col::railText);
    text (g, "INT", 1380, 936, 40, 16, 10.0f, col::railText.withAlpha (0.7f));
    text (g, "HOST", 1436, 936, 40, 16, 10.0f, col::railText.withAlpha (0.7f));
    text (g, "TB-3030", 1470, 900, 80, 22, 15.0f, col::railText, juce::Justification::centred);
    text (g, "v1.0", 1470, 920, 80, 16, 11.0f, col::railText.withAlpha (0.6f), juce::Justification::centred, false);
}

//==============================================================================
// Editor
//==============================================================================
TB3030Editor::TB3030Editor (TB3030Processor& p) : AudioProcessorEditor (p), proc (p), panel (p)
{
    addAndMakeVisible (panel);
    setWantsKeyboardFocus (true);
    setResizable (true, true);
    if (auto* c = getConstrainer())
    {
        c->setFixedAspectRatio ((double) PanelView::W / (double) PanelView::H);
        c->setSizeLimits (PanelView::W / 2, PanelView::H / 2, (PanelView::W * 6) / 5, (PanelView::H * 6) / 5);
    }
    setSize ((PanelView::W * 3) / 4, (PanelView::H * 3) / 4);
    startTimerHz (30);
}

void TB3030Editor::paint (juce::Graphics& g) { g.fillAll (juce::Colour (0xff1f1f20)); }

void TB3030Editor::resized()
{
    const float scale = (float) getWidth() / (float) PanelView::W;
    panel.setBounds (0, 0, PanelView::W, PanelView::H);
    panel.setTransform (juce::AffineTransform::scale (scale));
}

void TB3030Editor::timerCallback()
{
    if (! focusGrabbed && isShowing())
    {
        focusGrabbed = true;
        grabKeyboardFocus();
    }
    panel.refresh();
}

bool TB3030Editor::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress::spaceKey) { proc.pressButton (PanelBtn::RunStop, false); return true; }
    const auto ch = juce::CharacterFunctions::toLowerCase (k.getTextCharacter());
    if (ch == 'z') { keyOctave = juce::jmax (-2, keyOctave - 1); return true; }
    if (ch == 'x') { keyOctave = juce::jmin (3, keyOctave + 1); return true; }
    static const juce::String map ("awsedftgyhujk");
    return map.containsChar (ch);
}

bool TB3030Editor::keyStateChanged (bool)
{
    static const char map[] = "awsedftgyhujk";
    bool used = false;
    for (int i = 0; i < 13; ++i)
    {
        const bool down = juce::KeyPress::isKeyCurrentlyDown ((int) juce::CharacterFunctions::toUpperCase ((juce::juce_wchar) map[i]));
        const bool was = keysDown.count (i) > 0;
        if (down && ! was)
        {
            keysDown.insert (i);
            // while the sequencer runs the keys transpose the pattern (C3 = no transpose)
            keyNote[i] = juce::jlimit (0, 127, (proc.isRunning() ? 60 : 36) + keyOctave * 12 + i);
            proc.keyboardState.noteOn (1, keyNote[i], 0.7f);
            used = true;
        }
        else if (! down && was)
        {
            keysDown.erase (i);
            proc.keyboardState.noteOff (1, keyNote[i], 0.0f);
            used = true;
        }
    }
    return used;
}
