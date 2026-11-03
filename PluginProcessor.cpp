#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "FactoryData.h"

using namespace tb303;

namespace
{
    const int whiteNotes[8] = { 0, 2, 4, 5, 7, 9, 11, 12 };
    int whiteIndex (int note)
    {
        for (int i = 0; i < 8; ++i) if (whiteNotes[i] == note) return i;
        return -1;
    }
    int groupForKnob (int pos) { static const int g[7] = { 0,0,1,1,2,2,3 }; return g[juce::jlimit (0, 6, pos)]; }
    const char* groupRoman (int g) { static const char* r[4] = { "I", "II", "III", "IV" }; return r[juce::jlimit (0, 3, g)]; }
    juce::String patName (int idx) { return juce::String (idx % 8 + 1) + (idx >= 8 ? "B" : "A"); }
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout TB3030Processor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    auto pct = [] (const String& id, const String& name, float def)
    {
        return std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, NormalisableRange<float> (0.0f, 1.0f), def);
    };
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "tuning", 1 }, "Tuning",
                                                        NormalisableRange<float> (-12.0f, 12.0f), 0.0f));
    p.push_back (pct ("cutoff",    "Cut Off Freq", 0.30f));
    p.push_back (pct ("resonance", "Resonance",    0.72f));
    p.push_back (pct ("envmod",    "Env Mod",      0.60f));
    p.push_back (pct ("decay",     "Decay",        0.42f));
    p.push_back (pct ("accent",    "Accent",       0.65f));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "square", 1 }, "Waveform Square", false));
    p.push_back (pct ("drive",     "Drive",        0.0f));
    p.push_back (pct ("volume",    "Volume",       0.75f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "tempo", 1 }, "Tempo",
                                                        NormalisableRange<float> (40.0f, 300.0f, 0.1f), 125.0f));
    p.push_back (pct ("shuffle",   "Shuffle",      0.0f));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "hostsync", 1 }, "Host Sync", true));
    return { p.begin(), p.end() };
}

TB3030Processor::TB3030Processor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    loadFactoryPatterns (data);
    syncSequencerSettings();
    heldNotes.reserve (128);
}

bool TB3030Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void TB3030Processor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    voice.setSampleRate (sampleRate);
    keyboardState.reset();
    heldNotes.clear();
}

SoundParams TB3030Processor::currentSoundFromParams() const
{
    SoundParams s;
    s.tuning    = apvts.getRawParameterValue ("tuning")->load();
    s.cutoff    = apvts.getRawParameterValue ("cutoff")->load();
    s.resonance = apvts.getRawParameterValue ("resonance")->load();
    s.envMod    = apvts.getRawParameterValue ("envmod")->load();
    s.decay     = apvts.getRawParameterValue ("decay")->load();
    s.accent    = apvts.getRawParameterValue ("accent")->load();
    s.waveform  = apvts.getRawParameterValue ("square")->load() > 0.5f ? 1 : 0;
    s.drive     = apvts.getRawParameterValue ("drive")->load();
    return s;
}

void TB3030Processor::applySoundToParams (const SoundParams& s)
{
    auto set = [this] (const char* id, float v)
    {
        if (auto* p = apvts.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (v));
    };
    set ("tuning", s.tuning);    set ("cutoff", s.cutoff);  set ("resonance", s.resonance);
    set ("envmod", s.envMod);    set ("decay", s.decay);    set ("accent", s.accent);
    set ("square", (float) s.waveform); set ("drive", s.drive);
}

//==============================================================================
void TB3030Processor::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn())
    {
        int note = m.getNoteNumber();
        if (seq.isRunning()) { midiTranspose = juce::jlimit (-24, 24, note - 60); return; }
        heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), note), heldNotes.end());
        heldNotes.push_back (note);
        voice.noteOn ((float) note, m.getVelocity() >= 100, heldNotes.size() > 1);
    }
    else if (m.isNoteOff())
    {
        int note = m.getNoteNumber();
        if (heldNotes.empty()) return;
        bool wasLast = heldNotes.back() == note;
        heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), note), heldNotes.end());
        if (seq.isRunning()) return;
        if (heldNotes.empty()) voice.noteOff();
        else if (wasLast) voice.noteOn ((float) heldNotes.back(), false, true);   // slide back (legato)
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        heldNotes.clear(); voice.noteOff();
    }
    else if (m.isController())
    {
        const float v = m.getControllerValue() / 127.0f;
        const char* id = nullptr;
        switch (m.getControllerNumber())
        {
            case 74: id = "cutoff"; break;
            case 71: id = "resonance"; break;
            case 12: id = "envmod"; break;
            case 13: id = "decay"; break;
            case 16: id = "accent"; break;
            case 17: id = "drive"; break;
            case 18: id = "tuning"; break;
            case 7:  id = "volume"; break;
            default: break;
        }
        if (id != nullptr)
            if (auto* p = apvts.getParameter (id)) p->setValueNotifyingHost (v);
    }
}

