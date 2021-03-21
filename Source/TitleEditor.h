#pragma once
#include "Shuttle.h"
#include <JuceHeader.h>

struct TitleEditor :
    public juce::Component
{
    TitleEditor(Nel19AudioProcessor& p, const NELGUtil& u) :
        utils(u),
        processor(p),
        shuttle(p),
        titleArea(),
        paramAreaX(0), paramAreaHeight(0),
        shuttleX(6)
    {
        addAndMakeVisible(shuttle);
        
        shuttle.setMouseCursor(u.cursors[NELGUtil::Cursor::Norm]);
    }
    
    void drawBuildDate(juce::Graphics& g) {
        const auto buildDate = juce::String(__DATE__);
        g.setColour(juce::Colour(0x44ffffff));
        g.drawFittedText("Build: " + buildDate, getLocalBounds(), juce::Justification::bottomRight, 1, 0);
    }
   
    void setArea(float x, float y) {
        paramAreaX = x;
        paramAreaHeight = y;
    }
    void update() {
        shuttle.update();
    }

    const NELGUtil& utils;
    Nel19AudioProcessor& processor;
    Shuttle shuttle;
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
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TitleEditor)
};
