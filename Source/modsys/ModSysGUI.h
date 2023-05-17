#pragma once
#include "../PluginProcessor.h"
#include "../NELG.h"
#include "ModSys.h"
#include <array>
#include <cstdint>
#include "../FormulaParser.h"

namespace gui
{
    using Notify = std::function<bool(const int, const void*)>;
    using Params = modSys6::Params;
	using Param = modSys6::Param;
    using PID = modSys6::PID;
        
    enum NotificationType
    {
        ColourChanged,
        TooltipsEnabledChanged,
        TooltipUpdated,
        ModSelectionChanged,
        ModDialDragged,
        ModDraggerDragged,
        ParameterHovered,
        ParameterDragged,
        KillPopUp,
        EnterValue,
        EnterModDepth,
        EnterModBias,
        KillEnterValue,
        PatchUpdated,
        NumTypes
    };

    struct Events
    {
        struct Evt
        {
            Evt(Events& _events) :
                events(_events),
                notifier(nullptr)
            {
            }
            
            Evt(Events& _events, const Notify& _notifier) :
                events(_events),
                notifier(_notifier)
            {
                events.add(this);
            }
            
            Evt(Events& _events, Notify&& _notifier) :
                events(_events),
                notifier(_notifier)
            {
                events.add(this);
            }
            
            ~Evt()
            {
                events.remove(this);
            }
            
            void operator()(const int type, const void* stuff = nullptr)
            {
                events.notify(type, stuff);
            }
            
            Notify notifier;
            
            Events& getEvents() noexcept
            {
                return events;
            }
        protected:
            Events& events;
        };

        Events() :
            evts()
        {
        }
        
        void add(Evt* e)
        {
            evts.push_back(e);
        }
        
        void remove(const Evt* e)
        {
            for (auto i = 0; i < evts.size(); ++i)
                if (e == evts[i])
                {
                    evts.erase(evts.begin() + i);
                    return;
                }
        }
        
        void notify(int notificationType, const void* stuff = nullptr)
        {
            for (auto e : evts)
                if (e->notifier(notificationType, stuff))
                    return;
        }
        
    protected:
        std::vector<Evt*> evts;
    };

    enum class ColourID { Txt, Bg, Abort, Mod, Bias, Interact, Inactive, Darken, Hover, Transp, NumCols };
	static constexpr int NumCols = static_cast<int>(ColourID::NumCols);
        
    inline Colour getDefault(ColourID i) noexcept
    {
        switch (i)
        {
        case ColourID::Txt: return Colour(0xffcc5f2d);
        case ColourID::Bg: return Colour(0xffc5c9ca);
        case ColourID::Inactive: return Colour(0xff82919b);
        case ColourID::Abort: return Colour(0xffd21846);
        case ColourID::Mod: return Colour(0xff1a6a62);
        case ColourID::Bias: return Colour(0xff20a1ff);
        case ColourID::Interact: return Colour(0xff68b39d);
        case ColourID::Darken: return Colour(0xea000000);
        case ColourID::Hover: return Colour(0xff818181);
        default: return Colour(0x00000000);
        }
    }
        
    inline String toString(ColourID i)
    {
        switch (i)
        {
        case ColourID::Txt: return "text";
        case ColourID::Bg: return "background";
        case ColourID::Abort: return "abort";
        case ColourID::Mod: return "mod";
        case ColourID::Bias: return "bias";
        case ColourID::Interact: return "interact";
        case ColourID::Inactive: return "inactive";
        case ColourID::Darken: return "darken";
        case ColourID::Hover: return "hover";
        case ColourID::Transp: return "transp";
        default: return "";
        }
    }

    inline String toStringProps(ColourID i)
    {
        return "colour" + toString(i);
    }

    enum class CursorType { Default, Interact, Mod, Cross, NumCursors };

    inline int getNumRows(const String& txt) noexcept
    {
        auto rows = 1;
        for (auto i = 0; i < txt.length(); ++i)
            rows = txt[i] == '\n' ? rows + 1 : rows;
        return rows;
    }

    inline PointF boundsOf(const Font& font, const String& txt) noexcept
    {
        const auto w = font.getStringWidthFloat(txt);
            
        auto numLines = 2.f;
        for (auto t = 0; t < txt.length() - 1; ++t)
            if (txt[t] == '\n')
                ++numLines;

        const auto h = font.getHeightInPoints() * numLines;
        return { w, h };
    }

    inline void repaintWithChildren(Component* comp)
    {
        comp->repaint();
        for (auto c = 0; c < comp->getNumChildComponents(); ++c)
            repaintWithChildren(comp->getChildComponent(c));
    }
        
    struct Shared
    {
        Shared() :
            props(nullptr),
            colours(),
            tooltipDefault(""),
            tooltipsEnabled(true),
            font(Font(juce::Typeface::createSystemTypefaceFor
            (
                BinaryData::nel19_ttf, BinaryData::nel19_ttfSize))
            ),
            fontFlx(Font(juce::Typeface::createSystemTypefaceFor
            (
                BinaryData::felixhand_02_ttf, BinaryData::felixhand_02_ttfSize))
            )
        {
            fontFlx.setHeight(fontFlx.getHeight() * 1.25f);
        }
            
        void init(juce::PropertiesFile* p)
        {
            props = p;
            if (props->isValidFile())
            {
                // INIT COLOURS
                for (auto c = 0; c < colours.size(); ++c)
                {
                    const auto cID = static_cast<ColourID>(c);
                    const auto colStr = props->getValue(toStringProps(cID), getDefault(cID).toString());
                    setColour(c, Colour::fromString(colStr));
                }
                // INIT TOOLTIPS
                tooltipsEnabled = props->getBoolValue("tooltips", true);
                setTooltipsEnabled(tooltipsEnabled);
            }
        }

        bool setColour(const String& i, Colour col)
        {
            for (auto c = 0; c < colours.size(); ++c)
                if (i == colours[c].toString())
                    return setColour(c, col);
            return false;
        }
            
        bool setColour(ColourID i, Colour col) noexcept
        {
            return setColour(static_cast<int>(i), col);
        }
            
        bool setColour(int i, Colour col) noexcept
        {
            if (props->isValidFile())
            {
                colours[i] = col;
                props->setValue(toStringProps(ColourID(i)), col.toString());
                if (props->needsToBeSaved())
                {
                    props->save();
                    props->sendChangeMessage();
                    return true;
                }
            }
            return false;
        }
            
        Colour colour(ColourID i) const noexcept
        {
            return colour(static_cast<int>(i));
        }
            
        Colour colour(int i) const noexcept
        {
            return colours[i];
        }

        String getTooltipsEnabledID() const
        {
            return "tooltips";
        }
            
        bool setTooltipsEnabled(bool e)
        {
            if (props->isValidFile())
            {
                tooltipsEnabled = e;
                props->setValue(getTooltipsEnabledID(), e);
                if (props->needsToBeSaved())
                {
                    props->save();
                    props->sendChangeMessage();
                    return true;
                }
            }
            return false;
        }
            
        bool updateProperty(String&& pID, const juce::var& var)
        {
            if (props->isValidFile())
            {
                props->setValue(pID, var);
                if (props->needsToBeSaved())
                {
                    props->save();
                    props->sendChangeMessage();
                    return true;
                }
            }
            return false;
        }

        juce::PropertiesFile* props;
        std::array<Colour, static_cast<int>(ColourID::NumCols)> colours;
        String tooltipDefault;
        bool tooltipsEnabled;
        Font font, fontFlx;

        static Shared shared;
    };

    inline void visualizeGroup(Graphics& g, BoundsF bounds, float thicc)
    {
        Stroke stroke(thicc, Stroke::JointStyle::curved, Stroke::EndCapStyle::rounded);

        {
            const auto x = bounds.getX();
            const auto y = bounds.getY();
            const auto w = bounds.getWidth();
            const auto h = bounds.getHeight();

            const auto midDimen = std::min(w, h);

            const auto x0 = x;
            const auto x125 = x + .125f * midDimen;
            const auto x25 = x + .25f * midDimen;
            const auto x75 = x + w - .25f * midDimen;
            const auto x875 = x + w - .125f * midDimen;
            const auto x1 = x + w;

            const auto y0 = y;
            const auto y125 = y + .125f * midDimen;
            const auto y25 = y + .25f * midDimen;
            const auto y75 = y + h - .25f * midDimen;
            const auto y875 = y + h - .125f * midDimen;
            const auto y1 = y + h;

            Path p;
            p.startNewSubPath(x0, y25);
            p.lineTo(x0, y125);
            p.lineTo(x125, y0);
            p.lineTo(x25, y0);
            for (auto i = 1; i < 3; ++i)
            {
                const auto iF = static_cast<float>(i);
                const auto n = iF / 3.f;

                const auto nY = y0 + n * (y125 - y0);
                const auto nX = x0 + n * (x125 - x0);

                p.startNewSubPath(x0, nY);
                p.lineTo(nX, y0);
            }

            p.startNewSubPath(x875, y0);
            p.lineTo(x1, y0);
            p.lineTo(x1, y125);

            p.startNewSubPath(x1, y75);
            p.lineTo(x1, y875);
            p.lineTo(x875, y1);
            p.lineTo(x75, y1);
            for (auto i = 1; i < 3; ++i)
            {
                const auto iF = static_cast<float>(i);
                const auto n = iF / 3.f;

                const auto nY = y1 + n * (y875 - y1);
                const auto nX = x1 + n * (x875 - x1);

                p.startNewSubPath(x1, nY);
                p.lineTo(nX, y1);
            }

            for (auto i = 1; i <= 3; ++i)
            {
                const auto iF = static_cast<float>(i);
                const auto n = iF / 3.f;

                const auto nY = y1 + n * (y875 - y1);
                const auto nX = x0 + n * (x125 - x0);

                p.startNewSubPath(x0, nY);
                p.lineTo(nX, y1);
            }

            g.strokePath(p, stroke);
        }
    }

    inline void visualizeGroup(Graphics& g, String&& txt,
        BoundsF bounds, Colour col, float thicc)
    {
		Stroke stroke(thicc, Stroke::JointStyle::curved, Stroke::EndCapStyle::rounded);
        g.setColour(col);
        {
            const auto x = bounds.getX();
            const auto y = bounds.getY();
            const auto w = bounds.getWidth();
            const auto h = bounds.getHeight();

			const auto midDimen = std::min(w, h);

            const auto x0 = x;
			const auto x125 = x + .125f * midDimen;
			const auto x25 = x + .25f * midDimen;
			const auto x50 = x + .5f * w;
			const auto x75 = x + w - .25f * midDimen;
			const auto x875 = x + w - .125f * midDimen;
			const auto x1 = x + w;

			const auto y0 = y;
			const auto y125 = y + .125f * midDimen;
			const auto y25 = y + .25f * midDimen;
			const auto y75 = y + h - .25f * midDimen;
			const auto y875 = y + h - .125f * midDimen;
			const auto y1 = y + h;

			Path p;
			p.startNewSubPath(x0, y25);
			p.lineTo(x0, y125);
			p.lineTo(x125, y0);
			p.lineTo(x25, y0);
            for (auto i = 1; i < 3; ++i)
            {
                const auto iF = static_cast<float>(i);
                const auto n = iF / 3.f;

                const auto nY = y0 + n * (y125 - y0);
                const auto nX = x0 + n * (x125 - x0);

                p.startNewSubPath(x0, nY);
				p.lineTo(nX, y0);
            }

            p.startNewSubPath(x875, y0);
			p.lineTo(x1, y0);
			p.lineTo(x1, y125);

            p.startNewSubPath(x1, y75);
            p.lineTo(x1, y875);
            p.lineTo(x875, y1);
            p.lineTo(x75, y1);
            for (auto i = 1; i < 3; ++i)
            {
                const auto iF = static_cast<float>(i);
                const auto n = iF / 3.f;

                const auto nY = y1 + n * (y875 - y1);
                const auto nX = x1 + n * (x875 - x1);

                p.startNewSubPath(x1, nY);
                p.lineTo(nX, y1);
            }

            for (auto i = 1; i <= 3; ++i)
            {
                const auto iF = static_cast<float>(i);
                const auto n = iF / 3.f;

                const auto nY = y1 + n * (y875 - y1);
                const auto nX = x0 + n * (x125 - x0);

                p.startNewSubPath(x0, nY);
                p.lineTo(nX, y1);
            }

            g.strokePath(p, stroke);
                
            if (txt.isNotEmpty())
            {
                g.setFont(Shared::shared.font);
                BoundsF area
                (
                    x25, y0,
                    x50 - x25,
                    y25 - y0
                );
				g.drawFittedText(txt, area.toNearestInt(), juce::Justification::topLeft, 1);
            }
        }
    }

