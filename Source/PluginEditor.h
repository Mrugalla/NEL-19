#pragma once

#define DebugMenuExists true
#define PresetsExist true

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "modsys/ModSysGUI.h"
#include "dsp/ModsGUI.h"

#if DebugMenuExists
#include "Menu.h"
#endif

struct Nel19AudioProcessorEditor :
    public juce::AudioProcessorEditor,
    public juce::Timer
{
    static constexpr int MinEditorBounds = 120;
    
    Nel19AudioProcessorEditor(Nel19AudioProcessor&);

    void resized() override;
    void paint(juce::Graphics&) override;
    void mouseEnter(const juce::MouseEvent&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void timerCallback() override;

protected:
    Nel19AudioProcessor& audioProcessor;
    
    gui::Layout layout, layoutMacros, layoutMainParams, layoutBottomBar, layoutTopBar;
    
    gui::Utils utils;
    gui::Events::Evt notify;

    gui::Label nelLabel;

    gui::Tooltip tooltips;
    gui::BuildDate buildDate;

    std::vector<gui::Paramtr*> modulatables;
    
    gui::Paramtr macro0, macro1, macro2, macro3;
    std::array<gui::ModComp, 2> modComps;

    gui::Visualizer visualizer;

    gui::Paramtr bufferSizes, modsDepth, modsMix, dryWetMix, gainWet, stereoConfig, feedback, damp;

    gui::ModDragger macro0Dragger, macro1Dragger, macro2Dragger, macro3Dragger;

    gui::ParamtrRandomizer paramRandomizer;
    gui::Paramtr hq, lookahead;

    gui::PopUp popUp;
    gui::EnterValueComp enterValue;
    
#if DebugMenuExists
    std::unique_ptr<menu2::Menu> menu;
    gui::Button menuButton;
#endif

#if PresetsExist
    gui::PresetBrowser presetBrowser;
#endif
};

/*

mod selector to do:

modSelector2 options shall show symbols
    on hover behaviour (animated)
modSelector2 options setVisible on drag
    but unique_ptr would be smoother

*/