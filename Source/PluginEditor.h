#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "modsys/ModSysGUI.h"
#include "dsp/ModsGUI.h"
#include "NELG.h"
#include "Menu.h"

class Nel19AudioProcessorEditor :
    public juce::AudioProcessorEditor
{
    static constexpr int MinEditorBounds = 120;
public:
    Nel19AudioProcessorEditor(Nel19AudioProcessor&);
private:
    Nel19AudioProcessor& audioProcessor;
    
    nelG::Layout layout, layoutMacros, layoutMainParams, layoutBottomBar, layoutTopBar;
    
    modSys6::gui::Utils utils;
    modSys6::gui::Events::Evt notify;

    modSys6::gui::Label nelLabel;

    modSys6::gui::Tooltip tooltips;
    modSys6::gui::BuildDate buildDate;

    std::vector<modSys6::gui::Paramtr*> modulatables;
    
    modSys6::gui::Paramtr macro0, macro1, macro2, macro3;
    std::array<modSys6::gui::ModComp, 2> modComps;

    modSys6::gui::Visualizer visualizer;

    modSys6::gui::Paramtr bufferSizes, modsDepth, modsMix, dryWetMix, gainWet, stereoConfig, feedback, damp;

    modSys6::gui::ModDragger macro0Dragger, macro1Dragger, macro2Dragger, macro3Dragger;

    modSys6::gui::ParamtrRandomizer paramRandomizer;
    modSys6::gui::Paramtr hq, lookahead;

    modSys6::gui::PopUp popUp;
    modSys6::gui::EnterValueComp enterValue;
    
    std::unique_ptr<menu2::Menu> menu;
    menu2::ButtonM menuButton;

    modSys6::gui::PresetBrowser presetBrowser;

    void resized() override;
    void paint(juce::Graphics&) override;
    void mouseEnter(const juce::MouseEvent&) override;
    void mouseDown(const juce::MouseEvent&) override;
};
/*

mod selector to do:

modSelector2 options shall show symbols
    on hover behaviour (animated)
modSelector2 options setVisible on drag
    but unique_ptr would be smoother

*/