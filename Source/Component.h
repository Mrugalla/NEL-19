#pragma once
#include <JuceHeader.h>

struct Utils {
    enum Cursor { Norm, Hover, Cross };
    Utils(Nel19AudioProcessor& p) :
        cursors(),
        tooltipID("tooltip"),
        tooltip(nullptr),
        font(getCustomFont()),
        tooltipEnabled(true)
    {
        // CURSOR
        const auto scale = 3;
        const auto cursorImage = nelG::load(BinaryData::cursor_png, BinaryData::cursor_pngSize, scale);
        auto cursorHoverImage = cursorImage.createCopy();
        for (auto x = 0; x < cursorImage.getWidth(); ++x)
            for (auto y = 0; y < cursorImage.getHeight(); ++y)
                if (cursorImage.getPixelAt(x, y) == juce::Colour(nelG::ColGreen))
                    cursorHoverImage.setPixelAt(x, y, juce::Colour(nelG::ColYellow));
        const juce::MouseCursor cursor(cursorImage, 0, 0);
        juce::MouseCursor cursorHover(cursorHoverImage, 0, 0);
        cursors.push_back(cursor);
        cursors.push_back(cursorHover);
        const auto cursorCrossImage = nelG::load(BinaryData::cursorCross_png, BinaryData::cursorCross_pngSize, scale);
        const auto midPoint = 5 * scale;
        const juce::MouseCursor cursorCross(cursorCrossImage, midPoint, midPoint);
        cursors.push_back(cursorCross);

        // TOOL TIPS
        auto user = p.appProperties.getUserSettings();
        if (user->isValidFile()) {
            tooltipEnabled = user->getBoolValue(tooltipID, tooltipEnabled);
            user->setValue(tooltipID, tooltipEnabled);
        }
    }
    void switchToolTip(Nel19AudioProcessor& p) {
        auto user = p.appProperties.getUserSettings();
        if (user->isValidFile()) {
            tooltipEnabled = !user->getBoolValue(tooltipID, tooltipEnabled);
            user->setValue(tooltipID, tooltipEnabled);
        }
    }
    std::vector<juce::MouseCursor> cursors;
    juce::Identifier tooltipID;
    juce::String* tooltip;
    juce::Font font;
    bool tooltipEnabled;
private:
    juce::Font getCustomFont() {
        return juce::Font(juce::Typeface::createSystemTypefaceFor(BinaryData::nel19_ttf,
            BinaryData::nel19_ttfSize));
    }
};

struct Component :
    public juce::Component
{
    Component(Nel19AudioProcessor& p, Utils& u, const juce::String& tooltp = "") :
        processor(p),
        utils(u),
        tooltip(tooltp)
    { setMouseCursor(utils.cursors[utils.Cursor::Norm]); }
    Nel19AudioProcessor& processor;
    Utils& utils;
    juce::String tooltip;

    void mouseMove(const juce::MouseEvent&) override { utils.tooltip = &tooltip; }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Component)
};
