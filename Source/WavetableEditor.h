#pragma once
#include <JuceHeader.h>

struct SplineEditor :
    public NELComp {
    SplineEditor(Nel19AudioProcessor& p, NELGUtil& u) :
        NELComp(p, u, "The Spline Editor hasn't been implemented yet")
    {
        setMouseCursor(u.cursors[NELGUtil::Cursor::Hover]);
    }
private:
    void paint(juce::Graphics& g) override {
#if DebugLayout
        g.setColour(juce::Colours::red);
        g.drawRect(getLocalBounds());
#endif
    }

    void resized() override {
        const auto width = static_cast<float>(getWidth());
        const auto height = static_cast<float>(getHeight());
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SplineEditor)
};

struct WavetableEditor :
    public NELComp {
    WavetableEditor(Nel19AudioProcessor& p, NELGUtil& u) :
        NELComp(p, u),
        splineEditor(p, u)
    {
        addAndMakeVisible(splineEditor);
    }
private:
    SplineEditor splineEditor;

    void paint(juce::Graphics& g) override {
#if DebugLayout
        g.setColour(juce::Colours::red);
        g.drawRect(getLocalBounds());
#endif
    }

    void resized() override {
        const auto width = static_cast<float>(getWidth());
        const auto height = static_cast<float>(getHeight());
        nelG::Ratio ratioX({ 4,12,8,8,18,6 }, width);
        nelG::Ratio ratioY({ 2,10,2 }, height);
        nelG::RatioBounds ratioBounds(ratioX, ratioY);

        splineEditor.setBounds(ratioBounds(2, 1, 4, 1));
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WavetableEditor)
};