    inline void makeCursor(Component& c, CursorType t)
    {
        juce::Image img;
        if (t == CursorType::Cross)
            img = juce::ImageCache::getFromMemory(BinaryData::cursorCross_png, BinaryData::cursorCross_pngSize).createCopy();
        else
            img = juce::ImageCache::getFromMemory(BinaryData::cursor_png, BinaryData::cursor_pngSize).createCopy();
        {
            const Colour imgCol(0xff37946e);
            const auto col = t == CursorType::Default ?
                Shared::shared.colour(gui::ColourID::Txt) :
                t == CursorType::Mod ?
                    Shared::shared.colour(gui::ColourID::Mod) :
                    Shared::shared.colour(gui::ColourID::Interact);
            for (auto y = 0; y < img.getHeight(); ++y)
                for (auto x = 0; x < img.getWidth(); ++x)
                    if (img.getPixelAt(x, y) == imgCol)
                        img.setPixelAt(x, y, col);
        }

        static constexpr int scale = 3;
        img = img.rescaled(img.getWidth() * scale, img.getHeight() * scale, Graphics::ResamplingQuality::lowResamplingQuality);

        juce::MouseCursor cursor(img, 0, 0);
        c.setMouseCursor(cursor);
    }

    struct Utils
    {
        Utils(Component& _pluginTop, juce::PropertiesFile* props,
            Nel19AudioProcessor& _processor) :
            events(),
            pluginTop(_pluginTop),
            audioProcessor(_processor),
            thicc(1.f), dragSpeed(1.f),
            tooltip(&Shared::shared.tooltipDefault),
            selectedMod(0),
            notify(events)
        {
            Shared::shared.init(props);
        }

        void resized() noexcept
        {
            static constexpr float DragSpeed = .5f;
            const auto height = static_cast<float>(pluginTop.getHeight());
            dragSpeed = 1.f / (DragSpeed * height);
                
            auto a = std::min(pluginTop.getWidth(), pluginTop.getHeight());
            auto t = static_cast<float>(a) * .004f;
            thicc = t < 1.f ? 1.f : t;
        }
            
        void updatePatch(const ValueTree& state)
        {
            audioProcessor.suspendProcessing(true);
            audioProcessor.params.updatePatch(state);
            audioProcessor.forcePrepare();
        }

        void selectMod(int i) noexcept
        {
            selectedMod = i;
        }
            
        int getSelectedMod() const noexcept
        {
            return selectedMod;
        }

        void killEnterValue()
        {
            notify(NotificationType::KillEnterValue);
        }

        const Param& getParam(PID p) const noexcept
        {
            return audioProcessor.params(p);
        }
            
        Param& getParam(PID p) noexcept
        {
            return audioProcessor.params(p);
        }
            
        const Param& getParam(PID p, int offset) const noexcept
        {
            return audioProcessor.params(modSys6::withOffset(p, offset));
        }
            
        Param& getParam(PID p, int offset) noexcept
        {
            return audioProcessor.params(modSys6::withOffset(p, offset));
        }

        void setTooltipsEnabled(bool e)
        {
            if (Shared::shared.setTooltipsEnabled(e))
                notify(NotificationType::TooltipsEnabledChanged);
        }
            
        bool getTooltipsEnabled() const noexcept
        {
            return Shared::shared.tooltipsEnabled;
        }
            
        void setTooltip(String* newTooltip)
        {
            if (tooltip == newTooltip)
                return;
            if (newTooltip == nullptr)
                tooltip = &Shared::shared.tooltipDefault;
            else
                tooltip = newTooltip;
            notify(NotificationType::TooltipUpdated);
        }
            
        const String* getTooltip() const noexcept
        {
            return tooltip;
        }
            
        String* getTooltip() noexcept
        {
            return tooltip;
        }

        Point getWindowPos(const Component& comp) noexcept
        {
            return comp.getScreenPosition() - pluginTop.getScreenPosition();
        }
            
        Point getWindowCentre(const Component& comp) noexcept
        {
            const Point centre
            (
                comp.getWidth() / 2,
                comp.getHeight() / 2
            );
            return getWindowPos(comp) + centre;
        }
            
        Point getWindowNearby(const Component& comp) noexcept
        {
            const auto pos = getWindowCentre(comp).toFloat();
            const auto w = static_cast<float>(comp.getWidth());
            const auto h = static_cast<float>(comp.getHeight());
            const auto off = (w + h) * .5f;
            const auto pluginCentre = Point
            (
                pluginTop.getWidth(),
                pluginTop.getHeight()
            ).toFloat() * .5f;
            const LineF vec(pos, pluginCentre);
            return LineF::fromStartAndAngle(pos, off, vec.getAngle()).getEnd().toInt();
        }
            
        Bounds getWindowBounds(const Component& comp) noexcept
        {
            return comp.getScreenBounds() - pluginTop.getScreenPosition();
        }

        ValueTree& getState() noexcept
        {
            return audioProcessor.params.state;
        }

        Events events;
        Component& pluginTop;
        Nel19AudioProcessor& audioProcessor;
        float thicc, dragSpeed;
    protected:
        String* tooltip;
        int selectedMod;
        Events::Evt notify;
    };

    struct Comp :
        public Component
    {
        Comp(Utils& _utils, String&& _tooltip = "", CursorType _cursorType = CursorType::Interact) :
            utils(_utils),
            tooltip(_tooltip),
            notifyBasic(utils.events, makeNotifyBasic(this)),
            notify(utils.events),
            cursorType(_cursorType)
        {
            setBufferedToImage(true);
            makeCursor(*this, cursorType);
        }
            
        Comp(Utils& _utils, const String& _tooltip = "", CursorType _cursorType = CursorType::Interact) :
            utils(_utils),
            tooltip(_tooltip),
            notifyBasic(utils.events, makeNotifyBasic(this)),
            notify(utils.events),
            cursorType(_cursorType)
        {
            setBufferedToImage(true);
            makeCursor(*this, cursorType);
        }
            
        Comp(Utils& _utils, Notify _notify, String&& _tooltip = "", CursorType _cursorType = CursorType::Interact) :
            utils(_utils),
            tooltip(_tooltip),
            notifyBasic(utils.events, makeNotifyBasic(this)),
            notify(utils.events, _notify),
            cursorType(_cursorType)
        {
            setBufferedToImage(true);
            makeCursor(*this, cursorType);
        }
            
        Comp(Utils& _utils, Notify _notify, const String& _tooltip = "", CursorType _cursorType = CursorType::Interact) :
            utils(_utils),
            tooltip(_tooltip),
            notifyBasic(utils.events, makeNotifyBasic(this)),
            notify(utils.events, _notify),
            cursorType(_cursorType)
        {
            setBufferedToImage(true);
            makeCursor(*this, cursorType);
        }
            
        void setMouseCursor(CursorType ct = CursorType::NumCursors)
        {
            if (ct == CursorType::NumCursors)
                makeCursor(*this, cursorType);
            else
                makeCursor(*this, ct);
        }
            
        const String& getTooltip() const noexcept 
        {
            return tooltip;
        }

        virtual void updateTimer() {};

        Utils& utils;
    protected:
        String tooltip;
        Events::Evt notifyBasic, notify;
            
        void mouseEnter(const Mouse&) override
        {
            utils.setTooltip(&tooltip);
        }
            
        void mouseDown(const Mouse&) override
        {
            notifyBasic(NotificationType::KillEnterValue);
        }

        void paint(Graphics& g) override
        {
            const auto t = utils.thicc;
            const auto col = Shared::shared.colour(ColourID::Abort);
            g.setColour(col);
            g.drawRoundedRectangle(getLocalBounds().toFloat(), t, t);
        }

    private:
        const CursorType cursorType;
            
        inline Notify makeNotifyBasic(Comp* comp)
        {
            return [c = comp](int type, const void*)
            {
                if (type == NotificationType::ColourChanged)
                {
                    c->setMouseCursor();
                    repaintWithChildren(c);
                }
                if (type == NotificationType::PatchUpdated)
                {
                    repaintWithChildren(c);
                }
                return false;
            };
        }
    };

    struct BgGenComp :
        public Comp
    {
        // if you ever want to start writing this class,
        // you have to solve this before:
        // 1. knobs ticks' are filling the background to appear transparent
        // 2. buttons have no background fill yet
        BgGenComp(Utils& u) :
			Comp(u, "", CursorType::Default)
        {}
    };

    struct BlinkyBoy :
        public Timer
    {
        BlinkyBoy(Comp* _comp) :
            Timer(),
            comp(_comp),
            env(0.f), x(0.f),
            eps(1.f)
        {

        }
            
        void flash(Graphics& g, Colour col)
        {
            g.fillAll(col.withAlpha(env * env));
        }
            
        void flash(Graphics& g, const BoundsF& bounds, Colour col, float thicc)
        {
            g.setColour(col.withAlpha(env * env));
            g.fillRoundedRectangle(bounds, thicc);
        }

        void trigger(float durationInSec)
        {
            startTimerHz(60);
            env = 1.f;
            x = 1.f - 1.f / (60.f / durationInSec);
            eps = (1.f - x) * 2.f;
        }
            
    protected:
        Comp* comp;
        float env, x, eps;

        void timerCallback() override
        {
            env *= x;
            if (env < eps)
            {
                env = 0.f;
                stopTimer();
            } 
            comp->repaint();
        }
    };

    struct ImageComp :
        public Comp
    {
        ImageComp(Utils& u, String&& _tooltip, Image&& _img) :
            Comp(u, std::move(_tooltip), CursorType::Default),
            img(_img)
        {}
            
        ImageComp(Utils& u, String&& _tooltip, const char* data, const int size) :
            Comp(u, std::move(_tooltip), CursorType::Default),
            img(juce::ImageCache::getFromMemory(data, size))
        {}
            
        Image img;
            
        void paint(Graphics& g) override
        {
            const auto q = Graphics::lowResamplingQuality;
            const auto thicc = utils.thicc;
            const auto bounds = maxQuadIn(getBounds().toFloat()).reduced(thicc);
            {
                const auto w = static_cast<int>(bounds.getWidth());
                g.drawImageAt(img.rescaled(w, w, q), 0, 0, false);
            }
        }
    };

    struct Label :
        public Comp
    {
        Label(Utils& u, String&& _txt, ColourID _bgC = ColourID::Transp,
            ColourID _outlineC = ColourID::Transp, ColourID _txtC = ColourID::Txt) :
            Comp(u, "", CursorType::Default),
            font(),
            txt(_txt),
            bgC(_bgC),
            outlineC(_outlineC),
            txtC(_txtC),
            just(juce::Justification::centred)
        {
            setInterceptsMouseClicks(false, false);
        }
            
        Label(Utils& u, const String& _txt, ColourID _bgC = ColourID::Transp,
            ColourID _outlineC = ColourID::Transp, ColourID _txtC = ColourID::Txt) :
            Comp(u, "", CursorType::Default),
            font(),
            txt(_txt),
            bgC(_bgC),
            outlineC(_outlineC),
            txtC(_txtC),
            just(juce::Justification::centred)
        {
            setInterceptsMouseClicks(false, false);
        }

        void setText(const String& t)
        {
            txt = t;
            updateBounds();
        }
            
        void setText(String&& t)
        {
            setText(t);
        }
            
        const String& getText() const noexcept
        {
            return txt;
        }
           
        String& getText() noexcept
        {
            return txt;
        }
            
        void setJustifaction(Just j) noexcept
        {
            just = j;
        }
            
        Just getJustification() const noexcept
        {
            return just;
        }

        void resized() override
        {
            updateBounds();
        }

        void paint(Graphics& g) override
        {
            const auto thicc = utils.thicc;
            const auto bounds = getLocalBounds().toFloat().reduced(thicc);
            g.setFont(font);
            g.setColour(Shared::shared.colour(bgC));
            g.fillRoundedRectangle(bounds, thicc);
            g.setColour(Shared::shared.colour(outlineC));
            g.drawRoundedRectangle(bounds, thicc, thicc);
            g.setColour(Shared::shared.colour(txtC));
            g.drawFittedText
            (
                txt, getLocalBounds(), just, 1
            );
        }

        void updateBounds()
        {
            const auto height = static_cast<float>(getHeight());
            const auto minHeight = font.getHeight() * static_cast<float>(getNumRows(txt));
            const auto dif = static_cast<int>(height - minHeight);
            if (dif < 0)
            {
                const auto dif2 = dif / 2;
                const auto x = getX();
                const auto y = getY() + dif2;
                const auto w = getWidth();
                const auto h = getHeight() - dif;
                setBounds(x, y, w, h);
            }
        }

        Font font;
    protected:
        String txt;
        ColourID bgC, outlineC, txtC;
        Just just;
    };

    struct Button :
        public Comp
    {
        using OnPaint = std::function<void(Graphics&, Button&)>;
        using OnClick = std::function<void()>;

        Button(Utils& u, const String& _tooltip) :
            Comp(u, _tooltip),
            onPaint([](Graphics&, Button&){}),
            onClick(nullptr),
            state(0)
        {}
            
        Button(Utils& u, Notify _notify, const String& _tooltip) :
			Comp(u, _notify, _tooltip),
            onPaint([](Graphics&, Button&) {}),
            onClick(nullptr),
            state(0)
        {}
            
        int getState() const noexcept
        {
            return state;
        }
            
        void setState(int x) noexcept
        {
            state = x;
        }

        OnPaint onPaint;
        OnClick onClick;
    protected:
        int state;

        void paint(Graphics& g) override
        {
            onPaint(g, *this);
        }
            
        void mouseEnter(const Mouse& evt) override
        {
            Comp::mouseEnter(evt);
            repaint();
        }
            
        void mouseExit(const Mouse&) override
        {
            repaint();
        }
            
        void mouseDown(const Mouse&) override
        {
            repaint();
        }
            
        void mouseUp(const Mouse& evt) override
        {
            if (!evt.mouseWasDraggedSinceMouseDown())
                if (onClick)
                    onClick();
            repaint();
        }
    };

