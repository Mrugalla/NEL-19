#pragma once
#include "../PluginProcessor.h"
#include "ModSys.h"
#include <array>
#include <cstdint>

namespace modSys6
{
	namespace gui
	{
        using Buffer = std::vector<std::vector<float>>;

        using Notify = std::function<bool(const int, const void*)>;
        
        enum NotificationType
        {
            ColourChanged,
            TooltipsEnabledChanged,
            TooltipUpdated,
            ModSelectionChanged,
            ConnectionEnabled,
            ConnectionDisabled,
            ConnectionDepthChanged,
            ParameterHovered,
            ParameterDragged,
            KillPopUp,
            EnterValue,
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
                Events& getEvents() noexcept { return events; }
            protected:
                Events& events;
            };

            Events() :
                evts()
            {
            }
            void add(Evt* e) { evts.push_back(e); }
            void remove(const Evt* e)
            {
                for (auto i = 0; i < evts.size(); ++i)
                    if (e == evts[i])
                    {
                        evts.erase(evts.begin() + i);
                        return;
                    }
            }
            void notify(const int notificationType, const void* stuff = nullptr)
            {
                for (auto e : evts)
                    if (e->notifier(notificationType, stuff))
                        return;
            }
        protected:
            std::vector<Evt*> evts;
        };

        enum class ColourID { Txt, Bg, Abort, Mod, Interact, Inactive, Darken, Hover, Transp, NumCols };
        
        inline juce::Colour getDefault(ColourID i) noexcept
        {
            switch (i)
            {
            case ColourID::Txt: return juce::Colour(0xff67cf00);
            case ColourID::Bg: return juce::Colour(0xff100021);
            case ColourID::Inactive: return juce::Colour(0xff808080);
            case ColourID::Abort: return juce::Colour(0xffff0000);
            case ColourID::Mod: return juce::Colour(0xff0046ff);
            case ColourID::Interact: return juce::Colour(0xff00ffc5);
            case ColourID::Darken: return juce::Colour(0xea000000);
            case ColourID::Hover: return juce::Colour(0x53ffffff);
            default: return juce::Colour(0x00000000);
            }
        }
        
        inline juce::String toString(ColourID i)
        {
            switch (i)
            {
            case ColourID::Txt: return "text";
            case ColourID::Bg: return "background";
            case ColourID::Abort: return "abort";
            case ColourID::Mod: return "mod";
            case ColourID::Interact: return "interact";
            case ColourID::Inactive: return "inactive";
            case ColourID::Darken: return "darken";
            case ColourID::Hover: return "hover";
            case ColourID::Transp: return "transp";
            default: return "";
            }
        }

        inline juce::String toStringProps(ColourID i)
        {
            return "colour" + toString(i);
        }

        enum class CursorType { Default, Interact, Mod, Cross, NumCursors };

        inline int getNumRows(const juce::String& txt)
        {
            auto rows = 1;
            for (auto i = 0; i < txt.length(); ++i)
                rows = txt[i] == '\n' ? rows + 1 : rows;
            return rows;
        }

        inline juce::Rectangle<float> maxQuadIn(const juce::Rectangle<float>& b) noexcept
        {
            const auto minDimen = std::min(b.getWidth(), b.getHeight());
            const auto x = b.getX() + .5f * (b.getWidth() - minDimen);
            const auto y = b.getY() + .5f * (b.getHeight() - minDimen);
            return { x, y, minDimen, minDimen };
        }
        
        inline juce::Rectangle<float> maxQuadIn(juce::Rectangle<int> b) noexcept
        {
            return maxQuadIn(b.toFloat());
        }

        inline juce::Point<float> boundsOf(juce::Font& font, juce::String& txt)
        {
            const auto w = font.getStringWidthFloat(txt);
            
            auto numLines = 2.f;
            for (auto t = 0; t < txt.length() - 1; ++t)
                if (txt[t] == '\n')
                    ++numLines;

            const auto h = font.getHeightInPoints() * numLines;
            return { w, h };
        }

