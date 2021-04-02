#include "PluginProcessor.h"
#include "PluginEditor.h"

Nel19AudioProcessorEditor::Nel19AudioProcessorEditor(Nel19AudioProcessor& p) :
    AudioProcessorEditor(&p),
    audioProcessor(p),
    utils(p),
    shuttle(p, utils),
    
    depthMaxK(p, nelDSP::param::ID::DepthMax, utils, "Higher DepthMax values sacrifice latency for a stronger vibrato."),
    lrmsK(p, nelDSP::param::ID::LRMS, utils, "L/R for traditional stereo width, M/S for mono compatibility."),
    depthK(p, nelDSP::param::ID::Depth, utils, "Defines the depth of the vibrato."),
    freqK(p, nelDSP::param::ID::Freq, utils, "The frequency in which random values are picked for the LFO."),
    widthK(p, nelDSP::param::ID::Width, utils, "The offset of each channel's Random LFO."),
    mixK(p, nelDSP::param::ID::Mix, utils, "Turn Mix down for Chorus/Flanger-Sounds."),
    shapeK(p, nelDSP::param::ID::Shape, utils, "Defines the shape of the randomizer."),

    wavetable(p, utils),
    settings(p, utils),
    settingsButton(p, settings, utils),
    randomizerButton(p, utils),
    tooltip(p, utils)
{
    setMouseCursor(utils.cursors[NELGUtil::Cursor::Norm]);

    addAndMakeVisible(shuttle);
    
    addAndMakeVisible(depthK);
    addAndMakeVisible(freqK);
    addAndMakeVisible(shapeK);
    addAndMakeVisible(widthK);
    addAndMakeVisible(mixK);
    addAndMakeVisible(lrmsK);
    addAndMakeVisible(depthMaxK);

    addAndMakeVisible(wavetable);
    addAndMakeVisible(randomizerButton);
    addAndMakeVisible(settings); settings.setVisible(false);
    addAndMakeVisible(settingsButton);
    addAndMakeVisible(tooltip);

    setSize(
        static_cast<int>(nelG::Width * nelG::Scale),
        static_cast<int>(nelG::Height * nelG::Scale)
    );
}
void Nel19AudioProcessorEditor::resized() {
    const auto bounds = getLocalBounds().toFloat().reduced(nelG::Thicc);
    const auto width = bounds.getWidth();
    const auto height = bounds.getHeight();
    nelG::Ratio ratioX({ 7, 25, 15, 15, 5, 10, 10 }, width, nelG::Thicc);
    nelG::Ratio ratioY({ 8, 8, 20, 8, 17, 55, 8 }, height, nelG::Thicc);
    nelG::RatioBounds ratioBounds(ratioX, ratioY);

    settings.setBounds(getLocalBounds());
    settingsButton.setBounds(ratioBounds(0, 0));
    randomizerButton.setBounds(ratioBounds(0, 1));
    tooltip.setBounds(ratioBounds(0, 6, 6, 6));
    shuttle.setBounds(ratioBounds(0, 0, 1, 4));

    depthK.setQBounds(
        ratioBounds(2, 0),
        ratioBounds(2, 1, 2, 2),
        ratioBounds(2, 3)
    );
    depthMaxK.init(
        ratioBounds(2, 4),
        nelG::Tau * 0,
        nelG::Tau * 1,
        nelG::PiQuart * 0.f
    );
    freqK.setQBounds(
        ratioBounds(3, 0),
        ratioBounds(3, 1, 3, 2),
        ratioBounds(3, 3)
    );
    shapeK.setBounds(
        ratioBounds(4, 1, 4, 3)
    );
    widthK.setQBounds(
        ratioBounds(5, 0),
        ratioBounds(5, 1, 5, 2),
        ratioBounds(5, 3)
    );
    lrmsK.setBounds(
        ratioBounds(5, 4)
    );
    mixK.setQBounds(
        ratioBounds(6, 0),
        ratioBounds(6, 1, 6, 2),
        ratioBounds(6, 3)
    );

    /*
    const auto width = static_cast<float>(getWidth());
    const auto height = static_cast<float>(getHeight());
    const auto heightHalf = height * .5f;

    const auto tooltipHeight = utils.font.getHeight();
    tooltip.setBounds(0, getHeight() - tooltipHeight, getWidth(), tooltipHeight);

    const auto spaceX = width * .15f;
    const auto spaceY = 0.f;
    const auto spaceHeight = heightHalf - tooltipHeight;
    const auto spaceWidth = width - spaceX;
    shuttle.setBounds(spaceX, 0, shuttle.img.getWidth(), spaceHeight);
    const auto shuttleWidth = shuttle.getWidth();
    params.setQBounds({
        shuttleWidth + shuttle.getX(),
        0,
        getWidth() - shuttleWidth - shuttle.getX(),
        (int)spaceHeight
    });
    wavetable.setBounds(juce::Rectangle<float>(0, heightHalf - tooltipHeight, width, heightHalf).toNearestInt());

    // left bar of buttons
    const auto buttonX = 4.f * nelG::Scale;
    const auto buttonY = buttonX;
    const auto buttonWidth = 30.f * nelG::Scale;
    const auto buttonHeight = buttonWidth;
    settingsButton.setBounds(juce::Rectangle<float>(
        buttonX, buttonY, buttonWidth, buttonHeight).toNearestInt()
    );
    juce::Rectangle<float> settingsBounds(0.f, 0.f,
        width,
        height * .925f
    );
    settings.setBounds(settingsBounds.toNearestInt());
    juce::Rectangle<float> randButtonBounds(
        buttonX,
        static_cast<float>(settingsButton.getBottom()) + buttonY,
        buttonWidth, buttonHeight
    );
    randomizerButton.setBounds(randButtonBounds.toNearestInt());
    */
}

void Nel19AudioProcessorEditor::paint(juce::Graphics& g) { g.fillAll(juce::Colour(nelG::ColBlack)); }