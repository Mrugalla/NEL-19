#pragma once
#include "Approx.h"
#include "Interpolation.h"
#include "dsp/DryWetProcessor.h"
#include "dsp/MidSideEncoder.h"
#include "dsp/Modulator.h"
#include "dsp/Vibrato.h"
#include "dsp/Smooth.h"
#include "oversampling/Oversampling.h"
#include <JuceHeader.h>
#include "modsys/ModSys.h"
#include <limits>

struct Nel19AudioProcessor :
    public juce::AudioProcessor
{
    using Smooth = smooth::Smooth<float>;
    static constexpr int NumActiveMods = 2;

    Nel19AudioProcessor();
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    void savePatch();
    void loadPatch();
    juce::PropertiesFile::Options makeOptions();

    juce::ApplicationProperties appProperties;
    const int numChannels;

    drywet::Processor dryWet;

    modSys6::ModSys modSys;

    midSide::Processor midSideProcessor;
    oversampling::Processor oversampling;
    
    std::array<vibrato::Modulator, NumActiveMods> modulators;
    std::array<std::vector<float>, 2> modsBuffer;
    std::array<vibrato::ModType, NumActiveMods> modType;
    
    vibrato::Processor vibrat;
    
    std::vector<float> visualizerValues;
private:
    const juce::CriticalSection mutex;
    Smooth depthSmooth, modsMixSmooth;
    std::vector<float> depthBuf, modsMixBuf;

    void processBlockVibrato(juce::AudioBuffer<float>&, const juce::MidiBuffer&, int, int);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Nel19AudioProcessor)
};

/*

todo:

perlin
    procedural not smooth
better oversampler
    performance
    sound
remove unused interpolation types
    or fix them
wavetables
    envfol (limiter mit tanh statt nur smooth)
    more (like fm maybe?)
    bigger
    triangle: less random shit (dialing in octaves?)
modulation destinations
    +amp
    +filter
replace parameter range with withCentre
    dropout
        chance (10ms, 10sec, 420ms)
        decay (1ms, 10sec, 40ms)
    lfo
        rate (0hz, 40hz, 2hz)

debugger:

steinberg validator
C:\Users\Eine Alte Oma\Documents\CPP\vst3sdk\out\build\x64-Debug (default)\bin\validator.exe
-e "C:/Program Files/Common Files/VST3/NEL-19.vst3"

DAWS Debug:
D:\Pogramme\Cubase 9.5\Cubase9.5.exe
D:\Pogramme\FL Studio 20\FL64.exe
D:\Pogramme\Studio One 5\Studio One.exe
D:\Pogramme\Reaper\reaper.exe

*/