    inline Button::OnPaint makeTextButtonOnPaint(const String& text, Just just = Just::centred,
        int targetToggleState = -1)
    {
        return[txt = text, just, targetToggleState](Graphics& g, Button& button)
        {
            const auto thicc = button.utils.thicc;
            const auto bounds = button.getLocalBounds().toFloat().reduced(thicc);
            g.setColour(Shared::shared.colour(ColourID::Bg));
            g.fillRoundedRectangle(bounds, thicc);
                
            if (button.isMouseOver())
            {
				g.setColour(Shared::shared.colour(ColourID::Hover));
                g.fillRoundedRectangle(bounds, thicc);
                if(button.isMouseButtonDown())
                    g.fillRoundedRectangle(bounds, thicc);
            }
            g.setColour(Shared::shared.colour(ColourID::Interact));
                
            const auto state = button.getState();
            if (state == targetToggleState)
                g.drawRoundedRectangle(bounds, thicc, thicc);
            else
                visualizeGroup(g, bounds, thicc);
                
            if (txt.isNotEmpty())
            {
                g.setFont(Shared::shared.font);
                g.drawFittedText(txt, bounds.toNearestInt(), just, 1);
            }
        };
    }
        
    inline Button::OnPaint makeButtonOnPaintBrowse()
    {
        return [](Graphics& g, Button& button)
        {
            const auto thicc = button.utils.thicc;
            const auto bounds = button.getLocalBounds().toFloat().reduced(thicc);
            Colour mainCol, hoverCol(0x00000000);
            if (button.isMouseOver())
            {
                hoverCol = Shared::shared.colour(ColourID::Hover);
                if (button.isMouseButtonDown())
                    hoverCol = hoverCol.withMultipliedAlpha(2.f);
                mainCol = Shared::shared.colour(ColourID::Interact);
            }
            else
                mainCol = Shared::shared.colour(ColourID::Txt);
            g.setColour(mainCol);
            g.drawRoundedRectangle(bounds, thicc, thicc);
            const PointF rad
            (
                bounds.getWidth() * .5f,
                bounds.getHeight() * .5f
            );
            const PointF centre
            (
                bounds.getX() + rad.x,
                bounds.getY() + rad.y
            );
                
            {
                const auto x = centre.x - rad.x * .5f;
                const auto y = centre.y;
                const auto w = rad.x;
                const auto h = centre.y;
                g.setColour(Shared::shared.colour(ColourID::Bg));
                g.fillRect(x, y, w, h);
            }
                
            if (!hoverCol.isTransparent())
            {
                g.setColour(hoverCol);
                g.fillRoundedRectangle(bounds, thicc);
            }  
            {
                Path arrow;
                arrow.startNewSubPath(centre);
                arrow.lineTo(centre.x + rad.x * .25f, centre.y + rad.y * .25f);
                arrow.lineTo(centre.x + rad.x * .125f, centre.y + rad.y * .25f);
                arrow.lineTo(centre.x + rad.x * .125f, bounds.getBottom());
                arrow.lineTo(centre.x - rad.x * .125f, bounds.getBottom());
                arrow.lineTo(centre.x - rad.x * .125f, centre.y + rad.y * .25f);
                arrow.lineTo(centre.x - rad.x * .25f, centre.y + rad.y * .25f);
                arrow.closeSubPath();
                g.setColour(mainCol);
                g.strokePath(arrow, Stroke(thicc));
            }
        };
    }
        
    inline Button::OnPaint makeButtonOnPaintSave()
    {
        return [](Graphics& g, Button& button)
        {
            const auto thicc = button.utils.thicc;
            const auto bounds = button.getLocalBounds().toFloat().reduced(thicc);
            g.setColour(Shared::shared.colour(ColourID::Bg));
            g.fillRoundedRectangle(bounds, thicc);
            Colour mainCol, hoverCol(0x00000000);
            if (button.isMouseOver())
            {
                hoverCol = Shared::shared.colour(ColourID::Hover);
                if (button.isMouseButtonDown())
                    hoverCol = hoverCol.withMultipliedAlpha(2.f);
                mainCol = Shared::shared.colour(ColourID::Interact);
            }
            else
                mainCol = Shared::shared.colour(ColourID::Txt);
            g.setColour(mainCol);
            g.drawRoundedRectangle(bounds, thicc, thicc);
            const PointF rad
            (
                bounds.getWidth() * .5f,
                bounds.getHeight() * .5f
            );
            const PointF centre
            (
                bounds.getX() + rad.x,
                bounds.getY() + rad.y
            );
            if (!hoverCol.isTransparent())
            {
                g.setColour(hoverCol);
                g.fillRoundedRectangle(bounds, thicc);
            }
            {
                Path path;
                LineF arrowLine
                (
                    centre.x,
                    centre.y - rad.y * .3f,
                    centre.x,
                    centre.y + rad.y * .3f
                );
                    
                path.addArrow(arrowLine, thicc, thicc * 2.f, thicc);

                {
                    auto yUp = centre.y + rad.y * .3f;
                    auto yDown = centre.y + rad.y * .4f;
                    auto xLeft = centre.x - rad.x * .4f;
                    auto xRight = centre.x + rad.x * .4f;
                    path.startNewSubPath(xLeft, yUp);
                    path.lineTo(xLeft, yDown);
                    path.lineTo(xRight, yDown);
                    path.lineTo(xRight, yUp);
                }
                    
                g.setColour(mainCol);
                g.strokePath(path, Stroke(thicc));
            }
        };
    }
        
    inline Button::OnPaint makeButtonOnPaintDirectory()
    {
        return [](Graphics& g, Button& button)
        {
            const auto thicc = button.utils.thicc;
            const auto bounds = button.getLocalBounds().toFloat().reduced(thicc);
            g.setColour(Shared::shared.colour(ColourID::Bg));
            g.fillRoundedRectangle(bounds, thicc);
            Colour mainCol, hoverCol(0x00000000);
            if (button.isMouseOver())
            {
                hoverCol = Shared::shared.colour(ColourID::Hover);
                if (button.isMouseButtonDown())
                    hoverCol = hoverCol.withMultipliedAlpha(2.f);
                mainCol = Shared::shared.colour(ColourID::Interact);
            }
            else
                mainCol = Shared::shared.colour(ColourID::Txt);
            g.setColour(mainCol);
            g.drawRoundedRectangle(bounds, thicc, thicc);
            const PointF rad
            (
                bounds.getWidth() * .5f,
                bounds.getHeight() * .5f
            );
            const PointF centre
            (
                bounds.getX() + rad.x,
                bounds.getY() + rad.y
            );
            if (!hoverCol.isTransparent())
            {
                g.setColour(hoverCol);
                g.fillRoundedRectangle(bounds, thicc);
            }
            {
                Path path;
                const PointF margin(thicc, thicc);
                const auto margin2 = margin * 2.f;
                    
                const auto bottomLeft = bounds.getBottomLeft();
                const PointF origin
                (
                    bottomLeft.x + margin2.x,
                    bottomLeft.y - margin2.y * 2.f
                );
                path.startNewSubPath(origin);
                const auto xShift = rad.x * .4f;
                const auto yIdk = centre.x - rad.x * .1f;
                path.lineTo
                (
                    path.getCurrentPosition().x + xShift,
                    yIdk
                );
                const auto rightest = bounds.getRight() - margin2.x;
                path.lineTo
                (
                    rightest,
                    yIdk
                );
                const auto almostRightest = path.getCurrentPosition().x - xShift;
                path.lineTo
                (
                    almostRightest,
                    origin.y
                );
                path.lineTo
                (
                    origin.x,
                    origin.y
                );
                const auto topLeft = bounds.getTopLeft();
                const auto yShift = rad.y * .2f;
                const PointF upperLeftCorner
                (
                    origin.x,
                    topLeft.y + yShift * 2.f
                );
                const auto xShiftMoar = rad.x * .25f;
                path.lineTo(upperLeftCorner);
                path.lineTo
                (
                    path.getCurrentPosition().x + xShift + xShiftMoar,
                    upperLeftCorner.y
                );
                const auto xShift4000 = rad.x * .1f;
                path.lineTo
                (
                    path.getCurrentPosition().x + xShift4000,
                    yIdk - yShift
                );
                path.lineTo
                (
                    almostRightest,
                    path.getCurrentPosition().y
                );
                path.lineTo
                (
                    almostRightest,
                    yIdk
                );

                g.setColour(mainCol);
                const Stroke strokeType
                (
                    thicc,
                    Stroke::JointStyle::curved,
                    Stroke::EndCapStyle::rounded
                );
                g.strokePath(path, strokeType);
            }
        };
    }

    static constexpr float LockAlpha = .3f;

    inline void makeLockButton(Button& button, PID pID)
    {
        auto& param = button.utils.getParam(pID);
        button.setState(param.locked.load() ? 1 : 0);

        button.onPaint = [](Graphics& g, Button& btn)
        {
            const auto thicc = btn.utils.thicc;
            const auto bounds = btn.getLocalBounds().toFloat().reduced(thicc);
            const PointF centre
            (
                bounds.getX() + bounds.getWidth() * .5f,
                bounds.getY() + bounds.getHeight() * .5f
            );

            if (btn.isMouseOver())
            {
                g.setColour(Shared::shared.colour(ColourID::Hover));
                g.fillRoundedRectangle(bounds, thicc);
                if (btn.isMouseButtonDown())
                    g.fillRoundedRectangle(bounds, thicc);
                g.setColour(Shared::shared.colour(ColourID::Interact));
            }
            else
                g.setColour(Colour(0xff999999));

            BoundsF bodyArea, arcArea;
            {
                auto x = bounds.getX();
                auto w = bounds.getWidth();
                auto h = bounds.getHeight() * .6f;
                auto y = bounds.getBottom() - h;
                bodyArea.setBounds(x, y, w, h);
                g.fillRoundedRectangle(bodyArea, thicc);
            }
            {
                const PointF rad
                (
                    bounds.getWidth() * .5f - thicc,
                    bounds.getHeight() * .5f - thicc
                );
                Path arc;
                arc.addCentredArc(centre.x, centre.y, rad.x, rad.y, 0.f, -PiHalf, PiHalf, true);
                g.strokePath(arc, Stroke(thicc));
            }
        };

        button.onClick = [pID, &btn = button]()
        {
            auto& param = btn.utils.getParam(pID);
            auto state = !param.locked.load();
            param.locked.store(state);
            btn.setState(state ? 1 : 0);
        };
    }

    enum class ParameterType
    {
        Knob,
        Switch,
        RadioButton,
        NumTypes
    };

    static constexpr float SensitiveDrag = .2f;
    static constexpr float WheelDefaultSpeed = .02f;
    static constexpr float WheelInertia = .9f;

