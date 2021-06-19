#pragma once
#include "NELG.h"
#include "Component.h"
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>

struct Nel19AudioProcessorEditor :
    public juce::AudioProcessorEditor
{
    Nel19AudioProcessorEditor(Nel19AudioProcessor&);
private:
    Nel19AudioProcessor& audioProcessor;
    Utils utils;
    nelG::Layout layout, layoutMacros;

    void resized() override;
    void paint(juce::Graphics&) override;
    void mouseEnter(const juce::MouseEvent& evt) override;
    void mouseMove(const juce::MouseEvent& evt) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Nel19AudioProcessorEditor)
};