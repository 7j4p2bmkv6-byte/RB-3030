// TB-3030 factory sounds and factory patterns (all names and patterns are original)
#pragma once
#include "TB303Engine.h"

namespace tb303
{
struct NamedSound { const char* name; SoundParams p; };

// 32 factory sounds = groups A..D x 1..8
//                      name                 tune  cut   res   env   dec   acc   wave drive
inline const NamedSound* factorySounds()
{
    static const NamedSound s[32] = {
        // A - classic acid
        { "Classic Acid",      {  0.0f, 0.30f, 0.72f, 0.60f, 0.42f, 0.65f, 0, 0.00f } },
        { "Squelch Line",      {  0.0f, 0.22f, 0.86f, 0.78f, 0.35f, 0.80f, 0, 0.00f } },
        { "Rubber Bass",       {  0.0f, 0.18f, 0.55f, 0.48f, 0.55f, 0.50f, 0, 0.00f } },
        { "Warehouse 88",      {  0.0f, 0.34f, 0.80f, 0.66f, 0.30f, 0.75f, 0, 0.10f } },
        { "Hollow Square",     {  0.0f, 0.36f, 0.68f, 0.55f, 0.40f, 0.60f, 1, 0.00f } },
        { "Chicago Jack",      {  0.0f, 0.26f, 0.78f, 0.70f, 0.48f, 0.70f, 0, 0.05f } },
        { "Phuture Shock",     {  0.0f, 0.15f, 0.92f, 0.85f, 0.25f, 0.90f, 0, 0.00f } },
        { "Clean Sequence",    {  0.0f, 0.42f, 0.45f, 0.35f, 0.38f, 0.40f, 0, 0.00f } },
        // B - resonant leads & squelch
        { "Screamer",          { 12.0f, 0.40f, 0.94f, 0.80f, 0.32f, 0.85f, 0, 0.25f } },
        { "Wah Machine",       {  0.0f, 0.12f, 0.90f, 0.95f, 0.70f, 0.70f, 0, 0.00f } },
        { "Glass Whistle",     { 12.0f, 0.55f, 0.96f, 0.40f, 0.30f, 0.50f, 1, 0.00f } },
        { "Talking Filter",    {  0.0f, 0.20f, 0.88f, 0.75f, 0.85f, 0.65f, 0, 0.00f } },
        { "Laser Pluck",       {  7.0f, 0.30f, 0.82f, 0.90f, 0.10f, 0.60f, 1, 0.00f } },
        { "Acid Lead",         { 12.0f, 0.46f, 0.80f, 0.60f, 0.50f, 0.70f, 0, 0.35f } },
        { "Bubble Square",     {  0.0f, 0.28f, 0.84f, 0.68f, 0.22f, 0.75f, 1, 0.00f } },
        { "Resonator",         {  0.0f, 0.33f, 0.98f, 0.50f, 0.60f, 0.55f, 0, 0.00f } },
        // C - deep & sub
        { "Sub Pulse",         { -12.0f,0.10f, 0.30f, 0.25f, 0.65f, 0.30f, 1, 0.00f } },
        { "Dark Matter",       {  0.0f, 0.08f, 0.62f, 0.35f, 0.70f, 0.45f, 0, 0.00f } },
        { "Round Bottom",      {  0.0f, 0.14f, 0.40f, 0.30f, 0.80f, 0.35f, 1, 0.00f } },
        { "Night Drive",       {  0.0f, 0.16f, 0.66f, 0.45f, 0.55f, 0.55f, 0, 0.12f } },
        { "Minimal Thump",     {  0.0f, 0.05f, 0.50f, 0.60f, 0.12f, 0.80f, 0, 0.00f } },
        { "Deep Square",       { -12.0f,0.22f, 0.55f, 0.40f, 0.45f, 0.40f, 1, 0.00f } },
        { "Dub Acid",          {  0.0f, 0.12f, 0.75f, 0.50f, 0.90f, 0.40f, 0, 0.00f } },
        { "Velvet Line",       {  0.0f, 0.24f, 0.35f, 0.20f, 0.60f, 0.25f, 0, 0.00f } },
        // D - distorted / aggressive (uses DRIVE)
        { "Distorted 303",     {  0.0f, 0.28f, 0.82f, 0.70f, 0.40f, 0.75f, 0, 0.60f } },
        { "Hardfloor",         {  0.0f, 0.24f, 0.90f, 0.80f, 0.35f, 0.85f, 0, 0.75f } },
        { "Fuzz Square",       {  0.0f, 0.30f, 0.70f, 0.60f, 0.40f, 0.60f, 1, 0.70f } },
        { "Overdrive Rave",    {  0.0f, 0.36f, 0.86f, 0.75f, 0.30f, 0.80f, 0, 0.50f } },
        { "Industrial",        { -12.0f,0.20f, 0.80f, 0.85f, 0.25f, 0.90f, 1, 0.90f } },
        { "Burner",            {  0.0f, 0.18f, 0.95f, 0.90f, 0.45f, 0.95f, 0, 0.85f } },
        { "Crunch Sequence",   {  0.0f, 0.40f, 0.60f, 0.50f, 0.35f, 0.55f, 0, 0.45f } },
        { "Total Acid",        {  0.0f, 0.22f, 0.97f, 0.95f, 0.50f, 1.00f, 0, 1.00f } },
    };
    return s;
}

// Factory patterns for group I (1A..8A, 1B..8B) and group II.
// token: note [C C# D ... B C+] + flags u(p) d(own) a(ccent) s(lide);  '-' = tie  '.' = rest
inline void loadFactoryPatterns (SequenceData& d)
{
    for (int g = 0; g < 4; ++g) for (int i = 0; i < 16; ++i) d.patterns[g][i].clear();
    for (auto& t : d.tracks) t.measures.clear();

    const char* groupI[16] = {
        // A section
        "Cd Cd C Cda Cd D#s F Cd Cd C C# Cd A#d C Cds C",
        "C C+a C D#s F - C G#da C C+ A# G F D# Cs C",
        "Cd . C Cd D#a Cd . C+s C Cd G#d Cd Fa . Cs D#",
        "A#d A#d A# A#da Cs D# A#d A#d F A#d G#a A#d . A# A#d A#s",
        "C C D#a Cs Gd Cd F Cd C D#a G Fs D# C . Cd",
        "Cd - C C+as C G F#a F D#s C Cd C - Gd A#ds C",
        "F#d F#d F# F#a Ds E F#d . F#da C# F#d F#s G# F# E F#d",
        "C . Cd C+a . C D#d Cs G . Cd A#a . Cd Gs D#",
        // B section
        "Cd Cd C Cda Cd D#s F Cd Gd Gd G Gda Gd A#s C+ Gd",
        "C+a C+ G D#s C A#d Ca C F#a F D# C Gd A#ds C D#",
        "Cd Cd D#d Fd Gda A#ds C D# Fa D# C A#d Gds F D# C",
        "A#d A# F A#a A#d C+s A# G#da A#d A#d C# D#a Fs D# C# A#d",
        "C D#a Gs C+ G D# C Gd C D#a Fs F# G A#ds C C",
        "Cda C Cd Cd Cs D# C C+a Cd C - Gds G# G F D#",
        "F#da F#d A F#d C#a C# Ds E F#d F# Ba F# Es D C# F#d",
        "C+as C+ G G# Gs F D#a Ds C Cd A#da Gd A#ds C Cd C",
    };
    for (int i = 0; i < 16; ++i) d.patterns[0][i] = parsePattern (groupI[i]);

    const char* groupII[8] = {
        "Ed Eda E Ed Gs Ed A Ed Ed Ea Bd E Ed Ds Ed D",
        "Gd G Gd A#a Gd Cs G Gd Fd Fa F Fd G#s F Fd D#",
        "C Cd C Da Ds D# Cd C+a C G#d C Cd A#ds C Cd Ga",
        "D Dd D Fa Ds A Dd D C+a Dd A Dd Fs G Dd A",
        "Cd . Cd Cd D#da . Cd Cd Gd . A#d G#d Gda Fs D# Cd",
        "Cs D#s Fs Gs A#s C+as A#s Gs Fs D#s C Cd . Cd C+a .",
        "A#d A# A#d A#a Cs D Fd A#d G#d G#a G#d Cs D# F G#d G#",
        "E Ea Ed Es G A Ed Ea B Ba Bd Bs A G Ed E",
    };
    for (int i = 0; i < 8; ++i) d.patterns[1][i] = parsePattern (groupII[i]);
    d.patterns[1][8] = parsePattern ("C C C D#a C Fs G C C# C Ca G#d . C D# Gs", false);
    d.patterns[1][9] = parsePattern ("Cd C D#a Cd Fs G A#da C+ G", false);
    d.patterns[1][9].length = 9;
    d.patterns[1][10] = parsePattern ("Cd C+a G Cd D#s F Cd C+ G Cd D#a F", true);  // triplet groove

    // Demo tracks: Track 1 (group I) and Track 3 (group II)
    auto& t1 = d.tracks[0].measures;
    t1 = { {0,0,0},{0,0,0},{0,1,0},{0,0,5},{1,0,0},{1,0,0},{1,1,0},{4,0,7} };
    auto& t3 = d.tracks[2].measures;
    t3 = { {0,0,0},{1,0,0},{0,0,0},{1,1,-2},{2,0,0},{3,0,0},{2,0,0},{5,0,0} };
}

} // namespace tb303
