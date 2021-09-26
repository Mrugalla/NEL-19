#pragma once
#include "PixelArt.h"

class VisualizerComp :
    public Comp,
    public juce::Timer
{
    struct SideScroller {
        SideScroller(const Utils& u) :
            utils(u),
            img(),
            midY(0),
            idx(0)
        {}
        void resized(juce::Rectangle<int> bounds) {
            midY = static_cast<int>(std::rint(static_cast<float>(bounds.getHeight()) * .5f));
            img = juce::Image(juce::Image::ARGB, bounds.getWidth(), bounds.getHeight(), true);
        }
        void scroll(float value /* [-1, 1] */) {
            if (!img.isValid()) return;
            inc();
            const auto height = static_cast<float>(img.getHeight());
            const auto yEnd = static_cast<int>(height - (value * .5f + .5f) * height);
            img.multiplyAllAlphas(.999f);
            auto y = midY;
            img.setPixelAt(idx, y, utils.colours[Utils::Normal]);
            if(y < yEnd)
                while (y < yEnd) {
                    img.setPixelAt(idx, y, utils.colours[Utils::Normal]);
                    ++y;
                }
            else if(y > yEnd)
                while (y > yEnd) {
                    img.setPixelAt(idx, y, utils.colours[Utils::Normal]);
                    --y;
                }
        }
        void paint(juce::Graphics& g) {
            if (!img.isValid()) return;
            const auto width = img.getWidth();
            const auto height = img.getHeight();
            const auto w0 = width - idx;
            g.drawImageAt(
                img.getClippedImage({ 0,0,idx,height }),
                w0,
                0,
                false
            );
            g.drawImageAt(
                img.getClippedImage({ idx + 1,0,w0 - 1,height }),
                0,
                0,
                false
            );
        }
    protected:
        const Utils& utils;
        juce::Image img;
        int midY, idx;
    private:
        void inc() noexcept {
            ++idx;
            if (idx >= img.getWidth())
                idx = 0;
        }
    };
public:
    VisualizerComp(Nel19AudioProcessor& p, Utils& u) :
        Comp(p, u, "The Vibrato is visualized here."),
        sideScroller(u)
    {
        startTimerHz(24);
        setBufferedToImage(true);
    }
protected:
    SideScroller sideScroller;

    void timerCallback() override {
        const auto val = processor.vibDelayVisualizerValue[0].get();
        sideScroller.scroll(val);
        repaint();
    }
    void resized() override { sideScroller.resized(getLocalBounds()); }
    void paint(juce::Graphics& g) override { sideScroller.paint(g); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VisualizerComp)
};

/*
to do

add both channels to visualization (mid/side in other colours?)

why outliers in signal (not in value. maybe timerCallback and paint happen simultanously)

*/