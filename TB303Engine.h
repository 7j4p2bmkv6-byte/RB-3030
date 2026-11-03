// TB-3030 - acid bass line synthesizer
// Pure C++ sound engine + sequencer (no JUCE dependency, unit-testable).
// (c) Landgren Audio. Inspired by the classic 1981 bass line synthesizer.
#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>
#include <algorithm>
#include <random>

namespace tb303
{
static constexpr double kPi = 3.14159265358979323846;

inline float clampf (float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

//==============================================================================
// SOUND PARAMETERS (one "memory" sound)
//==============================================================================
struct SoundParams
{
    float tuning    = 0.0f;  // semitones, -12..+12
    float cutoff    = 0.35f; // 0..1
    float resonance = 0.70f; // 0..1
    float envMod    = 0.55f; // 0..1
    float decay     = 0.40f; // 0..1
    float accent    = 0.60f; // 0..1
    int   waveform  = 0;     // 0 = saw, 1 = square
    float drive     = 0.0f;  // 0..1  (TB-3030 addition)
};

//==============================================================================
// SEQUENCER DATA
//==============================================================================
enum class TimeType : uint8_t { Note = 0, Tie = 1, Rest = 2 };

struct Step
{
    uint8_t  note   = 0;      // 0..12 (C .. upper C)
    bool     up     = false;
    bool     down   = false;
    bool     accent = false;
    bool     slide  = false;
    TimeType time   = TimeType::Rest;
};

struct Pattern
{
    static constexpr int maxSteps = 16;
    Step steps[maxSteps];
    int  length  = 16;
    bool triplet = false;

    void clear()
    {
        for (auto& s : steps) s = Step();
        length = 16; triplet = false;
    }
};

struct Measure
{
    uint8_t pattern   = 0;  // 0..7
    uint8_t section   = 0;  // 0 = A, 1 = B
    int8_t  transpose = 0;  // -12..+24
};

struct Track
{
    static constexpr int maxMeasures = 64;
    std::vector<Measure> measures;
};

// 4 groups (I..IV) x 8 patterns x 2 sections (A/B) = 64 patterns. 7 tracks.
struct SequenceData
{
    Pattern patterns[4][16]; // [group][pattern + 8*section]
    Track   tracks[7];

    static int groupForTrack (int track) { static const int g[7] = { 0,0,1,1,2,2,3 }; return g[std::clamp (track, 0, 6)]; }
    Pattern&       get (int group, int idx)       { return patterns[std::clamp (group,0,3)][std::clamp (idx,0,15)]; }
    const Pattern& get (int group, int idx) const { return patterns[std::clamp (group,0,3)][std::clamp (idx,0,15)]; }
};

// "C Dua D#s - . C+da" style text -> pattern (used for factory patterns)
inline Pattern parsePattern (const std::string& text, bool triplet = false)
{
    Pattern p; p.clear(); p.triplet = triplet;
    static const char* names[] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
    int n = 0; size_t i = 0;
    while (i < text.size() && n < Pattern::maxSteps)
    {
        while (i < text.size() && text[i] == ' ') ++i;
        if (i >= text.size()) break;
        size_t j = i; while (j < text.size() && text[j] != ' ') ++j;
        std::string tok = text.substr (i, j - i); i = j;
        Step s;
        if (tok == ".")      s.time = TimeType::Rest;
        else if (tok == "-") s.time = TimeType::Tie;
        else
        {
            s.time = TimeType::Note;
            int note = 0; size_t k = 0;
            for (int q = 11; q >= 0; --q)
            {
                std::string nm = names[q];
                if (tok.compare (0, nm.size(), nm) == 0 && (nm.size() == 2 || tok.size() == 1 || tok[1] != '#'))
                { note = q; k = nm.size(); break; }
            }
            if (k < tok.size() && tok[k] == '+') { note = 12; ++k; }
            s.note = (uint8_t) note;
            for (; k < tok.size(); ++k)
            {
                if (tok[k] == 'u') s.up = true;
                if (tok[k] == 'd') s.down = true;
                if (tok[k] == 'a') s.accent = true;
                if (tok[k] == 's') s.slide = true;
            }
        }
        p.steps[n++] = s;
    }
    p.length = std::max (1, n);
    return p;
}

//==============================================================================
// VOICE - monophonic 303-style voice, internally 2x oversampled
//==============================================================================
class Voice
{
public:
    void setSampleRate (double sr)
    {
        fsBase = sr; fs = sr * 2.0;
        slideCoef  = coefFor (0.022);   // ~60 ms glide (RC like)
        attackCoef = coefFor (0.003);
        releaseCoef= coefFor (0.008);
        sustainDecayCoef = coefFor (4.0);
        accentDecayCoef  = coefFor (0.20);
        // decimation filter (2x biquad Butterworth at 0.42 * base Nyquist)
        designLowpass (sr * 0.45, fs);
        hpFbCoef = (float) std::exp (-2.0 * kPi * 140.0 / fs);
        dcCoef   = (float) std::exp (-2.0 * kPi * 12.0 / sr);
        reset();
    }

    void reset()
    {
        phase = 0; meg = 0; amp = 0; accEnv = 0; accSmooth = 0; gate = false;
        for (auto& s : st) { s = 0; }
        fbHp = 0; fbHpIn = 0; lastOut = 0;
        for (auto& z : dz) { z = 0; }
        dcX = dcY = 0; driveLp = 0;
    }

    void setParams (const SoundParams& p) { params = p; }

    // pitch in MIDI note numbers (float), legato = slide from current note, no retrigger
    void noteOn (float midiNote, bool accented, bool legato)
    {
        targetPitch = midiNote;
        if (legato && gate)
        {
            sliding = true;
            if (accented) { accEnv = 1.0f; accentedNote = true; }
            return;
        }
        sliding = false;
        pitch = midiNote;
        gate = true;
        accentedNote = accented;
        meg = 1.0f;
        if (accented) accEnv = 1.0f;
        float dec = accented ? 0.20f : (0.20f + 2.3f * std::pow (params.decay, 1.6f));
        megCoef = (float) coefFor (dec / 4.6); // reaches ~1% after 'dec'
    }

    void noteOff() { gate = false; }
    bool isGateOn() const { return gate; }
    bool isActive() const { return gate || amp > 1.0e-4f; }

    // adds mono output into 'out'
    void render (float* out, int n, float volume)
    {
        const float k = 4.15f * std::pow (params.resonance, 0.85f);          // feedback amount
        const float accentAmt = params.accent;
        // accent "sweep" smoothing tau grows with resonance (the famous wow-effect)
        const float accSmoothCoef = (float) coefFor (0.004 + 0.045 * params.resonance);
        const float baseHz = 180.0f * std::pow (2.0f, params.cutoff * 4.6f);     // ~180 Hz .. ~4.4 kHz
        const float envOct = params.envMod * 4.2f;
        const float baseShift = -params.envMod * 0.9f;                            // env mod lowers resting cutoff
        const bool square = params.waveform == 1;
        const float drive = params.drive;
        const float preGain = 1.0f + drive * drive * 24.0f;
        const float driveComp = 1.0f / (1.0f + drive * 1.6f);

        for (int i = 0; i < n; ++i)
        {
            float acc2[2];
            for (int os = 0; os < 2; ++os)
            {
                // --- pitch / slide
                if (sliding) pitch += (targetPitch - pitch) * (1.0f - slideCoef);
                float hz = 440.0f * std::pow (2.0f, (pitch + params.tuning - 69.0f) / 12.0f);
                float dt = hz / (float) fs;

                // --- oscillator (PolyBLEP)
                phase += dt; if (phase >= 1.0f) phase -= 1.0f;
                float osc;
                if (! square)
                {
                    osc = 1.0f - 2.0f * phase;      // falling ramp like the original
                    osc += polyBlep (phase, dt);
                }
                else
                {
                    // 303 square: derived from the saw through a transistor shaper -> slightly sloped tops
                    float sq = phase < 0.5f ? 1.0f : -1.0f;
                    sq += polyBlep (phase, dt);
                    float p2 = phase + 0.5f; if (p2 >= 1.0f) p2 -= 1.0f;
                    sq -= polyBlep (p2, dt);
                    osc = 0.82f * sq + 0.18f * (1.0f - 2.0f * phase) * 0.6f;
                }

                // --- envelopes
                meg *= megCoef;
                accEnv *= accentDecayCoef;
                accSmooth += (accEnv * (accentedNote ? 1.0f : 0.0f) - accSmooth) * (1.0f - accSmoothCoef);
                if (gate) { amp += (1.0f - amp) * (1.0f - attackCoef); ampSus *= sustainDecayCoef; }
                else      { amp *= releaseCoef; }
                if (gate && amp > 0.999f) amp = 0.999f;

                // --- cutoff
                float oct = baseShift + meg * envOct * (accentedNote ? 0.75f : 1.0f) + accSmooth * accentAmt * 3.0f;
                float fc = baseHz * std::pow (2.0f, oct);
                fc = clampf (fc, 20.0f, (float) fs * 0.42f);
                float g = 1.0f - std::exp (-2.0f * (float) kPi * fc / (float) fs);

                // --- 4-pole ladder with high-passed resonance feedback (diode-ladder flavour)
                float fbIn = st[3];
                fbHp = hpFbCoef * (fbHp + fbIn - fbHpIn); fbHpIn = fbIn;
                float x = osc * 0.9f - k * fbHp;
                x = std::tanh (x);
                st[0] += g * 1.05f * (x - st[0]);
                st[1] += g * (st[0] - st[1]);
                st[2] += g * (st[1] - st[2]);
                st[3] += g * (st[2] - st[3]);
                float y = st[3] * (1.0f + k * 0.55f);

                // --- VCA with accent boost
                float vca = amp * (1.0f + accentAmt * 0.9f * (accentedNote ? accEnv : 0.0f) + 0.2f * accentAmt * accSmooth);
                acc2[os] = y * vca;
            }
            // decimate
            float d = 0.0f;
            for (int os = 0; os < 2; ++os) d = biquad (acc2[os]);
            float s = d;

            // --- drive stage (TB-3030 addition)
            if (drive > 0.001f)
            {
                float v = s * preGain;
                float sat = v >= 0 ? std::tanh (v) : std::tanh (v * 0.8f) * 1.1f;
                driveLp += (sat - driveLp) * (0.35f + 0.5f * (1.0f - drive));
                s = (s * (1.0f - drive) + driveLp * drive * 0.9f) * (1.0f + drive * 0.4f) * driveComp * 1.6f;
            }

            // --- DC blocker + volume
            float yy = s - dcX + dcCoef * dcY; dcX = s; dcY = yy;
            out[i] += yy * volume * 0.42f;
        }
    }

private:
    double coefFor (double tauSeconds) const { return std::exp (-1.0 / (tauSeconds * fs)); }

    static float polyBlep (float t, float dt)
    {
        if (t < dt)        { t /= dt; return t + t - t * t - 1.0f; }
        if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
        return 0.0f;
    }

    void designLowpass (double fc, double sr)
    {
        // 2 cascaded biquads (4th order Butterworth)
        const double qs[2] = { 0.54119610, 1.3065630 };
        for (int b = 0; b < 2; ++b)
        {
            double w0 = 2.0 * kPi * fc / sr, cw = std::cos (w0), al = std::sin (w0) / (2.0 * qs[b]);
            double a0 = 1.0 + al;
            bc[b][0] = (float) ((1.0 - cw) * 0.5 / a0);
            bc[b][1] = (float) ((1.0 - cw) / a0);
            bc[b][2] = bc[b][0];
            bc[b][3] = (float) (-2.0 * cw / a0);
            bc[b][4] = (float) ((1.0 - al) / a0);
        }
    }
    float biquad (float x)
    {
        for (int b = 0; b < 2; ++b)
        {
            float* z = &dz[b * 2]; const float* c = bc[b];
            float y = c[0] * x + z[0];
            z[0] = c[1] * x - c[3] * y + z[1];
            z[1] = c[2] * x - c[4] * y;
            x = y;
        }
        return x;
    }

    SoundParams params;
    double fsBase = 44100.0, fs = 88200.0;
    float phase = 0, pitch = 36, targetPitch = 36;
    bool gate = false, sliding = false, accentedNote = false;
    float meg = 0, megCoef = 0.999f, amp = 0, ampSus = 1, accEnv = 0, accSmooth = 0;
    float slideCoef = 0.999f, attackCoef = 0.99f, releaseCoef = 0.99f, sustainDecayCoef = 1.0f, accentDecayCoef = 0.999f;
    float st[4] = { 0,0,0,0 }, fbHp = 0, fbHpIn = 0, hpFbCoef = 0.99f, lastOut = 0;
    float bc[2][5] = {}, dz[4] = {};
    float dcX = 0, dcY = 0, dcCoef = 0.999f, driveLp = 0;
};

//==============================================================================
// SEQUENCER
//==============================================================================
enum class Mode : int { TrackWrite = 0, TrackPlay = 1, PatternPlay = 2, PatternWrite = 3 };

struct PatternRef { int group = 0; int index = 0; int transpose = 0; };

class Sequencer
{
public:
    // ---- configuration set by the host/UI (audio thread reads them)
    Mode mode = Mode::PatternPlay;
    int  groupKnob = 0;                 // 0..6 (positions 1..7)
    std::vector<int> chain { 0 };       // pattern indexes (0..15) for pattern play, in current group
    std::vector<int> queuedChain;       // applied at the end of the current pattern
    bool hasQueued = false;
    int  editPattern = 0;               // pattern edited in pattern write (index 0..15)
    int  externalTranspose = 0;         // from MIDI keyboard / octave keys
    float shuffle = 0.0f;               // 0..1  (TB-3030 addition)

    // ---- display info (read by UI)
    int  displayStep = -1;
    int  displayPattern = 0;
    int  displayMeasure = 0;

    bool isRunning() const { return running; }

    void start()
    {
        running = true; started = false; stepCounter = 0; stepPos = 0.0;
        nextStepPos = 0.0; gateOffPos = -1.0; prevHeld = false;
    }
    void stop (Voice& v)
    {
        running = false; v.noteOff(); displayStep = -1; prevHeld = false; gateOffPos = -1.0;
        if (hasQueued) { chain = queuedChain; hasQueued = false; }
    }

    int currentGroup() const
    {
        if (mode == Mode::TrackPlay || mode == Mode::TrackWrite) return SequenceData::groupForTrack (groupKnob);
        static const int g[7] = { 0,0,1,1,2,2,3 };
        return g[std::clamp (groupKnob, 0, 6)];
    }
    int currentTrack() const { return std::clamp (groupKnob, 0, 6); }

    // Re-align to an absolute host position (in steps). Called when host transport jumps/starts.
    void syncTo (double absSteps)
    {
        running = true; started = false; prevHeld = false; gateOffPos = -1.0;
        stepPos = absSteps;
        double k = std::ceil (absSteps - 1.0e-9);
        stepCounter = (int64_t) k;
        nextStepPos = k + swingOffset ((int64_t) k);
        alignIndex = (int64_t) k;
    }

    // Advance by 'n' samples and render the voice into 'out'.
    // stepsPerSample = bpm/60*stepsPerBeat/sr (computed by caller via stepsPerSampleFor()).
    void process (const SequenceData& data, Voice& voice, float* out, int n, double bpm, double sampleRate, float volume)
    {
        int done = 0;
        while (done < n)
        {
            if (! running) { voice.render (out + done, n - done, volume); return; }
            const double sps = stepsPerSampleFor (data, bpm, sampleRate);
            // next event position (in steps)
            double evPos = nextStepPos;
            bool isGateOff = false;
            if (gateOffPos >= 0.0 && gateOffPos < evPos) { evPos = gateOffPos; isGateOff = true; }
            double samplesToEv = (evPos - stepPos) / sps;
            int chunk = (int) std::ceil (samplesToEv - 1.0e-9);
            if (chunk < 0) chunk = 0;
            if (chunk >= n - done)
            {
                voice.render (out + done, n - done, volume);
                stepPos += (n - done) * sps;
                return;
            }
            if (chunk > 0) { voice.render (out + done, chunk, volume); done += chunk; stepPos += chunk * sps; }
            // fire event
            if (isGateOff) { voice.noteOff(); gateOffPos = -1.0; }
            else
            {
                fireStep (data, voice);
                ++stepCounter;
                nextStepPos = (double) stepCounter + swingOffset (stepCounter);
            }
        }
    }

    double stepsPerBeatFor (const SequenceData& data) const
    {
        PatternRef r; const Pattern* p = currentPatternPtr (data, r);
        return (p != nullptr && p->triplet) ? 6.0 : 4.0;
    }
    double stepsPerSampleFor (const SequenceData& data, double bpm, double sr) const
    {
        return bpm / 60.0 * stepsPerBeatFor (data) / sr;
    }

    // Returns the pattern a given chain/measure position refers to.
    PatternRef refAt (const SequenceData& data, int pos) const
    {
        PatternRef r;
        if (mode == Mode::TrackPlay || mode == Mode::TrackWrite)
        {
            const auto& tr = data.tracks[currentTrack()];
            r.group = SequenceData::groupForTrack (currentTrack());
            if (tr.measures.empty()) { r.index = -1; return r; }
            const auto& m = tr.measures[(size_t) (pos % (int) tr.measures.size())];
            r.index = m.pattern + 8 * m.section; r.transpose = m.transpose;
            return r;
        }
        r.group = currentGroup();
        if (mode == Mode::PatternWrite) { r.index = editPattern; return r; }
        if (chain.empty()) { r.index = 0; return r; }
        r.index = chain[(size_t) (pos % (int) chain.size())];
        return r;
    }
    int chainLength (const SequenceData& data) const
    {
        if (mode == Mode::TrackPlay || mode == Mode::TrackWrite) return std::max (1, (int) data.tracks[currentTrack()].measures.size());
        if (mode == Mode::PatternWrite) return 1;
        return std::max (1, (int) chain.size());
    }

private:
    const Pattern* currentPatternPtr (const SequenceData& data, PatternRef& r) const
    {
        r = refAt (data, std::max (0, cursorChain));
        if (r.index < 0) return nullptr;
        return &data.get (r.group, r.index);
    }

    double swingOffset (int64_t k) const { return (k & 1) ? shuffle * 0.33 : 0.0; }

    struct Cursor { int chainPos = 0; int step = 0; };

    Cursor nextCursor (const SequenceData& data, Cursor c) const
    {
        PatternRef r = refAt (data, c.chainPos);
        int len = r.index < 0 ? 16 : data.get (r.group, r.index).length;
        c.step++;
        if (c.step >= len) { c.step = 0; c.chainPos = (c.chainPos + 1) % chainLength (data); }
        return c;
    }

    const Step* stepAt (const SequenceData& data, Cursor c, PatternRef& r) const
    {
        r = refAt (data, c.chainPos);
        if (r.index < 0) return nullptr;
        const auto& p = data.get (r.group, r.index);
        return &p.steps[std::clamp (c.step, 0, p.length - 1)];
    }

    void fireStep (const SequenceData& data, Voice& voice)
    {
        if (! started)
        {
            started = true;
            // align with host bar when synced mid-way
            cur.chainPos = 0;
            PatternRef r0 = refAt (data, 0);
            int len = r0.index < 0 ? 16 : data.get (r0.group, r0.index).length;
            cur.step = (int) (((alignIndex % len) + len) % len);
            alignIndex = 0;
        }
        else
        {
            Cursor nc = nextCursor (data, cur);
            if (nc.chainPos != cur.chainPos || (nc.step == 0 && chainLength (data) == 1))
            {
                if (nc.step == 0 && hasQueued && mode == Mode::PatternPlay)
                { chain = queuedChain; hasQueued = false; nc.chainPos = 0; }
            }
            cur = nc;
        }
        cursorChain = cur.chainPos;

        PatternRef r; const Step* s = stepAt (data, cur, r);
        displayStep = cur.step; displayPattern = r.index; displayMeasure = cur.chainPos;
        double here = (double) stepCounter + swingOffset (stepCounter);
        double stepLen = 1.0 + swingOffset (stepCounter + 1) - swingOffset (stepCounter);

        if (s == nullptr) { voice.noteOff(); prevHeld = false; return; }

        PatternRef rn; const Step* nx = stepAt (data, nextCursor (data, cur), rn);
        bool nextIsTie = nx != nullptr && nx->time == TimeType::Tie;

        if (s->time == TimeType::Rest)
        {
            voice.noteOff(); prevHeld = false; gateOffPos = -1.0;
            return;
        }
        if (s->time == TimeType::Tie)
        {
            if (! voice.isGateOn()) { prevHeld = false; return; }    // tie after a rest = rest
            bool hold = nextIsTie || s->slide;
            if (s->slide && nx && nx->time == TimeType::Note) hold = true;
            gateOffPos = hold ? -1.0 : here + stepLen * 0.5;
            prevHeld = hold;
            return;
        }
        int midi = 36 + s->note + (s->up ? 12 : 0) - (s->down ? 12 : 0) + r.transpose + externalTranspose;
        midi = std::clamp (midi, 12, 96);
        bool legato = prevHeld && voice.isGateOn();
        voice.noteOn ((float) midi, s->accent, legato);
        bool hold = (s->slide && nx && nx->time != TimeType::Rest) || nextIsTie;
        gateOffPos = hold ? -1.0 : here + stepLen * 0.5;
        prevHeld = hold;
    }

    bool running = false, started = false, prevHeld = false;
    int64_t stepCounter = 0, alignIndex = 0;
    double stepPos = 0.0, nextStepPos = 0.0, gateOffPos = -1.0;
    Cursor cur;
    int cursorChain = 0;
};

//==============================================================================
// Acid pattern randomizer (TB-3030 addition)
//==============================================================================
inline Pattern randomPattern (uint32_t seed, int length = 16)
{
    std::mt19937 rng (seed);
    std::uniform_real_distribution<float> u (0.0f, 1.0f);
    static const int scale[] = { 0, 3, 5, 7, 10, 12, 0, 0, 7 }; // minor-pentatonic flavoured
    Pattern p; p.clear(); p.length = std::clamp (length, 1, 16);
    for (int i = 0; i < p.length; ++i)
    {
        Step s;
        float r = u (rng);
        s.time = r < 0.68f ? TimeType::Note : (r < 0.82f ? TimeType::Tie : TimeType::Rest);
        if (i == 0) s.time = TimeType::Note;
        s.note = (uint8_t) scale[(int) (u (rng) * 9) % 9];
        float o = u (rng);
        s.up = o > 0.82f; s.down = o < 0.12f;
        s.accent = u (rng) < 0.28f;
        s.slide  = u (rng) < 0.22f;
        p.steps[i] = s;
    }
    return p;
}

} // namespace tb303
