#include "../Source/FactoryData.h"
#include <cstdio>
using namespace tb303;
int main(){
  SequenceData d; loadFactoryPatterns(d); Voice v; v.setSampleRate(48000); v.setParams(factorySounds()[1].p);
  Sequencer s; std::vector<float> b(480);
  int bad=0;
  // triplet pattern, pattern write mode, group II idx 10
  s.mode=Mode::PatternWrite; s.groupKnob=2; s.editPattern=10; s.start();
  for(int i=0;i<400;i++){ std::fill(b.begin(),b.end(),0.f); s.process(d,v,b.data(),480,140,48000,0.8f); for(float x:b) if(!std::isfinite(x)||std::fabs(x)>2) bad++; }
  printf("triplet spb=%.0f step=%d\n", s.stepsPerBeatFor(d), s.displayStep);
  // host sync mid-bar, chain A+B with shuffle
  s.mode=Mode::PatternPlay; s.groupKnob=0; s.chain={2,10}; s.shuffle=0.5f; s.syncTo(37.5);
  for(int i=0;i<400;i++){ s.process(d,v,b.data(),480,128,48000,0.8f); for(float x:b) if(!std::isfinite(x)) bad++; }
  printf("chain pattern=%d step=%d\n", s.displayPattern, s.displayStep);
  // queued change, empty track
  s.queuedChain={5}; s.hasQueued=true;
  for(int i=0;i<400;i++) s.process(d,v,b.data(),480,128,48000,0.8f);
  printf("after queue pattern=%d\n", s.displayPattern);
  s.mode=Mode::TrackPlay; s.groupKnob=6; for(int i=0;i<100;i++) s.process(d,v,b.data(),480,128,48000,0.8f);
  s.stop(v);
  // extreme params
  SoundParams x; x.resonance=1; x.cutoff=1; x.envMod=1; x.accent=1; x.drive=1; x.tuning=12; v.setParams(x);
  s.mode=Mode::PatternPlay; s.chain={0}; s.start(); for(int i=0;i<400;i++){ std::fill(b.begin(),b.end(),0.f); s.process(d,v,b.data(),480,300,48000,1.f); for(float y:b) if(!std::isfinite(y)||std::fabs(y)>2) bad++; }
  printf("bad=%d\n",bad);
}
