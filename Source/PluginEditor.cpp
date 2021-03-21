#include "PluginProcessor.h"
#include "PluginEditor.h"

Nel19AudioProcessorEditor::Nel19AudioProcessorEditor(Nel19AudioProcessor& p) :
    AudioProcessorEditor(&p),
    audioProcessor(p),
    nelGUtil(),
    space(),
    title(p, nelGUtil),
    params(p, nelGUtil)
{
    addAndMakeVisible(space);
    addAndMakeVisible(title);
    addAndMakeVisible(params);

    space.setMouseCursor(nelGUtil.cursors[NELGUtil::Cursor::Norm]);

    startTimerHz(nelG::FPS);
    setSize(
        static_cast<int>(nelG::Width * nelG::Scale),
        static_cast<int>(nelG::Height * nelG::Scale)
    );
}
void Nel19AudioProcessorEditor::resized() {
    const auto width = static_cast<float>(getWidth());
    const auto height = static_cast<float>(getHeight());

    const auto spaceX = width * .15f;
    const auto spaceY = 0.f;
    const auto spaceHeight = height * .92f;
    const auto spaceWidth = width - spaceX;
    space.setBounds(juce::Rectangle<float>(spaceX, spaceY, spaceWidth, spaceHeight).toNearestInt());
    title.setArea(spaceX, spaceHeight);
    title.setBounds(getLocalBounds());
    const auto shuttleWidth = title.shuttle.getWidth();
    params.setQBounds({
        shuttleWidth + title.shuttleX,
        0,
        getWidth() - shuttleWidth - title.shuttleX,
        space.getHeight()
    });
}
void Nel19AudioProcessorEditor::timerCallback() { title.update(); }

/*
to do:
vst logo
settingseditor
    link discord, github, juce, josh, paypal
    dank
*/