void TB3030Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();

    keyboardState.processNextMidiBuffer (midi, 0, n, true);

    voice.setParams (currentSoundFromParams());
    const float volRaw = apvts.getRawParameterValue ("volume")->load();
    const float vol = volRaw * volRaw * 1.4f;
    const bool hostSync = apvts.getRawParameterValue ("hostsync")->load() > 0.5f;
    double bpm = apvts.getRawParameterValue ("tempo")->load();

    bool hostPlaying = false, hasPpq = false; double ppq = 0.0;
    if (hostSync)
        if (auto* ph = getPlayHead())
            if (auto pos = ph->getPosition())
            {
                if (auto b = pos->getBpm()) bpm = *b;
                if (auto p = pos->getPpqPosition()) { ppq = *p; hasPpq = true; }
                hostPlaying = pos->getIsPlaying();
            }
    bpm = juce::jlimit (20.0, 400.0, bpm);
    lastBpm.store ((float) bpm);

    float* mono = buffer.getWritePointer (0);

    {
        // the panel only holds this lock for a few microseconds while reading/editing pattern memory
        const juce::SpinLock::ScopedLockType sl (lock);
        seq.shuffle = apvts.getRawParameterValue ("shuffle")->load();
        seq.externalTranspose = octaveShift.load() + midiTranspose;
        const bool synced = hostSync && hasPpq;
        const double spb = seq.stepsPerBeatFor (data);

        if (stopRequest.exchange (false)) seq.stop (voice);
        if (startRequest.exchange (false))
        {
            heldNotes.clear(); voice.noteOff();
            if (synced && hostPlaying) seq.syncTo (ppq * spb); else seq.start();
        }
        if (synced)
        {
            if (hostPlaying && (! hostWasPlaying || std::abs (ppq - expectedPpq) > 0.05))
                seq.syncTo (ppq * spb);
            else if (! hostPlaying && hostWasPlaying)
                seq.stop (voice);
            expectedPpq = ppq + (double) n / sr * bpm / 60.0;
        }
        hostWasPlaying = synced && hostPlaying;

        // audition notes from the panel
        int aud = auditionRequest.exchange (-1);
        if (aud >= 0 && ! seq.isRunning() && heldNotes.empty())
        {
            voice.noteOn ((float) (aud % 1000), aud >= 1000, false);
            auditionSamplesLeft = (int) (0.22 * sr);
        }

        int pos = 0;
        for (const auto meta : midi)
        {
            const int t = juce::jlimit (0, n, meta.samplePosition);
            if (t > pos) { seq.process (data, voice, mono + pos, t - pos, bpm, sr, vol); pos = t; }
            handleMidi (meta.getMessage());
        }
        if (pos < n) seq.process (data, voice, mono + pos, n - pos, bpm, sr, vol);

        if (auditionSamplesLeft > 0)
        {
            auditionSamplesLeft -= n;
            if (auditionSamplesLeft <= 0 && heldNotes.empty() && ! seq.isRunning()) voice.noteOff();
        }

        running.store (seq.isRunning());
        playingStep.store (seq.isRunning() ? seq.displayStep : -1);
        playingPattern.store (seq.displayPattern);
        playingMeasure.store (seq.displayMeasure);
    }

    for (int ch = 1; ch < buffer.getNumChannels(); ++ch)
        buffer.copyFrom (ch, 0, buffer, 0, 0, n);
    midi.clear();
}

//==============================================================================
// Front panel logic
//==============================================================================
void TB3030Processor::syncSequencerSettings()
{
    const juce::SpinLock::ScopedLockType sl (lock);
    seq.mode = (Mode) mode;
    seq.groupKnob = groupKnob;
    seq.editPattern = selPattern + 8 * section;
    if (seq.isRunning() && mode == 2) { seq.queuedChain = chain; seq.hasQueued = true; }
    else { seq.chain = chain; seq.hasQueued = false; }
}

