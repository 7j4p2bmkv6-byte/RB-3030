#include "SoundBank.h"
#include "FactoryData.h"

//==============================================================================
juce::String SoundBank::slotName (int slot)
{
    slot = juce::jlimit (0, numSlots - 1, slot);
    return juce::String::charToString ((juce::juce_wchar) ('A' + slot / 8)) + "-" + juce::String (slot % 8 + 1);
}

std::unique_ptr<juce::XmlElement> SoundBank::toXml() const
{
    auto xml = std::make_unique<juce::XmlElement> ("TB3030Bank");
    xml->setAttribute ("version", 1);
    xml->setAttribute ("name", name);
    for (int i = 0; i < numSlots; ++i)
    {
        auto* e = xml->createNewChildElement ("Sound");
        const auto& s = sounds[i];
        e->setAttribute ("slot", i);
        e->setAttribute ("name", s.name);
        e->setAttribute ("tuning", s.p.tuning);
        e->setAttribute ("cutoff", s.p.cutoff);
        e->setAttribute ("resonance", s.p.resonance);
        e->setAttribute ("envmod", s.p.envMod);
        e->setAttribute ("decay", s.p.decay);
        e->setAttribute ("accent", s.p.accent);
        e->setAttribute ("waveform", s.p.waveform);
        e->setAttribute ("drive", s.p.drive);
    }
    return xml;
}

bool SoundBank::fromXml (const juce::XmlElement& xml)
{
    if (! xml.hasTagName ("TB3030Bank")) return false;
    name = xml.getStringAttribute ("name", "Imported Bank");
    for (auto* e : xml.getChildWithTagNameIterator ("Sound"))
    {
        int i = e->getIntAttribute ("slot", -1);
        if (i < 0 || i >= numSlots) continue;
        auto& s = sounds[i];
        s.name        = e->getStringAttribute ("name", "Init");
        s.p.tuning    = (float) e->getDoubleAttribute ("tuning", 0.0);
        s.p.cutoff    = (float) e->getDoubleAttribute ("cutoff", 0.35);
        s.p.resonance = (float) e->getDoubleAttribute ("resonance", 0.7);
        s.p.envMod    = (float) e->getDoubleAttribute ("envmod", 0.55);
        s.p.decay     = (float) e->getDoubleAttribute ("decay", 0.4);
        s.p.accent    = (float) e->getDoubleAttribute ("accent", 0.6);
        s.p.waveform  = e->getIntAttribute ("waveform", 0);
        s.p.drive     = (float) e->getDoubleAttribute ("drive", 0.0);
    }
    return true;
}

bool SoundBank::save() const
{
    if (isFactory || file == juce::File()) return false;
    file.getParentDirectory().createDirectory();
    if (auto xml = toXml()) return xml->writeTo (file);
    return false;
}

SoundBank SoundBank::makeFactory()
{
    SoundBank b;
    b.name = "Factory";
    b.isFactory = true;
    const auto* fs = tb303::factorySounds();
    for (int i = 0; i < numSlots; ++i) { b.sounds[i].name = fs[i].name; b.sounds[i].p = fs[i].p; }
    return b;
}

//==============================================================================
BankManager::BankManager() { rescan(); }

juce::File BankManager::rootFolder()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("TB-3030");
}
juce::File BankManager::bankFolder()    { return rootFolder().getChildFile ("Banks"); }
juce::File BankManager::patternFolder() { return rootFolder().getChildFile ("Patterns"); }

void BankManager::rescan()
{
    banks.clear();
    banks.push_back (SoundBank::makeFactory());
    auto folder = bankFolder();
    if (! folder.isDirectory()) return;
    auto files = folder.findChildFiles (juce::File::findFiles, false, "*.tb3bank");
    files.sort();
    for (auto& f : files)
    {
        if (auto xml = juce::XmlDocument::parse (f))
        {
            SoundBank b;
            if (b.fromXml (*xml)) { b.file = f; b.isFactory = false; banks.push_back (b); }
        }
    }
}

int BankManager::indexOf (const juce::String& bankName) const
{
    for (size_t i = 0; i < banks.size(); ++i)
        if (banks[i].name == bankName) return (int) i;
    return -1;
}

juce::String BankManager::uniqueName (const juce::String& base) const
{
    for (int n = 1; n < 1000; ++n)
    {
        auto candidate = base + " " + juce::String (n);
        if (indexOf (candidate) < 0) return candidate;
    }
    return base + " " + juce::String (juce::Random::getSystemRandom().nextInt (99999));
}

int BankManager::createUserBank (const juce::String& baseName)
{
    SoundBank b = SoundBank::makeFactory();   // a new bank starts as a copy of the factory sounds
    b.isFactory = false;
    b.name = uniqueName (baseName);
    b.file = bankFolder().getNonexistentChildFile (juce::File::createLegalFileName (b.name), ".tb3bank", false);
    b.save();
    banks.push_back (b);
    return numBanks() - 1;
}

int BankManager::importBank (const juce::File& f)
{
    auto xml = juce::XmlDocument::parse (f);
    if (xml == nullptr) return -1;
    SoundBank b;
    if (! b.fromXml (*xml)) return -1;
    b.isFactory = false;
    if (b.name.isEmpty() || indexOf (b.name) >= 0) b.name = uniqueName (b.name.isEmpty() ? juce::String ("Imported") : b.name);
    b.file = bankFolder().getNonexistentChildFile (juce::File::createLegalFileName (b.name), ".tb3bank", false);
    b.save();
    banks.push_back (b);
    return numBanks() - 1;
}

bool BankManager::exportBank (int index, const juce::File& f) const
{
    if (auto xml = bank (index).toXml()) return xml->writeTo (f);
    return false;
}

bool BankManager::deleteBank (int index)
{
    if (index <= 0 || index >= numBanks()) return false;
    auto f = banks[(size_t) index].file;
    if (f.existsAsFile()) f.moveToTrash();
    banks.erase (banks.begin() + index);
    return true;
}
