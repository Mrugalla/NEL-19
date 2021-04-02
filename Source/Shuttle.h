#pragma once
#include <JuceHeader.h>
#include <array>

struct Shuttle :
    public NELComp,
    public juce::Timer
{
    // chromatic aberration fx on shuttle visualizes vibrato strength
    enum { R, G, B };
    enum { Left, Right };

    struct Colour {
        Colour(float a, juce::uint8 co) :
            alpha(a),
            c(co)
        {}
        float alpha;
        juce::uint8 c;
    };

    struct ColourVec {
        void resize(int w, int h) {
            colours.resize(w);
            for (auto& i : colours) i.resize(h, 0);
        }
        void set(int x, int y, juce::uint8 c) {
            colours[x][y] = c;
        }
        const juce::uint8 getCol(int x, int y) const { return colours[x][y]; }
        std::vector<std::vector<juce::uint8>> colours;
    };

    Shuttle(Nel19AudioProcessor& p, NELGUtil& u) :
        NELComp(p, u, "Lionel's space shuttle vibrates your music! :>"),
        img(nelG::load(BinaryData::shuttle_png, BinaryData::shuttle_pngSize, 2)),
        cVecs(),
        lfoValues(p.nel19.getLFOValues()),
        lastValue(), offset(),
        heightMax(0), shuttleYMax(0)
    {
        for (auto& x : lastValue) x = 0;
        for (auto& x : offset) x = 0;

        startTimerHz(nelG::FPS);
    }
    juce::Image img;
    std::array<ColourVec, 3> cVecs;
    const std::array<std::atomic<float>, 2>& lfoValues;
    std::array<float, 2> lastValue;
    std::array<int, 2> offset;
    int heightMax, shuttleYMax;
private:
    void paint(juce::Graphics& g) override {
#if DebugLayout
        g.setColour(juce::Colours::red);
        g.drawRect(getLocalBounds());
#endif
        for (auto y = 0; y < img.getHeight(); ++y) {
            const auto yR = juce::jlimit(0, heightMax, y + offset[Left]);
            const auto yG = juce::jlimit(0, heightMax, y - offset[Left]);
            const auto yB = juce::jlimit(0, heightMax, y + offset[Right]);
            
            for (auto x = 0; x < img.getWidth(); ++x) {
                const auto red = cVecs[R].getCol(x, yR);
                const auto green = cVecs[G].getCol(x, yG);
                const auto blue = cVecs[B].getCol(x, yB);
                if (red + green + blue == 0)
                    img.setPixelAt(x, y, juce::Colours::transparentBlack);
                else
                    img.setPixelAt(x, y, juce::Colour(red, green, blue));
            }
        }
        g.drawImageAt(img, 0, 0, false);
    }
    void resized() override {
        const auto yOffset = (getHeight() - img.getHeight()) / 2;
        for (auto& c : cVecs)
            c.resize(getWidth(), getHeight());
        for (auto y = yOffset; y < getHeight(); ++y) {
            const auto n = y - yOffset;
            for (auto x = 0; x < getWidth(); ++x) {
                const auto pxl = img.getPixelAt(x, n);
                cVecs[R].set(x, y, pxl.getRed());
                cVecs[G].set(x, y, pxl.getGreen());
                cVecs[B].set(x, y, pxl.getBlue());
            }
        }
        heightMax = getHeight() - 1;
        const auto centreY = getHeight() / 2;
        shuttleYMax = centreY / 4;
        img = juce::Image(juce::Image::ARGB, img.getWidth(), getHeight(), false);
    }
    void timerCallback() override {
        const auto lValue = lfoValues[Left].load();
        const auto rValue = lfoValues[Right].load();
        offset[Left] = static_cast<int>((lValue - lastValue[Left]) * shuttleYMax);
        offset[Right] = static_cast<int>((rValue - lastValue[Right]) * shuttleYMax);
        lastValue[Left] = lValue;
        lastValue[Right] = rValue;
        repaint();
    }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Shuttle)
};