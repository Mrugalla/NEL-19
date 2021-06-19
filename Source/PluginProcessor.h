#pragma once
#include "Param.h"
#include <JuceHeader.h>

class Nel19AudioProcessor :
    public juce::AudioProcessor,
    public juce::AsyncUpdater
{
public:
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
    void processParameters();
    void handleAsyncUpdate() override;

    juce::ApplicationProperties appProperties;

    param::MultiRange modRateRanges;
    juce::AudioProcessorValueTreeState apvts;
    std::vector<std::atomic<float>*> params;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Nel19AudioProcessor)
};
