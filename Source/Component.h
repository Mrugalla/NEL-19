#pragma once
#include <functional>

struct Utils {
    enum ColourID { Background, Normal, Interactable, Modulation, Inactive, Abort, HoverButton, Darken, EnumSize };
    struct ColourSheme {
        ColourSheme(Nel19AudioProcessor& p) :
            colour(),
            coloursID("colours")
        {
            for (auto d = 0; d < EnumSize; ++d)
                colour[d] = getDefault(d);

            auto user = p.appProperties.getUserSettings();
            if (user->isValidFile()) {
                for (auto c = 0; c < EnumSize; ++c) {
                    auto colID = "colour" + identify(c);
                    auto colAsString = user->getValue(colID);
                    if(colAsString != "")
                        colour[c] = juce::Colour::fromString(colAsString);
                    user->setValue(colID, colour[c].toString());
                }
            }
        }
        juce::Colour getDefault(const int i) const {
            switch (i) {
            case ColourID::Background: return juce::Colour(nelG::ColBlack);
            case ColourID::Normal: return juce::Colour(nelG::ColGreen);
            case ColourID::Interactable: return juce::Colour(nelG::ColYellow);
            case ColourID::Modulation: return juce::Colour(nelG::ColGreenNeon);
            case ColourID::Inactive: return juce::Colour(nelG::ColGrey);
            case ColourID::Abort: return juce::Colour(nelG::ColRed);
            case ColourID::HoverButton: return juce::Colour(0x55ffffff);
            case ColourID::Darken: return juce::Colour(0x87000000);
            default: return juce::Colour(0x00000000);
            }
        }
        juce::String identify(int i) const {
            switch (i) {
            case ColourID::Background: return "Background";
            case ColourID::Normal: return "Normal";
            case ColourID::Interactable: return "Interactable";
            case ColourID::Modulation: return "Modulation";
            case ColourID::Inactive: return "Inactive";
            case ColourID::Abort: return "Abort";
            case ColourID::HoverButton: return "HoverButton";
            case ColourID::Darken: return "Darken";
            default: return "";
            }
        }
        const juce::Colour operator[](const int i) const noexcept { return colour[i]; }
        juce::Colour& operator[](const int i) noexcept { return colour[i]; }
        void setColour(const juce::String& colourName, juce::Colour col) {
            for (auto c = 0; c < EnumSize; ++c)
                if (colourName == identify(c)) {
                    colour[c] = col;
                    return;
                }
        }
        std::array<juce::Colour, ColourID::EnumSize> colour;
        juce::Identifier coloursID;
    };

    enum Cursor { Norm, Hover, Cross };
    Utils(Nel19AudioProcessor& p) :
        cursors(),
        tooltipID("tooltip"), popUpID("popUp"),
        tooltip(nullptr), popUp(""),
        font(getCustomFont()),
        colours(p),
        tooltipEnabled(true), popUpEnabled(true), popUpUpdated(false)
    {
        // CURSOR
        const auto scale = 3;
        auto cursorImage = nelG::load(BinaryData::cursor_png, BinaryData::cursor_pngSize, scale);
        auto cursorHoverImage = cursorImage.createCopy();
        juce::Colour genericGreen(nelG::ColGreen);
        for (auto x = 0; x < cursorImage.getWidth(); ++x)
            for (auto y = 0; y < cursorImage.getHeight(); ++y)
                if (cursorImage.getPixelAt(x, y) == genericGreen) {
                    cursorImage.setPixelAt(x, y, colours[ColourID::Normal]);
                    cursorHoverImage.setPixelAt(x, y, colours[Interactable]);
                }
        const juce::MouseCursor cursor(cursorImage, 0, 0);
        juce::MouseCursor cursorHover(cursorHoverImage, 0, 0);
        cursors.push_back(cursor);
        cursors.push_back(cursorHover);
        const auto cursorCrossImage = nelG::load(BinaryData::cursorCross_png, BinaryData::cursorCross_pngSize, scale);
        const auto midPoint = 5 * scale;
        const juce::MouseCursor cursorCross(cursorCrossImage, midPoint, midPoint);
        cursors.push_back(cursorCross);

        auto user = p.appProperties.getUserSettings();
        if (user->isValidFile()) {
            // TOOL TIPS
            tooltipEnabled = user->getBoolValue(tooltipID, tooltipEnabled);
            user->setValue(tooltipID, tooltipEnabled);
            // POP UP
            popUpEnabled = user->getBoolValue(popUpID, popUpEnabled);
            user->setValue(popUpID, popUpEnabled);
        }
    }
    void switchToolTipEnabled(Nel19AudioProcessor& p) { setTooltipEnabled(p, !tooltipEnabled); }
    void setTooltipEnabled(Nel19AudioProcessor& p, bool e) {
        auto user = p.appProperties.getUserSettings();
        if (user->isValidFile()) {
            tooltipEnabled = e;
            user->setValue(tooltipID, tooltipEnabled);
        }
    }
    void switchPopUpEnabled(Nel19AudioProcessor& p) { setPopUpEnabled(p, !popUpEnabled); }
    void setPopUpEnabled(Nel19AudioProcessor& p, bool e) {
        auto user = p.appProperties.getUserSettings();
        if (user->isValidFile()) {
            popUpEnabled = e;
            user->setValue(popUpID, e);
        }
    }
    
    void save(Nel19AudioProcessor& p, const ColourID i, const juce::Colour col = juce::Colour(0x00000000)) {
        auto user = p.appProperties.getUserSettings();
        if (user->isValidFile()) {
            if (col.isTransparent())
                colours[i] = colours.getDefault(i);
            else
                colours[i] = col;
            const auto colID = "colour" + colours.identify(i);
            user->setValue(colID, colours[i].toString());
        }
    }

