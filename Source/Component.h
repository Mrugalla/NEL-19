#pragma once
#include <JuceHeader.h>
#include <functional>

struct Utils {
    enum { Background, Normal, Interactable, Modulation, Inactive, Abort, SideNote, EnumSize }; // colours
    struct ColourSheme {
        ColourSheme(Nel19AudioProcessor& p) :
            colour(),
            coloursID("colours")
        {
            auto state = p.apvts.state;
            auto coloursState = state.getChildWithName(coloursID);
            if (!coloursState.isValid()) {
                colour[Background] = juce::Colour(nelG::ColBlack);
                colour[Normal] = juce::Colour(nelG::ColGreen);
                colour[Interactable] = juce::Colour(nelG::ColYellow);
                colour[Modulation] = juce::Colour(nelG::ColGreenNeon);
                colour[Inactive] = juce::Colour(nelG::ColGrey);
                colour[Abort] = juce::Colour(nelG::ColRed);
                colour[SideNote] = juce::Colour(0x55ffffff);

                coloursState = juce::ValueTree(coloursID);
                for (auto i = 0; i < EnumSize; ++i)
                    coloursState.setProperty(identify(i), static_cast<juce::int64>(colour[i].getARGB()), nullptr);
            }
            else {
                for (auto i = 0; i < EnumSize; ++i) {
                    auto prop = coloursState.getProperty(identify(i), 0).isInt64();
                    auto var = static_cast<juce::uint32>(prop);
                    colour[i] = juce::Colour(var);
                }
            }
        }
        juce::String identify(int i) const {
            switch (i) {
            case Background: return "Background";
            case Normal: return "Normal";
            case Interactable: return "Interactable";
            case Modulation: return "Modulation";
            case Inactive: return "Inactive";
            case Abort: return "Abort";
            case SideNote: return "SideNote";
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
        std::array<juce::Colour, EnumSize> colour;
        juce::Identifier coloursID;
        /*
        implement options menu
        implement sub-menu for look and feel
        implement sub-menu for colours
        every colour has https://docs.juce.com/develop/classColourSelector.html
        if a colour has changed: repaint all components
        */
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
        const auto cursorImage = nelG::load(BinaryData::cursor_png, BinaryData::cursor_pngSize, scale);
        auto cursorHoverImage = cursorImage.createCopy();
        for (auto x = 0; x < cursorImage.getWidth(); ++x)
            for (auto y = 0; y < cursorImage.getHeight(); ++y)
                if (cursorImage.getPixelAt(x, y) == colours[Normal])
                    cursorHoverImage.setPixelAt(x, y, colours[Interactable]);
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
    void switchToolTip(Nel19AudioProcessor& p) {
        auto user = p.appProperties.getUserSettings();
        if (user->isValidFile()) {
            tooltipEnabled = !user->getBoolValue(tooltipID, tooltipEnabled);
            user->setValue(tooltipID, tooltipEnabled);
        }
    }
    void switchPopUp(Nel19AudioProcessor& p) {
        auto user = p.appProperties.getUserSettings();
        if (user->isValidFile()) {
            popUpEnabled = !user->getBoolValue(popUpID, popUpEnabled);
            user->setValue(popUpID, popUpEnabled);
        }
    }

    void updatePopUp(const juce::String&& msg) {
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

struct Component :
    public juce::Component
{
    Component(Nel19AudioProcessor& p, Utils& u, const juce::String& tooltp = "", Utils::Cursor curs = Utils::Cursor::Norm) :
        processor(p),
        utils(u),
        tooltip(tooltp)
    { setMouseCursor(utils.cursors[curs]); }
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

    void mouseMove(const juce::MouseEvent&) override { utils.tooltip = &tooltip; }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Component)
};

struct TooltipComponent :
    public Component,
    public juce::Timer
{
    TooltipComponent(Nel19AudioProcessor& p, Utils& u) :
        Component(p, u, "Tooltips are shown here."),
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
        if (tooltip == nullptr) return;
        g.setColour(utils.colours[Utils::SideNote]);
        g.drawFittedText(*tooltip, getLocalBounds(), juce::Justification::centredLeft, 1);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TooltipComponent)
};

struct BuildDateComponent :
    public Component
{
    BuildDateComponent(Nel19AudioProcessor& p, Utils& u) :
        Component(p, u, "identifies plugin version.")
    { buildDate = static_cast<juce::String>(__DATE__); }
protected:
    juce::String buildDate;
    void paint(juce::Graphics& g) override {
        g.setFont(utils.font);
        g.setColour(utils.colours[Utils::SideNote]);
        g.drawFittedText(buildDate, getLocalBounds(), juce::Justification::centredRight, 1);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BuildDateComponent)
};

struct PopUpComponent :
    public Component,
    public juce::Timer
{
    PopUpComponent(Nel19AudioProcessor& p, Utils& u, float durationInMs = 1000.f) :
        Component(p, u),
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
        g.fillAll(juce::Colour(0x55000000));
        g.setColour(juce::Colour(0xbbffffff));
        g.drawFittedText(utils.popUp, getLocalBounds(), juce::Justification::centred, 2);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PopUpComponent)
};

#include "Parameter.h"

#include "ParameterRandomizer.h"

struct DropDownMenu :
    public Component
{
    class List :
        public Component
    {
        struct Entry
        {
            Entry(juce::String&& _name) :
                bounds(),
                name(_name),
                selected(false), hovered(false)
            {}
            void paint(juce::Graphics& g) {
                if (hovered) {
                    g.setColour(juce::Colour(0x22ffffff));
                    g.fillRect(bounds);
                }
                    
                if (selected)
                    g.setColour(juce::Colour(nelG::ColYellow));
                else
                    g.setColour(juce::Colour(nelG::ColGreen));
                g.drawRect(bounds.toFloat());
                g.drawFittedText(name, bounds, juce::Justification::centred, 1);
            }

            juce::Rectangle<int> bounds;
            juce::String name;
            bool selected, hovered;
        };
    public:
        std::vector<Entry> entries;
        List(Nel19AudioProcessor& p, Utils& u) :
            Component(p, u, "", Utils::Cursor::Hover),
            entries()
        {}

        void addEntry(juce::String&& entryName) {
            entries.push_back(std::move(entryName));
        }
    protected:
        void paint(juce::Graphics& g) override {
            g.fillAll(juce::Colour(nelG::ColBlack));
            g.setColour(juce::Colour(nelG::ColGreen));
            g.drawRect(getLocalBounds().toFloat());
            for (auto& entry: entries)
                entry.paint(g);
        }
        void resized() {
            const auto numEntries = static_cast<float>(entries.size());
            const auto width = static_cast<float>(getWidth());
            const auto height = static_cast<float>(getHeight());
            const auto entryHeight = height / numEntries;
            const auto x = 0.f;
            auto y = 0.f;
            for (auto e = 0; e < numEntries; ++e) {
                juce::Rectangle<float> entryArea(x, y, width, entryHeight);
                entries[e].bounds = entryArea.toNearestInt();
                y += entryHeight;
            }
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(List)
    };

    DropDownMenu(Nel19AudioProcessor& p, Utils& u, const juce::String& tooltp) :
        Component(p, u, tooltp, Utils::Cursor::Hover),
        list(p, u),
        onSelect(nullptr), onDown(nullptr),
        hovering(false), drag(false)
    {}
    void setOnSelect(const std::function<void(int)>& os) { onSelect = os; }
    void setOnDown(const std::function<void()>& od) { onDown = od; }
    void setActive(int entryIdx) noexcept {
        for (auto& entry : list.entries)
            entry.selected = false;
        list.entries[entryIdx].selected = true;
    }
    void addEntry(juce::String&& entryName) { list.addEntry(std::move(entryName)); }
    List list;
protected:
    std::function<void(int)> onSelect;
    std::function<void()> onDown;
    bool hovering, drag;
    void paint(juce::Graphics& g) override {
        if (drag) {
            g.setColour(juce::Colour(0x77ffffff));
            g.fillRect(getLocalBounds().toFloat());
        }
        if (hovering)
            g.setColour(utils.colours[Utils::Interactable]);
        else
            g.setColour(utils.colours[Utils::Normal]);
        g.drawRect(getLocalBounds().toFloat());
        g.drawFittedText("<< select", getLocalBounds(), juce::Justification::centred, 1);
    }
    void mouseEnter(const juce::MouseEvent& evt) override {
        hovering = true;
        repaint();
    }
    void mouseExit(const juce::MouseEvent& evt) override {
        hovering = false;
        repaint();
    }
    void mouseDown(const juce::MouseEvent& evt) override {
        if (onDown != nullptr) onDown();
        list.setVisible(true);
        drag = true;
        repaint();
    }
    void mouseDrag(const juce::MouseEvent& evt) override {
        const auto pos = juce::Desktop::getMousePosition() - list.getScreenPosition();
        for (auto& entry : list.entries)
            if (entry.bounds.contains(pos)) {
                if (!entry.hovered) {
                    for (auto& entry1 : list.entries)
                        entry1.hovered = false;
                    entry.hovered = true;
                    list.repaint();
                    return;
                }
            }  
    }
    void mouseUp(const juce::MouseEvent& evt) override {
        const auto selectionIdx = selectListEntry();
        list.setVisible(false);
        drag = false;
        if (evt.mouseWasDraggedSinceMouseDown() && onSelect != nullptr)
            if(selectionIdx != -1)
                onSelect(selectionIdx);
        repaint();
    }

    int selectListEntry() {
        const auto pos = juce::Desktop::getMousePosition() - list.getScreenPosition();
        for (auto e = 0; e < list.entries.size(); ++e) {
            auto& entry = list.entries[e];
            if (entry.bounds.contains(pos)) {
                for (auto& entry1 : list.entries) {
                    entry1.selected = false;
                    entry1.hovered = false;
                }
                entry.selected = true;
                return e;
            }
        }
        return -1;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DropDownMenu)
};

#include "ModsComps.h"

#include "Visualizer.h"

#include "Menu.h"

/*

make seperate headers for each thing

utils:
    - more sophisticated fonts handling system for when i'll have more fonts

poup component
    stays at currently touched mod (because that's where the eyes look at)

dropdownmenu
    looks really bad atm, fix :>



*/