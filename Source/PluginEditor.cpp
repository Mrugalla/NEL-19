#include "PluginProcessor.h"
#include "PluginEditor.h"

Nel19AudioProcessorEditor::Nel19AudioProcessorEditor(Nel19AudioProcessor& p) :
    AudioProcessorEditor(&p),
    audioProcessor(p),
    utils(p),
    rb2(
        { 0, 7, 30, 15, 15, 10, 10, 10 },
        { 0, 8, 8, 20, 8, 17, 6 }
    ),
    shuttle(p, utils),
    
    depthMaxK(p, param::ID::DepthMax, utils, "Higher values sacrifice latency for a stronger vibrato."),
    lrmsK(p, param::ID::LRMS, utils, "Seperate rand LFOs for L/R or M/S."),
    depthK(p, param::ID::Depth, utils, "The depth of the rand LFO."),
    freqK(p, param::ID::Freq, utils, "The frequency in which random values are picked for the LFO."),
    widthK(p, param::ID::Width, utils, "The offset of each channel's random LFO."),
    mixK(p, param::ID::Mix, utils, "Dry/Wet -> Mix."),
    shapeK(p, param::ID::Smooth, utils, "Higher values for a smoother rand LFO."),
    //splineMixK(p, param::ID::SplineMix, utils, "Mixes the spline with the rand LFO."),

    //splineEditor(p, utils),
    //splinePresetMenu(p, utils, splineEditor),
    //splinePresetButton(p, utils, "Load or save a spline preset.", splinePresetMenu),
    settings(p, utils),
    settingsButton(p, settings, utils),
    randomizerButton(p, utils),
    tooltip(p, utils),

    numChannels(audioProcessor.getChannelCountOfBus(true, 0))
{
    setOpaque(true);
    setMouseCursor(utils.cursors[nelG::Utils::Cursor::Norm]);

    addAndMakeVisible(shuttle);
    addAndMakeVisible(depthK);
    addAndMakeVisible(freqK);
    addAndMakeVisible(shapeK);
    addAndMakeVisible(widthK);
    addAndMakeVisible(mixK);
    addAndMakeVisible(lrmsK);
    //addAndMakeVisible(splineEditor);
   // addAndMakeVisible(splinePresetButton);
    addAndMakeVisible(randomizerButton);
    addAndMakeVisible(depthMaxK);
    addChildComponent(settings);
    addAndMakeVisible(settingsButton);
   // addChildComponent(splinePresetMenu);
    addAndMakeVisible(tooltip);

    const auto numInputChannels = audioProcessor.getChannelCountOfBus(true, 0);
    updateChannelCount(numInputChannels);

    setSize(
        static_cast<int>(nelG::Width * nelG::Scale),
        static_cast<int>(nelG::Height * nelG::Scale)
    );
}
void Nel19AudioProcessorEditor::resized() {
    const auto bounds = getLocalBounds().toFloat().reduced(util::Thicc);
    rb2.setBounds(bounds);

    settings.setBounds(rb2.exceptBottomBar().toNearestInt());
    settingsButton.setBounds(rb2(0, 0).toNearestInt());
    randomizerButton.setBounds(rb2(0, 1).toNearestInt());
    tooltip.setBounds(rb2.bottomBar().toNearestInt());
    shuttle.setBounds(rb2(0, 0, 2, 5).toNearestInt());
    depthK.setQBounds(
        rb2(2, 0).toNearestInt(),
        rb2(2, 1, 1, 2).toNearestInt(),
        rb2(2, 3).toNearestInt()
    );
    depthMaxK.init(
        rb2(2, 4).toNearestInt(),
        util::Tau * .75f,
        util::Tau * .5f,
        util::PiQuart * .5f
    );
    freqK.setQBounds(
        rb2(3, 0).toNearestInt(),
        rb2(3, 1, 1, 2).toNearestInt(),
        rb2(3, 3).toNearestInt()
    );
    shapeK.setQBounds(
        rb2(4, 0).toNearestInt(),
        rb2(4, 1, 1, 2).toNearestInt(),
        rb2(4, 3).toNearestInt()
    );
    widthK.setQBounds(
        rb2(5, 0).toNearestInt(),
        rb2(5, 1, 1, 2).toNearestInt(),
        rb2(5, 3).toNearestInt()
    );
    lrmsK.setBounds(rb2(5, 4).toNearestInt());
    mixK.setQBounds(
        rb2(6, 0).toNearestInt(),
        rb2(6, 1, 1, 2).toNearestInt(),
        rb2(6, 3).toNearestInt()
    );
    //splineEditor.setBounds(rb2(2,6,4,1).toNearestInt());
    //splinePresetMenu.setBounds(rb2.exceptBottomBar().toNearestInt());
    //splinePresetButton.setBounds(rb2(2, 5, 4, 1).toNearestInt());
}


void Nel19AudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(util::ColBlack));
#if DebugRatioBounds
    rb2.paintGrid(g);
#endif
}
void Nel19AudioProcessorEditor::mouseEnter(const juce::MouseEvent&) { utils.tooltip = nullptr; }
void Nel19AudioProcessorEditor::updateChannelCount(int numChannels) {
    bool enableWidth = numChannels > 1;
    widthK.setEnabled(enableWidth);
    lrmsK.setEnabled(enableWidth);
}