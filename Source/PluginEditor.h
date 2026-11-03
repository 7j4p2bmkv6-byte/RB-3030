#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
class TB3030AudioProcessorEditor:public juce::AudioProcessorEditor,private juce::Timer{
public:explicit TB3030AudioProcessorEditor(TB3030AudioProcessor&);void paint(juce::Graphics&)override;void resized()override;
private:void timerCallback()override;void refreshSteps();TB3030AudioProcessor&p;std::array<juce::Slider,11>knobs;std::array<juce::TextButton,16>on,acc,slide,up,down;std::array<juce::ComboBox,16>notes;juce::TextButton run{"RUN"},clear{"CLEAR"},random{"RANDOM"},save{"SAVE BANK"},load{"LOAD BANK"},store{"STORE"};juce::ComboBox presets;juce::Label title;std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>>atts;JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TB3030AudioProcessorEditor)};
