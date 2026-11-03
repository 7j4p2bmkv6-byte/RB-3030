# TB-3030 v1.0 source release
Original acid-bass instrument inspired by classic monophonic bass-synth behaviour. No Roland branding, artwork, samples or code are included.

## Complete feature set in this package
- Windows standalone and VST3 targets
- MIDI input and host-tempo sync
- Continuously variable saw-to-square oscillator
- Monophonic legato/glide
- Resonant 24 dB ladder filter model
- Envelope modulation, decay, accent, drive and master volume
- 16-step sequencer with note, enable, accent, slide, octave up/down and variable gate data
- Pattern length support in engine, swing, clear and randomise
- 16-slot factory/user sound bank
- Store sound, save bank to XML and load bank from XML
- DAW state and sequence persistence
- GitHub Actions Windows build and packaged artifacts

## Build
Upload the folder contents to a GitHub repository, enable Actions, then run **Build TB-3030 Windows**. The artifact contains the standalone EXE and VST3 bundle.

Local Visual Studio build:
`cmake -B build -G "Visual Studio 17 2022" -A x64`
`cmake --build build --config Release --target TB3030_All`

## Important validation note
The source implements the complete product workflow and acid synthesis behaviours. Exact hardware matching still depends on listening tests, gain calibration and comparison against a particular physical unit, because original analogue units vary. This project should therefore be treated as the release candidate for sonic tuning, not as a circuit-certified clone.
