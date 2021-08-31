#pragma once
#include "NELG.h"
#include "Component.h"
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>

struct Nel19AudioProcessorEditor :
    public juce::AudioProcessorEditor,
    public juce::Timer
{
    Nel19AudioProcessorEditor(Nel19AudioProcessor&);
private:
    Nel19AudioProcessor& audioProcessor;
    Utils utils;
    nelG::Layout layout, layoutMacros, layoutDepthMix, layoutBottomBar, layoutParams, layoutTopBar;

    BuildDateComponent buildDate;
    TooltipComponent tooltips;
    Knob macro0, macro1, macro2, macro3;
    ModulatableKnob modulatorsMix, depth, dryWetMix;
    std::vector<Modulatable*> modulatables;
    ModDragger macroDragger0, macroDragger1, macroDragger2, macroDragger3;
    std::vector<std::unique_ptr<DropDownMenu>> modSelector;
    PopUpComponent popUp;
    std::array<std::array<std::unique_ptr<ModulatorComp>, 4>, 2> modulatorComps;
    VisualizerComp visualizer;
    
    void resized() override;
    void paint(juce::Graphics&) override;
    void mouseEnter(const juce::MouseEvent& evt) override;
    void mouseMove(const juce::MouseEvent& evt) override;
    void timerCallback() override;

    void resetModulatorComponent(int, int);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Nel19AudioProcessorEditor)
};

/*
mouseMove repaints whole window instead of selected part
*/