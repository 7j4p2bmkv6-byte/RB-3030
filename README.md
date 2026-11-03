# TB-3030 v1.0 - Landgren Audio

A clone of the classic 1981 computer-controlled bass line synth. It runs as a stand-alone Windows app and as a VST3 instrument, and adds Juno-style sound memory banks.

## The panel (like the original)
| Control | What it does |
|---|---|
| WAVEFORM switch (top left) | Saw / square |
| TUNING | +/- 12 semitones (double-click = 0) |
| CUT OFF FREQ, RESONANCE, ENV MOD, DECAY, ACCENT | The classic acid controls. Accented notes use a short fixed decay, and successive accents build up the "wow" sweep (stronger at high resonance) |
| TEMPO | 40-300 BPM (internal clock) |
| TRACK / PATT. GROUP | Positions 1-7. In pattern modes: group I (1-2), II (3-4), III (5-6), IV (7). In track modes: track 1-7 |
| MODE | TRACK WRITE, TRACK PLAY, PATTERN PLAY, PATTERN WRITE |
| VOLUME | Output level |

Turn the knobs and rotary switches by dragging up/down or with the mouse wheel. Hovering shows the value.

## Playing patterns (MODE = PATTERN PLAY)
- **Keys C D E F G A B C** = patterns **1-8**. ACCENT = section **A**, SLIDE = section **B**.
- **Shift-click** pattern keys to chain patterns (e.g. 1A > 2A > 1B). Shift-click A or B to chain the A and B sections.
- A change made while running takes effect at the end of the current pattern.
- RUN/STOP (or Space) starts and stops. DOWN/UP shift everything one octave.
- A MIDI keyboard transposes the running pattern (C3 = no transposition).

## Writing patterns (MODE = PATTERN WRITE)
**Pitch mode** (the PITCH MODE LED is lit):
1. Press a note key (C-C'). You hear the note.
2. DOWN / UP / ACCENT / SLIDE toggle those flags on the step.
3. WRITE/NEXT moves to the next step and BACK moves to the previous one.

**Time mode** (press TIME MODE):
- DOWN = note, UP = tie, ACCENT = rest. Each press moves to the next step automatically.

**FUNCTION** (LED lit):
- Note keys 1-8 and A/B select the pattern to edit.
- UP (the triplet key) toggles triplet timing.
- WRITE/NEXT ends the pattern at the cursor step, and BACK resets it to 16 steps.
- PATTERN CLEAR erases the pattern. It also works outside FUNCTION.

**Step display (TB-3030 addition):** shows the 16 steps with note, D/U/A/S flags, ties and rests.
- Click a cell to select it and right-click for a full edit menu.
- Use the mouse wheel to change the note and double-click to toggle note/rest.

## Tracks (MODE = TRACK WRITE / TRACK PLAY)
- The TRACK knob picks track 1-7. Tracks 1-2 use group I, 3-4 group II, 5-6 group III and 7 group IV.
- **Track write:** keys 1-8 + A/B set the pattern for the current bar. WRITE/NEXT goes to the next bar (or adds one at the end) and BACK goes back.
  - C# = **DEL** bar and D# = **INS** bar. PATTERN CLEAR erases the track.
  - With FUNCTION on, the note keys set the bar's transposition (C-C') and DOWN/UP shift it by an octave.
- **Track play:** RUN plays the track in a loop (up to 64 bars).

## Sound memory (TB-3030 addition, like the Juno)
- Every bank holds 32 sounds: **groups A-D x numbers 1-8**. Use BANK < > to change bank and SOUND < > to step through sounds.
- **WRITE** stores the panel sound (all knobs, waveform and drive) with a name. The factory bank is read-only, so the first time you write, *User Bank 1* is created.
- **BANK...** has New / Import / Export / Rename / Delete. Banks are `.tb3bank` files in `Documents\TB-3030\Banks`.
- An asterisk (`*`) on the display means the sound has been changed since it was loaded.
- In a DAW, the 32 sounds also appear as plug-in programs.

## Other additions
- **DRIVE:** acid distortion, from clean to fuzz.
- **SHUFFLE:** swing.
- **CLOCK INT/HOST:** with HOST, the sequencer follows the DAW's tempo and transport.
- **PATTERNS...:** copy, paste, randomise (acid generator), clear, save/load all patterns and tracks (`.tb3pat`) and restore the factory patterns.
- **Computer keyboard:** A W S E D F T G Y H U J K play notes, and Z / X change the octave.
- **MIDI:** notes play the synth when the sequencer is stopped. Velocity 100+ gives an accent and overlapping (legato) notes slide.
  - CC74 cutoff, CC71 resonance, CC12 env mod, CC13 decay, CC16 accent, CC17 drive, CC18 tuning and CC7 volume.
- Patterns, tracks and the panel state are saved with your DAW project and remembered by the stand-alone app.

## Factory content
- **32 sounds:** A = classic acid, B = resonant leads/squelch, C = deep and sub, D = distorted (DRIVE).
- **Patterns:** group I has 16 acid patterns (1A-8B). Group II has 11 patterns, including a triplet groove at 3B.
- **Demo tracks:** tracks 1 and 3.

## Building
See BUILD_GUIDE.md.
