#pragma once
#include <JuceHeader.h>
#include "Step.h"
class AcidSequencer {
public:
 std::array<AcidStep,16> steps; void prepare(double);void reset();void setRunning(bool);bool isRunning()const{return running;}
 void setTempo(double b){bpm=juce::jlimit(30.0,300.0,b);}void setSwing(float s){swing=juce::jlimit(0.f,.49f,s);}void setLength(int l){length=juce::jlimit(1,16,l);}int getLength()const{return length;}int getCurrentStep()const{return current;}
 void process(int,juce::MidiBuffer&);juce::ValueTree toTree()const;void fromTree(const juce::ValueTree&);void clear();void randomise(juce::Random&);
private:double sr=44100,bpm=120,countdown=0;float swing=0;int current=-1,length=16,active=-1;bool running=false;};