    void updateTooltip(juce::String* msg) {
        if (!tooltipEnabled) return;
        tooltip = msg;
    }
    void updatePopUp(const juce::String& msg) {
        if (popUp != msg) {
            popUp = msg;
            popUpUpdated = true;
        }
    }
    std::vector<juce::MouseCursor> cursors;
    juce::Identifier tooltipID, popUpID;
    juce::String* tooltip;
    juce::String popUp;
    juce::Font font;
    ColourSheme colours;
    bool tooltipEnabled, popUpEnabled, popUpUpdated;
private:
    juce::Font getCustomFont() {
        return juce::Font(juce::Typeface::createSystemTypefaceFor(BinaryData::nel19_ttf,
            BinaryData::nel19_ttfSize));
    }
};

struct Comp :
    public juce::Component
{
    Comp(Nel19AudioProcessor& p, Utils& u, const juce::String& tooltp = "", Utils::Cursor curs = Utils::Cursor::Norm) :
        processor(p),
        utils(u),
        tooltip(tooltp)
    {
        setMouseCursor(utils.cursors[curs]);
    }
    Comp(Nel19AudioProcessor& p, Utils& u, juce::String&& tooltp, Utils::Cursor curs = Utils::Cursor::Norm) :
        processor(p),
        utils(u),
        tooltip(tooltp)
    {
        setMouseCursor(utils.cursors[curs]);
    }
protected:
    Nel19AudioProcessor& processor;
    Utils& utils;
    juce::String tooltip;
    void paint(juce::Graphics& g) override {
#if DebugLayout
        g.setColour(juce::Colours::red);
        g.drawRect(getLocalBounds().toFloat(), 1);
#endif
    }

    void mouseEnter(const juce::MouseEvent&) override { utils.updateTooltip(&tooltip); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Comp)
};

struct TooltipComp :
    public Comp,
    public juce::Timer
{
    TooltipComp(Nel19AudioProcessor& p, Utils& u) :
        Comp(p, u, "Tooltips are shown here."),
        tooltip(nullptr),
        enabled(true)
    { startTimerHz(24); }
protected:
    juce::String* tooltip;
    bool enabled;

    void timerCallback() override {
        if (!utils.tooltipEnabled) {
            if (enabled) {
                startTimerHz(1);
                enabled = false;
            }
            return;
        }
        if (!enabled) {
            if (!utils.tooltipEnabled) return;
            startTimerHz(24);
            enabled = true;
            tooltip = utils.tooltip;
            repaint();
            return;
        }
        else if (tooltip == utils.tooltip) return;
        tooltip = utils.tooltip;
        repaint();
    }
    void paint(juce::Graphics& g) override {
        if (!enabled || tooltip == nullptr) return;
        g.setColour(utils.colours[Utils::Normal]);
        g.drawFittedText(*tooltip, getLocalBounds(), juce::Justification::centredLeft, 1);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TooltipComp)
};

struct BuildDateComp :
    public Comp
{
    BuildDateComp(Nel19AudioProcessor& p, Utils& u) :
        Comp(p, u, "identifies plugin version."),
        buildDate(static_cast<juce::String>(__DATE__))
    { setBufferedToImage(true); }
protected:
    juce::String buildDate;
    void paint(juce::Graphics& g) override {
        g.setColour(utils.colours[Utils::Normal]);
        g.drawFittedText(buildDate, getLocalBounds(), juce::Justification::centredRight, 1);
    }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BuildDateComp)
};

struct PopUpComp :
    public Comp,
    public juce::Timer
{
    PopUpComp(Nel19AudioProcessor& p, Utils& u, float durationInMs = 1000.f) :
        Comp(p, u),
        idx(0), duration(0)
    {
        const auto fps = 24.f;
        duration = static_cast<int>(durationInMs * fps / 1000.f);
        setInterceptsMouseClicks(false, false);
        startTimerHz(fps);
    }
protected:
    int idx, duration;
    void timerCallback() override {
        if (utils.popUpUpdated) {
            setVisible(true);
            idx = 0;
            utils.popUpUpdated = false;
        }
        if (!isVisible()) return;
        ++idx;
        if (idx > duration)
            return setVisible(false);
        const auto parent = getParentComponent();
        const auto newPos = juce::Desktop::getInstance().getMainMouseSource().getScreenPosition().toInt()
            - parent->getScreenPosition();
        const juce::Rectangle<int> newBounds(newPos.x, newPos.y, getWidth(), getHeight());
        const auto constrainedBounds = newBounds.constrainedWithin(parent->getBounds());
        setBounds(constrainedBounds);
        repaint();
    }
    void paint(juce::Graphics& g) override {
        if (!utils.popUpEnabled) return;
        const auto bounds = getLocalBounds().toFloat().reduced(nelG::Thicc);
        g.setColour(utils.colours[Utils::ColourID::Darken]);
        g.fillRoundedRectangle(bounds, nelG::Thicc);
        g.setColour(utils.colours[Utils::ColourID::Interactable]);
        g.drawFittedText(utils.popUp, getLocalBounds(), juce::Justification::centred, 2);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PopUpComp)
};

#include "Parameter.h"

#include "ParameterRandomizer.h"

#include "ModsComps.h"

#include "Visualizer.h"

#include "Menu.h"

/*

make seperate headers for each thing

utils
    more sophisticated fonts handling system for when i'll have more fonts

poup Comp
    stays at currently touched mod (because that's where the eyes look at)

*/