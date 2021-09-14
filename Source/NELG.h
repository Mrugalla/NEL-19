#pragma once
#include <JuceHeader.h>
#define DebugLayout true && JUCE_DEBUG

namespace nelG {
    static constexpr float Pi = 3.14159265359f;
    static constexpr float Tau = 6.28318530718f;
    static constexpr float PiHalf = Pi * .5f;
    static constexpr float PiQuart = Pi * .25f;

    static constexpr float Thicc = 2.5f, Thicc2 = Thicc * 2.f;
    static constexpr int Width = 512 + 256;
    static constexpr int Height = 256 + 128 + 16;

    static constexpr unsigned int ColBlack = 0xff171623;
    static constexpr unsigned int ColDarkGrey = 0xff595652;
    static constexpr unsigned int ColBeige = 0xff9c8980;
    static constexpr unsigned int ColGrey = 0xff696a6a;
    static constexpr unsigned int ColRed = 0xffac3232;
    static constexpr unsigned int ColGreen = 0xff37946e;
    static constexpr unsigned int ColYellow = 0xfffffa8f;
    static constexpr unsigned int ColGreenNeon = 0xff99c550;

    static juce::Rectangle<float> maxQuadIn(const juce::Rectangle<float>& b) noexcept {
        const auto minDimen = std::min(b.getWidth(), b.getHeight());
        const auto x = b.getX() + .5f * (b.getWidth() - minDimen);
        const auto y = b.getY() + .5f * (b.getHeight() - minDimen);
        return { x, y, minDimen, minDimen };
    }

