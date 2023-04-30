#pragma once
#include <JuceHeader.h>
#define DebugLayout false && JUCE_DEBUG

namespace gui
{
    using Path = juce::Path;
    using Stroke = juce::PathStrokeType;
    using Bounds = juce::Rectangle<int>;
    using BoundsF = juce::Rectangle<float>;
    using Point = juce::Point<int>;
    using PointF = juce::Point<float>;
    using Line = juce::Line<int>;
    using LineF = juce::Line<float>;
    using Just = juce::Justification;
    using String = juce::String;
    using Graphics = juce::Graphics;
    using Colour = juce::Colour;
    using Font = juce::Font;
    using Component = juce::Component;
    using Timer = juce::Timer;
    using Mouse = juce::MouseEvent;
    using MouseWheel = juce::MouseWheelDetails;
    using Image = juce::Image;
    using Just = juce::Justification;
    using Random = juce::Random;
    using ValueTree = juce::ValueTree;
	using Identifier = juce::Identifier;
    
    // MATH UTILS
    static constexpr float Pi = 3.14159265359f;
    static constexpr float Tau = 6.28318530718f;
    static constexpr float PiHalf = Pi * .5f;
    static constexpr float PiQuart = Pi * .25f;
    // DEFAULT BOUNDS
    static constexpr int Width = 546;
    static constexpr int Height = 447;

    inline BoundsF maxQuadIn(const BoundsF& b) noexcept
    {
        const auto minDimen = std::min(b.getWidth(), b.getHeight());
        const auto x = b.getX() + .5f * (b.getWidth() - minDimen);
        const auto y = b.getY() + .5f * (b.getHeight() - minDimen);
        return { x, y, minDimen, minDimen };
    }

    inline BoundsF maxQuadIn(Bounds b) noexcept
    {
        return maxQuadIn(b.toFloat());
    }