    class Paramtr :
        public Comp
    {
        static constexpr float AngleWidth = PiQuart * 3.f;
        static constexpr float AngleRange = AngleWidth * 2.f;

        using OnPaint = std::function<void(Graphics&)>;

        struct ModDial :
            public Comp
        {
            ModDial(Utils& u, Paramtr& _paramtr) :
                Comp(u, "Drag to modulate parameter. Rightclick to remove modulator.", CursorType::Mod),
                paramtr(_paramtr),
                dragY(0.f), depth(paramtr.param.modDepth[utils.getSelectedMod()].load()),
                maxThicc(0.f)
            {
                setBufferedToImage(false);
            }

            void updateDepth() noexcept
            {
                const auto mIdx = utils.getSelectedMod();
                auto& prm = paramtr.param;
                depth = prm.modDepth[mIdx].load();
            }

            void resized() override
            {
                const auto thicc = utils.thicc;
                maxThicc = static_cast<float>(getWidth()) * .5f - thicc * 2.f;
            }

            void paint(Graphics& g) override
            {
                const auto thicc = utils.thicc;
                const auto bounds = getLocalBounds().toFloat().reduced(thicc);
                if (depth == 0.f)
                    g.setColour(Shared::shared.colour(ColourID::Interact));
                else
                    g.setColour(Shared::shared.colour(ColourID::Mod));

                const auto depthThicc = thicc + maxThicc * std::abs(depth);
                g.drawEllipse(bounds.reduced(depthThicc), depthThicc);
            }

            void mouseDown(const Mouse& evt) override
            {
                dragY = evt.position.y * utils.dragSpeed;
            }

            void mouseDrag(const Mouse& evt) override
            {
                auto mms = juce::Desktop::getInstance().getMainMouseSource();
                mms.enableUnboundedMouseMovement(true, false);

                const auto dragYNew = evt.position.y * utils.dragSpeed;
                const auto sensitive = evt.mods.isShiftDown() ? SensitiveDrag : 1.f;
                const auto dragMove = (dragYNew - dragY) * sensitive;

                const auto mIdx = utils.getSelectedMod();
                auto& prm = paramtr.param;
                auto& params = utils.audioProcessor.params;

                if (evt.mods.isLeftButtonDown())
                {
                    auto md = juce::jlimit(-1.f, 1.f, prm.modDepth[mIdx].load() - dragMove);
                    params.setModDepth(prm.id, md, mIdx);
                }
                else
                {
                    auto mb = juce::jlimit(-1.f, 1.f, prm.modBias[mIdx].load() - dragMove);
                    prm.setModBias(mb, mIdx);
                }
                
                depth = prm.modDepth[mIdx].load();
                notify(NotificationType::ModDialDragged, &paramtr.param.id);
                
				dragY = dragYNew;
            }

            void mouseUp(const Mouse& evt) override
            {
                if (evt.mouseWasDraggedSinceMouseDown())
                {
                    auto mms = juce::Desktop::getInstance().getMainMouseSource();
                    const Point centre(getWidth() / 2, getHeight() / 2);
                    mms.setScreenPosition((getScreenPosition() + centre).toFloat());
                    mms.enableUnboundedMouseMovement(false, true);
                    return;
                }
                else if (evt.mods.isAltDown())
                {
                    const auto type = evt.mods.isLeftButtonDown() ?
                        NotificationType::EnterModDepth :
                        NotificationType::EnterModBias;
                    notify(type, &paramtr);
                    repaintWithChildren(getParentComponent());
                }

                repaintWithChildren(getParentComponent());
            }

            void mouseDoubleClick(const Mouse& evt) override
            {
                clearModulation(evt.mods.isRightButtonDown());
                repaintWithChildren(getParentComponent());
            }

            void mouseWheelMove(const Mouse& evt, const MouseWheel& wheel) override
            {
                if (evt.mods.isAnyMouseButtonDown())
                    return;

                const bool reversed = wheel.isReversed ? -1.f : 1.f;
                const bool isTrackPad = wheel.deltaY * wheel.deltaY < .0549316f;
                if (isTrackPad)
                    dragY = reversed * wheel.deltaY;
                else
                {
                    const auto deltaYPos = wheel.deltaY > 0.f ? 1.f : -1.f;
                    dragY = reversed * WheelDefaultSpeed * deltaYPos;
                }
                if (evt.mods.isShiftDown())
                    dragY *= SensitiveDrag;

                const auto mIdx = utils.getSelectedMod();
                auto& prm = paramtr.param;

                depth = juce::jlimit(-1.f, 1.f, prm.modDepth[mIdx].load() + dragY);
                prm.modDepth[mIdx].store(depth);
                depth = prm.modDepth[mIdx].load();
                notify(NotificationType::ModDialDragged, &paramtr.param.id);
            }

            Paramtr& paramtr;
            float dragY, depth;

        protected:
            float maxThicc;

            void clearModulation(bool justBias) noexcept
            {
                const auto mIdx = utils.getSelectedMod();
                auto& prm = paramtr.param;
				auto& params = utils.audioProcessor.params;
                if(!justBias)
                    params.setModDepth(prm.id, 0.f, mIdx);
                prm.setModBias(.5f, mIdx);
                depth = prm.modDepth[mIdx].load();
            }
        };

        inline Notify makeNotify(Paramtr& parameter)
        {
            return [&p = parameter](int t, const void*)
            {
                if (t == NotificationType::ModSelectionChanged)
                {
                    p.modDial.updateDepth();
                    p.modDial.repaint();
                }
                if (t == NotificationType::ModDraggerDragged)
                {
					p.modDial.updateDepth();
					p.modDial.repaint();
                }
                return false;
            };
        }
        
    public:
        Paramtr(Utils& u, String&& _name, String&& _tooltip, PID _pID,
            std::vector<Paramtr*>& modulatables, ParameterType _pType = ParameterType::Knob) :
            Comp(u, makeNotify(*this), std::move(_tooltip)),
            onPaint(nullptr),
            targetToggleState(-1),
            param(u.getParam(_pID)),
            pType(_pType),
            label(u, std::move(_name)),
            modDial(u, *this),
            lockr(u, toString(getPID())),
            attachedModSelected(u.getSelectedMod() == param.attachedMod),
            valNorm(0.f), valSum(0.f), modDepth(0.f),
            dragY(0.f)
        {
            init(modulatables);
        }

        PID getPID() const noexcept
        {
            return param.id;
        }
            
        void updateTimer() override
        {
            modDial.updateTimer();
            lockr.updateTimer();

            const auto locked = param.locked.load();
            const auto nAlpha = locked ? LockAlpha : 1.f;
            setAlpha(nAlpha);

            switch (pType)
            {
            case ParameterType::Knob: return callbackKnob();
            case ParameterType::Switch: return callbackSwitch();
			case ParameterType::RadioButton: return callbackRadioButton();
            }
        }
            
        OnPaint onPaint;
        int targetToggleState;
        Param& param;
        const ParameterType pType;
        Label label;
        ModDial modDial;
        Button lockr;
        bool attachedModSelected;
        float valNorm, valSum, modDepth, modBias, dragY;

        void init(std::vector<Paramtr*>& modulatables)
        {
            makeLockButton(lockr, getPID());

            setName(label.getText());
            if (pType == ParameterType::Knob)
            {
                addAndMakeVisible(label);
                label.font = Shared::shared.fontFlx.withHeight(20.f);
                modulatables.push_back(this);
                addAndMakeVisible(modDial);
            }
            addAndMakeVisible(lockr);
        }
            
        void callbackKnob()
        {
            const auto vn = param.getValue();
            const auto vs = param.getValueSum();
            const auto mIdx = utils.getSelectedMod();
            const auto md = param.modDepth[mIdx].load();
            const auto mb = param.modBias[mIdx].load();
            if (valNorm != vn || valSum != vs || modDepth != md || modBias != mb)
            {
                valNorm = vn;
                valSum = vs;
                modDepth = md;
				modBias = mb;
                repaint();
            }
        }
            
        void callbackSwitch()
        {
            const auto vn = param.getValue();
            if (valNorm != vn)
            {
                valNorm = vn;
                repaint();
            }
        }

        void callbackRadioButton()
        {
            const auto vn = param.getValue();
            if (valNorm != vn)
            {
                valNorm = vn;
                repaint();
            }
        }

        void paint(Graphics& g) override
        {
            g.setFont(Shared::shared.fontFlx);
                
            switch (pType)
            {
            case ParameterType::Knob: return paintKnob(g);
            case ParameterType::Switch: return paintSwitch(g);
			case ParameterType::RadioButton: return paintRadioButton(g);
            }
        }
            
        void paintKnob(Graphics& g)
        {
            const auto thicc = utils.thicc;
            const auto thicc2 = thicc * 2.f;
            const auto thicc4 = thicc * 4.f;
            const auto bounds = maxQuadIn(getLocalBounds().toFloat()).reduced(thicc2);
            const Stroke strokeType(thicc, Stroke::JointStyle::curved, Stroke::EndCapStyle::rounded);
            const auto radius = bounds.getWidth() * .5f;
            const auto radiusInner = radius - thicc2;
            PointF centre
            (
                radius + bounds.getX(),
                radius + bounds.getY()
            );

            //draw outline
            {
                g.setColour(Shared::shared.colour(ColourID::Interact));
                Path outtaArc;

                outtaArc.addCentredArc
                (
                    centre.x, centre.y,
                    radius, radius,
                    0.f,
                    -AngleWidth, AngleWidth,
                    true
                );
                outtaArc.addCentredArc
                (
                    centre.x, centre.y,
                    radiusInner, radiusInner,
                    0.f,
                    -AngleWidth, AngleWidth,
                    true
                );

                g.strokePath(outtaArc, strokeType);
            }

            const auto valNormAngle = valNorm * AngleRange;
            const auto valAngle = -AngleWidth + valNormAngle;
            const auto radiusExt = radius + thicc;

            // draw modulation
            {
                if (modBias != .5f)
                {
                    const auto biasAngle = -AngleWidth + modBias * AngleRange;
                    const auto biasTick = LineF::fromStartAndAngle(centre, radiusExt - thicc4, biasAngle);
                    g.setColour(Shared::shared.colour(ColourID::Bias));
                    g.drawLine(biasTick.withShortenedStart(radiusInner - thicc4), thicc2);
                }

                const auto modCol = Shared::shared.colour(ColourID::Mod);
                const auto valSumAngle = valSum * AngleRange;
                const auto sumAngle = -AngleWidth + valSumAngle;
                const auto sumTick = LineF::fromStartAndAngle(centre, radiusExt, sumAngle);
                const auto sumTickLine = sumTick.withShortenedStart(radiusInner);
                g.setColour(Shared::shared.colour(ColourID::Bg));
                g.drawLine(sumTickLine, thicc4);
                g.setColour(modCol);
                g.drawLine(sumTickLine, thicc2);

                const auto mdAngle = juce::jlimit(-AngleWidth, AngleWidth, valAngle + modDepth * AngleRange);
                {
                    Path modPath;
                    modPath.addCentredArc
                    (
                        centre.x, centre.y,
                        radius, radius,
                        0.f,
                        valAngle, mdAngle,
                        true
                    );
                    g.strokePath(modPath, strokeType);
                }
            }
            // draw tick
            {
                const auto tickLine = juce::Line<float>::fromStartAndAngle(centre, radiusExt, valAngle);
                const auto tickLineX = tickLine.withShortenedStart(radiusInner);
                g.setColour(Shared::shared.colour(ColourID::Bg));
                g.drawLine(tickLineX, thicc4);
                g.setColour(Shared::shared.colour(ColourID::Interact));
                g.drawLine(tickLineX, thicc2);
            }
        }
            
        void paintSwitch(Graphics& g)
        {
            const auto thicc = utils.thicc;
            const auto bounds = getLocalBounds().toFloat().reduced(thicc);
                
			auto col = Shared::shared.colour(ColourID::Hover);
                
            if (isMouseOverOrDragging())
            {
                g.setColour(col);
                g.fillRoundedRectangle(bounds, thicc);
            }
                
            const auto valNormInt = static_cast<int>(std::round(valNorm));
            const auto active = valNormInt == targetToggleState;
            col = Shared::shared.colour(ColourID::Interact);
                
            if (active)
            {
                g.setColour(col);
                g.drawRoundedRectangle(bounds, thicc, thicc);
            }  
            else
                visualizeGroup(g, "", bounds, col, thicc);
                
            if(onPaint == nullptr)
                g.drawFittedText
                (
                    param.getCurrentValueAsText(),
                    bounds.toNearestInt(),
                    Just::centred,
                    1
                );
			else
				onPaint(g);
        }

        void paintRadioButton(Graphics& g)
        {
            const auto thicc = utils.thicc;
            const auto bounds = getLocalBounds().toFloat();
            
            const auto boundsText = bounds.withHeight(bounds.getHeight() * .5f);
            const auto boundsBox = maxQuadIn(bounds.withY(boundsText.getBottom()).withHeight(boundsText.getHeight())).reduced(thicc);

            const auto& range = param.range;
            const auto start = range.start;
            
            const auto stateOffset = start + targetToggleState;
            const auto stateF = static_cast<float>(stateOffset);

			const auto valDenormInt = static_cast<int>(std::round(range.convertFrom0to1(valNorm)));
            
            if (isMouseOverOrDragging())
            {
                g.setColour(Shared::shared.colour(ColourID::Hover));
                g.fillEllipse(boundsBox);
            }

            g.setColour(Shared::shared.colour(ColourID::Interact));

            if (valDenormInt == stateOffset)
            {
                g.fillEllipse(boundsBox);
            }
            else
            {
                g.drawEllipse(boundsBox, thicc);
            }

			g.drawFittedText
            (
                param.valToStr(stateF),
                boundsText.toNearestInt(),
                Just::centred,
                1
            );
        }
        
        void resized() override
        {
            switch (pType)
            {
            case ParameterType::Knob: return resizedKnob();
            case ParameterType::Switch: return resizedSwitch();
            case ParameterType::RadioButton: return resizedRadioButton();
            }
        }
            
        void resizedKnob()
        {
            const auto thicc = utils.thicc;
            const auto bounds = getLocalBounds().toFloat().reduced(thicc);
            label.setBounds(bounds.toNearestInt());
            {
                const auto w = bounds.getWidth() * .33333333f;
                const auto h = bounds.getHeight() * .33333333f;
                const auto x = bounds.getX() + (bounds.getWidth() - w) * .5f;
                const auto y = bounds.getY() + (bounds.getHeight() - h);
                const BoundsF dialArea(x, y, w, h);
                modDial.setBounds(maxQuadIn(dialArea).toNearestInt());
            }
            {
                const auto w = static_cast<float>(modDial.getWidth()) * Pi * .25f;
                const auto h = w;
                const auto x = static_cast<float>(getWidth()) - w;
                const auto y = 0.f;
                lockr.setBounds(BoundsF(x, y, w, h).toNearestInt());
            }
        }
            
        void resizedSwitch()
        {
            const auto w = std::min(getWidth(), getHeight()) / 3;
            const auto h = w;
            const auto x = getWidth() - w;
            const auto y = 0;
            lockr.setBounds(x, y, w, h);
        }

		void resizedRadioButton()
		{
            const auto w = std::min(getWidth(), getHeight()) / 3;
            const auto h = w;
            const auto x = getWidth() - w;
            const auto y = 0;
            lockr.setBounds(x, y, w, h);
		}

        void mouseEnter(const Mouse& evt) override
        {
            Comp::mouseEnter(evt);
            notify(NotificationType::ParameterHovered, this);
            if (pType != ParameterType::Knob)
                repaint();
        }
            
        void mouseExit(const Mouse&) override
        {
            if (pType != ParameterType::Knob)
                repaint();
        }
            
        void mouseDown(const Mouse& evt) override
        {
            switch (pType)
            {
            case ParameterType::Knob:
                if (evt.mods.isLeftButtonDown())
                {
                    notify(NotificationType::KillEnterValue);
                    param.beginGesture();
                    dragY = evt.position.y * utils.dragSpeed;
                }
                return;
            case ParameterType::Switch:
                notify(NotificationType::KillEnterValue);
                return repaint();
            }
        }
            
        void mouseDrag(const Mouse& evt) override
        {
            if (pType != ParameterType::Knob)
                return;
                
            if (evt.mods.isLeftButtonDown())
            {
                auto mms = juce::Desktop::getInstance().getMainMouseSource();
                mms.enableUnboundedMouseMovement(true, false);
                    
                const auto dragYNew = evt.position.y * utils.dragSpeed;
                auto dragOffset = dragYNew - dragY;
                if (evt.mods.isShiftDown())
                    dragOffset *= SensitiveDrag;
                const auto newValue = juce::jlimit(0.f, 1.f, param.getValue() - dragOffset);
                param.setValueNotifyingHost(newValue);
                dragY = dragYNew;
                notify(NotificationType::ParameterDragged, this);
            }
        }
            
        void mouseUp(const Mouse& evt) override
        {
            switch (pType)
            {
            case ParameterType::Knob: return mouseUpKnob(evt);
            case ParameterType::Switch: return mouseUpSwitch(evt);
			case ParameterType::RadioButton: return mouseUpRadioButton(evt);
            }
        }
            
        void mouseWheelMove(const Mouse& evt, const MouseWheel& wheel) override
        {
            if (evt.mods.isAnyMouseButtonDown())
                return;
                
            if (pType == ParameterType::Knob)
            {
                const bool reversed = wheel.isReversed ? -1.f : 1.f;
                const bool isTrackPad = wheel.deltaY * wheel.deltaY < .0549316f;
                if (isTrackPad)
                    dragY = reversed * wheel.deltaY;
                else
                {
                    const auto deltaYPos = wheel.deltaY > 0.f ? 1.f : -1.f;
                    dragY = reversed * WheelDefaultSpeed * deltaYPos;
                }
                if (evt.mods.isShiftDown())
                    dragY *= SensitiveDrag;
                const auto newValue = juce::jlimit(0.f, 1.f, param.getValue() + dragY);
                param.setValueWithGesture(newValue);
            }
            else if (pType == ParameterType::Switch)
            {
                const auto newValue = param.getValue() + (wheel.deltaY > 0.f ? 1.f : -1.f);
                param.setValueWithGesture(juce::jlimit(0.f, 1.f, newValue));
            }
                
            notify(NotificationType::ParameterDragged, this);
        }

        void mouseDoubleClick(const Mouse&) override
        {
            if (pType == ParameterType::Knob)
            {
                param.setValueWithGesture(param.getDefaultValue());
                notify(NotificationType::ParameterDragged, this);
            }
        }

        void mouseUpKnob(const Mouse& evt)
        {
            if (evt.mods.isLeftButtonDown())
            {
                if (!evt.mouseWasDraggedSinceMouseDown())
                {
                    if (evt.mods.isCtrlDown())
                        param.setValueNotifyingHost(param.getDefaultValue());
                    else if (evt.mods.isAltDown())
                        notify(NotificationType::EnterValue, this);
                }
                else
                {
                    auto mms = juce::Desktop::getInstance().getMainMouseSource();
                    const Point centre(getWidth() / 2, getHeight() / 2);
                    mms.setScreenPosition((getScreenPosition() + centre).toFloat());
                    mms.enableUnboundedMouseMovement(false, true);
                }
                param.endGesture();
                notify(NotificationType::ParameterDragged, this);
            }
            else if (evt.mods.isRightButtonDown())
                if (!evt.mouseWasDraggedSinceMouseDown())
                    if (evt.mods.isCtrlDown())
                        param.setValueWithGesture(param.getDefaultValue());
                    else
                        notify(NotificationType::EnterValue, this);
        }

        void mouseUpSwitch(const Mouse& evt)
        {
            if (!evt.mods.isLeftButtonDown())
                return;

            if (!evt.mouseWasDraggedSinceMouseDown())
                if (evt.mods.isCtrlDown())
                    param.setValueWithGesture(param.getDefaultValue());
                else
                    param.setValueWithGesture(1.f - param.getValue());
            notify(NotificationType::ParameterDragged, this);
        }

        void mouseUpRadioButton(const Mouse& evt)
        {
            if (evt.mouseWasDraggedSinceMouseDown())
                return;
            
            if (!evt.mods.isLeftButtonDown())
                return;
            
            if (evt.mods.isCtrlDown())
                param.setValueWithGesture(param.getDefaultValue());
            else
            {
                auto& range = param.range;
                auto start = range.start;
                auto stateOffset = static_cast<float>(start + targetToggleState);
                auto nVal = range.convertTo0to1(stateOffset);
                param.setValueWithGesture(nVal);
            }

            notify(NotificationType::ParameterDragged, this);
        }
    };

