#pragma once
#include <JuceHeader.h>
#include <array>

namespace Util {
    static constexpr double Tau = 6.28318530718;
    static constexpr int SliderWidth = 4;
    static constexpr int SliderHeight = 21;
    static constexpr int LFOY = 122, LFOHeight = 2, LFOWidthHalf = 14;
    static constexpr double LFOMidX = 99.5;

    template<typename Float>
    static void dbg(const std::vector<Float>& v, juce::String title = "") {
        if (title.length() != 0) title += " >> ";
        title += juce::String(v.size()) + ":\n";
        int idx = 0;
        for (const auto& i : v) {
            ++idx;
            if (idx > 14) {
                idx = 0;
                title += "\n";
            }
            title += juce::String(i) + ", ";
        }
        title += "\n";
        DBG(title);
    }

    template<typename Float>
    static Float ms2frames(Float ms, int fps) { return ms * fps / 1000; }

    template<typename Float>
    static Float hz2frames(Float hz, int fps) { return (Float)fps / hz; }

    static void updateParam(juce::Image& dest, juce::Colour& en, juce::Colour& dis, float depth, int x, int y) {
        auto dQuant = int((1 - depth) * SliderHeight);
        for (auto yy = 0; yy < dQuant; ++yy)
            for (auto xx = 0; xx < SliderWidth; ++xx)
                dest.setPixelAt(x + xx,y + yy, dis);
        for (auto yy = dQuant; yy < SliderHeight; ++yy)
            for (auto xx = 0; xx < SliderWidth; ++xx)
                dest.setPixelAt(x + xx,y + yy, en);
    }

    static void addWidthDisabled(juce::Image& dest, const juce::Image& disabled) {
        auto x = 107;
        auto y = 76;
        auto width = 6;
        auto height = 31;

        for (auto yy = 0; yy < height; ++yy) {
            auto yOff = yy + y;
            for (auto xx = 0; xx < width; ++xx) {
                const auto& pixel = disabled.getPixelAt(xx, yy);
                if (!pixel.isTransparent()) {
                    auto xOff = xx + x;
                    dest.setPixelAt(xOff, yOff, pixel);
                }
            }
        }
    }

    static const juce::Image setStudioColour(const juce::Image& ui, const juce::Colour& c) {
        auto x = 10;
        auto y = 46;
        auto width = 28;
        auto height = 8;

        juce::Image newImage(juce::Image::ARGB, width, height, true);

        for (auto yy = 0; yy < height; ++yy) {
            auto yOff = yy + y;
            for (auto xx = 0; xx < width; ++xx) {
                auto xOff = xx + x;
                if (ui.getPixelAt(xOff, yOff).isTransparent())
                    newImage.setPixelAt(xx, yy, c);
                else
                    newImage.setPixelAt(xx, yy, juce::Colour(0x0));
            }
        }
        return newImage;
    }

    static void addStudio(juce::Image& ui, const juce::Image& studio) {
        auto x = 86;
        auto y = 109;
        auto width = 28;
        auto height = 8;

        for (auto yy = 0; yy < height; ++yy) {
            auto yOff = yy + y;
            for (auto xx = 0; xx < width; ++xx) {
                const auto& pixel = studio.getPixelAt(xx, yy);
                if (!pixel.isTransparent()) {
                    auto xOff = xx + x;
                    ui.setPixelAt(xOff, yOff, pixel);
                }
            }
        }
    }

    static juce::Image getUpscaledCursor(float upscaleFactor) {
        auto cursor = juce::ImageCache::getFromMemory(BinaryData::cursor_png, BinaryData::cursor_pngSize);
        auto width = (int)(cursor.getWidth() * upscaleFactor);
        auto height = (int)(cursor.getHeight() * upscaleFactor);
        juce::Image newCursor(juce::Image::ARGB, width, height, true);
        juce::Graphics g{ newCursor };
        g.setImageResamplingQuality(juce::Graphics::lowResamplingQuality);
        g.drawImage(cursor, {0.f, 0.f, (float)width, (float)height} );
        return newCursor;
    }
}

