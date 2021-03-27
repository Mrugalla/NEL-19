#pragma once
#include "Shuttle.h"
#include "SettingsEditor.h"
#include "TooltipComponent.h"
#include <JuceHeader.h>

struct TitleEditor :
    public NELComp
{
    TitleEditor(Nel19AudioProcessor& p, NELGUtil& u) :
        NELComp(p, u),
        shuttle(p, u),
        tooltip(p, u),
        titleArea(),
        paramAreaX(0), paramAreaHeight(0),
        shuttleX(6)
    {
        addAndMakeVisible(shuttle);
        addAndMakeVisible(tooltip);
    }
    void setArea(float x, float y) {
        paramAreaX = x;
        paramAreaHeight = y;
    }

    Shuttle shuttle;
    TooltipComponent tooltip;
    juce::Path titleArea;
    float paramAreaX, paramAreaHeight;
    int shuttleX;
private:
    void paint(juce::Graphics& g) override {
        g.setColour(juce::Colour(nelG::ColBlack));
        g.fillPath(titleArea);
        drawBuildDate(g);
    }
    void resized() {
        const auto width = static_cast<float>(getWidth());
        const auto height = static_cast<float>(getHeight());
        titleArea.clear();
        auto x = 0.f, y = 0.f;
        titleArea.startNewSubPath(x, y);
        x = paramAreaX;
        titleArea.lineTo(x, y);
        auto destHeight = paramAreaHeight;
        auto inc = 8.f / destHeight;
        while (x < getWidth()) {
            ++x;
            y += inc * (destHeight - y);
            titleArea.lineTo(x, y);
        }
        titleArea.lineTo(x, height);
        titleArea.lineTo(0, height);
        titleArea.closeSubPath();

        shuttle.setBounds(shuttleX, 0, shuttle.img.getWidth(), getHeight());
        const auto tooltipHeight = static_cast<int>(destHeight);
        tooltip.setBounds(0, tooltipHeight, getWidth(), getHeight() - tooltipHeight);
    }
    void drawBuildDate(juce::Graphics& g) {
        const auto buildDate = juce::String(__DATE__);
        g.setColour(juce::Colour(0x44ffffff));
        g.setFont(utils.font);
        g.drawFittedText("Build: " + buildDate, getLocalBounds(), juce::Justification::bottomRight, 1, 0);
    }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TitleEditor)
};