    struct ModDragger :
        public Comp
    {
        inline Notify makeNotify(ModDragger& modDragger, Utils& _utils)
        {
            return [&m = modDragger, &u = _utils](int t, const void*)
            {
                if (t == NotificationType::ModSelectionChanged)
                {
                    const auto mIdx = m.mIdx;
                    const bool selected = mIdx == u.getSelectedMod();
                    m.selected = selected;
                    m.repaint();
                }
                return false;
            };
        }

        ModDragger(Utils& u, int _mIdx, std::vector<Paramtr*>& _modulatables) :
            Comp(u, makeNotify(*this, u), "Drag this modulator to any parameter.", CursorType::Mod),
            mIdx(_mIdx),
            hoveredParameter(nullptr),
            modulatables(_modulatables),
            origin(),
            draggerfall(),
            selected(mIdx == u.getSelectedMod()),
            dragging(false)
        {}
            
        void setQBounds(const BoundsF& b)
        {
            origin = maxQuadIn(b).toNearestInt();
            setBounds(origin);
        }
            
        void paint(Graphics& g) override
        {
            auto col = Shared::shared.colour(ColourID::Mod);
            if(!selected)
                col = Shared::shared.colour(ColourID::Inactive);
            else if (dragging && hoveredParameter == nullptr)
                col = col.withMultipliedSaturation(.5f);
            
            g.setColour(col);

            const auto thicc = utils.thicc;
            const auto tBounds = getLocalBounds().toFloat();
            PointF centre
            (
                tBounds.getX() + tBounds.getWidth() * .5f,
                tBounds.getY() + tBounds.getHeight() * .5f
            );
            const auto arrowHead = tBounds.getWidth() * .25f;
            g.drawArrow(LineF(centre, { centre.x, tBounds.getBottom() }), thicc, arrowHead, arrowHead);
            g.drawArrow(LineF(centre, { tBounds.getX(), centre.y }), thicc, arrowHead, arrowHead);
            g.drawArrow(LineF(centre, { centre.x, tBounds.getY() }), thicc, arrowHead, arrowHead);
            g.drawArrow(LineF(centre, { tBounds.getRight(), centre.y }), thicc, arrowHead, arrowHead);
        }

        void mouseDown(const Mouse& evt) override
        {
            utils.selectMod(mIdx);
            notify(NotificationType::ModSelectionChanged);
            draggerfall.startDraggingComponent(this, evt);
        }

        void mouseDrag(const Mouse& evt) override
        {
            draggerfall.dragComponent(this, evt, nullptr);
            hoveredParameter = getHoveredParameter();
            dragging = true;
            repaint();
        }

        void mouseUp(const Mouse&) override
        {
            if (hoveredParameter != nullptr)
            {
                const auto pID = hoveredParameter->getPID();
                auto& param = utils.getParam(pID);
                const auto pValue = param.getValue();
                const auto depth = 1.f - pValue;
                auto& params = utils.audioProcessor.params;
                params.setModDepth(pID, depth, mIdx);
				notify(NotificationType::ModDraggerDragged);
                hoveredParameter = nullptr;
            }
            setBounds(origin);
            dragging = false;
            repaint();
        }

    protected:
        Paramtr* hoveredParameter;
        std::vector<Paramtr*>& modulatables;
        Bounds origin;
        juce::ComponentDragger draggerfall;
        bool dragging;

        Paramtr* getHoveredParameter() const noexcept
        {
            for (const auto p : modulatables)
                if (p->isShowing())
                {
                    const auto pID = p->getPID();
                    const auto& param = utils.getParam(pID);
                    if (!param.locked.load())
                        if (!utils.getWindowBounds(*this).getIntersection(utils.getWindowBounds(*p)).isEmpty())
                            return p;
                }
                    
            return nullptr;
        }
    public:
        const int mIdx;
        bool selected;
    };

    struct Tooltip :
        public Comp
    {
        inline Notify makeNotify(Tooltip& ctt)
        {
            return [&c = ctt](int t, const void*)
            {
                if (t == NotificationType::TooltipsEnabledChanged ||
                    t == NotificationType::TooltipUpdated)
                {
                    c.tooltipsUpdated();
                    return true;
                }
                return false; 
            };
        }

        Tooltip(Utils& u) :
            Comp
            (
                u,
                makeNotify(*this),
                "This component literally displays this very tooltip.",
                CursorType::Default
            ),
            tooltipPtr(utils.getTooltip())
        { 
        }
            
        void tooltipsUpdated()
        {
            if (Shared::shared.tooltipsEnabled)
            {
                tooltipPtr = utils.getTooltip();
                return repaint();
            }
            else
            {
                if (tooltipPtr == &Shared::shared.tooltipDefault)
                    return;
                tooltipPtr = &Shared::shared.tooltipDefault;
                return repaint();
            }
        }
            
    protected:
        String* tooltipPtr;

        void paint(Graphics& g) override
        {
            g.setFont(Shared::shared.fontFlx.withHeight(24.f));
            g.setColour(Shared::shared.colour(ColourID::Txt));
            g.drawFittedText
            (
                *tooltipPtr, getLocalBounds(), juce::Justification::bottomLeft, 1
            );
        }
    };

    class PopUp :
        public Comp
    {
        static constexpr float FreezeTimeInMs = 420.f;
        static constexpr int FreezeTime = static_cast<int>(FreezeTimeInMs / 1000.f * 60.f);

        inline Notify makeNotify(PopUp& popUp)
        {
            return [&p = popUp](int t, const void* stuff)
            {
                if (t == NotificationType::ParameterHovered)
                {
                        
                    const auto& paramtr = *static_cast<const Paramtr*>(stuff);
					const auto pID = paramtr.getPID();
					const auto& param = paramtr.utils.getParam(pID);
                    const auto valTxt = param.getCurrentValueAsText();
                    if (valTxt.isNotEmpty())
                    {
                        const auto pt = p.utils.getWindowNearby(paramtr);
                        p.update(paramtr.getName() + "\n" + valTxt, pt);
                        return true;
                    }
                    p.kill();
                    return true;
                }
                if (t == NotificationType::ParameterDragged)
                {
                    const auto& paramtr = *static_cast<const Paramtr*>(stuff);
                    const auto pID = paramtr.getPID();
                    const auto& param = paramtr.utils.getParam(pID);
                    const auto valTxt = param.getCurrentValueAsText();
                    if (valTxt.isNotEmpty())
                    {
                        p.update(paramtr.getName() + "\n" + valTxt);
                        return true;
                    }
                    p.kill();
                    return true;
                }
                if (t == NotificationType::ModDialDragged)
                {
                    const auto pID = *static_cast<const PID*>(stuff);
                    const auto& param = p.utils.getParam(pID);
                    const auto valSumDenorm = param.getValSumDenorm();
                    const auto valTxt = param.valToStr(valSumDenorm);
                    if (valTxt.isNotEmpty())
                    {
                        p.update(modSys6::toString(pID) + "\n" + valTxt);
                        return true;
                    }
                    p.kill();
                    return true;
                }
                if (t == NotificationType::KillPopUp)
                {
                    p.kill();
                    return true;
                }
                return false;
            };
        }

    public:
        PopUp(Utils& u) :
            Comp(u, makeNotify(*this), "", CursorType::Default),
            label(u, "", ColourID::Darken, ColourID::Transp, ColourID::Txt),
            freezeIdx(0)
        {
            label.font = Shared::shared.fontFlx.withHeight(18.f);
            setInterceptsMouseClicks(false, false);
            addChildComponent(label);
        }
            
        void update(String&& txt, Point pt)
        {
            setCentrePosition(pt);
            update(std::move(txt));
        }
            
        void update(const String& txt)
        {
            label.setText(txt);
            label.setVisible(true);
            freezeIdx = 0;
            const auto txtBounds = boundsOf(label.font, label.getText()).toInt();
            setSize(txtBounds.x, txtBounds.y);
            label.repaint();
        }
            
        void kill()
        {
            freezeIdx = FreezeTime + 1;
            label.setVisible(false);
        }
            
        void paint(Graphics& g) override
        {
            if (!label.isVisible())
                return;
            g.setColour(Shared::shared.colour(ColourID::Darken));
            const auto t = utils.thicc;
            g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(t), t);
        }