    class Layout {
        struct NamedLocation {
            NamedLocation(juce::String&& n, int xx, int yy, int widthh, int heightt, bool quad, bool ellip, bool visible) :
                name(n),
                x(xx),
                y(yy),
                width(widthh),
                height(heightt),
                isQuad(quad),
                isElliptic(ellip),
                isVisible(visible)
            {}
            juce::String name;
            int x, y, width, height;
            bool isQuad, isElliptic, isVisible;
        };
    public:
        Layout(const std::vector<float> xxx, const std::vector<float> yyy) :
            rXRaw(),
            rYRaw(),
            rX(),
            rY()
        {
            rXRaw.resize(xxx.size() + 1, 0.f);
            rYRaw.resize(yyy.size() + 1, 0.f);
            auto sum = 0.f;
            for (auto x = 0; x < xxx.size(); ++x) {
                rXRaw[x + 1] = xxx[x];
                sum += xxx[x];
            }
            auto gain = 1.f / sum;
            for (auto& x : rXRaw) x *= gain;

            sum = 0.f;
            for (auto y = 0; y < yyy.size(); ++y) {
                rYRaw[y + 1] = yyy[y];
                sum += yyy[y];
            }
            gain = 1.f / sum;
            for (auto& y : rYRaw) y *= gain;

            for (auto x = 1; x < rXRaw.size(); ++x)
                rXRaw[x] += rXRaw[x - 1];
            for (auto y = 1; y < rYRaw.size(); ++y)
                rYRaw[y] += rYRaw[y - 1];

            rX.resize(rXRaw.size(), 0.f);
            rY.resize(rYRaw.size(), 0.f);
        }
        // SET
        void setBounds(const juce::Rectangle<float>& bounds) noexcept {
            for (auto x = 0; x < rX.size(); ++x)
                rX[x] = rXRaw[x] * bounds.getWidth();
            for (auto y = 0; y < rY.size(); ++y)
                rY[y] = rYRaw[y] * bounds.getHeight();
            for (auto& x : rX) x += bounds.getX();
            for (auto& y : rY) y += bounds.getY();
        }
        // PROCESS
        void place(juce::Component& comp, int x, int y, int width = 1, int height = 1, bool isQuad = false) {
            const auto cBounds = this->operator()(x, y, width, height);
            if(!isQuad) comp.setBounds(cBounds.toNearestInt());
            else comp.setBounds(maxQuadIn(cBounds).toNearestInt());
        }
        void place(juce::Component* comp, int x, int y, int width = 1, int height = 1, bool isQuad = false) {
            if (comp == nullptr) return;
            const auto cBounds = this->operator()(x, y, width, height);
            if (!isQuad) comp->setBounds(cBounds.toNearestInt());
            else comp->setBounds(maxQuadIn(cBounds).toNearestInt());
        }
        juce::Rectangle<float> operator()() {
            return {
                rX[0],
                rY[0],
                rX[rX.size() - 1],
                rY[rY.size() - 1]
            };
        }
        juce::Rectangle<float> operator()(int x, int y, int width = 1, int height = 1, bool isQuad = false) const {
            juce::Rectangle<float> nBounds(rX[x], rY[y], rX[x + width] - rX[x], rY[y + height] - rY[y]);
            if (!isQuad) return nBounds;
            else return maxQuadIn(nBounds);
        }
        juce::Rectangle<float> bottomBar() const {
            return {
                rX[0],
                rY[rY.size() - 2],
                rX[rX.size() - 1] - rX[0],
                rY[rY.size() - 1] - rY[rY.size() - 2] };
        }
        juce::Rectangle<float> topBar() const {
            return {
                rX[0],
                rY[0],
                rX[rX.size() - 1] - rX[0],
                rY[1] - rY[0] };
        }
        juce::Rectangle<float> exceptBottomBar() const {
            return {
                rX[0],
                rY[0],
                rX[rX.size() - 1] - rX[0],
                rY[rY.size() - 2] - rY[0] };
        }
        void paintGrid(juce::Graphics& g) {
#if DebugLayout
            const auto shownAThing = paintNamedLocs(g);
            if(shownAThing)
                g.setColour(juce::Colour(0x77ffffff));
            else
                g.setColour(juce::Colour(0x22ffffff));
            for (auto x = 0; x < rX.size(); ++x)
                g.drawLine(rX[x], rY[0], rX[x], rY[rY.size() - 1]);
            for (auto y = 0; y < rY.size(); ++y)
                g.drawLine(rX[0], rY[y], rX[rX.size() - 1], rY[y]);
#endif
        }
        void addNamedLocation(juce::String&& name, int x, int y, int width, int height, bool isQuad = false, bool isElliptic = false, bool isVisible = true) {
#if DebugLayout
            namedLocs.push_back({std::move(name), x, y, width, height, isQuad, isElliptic, isVisible});
#endif
        }
        void mouseMove(juce::Point<float> p) {
#if DebugLayout
            pos = p;
#endif
        }
    private:
        std::vector<float> rXRaw, rYRaw, rX, rY;
#if DebugLayout
        std::vector<NamedLocation> namedLocs;
        juce::Point<float> pos;

        bool paintNamedLocs(juce::Graphics& g) {
            bool shownAThing = false;
            juce::String str;
            juce::Colour col(0x77ff00dd);
            g.setColour(col);
            for (const auto& nl : namedLocs) {
                if (nl.isVisible) {
                    const auto area = this->operator()(nl.x, nl.y, nl.width, nl.height);
                    if (area.contains(pos.x, pos.y)) {
                        shownAThing = true;
                        if (str.isNotEmpty()) {
                            col = col.withRotatedHue(.2f);
                            g.setColour(col);
                            str.clear();
                        }
                        str += nl.name + juce::String(": ") +
                            juce::String(nl.x) + ", " + juce::String(nl.y) + ", " +
                            juce::String(nl.width) + ", " + juce::String(nl.height);
                        if (!nl.isQuad) g.fillRect(area);
                        else
                            if (!nl.isElliptic)
                                g.fillRect(maxQuadIn(area));
                            else
                                g.fillEllipse(maxQuadIn(area));
                        g.setColour(juce::Colours::white);
                        g.drawFittedText(str, area.toNearestInt(), juce::Justification::centredTop, 2);
                    }
                }
            }
            return shownAThing;
        }
#endif
    };

    static juce::Image load(const void* d, int s) {
        auto img = juce::ImageCache::getFromMemory(d, s);

        auto b = img.getBounds();
        for (auto y = 0; y < img.getHeight(); ++y)
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
}

/*


        
*/