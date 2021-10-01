#pragma once
#include "NELG.h"
#include <JuceHeader.h>
#include "comp/Component.h"
#include "PluginProcessor.h"
#include <array>

struct Nel19AudioProcessorEditor :
    public juce::AudioProcessorEditor,
    public juce::Timer
{
    enum ModsType { EnvFol, LFO, Rand, Perlin, Pitchbend, Note, NumMods };

    Nel19AudioProcessorEditor(Nel19AudioProcessor&);
private:
    Nel19AudioProcessor& audioProcessor;
    Utils utils;
    nelG::Layout layout, layoutMacros, layoutMainParams, layoutBottomBar, layoutMiscs, layoutTopBar;

    BuildDateComp buildDate;
    TooltipComp tooltips;
    pComp::Knob macro0, macro1, macro2, macro3;
    pComp::Knob modulatorsMix, depth, dryWetMix;
    pComp::Switch midSideSwitch;
    std::vector<pComp::Parameter*> modulatables;
    pComp::ModDragger macroDragger0, macroDragger1, macroDragger2, macroDragger3;
    PopUpComp popUp;
    std::array<std::array<std::unique_ptr<ModulatorComp>, ModsType::NumMods>, 2> modulatorComps;

    pxl::ImgComp shuttle;

    std::unique_ptr<menu2::Menu> menu;
    menu2::Button menuButton;

    void resized() override;
    void paint(juce::Graphics&) override;
    void mouseEnter(const juce::MouseEvent& evt) override;
    void timerCallback() override;

    void resetModulatorComp(int, int);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Nel19AudioProcessorEditor)
        //JUCE_IS_REPAINT_DEBUGGING_ACTIVE
};
/*

mod selector to do:

modSelector2 options shall show symbols
    on hover behaviour (animated)
modSelector2 options setVisible on drag
    but unique_ptr would be smoother

*/