#include "PluginProcessor.h"
#include "PluginEditor.h"

Nel19AudioProcessorEditor::Nel19AudioProcessorEditor(Nel19AudioProcessor& p) :
    AudioProcessorEditor(&p),
    audioProcessor(p),
    fps(12),
    upscaleFactor(4),
    space(p, fps, upscaleFactor)
{
    setOpaque(true);
    addAndMakeVisible(space);
    startTimerHz(fps);
    setSize(128 * upscaleFactor, 128 * upscaleFactor);
}

void Nel19AudioProcessorEditor::resized() { space.setBounds(getLocalBounds()); }
void Nel19AudioProcessorEditor::timerCallback() { space.update(); }