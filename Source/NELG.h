#pragma once
#include <JuceHeader.h>

namespace nelG {
    static constexpr int Width = 512 + 32 * 4;
    static constexpr int Height = 512 - 8 * 32;
    static constexpr float Scale = 1;
    static constexpr int FPS = 12;

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
            for (auto& d : data)
                d *= max;
            x.reserve(data.size());
            auto inc = xOffset;
            for (const auto& d : data) {
                x.emplace_back(inc);
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
            sizeX(static_cast<int>(rX.size())),
            sizeY(static_cast<int>(rY.size()))
        {
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
        const juce::Rectangle<int> bottomBar() { return operator()(0, sizeY - 1, sizeX - 1, sizeY - 1); }
    private:
        std::vector<juce::Rectangle<int>> bounds;
        int sizeX, sizeY;
    };
    struct RatioBounds2 {
        RatioBounds2(const std::vector<float>&& ratioX, const std::vector<float>&& ratioY) :
            rXRaw(ratioX),
            rYRaw(ratioY),
            rX(ratioX),
            rY(ratioY)
        {
            auto sum = 0.f;
            for (const auto& x : rXRaw) sum += x;
            auto gain = 1.f / sum;
            for (auto& x : rXRaw) x *= gain;

            sum = 0.f;
            for (const auto& y : rYRaw) sum += y;
            gain = 1.f / sum;
            for (auto& y : rYRaw) y *= gain;

            for (auto x = 1; x < rXRaw.size(); ++x)
                rXRaw[x] += rXRaw[x - 1];
            for (auto y = 1; y < rY.size(); ++y)
                rYRaw[y] += rYRaw[y - 1];
        }
        // SET
        void setBounds(const juce::Rectangle<float>& bounds) {
            for (auto x = 0; x < rX.size(); ++x)
                rX[x] = rXRaw[x] * bounds.getWidth();
            for (auto y = 0; y < rY.size(); ++y)
                rY[y] = rYRaw[y] * bounds.getHeight();
            for (auto& x : rX) x += bounds.getX();
            for (auto& y : rY) y += bounds.getY();
        }
        // PROCESS
        juce::Rectangle<float> operator()(){
            return {
                rX[0],
                rY[0],
                rX[rX.size() - 1],
                rY[rY.size() - 1]
            };
        }
        juce::Rectangle<float> operator()(int x, int y, int width = 1, int height = 1) const {
            return { rX[x], rY[y], rX[x + width] - rX[x], rY[y + height] - rY[y] };
        }
        juce::Rectangle<float> bottomBar() const {
            return {
                rX[0],
                rY[rY.size() - 2],
                rX[rX.size() - 1] - rX[0],
                rY[rY.size() - 1] - rY[rY.size() - 2] };
        }
        juce::Rectangle<float> exceptBottomBar() const {
            return {
                rX[0],
                rY[0],
                rX[rX.size() - 1] - rX[0],
                rY[rY.size() - 2] - rY[0] };
        }
        void paintGrid(juce::Graphics& g) {
            g.setColour(juce::Colours::limegreen);
            for (auto x = 0; x < rX.size(); ++x)
                g.drawLine(rX[x], 0, rX[x], rY[rY.size() - 1]);
            for (auto y = 0; y < rY.size(); ++y)
                g.drawLine(0, rY[y], rX[rX.size() - 1], rY[y]);
        }
    private:
        std::vector<float> rXRaw, rYRaw, rX, rY;
    };

    struct Image {
        void paint(juce::Graphics& g) { g.drawImageAt(img, x, y, false); }
        void setBounds(juce::Rectangle<int>&& b) {
            x = b.getX();
            y = b.getY();
            img = juce::Image(juce::Image::ARGB, b.getWidth(), b.getHeight(), true);
        }
        const juce::Rectangle<int> getBounds() const { return img.getBounds(); }
        void operator()(int xx, int yy, const juce::Colour& cc) { img.setPixelAt(xx, yy, cc); }
        juce::Image img;
        int x, y;
    };

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
                    if (cursorImage.getPixelAt(x, y) == juce::Colour(util::ColGreen))
                        cursorHoverImage.setPixelAt(x, y, juce::Colour(util::ColYellow));
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

    struct Comp :
        public juce::Component {

        Comp(Nel19AudioProcessor& p, Utils& u, juce::String tooltp = "") :
            processor(p),
            utils(u),
            tooltip(tooltp)
        { setMouseCursor(u.cursors[Utils::Cursor::Norm]); }
        Nel19AudioProcessor& processor;
        Utils& utils;
        juce::String tooltip;
        void mouseMove(const juce::MouseEvent&) override { utils.tooltip = &tooltip; }
    };

    struct CompParam :
        public Comp {
        CompParam(Nel19AudioProcessor& p, Utils& u, juce::String tooltp = "") :
            Comp(p, u, tooltp),
            enabled(true)
        {

        }
        virtual void setEnabled(bool e) {
            enabled = e;
            if (enabled) setMouseCursor(utils.cursors[nelG::Utils::Cursor::Hover]);
            else setMouseCursor(utils.cursors[nelG::Utils::Cursor::Norm]);
            repaint();
        }

        bool enabled;
    };
}