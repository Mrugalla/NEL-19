#pragma once
#include "Approx.h"
#include "Outtakes.h"
#include "Param.h"
#include "Interpolation.h"
#include "dsp/MidSideEncoder.h"
#include "dsp/Vibrato.h"
#include "oversampling/Oversampling.h"
#include <JuceHeader.h>
#include "releasePool/ReleasePool.h"
#include "modsys/ModSystem.h"

#include <limits>

struct Nel19AudioProcessor :
    public juce::AudioProcessor
{
    enum Mods { EnvFol, LFO, Rand, Perlin };
    enum PID { Depth, ModsMix, StereoConfig, EnumSize };

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
    juce::PropertiesFile::Options makeOptions();

    juce::ApplicationProperties appProperties;
    param::MultiRange modRateRanges;

    // midi modulator stuff
    std::array<std::atomic<bool>, 2> midiLearn;
    std::array<float, 2> midiSignal;

    juce::AudioProcessorValueTreeState apvts;

    oversampling::Processor oversampling;

    ThreadSafePtr<modSys2::Matrix> matrix;
    std::vector<int> mtrxParams;

    std::array<std::vector<juce::Identifier>, 2> modsIDs;
    juce::Identifier modulatorsID;

    midSide::Processor midSideProcessor;
    std::array<juce::AudioBuffer<float>, 2> vibDelay;
    vibrato::Processor vibrato;
    std::vector<juce::Atomic<float>> vibDelayVisualizerValue;

private:
    const juce::CriticalSection mutex;

    bool processBlockReady(juce::AudioBuffer<float>&);
    const std::shared_ptr<modSys2::Matrix> processBlockModSys(juce::AudioBuffer<float>&, const juce::MidiBuffer&);
    void processBlockVibDelay(juce::AudioBuffer<float>&, const std::shared_ptr<modSys2::Matrix>&);
    void processBlockEmpty();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Nel19AudioProcessor)
};

/*

oversampling crunshes cpu on dbg. 4% on rls.
    replace import juceheader with smaller modules everywhere
    actually not oversample modsys if possible

debugger:

steinberg validator
C:\Users\Eine Alte Oma\Documents\CPP\vst3sdk\out\build\x64-Debug (default)\bin\validator.exe
-e "C:/Program Files/Common Files/VST3/NEL-19.vst3"

DAWS Debug:
D:\Pogramme\Cubase 9.5\Cubase9.5.exe
D:\Pogramme\FL Studio 20\FL64.exe
D:\Pogramme\Studio One 5\Studio One.exe

*/