    class Layout
    {
        struct NamedLocation
        {
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
        Layout(std::vector<float>&& xxx, std::vector<float>&& yyy) :
            rXRaw(),
            rYRaw(),
            rX(),
            rY()
        {
            rXRaw.resize(xxx.size() + 2, 0.f);
            rYRaw.resize(yyy.size() + 2, 0.f);
            auto sum = 0.f;
            for (auto x = 0; x < xxx.size(); ++x)
            {
                rXRaw[x + 1] = xxx[x];
                sum += xxx[x];
            }
            rXRaw[xxx.size() + 1] = 1.f;
            auto gain = 1.f / sum;
            for (auto& x : rXRaw)
                x *= gain;

            sum = 0.f;
            for (auto y = 0; y < yyy.size(); ++y)
            {
                rYRaw[y + 1] = yyy[y];
                sum += yyy[y];
            }
			rYRaw[yyy.size() + 1] = 1.f;
            gain = 1.f / sum;
            for (auto& y : rYRaw)
                y *= gain;

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
        void place(juce::Component& comp, int x, int y, int width = 1, int height = 1, float padding = 0.f, bool isQuad = false)
        {
            const auto cBounds = this->operator()(x, y, width, height);
            if(!isQuad)
                if(padding != 0.f)
                    comp.setBounds(cBounds.reduced(padding).toNearestInt());
                else
                    comp.setBounds(cBounds.toNearestInt());
            else
                if (padding != 0.f)
                    comp.setBounds(maxQuadIn(cBounds).reduced(padding).toNearestInt());
                else
                    comp.setBounds(maxQuadIn(cBounds).toNearestInt());
        }
        void place(juce::Component* comp, int x, int y, int width = 1, int height = 1, float padding = 0.f, bool isQuad = false) {
            if (comp == nullptr) return;
            place(*comp, x, y, width, height, padding, isQuad);
        }
        
        juce::Rectangle<float> operator()() const noexcept {
            return {
                rX[0],
                rY[0],
                rX[rX.size() - 1] - rX[0],
                rY[rY.size() - 1] - rY[0]
            };
        }
        
        //juce::Rectangle<float> operator()(int x, int y, int width = 1, int height = 1, bool isQuad = false) const noexcept {
        //    juce::Rectangle<float> nBounds(rX[x], rY[y], rX[x + width] - rX[x], rY[y + height] - rY[y]);
        //    if (!isQuad) return nBounds;
        //    else return maxQuadIn(nBounds);
        //}
        juce::Rectangle<float> operator()(int x, int y, int width = 1, int height = 1, float padding = 0.f, bool isQuad = false) const noexcept
        {
            juce::Rectangle<float> nBounds(rX[x], rY[y], rX[x + width] - rX[x], rY[y + height] - rY[y]);
            if (!isQuad)
                if (padding == 0.f)
                    return nBounds;
                else
                    return nBounds.reduced(padding);
            else
                if (padding == 0.f)
                    return maxQuadIn(nBounds);
                else
                    return maxQuadIn(nBounds).reduced(padding);
        }
        
        juce::Rectangle<float> bottomBar() const noexcept
        {
            return
            {
                rX[0],
                rY[rY.size() - 3],
                rX[rX.size() - 1] - rX[0],
                rY[rY.size() - 2] - rY[rY.size() - 3]
            };
        }
        
        juce::Rectangle<float> topBar() const noexcept
        {
            return
            {
                rX[0],
                rY[0],
                rX[rX.size() - 1] - rX[0],
                rY[1] - rY[0]
            };
        }
        
        juce::Rectangle<float> exceptBottomBar() const noexcept {
            return {
                rX[0],
                rY[0],
                rX[rX.size() - 1] - rX[0],
                rY[rY.size() - 2] - rY[0] };
        }
        
        float getX(int i) const noexcept
        {
			return rX[i];
		}

		float getY(int i) const noexcept
		{
			return rY[i];
		}
        
		float getW(int i) const noexcept
		{
			return rX[i + 1] - rX[i];
		}

		float getH(int i) const noexcept
		{
			return rY[i + 1] - rY[i];
		}

        void paintGrid(juce::Graphics&)
        {
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
#if DebugLayout
        void addNamedLocation(juce::String&& name, int x, int y, int width, int height, bool isQuad = false, bool isElliptic = false, bool isVisible = true)
        {
            namedLocs.push_back({ std::move(name), x, y, width, height, isQuad, isElliptic, isVisible });
        }
        
        void mouseMove(juce::Point<float> p)
        {
            pos = p;
        }
#else
        void addNamedLocation(juce::String&&, int, int, int, int, bool = false, bool = false, bool = true)
        {
        }
        
        void mouseMove(juce::Point<float>)
        {
        }
#endif
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

    inline void fillAndOutline(Graphics& g, BoundsF bounds, float thicc,
        Colour bg, Colour lines = juce::Colours::transparentBlack)
    {
        g.setColour(bg);
        g.fillRoundedRectangle(bounds, thicc);
        g.setColour(lines);
        g.drawRoundedRectangle(bounds, thicc, thicc);
    }
    
    inline void fillAndOutline(Graphics& g, const Component& comp, float thicc,
        Colour bg, Colour lines = juce::Colours::transparentBlack)
    {
        fillAndOutline(g, comp.getBounds().toFloat(), thicc, bg, lines);
    }
    
    inline void fillAndOutline(Graphics& g, const Layout& layout, float thicc,
        Colour bg, Colour lines = juce::Colours::transparentBlack)
    {
        fillAndOutline(g, layout(), thicc, bg, lines);
    }

    inline Image load(const void* d, int s)
    {
        auto img = juce::ImageCache::getFromMemory(d, s);

        auto b = img.getBounds();
        for (auto y = 0; y < img.getHeight(); ++y)
            for (auto x = 0; x < img.getWidth(); ++x)
                if (!img.getPixelAt(x, y).isTransparent())
                {
                    b.setY(y);
                    break;
                }
        for (auto x = 0; x < img.getWidth(); ++x)
            for (auto y = 0; y < img.getHeight(); ++y)
                if (!img.getPixelAt(x, y).isTransparent())
                {
                    b.setX(x);
                    break;
                }
        for (auto y = img.getHeight() - 1; y > -1; --y)
            for (auto x = img.getWidth() - 1; x > -1; --x)
                if (!img.getPixelAt(x, y).isTransparent())
                {
                    b.setHeight(y - b.getY());
                    break;
                }
        for (auto x = img.getWidth() - 1; x > -1; --x)
            for (auto y = img.getHeight() - 1; y > -1; --y)
                if (!img.getPixelAt(x, y).isTransparent())
                {
                    b.setWidth(x - b.getX());
                    break;
                }
        if (b.getX() < 0 || b.getY() < 0 || b.getWidth() < 0 || b.getHeight() < 0)
            return img.createCopy();
        return img.getClippedImage(b).createCopy();
    }
    
    inline Image load(const void* d, int s, int scale)
    {
        const auto img = load(d, s);
        return img.rescaled
        (
            img.getWidth() * scale,
            img.getHeight() * scale,
            Graphics::lowResamplingQuality
        ).createCopy();
    }
}

/*


        
*/