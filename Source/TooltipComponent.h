#pragma once
#include <JuceHeader.h>

struct TooltipComponent :
    public NELComp,
    public juce::Timer
{
    TooltipComponent(Nel19AudioProcessor& p, NELGUtil& u) :
        NELComp(p, u, "Lmao, I'm the tooltip bar.")
    {
        startTimerHz(nelG::FPS);
        ttFont = utils.font;
        ttFont.setHeight(11);
    }
private:
    juce::Font ttFont;
    void paint(juce::Graphics& g) override {
#if DebugLayout
        g.setColour(juce::Colours::red);
        g.drawRect(getLocalBounds());
#endif
        drawBuildDate(g);
        if (!utils.tooltipEnabled || utils.tooltip == nullptr) return;
        const auto bounds = getLocalBounds().toFloat().reduced(nelG::Thicc);
        g.setColour(juce::Colour(nelG::ColBeige));
        g.setFont(ttFont);
        g.drawFittedText(*utils.tooltip, bounds.toNearestInt(), juce::Justification::left, 1);
    }
    void drawBuildDate(juce::Graphics& g) {
        const auto buildDate = juce::String(__DATE__);
        g.setColour(juce::Colour(0x33ffffff));
        auto font = utils.font;
        font.setHeight(10);
        g.setFont(font);
        g.drawFittedText("Build: " + buildDate, getLocalBounds(), juce::Justification::centredRight, 1, 0);
    }
    void timerCallback() override { repaint(); }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TooltipComponent)
};