struct Letter { int x, width; };
struct Font {
    static constexpr int LetterCount = 15;
    static constexpr int Height = 9;
    enum { Zero, One, Two, Three, Four, Five, Six, Seven, Eight, Nine, Start, Hz, Percent, Point, End };

    Font() :
        letter(initLetter()),
        srcImage(juce::ImageCache::getFromMemory(BinaryData::font_png, BinaryData::font_pngSize))
    {}

    const Letter getLetter(const juce::juce_wchar chr) {
        switch (chr) {
        case '0': return letter[Zero];
        case '1': return letter[One];
        case '2': return letter[Two];
        case '3': return letter[Three];
        case '4': return letter[Four];
        case '5': return letter[Five];
        case '6': return letter[Six];
        case '7': return letter[Seven];
        case '8': return letter[Eight];
        case '9': return letter[Nine];
        case ' ': return letter[Start];
        case '.': return letter[Point];
        }
    }

    const std::array<Letter, LetterCount> letter;
    const juce::Image srcImage;
private:
    const std::array<Letter, LetterCount> initLetter() {
        std::array<Letter, LetterCount> ltr;
        ltr[Start] = { 0, 2 };
        ltr[Zero] = { 2, 4 };
        ltr[One] = { 6, 3 };
        ltr[Two] = { 9, 3 };
        ltr[Three] = { 12, 3 };
        ltr[Four] = { 15, 4 };
        ltr[Five] = { 19, 3 };
        ltr[Six] = { 22, 4 };
        ltr[Seven] = { 26, 4 };
        ltr[Eight] = { 30, 4 };
        ltr[Nine] = { 34, 4 };
        ltr[Hz] = { 38, 8 };
        ltr[Percent] = { 46, 5 };
        ltr[Point] = { 51, 2 };
        ltr[End] = { 53, 1 };
        return ltr;
    }
};
struct ParameterTextField {
    enum Type { Percent, Freq };

    ParameterTextField(Font& font, juce::Image& textImage, float upscale, Type t) :
        font(font),
        textImage(textImage),
        upscaledTextImage(),
        upscaleFactor(upscale),
        type(t)
    {}
    // PARAM
    void updateParameterValue(float value) {
        switch (type) {
        case Percent: updatePercentImage(value); return;
        case Freq: updateFreqImage(value); return;
        }
    }
    // GET
    void draw(juce::Graphics& g, juce::Point<int> p) {
        g.drawImageAt(upscaledTextImage, p.getX() - upscaledTextImage.getWidth(), p.getY(), false);
    }
    Font& font;
    juce::Image& textImage;
    juce::Image upscaledTextImage;
    juce::Rectangle<float> upscaledBounds;
    float upscaleFactor;
    Type type;
private:
    void updateNumber(juce::Graphics& g, float value, int& x) {
        auto preDecimal = int(value);
        auto postDecimal = (int)((value - preDecimal) * 10);
        auto pre0 = preDecimal / 10;
        if (pre0 != 0)
            drawLetter(g, font.letter[pre0], x);
        auto pre1 = preDecimal - 10 * pre0;
        drawLetter(g, font.letter[pre1], x);
        if (postDecimal != 0) {
            drawLetter(g, font.letter[font.Point], x);
            drawLetter(g, font.letter[postDecimal], x);
        }
    }
    void updatePercentImage(float normalized) {
        auto percent = normalized * 100;
        juce::Graphics g{ textImage };
        g.setImageResamplingQuality(juce::Graphics::lowResamplingQuality);
        auto x = 0;
        drawLetter(g, font.letter[font.Start], x);
        if (percent != 100) updateNumber(g, percent, x);
        else {
            drawLetter(g, font.letter[font.One], x);
            drawLetter(g, font.letter[font.Zero], x);
            drawLetter(g, font.letter[font.Zero], x);
        }
        drawLetter(g, font.letter[font.Percent], x);
        drawLetter(g, font.letter[font.End], x);
        scaleUp(x);
    }
    void updateFreqImage(float freq) {
        juce::Graphics g{ textImage };
        g.setImageResamplingQuality(juce::Graphics::lowResamplingQuality);
        auto x = 0;
        drawLetter(g, font.letter[font.Start], x);
        updateNumber(g, freq, x);
        drawLetter(g, font.letter[font.Hz], x);
        drawLetter(g, font.letter[font.End], x);
        scaleUp(x);
    }

