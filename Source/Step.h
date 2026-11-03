#pragma once
#include <JuceHeader.h>
struct AcidStep {
 int note=36; bool enabled=false, accent=false, slide=false, octaveUp=false, octaveDown=false; float gate=.75f;
 juce::ValueTree toTree(int i) const { juce::ValueTree v("STEP"); v.setProperty("i",i,nullptr);v.setProperty("note",note,nullptr);v.setProperty("on",enabled,nullptr);v.setProperty("accent",accent,nullptr);v.setProperty("slide",slide,nullptr);v.setProperty("up",octaveUp,nullptr);v.setProperty("down",octaveDown,nullptr);v.setProperty("gate",gate,nullptr);return v; }
 void fromTree(const juce::ValueTree&v){note=(int)v["note"];enabled=(bool)v["on"];accent=(bool)v["accent"];slide=(bool)v["slide"];octaveUp=(bool)v["up"];octaveDown=(bool)v["down"];gate=(float)v.getProperty("gate",.75f);}
 int effectiveNote()const{return juce::jlimit(12,108,note+(octaveUp?12:0)-(octaveDown?12:0));}
};
