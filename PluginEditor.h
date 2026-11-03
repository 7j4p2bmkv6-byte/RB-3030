#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "PanelComponents.h"
#include <set>

//==============================================================================
// 16-step display/editor strip (TB-3030 addition)
class StepStrip : public juce::Component
{
public:
    explicit StepStrip (TB3030Processor& p) : proc (p) { setMouseClickGrabsKeyboardFocus (false); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    bool refresh();   // returns true if repainted
private:
    int cellAt (juce::Point<int> p) const;
    juce::Rectangle<float> cellBounds (int i) const;
    void showStepMenu (int i);
    TB3030Processor& proc;
    tb303::Pattern shown;
    int shownGroup = 0, shownIndex = 0, lastPlaying = -2, lastCursor = -2;
    juce::String lastStatus;
};

//==============================================================================
// The complete front panel, drawn in a fixed 1560 x 980 coordinate space
class PanelView : public juce::Component
{
public:
    explicit PanelView (TB3030Processor&);
    ~PanelView() override;
    void paint (juce::Graphics&) override;
    void resized() override {}
    void refresh();

    static constexpr int W = 1560, H = 980;

private:
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void makeKnob (juce::Slider& s, const char* paramId, juce::Point<int> centre, int size, const char* style);
    void addKey (PanelBtn id, juce::Rectangle<int> r, juce::Point<int> ledPos);
    void drawRotaryLabels (juce::Graphics& g);
    void drawSound (juce::Graphics& g);
    void writeDialog();
    void showBankMenu();
    void showPatternMenu();
    void message (const juce::String& title, const juce::String& text);

    TB3030Processor& proc;
    tbui::TBLookAndFeel lnf;

    juce::Slider tuning, cutoff, resonance, envmod, decay, accent, tempo, volume, drive, shuffle;
    juce::Slider groupSw, modeSw;
    tbui::SlideSwitch waveSw, clockSw;
    std::vector<std::unique_ptr<SA>> sliderAtt;
    std::vector<std::unique_ptr<BA>> buttonAtt;

    struct KeyItem { PanelBtn id; std::unique_ptr<tbui::PanelKey> key; std::unique_ptr<tbui::Led> led; };
    std::vector<KeyItem> keys;

    tbui::Lcd soundLcd;
    StepStrip strip;
    juce::TextButton bankPrev { "<" }, bankNext { ">" }, slotPrev { "<" }, slotNext { ">" };
    juce::TextButton groupBtns[4], numBtns[8];
    juce::TextButton writeBtn { "WRITE" }, bankMenuBtn { "BANK..." }, pattMenuBtn { "PATTERNS..." };
    std::unique_ptr<juce::FileChooser> chooser;
};

//==============================================================================
class TB3030Editor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit TB3030Editor (TB3030Processor&);
    ~TB3030Editor() override = default;
    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
    bool keyStateChanged (bool isKeyDown) override;

private:
    void timerCallback() override;
    TB3030Processor& proc;
    PanelView panel;
    int keyOctave = 0;
    std::set<int> keysDown;
    int keyNote[13] = {};
    bool focusGrabbed = false;
};