void TB3030Processor::setMode (int m)
{
    mode = juce::jlimit (0, 3, m);
    functionOn = false;
    if (mode != 3) timeMode = false;
    syncSequencerSettings();
}

void TB3030Processor::setGroupKnob (int p)
{
    groupKnob = juce::jlimit (0, 6, p);
    measureCursor = 0;
    syncSequencerSettings();
}

void TB3030Processor::audition (int midiNote)
{
    if (! running.load()) auditionRequest.store (midiNote);
}

int TB3030Processor::getTrackLength() const
{
    const juce::SpinLock::ScopedLockType sl (lock);
    return (int) data.tracks[groupKnob].measures.size();
}

Step& TB3030Processor::cursorStepRef()
{
    auto& p = data.get (groupForKnob (groupKnob), selPattern + 8 * section);
    cursorStep = juce::jlimit (0, Pattern::maxSteps - 1, cursorStep);
    return p.steps[cursorStep];
}

void TB3030Processor::setCursorStep (int s)
{
    cursorStep = juce::jlimit (0, Pattern::maxSteps - 1, s);
}

void TB3030Processor::pressButton (PanelBtn b, bool shift)
{
    const int bi = (int) b;
    const bool isNote = bi >= 0 && bi <= 12;
    const int note = isNote ? bi : -1;
    const int w = isNote ? whiteIndex (note) : -1;

    if (b == PanelBtn::RunStop)
    {
        if (running.load()) stopRequest.store (true);
        else { syncSequencerSettings(); startRequest.store (true); }
        return;
    }

    switch (mode)
    {
        case 2: // ---------------------------------------------------- PATTERN PLAY
        {
            if (w >= 0)
            {
                selPattern = w;
                int idx = w + 8 * section;
                if (shift && (int) chain.size() < 16) chain.push_back (idx);
                else chain = { idx };
                syncSequencerSettings();
            }
            else if (b == PanelBtn::Accent || b == PanelBtn::Slide)
            {
                section = (b == PanelBtn::Accent) ? 0 : 1;
                if (shift) chain = { selPattern, selPattern + 8 };
                else chain = { selPattern + 8 * section };
                syncSequencerSettings();
            }
            else if (b == PanelBtn::Down) octaveShift.store (octaveShift.load() == -12 ? 0 : -12);
            else if (b == PanelBtn::Up)   octaveShift.store (octaveShift.load() == 12 ? 0 : 12);
            break;
        }
        case 3: // ---------------------------------------------------- PATTERN WRITE
        {
            if (b == PanelBtn::Function) { functionOn = ! functionOn; break; }
            if (b == PanelBtn::PitchMode) { timeMode = false; functionOn = false; break; }
            if (b == PanelBtn::TimeMode)  { timeMode = ! timeMode; functionOn = false; break; }

            if (functionOn)
            {
                if (w >= 0) { selPattern = w; cursorStep = 0; syncSequencerSettings(); }
                else if (b == PanelBtn::Accent) { section = 0; cursorStep = 0; syncSequencerSettings(); }
                else if (b == PanelBtn::Slide)  { section = 1; cursorStep = 0; syncSequencerSettings(); }
                else if (b == PanelBtn::Up)
                {
                    const juce::SpinLock::ScopedLockType sl (lock);
                    auto& p = data.get (groupForKnob (groupKnob), selPattern + 8 * section);
                    p.triplet = ! p.triplet;
                }
                else if (b == PanelBtn::WriteNext)
                {
                    const juce::SpinLock::ScopedLockType sl (lock);
                    data.get (groupForKnob (groupKnob), selPattern + 8 * section).length = cursorStep + 1;
                }
                else if (b == PanelBtn::Back)
                {
                    const juce::SpinLock::ScopedLockType sl (lock);
                    data.get (groupForKnob (groupKnob), selPattern + 8 * section).length = 16;
                }
                else if (b == PanelBtn::PatternClear)
                {
                    const juce::SpinLock::ScopedLockType sl (lock);
                    data.get (groupForKnob (groupKnob), selPattern + 8 * section).clear();
                    cursorStep = 0;
                }
                break;
            }

            int len;
            {
                const juce::SpinLock::ScopedLockType sl (lock);
                len = data.get (groupForKnob (groupKnob), selPattern + 8 * section).length;
            }
            if (b == PanelBtn::WriteNext) { cursorStep = (cursorStep + 1) % len; break; }
            if (b == PanelBtn::Back)      { cursorStep = (cursorStep - 1 + len) % len; break; }
            if (b == PanelBtn::PatternClear)
            {
                const juce::SpinLock::ScopedLockType sl (lock);
                data.get (groupForKnob (groupKnob), selPattern + 8 * section).clear();
                cursorStep = 0;
                break;
            }

            int auditionMidi = -1;
            bool advance = false;
            {
                const juce::SpinLock::ScopedLockType sl (lock);
                auto& s = cursorStepRef();
                if (isNote)
                {
                    s.note = (uint8_t) note; s.time = TimeType::Note;
                    auditionMidi = 36 + note + (s.up ? 12 : 0) - (s.down ? 12 : 0) + (s.accent ? 1000 : 0);
                }
                else if (timeMode)
                {
                    if (b == PanelBtn::Down)   { s.time = TimeType::Note; advance = true; }
                    if (b == PanelBtn::Up)     { s.time = TimeType::Tie;  advance = true; }
                    if (b == PanelBtn::Accent) { s.time = TimeType::Rest; advance = true; }
                    if (b == PanelBtn::Slide)  s.slide = ! s.slide;
                }
                else
                {
                    if (b == PanelBtn::Down)   { s.down = ! s.down; if (s.down) s.up = false; }
                    if (b == PanelBtn::Up)     { s.up = ! s.up; if (s.up) s.down = false; }
                    if (b == PanelBtn::Accent) s.accent = ! s.accent;
                    if (b == PanelBtn::Slide)  s.slide = ! s.slide;
                    if (b == PanelBtn::Down || b == PanelBtn::Up)
                        if (s.time == TimeType::Note)
                            auditionMidi = 36 + s.note + (s.up ? 12 : 0) - (s.down ? 12 : 0) + (s.accent ? 1000 : 0);
                }
            }
            if (advance) cursorStep = (cursorStep + 1) % len;
            if (auditionMidi >= 0) audition (auditionMidi);
            break;
        }
        case 0: // ---------------------------------------------------- TRACK WRITE
        {
            if (b == PanelBtn::Function) { functionOn = ! functionOn; break; }
            const juce::SpinLock::ScopedLockType sl (lock);
            auto& ms = data.tracks[groupKnob].measures;
            if (b == PanelBtn::PatternClear) { ms.clear(); measureCursor = 0; break; }
            if (ms.empty()) ms.push_back (Measure());
            measureCursor = juce::jlimit (0, (int) ms.size() - 1, measureCursor);
            auto& m = ms[(size_t) measureCursor];

            if (b == PanelBtn::WriteNext)
            {
                if (measureCursor == (int) ms.size() - 1 && (int) ms.size() < Track::maxMeasures)
                    ms.push_back (m);
                measureCursor = juce::jmin (measureCursor + 1, (int) ms.size() - 1);
                break;
            }
            if (b == PanelBtn::Back) { measureCursor = juce::jmax (0, measureCursor - 1); break; }

            if (functionOn)
            {
                if (isNote) m.transpose = (int8_t) note;
                else if (b == PanelBtn::Down) m.transpose = (int8_t) juce::jmax (-12, m.transpose - 12);
                else if (b == PanelBtn::Up)   m.transpose = (int8_t) juce::jmin (24, m.transpose + 12);
            }
            else
            {
                if (w >= 0) m.pattern = (uint8_t) w;
                else if (b == PanelBtn::Accent) m.section = 0;
                else if (b == PanelBtn::Slide)  m.section = 1;
                else if (note == 1) // C# = DEL
                {
                    ms.erase (ms.begin() + measureCursor);
                    measureCursor = juce::jlimit (0, juce::jmax (0, (int) ms.size() - 1), measureCursor);
                }
                else if (note == 3 && (int) ms.size() < Track::maxMeasures) // D# = INS
                {
                    Measure copy = m;
                    ms.insert (ms.begin() + measureCursor, copy);
                }
            }
            break;
        }
        case 1: // ---------------------------------------------------- TRACK PLAY
        default:
            break;
    }
}

