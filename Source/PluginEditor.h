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

    static constexpr int Width = 512 + 32 * 3;
    static constexpr int Height = 256 - 16;
    static constexpr float Scale = 1;
    static constexpr int FPS = 12;

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

        juce::Rectangle<int> b;
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
            for (auto x = 0; x < img.getWidth(); ++x)
                if (!img.getPixelAt(x, y).isTransparent()) {
                    b.setHeight(y - b.getY());
                    break;
                }
        for (auto x = img.getWidth(); x > -1; --x)
            for (auto y = img.getHeight() - 1; y > -1; --y)
                if (!img.getPixelAt(x, y).isTransparent()) {
                    b.setWidth(x - b.getX());
                    break;
                }

        return img.getClippedImage(b).createCopy();
    }
    static juce::Image load(const void* d, int s, int scale) {
        const auto img = juce::ImageCache::getFromMemory(d, s);
        return img.rescaled(img.getWidth() * scale, img.getHeight() * scale, juce::Graphics::lowResamplingQuality).createCopy();
    }
    static int widthRel(float x) { return static_cast<int>(x * Width); }
    static int heightRel(float x) { return static_cast<int>(x * Height); }
    static juce::Rectangle<int> boundsDownscaled() { return { 0, 0 , Width, Height }; }
}

struct NELGUtil {
    enum Cursor { Norm, Hover };
    NELGUtil() :
        cursors()
    {
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
    }
    std::vector<juce::MouseCursor> cursors;
};

#include "SpaceEditor.h"
#include "TitleEditor.h"
#include "ParametersEditor.h"

class Nel19AudioProcessorEditor :
    public juce::AudioProcessorEditor,
    public juce::Timer {
public:
    Nel19AudioProcessorEditor (Nel19AudioProcessor&);
    void resized() override;
    void timerCallback() override;
private:
    Nel19AudioProcessor& audioProcessor;
    NELGUtil nelGUtil;
    SpaceEditor space;
    TitleEditor title;
    ParametersEditor params;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Nel19AudioProcessorEditor)
};