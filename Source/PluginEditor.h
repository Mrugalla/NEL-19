#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>

namespace nelG {
    static constexpr float Pi = 3.14159265359f;
    static constexpr float Tau = 6.28318530718f;
    static constexpr float PiHalf = Pi * .5f;
    static constexpr float PiQuart = Pi * .25f;
    static constexpr float PiInv = 1.f / Pi;

    static constexpr int Width = 512 + 32 * 4;
    static constexpr int Height = 256 - 8 * 3;
    static constexpr float Scale = 1;
    static constexpr int FPS = 12;

    static constexpr float Thicc = 3.f;
    static constexpr float Rounded = 4.f;

    static constexpr unsigned int ColBlack = 0xff171623;
    static constexpr unsigned int ColDarkGrey = 0xff595652;
    static constexpr unsigned int ColBeige = 0xff9c8980;
    static constexpr unsigned int ColGrey = 0xff696a6a;
    static constexpr unsigned int ColRed = 0xffac3232;
    static constexpr unsigned int ColGreen = 0xff37946e;
    static constexpr unsigned int ColYellow = 0xfffffa8f;
    static constexpr unsigned int ColGreenNeon = 0xff99c550;

    static juce::Image load(const void* d, int s) {
        auto img = juce::ImageCache::getFromMemory(d, s);

        auto b = img.getBounds();
        for(auto y = 0; y < img.getHeight(); ++y)
            for (auto x = 0; x < img.getWidth(); ++x)
                if (!img.getPixelAt(x, y).isTransparent()) {
                    b.setY(y);
                    break;
                }
        for (auto x = 0; x < img.getWidth(); ++x)
            for (auto y = 0; y < img.getHeight(); ++y)
                if (!img.getPixelAt(x, y).isTransparent()) {
                    b.setX(x);
                    break;
                }
        for (auto y = img.getHeight() - 1; y > -1; --y)
            for (auto x = img.getWidth() - 1; x > -1; --x)
                if (!img.getPixelAt(x, y).isTransparent()) {
                    b.setHeight(y - b.getY());
                    break;
                }
        for (auto x = img.getWidth() - 1; x > -1; --x)
            for (auto y = img.getHeight() - 1; y > -1; --y)
                if (!img.getPixelAt(x, y).isTransparent()) {
                    b.setWidth(x - b.getX());
                    break;
                }
        if (b.getX() < 0 || b.getY() < 0 || b.getWidth() < 0 || b.getHeight() < 0)
            return img.createCopy();
        return img.getClippedImage(b).createCopy();
    }
    static juce::Image load(const void* d, int s, int scale) {
        const auto img = load(d, s);
        return img.rescaled(img.getWidth() * scale, img.getHeight() * scale, juce::Graphics::lowResamplingQuality).createCopy();
    }
    static int widthRel(float x) { return static_cast<int>(x * Width); }
    static int heightRel(float x) { return static_cast<int>(x * Height); }
    static juce::Rectangle<int> boundsDownscaled() { return { 0, 0 , Width, Height }; }
}

struct NELGUtil {
    enum Cursor { Norm, Hover };
    NELGUtil(Nel19AudioProcessor& p) :
        cursors(),
        tooltipID("tooltip"),
        tooltip(nullptr),
        font(getCustomFont()),
        tooltipEnabled(true)
    {
        // CURSOR
        const auto cursorImage = nelG::load(BinaryData::cursor_png, BinaryData::cursor_pngSize, 3);
        auto cursorHoverImage = cursorImage.createCopy();
        for (auto x = 0; x < cursorImage.getWidth(); ++x)
            for (auto y = 0; y < cursorImage.getHeight(); ++y)
                if (cursorImage.getPixelAt(x, y) == juce::Colour(nelG::ColGreen))
                    cursorHoverImage.setPixelAt(x, y, juce::Colour(nelG::ColYellow));
        const juce::MouseCursor cursor(cursorImage, 0, 0);
        juce::MouseCursor cursorHover(cursorHoverImage, 0, 0);
        cursors.push_back(cursor);
        cursors.push_back(cursorHover);

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

struct NELComp :
    public juce::Component {

    NELComp(Nel19AudioProcessor& p, NELGUtil& u, juce::String tooltp = "") :
        processor(p),
        utils(u),
        tooltip(tooltp)
    { setMouseCursor(u.cursors[NELGUtil::Cursor::Norm]); }
    Nel19AudioProcessor& processor;
    NELGUtil& utils;
    juce::String tooltip;
    void mouseMove(const juce::MouseEvent&) override { utils.tooltip = &tooltip; }
};

#include "SpaceEditor.h"
#include "TitleEditor.h"
#include "ParametersEditor.h"

struct Nel19AudioProcessorEditor :
    public juce::AudioProcessorEditor
{
    Nel19AudioProcessorEditor (Nel19AudioProcessor&);
private:
    Nel19AudioProcessor& audioProcessor;
    NELGUtil nelGUtil;
    SpaceEditor space;
    TitleEditor title;
    ParametersEditor params;
    Settings settings;
    SettingsButton settingsButton;
    RandomizerButton randomizerButton;

    void resized() override;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Nel19AudioProcessorEditor)
};