bool TB3030Processor::ledOn (PanelBtn b) const
{
    const int bi = (int) b;
    const bool isNote = bi >= 0 && bi <= 12;
    const int w = isNote ? whiteIndex (bi) : -1;
    const bool run = running.load();

    switch (b)
    {
        case PanelBtn::RunStop:   return run;
        case PanelBtn::Function:  return functionOn;
        case PanelBtn::PitchMode: return mode == 3 && ! timeMode && ! functionOn;
        case PanelBtn::TimeMode:  return mode == 3 && timeMode && ! functionOn;
        default: break;
    }

    const juce::SpinLock::ScopedLockType sl (lock);
    const int g = groupForKnob (groupKnob);

    if (mode == 2)
    {
        if (w >= 0)
        {
            if (run) return playingPattern.load() % 8 == w;
            for (int c : chain) if (c % 8 == w) return true;
            return false;
        }
        bool hasA = false, hasB = false;
        for (int c : chain) { if (c < 8) hasA = true; else hasB = true; }
        if (b == PanelBtn::Accent) return hasA;
        if (b == PanelBtn::Slide)  return hasB;
        if (b == PanelBtn::Down)   return octaveShift.load() == -12;
        if (b == PanelBtn::Up)     return octaveShift.load() == 12;
        return false;
    }
    if (mode == 3)
    {
        const auto& p = data.get (g, selPattern + 8 * section);
        const auto& s = p.steps[juce::jlimit (0, 15, cursorStep)];
        if (functionOn)
        {
            if (w >= 0) return w == selPattern;
            if (b == PanelBtn::Accent) return section == 0;
            if (b == PanelBtn::Slide)  return section == 1;
            if (b == PanelBtn::Up)     return p.triplet;
            return false;
        }
        if (isNote)
        {
            if (run && playingStep.load() >= 0)
            {
                const auto& ps = p.steps[juce::jlimit (0, 15, playingStep.load())];
                return ps.time == TimeType::Note && ps.note == bi;
            }
            return s.time == TimeType::Note && s.note == bi;
        }
        if (timeMode)
        {
            if (b == PanelBtn::Down)   return s.time == TimeType::Note;
            if (b == PanelBtn::Up)     return s.time == TimeType::Tie;
            if (b == PanelBtn::Accent) return s.time == TimeType::Rest;
            if (b == PanelBtn::Slide)  return s.slide;
            return false;
        }
        if (b == PanelBtn::Down)   return s.down;
        if (b == PanelBtn::Up)     return s.up;
        if (b == PanelBtn::Accent) return s.accent;
        if (b == PanelBtn::Slide)  return s.slide;
        return false;
    }
    // track modes
    const auto& ms = data.tracks[groupKnob].measures;
    if (ms.empty()) return false;
    int mi = (mode == 1 || run) ? playingMeasure.load() : measureCursor;
    if (mode == 0 && ! run) mi = measureCursor;
    const auto& m = ms[(size_t) juce::jlimit (0, (int) ms.size() - 1, mi)];
    if (mode == 0 && functionOn)
    {
        if (isNote) return bi == ((m.transpose % 12) + 12) % 12 || (bi == 12 && m.transpose == 12);
        if (b == PanelBtn::Down) return m.transpose < 0;
        if (b == PanelBtn::Up)   return m.transpose >= 12;
        return false;
    }
    if (w >= 0) return m.pattern == w;
    if (b == PanelBtn::Accent) return m.section == 0;
    if (b == PanelBtn::Slide)  return m.section == 1;
    return false;
}

