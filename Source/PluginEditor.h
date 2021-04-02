#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>
#define DebugLayout false

namespace nelG {
    static constexpr float Pi = 3.14159265359f;
    static constexpr float Tau = 6.28318530718f;
    static constexpr float PiHalf = Pi * .5f;
    static constexpr float PiQuart = Pi * .25f;
    static constexpr float PiInv = 1.f / Pi;

    static constexpr int Width = 512 + 32 * 2;
    static constexpr int Height = 512 - 8 * 3;
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

    struct Ratio {
        Ratio(const std::vector<float>&& entries, float scaled = 1.f, float xOffset = 0.f) :
            data(entries), x()
        {
            auto max = 0.f;
            for (const auto& d : data) max += d;
            max = scaled / max;
            x.reserve(data.size());
            float inc = 0.f;
            for (auto& d : data) {
                d *= max;
                x.emplace_back(inc + xOffset);
                inc += d;
            }
        }
        const size_t size() const { return data.size(); }
        const float operator[](int i) const { return data[i]; }
        const float getX(int i) const { return x[i]; }
    private:
        std::vector<float> data, x;
    };
    struct RatioBounds {
        RatioBounds(const Ratio& rX, const Ratio& rY) :
            bounds(),
            sizeX(rX.size())
        {
            const auto sizeY = rY.size();
            const auto size = sizeX * sizeY;
            bounds.reserve(size);
            for (auto i = 0; i < size; ++i) {
                const auto x = i % sizeX;
                const auto y = i / sizeX;
                bounds.emplace_back(juce::Rectangle<float>(rX.getX(x), rY.getX(y), rX[x], rY[y]).toNearestInt());
            }
        }
        const juce::Rectangle<int> operator()(int x, int y) const { return bounds[x + y * sizeX]; }
        const juce::Rectangle<int> operator()(int x0, int y0, int x1, int y1) const {
            const auto startBounds = bounds[x0 + y0 * sizeX];
            const auto endBounds = bounds[x1 + y1 * sizeX];
            return {
                startBounds.getX(),
                startBounds.getY(),
                endBounds.getRight() - startBounds.getX(),
                endBounds.getBottom() - startBounds.getY()
            };
        }
    private:
        std::vector<juce::Rectangle<int>> bounds;
        size_t sizeX;
    };
    struct Image {
        void paint(juce::Graphics& g) { g.drawImageAt(img, x, y, false); }
        void setBounds(juce::Rectangle<int>&& b) {
            x = b.getX();
            y = b.getY();
            img = juce::Image(juce::Image::ARGB, b.getWidth(), b.getHeight(), true);
        }
        const juce::Rectangle<int> getBounds() const { return img.getBounds(); }
        void operator()(int x, int y, const juce::Colour& c) { img.setPixelAt(x, y, c); }
        juce::Image img;
        int x, y;
    };
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

#include "Shuttle.h"
#include "SettingsEditor.h"
#include "RandomizerButton.h"
#include "TooltipComponent.h"
#include "ParametersEditor.h"
#include "WavetableEditor.h"

struct Nel19AudioProcessorEditor :
    public juce::AudioProcessorEditor
{
    Nel19AudioProcessorEditor(Nel19AudioProcessor&);
private:
    Nel19AudioProcessor& audioProcessor;
    NELGUtil utils;
    Shuttle shuttle;

    QuickAccessWheel depthMaxK;
    ButtonSwitch lrmsK;
    Knob depthK, freqK, widthK, mixK;
    SliderShape shapeK;

    WavetableEditor wavetable;
    Settings settings;
    SettingsButton settingsButton;
    RandomizerButton randomizerButton;
    TooltipComponent tooltip;

    void resized() override;
    void paint(juce::Graphics&) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Nel19AudioProcessorEditor)
};