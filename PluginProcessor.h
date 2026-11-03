#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "TB303Engine.h"
#include "SoundBank.h"

// Front panel button ids (same layout as the original panel)
enum class PanelBtn : int
{
    Note0 = 0, // C, C#, D, D#, E, F, F#, G, G#, A, A#, B, C'  (Note0 + 0..12)
    PatternClear = 20, RunStop, PitchMode, Function, TimeMode,
    Down, Up, Accent, Slide, Back, WriteNext
};

class TB3030Processor : public juce::AudioProcessor
{
public:
    TB3030Processor();
    ~TB3030Processor() override = default;

    //== AudioProcessor ========================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "TB-3030"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }

    // host programs = the 32 sounds of the current bank
    int getNumPrograms() override { return SoundBank::numSlots; }
    int getCurrentProgram() override { return currentSlot; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //== Parameters ============================================================
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    tb303::SoundParams currentSoundFromParams() const;
    void applySoundToParams (const tb303::SoundParams&);

    //== Front panel logic (message thread) ====================================
    void pressButton (PanelBtn b, bool shift);
    bool ledOn (PanelBtn b) const;
    void setMode (int m);
    int  getMode() const { return mode; }
    void setGroupKnob (int pos);
    int  getGroupKnob() const { return groupKnob; }
    bool isRunning() const { return running.load(); }
    bool isTimeMode() const { return timeMode; }
    bool isFunctionOn() const { return functionOn; }

    // step strip / display support
    int  getCursorStep() const { return cursorStep; }
    void setCursorStep (int s);
    int  getPlayingStep() const { return playingStep.load(); }
    int  getPlayingPatternIndex() const { return playingPattern.load(); }
    int  getPlayingMeasure() const { return playingMeasure.load(); }
    int  getMeasureCursor() const { return measureCursor; }
    int  getTrackLength() const;
    tb303::Pattern getShownPattern (int& group, int& index) const;   // pattern shown in the step strip
    void editShownPattern (const std::function<void (tb303::Pattern&)>& fn);
    juce::String statusLine() const;
    float currentBpm() const { return lastBpm.load(); }

    void copyPattern();
    void pastePattern();
    void randomisePattern();
    void restoreFactoryPatterns();

    bool savePatternsToFile (const juce::File& f);
    bool loadPatternsFromFile (const juce::File& f);

    //== Sound memory ==========================================================
    BankManager banks;
    int  getBankIndex() const { return currentBank; }
    int  getSlot() const { return currentSlot; }
    void selectBank (int index);
    void selectSlot (int slot);
    bool isSoundModified() const;
    juce::String currentSoundName() const;
    // returns index of the bank written to (a user bank is created if the factory bank is selected)
    int  writeSound (int slot, const juce::String& name);
    void newBank();
    bool importBankFile (const juce::File& f);
    bool exportCurrentBank (const juce::File& f);
    bool deleteCurrentBank();

    juce::MidiKeyboardState keyboardState;   // computer keyboard / audition notes

private:
    juce::String encodePattern (const tb303::Pattern&) const;
    void decodePattern (const juce::String&, tb303::Pattern&) const;
    std::unique_ptr<juce::XmlElement> sequenceToXml() const;
    void sequenceFromXml (const juce::XmlElement&);
    void syncSequencerSettings();
    void audition (int midiNote);
    void handleMidi (const juce::MidiMessage& m);
    tb303::Step& cursorStepRef();

    tb303::Voice voice;
    tb303::Sequencer seq;
    tb303::SequenceData data;
    mutable juce::SpinLock lock;

    // UI state (message thread)
    int mode = 2;            // 0 TrackWrite, 1 TrackPlay, 2 PatternPlay, 3 PatternWrite
    int groupKnob = 0;       // 0..6
    int selPattern = 0;      // 0..7 (pattern key)
    int section = 0;         // 0 = A, 1 = B
    std::vector<int> chain { 0 };
    std::atomic<int> octaveShift { 0 };     // pattern play octave keys
    bool functionOn = false, timeMode = false;
    int cursorStep = 0;
    int measureCursor = 0;
    tb303::Pattern clipboard; bool hasClipboard = false;
    uint32_t randomSeed = 1;

    std::atomic<bool> running { false };
    std::atomic<bool> startRequest { false }, stopRequest { false };
    std::atomic<int> playingStep { -1 }, playingPattern { 0 }, playingMeasure { 0 };
    std::atomic<float> lastBpm { 120.0f };

    int currentBank = 0, currentSlot = 0;

    // audio thread
    std::vector<int> heldNotes;
    double sr = 44100.0;
    bool hostWasPlaying = false;
    double expectedPpq = 0.0;
    int midiTranspose = 0;
    int auditionSamplesLeft = 0;
    std::atomic<int> auditionRequest { -1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TB3030Processor)
};