Pattern TB3030Processor::getShownPattern (int& group, int& index) const
{
    const juce::SpinLock::ScopedLockType sl (lock);
    group = groupForKnob (groupKnob);
    index = selPattern + 8 * section;
    if (mode == 2)
        index = running.load() ? playingPattern.load() : (chain.empty() ? 0 : chain.front());
    else if (mode == 0 || mode == 1)
    {
        const auto& ms = data.tracks[groupKnob].measures;
        if (! ms.empty())
        {
            int mi = (running.load() || mode == 1) ? playingMeasure.load() : measureCursor;
            if (mode == 0 && ! running.load()) mi = measureCursor;
            const auto& m = ms[(size_t) juce::jlimit (0, (int) ms.size() - 1, mi)];
            index = m.pattern + 8 * m.section;
        }
    }
    index = juce::jlimit (0, 15, index);
    return data.get (group, index);
}

void TB3030Processor::editShownPattern (const std::function<void (Pattern&)>& fn)
{
    int g, i;
    getShownPattern (g, i);
    const juce::SpinLock::ScopedLockType sl (lock);
    fn (data.get (g, i));
}

void TB3030Processor::copyPattern()
{
    int g, i;
    clipboard = getShownPattern (g, i);
    hasClipboard = true;
}

void TB3030Processor::pastePattern()
{
    if (! hasClipboard) return;
    auto c = clipboard;
    editShownPattern ([c] (Pattern& p) { p = c; });
}

