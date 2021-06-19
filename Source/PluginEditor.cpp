#include "PluginProcessor.h"
#include "PluginEditor.h"

Nel19AudioProcessorEditor::Nel19AudioProcessorEditor(Nel19AudioProcessor& p) :
    AudioProcessorEditor(&p),
    audioProcessor(p),
    utils(p),
    layout(
        { 30, 120, 50, 50, 100 },
        { 30, 100, 100, 100, 100, 30 }
    ),
    layoutMacros(
        { 1 },
        { 25, 25, 25, 25 }
    ),
    layoutDepthMix(
        { 50, 120, 50 },
        { 50, 120, 50 }
    ),
    layoutBottomBar(
        { 200, 80 },
        { 1 }
    ),
    layoutParams(
        { 50, 50 },
        { 20, 80, 20 }
    )
{
    layout.addNamedLocation("top bar", 0, 0, 5, 1);
    layout.addNamedLocation("macros", 0, 1, 1, 4, false, false, false);
    layout.addNamedLocation("modulator 0", 1, 1, 2, 2);
    layout.addNamedLocation("modulator 1", 1, 3, 2, 2);
    layout.addNamedLocation("params", 3, 1, 2, 4, false, false, true);
    layout.addNamedLocation("depth&ModMix", 2, 2, 2, 2, false, false, false);
    layout.addNamedLocation("bottom bar", 0, 5, 5, 1, false, false, false);

    layoutMacros.addNamedLocation("macro 0", 0, 0, 1, 1, true, true);
    layoutMacros.addNamedLocation("macro 1", 0, 1, 1, 1, true, true);
    layoutMacros.addNamedLocation("macro 2", 0, 2, 1, 1, true, true);
    layoutMacros.addNamedLocation("macro 3", 0, 3, 1, 1, true, true);

    layoutDepthMix.addNamedLocation("depth", 1, 1, 1, 1, true, true);
    layoutDepthMix.addNamedLocation("mods mix", 0, 0, 3, 3, true, true);

    layoutBottomBar.addNamedLocation("tooltip", 0, 0, 1, 1);
    layoutBottomBar.addNamedLocation("build num", 1, 0, 1, 1);

    layoutParams.addNamedLocation("drywet mix", 0, 1, 1, 1, true, true);
    layoutParams.addNamedLocation("voices", 0, 2, 1, 1, true, false);
    layoutParams.addNamedLocation("depth max", 1, 2, 1, 1, true, false);

    setOpaque(true);
    setMouseCursor(utils.cursors[utils.Cursor::Norm]);

    setResizable(true, true);
    setSize(nelG::Width, nelG::Height);
}
void Nel19AudioProcessorEditor::resized() {
    layout.setBounds(getLocalBounds().toFloat().reduced(8.f));
    layoutMacros.setBounds(layout(0, 1, 1, 4));
    layoutDepthMix.setBounds(layout(2, 2, 2, 2));
    layoutBottomBar.setBounds(layout.bottomBar());
    layoutParams.setBounds(layout(4, 1, 1, 4));
}
void Nel19AudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(nelG::ColBlack));
    layout.paintGrid(g);
    layoutMacros.paintGrid(g);
    layoutDepthMix.paintGrid(g);
    layoutBottomBar.paintGrid(g);
    layoutParams.paintGrid(g);
}
void Nel19AudioProcessorEditor::mouseEnter(const juce::MouseEvent&) { utils.tooltip = nullptr; }
void Nel19AudioProcessorEditor::mouseMove(const juce::MouseEvent& evt) {
    layout.mouseMove(evt.position);
    layoutMacros.mouseMove(evt.position);
    layoutDepthMix.mouseMove(evt.position);
    layoutBottomBar.mouseMove(evt.position);
    layoutParams.mouseMove(evt.position);
    repaint();
}