        inline void repaintWithChildren(juce::Component* comp)
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
                thicc(2.2f),
                tooltipsEnabled(true),
                font(juce::Font(juce::Typeface::createSystemTypefaceFor(BinaryData::nel19_ttf,
                    BinaryData::nel19_ttfSize)))
            {
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
                        setColour(c, juce::Colour::fromString(colStr));
                    }
                    // INIT TOOLTIPS
                    tooltipsEnabled = props->getBoolValue("tooltips", true);
                    setTooltipsEnabled(tooltipsEnabled);
                }
            }

            bool setColour(const juce::String& i, juce::Colour col) {
                for (auto c = 0; c < colours.size(); ++c)
                    if (i == colours[c].toString())
                        return setColour(c, col);
                return false;
            }
            
            bool setColour(ColourID i, juce::Colour col) noexcept { return setColour(static_cast<int>(i), col); }
            
            bool setColour(int i, juce::Colour col) noexcept
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
            
            juce::Colour colour(ColourID i) const noexcept { return colour(static_cast<int>(i)); }
            
            juce::Colour colour(int i) const noexcept { return colours[i]; }

            juce::String getTooltipsEnabledID() const { return "tooltips"; }
            
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
            
            bool updateProperty(juce::String&& pID, const juce::var& var)
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
            std::array<juce::Colour, static_cast<int>(ColourID::NumCols)> colours;
            juce::String tooltipDefault;
            float thicc;
            bool tooltipsEnabled;
            juce::Font font;

            static Shared shared;
        };

        inline void visualizeGroup(juce::Graphics& g, juce::String&& txt,
            juce::Rectangle<float> bounds, juce::Colour col, float thicc,
            bool drawLeft = true, bool drawRight = true)
        {
            g.setColour(col);
            g.setFont(Shared::shared.font);
            {
                const auto edgeLen = std::min(bounds.getWidth(), bounds.getHeight()) * .2f;
                {
                    const auto x = bounds.getX();
                    const auto y = bounds.getY();
                    g.fillRect(x + thicc, y, edgeLen, thicc);
                    if(drawLeft)
                        g.fillRect(x, y + thicc, thicc, edgeLen);
                    const juce::Rectangle<float> txtBounds(
                        x + edgeLen + thicc * 2.f,
                        y,
                        bounds.getWidth(),
                        bounds.getHeight()
                    );
                    g.drawFittedText(txt, txtBounds.toNearestInt(), juce::Justification::topLeft, 1);
                }
                {
                    const auto x = bounds.getRight();
                    const auto y = bounds.getY();
                    g.fillRect(x - edgeLen - thicc, y, edgeLen, thicc);
                    if(drawRight)
                        g.fillRect(x, y + thicc, thicc, edgeLen);
                }
                {
                    const auto x = bounds.getRight();
                    const auto y = bounds.getBottom();
                    g.fillRect(x - edgeLen - thicc, y, edgeLen, thicc);
                    if(drawRight)
                        g.fillRect(x, y - edgeLen - thicc, thicc, edgeLen);
                }
                {
                    const auto x = bounds.getX();
                    const auto y = bounds.getBottom();
                    g.fillRect(x + thicc, y, edgeLen, thicc);
                    if(drawLeft)
                        g.fillRect(x, y - edgeLen - thicc, thicc, edgeLen);
                }
            }
        }

        inline void makeCursor(juce::Component& c, CursorType t)
        {
            juce::Image img;
            if (t == CursorType::Cross)
                img = juce::ImageCache::getFromMemory(BinaryData::cursorCross_png, BinaryData::cursorCross_pngSize).createCopy();
            else
                img = juce::ImageCache::getFromMemory(BinaryData::cursor_png, BinaryData::cursor_pngSize).createCopy();
            {
                const juce::Colour imgCol(0xff37946e);
                const auto col = t == CursorType::Default ?
                    Shared::shared.colour(modSys6::gui::ColourID::Txt) :
                    t == CursorType::Mod ?
                        Shared::shared.colour(modSys6::gui::ColourID::Mod) :
                        Shared::shared.colour(modSys6::gui::ColourID::Interact);
                for (auto y = 0; y < img.getHeight(); ++y)
                    for (auto x = 0; x < img.getWidth(); ++x)
                        if (img.getPixelAt(x, y) == imgCol)
                            img.setPixelAt(x, y, col);
            }

            static constexpr int scale = 3;
            img = img.rescaled(img.getWidth() * scale, img.getHeight() * scale, juce::Graphics::ResamplingQuality::lowResamplingQuality);

            juce::MouseCursor cursor(img, 0, 0);
            c.setMouseCursor(cursor);
        }

        struct Utils
        {
            Utils(ModSys& _modSys, juce::Component& _pluginTop, juce::PropertiesFile* props,
                Nel19AudioProcessor& _processor) :
                events(),
                pluginTop(_pluginTop),
                audioProcessor(_processor),
                modSys(_modSys),
                tooltip(&Shared::shared.tooltipDefault),
                selectedMod({ ModType::Macro, 0 }),
                notify(events)
            {
                Shared::shared.init(props);
            }
            
            void init()
            {
                notify(NotificationType::ModSelectionChanged);
            }

            float getDragSpeed() noexcept
            {
                static constexpr float DragSpeed = .5f;
                const auto height = static_cast<float>(pluginTop.getHeight());
                const auto speed = DragSpeed * height;
                return speed;
            }

            void updatePatch(const juce::String& xmlString)
            {
                audioProcessor.suspendProcessing(true);
                modSys.updatePatch(xmlString);
                audioProcessor.suspendProcessing(false);
            }
            
            void updatePatch(const juce::ValueTree& state)
            {
                audioProcessor.suspendProcessing(true);
                modSys.updatePatch(state);
                audioProcessor.suspendProcessing(false);
            }

            juce::Colour colour(ColourID c) const noexcept { return Shared::shared.colour(c); }

            void selectMod(ModTypeContext mtc)
            {
                if (selectedMod != mtc)
                {
                    selectedMod = mtc;
                    notify(NotificationType::ModSelectionChanged);
                }
            }
            
            ModTypeContext getSelectedMod() const noexcept { return selectedMod; }

            void killEnterValue()
            {
                notify(NotificationType::KillEnterValue);
            }

            const Param* getParam(PID p) const noexcept { return modSys.getParam(p); }
            Param* getParam(PID p) noexcept { return modSys.getParam(p); }
            const Param* getParam(PID p, int offset) const noexcept
            {
                return modSys.getParam(modSys6::withOffset(p, offset));
            }
            Param* getParam(PID p, int offset) noexcept
            {
                return modSys.getParam(modSys6::withOffset(p, offset));
            }
            int getParamIdx(PID p) noexcept { return modSys.getParamIdx(p); }
            int getModIdx(ModTypeContext mtc) const noexcept { return modSys.getModIdx(mtc); }
            int getSelectedModIdx() const noexcept { return getModIdx(selectedMod); }
            int getConnecIdxWith(int mIdx, int pIdx) const noexcept { return modSys.getConnecIdxWith(mIdx, pIdx); }
            int getConnecIdxWith(int mIdx, PID pID) const noexcept { return modSys.getConnecIdxWith(mIdx, pID); }

            float getConnecDepth(int cIdx) const noexcept
            {
                if (cIdx == -1) return 0.f;
                return modSys.getConnecDepth(cIdx);
            }
            void setConnecDepth(int cIdx, float depth) noexcept
            {
                if(modSys.setConnecDepth(cIdx, depth))
                    notify(NotificationType::ConnectionDepthChanged);
            }

            bool enableConnection(ModTypeContext mtc, PID pID, float depth) noexcept
            {
                if (modSys.enableConnection(getModIdx(mtc), getParamIdx(pID), depth))
                {
                    notify(NotificationType::ConnectionEnabled);
                    return true;
                }
                return false;
            }
            void disableConnection(int cIdx) noexcept
            {
                if(modSys.disableConnection(cIdx))
                    notify(NotificationType::ConnectionDisabled);
            }

            bool hasPlayHead() const noexcept { return modSys.getHasPlayHead(); }

            void setTooltipsEnabled(bool e)
            {
                if (Shared::shared.setTooltipsEnabled(e))
                    notify(NotificationType::TooltipsEnabledChanged);
            }
            bool getTooltipsEnabled() const noexcept
            {
                return Shared::shared.tooltipsEnabled;
            }
            void setTooltip(juce::String* newTooltip)
            {
                if (tooltip == newTooltip) return;
                if (newTooltip == nullptr)
                    tooltip = &Shared::shared.tooltipDefault;
                else
                    tooltip = newTooltip;
                notify(NotificationType::TooltipUpdated);
            }
            const juce::String* getTooltip() const noexcept { return tooltip; }
            juce::String* getTooltip() noexcept { return tooltip; }

            juce::Point<int> getWindowPos(const juce::Component& comp) noexcept
            {
                return comp.getScreenPosition() - pluginTop.getScreenPosition();
            }
            juce::Point<int> getWindowCentre(const juce::Component& comp) noexcept
            {
                const juce::Point<int> centre
                (
                    comp.getWidth() / 2,
                    comp.getHeight() / 2
                );
                return getWindowPos(comp) + centre;
            }
            juce::Point<int> getWindowNearby(const juce::Component& comp) noexcept
            {
                const auto pos = getWindowCentre(comp).toFloat();
                const auto w = static_cast<float>(comp.getWidth());
                const auto h = static_cast<float>(comp.getHeight());
                const auto off = (w + h) * .5f;
                const auto pluginCentre = juce::Point<int>(
                    pluginTop.getWidth(),
                    pluginTop.getHeight()
                ).toFloat() * .5f;
                const juce::Line<float> vec(pos, pluginCentre);
                return juce::Line<float>::fromStartAndAngle(pos, off, vec.getAngle()).getEnd().toInt();
            }
            inline juce::Rectangle<int> getWindowBounds(const juce::Component& comp) noexcept
            {
                return comp.getScreenBounds() - pluginTop.getScreenPosition();
            }

            juce::ValueTree& getState() noexcept { return modSys.state; }

            Events events;
            juce::Component& pluginTop;
            Nel19AudioProcessor& audioProcessor;
        protected:
            ModSys& modSys;
            juce::String* tooltip;
            ModTypeContext selectedMod;
            Events::Evt notify;
        };

        struct Comp :
            public juce::Component
        {
            Comp(Utils& _utils, juce::String&& _tooltip = "", CursorType _cursorType = CursorType::Interact) :
                utils(_utils),
                tooltip(_tooltip),
                notifyBasic(utils.events, makeNotifyBasic(this)),
                notify(utils.events),
                cursorType(_cursorType)
            {
                setBufferedToImage(true);
                makeCursor(*this, cursorType);
            }
            Comp(Utils& _utils, const juce::String& _tooltip = "", CursorType _cursorType = CursorType::Interact) :
                utils(_utils),
                tooltip(_tooltip),
                notifyBasic(utils.events, makeNotifyBasic(this)),
                notify(utils.events),
                cursorType(_cursorType)
            {
                setBufferedToImage(true);
                makeCursor(*this, cursorType);
            }
            Comp(Utils& _utils, Notify _notify, juce::String&& _tooltip = "", CursorType _cursorType = CursorType::Interact) :
                utils(_utils),
                tooltip(_tooltip),
                notifyBasic(utils.events, makeNotifyBasic(this)),
                notify(utils.events, _notify),
                cursorType(_cursorType)
            {
                setBufferedToImage(true);
                makeCursor(*this, cursorType);
            }
            Comp(Utils& _utils, Notify _notify, const juce::String& _tooltip = "", CursorType _cursorType = CursorType::Interact) :
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
            const juce::String& getTooltip() const noexcept { return tooltip; }
        protected:
            Utils& utils;
            juce::String tooltip;
            Events::Evt notifyBasic, notify;

            void mouseEnter(const juce::MouseEvent&) override
            {
                utils.setTooltip(&tooltip);
            }
            void mouseDown(const juce::MouseEvent&) override
            {
                notifyBasic(NotificationType::KillEnterValue);
            }

            void paint(juce::Graphics& g) override
            {
                const auto t = Shared::shared.thicc;
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

        struct BlinkyBoy :
            public juce::Timer
        {
            BlinkyBoy(Comp* _comp) :
                juce::Timer(),
                comp(_comp),
                env(0.f), x(0.f),
                eps(1.f)
            {

            }
            void flash(juce::Graphics& g, juce::Colour col)
            {
                g.fillAll(col.withAlpha(env * env));
            }
            void flash(juce::Graphics& g, const juce::Rectangle<float>& bounds, juce::Colour col)
            {
                const auto thicc = Shared::shared.thicc;
                g.setColour(col.withAlpha(env * env));
                g.fillRoundedRectangle(bounds, thicc);
            }

            void trigger(float durationInSec)
            {
                startTimerHz(30);
                env = 1.f;
                x = 1.f - 1.f / (30.f / durationInSec);
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
            ImageComp(Utils& u, juce::String&& _tooltip, juce::Image&& _img) :
                Comp(u, std::move(_tooltip), CursorType::Default),
                img(_img)
            {}
            ImageComp(Utils& u, juce::String&& _tooltip, const char* data, const int size) :
                Comp(u, std::move(_tooltip), CursorType::Default),
                img(juce::ImageCache::getFromMemory(data, size))
            {}
            juce::Image img;
        protected:
            void paint(juce::Graphics& g) override
            {
                const auto q = juce::Graphics::lowResamplingQuality;
                const auto thicc = Shared::shared.thicc;
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
            Label(Utils& u, juce::String&& _txt, ColourID _bgC = ColourID::Transp, ColourID _outlineC = ColourID::Transp, ColourID _txtC = ColourID::Txt) :
                Comp(u, "", modSys6::gui::CursorType::Default),
                font(),
                txt(_txt),
                bgC(_bgC),
                outlineC(_outlineC),
                txtC(_txtC),
                just(juce::Justification::centred)
            {
                setInterceptsMouseClicks(false, false);
            }
            Label(Utils& u, const juce::String& _txt, ColourID _bgC = ColourID::Transp, ColourID _outlineC = ColourID::Transp, ColourID _txtC = ColourID::Txt) :
                Comp(u, "", modSys6::gui::CursorType::Default),
                font(),
                txt(_txt),
                bgC(_bgC),
                outlineC(_outlineC),
                txtC(_txtC),
                just(juce::Justification::centred)
            {
                setInterceptsMouseClicks(false, false);
            }

            void setText(const juce::String& t)
            {
                txt = t;
                updateBounds();
            }
            void setText(juce::String&& t)
            {
                setText(t);
            }
            const juce::String& getText() const noexcept { return txt; }
            juce::String& getText() noexcept { return txt; }
            
            void setJustifaction(juce::Justification j) noexcept { just = j; }
            juce::Justification getJustification() const noexcept { return just; }

            juce::Font font;
        protected:
            juce::String txt;
            ColourID bgC, outlineC, txtC;
            juce::Justification just;

            void resized() override
            {
                updateBounds();
            }

            void paint(juce::Graphics& g) override
            {
                const auto thicc = Shared::shared.thicc;
                const auto bounds = getLocalBounds().toFloat().reduced(Shared::shared.thicc);
                g.setFont(font);
                g.setColour(Shared::shared.colour(bgC));
                g.fillRoundedRectangle(bounds, thicc);
                g.setColour(Shared::shared.colour(outlineC));
                g.drawRoundedRectangle(bounds, thicc, thicc);
                g.setColour(Shared::shared.colour(txtC));
                g.drawFittedText(
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
        };

        struct Button :
            public Comp
        {
            using OnPaint = std::function<void(juce::Graphics&, Button&)>;
            using OnClick = std::function<void()>;

            Button(Utils& u, const juce::String& _tooltip) :
                Comp(u, _tooltip),
                onPaint([](juce::Graphics&, Button&){}),
                onClick(nullptr),
                state(0)
            {}
            int getState() const noexcept { return state; }
            void setState(int x) { state = x; }

            OnPaint onPaint;
            OnClick onClick;
        protected:
            int state;

            void paint(juce::Graphics& g) override
            {
                onPaint(g, *this);
            }
            void mouseEnter(const juce::MouseEvent& evt) override
            {
                Comp::mouseEnter(evt);
                repaint();
            }
            void mouseExit(const juce::MouseEvent&) override
            {
                repaint();
            }
            void mouseDown(const juce::MouseEvent&) override
            {
                repaint();
            }
            void mouseUp(const juce::MouseEvent& evt) override
            {
                if (!evt.mouseWasDraggedSinceMouseDown())
                    if (onClick != nullptr)
                        onClick();
                repaint();
            }
        };

        inline Button::OnPaint makeTextButtonOnPaint(juce::String&& text, juce::Justification just = juce::Justification::centred)
        {
            return[txt = std::move(text), just](juce::Graphics& g, Button& button)
            {
                const auto thicc = Shared::shared.thicc;
                const auto bounds = button.getLocalBounds().toFloat().reduced(thicc);
                g.setColour(Shared::shared.colour(ColourID::Bg));
                g.fillRoundedRectangle(bounds, thicc);
                juce::Colour mainCol;
                if (button.isMouseButtonDown())
                {
                    g.setColour(Shared::shared.colour(ColourID::Hover));
                    g.fillRoundedRectangle(bounds, thicc);

                    if (button.isMouseOver())
                        g.fillRoundedRectangle(bounds, thicc);
                    mainCol = Shared::shared.colour(ColourID::Interact);
                }
                else
                {
                    if (button.isMouseOver())
                    {
                        g.setColour(Shared::shared.colour(ColourID::Hover));
                        g.fillRoundedRectangle(bounds, thicc);
                        mainCol = Shared::shared.colour(ColourID::Interact);
                    }
                    else
                        mainCol = Shared::shared.colour(ColourID::Txt);
                }
                g.setColour(mainCol);
                g.drawRoundedRectangle(bounds, thicc, thicc);
                g.setFont(Shared::shared.font);
                g.drawFittedText(txt, bounds.toNearestInt(), just, 1);
            };
        }
        
        inline Button::OnPaint makeButtonOnPaintBrowse()
        {
            return [](juce::Graphics& g, Button& button)
            {
                const auto thicc = Shared::shared.thicc;
                const auto bounds = button.getLocalBounds().toFloat().reduced(thicc);
                juce::Colour mainCol, hoverCol(0x00000000);
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
                const juce::Point<float> rad(
                    bounds.getWidth() * .5f,
                    bounds.getHeight() * .5f
                );
                const juce::Point<float> centre(
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
                    juce::Path arrow;
                    arrow.startNewSubPath(centre);
                    arrow.lineTo(centre.x + rad.x * .25f, centre.y + rad.y * .25f);
                    arrow.lineTo(centre.x + rad.x * .125f, centre.y + rad.y * .25f);
                    arrow.lineTo(centre.x + rad.x * .125f, bounds.getBottom());
                    arrow.lineTo(centre.x - rad.x * .125f, bounds.getBottom());
                    arrow.lineTo(centre.x - rad.x * .125f, centre.y + rad.y * .25f);
                    arrow.lineTo(centre.x - rad.x * .25f, centre.y + rad.y * .25f);
                    arrow.closeSubPath();
                    g.setColour(mainCol);
                    g.strokePath(arrow, juce::PathStrokeType(2.f));
                }
            };
        }
        
        inline Button::OnPaint makeButtonOnPaintSave()
        {
            return [](juce::Graphics& g, Button& button)
            {
                const auto thicc = Shared::shared.thicc;
                const auto bounds = button.getLocalBounds().toFloat().reduced(thicc);
                g.setColour(Shared::shared.colour(ColourID::Bg));
                g.fillRoundedRectangle(bounds, thicc);
                juce::Colour mainCol, hoverCol(0x00000000);
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
                const juce::Point<float> rad(
                    bounds.getWidth() * .5f,
                    bounds.getHeight() * .5f
                );
                const juce::Point<float> centre(
                    bounds.getX() + rad.x,
                    bounds.getY() + rad.y
                );
                if (!hoverCol.isTransparent())
                {
                    g.setColour(hoverCol);
                    g.fillRoundedRectangle(bounds, thicc);
                }
                {
                    juce::Path path;
                    juce::Line<float> arrowLine(
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
                    g.strokePath(path, juce::PathStrokeType(thicc));
                }
            };
        }
        
        inline Button::OnPaint makeButtonOnPaintDirectory()
        {
            return [](juce::Graphics& g, Button& button)
            {
                const auto thicc = Shared::shared.thicc;
                const auto bounds = button.getLocalBounds().toFloat().reduced(thicc);
                g.setColour(Shared::shared.colour(ColourID::Bg));
                g.fillRoundedRectangle(bounds, thicc);
                juce::Colour mainCol, hoverCol(0x00000000);
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
                const juce::Point<float> rad(
                    bounds.getWidth() * .5f,
                    bounds.getHeight() * .5f
                );
                const juce::Point<float> centre(
                    bounds.getX() + rad.x,
                    bounds.getY() + rad.y
                );
                if (!hoverCol.isTransparent())
                {
                    g.setColour(hoverCol);
                    g.fillRoundedRectangle(bounds, thicc);
                }
                {
                    juce::Path path;
                    const juce::Point<float> margin(thicc, thicc);
                    const auto margin2 = margin * 2.f;
                    
                    const auto bottomLeft = bounds.getBottomLeft();
                    const juce::Point<float> origin(
                        bottomLeft.x + margin2.x,
                        bottomLeft.y - margin2.y * 2.f
                    );
                    path.startNewSubPath(origin);
                    const auto xShift = rad.x * .4f;
                    const auto yIdk = centre.x - rad.x * .1f;
                    path.lineTo(
                        path.getCurrentPosition().x + xShift,
                        yIdk
                    );
                    const auto rightest = bounds.getRight() - margin2.x;
                    path.lineTo(
                        rightest,
                        yIdk
                    );
                    const auto almostRightest = path.getCurrentPosition().x - xShift;
                    path.lineTo(
                        almostRightest,
                        origin.y
                    );
                    path.lineTo(
                        origin.x,
                        origin.y
                    );
                    const auto topLeft = bounds.getTopLeft();
                    const auto yShift = rad.y * .2f;
                    const juce::Point<float> upperLeftCorner(
                        origin.x,
                        topLeft.y + yShift * 2.f
                    );
                    const auto xShiftMoar = rad.x * .25f;
                    path.lineTo(upperLeftCorner);
                    path.lineTo(
                        path.getCurrentPosition().x + xShift + xShiftMoar,
                        upperLeftCorner.y
                    );
                    const auto xShift4000 = rad.x * .1f;
                    path.lineTo(
                        path.getCurrentPosition().x + xShift4000,
                        yIdk - yShift
                    );
                    path.lineTo(
                        almostRightest,
                        path.getCurrentPosition().y
                    );
                    path.lineTo(
                        almostRightest,
                        yIdk
                    );

                    g.setColour(mainCol);
                    const juce::PathStrokeType strokeType(
                        thicc,
                        juce::PathStrokeType::JointStyle::curved,
                        juce::PathStrokeType::EndCapStyle::rounded
                    );
                    g.strokePath(path, strokeType);
                }
            };
        }

        struct Lock :
            public Comp
        {
            static constexpr float LockAlpha = .4f;

            inline juce::Colour col(juce::Colour c)
            {
                return c.withMultipliedAlpha(locked ? LockAlpha : 1.f);
            }

            Lock(Utils& u, Comp* _comp, juce::String&& _id) :
                Comp(u, "Click here to (un)lock this component."),
                comp(_comp),
                id(toID(_id)),
                locked(false)
            {
                auto& state = u.getState();
                const juce::Identifier locksID("locks");
                auto child = state.getChildWithName(locksID);
                if (!child.isValid())
                {
                    child = juce::ValueTree(locksID);
                    state.appendChild(child, nullptr);
                }
                locked = static_cast<int>(child.getProperty(id, 0)) == 0 ? false : true;
            }
            bool isLocked() { return locked; }
            void switchLock()
            {
                locked = !locked;
                auto& state = this->utils.getState();
                const juce::Identifier locksID("locks");
                auto child = state.getChildWithName(locksID);
                if (!child.isValid())
                {
                    child = juce::ValueTree(locksID);
                    state.appendChild(child, nullptr);
                }
                child.setProperty(id, locked ? 1 : 0, nullptr);
            }
        protected:
            Comp* comp;
            juce::Identifier id;
            bool locked;

            void paint(juce::Graphics& g) override
            {
                const auto thicc = Shared::shared.thicc;
                const auto bounds = getLocalBounds().toFloat().reduced(thicc);
                const juce::Point<float> centre(
                    bounds.getX() + bounds.getWidth() * .5f,
                    bounds.getY() + bounds.getHeight() * .5f
                );

                if (isMouseOver())
                {
                    g.setColour(Shared::shared.colour(ColourID::Hover));
                    g.fillRoundedRectangle(bounds, thicc);
                    if (isMouseButtonDown())
                        g.fillRoundedRectangle(bounds, thicc);
                    g.setColour(Shared::shared.colour(ColourID::Interact));
                }
                else
                    g.setColour(juce::Colour(0xff999999));

                juce::Rectangle<float> bodyArea, arcArea;
                {
                    auto x = bounds.getX();
                    auto w = bounds.getWidth();
                    auto h = bounds.getHeight() * .6f;
                    auto y = bounds.getBottom() - h;
                    bodyArea.setBounds(x, y, w, h);
                    g.fillRoundedRectangle(bodyArea, thicc);
                }
                {
                    const juce::Point<float> rad(
                        bounds.getWidth() * .5f - thicc,
                        bounds.getHeight() * .5f - thicc
                    );
                    juce::Path arc;
                    arc.addCentredArc(centre.x, centre.y, rad.x, rad.y, 0.f, -piHalf, piHalf, true);
                    g.strokePath(arc, juce::PathStrokeType(thicc));
                }
            }

            void mouseEnter(const juce::MouseEvent& evt) override
            {
                Comp::mouseEnter(evt);
                repaint();
            }
            void mouseExit(const juce::MouseEvent&) override
            {
                repaint();
            }
            void mouseDown(const juce::MouseEvent&) override
            {
                repaint();
            }
            void mouseUp(const juce::MouseEvent& evt) override
            {
                if (!evt.mouseWasDraggedSinceMouseDown())
                {
                    switchLock();
                    comp->repaint();
                }
                repaint();
            }
        };

        enum class ParameterType { Knob, Switch, NumTypes };

        static constexpr float SensitiveDrag = .2f;
        static constexpr float WheelDefaultSpeed = .02f;
        static constexpr float WheelInertia = .9f;

        class Paramtr :
            public Comp,
            public juce::Timer
        {
            static constexpr float AngleWidth = piQuart * 3.f;
            static constexpr float AngleRange = AngleWidth * 2.f;

            static constexpr float LockAlpha = .4f;

            using OnPaint = std::function<void(juce::Graphics&)>;

            inline Notify makeNotify(Paramtr& parameter, Utils& _utils)
            {
                return [&p = parameter, &u = _utils](int t, const void*)
                {
                    if (t == NotificationType::ModSelectionChanged)
                    {
                        const auto selectedMod = u.getSelectedMod();
                        const auto selectedModIdx = selectedMod.idx;
                        auto& modDial = p.getModDial();
                        const auto selectedChanged = modDial.updateSetSelected(selectedModIdx);
                        if (selectedChanged)
                        {
                            p.updateConnecDepth();
                            p.repaint();
                        }
                    }
                    else if (t == NotificationType::ConnectionDisabled)
                    {
                        const auto selectedMod = u.getSelectedMod();
                        const auto connecIdx = u.getConnecIdxWith(selectedMod.idx, p.getPID());
                        if (connecIdx != -1) return false;
                        p.getModDial().updateSetSelected(selectedMod.idx);
                        p.repaint();
                    }
                    else if (t == NotificationType::ConnectionEnabled)
                    {
                        const auto selectedMod = u.getSelectedMod();
                        const auto connecIdx = u.getConnecIdxWith(selectedMod.idx, p.getPID());
                        if (connecIdx == -1) return false;
                        p.getModDial().updateSetSelected(selectedMod.idx);
                        p.updateConnecDepth();
                        p.repaint();
                    }
                    else if (t == NotificationType::ConnectionDepthChanged)
                    {
                        if (p.updateConnecDepth())
                            p.repaint();
                    }
                    else if (t == NotificationType::PatchUpdated)
                    {
                        auto& modDial = p.getModDial();
                        const auto selectedModIdx = p.utils.getSelectedModIdx();
                        if (modDial.updateSetSelected(selectedModIdx))
                        {
                            p.updateConnecDepth();
                            p.repaint();
                        }
                    }
                    return false;
                };
            }

            struct ModDial :
                public Comp
            {
                ModDial(Utils& u, Paramtr& _paramtr) :
                    Comp(u, "Drag to modulate parameter. Rightclick to remove modulator.", CursorType::Mod),
                    paramtr(_paramtr),
                    dragY(0.f),
                    connecIdx(u.getConnecIdxWith(u.getSelectedModIdx(), paramtr.getPID())),
                    tryRemove(false)
                {
                    setBufferedToImage(false);
                }
                bool updateSetSelected(int selectedModIdx) noexcept
                {
                    const auto c = this->utils.getConnecIdxWith(selectedModIdx, paramtr.getPID());
                    if (connecIdx != c)
                    {
                        select(c);
                        repaint();
                        return true;
                    }
                    return false;
                }
                void select(int _connecIdx) noexcept { connecIdx = _connecIdx; }
                const bool isSelected() const noexcept { return connecIdx != -1; }
                const bool isTryingToRemove() const noexcept { return tryRemove; }
                const int getConnecIdx() const noexcept { return connecIdx; }
            protected:
                Paramtr& paramtr;
                float dragY;
                int connecIdx;
                bool tryRemove;

                void paint(juce::Graphics& g) override
                {
                    const auto thicc = Shared::shared.thicc;
                    const auto bounds = getLocalBounds().toFloat().reduced(thicc);
                    g.setColour(Shared::shared.colour(ColourID::Bg));
                    g.fillEllipse(bounds);
                    juce::String txt;
                    if (isSelected())
                        if (tryRemove)
                        {
                            g.setColour(Shared::shared.colour(ColourID::Abort));
                            txt = "!";
                        }
                        else
                        {
                            g.setColour(paramtr.col(Shared::shared.colour(ColourID::Mod)));
                            txt = "M";
                        }
                            
                    else
                    {
                        g.setColour(paramtr.col(Shared::shared.colour(ColourID::Interact)));
                        txt = "M";
                    } 
                    g.drawEllipse(bounds, thicc);
                    {
                        const auto font = g.getCurrentFont();
                        const auto strWidth = font.getStringWidthFloat(txt);
                        const auto nHeight = .5f * font.getHeight() * bounds.getWidth() / strWidth;
                        g.setFont(nHeight);
                        g.drawFittedText(txt, bounds.toNearestInt(), juce::Justification::centred, 1);
                    }
                }

                void mouseDown(const juce::MouseEvent& evt) override
                {
                    if (paramtr.isLocked()) return;
                    dragY = evt.position.y / this->utils.getDragSpeed();
                    updateSetSelected(this->utils.getSelectedModIdx());
                }
                void mouseDrag(const juce::MouseEvent& evt) override
                {
                    if (!isSelected() || paramtr.isLocked()) return;
                    const auto dragYNew = evt.position.y / this->utils.getDragSpeed();
                    const auto sensitive = evt.mods.isShiftDown() ? SensitiveDrag : 1.f;
                    const auto dragMove = (dragYNew - dragY) * sensitive;
                    const auto depth = juce::jlimit(-1.f, 1.f, this->utils.getConnecDepth(connecIdx) - dragMove);
                    this->utils.setConnecDepth(connecIdx, depth);
                    dragY = dragYNew;
                }
                void mouseUp(const juce::MouseEvent& evt) override {
                    if (evt.mouseWasDraggedSinceMouseDown()) return;
                    else if (evt.mods.isLeftButtonDown()) return;
                    else if (!isSelected()) return;
                    // right clicks only
                    if (paramtr.isLocked()) return;
                    if (!tryRemove)
                        tryRemove = true;
                    else
                    {
                        this->utils.disableConnection(connecIdx);
                        tryRemove = false;
                    }
                    repaintWithChildren(getParentComponent());
                }
                void mouseExit(const juce::MouseEvent&) override
                {
                    tryRemove = false;
                    repaintWithChildren(getParentComponent());
                }
            };
        public:
            Paramtr(Utils& u, juce::String&& _name, juce::String&& _tooltip, PID _pID, std::vector<Paramtr*>& modulatables, ParameterType _pType = ParameterType::Knob) :
                juce::Timer(),
                Comp(u, makeNotify(*this, u), std::move(_tooltip)),
                param(*u.getParam(_pID)),
                pType(_pType),
                label(u, std::move(_name)),
                modDial(u, *this),
                lockr(u, this, toString(getPID())),
                attachedModSelected(u.getSelectedMod() == param.attachedMod),
                valNorm(0.f), valSum(0.f), connecDepth(u.getConnecDepth(modDial.getConnecIdx())), dragY(0.f),
                onPaint(nullptr)
            {
                init(modulatables);
            }
            
            Paramtr(Utils& u, juce::String&& _name, juce::String&& _tooltip, int _pID, std::vector<Paramtr*>& modulatables, ParameterType _pType = ParameterType::Knob) :
                juce::Timer(),
                Comp(u, makeNotify(*this, u), std::move(_tooltip)),
                param(*u.getParam(static_cast<PID>(_pID))),
                pType(_pType),
                label(u, std::move(_name)),
                modDial(u, *this),
                lockr(u, this, toString(getPID())),
                attachedModSelected(u.getSelectedMod() == param.attachedMod),
                valNorm(0.f), valSum(0.f), connecDepth(u.getConnecDepth(modDial.getConnecIdx())), dragY(0.f),
                onPaint(nullptr)
            {
                init(modulatables);
            }

            PID getPID() const noexcept { return param.id; }
            ModTypeContext getAttachedMod() const noexcept { return param.attachedMod; }
            bool isAttachedModSelected() const noexcept { return attachedModSelected; }
            void setAttachedModSelected(bool e) noexcept { attachedModSelected = e; }
            juce::String getValueAsText() const noexcept { return param.getText(param.getValue(), 10); }
            ModDial& getModDial() noexcept { return modDial; }
            
            bool updateConnecDepth()
            {
                const auto cIdx = modDial.getConnecIdx();
                const auto d = this->utils.getConnecDepth(cIdx);
                if (connecDepth != d)
                {
                    connecDepth = d;
                    return true;
                }
                return false;
            }
            
            bool isLocked()
            {
                return lockr.isLocked();
            }
            
            void switchLock()
            {
                lockr.switchLock();
            }
            
            OnPaint onPaint;
        protected:
            Param& param;
            const ParameterType pType;
            Label label;
            ModDial modDial;
            Lock lockr;
            bool attachedModSelected;
            float valNorm, valSum, connecDepth, dragY;

            void init(std::vector<Paramtr*>& modulatables)
            {
                setName(label.getText());
                if (pType == ParameterType::Knob)
                {
                    addAndMakeVisible(label);
                    modulatables.push_back(this);
                    addAndMakeVisible(modDial);
                }
                addAndMakeVisible(lockr);
                startTimerHz(24);
            }

            void timerCallback() override
            {
                switch (pType)
                {
                case ParameterType::Knob: return callbackKnob();
                case ParameterType::Switch: return callbackSwitch();
                }
            }
            
            void callbackKnob()
            {
                const auto vn = param.getValue();
                const auto vns = param.getValueSum();
                if (valNorm != vn || valSum != vns)
                {
                    if (lockr.isLocked())
                    {
                        param.setValueWithGesture(valNorm);
                    }
                    else
                    {
                        valSum = vns;
                        valNorm = vn;
                        repaint();
                    }
                }
            }
            
            void callbackSwitch()
            {
                const auto vn = param.getValue();
                if (valNorm != vn)
                {
                    if (lockr.isLocked())
                    {
                        param.setValueWithGesture(valNorm);
                    }
                    else
                    {
                        valNorm = vn;
                        repaint();
                    }
                }
            }

            void paint(juce::Graphics& g) override
            {
                switch (pType)
                {
                case ParameterType::Knob: return paintKnob(g);
                case ParameterType::Switch: return paintSwitch(g);
                }
            }
            
            void paintKnob(juce::Graphics& g)
            {
                const auto thicc = Shared::shared.thicc;
                const auto thicc2 = thicc * 2.f;
                const auto thicc4 = thicc * 4.f;
                const auto bounds = maxQuadIn(getLocalBounds().toFloat()).reduced(thicc2);
                const juce::PathStrokeType strokeType(thicc, juce::PathStrokeType::JointStyle::curved, juce::PathStrokeType::EndCapStyle::rounded);
                const auto radius = bounds.getWidth() * .5f;
                const auto radiusInner = radius - thicc2;
                juce::Point<float> centre(
                    radius + bounds.getX(),
                    radius + bounds.getY()
                );

                //draw outline
                {
                    g.setColour(col(Shared::shared.colour(ColourID::Interact)));
                    juce::Path outtaArc;

                    outtaArc.addCentredArc(
                        centre.x, centre.y,
                        radius, radius,
                        0.f,
                        -AngleWidth, AngleWidth,
                        true
                    );
                    outtaArc.addCentredArc(
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
                    const auto modCol = col(!modDial.isTryingToRemove() ? Shared::shared.colour(ColourID::Mod) : Shared::shared.colour(ColourID::Abort));
                    const auto valSumAngle = valSum * AngleRange;
                    const auto sumAngle = -AngleWidth + valSumAngle;
                    const auto sumTick = juce::Line<float>::fromStartAndAngle(centre, radiusExt, sumAngle);
                    g.setColour(Shared::shared.colour(ColourID::Bg));
                    g.drawLine(sumTick, thicc4);
                    g.setColour(modCol);
                    g.drawLine(sumTick.withShortenedStart(radiusInner), thicc2);

                    const auto connecDepthAngle = juce::jlimit(-AngleWidth, AngleWidth, valAngle + connecDepth * AngleRange);
                    if (modDial.isSelected())
                    {
                        juce::Path modPath;
                        modPath.addCentredArc(
                            centre.x, centre.y,
                            radius, radius,
                            0.f,
                            valAngle, connecDepthAngle,
                            true
                        );
                        g.strokePath(modPath, strokeType);
                    }
                }
                // draw tick
                {
                    const auto tickLine = juce::Line<float>::fromStartAndAngle(centre, radiusExt, valAngle);
                    g.setColour(Shared::shared.colour(ColourID::Bg));
                    g.drawLine(tickLine, thicc4);
                    g.setColour(col(Shared::shared.colour(ColourID::Interact)));
                    g.drawLine(tickLine.withShortenedStart(radiusInner), thicc2);
                }
            }
            
            void paintSwitch(juce::Graphics& g)
            {
                const auto thicc = Shared::shared.thicc;
                const auto bounds = getLocalBounds().toFloat().reduced(thicc);
                
                if (isMouseOver())
                {
                    if (!isLocked())
                    {
                        g.setColour(Shared::shared.colour(ColourID::Hover));
                        g.fillRoundedRectangle(bounds, thicc);
                        if (isMouseButtonDown())
                            g.fillRoundedRectangle(bounds, thicc);
                    }
                    g.setColour(col(Shared::shared.colour(ColourID::Interact)));
                }
                else
                    g.setColour(col(Shared::shared.colour(ColourID::Txt)));
                g.drawRoundedRectangle(bounds, thicc, thicc);
                if(onPaint == nullptr)
                    g.drawFittedText(param.getCurrentValueAsText(), bounds.toNearestInt(), juce::Justification::centred, 1);
				else
					onPaint(g);
            }

            void resized() override
            {
                switch (pType)
                {
                case ParameterType::Knob: return resizedKnob();
                case ParameterType::Switch: return resizedSwitch();
                }
            }
            
            void resizedKnob()
            {
                const auto thicc = Shared::shared.thicc;
                const auto bounds = getLocalBounds().toFloat().reduced(thicc);
                label.setBounds(bounds.toNearestInt());
                {
                    const auto w = bounds.getWidth() * .33333f;
                    const auto h = bounds.getHeight() * .33333f;
                    const auto x = bounds.getX() + (bounds.getWidth() - w) * .5f;
                    const auto y = bounds.getY() + (bounds.getHeight() - h);
                    const juce::Rectangle<float> dialArea(x, y, w, h);
                    modDial.setBounds(maxQuadIn(dialArea).toNearestInt());
                }
                {
                    const auto w = modDial.getWidth() * pi / 4;
                    const auto h = w;
                    const auto x = getWidth() - w;
                    const auto y = 0;
                    lockr.setBounds(juce::Rectangle<float>(x, y, w, h).toNearestInt());
                }
            }
            
            void resizedSwitch()
            {
                const auto thicc = Shared::shared.thicc;
                const auto bounds = getLocalBounds().toFloat().reduced(thicc);
                {
                    const auto w = std::min(getWidth(), getHeight()) / 3;
                    const auto h = w;
                    const auto x = getWidth() - w;
                    const auto y = 0;
                    lockr.setBounds(x, y, w, h);
                }
            }

            void mouseEnter(const juce::MouseEvent& evt) override
            {
                Comp::mouseEnter(evt);
                this->notify(NotificationType::ParameterHovered, this);
                if (pType == ParameterType::Switch)
                    repaint();
            }
            
            void mouseExit(const juce::MouseEvent&) override
            {
                if (pType == ParameterType::Switch)
                    repaint();
            }
            
            void mouseDown(const juce::MouseEvent& evt) override
            {
                if(!isLocked())
                    switch (pType)
                    {
                    case ParameterType::Knob:
                        if (evt.mods.isLeftButtonDown())
                        {
                            notify(NotificationType::KillEnterValue);
                            param.beginGesture();
                            dragY = evt.position.y / utils.getDragSpeed();
                        }
                        return;
                    case ParameterType::Switch:
                        notify(NotificationType::KillEnterValue);
                        return repaint();
                    }
            }
            
            void mouseDrag(const juce::MouseEvent& evt) override
            {
                if (isLocked()) 
                    return;
                
                if (pType != ParameterType::Knob)
                    return;
                
                if (evt.mods.isLeftButtonDown())
                {
                    auto mms = juce::Desktop::getInstance().getMainMouseSource();
                    mms.enableUnboundedMouseMovement(true, false);
                    
                    const auto dragYNew = evt.position.y / utils.getDragSpeed();
                    auto dragOffset = dragYNew - dragY;
                    if (evt.mods.isShiftDown())
                        dragOffset *= SensitiveDrag;
                    const auto newValue = juce::jlimit(0.f, 1.f, param.getValue() - dragOffset);
                    param.setValueNotifyingHost(newValue);
                    dragY = dragYNew;
                    notify(NotificationType::ParameterDragged, this);
                }
            }
            
            void mouseUp(const juce::MouseEvent& evt) override
            {
                if(!isLocked())
                    switch (pType)
                    {
                    case ParameterType::Knob: return mouseUpKnob(evt);
                    case ParameterType::Switch: return mouseUpSwitch(evt);
                    }
            }
            
            void mouseWheelMove(const juce::MouseEvent& evt, const juce::MouseWheelDetails& wheel) override
            {
                if (isLocked() || evt.mods.isAnyMouseButtonDown()) return;
                
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

            void mouseDoubleClick(const juce::MouseEvent&) override
            {
                if (pType == ParameterType::Knob)
                {
                    param.setValueWithGesture(param.getDefaultValue());
                    notify(NotificationType::ParameterDragged, this);
                }
            }

            void mouseUpKnob(const juce::MouseEvent& evt)
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
                        const juce::Point<int> centre(getWidth() / 2, getHeight() / 2);
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

            void mouseUpSwitch(const juce::MouseEvent& evt)
            {
                if (evt.mods.isLeftButtonDown())
                {
                    if (!evt.mouseWasDraggedSinceMouseDown())
                        if (evt.mods.isCtrlDown())
                            param.setValueWithGesture(param.getDefaultValue());
                        else
                            param.setValueWithGesture(1.f - param.getValue());
                    notify(NotificationType::ParameterDragged, this);
                }
                repaint();
            }

            juce::Colour col(juce::Colour c)
            {
                return lockr.col(c);
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
                        const auto mod = m.getMod();
                        const bool isSelected = mod == u.getSelectedMod();
                        if (m.isSelected() == isSelected) return false;
                        m.setSelected(isSelected);
                        m.repaint();
                    }
                    return false;
                };
            }

            ModDragger(Utils& u, ModType _mt, int _mtIdx, std::vector<Paramtr*>& _modulatables) :
                Comp(u, makeNotify(*this, u), "Drag this modulator to any parameter.", CursorType::Mod),
                mtc({ _mt, _mtIdx }),
                hoveredParameter(nullptr),
                modulatables(_modulatables),
                origin(),
                draggerfall(),
                selected(mtc == u.getSelectedMod())
            {}
            
            void setQBounds(const juce::Rectangle<float>& b)
            {
                origin = maxQuadIn(b).toNearestInt();
                setBounds(origin);
            }

            ModTypeContext getMod() const noexcept { return mtc; }
            bool isSelected() const noexcept { return selected; }
            void setSelected(bool e) noexcept { selected = e; }
        protected:
            ModTypeContext mtc;
            Paramtr* hoveredParameter;
            std::vector<Paramtr*>& modulatables;
            juce::Rectangle<int> origin;
            juce::ComponentDragger draggerfall;
            bool selected;

            void paint(juce::Graphics& g) override
            {
                if (hoveredParameter == nullptr)
                    if (selected)
                        g.setColour(Shared::shared.colour(ColourID::Mod));
                    else
                        g.setColour(Shared::shared.colour(ColourID::Inactive));
                else
                    g.setColour(Shared::shared.colour(ColourID::Interact));

                const auto thicc = Shared::shared.thicc;
                const auto tBounds = getLocalBounds().toFloat();
                juce::Point<float> centre(tBounds.getX() + tBounds.getWidth() * .5f, tBounds.getY() + tBounds.getHeight() * .5f);
                const auto arrowHead = tBounds.getWidth() * .25f;
                g.drawArrow(juce::Line<float>(centre, { centre.x, tBounds.getBottom() }), thicc, arrowHead, arrowHead);
                g.drawArrow(juce::Line<float>(centre, { tBounds.getX(), centre.y }), thicc, arrowHead, arrowHead);
                g.drawArrow(juce::Line<float>(centre, { centre.x, tBounds.getY() }), thicc, arrowHead, arrowHead);
                g.drawArrow(juce::Line<float>(centre, { tBounds.getRight(), centre.y }), thicc, arrowHead, arrowHead);
            }

            void mouseDown(const juce::MouseEvent& evt) override
            {
                this->utils.selectMod(mtc);
                draggerfall.startDraggingComponent(this, evt);
            }
            
            void mouseDrag(const juce::MouseEvent& evt) override
            {
                draggerfall.dragComponent(this, evt, nullptr);
                hoveredParameter = getHoveredParameter();
                repaint();
            }
            
            void mouseUp(const juce::MouseEvent&) override
            {
                if (hoveredParameter != nullptr)
                {
                    const auto pID = hoveredParameter->getPID();
                    const auto param = this->utils.getParam(pID);
                    const auto pValue = param->getValue();
                    const auto depth = 1.f - pValue;
                    this->utils.enableConnection(mtc, pID, depth);
                    hoveredParameter = nullptr;
                }
                setBounds(origin);
                repaint();
            }

            Paramtr* getHoveredParameter() const noexcept
            {
                for (const auto p : modulatables)
                    if (p->isShowing())
                        if(!p->isLocked())
                            if (!this->utils.getWindowBounds(*this).getIntersection(this->utils.getWindowBounds(*p)).isEmpty())
                                return p;
                return nullptr;
            }
            
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
                    modSys6::gui::CursorType::Default
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
            juce::String* tooltipPtr;

            void paint(juce::Graphics& g) override
            {
                g.setColour(Shared::shared.colour(ColourID::Txt));
                g.drawFittedText
                (
                    *tooltipPtr, getLocalBounds(), juce::Justification::bottomLeft, 1
                );
            }
        };

        class PopUp :
            public Comp,
            public juce::Timer
        {
            static constexpr int FreezeTimeInMs = 1500;

            inline Notify makeNotify(PopUp& popUp)
            {
                return [&p = popUp](int t, const void* stuff)
                {
                    if (t == NotificationType::ParameterHovered)
                    {
                        
                        const auto param = static_cast<const Paramtr*>(stuff);
                        const auto valTxt = param->getValueAsText();
                        if (valTxt.isNotEmpty())
                        {
                            const auto pt = p.utils.getWindowNearby(*param);
                            p.update(param->getName() + "\n" + valTxt, pt);
                            return true;
                        }
                        p.kill();
                        return true;
                    }
                    if (t == NotificationType::ParameterDragged)
                    {
                        const auto param = static_cast<const Paramtr*>(stuff);
                        const auto valTxt = param->getValueAsText();
                        if (valTxt.isNotEmpty())
                        {
                            p.update(param->getName() + "\n" + valTxt);
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
                Comp(u, makeNotify(*this), "", modSys6::gui::CursorType::Default),
                label(u, "", ColourID::Darken, ColourID::Transp, ColourID::Txt),
                freezeIdx(0)
            {
                setInterceptsMouseClicks(false, false);
                startTimerHz(6);
                addChildComponent(label);
            }
            void update(juce::String&& txt, juce::Point<int> pt)
            {
                setCentrePosition(pt);
                update(std::move(txt));
            }
            void update(juce::String&& txt)
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
                freezeIdx = FreezeTimeInMs + 1;
                label.setVisible(false);
            }
        protected:
            Label label;
            int freezeIdx;

            void paint(juce::Graphics& g) override
            {
                if (!label.isVisible()) return;
                g.setColour(Shared::shared.colour(ColourID::Darken));
                const auto t = Shared::shared.thicc;
                g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(t), t);
            }

            void resized() override { label.setBounds(getLocalBounds()); }

            void timerCallback() override
            {
                if (freezeIdx > FreezeTimeInMs)
                {
                    label.setVisible(false);
                    return;
                }
                freezeIdx += getTimerInterval();
            }
        };

        class EnterValueComp :
            public Comp,
            public juce::Timer
        {
            static constexpr int FPS = 4;

            inline Notify makeNotify(EnterValueComp& comp)
            {
                return [&evc = comp](int type, const void* stuff)
                {
                    if (type == NotificationType::EnterValue)
                    {
                        const auto param = static_cast<const Paramtr*>(stuff);
                        const auto pID = param->getPID();
                        evc.enable(pID, evc.utils.getWindowNearby(*param));
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
                juce::Timer(),
                txt(""),
                param(nullptr),
                initValue(0.f),
                tickIdx(0),
                drawTick(false)
            {
                setWantsKeyboardFocus(true);
            }
            
            void enable(PID pID, juce::Point<int> pt)
            {
                setCentrePosition(pt);
                param = this->utils.getParam(pID);
                initValue = param->getValue();
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
                startTimerHz(FPS);
                repaint();
            }
            
            bool isEnabled() const noexcept { return isTimerRunning(); }
            
            void disable()
            {
                stopTimer();
                setVisible(false);
            }
            
        protected:
            juce::String txt;
            Param* param;
            float initValue;
            int tickIdx;
            bool drawTick;

            bool keyPressed(const juce::KeyPress& key) override
            {
                if (key == key.escapeKey)
                {
                    disable();
                    return true;
                }
                if (key == key.returnKey)
                {
                    const auto val = juce::jlimit(0.f, 1.f, param->getValueForText(txt));
                    param->setValueWithGesture(val);
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
                    if(tickIdx > 0)
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

            void timerCallback() override
            {
                if (!hasKeyboardFocus(true))
                    return disable();
                drawTick = !drawTick;
                repaint();
            }

            void paint(juce::Graphics& g) override
            {
                const auto thicc = Shared::shared.thicc;
                const auto bounds = getLocalBounds().toFloat().reduced(Shared::shared.thicc);
                g.setColour(Shared::shared.colour(ColourID::Bg));
                g.fillRoundedRectangle(bounds, thicc);
                g.setColour(Shared::shared.colour(ColourID::Txt));
                g.drawRoundedRectangle(bounds, thicc, thicc);
                if(drawTick)
                    g.drawFittedText(
                        txt.substring(0, tickIdx) + "| " + txt.substring(tickIdx),
                        bounds.toNearestInt(), juce::Justification::centred, 1
                    );
                else
                    g.drawFittedText(
                        txt, bounds.toNearestInt(), juce::Justification::centred, 1
                    );
            }
        };

        struct BuildDate :
            public Comp
        {
            BuildDate(Utils& u) :
                Comp(u, "identifies the plugin's version by build date.")
            {
            }
        protected:
            void paint(juce::Graphics& g) override
            {
                const auto buildDate = static_cast<juce::String>(__DATE__) + " " + static_cast<juce::String>(__TIME__);
                g.setColour(Shared::shared.colour(ColourID::Txt).withAlpha(.4f));
                g.drawFittedText(buildDate, getLocalBounds(), juce::Justification::centredRight, 1);
            }
        };

        struct ParamtrRandomizer :
            public Comp
        {
            using RandFunc = std::function<void(juce::Random&)>;

            ParamtrRandomizer(Utils& u, std::vector<Paramtr*>& _randomizables, juce::String&& _id) :
                Comp(u, makeTooltip()),
                randomizables(_randomizables),
                randFuncs(),
                lock(u, this, std::move(_id))
            {
                addAndMakeVisible(lock);
            }
            
            ParamtrRandomizer(Utils& u, juce::String&& _id) :
                Comp(u, makeTooltip()),
                randomizables(),
                randFuncs(),
                lock(u, this, std::move(_id))
            {
                addAndMakeVisible(lock);
            }

            void clear()
            {
                randomizables.clear();
                randFuncs.clear();
            }
            
            void add(Paramtr* p) { randomizables.push_back(p); }
            
            void add(RandFunc&& r) { randFuncs.push_back(r); }

            void operator()(bool sensitive)
            {
                if (lock.isLocked()) return;
                juce::Random rand;
                for (auto& func : randFuncs)
                    func(rand);
                if(sensitive)
                    for (auto randomizable : randomizables)
                    {
                        const PID pID = randomizable->getPID();
                        auto param = utils.getParam(pID);

                        auto val = param->getValue();
                        val += (rand.nextFloat() - .5f) * .1f;
                        param->beginGesture();
                        param->setValueNotifyingHost(juce::jlimit(0.f, 1.f, val));
                        param->endGesture();
                    }
                else
                    for (auto randomizable : randomizables)
                    {
                        const PID pID = randomizable->getPID();
                        auto param = utils.getParam(pID);

                        param->beginGesture();
                        param->setValueNotifyingHost(rand.nextFloat());
                        param->endGesture();
                    }
            }
            
        protected:
            std::vector<Paramtr*> randomizables;
            std::vector<RandFunc> randFuncs;
            Lock lock;

            void resized() override
            {
                const auto thicc = Shared::shared.thicc;
                const auto bounds = getLocalBounds().toFloat().reduced(thicc);
                {
                    const auto w = std::min(getWidth(), getHeight()) / 3;
                    const auto h = w;
                    const auto x = getWidth() - w;
                    const auto y = 0;
                    lock.setBounds(x, y, w, h);
                }
            }

            void paint(juce::Graphics& g) override
            {
                const auto width = static_cast<float>(getWidth());
                const auto height = static_cast<float>(getHeight());
                const juce::Point<float> centre(width, height);
                auto minDimen = std::min(width, height);
                juce::Rectangle<float> bounds(
                    (width - minDimen) * .5f,
                    (height - minDimen) * .5f,
                    minDimen,
                    minDimen
                );
                const auto thicc = Shared::shared.thicc;
                bounds.reduce(thicc, thicc);
                g.setColour(lock.col(Shared::shared.colour(ColourID::Bg)));
                g.fillRoundedRectangle(bounds, thicc);
                juce::Colour mainCol;
                if (isMouseOver())
                {
                    g.setColour(lock.col(Shared::shared.colour(ColourID::Hover)));
                    g.fillRoundedRectangle(bounds, thicc);
                    mainCol = Shared::shared.colour(ColourID::Interact);
                }
                else
                    mainCol = Shared::shared.colour(ColourID::Txt);

                g.setColour(lock.col(mainCol));
                g.drawRoundedRectangle(bounds, thicc, thicc);

                minDimen = std::min(bounds.getWidth(), bounds.getHeight());
                const auto radius = minDimen * .5f;
                const auto pointSize = radius * .4f;
                const auto pointRadius = pointSize * .5f;
                const auto d4 = minDimen / 4.f;
                const auto x0 = d4 * 1.2f + bounds.getX();
                const auto x1 = d4 * 2.8f + bounds.getX();
                for (auto i = 1; i < 4; ++i) {
                    const auto y = d4 * i + bounds.getY();
                    g.fillEllipse(x0 - pointRadius, y - pointRadius, pointSize, pointSize);
                    g.fillEllipse(x1 - pointRadius, y - pointRadius, pointSize, pointSize);
                }
            }
            
            void mouseEnter(const juce::MouseEvent& evt) override
            {
                Comp::mouseEnter(evt);
                repaint();
            }
            
            void mouseUp(const juce::MouseEvent& evt) override
            {
                if (evt.mouseWasDraggedSinceMouseDown()) return;
				this->operator()(evt.mods.isShiftDown());
                tooltip = makeTooltip();
            }
            
            void mouseExit(const juce::MouseEvent&) override
            {
                tooltip = makeTooltip();
                repaint();
            }

            juce::String makeTooltip()
            {
                juce::Random rand;
                static constexpr float count = 187.f;
                const auto v = static_cast<int>(std::rint(rand.nextFloat() * count));
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
                case 25: return "It's " + (juce::Time::getCurrentTime().getHours() < 10 ? juce::String("0") + static_cast<juce::String>(juce::Time::getCurrentTime().getHours()) : static_cast<juce::String>(juce::Time::getCurrentTime().getHours())) + ":" + (juce::Time::getCurrentTime().getMinutes() < 10 ? juce::String("0") + static_cast<juce::String>(juce::Time::getCurrentTime().getMinutes()) : static_cast<juce::String>(juce::Time::getCurrentTime().getMinutes())) + " o'clock now.";
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
            public Comp,
            public juce::Timer
        {
            Visualizer(Utils& u, juce::String&& _tooltip, int _numChannels, int maxBlockSize) :
                Comp(u, std::move(_tooltip), CursorType::Default),
                juce::Timer(),

                onPaint([](juce::Graphics&, Visualizer&){}),
                onUpdate([](Buffer&) { return false; }),

                buffer(),
                numChannels(_numChannels == 1 ? 1 : 2),
                blockSize(maxBlockSize)
            {
                buffer.resize(numChannels);
                for (auto& b : buffer)
                    b.resize(blockSize, 0.f);
                startTimerHz(30);
            }

            std::function<void(juce::Graphics&, Visualizer&)> onPaint;
            std::function<bool(Buffer&)> onUpdate;

            Buffer buffer;
            const int numChannels, blockSize;
        protected:

            void paint(juce::Graphics& g) override
            {
                onPaint(g, *this);
            }

            void timerCallback() override
            {
                if(onUpdate(buffer))
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

            void paint(juce::Graphics& g) override
            {
                const auto thicc = Shared::shared.thicc;
                const auto bounds = getLocalBounds().toFloat().reduced(thicc);
                const auto posYAbs = bounds.getY() + bounds.getHeight() * posY;
                {
                    const auto x = bounds.getX();
                    const auto w = bounds.getWidth();
                    const auto h = bounds.getHeight() * .2f;
                    const auto radY = h * .5f;
                    const auto y = posYAbs - radY;
                    const juce::Rectangle<float> barBounds(x, y, w, h);
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
                dragY = evt.position.y / this->utils.getDragSpeed();
            }
            void mouseDrag(const juce::MouseEvent& evt)
            {
                const auto dragYNew = evt.position.y / this->utils.getDragSpeed();
                auto dragOffset = dragYNew - dragY;
                if (evt.mods.isShiftDown())
                    dragOffset *= SensitiveDrag;
                posY = juce::jlimit(0.f, 1.f, posY - dragOffset);
                onChange(posY);
                repaint();
                dragY = dragYNew;
            }
            void mouseWheelMove(const juce::MouseEvent& evt, const juce::MouseWheelDetails& wheel) override
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
            void addEntry(juce::String&& _tooltip,
                const Button::OnPaint& onPaint, const Button::OnClick& onClick)
            {
                entries.push_back(std::make_unique<Button>(this->utils, std::move(_tooltip)));
                auto& entry = entries.back();
                entry->onPaint = onPaint;
                entry->onClick = onClick;
                addAndMakeVisible(*entry);
                resized();
            }
            void addEntry(juce::String&& _name, juce::String&& _tooltip,
                const Button::OnClick& onClick, juce::Justification just = juce::Justification::centred)
            {
                entries.push_back(std::make_unique<Button>(this->utils, std::move(_tooltip)));
                auto& entry = entries.back();
                entry->setName(std::move(_name));
                entry->onPaint = makeTextButtonOnPaint(std::move(_name), just);
                entry->onClick = onClick;
                addAndMakeVisible(*entry);
                resized();
            }
            void clearEntries()
            {
                entries.clear();
            }
            size_t getNumEntries() const noexcept { return entries.size(); }
        protected:
            std::vector<Entry> entries;
            ScrollBar scrollBar;
            int yOffset;

            void paint(juce::Graphics& g) override
            {
                g.setColour(Shared::shared.colour(ColourID::Darken));
                g.fillRoundedRectangle(getLocalBounds().toFloat(), Shared::shared.thicc);
                if (entries.size() != 0) return;
                g.setColour(Shared::shared.colour(ColourID::Abort));
                g.setFont(Shared::shared.font);
                g.drawFittedText("Browser empty.", getLocalBounds(), juce::Justification::centred, 1);
            }

            void resized() override
            {
                if (entries.size() == 0)
                {
                    scrollBar.setVisible(false);
                    return;
                }
                const auto thicc = Shared::shared.thicc;
                const auto bounds = getLocalBounds().toFloat().reduced(thicc);
                const auto entriesSizeInv = 1.f / static_cast<float>(entries.size());
                auto entryHeight = bounds.getHeight() * entriesSizeInv;
                if(entryHeight >= MinEntryHeight)
                {
                    scrollBar.setVisible(false);
                    const auto x = bounds.getX();
                    auto y = bounds.getY();
                    const auto w = bounds.getWidth();
                    const auto h = entryHeight;
                    for (auto i = 0; i < entries.size(); ++i, y += h)
                        entries[i]->setBounds(juce::Rectangle<float>(x, y, w, h).toNearestInt());
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
                        scrollBar.setBounds(juce::Rectangle<float>(x, y, w, h).toNearestInt());
                    }
                    {
                        const auto x = entryX;
                        auto y = bounds.getY();
                        const auto w = scrollBarX;
                        const auto h = MinEntryHeight;
                        for (auto i = 0; i < entries.size(); ++i, y += h)
                            entries[i]->setBounds(juce::Rectangle<float>(
                                x,
                                y - yOffset,
                                w,
                                h
                            ).toNearestInt());
                    }
                }
            }
        };

        struct PresetBrowser :
            public Comp,
            public juce::Timer
        {
            using SavePatch = std::function<juce::ValueTree()>;
            using LoadPatch = std::function<void()>;

            PresetBrowser(Utils& u, juce::PropertiesFile& prop) :
                Comp(u, "", CursorType::Default),
                openCloseButton(u, "Click here to open or close the preset browser."),
                saveButton(u, "Click here to manifest the current preset into the browser with this name."),
                pathButton(u, "This button will lead you to your preset files."),
                browser(u),
                presetNameEditor("Enter Preset Name.."),

                user(prop),
                presetsPath(getPresetsPath()),
                savePatch()
            {
                juce::File direc(presetsPath);
                direc.createDirectory();

                setInterceptsMouseClicks(false, true);
                setBufferedToImage(false);
                addChildComponent(browser);
                addChildComponent(presetNameEditor);
                addChildComponent(saveButton);
                addChildComponent(pathButton);
                openCloseButton.onPaint = makeButtonOnPaintBrowse();
                openCloseButton.onClick = [this]()
                {
                    const bool e = !browser.isVisible();
                    setBrowserOpen(e);
                };

                pathButton.onPaint = makeButtonOnPaintDirectory();
                pathButton.onClick = [this]()
                {
                    juce::URL url(presetsPath);
                    url.launchInDefaultBrowser();
                };

                saveButton.onPaint = makeButtonOnPaintSave();
                saveButton.onClick = [this]()
                {
                    // save preset to list of presets
                    auto pName = presetNameEditor.getText();
                    if (!pName.endsWith(".nel"))
                        pName += juce::String(".nel");
                    juce::File pFile(presetsPath + juce::File::getSeparatorString() + pName);
                    if (pFile.exists())
                        pFile.deleteFile();
                    const auto state = savePatch();
                    pFile.appendText(
                        state.toXmlString(),
                        false,
                        false
                    );
                    refreshBrowser();
                };
            }
            void init(juce::Component* c)
            {
                c->addAndMakeVisible(openCloseButton);
            }
            const Button& getOpenCloseButton() const noexcept { return openCloseButton; }
            Button& getOpenCloseButton() noexcept { return openCloseButton; }

            SavePatch savePatch;
        protected:
            Button openCloseButton, saveButton, pathButton;
            Browser browser;
            juce::TextEditor presetNameEditor;

            juce::PropertiesFile& user;
            juce::String presetsPath;

            void setBrowserOpen(bool e)
            {
                browser.setVisible(e);
                presetNameEditor.setVisible(e);
                saveButton.setVisible(e);
                pathButton.setVisible(e);
                if (e)
                {
                    initBrowser();
                    startTimerHz(4);
                }
                else
                {
                    browser.clearEntries();
                    stopTimer();
                }
            }
        private:
            void paint(juce::Graphics&) override {}
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
                    presetNameEditor.setBounds(juce::Rectangle<float>(x, y, w, h).toNearestInt());
                }
                {
                    auto x = presetNameWidth;
                    auto y = bounds.getY();
                    auto w = (bounds.getWidth() - presetNameWidth) * .5f;
                    auto h = titleHeight;
                    saveButton.setBounds(maxQuadIn(juce::Rectangle<float>(x, y, w, h)).toNearestInt());
                    x += w;
                    pathButton.setBounds(maxQuadIn(juce::Rectangle<float>(x, y, w, h)).toNearestInt());
                }
                {
                    auto x = bounds.getX();
                    auto y = titleHeight;
                    auto w = bounds.getWidth();
                    auto h = bounds.getHeight() - titleHeight;
                    browser.setBounds(juce::Rectangle<float>(x, y, w, h).toNearestInt());
                }
            }
            void initBrowser()
            {
                const juce::File directory(presetsPath);
                const auto fileTypes = juce::File::TypesOfFileToFind::findFiles;
                const juce::String extension(".nel");
                const auto wildCard = "*" + extension;
                const juce::RangedDirectoryIterator files(
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
                        [this, file]()
                        {
                            utils.updatePatch(file.loadFileAsString());
                            notify(NotificationType::PatchUpdated);
                            //setBrowserOpen(false);
                        },
                        juce::Justification::left
                    );
                }
            }

            void refreshBrowser()
            {
                browser.clearEntries();
                initBrowser();
                resized();
            }

            juce::String getPresetsPath()
            {
                const auto& file = user.getFile();
                const auto pathStr = file.getFullPathName();
                for (auto i = pathStr.length() - 1; i > 0; --i)
                    if (pathStr.substring(i, i + 1) == juce::File::getSeparatorString())
                        return pathStr.substring(0, i + 1) + "Presets";
                return "";
            }

            void timerCallback() override
            {
                juce::File directory(presetsPath);
                const auto numFiles = directory.getNumberOfChildFiles(
                    juce::File::TypesOfFileToFind::findFiles,
                    "*nel"
                );
                if (numFiles != browser.getNumEntries())
                    refreshBrowser();
            }
        };

        inline std::function<void(juce::Graphics&, Visualizer&)> makeVibratoVisualizerOnPaint()
        {
            return[](juce::Graphics& g, Visualizer& v)
            {
                const auto thicc = Shared::shared.thicc;
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
                    const auto smpl = juce::jlimit(0.f, 1.f, v.buffer[0][0] * .5f + .5f);
                    const auto X = bounds.getX() + smpl * bounds.getWidth();
                    const auto x = X - tickThicc;
                    g.fillRect(x, y, w, h);
                }
                if (v.numChannels == 2)
                {
                    const auto smpl = juce::jlimit(0.f, 1.f, v.buffer[1][0] * .5f + .5f);
                    const auto X = bounds.getX() + smpl * bounds.getWidth();
                    const auto x = X - tickThicc;
                    g.fillRect(x, y, w, h);
                }
                g.drawRoundedRectangle(bounds, thicc, thicc);
            };
        }

        inline std::function<void(juce::Graphics&, Visualizer&)> makeVibratoVisualizerOnPaint2()
        {
            return[](juce::Graphics& g, Visualizer& v)
            {
                const auto thicc = Shared::shared.thicc;
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
                    auto smpl = juce::jlimit(0.f, 1.f, v.buffer[0][0] * .5f + .5f);
                    auto X = bounds.getX() + smpl * bounds.getWidth();
                    auto x0 = X - tickThicc;
                    if (v.numChannels == 1)
                    {
                        g.fillRect(x0, y, w, h);
                    }
                    else
                    {
                        smpl = juce::jlimit(0.f, 1.f, v.buffer[1][0] * .5f + .5f);
                        X = bounds.getX() + smpl * bounds.getWidth();
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
}

/*

browser
    every entry has vector of optional additional buttons (like delete, rename)

presetbrowser
    tag system for presets

ParamtrRandomizer
    should randomize?
        vibrato mods
            modtype
        parameter-modulations
            depth
            add + remove

Paramtr
    sometimes lock not serialized (on load?)
    sensitive drag shift only changable mid-drag in standalone

*/