void TB3030Processor::randomisePattern()
{
    randomSeed = randomSeed * 1664525u + 1013904223u + (uint32_t) juce::Time::getMillisecondCounter();
    auto seed = randomSeed;
    editShownPattern ([seed] (Pattern& p) { bool t = p.triplet; int len = p.length; p = randomPattern (seed, len); p.triplet = t; });
}

void TB3030Processor::restoreFactoryPatterns()
{
    const juce::SpinLock::ScopedLockType sl (lock);
    loadFactoryPatterns (data);
}

juce::String TB3030Processor::statusLine() const
{
    static const char* modeNames[4] = { "TRACK WRITE", "TRACK PLAY", "PATTERN PLAY", "PATTERN WRITE" };
    juce::String s = modeNames[mode];
    s << "   ";
    int g, idx;
    auto p = getShownPattern (g, idx);
    if (mode >= 2)
    {
        s << groupRoman (g) << "-";
        if (mode == 2 && chain.size() > 1)
        {
            for (size_t i = 0; i < chain.size() && i < 6; ++i) s << (i ? ">" : "") << patName (chain[i]);
            if (chain.size() > 6) s << "..";
        }
        else s << patName (idx);
        if (mode == 3) s << "   STEP " << juce::String (cursorStep + 1).paddedLeft ('0', 2) << "/" << juce::String (p.length);
        else s << "   LEN " << juce::String (p.length);
        if (p.triplet) s << "  TRIPLET";
        if (mode == 3 && functionOn) s << "   [FUNCTION]";
        if (mode == 3 && timeMode && ! functionOn) s << "   [TIME MODE]";
        if (mode == 2 && octaveShift.load() != 0) s << "   OCT " << (octaveShift.load() > 0 ? "+1" : "-1");
    }
    else
    {
        int len = getTrackLength();
        int mi = (running.load() || mode == 1) ? playingMeasure.load() : measureCursor;
        if (mode == 0 && ! running.load()) mi = measureCursor;
        s << "TRACK " << (groupKnob + 1) << " (" << groupRoman (groupForKnob (groupKnob)) << ")   BAR "
          << juce::String (len == 0 ? 0 : mi + 1).paddedLeft ('0', 2) << "/" << juce::String (len).paddedLeft ('0', 2);
        if (len > 0)
        {
            const juce::SpinLock::ScopedLockType sl (lock);
            const auto& m = data.tracks[groupKnob].measures[(size_t) juce::jlimit (0, len - 1, mi)];
            s << "   PATT " << patName (m.pattern + 8 * m.section);
            if (m.transpose != 0) s << "   TRANSP " << (m.transpose > 0 ? "+" : "") << (int) m.transpose;
        }
        if (functionOn) s << "   [FUNCTION]";
    }
    return s;
}

//==============================================================================
// Sound memory
//==============================================================================
void TB3030Processor::selectBank (int index)
{
    currentBank = juce::jlimit (0, banks.numBanks() - 1, index);
    selectSlot (currentSlot);
}

void TB3030Processor::selectSlot (int slot)
{
    currentSlot = juce::jlimit (0, SoundBank::numSlots - 1, slot);
    applySoundToParams (banks.bank (currentBank).sounds[currentSlot].p);
    updateHostDisplay (juce::AudioProcessorListener::ChangeDetails().withProgramChanged (true));
}

void TB3030Processor::setCurrentProgram (int index) { selectSlot (index); }

const juce::String TB3030Processor::getProgramName (int index)
{
    index = juce::jlimit (0, SoundBank::numSlots - 1, index);
    return SoundBank::slotName (index) + " " + banks.bank (currentBank).sounds[index].name;
}

bool TB3030Processor::isSoundModified() const
{
    auto a = currentSoundFromParams();
    const auto& b = banks.bank (currentBank).sounds[currentSlot].p;
    auto diff = [] (float x, float y) { return std::abs (x - y) > 0.004f; };
    return diff (a.tuning / 24.0f, b.tuning / 24.0f) || diff (a.cutoff, b.cutoff) || diff (a.resonance, b.resonance)
        || diff (a.envMod, b.envMod) || diff (a.decay, b.decay) || diff (a.accent, b.accent)
        || a.waveform != b.waveform || diff (a.drive, b.drive);
}

