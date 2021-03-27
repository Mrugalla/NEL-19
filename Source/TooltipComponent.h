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
        if (!utils.tooltipEnabled || utils.tooltip == nullptr) return;
        const auto bounds = getLocalBounds().toFloat().reduced(nelG::Thicc);
        g.setColour(juce::Colour(nelG::ColBeige));
        g.setFont(ttFont);
        g.drawFittedText(*utils.tooltip, bounds.toNearestInt(), juce::Justification::left, 1);
    }
    void timerCallback() override { repaint(); }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TooltipComponent)
};
