#pragma once
#include <JuceHeader.h>
class AcidEngine{
public:void prepare(double,int);void reset();void render(juce::AudioBuffer<float>&,const juce::MidiBuffer&,float wave,float tune,float cutoff,float res,float envMod,float decay,float accent,float drive,float glide,float volume);
private:double sr=44100,phase=0;float hz=110,target=110,env=0,amp=0,accentEnv=0,lastVelocity=0;bool gate=false;juce::dsp::LadderFilter<float> filter;float osc(float wave);};