        void resized() override
        {
            label.setBounds(getLocalBounds());
        }

        void updateTimer() override
        {
            if (freezeIdx > FreezeTime)
            {
                label.setVisible(false);
                return;
            }
            ++freezeIdx;
        }

    protected:
        Label label;
        int freezeIdx;
    };

    class EnterValueComp :
        public Comp
    {
        enum class ValueType { Value, Mod, Bias, NumTypes };

        inline Notify makeNotify(EnterValueComp& comp)
        {
            return [&evc = comp](int type, const void* stuff)
            {
                if (type == NotificationType::EnterValue)
                {
                    const auto param = static_cast<const Paramtr*>(stuff);
                    const auto pID = param->getPID();
                    evc.enable(pID, evc.utils.getWindowNearby(*param), ValueType::Value);
                    return true;
                }
                if (type == NotificationType::EnterModDepth)
                {
                    const auto param = static_cast<const Paramtr*>(stuff);
                    const auto pID = param->getPID();
                    evc.enable(pID, evc.utils.getWindowNearby(*param), ValueType::Mod);
                    return true;
                }
                if (type == NotificationType::EnterModBias)
                {
                    const auto param = static_cast<const Paramtr*>(stuff);
                    const auto pID = param->getPID();
                    evc.enable(pID, evc.utils.getWindowNearby(*param), ValueType::Bias);
                    return true;
                }
                if (type == NotificationType::KillEnterValue)
                {
                    evc.disable();
                    return true;
                }
                return false;
            };
        }
    
    public:    
        EnterValueComp(Utils& u) :
            Comp(u, makeNotify(*this), "Enter a new value for this parameter."),
            txt(""),
            param(nullptr),
            initValue(0.f),
            tickIdx(0),
            drawTick(false),
            timerUpdateIdx(0),
            timerRunning(false),
            valueType(ValueType::Value)
        {
            setWantsKeyboardFocus(true);
        }
            
        void enable(PID pID, Point pt, ValueType _valueType)
        {
            setCentrePosition(pt);
            param = &utils.getParam(pID);
            valueType = _valueType;
            switch (valueType)
            {
			case ValueType::Value:
                initValue = param->getValue();
				break;
            case ValueType::Mod:
                initValue = param->modDepth[utils.getSelectedMod()].load();
                break;
			case ValueType::Bias:
				initValue = param->modBias[utils.getSelectedMod()].load();
                break;
            }
            txt = "";
            enable();
        }
            
        void enable()
        {
            setVisible(true);
            tickIdx = txt.length();
            drawTick = true;
            // adjust bounds
            grabKeyboardFocus();
            timerRunning = true;
            repaint();
        }
            
        bool isEnabled() const noexcept
        {
            return timerRunning;
        }
            
        void disable()
        {
            timerRunning = false;
            setVisible(false);
        }
            
        bool keyPressed(const juce::KeyPress& key) override
        {
            if (key == key.escapeKey)
            {
                disable();
                return true;
            }
            if (key == key.returnKey)
            {
                const auto valNorm = param->getValueForText(txt);
                const auto mIdx = utils.getSelectedMod();
                const auto value = param->getValue();
                const auto modDepth = valNorm - value;
                auto params = utils.audioProcessor.params;
                fx::Parser parse;

                switch (valueType)
                {
                case ValueType::Value:
                    param->setValueWithGesture(valNorm);
                    break;
                case ValueType::Mod:
                    params.setModDepth(param->id, modDepth, mIdx);
                    break;
                case ValueType::Bias:
                    if(parse(txt))
                        param->setModBias(parse(0.f), mIdx);
                    break;
                }
                
                disable();
                return true;
            }
            if (key == key.leftKey)
            {
                if (tickIdx > 0)
                    --tickIdx;
                drawTick = true;
                repaint();
                return true;
            }
            if (key == key.rightKey)
            {
                if (tickIdx < txt.length())
                    ++tickIdx;
                drawTick = true;
                repaint();
                return true;
            }
            if (key == key.backspaceKey)
            {
                txt = txt.substring(0, tickIdx - 1) + txt.substring(tickIdx);
                if (tickIdx > 0)
                    --tickIdx;
                drawTick = true;
                repaint();
                return true;
            }
            if (key == key.deleteKey)
            {
                txt = txt.substring(0, tickIdx) + txt.substring(tickIdx + 1);
                drawTick = true;
                repaint();
                return true;
            }
            const auto chr = key.getTextCharacter();
            txt = txt.substring(0, tickIdx) + chr + txt.substring(tickIdx);
            ++tickIdx;
            drawTick = true;
            repaint();
            return true;
        }

        void updateTimer() override
        {
            if (!timerRunning)
                return;

            if (!hasKeyboardFocus(true))
                return disable();

            ++timerUpdateIdx;
            if (timerUpdateIdx < 15)
                return;
            timerUpdateIdx = 0;

            drawTick = !drawTick;
            repaint();
        }

        void paint(Graphics& g) override
        {
            const auto thicc = utils.thicc;
            const auto bounds = getLocalBounds().toFloat().reduced(thicc);
            g.setColour(Shared::shared.colour(ColourID::Bg));
            g.fillRoundedRectangle(bounds, thicc);
            g.setColour(Shared::shared.colour(ColourID::Txt));
            g.drawRoundedRectangle(bounds, thicc, thicc);
            if (drawTick)
                g.drawFittedText
                (
                    txt.substring(0, tickIdx) + "| " + txt.substring(tickIdx),
                    bounds.toNearestInt(), Just::centred, 1
                );
            else
                g.drawFittedText
                (
                    txt, bounds.toNearestInt(), Just::centred, 1
                );
        }
    protected:
        String txt;
        Param* param;
        float initValue;
        int tickIdx;
        bool drawTick;
        ValueType valueType;

        int timerUpdateIdx;
        bool timerRunning;
    };

    struct BuildDate :
        public Comp
    {
        BuildDate(Utils& u) :
            Comp(u, "identifies the plugin's version by build date.")
        {
        }
        
        void paint(Graphics& g) override
        {
            const auto buildDate = static_cast<String>(__DATE__) + " " + static_cast<String>(__TIME__);
            g.setColour(Shared::shared.colour(ColourID::Txt).withAlpha(.4f));
            g.drawFittedText(buildDate, getLocalBounds(), juce::Justification::centredRight, 1);
        }
    };

    struct ParamtrRandomizer :
        public Comp
    {
        using RandFunc = std::function<void(juce::Random&)>;

        ParamtrRandomizer(Utils& u, std::vector<Paramtr*>& _randomizables) :
            Comp(u, makeTooltip()),
            randomizables(_randomizables),
            randFuncs()
        {
        }
            
        ParamtrRandomizer(Utils& u) :
            Comp(u, makeTooltip()),
            randomizables(),
            randFuncs()
        {
        }

        void clear()
        {
            randomizables.clear();
            randFuncs.clear();
        }
            
        void add(Paramtr* p)
        {
            randomizables.push_back(p);
        }
            
        void add(RandFunc&& r)
        {
            randFuncs.push_back(r);
        }

        void operator()(bool sensitive)
        {
            Random rand;
            if(sensitive)
                for (auto randomizable : randomizables)
                {
                    const PID pID = randomizable->getPID();
                    auto& param = utils.getParam(pID);

                    auto val = param.getValue();
                    val += (rand.nextFloat() - .5f) * .1f;
					param.setValueWithGesture(juce::jlimit(0.f, 1.f, val));
                }
            else
            {
                for (auto& func : randFuncs)
                    func(rand);
                    
                for (auto randomizable : randomizables)
                {
                    const PID pID = randomizable->getPID();
                    auto& param = utils.getParam(pID);

                    param.setValueWithGesture(rand.nextFloat());
                }
            } 
        }
            
    protected:
        std::vector<Paramtr*> randomizables;
        std::vector<RandFunc> randFuncs;

        void paint(Graphics& g) override
        {
            const auto width = static_cast<float>(getWidth());
            const auto height = static_cast<float>(getHeight());
            const PointF centre(width, height);
            auto minDimen = std::min(width, height);
            BoundsF bounds
            (
                (width - minDimen) * .5f,
                (height - minDimen) * .5f,
                minDimen,
                minDimen
            );
            const auto thicc = utils.thicc;
            bounds.reduce(thicc, thicc);
            g.setColour(Shared::shared.colour(ColourID::Bg));
            g.fillRoundedRectangle(bounds, thicc);
            Colour mainCol;
            if (isMouseOver())
            {
                g.setColour(Shared::shared.colour(ColourID::Hover));
                g.fillRoundedRectangle(bounds, thicc);
                mainCol = Shared::shared.colour(ColourID::Interact);
            }
            else
                mainCol = Shared::shared.colour(ColourID::Txt);

            g.setColour(mainCol);
            g.drawRoundedRectangle(bounds, thicc, thicc);

            minDimen = std::min(bounds.getWidth(), bounds.getHeight());
            const auto radius = minDimen * .5f;
            const auto pointSize = radius * .4f;
            const auto pointRadius = pointSize * .5f;
            const auto d4 = minDimen / 4.f;
            const auto x0 = d4 * 1.2f + bounds.getX();
            const auto x1 = d4 * 2.8f + bounds.getX();
            for (auto i = 1; i < 4; ++i)
            {
                const auto y = d4 * i + bounds.getY();
                g.fillEllipse(x0 - pointRadius, y - pointRadius, pointSize, pointSize);
                g.fillEllipse(x1 - pointRadius, y - pointRadius, pointSize, pointSize);
            }
        }
            
        void mouseEnter(const Mouse& evt) override
        {
            Comp::mouseEnter(evt);
            repaint();
        }
            
        void mouseUp(const Mouse& evt) override
        {
            if (evt.mouseWasDraggedSinceMouseDown()) return;
			operator()(evt.mods.isShiftDown());
            tooltip = makeTooltip();
        }
            
        void mouseExit(const Mouse&) override
        {
            tooltip = makeTooltip();
            repaint();
        }

        String makeTooltip()
        {
            Random rand;
            static constexpr float Count = 187.f;
            const auto v = static_cast<int>(std::round(rand.nextFloat() * Count));
            switch (v)
            {
            case 0: return "Do it!";
            case 1: return "Don't you dare it!";
            case 2: return "But... what if it goes wrong???";
            case 3: return "Nature is random too, so this is basically analog, right?";
            case 4: return "Life is all about exploration..";
            case 5: return "What if I don't even exist?";
            case 6: return "Idk, it's all up to you.";
            case 7: return "This randomizes the parameter values. Yeah..";
            case 8: return "Born too early to explore space, born just in time to hit the randomizer.";
            case 9: return "Imagine someone sitting there writing down all these phrases.";
            case 10: return "This will not save your snare from sucking ass.";
            case 11: return "Producer-san >.< d.. don't tickle me there!!!";
            case 12: return "I mean, whatever.";
            case 13: return "Never commit. Just dream!";
            case 14: return "I wonder, what will happen if I...";
            case 15: return "Hit it for the digital warmth.";
            case 16: return "Do you like cats? They are so cute :3";
            case 17: return "We should collab some time, bro.";
            case 18: return "Did you just open the plugin UI just to see what's in here this time?";
            case 19: return "I should make a phaser. Would you want a phaser in here?";
            case 20: return "No time for figuring out parameter values manually, right?";
            case 21: return "My cat is meowing at the door, because there is a mouse.";
            case 22: return "Yeeeaaaaahhhh!!!! :)";
            case 23: return "Ur hacked now >:) no just kidding ^.^";
            case 24: return "What would you do if your computer could handle 1mil phasers?";
            case 25: return "It's " + (juce::Time::getCurrentTime().getHours() < 10 ? String("0") + static_cast<String>(juce::Time::getCurrentTime().getHours()) : static_cast<String>(juce::Time::getCurrentTime().getHours())) + ":" + (juce::Time::getCurrentTime().getMinutes() < 10 ? String("0") + static_cast<String>(juce::Time::getCurrentTime().getMinutes()) : static_cast<String>(juce::Time::getCurrentTime().getMinutes())) + " o'clock now.";
            case 26: return "I was a beat maker, too, but then I took a compressor to the knee.";
            case 27: return "It's worth a try.";
            case 28: return "Omg, your music is awesome dude. Keep it up!";
            case 29: return "I wish there was an anime about music producers.";
            case 30: return "Days are too short, but I also don't want gravity to get heavier.";
            case 31: return "Yo, let's order some pizza!";
            case 32: return "I wanna be the very best, like no one ever was!";
            case 33: return "Hm... yeah, that could be cool.";
            case 34: return "Maybe...";
            case 35: return "Well.. perhaps.";
            case 36: return "Here we go again.";
            case 37: return "What is the certainty of a certainty meaning a certain certainty?";
            case 38: return "My favourite car is an RX7 so i found it quite funny when Izotope released that plugin.";
            case 39: return "Do you know echobode? It's one of my favourites.";
            case 40: return "I never managed to make a proper eurobeat even though I love that genre.";
            case 41: return "Wanna lose control?";
            case 42: return "Do you have any more of dem randomness pls?";
            case 43: return "How random do you want it to be, sir? Yes.";
            case 44: return "Programming is not creative. I am a computer.";
            case 45: return "We should all be more mindful to each other.";
            case 46: return "Next-Level AI will randomize ur params!";
            case 47: return "All the Award-Winning Audio-Engineers use this button!!";
            case 48: return "The fact that you can't undo it only makes it better.";
            case 49: return "When things are almost as fast as light, reality bends.";
            case 50: return "I actually come from the future. Don't tell anyone pls.";
            case 51: return "You're mad!";
            case 52: return "Your ad could be here! ;)";
            case 53: return "What coloursheme does your tune sound like?";
            case 54: return "I wish Dyson Spheres existed already!";
            case 55: return "This is going to be so cool! OMG";
            case 56: return "Plants. There should be more of them.";
            case 57: return "10 Vibrato Mistakes Every Noob Makes: No 7 Will Make U Give Up On Music!";
            case 58: return "Yes, I'll add more of these some other time.";
            case 59: return "The world wasn't ready for No Man's Sky. That's all.";
            case 60: return "Temposynced Tremolos are not Sidechain Compressors.";
            case 61: return "I can't even!";
            case 62: return "Let's drift off into the distance together..";
            case 63: return "When I started making this plugin I thought I'd make a tape emulation.";
            case 64: return "Scientists still trying to figure this one out..";
            case 65: return "Would you recommend this button to your friends?";
            case 66: return "This is a very bad feature. Don't use it!";
            case 67: return "I don't know what to say about this button..";
            case 68: return "A parallel universe, in which you will use this button now, exists.";
            case 69: return "This is actually message no 69, haha";
            case 70: return "Who needs control anyway?";
            case 71: return "I have the feeling this time it will turn out really cool!";
            case 72: return "Turn all parameters up right to 11.";
            case 73: return "Tranquilize Your Music. Idk why, but it sounds cool.";
            case 74: return "I'm indecisive...";
            case 75: return "That's a good idea!";
            case 76: return "Once upon a time there was a traveller who clicked this button..";
            case 77: return "10/10 Best Decision!";
            case 78: return "Beware! Only really skilled audio professionals use this feature.";
            case 79: return "What would be your melody's name if it was a human being?";
            case 80: return "What if humanity was just a failed experiment by a higher species?";
            case 81: return "Enter the black hole to stop time!";
            case 82: return "Did you remember to water your plants yet?";
            case 83: return "I'm just a simple button. Nothing special to see here.";
            case 84: return "You're using this plugin. That makes you a cool person.";
            case 85: return "Only the greatest DSP technology in this parameter randomizer!";
            case 86: return "I am not fun at parties indeed.";
            case 87: return "This button makes it worse!";
            case 88: return "I am not sure what this is going to do.";
            case 89: return "If your music was a mountain, what shape would it be like?";
            case 90: return "This is the best Vibrato Plugin in the world. Tell ur friends";
            case 91: return "Do you feel the vibrations?";
            case 92: return "Defrost or Reheat? You decide.";
            case 93: return "Don't forget to hydrate yourself, king.";
            case 94: return "How long does it take to get to the next planet at this speed?";
            case 95: return "What if there is a huge wall around the whole universe?";
            case 96: return "Controlled loss of control. So basically drifting! Yeah!";
            case 97: return "I talk to the wind. My words are all carried away.";
            case 98: return "Captain, we need to warp now! There is no time.";
            case 99: return "Where are we now?";
            case 100: return "Randomize me harder, daddy!";
            case 101: return "Drama!";
            case 102: return "Turn it up! Well, this is not a knob, but you know, it's cool.";
            case 103: return "You like it dangerous, huh?";
            case 104: return "We are under attack.";
            case 105: return "Yes, you want this!";
            case 106: return "The randomizer is better than your presets!";
            case 107: return "Are you a decide-fan, or a random-enjoyer?";
            case 108: return "Let's get it started! :)";
            case 109: return "Do what you have to do...";
            case 110: return "This is a special strain of random. ;)";
            case 111: return "Return to the battlefield or get killed.";
            case 112: return "~<* Easy Peazy Lemon Squeezy *>~";
            case 113: return "Why does it sound like dubstep?";
            case 114: return "Excuse me.. Have you seen my sanity?";
            case 115: return "In case of an emergency, push the button!";
            case 116: return "Based.";
            case 117: return "Life is a series of random collisions.";
            case 118: return "It is actually possible to add too much salt to spaghetti.";
            case 119: return "You can't go wrong with random, except when you do.";
            case 120: return "I have not implemented undo yet, but you like to live dangerously :)";
            case 121: return "404 - Creative message not found. Contact our support pls.";
            case 122: return "Press jump twice to perform a doub.. oh wait, wrong app.";
            case 123: return "And now for the ultimate configuration!";
            case 124: return "Subscribe for more random messages! Only 40$/mon";
            case 125: return "I love you <3";
            case 126: return "Me? Well...";
            case 127: return "What happens if I press this?";
            case 128: return "Artificial Intelligence! Not used here, but it sounds cool.";
            case 129: return "My internet just broke so why not just write another msg in here, right?";
            case 130: return "Mood.";
            case 131: return "I'm only a randomizer, after all...";
            case 132: return "There is a strong correlation between you and awesomeness.";
            case 133: return "Yes! Yes! Yes!";
            case 134: return "Up for a surprise?";
            case 135: return "This is not a plugin. It is electricity arranged swag-wise.";
            case 136: return "Chairs do not exist.";
            case 137: return "There are giant spiders all over my house and I have no idea what to do.";
            case 138: return "My cat is lying on my lap purring and she's so cute omg!!";
            case 139: return "I come here and add more text whenever I procrastinate from fixing bugs.";
            case 140: return "Meow :3";
            case 141: return "N.. Nyan? uwu";
            case 142: return "Let's Go!";
            case 143: return "Never Gonna Let You Down! Never Gonna Give You Up!";
            case 144: return "Push It!";
            case 145: return "Do You Feel The NRG??";
            case 146: return "We could escape the great filter if we only vibed stronger..";
            case 147: return "Check The Clock. It's time for randomization.";
            case 148: return "The first version of this plugin was released in 2019.";
            case 149: return "This plugin was named after my son, Lionel.";
            case 150: return "If this plugin breaks, it's because your beat is so fire!";
            case 151: return "Go for it!";
            case 152: return "<!> nullptr exception: please contact the developer. <!>";
            case 153: return "Wild Missingno. appeared!";
            case 154: return "Do you have cats? Because I like cats. :3";
            case 155: return "There will be a netflix adaption of this plugin soon.";
            case 156: return "Studio Gib Ihm!";
            case 157: return "One Click And It's Perfect!";
            case 158: return "Elon Musk just twittered that this plugin is cool. Wait.. is that a good thing?";
            case 159: return "Remember to drink water, sempai!";
            case 160: return "Love <3";
            case 161: return "Your journey has just begun ;)";
            case 162: return "You will never be the same again...";
            case 163: return "Black holes are just functors that create this universe inside of this universe.";
            case 164: return "Feel the heat!";
            case 165: return "There is no going back. (Literally, because I didn't implement undo/redo)";
            case 166: return "Tbh, that would be crazy.";
            case 167: return "Your horoscope said you'll make the best beat of all time today.";
            case 168: return "Do it again! :)";
            case 169: return "Vibrato is not Vibrato, dude.";
            case 170: return "This is going to be all over the place!";
            case 171: return "Pitch and time... it belongs together.";
            case 172: return "A rainbow is actually transcendence that never dies.";
            case 173: return "It is not random. It is destiny!";
            case 174: return "Joy can enable you to change the world.";
            case 175: return "This is a very unprofessional plugin. It sucks out the 'pro' from your music.";
            case 176: return "They tried to achieve perfection, but they didn't hear this beat yet.";
            case 177: return "Dream.";
            case 178: return "Music is a mirror of your soul and has the potential to heal.";
            case 179: return "Lmao, nxt patch is going to be garbage";
            case 180: return "Insanity is doing the same thing over and over again.";
            case 181: return "If you're uninspired just do household-chores. Your brain will try to justify coming back.";
            case 182: return "You are defining the future standard!";
            case 183: return "Plugins are a lot like games, but you can't speedrun them.";
            case 184: return "When applying audiorate mod to mellow sounds it can make them sorta trumpet-ish.";
            case 185: return "NEL's oversampling is lightweight and reduces aliasing well, but it also alters the sound a bit.";
            case 186: return "This message was added 2022_03_15 at 18:10! just in case you wanted to know..";
            case 187: return "This is message no 187. Ratatatatatat.";
            default: "Are you sure?";
            }
            return "You are not supposed to read this message!";
        }
    };

    struct Visualizer :
        public Comp
    {
        using Buffer = std::array<std::vector<double>, 2>;

        Visualizer(Utils& u, String&& _tooltip, int _numChannels, int maxBlockSize) :
            Comp(u, std::move(_tooltip), CursorType::Default),

            onPaint([](Graphics&, Visualizer&){}),
            onUpdate([](Buffer&) { return false; }),

            buffer(),
            numChannels(_numChannels == 1 ? 1 : 2),
            blockSize(maxBlockSize)
        {
            for (auto& b : buffer)
                b.resize(blockSize, 0.f);
        }

        std::function<void(Graphics&, Visualizer&)> onPaint;
        std::function<bool(Buffer&)> onUpdate;

        Buffer buffer;
        const int numChannels, blockSize;

        void paint(Graphics& g) override
        {
            onPaint(g, *this);
        }

        void updateTimer() override
        {
            if (onUpdate(buffer))
                repaint();
        }
    };

    struct ScrollBar :
        public Comp
    {
        using OnChange = std::function<void(float)>;

        ScrollBar(Utils& u) :
            Comp(u, "Drag or mousewheel this to scroll."),
            onChange(),
            posY(0.f),
            dragY(0.f)
        {
            setBufferedToImage(false);
        }
            
        OnChange onChange;
    protected:
        float posY, dragY;

        void paint(Graphics& g) override
        {
            const auto thicc = utils.thicc;
            const auto bounds = getLocalBounds().toFloat().reduced(thicc);
            const auto posYAbs = bounds.getY() + bounds.getHeight() * posY;
            {
                const auto x = bounds.getX();
                const auto w = bounds.getWidth();
                const auto h = bounds.getHeight() * .2f;
                const auto radY = h * .5f;
                const auto y = posYAbs - radY;
                const BoundsF barBounds(x, y, w, h);
                g.setColour(Shared::shared.colour(ColourID::Bg));
                g.fillRoundedRectangle(barBounds, thicc);
                if (isMouseOver())
                {
                    g.setColour(Shared::shared.colour(ColourID::Hover));
                    g.fillRoundedRectangle(barBounds, thicc);
                }
                g.setColour(Shared::shared.colour(ColourID::Interact));
                g.drawRoundedRectangle(barBounds, thicc, thicc);
            }
        }

        void mouseEnter(const juce::MouseEvent& evt)
        {
            Comp::mouseEnter(evt);
            repaint();
        }
            
        void mouseExit(const juce::MouseEvent&)
        {
            repaint();
        }
            
        void mouseDown(const juce::MouseEvent& evt)
        {
            dragY = evt.position.y * utils.dragSpeed;
        }
            
        void mouseDrag(const juce::MouseEvent& evt)
        {
            const auto dragYNew = evt.position.y * utils.dragSpeed;
            auto dragOffset = dragYNew - dragY;
            if (evt.mods.isShiftDown())
                dragOffset *= SensitiveDrag;
            posY = juce::jlimit(0.f, 1.f, posY - dragOffset);
            onChange(posY);
            repaint();
            dragY = dragYNew;
        }
            
        void mouseWheelMove(const Mouse& evt, const MouseWheel& wheel) override
        {
            if (evt.mods.isAnyMouseButtonDown()) return;

            const bool reversed = wheel.isReversed ? -1.f : 1.f;
            const bool isTrackPad = wheel.deltaY * wheel.deltaY < .0549316f;
            if (isTrackPad)
                dragY = reversed * wheel.deltaY;
            else
            {
                const auto deltaYPos = wheel.deltaY > 0.f ? 1.f : -1.f;
                dragY = reversed * WheelDefaultSpeed * deltaYPos;
            }
            if (evt.mods.isShiftDown())
                dragY *= SensitiveDrag;
            posY = juce::jlimit(0.f, 1.f, posY - dragY);
            onChange(posY);
            repaint();
        }
    };

    struct Browser :
        public Comp
    {
        using Entry = std::unique_ptr<Button>;

        inline Notify makeNotify(Browser& browser)
        {
            return [&brwsr = browser](int, const void*)
            {
                return false;
            };
        }
        static constexpr int MinEntryHeight = 25;

        Browser(Utils& u) :
            Comp(u, makeNotify(*this), "", CursorType::Default),
            scrollBar(u),
            yOffset(0)
        {
            scrollBar.onChange = [this](float barY)
            {
                const auto numEntries = static_cast<float>(getNumEntries());
                const auto entryHeight = static_cast<float>(MinEntryHeight);
                const auto highest = numEntries * entryHeight;
                yOffset = static_cast<int>(highest * barY);
                resized();
            };
            setInterceptsMouseClicks(false, true);
            addChildComponent(scrollBar);
        }
        
        void addEntry(String&& _tooltip,
            const Button::OnPaint& onPaint, const Button::OnClick& onClick)
        {
            entries.push_back(std::make_unique<Button>(utils, std::move(_tooltip)));
            auto& entry = *entries.back();
            entry.onPaint = onPaint;
            entry.onClick = onClick;
            addAndMakeVisible(entry);
            resized();
        }
        
        void addEntry(String&& _name, String&& _tooltip,
            const Button::OnClick& onClick, Just just = Just::centred)
        {
            entries.push_back(std::make_unique<Button>(utils, std::move(_tooltip)));
            auto& entry = *entries.back();
            entry.setName(std::move(_name));
            entry.onPaint = makeTextButtonOnPaint(std::move(_name), just);
            entry.onClick = onClick;
            addAndMakeVisible(entry);
            resized();
        }
            
        void clearEntries()
        {
            entries.clear();
        }
            
        size_t getNumEntries() const noexcept
        {
            return entries.size();
        }
            
        void paint(Graphics& g) override
        {
            g.setColour(Shared::shared.colour(ColourID::Darken));
            g.fillRoundedRectangle(getLocalBounds().toFloat(), utils.thicc);
            if (entries.size() != 0)
                return;
            g.setColour(Shared::shared.colour(ColourID::Abort));
            g.setFont(Shared::shared.font);
            g.drawFittedText("Browser empty.", getLocalBounds(), Just::centred, 1);
        }

        void resized() override
        {
            if (entries.size() == 0)
            {
                scrollBar.setVisible(false);
                return;
            }
            const auto thicc = utils.thicc;
            const auto bounds = getLocalBounds().toFloat().reduced(thicc);
            const auto entriesSizeInv = 1.f / static_cast<float>(entries.size());
            auto entryHeight = bounds.getHeight() * entriesSizeInv;
            if (entryHeight >= MinEntryHeight)
            {
                scrollBar.setVisible(false);
                const auto x = bounds.getX();
                auto y = bounds.getY();
                const auto w = bounds.getWidth();
                const auto h = entryHeight;
                for (auto i = 0; i < entries.size(); ++i, y += h)
                    entries[i]->setBounds(BoundsF(x, y, w, h).toNearestInt());
            }
            else
            {
                scrollBar.setVisible(true);
                const auto entryX = bounds.getX();
                const auto scrollBarWidth = bounds.getWidth() * .05f;
                const auto scrollBarX = bounds.getRight() - scrollBarWidth;
                {
                    const auto x = scrollBarX;
                    const auto y = bounds.getY();
                    const auto w = scrollBarWidth;
                    const auto h = bounds.getHeight();
                    scrollBar.setBounds(BoundsF(x, y, w, h).toNearestInt());
                }
                {
                    const auto x = entryX;
                    auto y = bounds.getY();
                    const auto w = scrollBarX;
                    const auto h = MinEntryHeight;
                    for (auto i = 0; i < entries.size(); ++i, y += h)
                        entries[i]->setBounds(BoundsF
                        (
                            x,
                            y - yOffset,
                            w,
                            h
                        ).toNearestInt());
                }
            }
        }
       
    protected:
        std::vector<Entry> entries;
        ScrollBar scrollBar;
        int yOffset;
    };

    struct PresetBrowser :
        public Comp
    {
        using SaveFunc = std::function<juce::ValueTree()>;
		using LoadFunc = std::function<void(const juce::ValueTree&)>;

        PresetBrowser(Utils& u, const String& _extension, const String& subDirectory) :
            Comp(u, "", CursorType::Default),
            saveFunc(nullptr),
            loadFunc(nullptr),
            //
            openCloseButton(u, "Click here to open or close the preset browser."),
            saveButton(u, "Click here to manifest the current preset into the browser."),
            pathButton(u, "This button will show you where your files are stored."),
            browser(u),
            presetNameEditor("Enter Name.."),
            //
            directory(u.audioProcessor.appProperties.getUserSettings()->getFile().getParentDirectory().getChildFile(subDirectory)),
            extension(_extension),
            //
            timerIdx(0),
            timerRunning(false)
        {
            if (!directory.exists())
                directory.createDirectory();
            
            setInterceptsMouseClicks(false, true);
            setBufferedToImage(false);
            addChildComponent(browser);
            addChildComponent(presetNameEditor);
            addChildComponent(saveButton);
            addChildComponent(pathButton);

            openCloseButton.onPaint = makeButtonOnPaintBrowse();
            openCloseButton.onClick = [&]()
            {
                setBrowserOpen(!browser.isVisible());
            };

            pathButton.onPaint = makeButtonOnPaintDirectory();
            pathButton.onClick = [&]()
            {
                URL url(directory.getFullPathName());
                url.launchInDefaultBrowser();
            };

            saveButton.onPaint = makeButtonOnPaintSave();
            saveButton.onClick = [&]()
            {
                // save preset to list of presets
                auto pName = presetNameEditor.getText();
                if (pName.isEmpty())
                    return;
                if (!pName.endsWith(extension))
                    pName += extension;
                auto pFile = directory.getChildFile(pName);
                if (pFile.exists())
                    pFile.deleteFile();
                const auto state = saveFunc();
                pFile.appendText
                (
                    state.toXmlString(),
                    false,
                    false
                );
                refreshBrowser();
            };
        }
        
        void init(Component& c)
        {
            c.addAndMakeVisible(*this);
            c.addAndMakeVisible(openCloseButton);
        }
        
        const Button& getOpenCloseButton() const noexcept
        {
            return openCloseButton;
        }
        
        Button& getOpenCloseButton() noexcept
        {
            return openCloseButton;
        }

        void updateTimer() override
        {
            if (!timerRunning)
                return;

            ++timerIdx;
            if (timerIdx < 15)
                return;

            timerIdx = 0;
            
            const auto numFiles = directory.getNumberOfChildFiles
            (
                File::TypesOfFileToFind::findFiles,
                "*" + extension
            );
            if (numFiles != browser.getNumEntries())
                refreshBrowser();
        }

        SaveFunc saveFunc;
        LoadFunc loadFunc;
    protected:
        Button openCloseButton, saveButton, pathButton;
        Browser browser;
        juce::TextEditor presetNameEditor;
        
        const File directory;
        String extension;
        int timerIdx;
        bool timerRunning;

        void setBrowserOpen(bool e)
        {
            browser.setVisible(e);
            presetNameEditor.setVisible(e);
            saveButton.setVisible(e);
            pathButton.setVisible(e);
            if (e)
                initBrowser();
            else
                browser.clearEntries();
            timerRunning = e;
        }
    private:
        void paint(Graphics&) override {}
        
        void resized() override
        {
            const auto bounds = getLocalBounds().toFloat();
            const auto titleHeight = bounds.getHeight() * .14f;
            const auto presetNameWidth = bounds.getWidth() * .85f;
            {
                auto x = bounds.getX();
                auto y = bounds.getY();
                auto w = presetNameWidth;
                auto h = titleHeight;
                presetNameEditor.setBounds(BoundsF(x, y, w, h).toNearestInt());
            }
            {
                auto x = presetNameWidth;
                auto y = bounds.getY();
                auto w = (bounds.getWidth() - presetNameWidth) * .5f;
                auto h = titleHeight;
                saveButton.setBounds(maxQuadIn(BoundsF(x, y, w, h)).toNearestInt());
                x += w;
                pathButton.setBounds(maxQuadIn(BoundsF(x, y, w, h)).toNearestInt());
            }
            {
                auto x = bounds.getX();
                auto y = titleHeight;
                auto w = bounds.getWidth();
                auto h = bounds.getHeight() - titleHeight;
                browser.setBounds(BoundsF(x, y, w, h).toNearestInt());
            }
        }
        
        void initBrowser()
        {
            const auto fileTypes = File::TypesOfFileToFind::findFiles;
            const auto wildCard = "*" + extension;
            const juce::RangedDirectoryIterator files
            (
                directory,
                true,
                wildCard,
                fileTypes
            );
            
            for (const auto& it: files)
            {
                const auto file = it.getFile();
                const auto fileName = file.getFileName();
                const auto nameLen = fileName.length() - extension.length();
                browser.addEntry
                (
                    fileName.substring(0, nameLen),
                    "Click here to choose this preset.",
                    [&, str = file.loadFileAsString()]()
                    {
						loadFunc(ValueTree::fromXml(str));
                    },
                    Just::left
                );
            }
        }

        void refreshBrowser()
        {
            browser.clearEntries();
            initBrowser();
            resized();
        }
    };
    
    inline std::function<void(Graphics&, Visualizer&)> makeVibratoVisualizerOnPaint()
    {
        return[](Graphics& g, Visualizer& v)
        {
            const auto thicc = v.utils.thicc;
            const auto bounds = v.getLocalBounds().toFloat().reduced(thicc);
            const auto tickThicc = 1.f, w = tickThicc * 2.f + 1.f;
            const auto y = bounds.getY();
            const auto h = bounds.getHeight();
            g.setColour(Shared::shared.colour(ColourID::Hover));
            {
                const auto x = bounds.getX() + bounds.getWidth() * .5f - tickThicc;
                g.fillRect(x, y, w, h);
            }
            g.setColour(Shared::shared.colour(ColourID::Mod));
            {
                const auto smpl = static_cast<float>(juce::jlimit(0., 1., v.buffer[0][0] * .5 + .5));
                const auto X = bounds.getX() + smpl * bounds.getWidth();
                const auto x = X - tickThicc;
                g.fillRect(x, y, w, h);
            }
            if (v.numChannels == 2)
            {
                const auto smpl = static_cast<float>(juce::jlimit(0., 1., v.buffer[1][0] * .5 + .5));
                const auto X = bounds.getX() + smpl * bounds.getWidth();
                const auto x = X - tickThicc;
                g.fillRect(x, y, w, h);
            }
            g.drawRoundedRectangle(bounds, thicc, thicc);
        };
    }

    inline std::function<void(Graphics&, Visualizer&)> makeVibratoVisualizerOnPaint2()
    {
        return[](Graphics& g, Visualizer& v)
        {
            const auto thicc = v.utils.thicc;
            const auto bounds = v.getLocalBounds().toFloat().reduced(thicc);
            const auto tickThicc = 1.f, w = tickThicc * 2.f + 1.f;
            const auto y = bounds.getY();
            const auto h = bounds.getHeight();
            g.setColour(Shared::shared.colour(ColourID::Hover));
            {
                auto x = bounds.getX() - tickThicc;
                g.fillRect(x, y, w, h);
                x = bounds.getX() + bounds.getWidth() * .5f - tickThicc;
                g.fillRect(x, y, w, h);
                x = bounds.getRight() - tickThicc;
                g.fillRect(x, y, w, h);
            }
            g.setColour(Shared::shared.colour(ColourID::Mod));
            {
                auto smpl = juce::jlimit(0., 1., v.buffer[0][0] * .5 + .5);
                auto X = bounds.getX() + static_cast<float>(smpl) * bounds.getWidth();
                auto x0 = X - tickThicc;
                if (v.numChannels == 1)
                {
                    g.fillRect(x0, y, w, h);
                }
                else
                {
                    smpl = juce::jlimit(0., 1., v.buffer[1][0] * .5 + .5);
                    X = bounds.getX() + static_cast<float>(smpl) * bounds.getWidth();
                    auto x1 = X - tickThicc;
                    if(x0 == x1)
                        g.fillRect(x0, y, w, h);
                    else if (x0 > x1)
                        std::swap(x0, x1);

                    g.drawRoundedRectangle(x0, y, x1 - x0 + w, h, thicc, thicc);
                }
            }
                
        };
    }
}

/*

browser
    every entry has vector of optional additional buttons (like delete, rename)

presetbrowser
    tag system for presets

ParamtrRandomizer
    should randomize?
        parameter-modulations
            depth
            add + remove

Paramtr
    sometimes lock not serialized (on load?)

*/