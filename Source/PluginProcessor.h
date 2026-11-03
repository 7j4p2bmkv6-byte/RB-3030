#pragma once
#include <JuceHeader.h>
#include "AcidEngine.h"
#include "Sequencer.h"
#include "PresetBank.h"
class TB3030AudioProcessor:public juce::AudioProcessor{
public:TB3030AudioProcessor();void prepareToPlay(double,int)override;void releaseResources()override{};bool isBusesLayoutSupported(const BusesLayout&)const override;void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&)override;juce::AudioProcessorEditor*createEditor()override;bool hasEditor()const override{return true;}const juce::String getName()const override{return "TB-3030";}bool acceptsMidi()const override{return true;}bool producesMidi()const override{return false;}bool isMidiEffect()const override{return false;}double getTailLengthSeconds()const override{return 0;}int getNumPrograms()override{return bank.size();}int getCurrentProgram()override{return program;}void setCurrentProgram(int)override;const juce::String getProgramName(int)override;void changeProgramName(int,const juce::String&)override;void getStateInformation(juce::MemoryBlock&)override;void setStateInformation(const void*,int)override;static juce::AudioProcessorValueTreeState::ParameterLayout layout();void loadPreset(int);void storePreset(int,const juce::String&);
 juce::AudioProcessorValueTreeState state;AcidSequencer seq;PresetBank bank;
private:AcidEngine engine;int program=0;juce::Random random;JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TB3030AudioProcessor)};
