// TB-3030 sound memory banks (Juno-style), 32 sounds per bank = groups A..D x 1..8
#pragma once
#include <juce_core/juce_core.h>
#include "TB303Engine.h"

struct BankSound
{
    juce::String name { "Init" };
    tb303::SoundParams p;
};

class SoundBank
{
public:
    static constexpr int numSlots = 32;

    juce::String name { "User Bank" };
    bool isFactory = false;
    juce::File file;
    BankSound sounds[numSlots];

    std::unique_ptr<juce::XmlElement> toXml() const;
    bool fromXml (const juce::XmlElement& xml);
    bool save() const;

    static SoundBank makeFactory();
    static juce::String slotName (int slot);   // "A-1" .. "D-8"
};

class BankManager
{
public:
    BankManager();

    static juce::File rootFolder();     // Documents/TB-3030
    static juce::File bankFolder();     // Documents/TB-3030/Banks
    static juce::File patternFolder();  // Documents/TB-3030/Patterns

    void rescan();
    int  numBanks() const { return (int) banks.size(); }
    SoundBank&       bank (int i)       { return banks[(size_t) juce::jlimit (0, numBanks() - 1, i)]; }
    const SoundBank& bank (int i) const { return banks[(size_t) juce::jlimit (0, numBanks() - 1, i)]; }
    int  indexOf (const juce::String& bankName) const;

    int  createUserBank (const juce::String& baseName = "User Bank");    // returns index
    int  importBank (const juce::File& f);                               // returns index or -1
    bool exportBank (int index, const juce::File& f) const;
    bool deleteBank (int index);                                          // factory bank cannot be deleted
    juce::String uniqueName (const juce::String& base) const;

private:
    std::vector<SoundBank> banks;   // [0] = factory
};
