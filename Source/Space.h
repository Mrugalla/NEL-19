#pragma once
#include <JuceHeader.h>
#include <array>

namespace Util {
    static constexpr double Tau = 6.28318530718;
    static constexpr int SliderWidth = 4;
    static constexpr int SliderHeight = 17;
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
        auto y = 40;
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
        auto width = cursor.getWidth() * upscaleFactor;
        auto height = cursor.getHeight() * upscaleFactor;
        juce::Image newCursor(juce::Image::ARGB, width, height, true);
        juce::Graphics g{ newCursor };
        g.setImageResamplingQuality(juce::Graphics::lowResamplingQuality);
        g.drawImage(cursor, { 0.f, 0.f, width, height });
        return newCursor;
    }
}

#include "AboutComponent.h"

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

    struct Letter { int x, width; };
    struct Font {
        static constexpr int LetterCount = 15;
        static constexpr int Height = 9;
        enum { Zero, One, Two, Three, Four, Five, Six, Seven, Eight, Nine, Start, Hz, Percent, Point, End };

        Font() :
            letter(initLetter()),
            srcImage(juce::ImageCache::getFromMemory(BinaryData::font_png, BinaryData::font_pngSize))
        {}

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
            if(percent != 100) updateNumber(g, percent, x);
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
            timeIdx(0), timeSize(fps),
            mouseInBounds(false)
        {}
        // SET
        void setPosition(juce::Point<int> p) { pos = p; }
        void initParameterValues(std::array<float, 4>& paramValues) {
            for (auto i = 0; i < lastValues.size(); ++i) lastValues[i] = paramValues[i];
        }
        void enable() { mouseInBounds = true; }
        void disable() { mouseInBounds = false; }
        // PARAM
        void updateParameters(const std::array<float, 4>& paramValues) {
            if(mouseInBounds)
                for (auto i = 0; i < lastValues.size(); ++i) {
                    if (lastValues[i] != paramValues[i]) {
                        lastValues[i] = paramValues[i];
                        curParamID = i;
                        timeIdx = 0;
                        textfields[i].updateParameterValue(paramValues[i]);
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
            studioEnabled(1), numChannels(0)
        {}

        void operator()(juce::Graphics& g) {
            g.drawImageAt(uiImage, 76, 69, false);
            g.drawImageAt(helpImage, 1, 117, false);
            updateParam(g, space.processor.paramNormalized[PDepth], 90, 87);
            updateParam(g, space.processor.paramNormalized[PFreq], 99, 87);
            checkChannelCount();
            if (numChannels == 1) g.drawImageAt(widthDisabledImage, 107, 76, false);
            else updateParam(g, space.processor.paramNormalized[PWidth], 108, 87);
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
                    widthDisabledImage = juce::ImageCache::getFromMemory(BinaryData::widthDisabled_png, BinaryData::widthDisabled_pngSize);
                    lfoData.resize(numChannels, space.processor.getLFOValue(0));
                }
                else {
                    widthDisabledImage = juce::Image();
                    lfoData.clear();
                    lfoData.reserve(2);
                    for (auto ch = 0; ch < 2; ++ch)
                        lfoData.emplace_back(space.processor.getLFOValue(ch));
                }
            }
        }
        void checkLookahead() {
            if (studioEnabled != space.processor.paramValue[3]) {
                studioEnabled = space.processor.paramValue[3];
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
                const auto x = int(1 + Util::LFOMidX + *lfoData[ch] * Util::LFOWidthHalf);
                g.fillRect(juce::Rectangle<int>(x, Util::LFOY, 1, Util::LFOHeight));
            }
        }
    };

    struct Slider : public juce::Slider {
        static constexpr double SensitiveSpeed = .002;

        Slider(ParameterTextFields& p) :
            paramText(p),
            lastDragY(0),
            dragIsStarting(false)
        {
            setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
            setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
            setAlpha(0);
            setVelocityModeParameters(3, 1, 0, false, juce::ModifierKeys::shiftModifier);
        }
        // SET
        void setCursor(float upscaleFactor) {
            const juce::MouseCursor cursor(Util::getUpscaledCursor(upscaleFactor), 0, 0);
            setMouseCursor(cursor);
        }
        void mouseEnter(const juce::MouseEvent& evt) override {
            paramText.enable();
            juce::Slider::mouseEnter(evt);
        }
    private:
        ParameterTextFields& paramText;
        int lastDragY;
        bool dragIsStarting;

        void mouseWheelMove(const juce::MouseEvent& evt, const juce::MouseWheelDetails& wheel) override {
            if (evt.mods.isShiftDown()) {
                auto y = wheel.deltaY > 0 ? 1 : -1;
                processSensitivity(y);
                return;
            }
            juce::Slider::mouseWheelMove(evt, wheel);
        }
        void startedDragging() override {
            juce::Slider::startedDragging();
            dragIsStarting = true;
        }
        void mouseDown(const juce::MouseEvent& evt) override {
            if(evt.mods.isShiftDown()) setSliderSnapsToMousePosition(false);
            else setSliderSnapsToMousePosition(true);
            juce::Slider::mouseDown(evt);
        }
        void mouseDrag(const juce::MouseEvent& evt) override {
            if (evt.mods.isShiftDown()) {
                auto distance = evt.getDistanceFromDragStartY();
                int y;
                if (dragIsStarting) {
                    dragIsStarting = false;
                    y = distance < 0 ? 1 : -1;
                } 
                else {
                    
                    if (lastDragY < distance) y = -1;
                    else y = 1;
                }
                lastDragY = distance;
                
                processSensitivity(y);
                return;
            }
            
            juce::Slider::mouseDrag(evt);
        }
        void mouseMove(const juce::MouseEvent& evt) override {
            juce::Point<int> pos = getBounds().getTopLeft() + evt.getPosition();
            paramText.setPosition(pos);
        }
        
        void processSensitivity(int y) {
            auto rangedY = y * getRange().getLength();
            auto slowTFDown = rangedY * SensitiveSpeed;
            setValue(getValue() + slowTFDown);
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

    Space(Nel19AudioProcessor& processor, int framesPerSec, float upscale) :
        processor(processor),
        fps(framesPerSec), upscaleFactor(upscale),
        image(juce::Image::ARGB, Width, Width, true),
        bounds(),
        bg(*this),
        ui(*this),
        shuttle(*this),
        paramText(upscale, fps),

        param{ paramText, paramText, paramText },
        sAttach(),
        studioButton(),
        studioAttach(),

        about(upscale),
        aboutButton()
    {
        bg.setInterval(BgInterval);
        shuttle.setLFO((Float)ShuttleFreq, (Float)ShuttleAmp);
        for (auto p = 0; p < SlidersCount; ++p) {
            addAndMakeVisible(param[p]);
            sAttach[p] = std::make_unique<SAttach>(processor.apvts, tape::param::getID(p + 1), param[p]);
        }
        addAndMakeVisible(studioButton);
        studioAttach = std::make_unique<BAttach>(processor.apvts, tape::param::getID(tape::param::Lookahead), studioButton);
        paramText.initParameterValues(processor.paramValue);
        addAndMakeVisible(about);
        addAndMakeVisible(aboutButton);
        aboutButton.addListener(this);
        about.setVisible(false);
        makeCursors();
        setOpaque(true);
    }

    void paint(juce::Graphics& g) override {
        g.setImageResamplingQuality(juce::Graphics::ResamplingQuality::lowResamplingQuality);
        g.drawImageWithin(image, getX(), getY(), getWidth(), getHeight(), juce::RectanglePlacement::fillDestination, false);
        
        paramText.draw(g);
    }
    void mouseMove(const juce::MouseEvent& evt) override { paramText.setPosition(evt.getPosition()); juce::Component::mouseMove(evt); }
    void mouseEnter(const juce::MouseEvent& evt) override { paramText.enable(); juce::Component::mouseEnter(evt); }
    void mouseExit(const juce::MouseEvent& evt) override { paramText.disable(); juce::Component::mouseExit(evt); }
    void buttonClicked(juce::Button* b) override {
        if(b == &aboutButton)
            about.setVisible(aboutButton.getToggleState());
    }
    void update() {
        if (about.isVisible()) return;
        juce::Graphics g{ image };
        g.setImageResamplingQuality(juce::Graphics::lowResamplingQuality);
        bg(g);
        shuttle(g);
        ui(g);

        paramText.updateParameters(processor.paramValue);

        repaint();
    }
    void resized() override {
        bounds = getLocalBounds().toFloat();
        auto y = 87 * upscaleFactor;
        auto width = 4 * upscaleFactor;
        auto height = 17 * upscaleFactor;
        param[0].setBounds(90 * upscaleFactor, y, width, height);
        param[1].setBounds(99 * upscaleFactor, y, width, height);
        param[2].setBounds(108 * upscaleFactor, y, width, height);
        auto x = 86 * upscaleFactor;
        y = 109 * upscaleFactor;
        width = 28 * upscaleFactor;
        height = 8 * upscaleFactor;
        studioButton.setBounds(x, y, width, height);
        about.setBounds(getLocalBounds());
        aboutButton.setBounds(
            1,
            (int)(117 * upscaleFactor),
            (int)(7 * upscaleFactor),
            (int)(10 * upscaleFactor)
        );
    }
private:
    Nel19AudioProcessor& processor;
    int fps;
    float upscaleFactor;
    juce::Image image;
    juce::Rectangle<float> bounds;
    Background bg;
    UI ui;
    Shuttle shuttle;
    ParameterTextFields paramText;

    std::array<Slider, SlidersCount> param;
    std::array<std::unique_ptr<SAttach>, SlidersCount> sAttach;
    Button studioButton;
    std::unique_ptr<BAttach> studioAttach;

    AboutComponent about;
    Button aboutButton;

    void makeCursors() {
        auto cursorImage = Util::getUpscaledCursor(upscaleFactor);
        const juce::MouseCursor mainCursor(cursorImage, 0, 0);
        setMouseCursor(mainCursor);

        juce::Colour green(0xff37946e); // make on hover cursor
        juce::Colour yellow(0xfffffa8f);
        for (auto y = 0; y < cursorImage.getHeight(); ++y)
            for (auto x = 0; x < cursorImage.getWidth(); ++x)
                if (cursorImage.getPixelAt(x, y) == green)
                    cursorImage.setPixelAt(x, y, yellow);

        const juce::MouseCursor hoverCursor(cursorImage, 0, 0);

        for (auto& p : param)
            p.setMouseCursor(hoverCursor);
        studioButton.setMouseCursor(hoverCursor);
        aboutButton.setMouseCursor(hoverCursor);
        about.setCursor(mainCursor, hoverCursor);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Space)
};

/* TO DO

Installer
    because people might not know where vst3 is

Slider
    manchmal nicht responsive
    sensitive mode
        mausklick bleibt manchmal in sensitive stecken (kp ob immernoch)
    find out if setValue() bad!

TEST
    LIVE
        gitarristen 
        synths      
    DAWS
	    cubase      CHECK 9.5, 10
	    fl          CHECK
	    ableton     
	    bitwig            
	    protools
        studio one  
*/

/* IDEAS

interpolation switch (lofi)
combine depth and frequency for easier use.
feedback?

*/