juce::String TB3030Processor::currentSoundName() const
{
    return banks.bank (currentBank).sounds[currentSlot].name;
}

int TB3030Processor::writeSound (int slot, const juce::String& name)
{
    auto snd = currentSoundFromParams();
    if (banks.bank (currentBank).isFactory)
    {
        int existing = banks.indexOf ("User Bank 1");
        currentBank = existing > 0 ? existing : banks.createUserBank ("User Bank");
    }
    auto& bk = banks.bank (currentBank);
    slot = juce::jlimit (0, SoundBank::numSlots - 1, slot);
    bk.sounds[slot].p = snd;
    bk.sounds[slot].name = name.isEmpty() ? juce::String ("Init") : name.substring (0, 18);
    bk.save();
    currentSlot = slot;
    updateHostDisplay (juce::AudioProcessorListener::ChangeDetails().withProgramChanged (true));
    return currentBank;
}

void TB3030Processor::newBank()
{
    currentBank = banks.createUserBank ("User Bank");
    selectSlot (0);
}

bool TB3030Processor::importBankFile (const juce::File& f)
{
    int idx = banks.importBank (f);
    if (idx < 0) return false;
    currentBank = idx;
    selectSlot (0);
    return true;
}

bool TB3030Processor::exportCurrentBank (const juce::File& f) { return banks.exportBank (currentBank, f); }

bool TB3030Processor::deleteCurrentBank()
{
    if (! banks.deleteBank (currentBank)) return false;
    currentBank = 0;
    selectSlot (currentSlot);
    return true;
}

//==============================================================================
// State: parameters + pattern/track memory + panel state + selected sound
//==============================================================================
juce::String TB3030Processor::encodePattern (const Pattern& p) const
{
    juce::String s;
    for (const auto& st : p.steps)
    {
        int b0 = (st.note & 15) | ((int) st.time << 4);
        int b1 = (st.up ? 1 : 0) | (st.down ? 2 : 0) | (st.accent ? 4 : 0) | (st.slide ? 8 : 0);
        s << juce::String::toHexString (b0).paddedLeft ('0', 2) << juce::String::toHexString (b1).paddedLeft ('0', 2);
    }
    s << ":" << p.length << ":" << (p.triplet ? 1 : 0);
    return s;
}

void TB3030Processor::decodePattern (const juce::String& text, Pattern& p) const
{
    auto parts = juce::StringArray::fromTokens (text, ":", "");
    if (parts.size() < 3) return;
    const auto& hex = parts[0];
    for (int i = 0; i < Pattern::maxSteps && (i * 4 + 3) < hex.length(); ++i)
    {
        int b0 = hex.substring (i * 4, i * 4 + 2).getHexValue32();
        int b1 = hex.substring (i * 4 + 2, i * 4 + 4).getHexValue32();
        auto& st = p.steps[i];
        st.note = (uint8_t) juce::jlimit (0, 12, b0 & 15);
        st.time = (TimeType) juce::jlimit (0, 2, (b0 >> 4) & 3);
        st.up = (b1 & 1) != 0; st.down = (b1 & 2) != 0; st.accent = (b1 & 4) != 0; st.slide = (b1 & 8) != 0;
    }
    p.length = juce::jlimit (1, 16, parts[1].getIntValue());
    p.triplet = parts[2].getIntValue() != 0;
}

std::unique_ptr<juce::XmlElement> TB3030Processor::sequenceToXml() const
{
    auto xml = std::make_unique<juce::XmlElement> ("TB3030Patterns");
    const juce::SpinLock::ScopedLockType sl (lock);
    for (int g = 0; g < 4; ++g)
        for (int i = 0; i < 16; ++i)
        {
            auto* e = xml->createNewChildElement ("P");
            e->setAttribute ("g", g); e->setAttribute ("i", i);
            e->setAttribute ("d", encodePattern (data.patterns[g][i]));
        }
    for (int t = 0; t < 7; ++t)
    {
        auto* e = xml->createNewChildElement ("T");
        e->setAttribute ("t", t);
        juce::String s;
        for (const auto& m : data.tracks[t].measures)
            s << (int) m.pattern << "," << (int) m.section << "," << (int) m.transpose << ";";
        e->setAttribute ("m", s);
    }
    return xml;
}

