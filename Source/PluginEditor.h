#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>
#define DebugRatioBounds false

#include "NELG.h"
#include "Shuttle.h"
#include "SettingsEditor.h"
#include "RandomizerButton.h"
#include "TooltipComponent.h"
#include "ParametersEditor.h"
#include "SplineEditor.h"

struct Nel19AudioProcessorEditor :
    public juce::AudioProcessorEditor
{
    Nel19AudioProcessorEditor(Nel19AudioProcessor&);
private:
    Nel19AudioProcessor& audioProcessor;
    nelG::Utils utils;
    nelG::RatioBounds2 rb2;
    Shuttle shuttle;

    QuickAccessWheel depthMaxK;
    ButtonSwitch lrmsK;
    Knob depthK, freqK, widthK, mixK, shapeK;

    //spline::Editor splineEditor;
    //spline::PresetMenu splinePresetMenu;
    //spline::PresetButton splinePresetButton;
    Settings settings;
    SettingsButton settingsButton;
    RandomizerButton randomizerButton;
    TooltipComponent tooltip;
    int numChannels;

    void resized() override;
    void paint(juce::Graphics&) override;
    void mouseEnter(const juce::MouseEvent& evt) override;
    void updateChannelCount(int ch);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Nel19AudioProcessorEditor)
};