// Offline test: renders every factory sound playing factory patterns -> WAV
#include "../Source/FactoryData.h"
#include <cstdio>
#include <cmath>
using namespace tb303;
static void writeWav(const char* fn, const std::vector<float>& d, int sr){
  FILE* f=fopen(fn,"wb"); int n=(int)d.size(); int bytes=n*2;
  auto w32=[&](uint32_t v){fwrite(&v,4,1,f);}; auto w16=[&](uint16_t v){fwrite(&v,2,1,f);};
  fwrite("RIFF",1,4,f); w32(36+bytes); fwrite("WAVEfmt ",1,8,f); w32(16); w16(1); w16(1); w32(sr); w32(sr*2); w16(2); w16(16);
  fwrite("data",1,4,f); w32(bytes);
  for(float x: d){ int v=(int)std::lround(std::max(-1.f,std::min(1.f,x))*32767); w16((uint16_t)(int16_t)v);} fclose(f);}
int main(){
  const int sr=44100; const double bpm=128;
  SequenceData data; loadFactoryPatterns(data);
  Voice v; v.setSampleRate(sr); Sequencer seq;
  std::vector<float> all; float peakAll=0; int bad=0;
  const NamedSound* fs=factorySounds();
  for(int s=0;s<32;++s){
    v.setParams(fs[s].p); v.reset();
    seq.mode=Mode::PatternPlay; seq.groupKnob=0; seq.chain={ s%16 };
    seq.start();
    int len=(int)(sr*60.0/bpm*4*2); // 2 bars
    std::vector<float> buf(len,0.f);
    for(int i=0;i<len;i+=256){ int n=std::min(256,len-i); seq.process(data,v,buf.data()+i,n,bpm,sr,0.8f);}    
    seq.stop(v);
    float pk=0; double rms=0; for(float x:buf){ if(!std::isfinite(x)) bad++; pk=std::max(pk,std::fabs(x)); rms+=x*x;} rms=std::sqrt(rms/len);
    printf("%c-%d %-18s peak %.2f rms %.3f\n",'A'+s/8,s%8+1,fs[s].name,pk,rms);
    peakAll=std::max(peakAll,pk);
    all.insert(all.end(),buf.begin(),buf.end());
  }
  // track mode demo + triplet + MIDI-style legato
  v.setParams(fs[0].p); seq.mode=Mode::TrackPlay; seq.groupKnob=0; seq.start();
  int len=(int)(sr*60.0/bpm*4*8); std::vector<float> buf(len,0.f);
  for(int i=0;i<len;i+=512){ int n=std::min(512,len-i); seq.process(data,v,buf.data()+i,n,bpm,sr,0.8f);} seq.stop(v);
  all.insert(all.end(),buf.begin(),buf.end());
  printf("bad samples %d, max peak %.2f, total %.1fs\n",bad,peakAll,all.size()/(double)sr);
  for(auto& x: all) x*=0.85f/std::max(1.0f,peakAll);
  writeWav("demo.wav",all,sr);
}