void TB3030Processor::sequenceFromXml (const juce::XmlElement& xml)
{
    const juce::SpinLock::ScopedLockType sl (lock);
    for (auto* e : xml.getChildWithTagNameIterator ("P"))
    {
        int g = e->getIntAttribute ("g", -1), i = e->getIntAttribute ("i", -1);
        if (g < 0 || g > 3 || i < 0 || i > 15) continue;
        data.patterns[g][i].clear();
        decodePattern (e->getStringAttribute ("d"), data.patterns[g][i]);
    }
    for (auto* e : xml.getChildWithTagNameIterator ("T"))
    {
        int t = e->getIntAttribute ("t", -1);
        if (t < 0 || t > 6) continue;
        auto& ms = data.tracks[t].measures;
        ms.clear();
        for (auto& tok : juce::StringArray::fromTokens (e->getStringAttribute ("m"), ";", ""))
        {
            auto v = juce::StringArray::fromTokens (tok, ",", "");
            if (v.size() < 3) continue;
            Measure m;
            m.pattern = (uint8_t) juce::jlimit (0, 7, v[0].getIntValue());
            m.section = (uint8_t) juce::jlimit (0, 1, v[1].getIntValue());
            m.transpose = (int8_t) juce::jlimit (-12, 24, v[2].getIntValue());
            if ((int) ms.size() < Track::maxMeasures) ms.push_back (m);
        }
    }
}

bool TB3030Processor::savePatternsToFile (const juce::File& f)
{
    if (auto xml = sequenceToXml()) return xml->writeTo (f);
    return false;
}

bool TB3030Processor::loadPatternsFromFile (const juce::File& f)
{
    auto xml = juce::XmlDocument::parse (f);
    if (xml == nullptr || ! xml->hasTagName ("TB3030Patterns")) return false;
    sequenceFromXml (*xml);
    return true;
}

void TB3030Processor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::XmlElement root ("TB3030State");
    root.setAttribute ("version", 1);
    if (auto params = apvts.copyState().createXml()) root.addChildElement (params.release());
    if (auto seqXml = sequenceToXml()) root.addChildElement (seqXml.release());
    auto* ui = root.createNewChildElement ("Panel");
    ui->setAttribute ("mode", mode);
    ui->setAttribute ("group", groupKnob);
    ui->setAttribute ("pattern", selPattern);
    ui->setAttribute ("section", section);
    ui->setAttribute ("octave", octaveShift.load());
    juce::String c; for (int x : chain) c << x << ",";
    ui->setAttribute ("chain", c);
    ui->setAttribute ("bank", banks.bank (currentBank).name);
    ui->setAttribute ("slot", currentSlot);
    copyXmlToBinary (root, destData);
}

void TB3030Processor::setStateInformation (const void* d, int size)
{
    auto root = getXmlFromBinary (d, size);
    if (root == nullptr || ! root->hasTagName ("TB3030State")) return;
    if (auto* params = root->getChildByName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*params));
    if (auto* s = root->getChildByName ("TB3030Patterns")) sequenceFromXml (*s);
    if (auto* ui = root->getChildByName ("Panel"))
    {
        mode = juce::jlimit (0, 3, ui->getIntAttribute ("mode", 2));
        groupKnob = juce::jlimit (0, 6, ui->getIntAttribute ("group", 0));
        selPattern = juce::jlimit (0, 7, ui->getIntAttribute ("pattern", 0));
        section = juce::jlimit (0, 1, ui->getIntAttribute ("section", 0));
        octaveShift.store (juce::jlimit (-12, 12, ui->getIntAttribute ("octave", 0)));
        chain.clear();
        for (auto& t : juce::StringArray::fromTokens (ui->getStringAttribute ("chain"), ",", ""))
            if (t.isNotEmpty()) chain.push_back (juce::jlimit (0, 15, t.getIntValue()));
        if (chain.empty()) chain = { selPattern + 8 * section };
        banks.rescan();
        int bi = banks.indexOf (ui->getStringAttribute ("bank", "Factory"));
        currentBank = bi >= 0 ? bi : 0;
        currentSlot = juce::jlimit (0, SoundBank::numSlots - 1, ui->getIntAttribute ("slot", 0));
    }
    syncSequencerSettings();
}

//==============================================================================
juce::AudioProcessorEditor* TB3030Processor::createEditor() { return new TB3030Editor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new TB3030Processor(); }
