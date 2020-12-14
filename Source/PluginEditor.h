#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Space.h"

class Nel19AudioProcessorEditor :
    public juce::AudioProcessorEditor,
    public juce::Timer
{
public:
    Nel19AudioProcessorEditor (Nel19AudioProcessor&);
    void resized() override;
    void timerCallback() override;
private:
    Nel19AudioProcessor& audioProcessor;
    int fps, upscaleFactor;
    Space<double> space;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Nel19AudioProcessorEditor)
};
