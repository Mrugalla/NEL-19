#pragma once
#include "NELG.h"
#include <JuceHeader.h>
#include "Component.h"
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

    BuildDateComp buildDate;
    TooltipComp tooltips;
    Knob macro0, macro1, macro2, macro3;
    ModulatableKnob modulatorsMix, depth, dryWetMix;
    Switch midSideSwitch;
    std::vector<Modulatable*> modulatables;
    ModDragger macroDragger0, macroDragger1, macroDragger2, macroDragger3;
    PopUpComp popUp;
    std::array<std::array<std::unique_ptr<ModulatorComp>, 4>, 2> modulatorComps;
    VisualizerComp visualizer;

    std::unique_ptr<menu2::Menu> menu;
    menu2::Button menuButton;
    
    void resized() override;
    void paint(juce::Graphics&) override;
    void mouseEnter(const juce::MouseEvent& evt) override;
    void mouseMove(const juce::MouseEvent& evt) override;
    void timerCallback() override;

    void resetModulatorComp(int, int);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Nel19AudioProcessorEditor)
};

/*

mod selector to do:

modSelector2 options shall show symbols
    on hover behaviour (animated)
modSelector2 options setVisible on drag
    but unique_ptr would be smoother

*/