    void drawLetter(juce::Graphics& g, const Letter ltr, int& x) {
        g.drawImageAt(font.srcImage.getClippedImage({ ltr.x, 0, ltr.width, font.Height }), x, 0, false);
        x += ltr.width;
    }

    void scaleUp(int x) {
        juce::Rectangle<int> region(0, 0, x, font.Height);
        auto w = (int)(x * upscaleFactor);
        auto h = (int)(textImage.getHeight() * upscaleFactor);
        upscaledTextImage = textImage.getClippedImage(region).rescaled(w, h, juce::Graphics::lowResamplingQuality);
    }
};
struct ParameterTextFields {
    ParameterTextFields(float upscale, int fps) :
        font(),
        textImage(initImage()),
        textfields({
            ParameterTextField(font, textImage, upscale, ParameterTextField::Percent),
            ParameterTextField(font, textImage, upscale, ParameterTextField::Freq),
            ParameterTextField(font, textImage, upscale, ParameterTextField::Percent)
            }),
        pos(0, 0),
        lastValues({ -1, -1, -1 }),
        curParamID(-1),
        timeIdx(0), timeSize(Util::ms2frames(250, fps)),
        mouseInBounds(false)
    {}
    // SET
    void setPosition(juce::Point<int> p) { pos = p; }
    void initParameterValues(std::array<float, 3> values) {
        for (auto i = 0; i < lastValues.size(); ++i) lastValues[i] = values[i];
    }
    void enable() { mouseInBounds = true; }
    void disable() { mouseInBounds = false; }
    // PARAM
    void updateParameters(std::array<float, 3> values) {
        if (mouseInBounds)
            for (auto i = 0; i < lastValues.size(); ++i) {
                const auto value = values[i];
                if (lastValues[i] != value) {
                    lastValues[i] = value;
                    curParamID = i;
                    timeIdx = 0;
                    textfields[i].updateParameterValue(value);
                    return;
                }
            }
        ++timeIdx;
        if (timeIdx == timeSize)
            curParamID = timeIdx = -1;
    }

    // GET
    void draw(juce::Graphics& g) { if (curParamID != -1) textfields[curParamID].draw(g, pos); }
private:
    Font font;
    juce::Image textImage;
    std::array<ParameterTextField, 3> textfields;
    juce::Point<int> pos;
    std::array<float, 3> lastValues;
    int curParamID, timeIdx, timeSize;
    bool mouseInBounds;

    juce::Image initImage() {
        const int width = // construct longest word
            font.letter[font.Start].width +
            font.letter[font.Four].width * 3 +
            font.letter[font.Point].width +
            font.letter[font.Hz].width +
            font.letter[font.End].width;
        return juce::Image(juce::Image::ARGB, width, font.Height, true);
    }

    void updateParameter(const float value, const int id) { textfields[id].updateParameterValue(value); }
};

#include "AboutComponent.h"
#include "CSlider.h"

