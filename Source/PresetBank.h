#pragma once
#include <JuceHeader.h>
class PresetBank{
public:struct Preset{juce::String name;juce::ValueTree state;};void initialise();int size()const{return (int)presets.size();}const Preset&get(int i)const{return presets[(size_t)juce::jlimit(0,size()-1,i)];}void store(int,const juce::String&,const juce::ValueTree&);juce::StringArray names()const;void saveUserBank(const juce::File&)const;void loadUserBank(const juce::File&);
private:std::vector<Preset>presets;};
