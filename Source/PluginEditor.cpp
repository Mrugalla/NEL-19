#include "PluginProcessor.h"
#include "PluginEditor.h"

Nel19AudioProcessorEditor::Nel19AudioProcessorEditor(Nel19AudioProcessor& p) :
    AudioProcessorEditor(&p),
    audioProcessor(p),
    nelGUtil(p),
    space(p, nelGUtil),
    title(p, nelGUtil),
    params(p, nelGUtil),
    settings(p, nelGUtil),
    settingsButton(p, settings, nelGUtil),
    randomizerButton(p, nelGUtil)
{
    addAndMakeVisible(space);
    addAndMakeVisible(title);
    addAndMakeVisible(params);
    addAndMakeVisible(randomizerButton);
    addAndMakeVisible(settings); settings.setVisible(false);
    addAndMakeVisible(settingsButton);

    space.setMouseCursor(nelGUtil.cursors[NELGUtil::Cursor::Norm]);

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
}