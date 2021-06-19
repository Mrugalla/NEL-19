#include "PluginProcessor.h"
#include "PluginEditor.h"

Nel19AudioProcessorEditor::Nel19AudioProcessorEditor(Nel19AudioProcessor& p) :
    AudioProcessorEditor(&p),
    audioProcessor(p),
    utils(p),
    layout(
        { 5, 30, 120, 25, 25, 100, 5 },
        { 8, 20, 100, 35, 35, 100, 20, 8 }
    ),
    layoutMacros(
        { 1 },
        { 25, 25, 25, 25 }
    )
{
    layout.addNamedLocation("top bar", 1, 1, 5, 1);
    layout.addNamedLocation("macros", 1, 2, 1, 4);
    layout.addNamedLocation("modulator 0", 2, 2, 2, 2);
    layout.addNamedLocation("modulator 1", 2, 4, 2, 2);
    layout.addNamedLocation("params", 4, 2, 2, 4);
    layout.addNamedLocation("depth", 3, 3, 2, 2, true, true);
    layout.addNamedLocation("bottom bar", 1, 6, 5, 1);

    layoutMacros.addNamedLocation("macro 0", 0, 0, 1, 1, true, true);
    layoutMacros.addNamedLocation("macro 1", 0, 1, 1, 1, true, true);
    layoutMacros.addNamedLocation("macro 2", 0, 2, 1, 1, true, true);
    layoutMacros.addNamedLocation("macro 3", 0, 3, 1, 1, true, true);

    setOpaque(true);
    setMouseCursor(utils.cursors[utils.Cursor::Norm]);

    setResizable(true, true);
    setSize(nelG::Width, nelG::Height);
}
void Nel19AudioProcessorEditor::resized() {
    layout.setBounds(getLocalBounds().toFloat());
    layoutMacros.setBounds(layout(1, 2, 1, 4));
}
void Nel19AudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(nelG::ColBlack));
    layout.paintGrid(g);
    layoutMacros.paintGrid(g);
}
void Nel19AudioProcessorEditor::mouseEnter(const juce::MouseEvent&) { utils.tooltip = nullptr; }
void Nel19AudioProcessorEditor::mouseMove(const juce::MouseEvent& evt) {
    layout.mouseMove(evt.position);
    layoutMacros.mouseMove(evt.position);
    repaint();
}