template<typename Float>
struct Space :
    public juce::Component,
    public juce::Button::Listener
{
    enum { PDepth, PFreq, PWidth, PLookahead };

    typedef juce::AudioProcessorValueTreeState::SliderAttachment SAttach;
    typedef juce::AudioProcessorValueTreeState::ButtonAttachment BAttach;

    // PARAM CONST
    static constexpr int SlidersCount = 3;
    // IMAGE CONST
    static constexpr int Width = 128;
    // ANIMATION CONST
    static constexpr int BgInterval = 1300;
    static constexpr double ShuttleFreq = .2;
    static constexpr double ShuttleAmp = 2.2;

    struct Background {
        Background(Space& space) :
            space(space),
            image({ juce::ImageCache::getFromMemory(BinaryData::space1_png, BinaryData::space1_pngSize),
                    juce::ImageCache::getFromMemory(BinaryData::space2_png, BinaryData::space2_pngSize) }),
            idx(0),
            length(1),
            curImage(0)
        {}
        void setInterval(Float ms) { length = (int)Util::ms2frames(ms, space.fps); idx = length; }
        void operator()(juce::Graphics& g) {
            increment();
            g.drawImageAt(image[curImage], 0, 0, false);
        }
    private:
        Space& space;
        std::array<juce::Image, 2> image;
        int idx, length, curImage;

        void increment() {
            ++idx;
            if (idx >= length) {
                idx = 0;
                curImage = 1 - curImage;
            }
        }
    };
    struct Shuttle {
        Shuttle(Space& space) :
            space(space),
            lfo(),
            image(juce::ImageCache::getFromMemory(BinaryData::shuttle_png, BinaryData::shuttle_pngSize)),
            idx(0)
        {}
        void setLFO(Float hz, Float gain) {
            lfo.clear();
            auto size = (int)Util::hz2frames(hz, space.fps);
            lfo.reserve(size);
            for (auto i = 0; i < size; ++i) {
                auto x = (Float)i / size;
                lfo.emplace_back((int)(std::sin(x * (Float)Util::Tau) * gain));
            }
        }
        void operator()(juce::Graphics& g) {
            increment();
            const auto x = 5;
            const auto y = 6 + lfo[idx];
            g.drawImageAt(image, x, y, false);
        }
    private:
        Space& space;
        std::vector<Float> lfo;
        juce::Image image;
        int idx;

        void increment() {
            ++idx;
            if (idx >= lfo.size())
                idx = 0;
        }
    };
    
    struct UI {
        UI(Space& space) :
            space(space),
            lfoData(),
            cEnabled(0xff37946e), cDisabled(0xffac3232),
            uiImage(juce::ImageCache::getFromMemory(BinaryData::ui_png, BinaryData::ui_pngSize)),
            studioImage(Util::setStudioColour(uiImage, cEnabled)),
            widthDisabledImage(),
            helpImage(juce::ImageCache::getFromMemory(BinaryData::help_png, BinaryData::help_pngSize)),
            cursorImage(Util::getUpscaledCursor(space.upscaleFactor)),
            studioEnabled(1), numChannels(-1)
        {}

        void operator()(juce::Graphics& g) {
            g.setImageResamplingQuality(juce::Graphics::lowResamplingQuality);
            g.drawImageAt(uiImage, 76, 63, false);
            g.drawImageAt(helpImage, 1, 117, false);
            updateParam(g, space.param[PDepth].getValueNormalized(), 90, 81);
            updateParam(g, space.param[PFreq].getValueNormalized(), 99, 81);
            checkChannelCount();
            if (numChannels == 1)
                updateParam(g, 0, 108, 81);
            else updateParam(g, space.param[PWidth].getValueNormalized(), 108, 81);
            checkLookahead();
            g.drawImageAt(studioImage, 86, 109, false);
            paintLFOs(g);
        }
    private:
        Space& space;
        std::vector<const Float*> lfoData;
        
        juce::Colour cEnabled, cDisabled;
        juce::Image uiImage, studioImage, widthDisabledImage, helpImage, cursorImage;
        int studioEnabled, numChannels;

        void checkChannelCount() {
            auto curNumChannels = space.processor.getChannelCountOfBus(true, 0);
            bool changed = numChannels != curNumChannels;
            if (changed) {
                numChannels = curNumChannels;
                if (numChannels == 1) {
                    lfoData.resize(numChannels, space.processor.getLFOValue(0));
                    uiImage = juce::ImageCache::getFromMemory(BinaryData::ui_png, BinaryData::ui_pngSize).createCopy();
                    space.param[2].setEnabled(false);
                    juce::Colour contour(0xff222034);
                    juce::Colour grey(0xff9badb7);
                    auto y = 7;
                    auto x = 31;
                    auto width = 6;
                    auto height = 35;
                    for (auto yy = 0; yy < height; ++yy) {
                        auto yyy = y + yy;
                        for (auto xx = 0; xx < width; ++xx) {
                            auto xxx = x + xx;
                            if (uiImage.getPixelAt(xxx, yyy) == contour)
                                uiImage.setPixelAt(xxx, yyy, grey);
                        }
                    }  
                }
                else {
                    lfoData.clear();
                    lfoData.reserve(2);
                    for (auto ch = 0; ch < 2; ++ch)
                        lfoData.emplace_back(space.processor.getLFOValue(ch));
                    space.param[2].setEnabled(true);
                    uiImage = juce::ImageCache::getFromMemory(BinaryData::ui_png, BinaryData::ui_pngSize);
                }
            }
        }
        void checkLookahead() {
            if (studioEnabled != space.studioButton.getToggleState()) {
                studioEnabled = space.studioButton.getToggleState();
                if (studioEnabled) studioImage = Util::setStudioColour(uiImage, cEnabled);
                else studioImage = Util::setStudioColour(uiImage, cDisabled);
            }
        }
        void updateParam(juce::Graphics& g, float depth, int x, int y) {
            auto disabledHeight = (int)((1 - depth) * Util::SliderHeight);
            g.setColour(cDisabled);
            g.fillRect(x, y, Util::SliderWidth, disabledHeight);
            g.setColour(cEnabled);
            g.fillRect(x, y + disabledHeight, Util::SliderWidth, Util::SliderHeight - disabledHeight);
        }
        void paintLFOs(juce::Graphics& g) {
            for (auto ch = 0; ch < numChannels; ++ch) {
                const auto wave = juce::jlimit(static_cast<Float>(-.9), static_cast<Float>(.9), *lfoData[ch]);
                const auto x = static_cast<int>(Util::LFOMidX + wave * Util::LFOWidthHalf);
                g.fillRect(juce::Rectangle<int>(x, Util::LFOY, 1, Util::LFOHeight));
            }
        }
    };

    struct Button : public juce::ToggleButton {
        Button() {
            setAlpha(0);
        }
        void setCursor(float upscaleFactor) {
            const juce::MouseCursor cursor(Util::getUpscaledCursor(upscaleFactor), 0, 0);
            setMouseCursor(cursor);
        }
    };

    Space(Nel19AudioProcessor& processor, int framesPerSec, int upscale) :
        processor(processor),
        fps(framesPerSec), upscaleFactor(upscale),
        cursors(makeCursors()),
        image(juce::Image::ARGB, Width, Width, true),
        bounds(),
        bg(*this),
        ui(*this),
        shuttle(*this),
        paramText(upscale, fps),

        param{
            CSlider(processor.apvts, tape::param::getID(tape::param::VibratoDepth)),
            CSlider(processor.apvts, tape::param::getID(tape::param::VibratoFreq)),
            CSlider(processor.apvts, tape::param::getID(tape::param::VibratoWidth))
        },
        sAttach(),
        studioButton(),
        studioAttach(),

        about(upscale),
        aboutButton(),

        midiLearnButton(processor.tape, upscale),
        bufferSizeTextField(processor.apvts, tape::param::getID(tape::param::ID::VibratoDelaySize), upscaleFactor)
    {
        bg.setInterval(BgInterval);
        shuttle.setLFO((Float)ShuttleFreq, (Float)ShuttleAmp);
        for (auto p = 0; p < SlidersCount; ++p) {
            addAndMakeVisible(param[p]);
            param[p].paramText = &paramText;
        }
        param[PDepth].otherSlider = &param[PFreq];
        param[PFreq].otherSlider = &param[PDepth];
        paramText.initParameterValues(
            { param[PDepth].getValue(),
            param[PFreq].getValue(),
            param[PWidth].getValue() }
        );
        addAndMakeVisible(studioButton);
        studioAttach = std::make_unique<BAttach>(processor.apvts, tape::param::getID(tape::param::Lookahead), studioButton);
        addAndMakeVisible(about);
        addAndMakeVisible(aboutButton);
        aboutButton.addListener(this);
        about.setVisible(false);
        addAndMakeVisible(midiLearnButton);
        addAndMakeVisible(bufferSizeTextField);
        setCursors();
        setOpaque(true);
    }

    void paint(juce::Graphics& g) override {
        g.setImageResamplingQuality(juce::Graphics::ResamplingQuality::lowResamplingQuality);
        g.drawImageWithin(image, getX(), getY(), getWidth(), getHeight(), juce::RectanglePlacement::fillDestination, false);
        
        paramText.draw(g);
    }
    void buttonClicked(juce::Button* b) override {
        if(b == &aboutButton)
            about.setVisible(true);
    }
    void update() {
        if (about.isVisible()) return;
        juce::Graphics g{ image };
        g.setImageResamplingQuality(juce::Graphics::lowResamplingQuality);
        bg(g);
        shuttle(g);
        ui(g);

        paramText.updateParameters(
            { param[PDepth].getValue(),
            param[PFreq].getValue(),
            param[PWidth].getValue() }
        );
        midiLearnButton.update();

        repaint();
    }
    void resized() override {
        bounds = getLocalBounds().toFloat();
        auto y = 81 * upscaleFactor;
        auto width = (1 + Util::SliderWidth) * upscaleFactor;
        auto height = Util::SliderHeight * upscaleFactor;
        param[PDepth].setBounds(89 * upscaleFactor, y, width, height);
        param[PFreq].setBounds(98 * upscaleFactor, y, width, height);
        param[PWidth].setBounds(107 * upscaleFactor, y, width, height);
        auto x = 84 * upscaleFactor;
        y = 108 * upscaleFactor;
        width = 31 * upscaleFactor;
        height = 11 * upscaleFactor;
        studioButton.setBounds(x, y, width, height);
        about.setBounds(getLocalBounds());
        aboutButton.setBounds(
            1,
            (int)(117 * upscaleFactor),
            (int)(7 * upscaleFactor),
            (int)(10 * upscaleFactor)
        );

        midiLearnButton.setBounds(85 * upscaleFactor, 4 * upscaleFactor, 38 * upscaleFactor, 11 * upscaleFactor);
        bufferSizeTextField.setBounds(4 * upscaleFactor, 4 * upscaleFactor, 30 * upscaleFactor, 15 * upscaleFactor);
    }
private:
    Nel19AudioProcessor& processor;
    int fps, upscaleFactor;
    std::array<juce::MouseCursor, 3> cursors; // 0 = disabled, 1 = enabled, 2 = invisible
    juce::Image image;
    juce::Rectangle<float> bounds;
    Background bg;
    UI ui;
    Shuttle shuttle;
    ParameterTextFields paramText;

    std::array<CSlider, SlidersCount> param;
    std::array<std::unique_ptr<SAttach>, SlidersCount> sAttach;
    Button studioButton;
    std::unique_ptr<BAttach> studioAttach;

    AboutComponent about;
    Button aboutButton;

    MidiLearnButton<Float> midiLearnButton;
    BufferSizeTextField bufferSizeTextField;

    std::array<juce::MouseCursor, 3> makeCursors() {
        auto cursorImage = Util::getUpscaledCursor(upscaleFactor);
        const juce::MouseCursor mainCursor(cursorImage, 0, 0);

        juce::Colour green(0xff37946e); // make on hover cursor
        juce::Colour yellow(0xfffffa8f);
        for (auto y = 0; y < cursorImage.getHeight(); ++y)
            for (auto x = 0; x < cursorImage.getWidth(); ++x)
                if (cursorImage.getPixelAt(x, y) == green)
                    cursorImage.setPixelAt(x, y, yellow);

        const juce::MouseCursor hoverCursor(cursorImage, 0, 0);
        
        const juce::Image noImage(juce::Image::ARGB, 1, 1, true);
        const juce::MouseCursor noCursor(noImage, 0, 0);

        return { mainCursor, hoverCursor, noCursor };
    }
    void setCursors() {
        setMouseCursor(cursors[0]);
        for (auto& p : param)
            p.setCursors(&cursors[1], &cursors[2], &cursors[0]);
        studioButton.setMouseCursor(cursors[1]);
        aboutButton.setMouseCursor(cursors[1]);
        about.setCursor(cursors[0], cursors[1]);
        midiLearnButton.setMouseCursor(cursors[1]);
        bufferSizeTextField.setMouseCursor(cursors[1]);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Space)
};