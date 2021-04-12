#pragma once
#include <JuceHeader.h>

class QuickAccessWheel :
    public nelG::Comp
{
    enum class State { Def, Drag };
    
    struct Button :
        public nelG::Comp
    {
        Button(QuickAccessWheel& w, param::ID id, const juce::String& s) :
            nelG::Comp(w.processor, w.utils, s),
            wheel(w),
            name(param::getName(id))
        {
            qaFont = utils.font;
            qaFont.setHeight(11);
        }
    private:
        QuickAccessWheel& wheel;
        juce::String name;
        juce::Font qaFont;
        void paint(juce::Graphics& g) {
            const auto bounds = getLocalBounds().toFloat().reduced(util::Thicc);
            g.setFont(qaFont);
            if (wheel.state == State::Def) {
                g.setColour(juce::Colour(util::ColGreen));
                g.drawRoundedRectangle(bounds, util::Rounded, util::Thicc);
                g.drawFittedText(name + "\n" + wheel.param->getCurrentValueAsText(), bounds.toNearestInt(), juce::Justification::centred, 1, 1);
            }
            else {
                if (wheel.selectedChoice == -1) {
                    g.setColour(juce::Colour(util::ColRed));
                    g.drawFittedText("X", getLocalBounds(), juce::Justification::centred, 1, 1);
                    g.drawRoundedRectangle(bounds, util::Rounded, util::Thicc);
                    return;
                }
                g.setColour(juce::Colour(util::ColGreen));
                g.drawRoundedRectangle(bounds, util::Rounded, util::Thicc);
                g.drawFittedText(name + "\n" + wheel.param->getCurrentValueAsText(), bounds.toNearestInt(), juce::Justification::centred, 1, 1);
            }
            
        }
        void mouseDown(const juce::MouseEvent&) override {
            wheel.state = State::Drag;
            wheel.selectedChoice = -1;
            for (auto c = 0; c < wheel.choices.size(); ++c)
                wheel.choices[c].get()->setVisible(true);
            wheel.repaint();
        }
        void mouseDrag(const juce::MouseEvent& evt) override {
            const auto width = static_cast<float>(getWidth());
            const auto height = static_cast<float>(getHeight());
            juce::Point<float> centre(width * .5f, height * .5f);
            juce::Line<float> dragLine(centre, evt.position);
            if (dragLine.getLength() < wheel.minDimen)
                wheel.selectedChoice = -1;
            else {
                auto angle = dragLine.getAngle();
                angle -= wheel.startAngle;
                while (angle < 0) angle += util::Tau;
                if (angle < wheel.angleRange) {
                    const auto size = static_cast<float>(wheel.choices.size());
                    const auto value = size * angle / wheel.angleRange;
                    const auto rValue = static_cast<int>(value);
                    wheel.selectedChoice = juce::jlimit(0, static_cast<int>(wheel.choices.size()) - 1, rValue);
                }
                else if (angle < wheel.angleRange + wheel.overshootAngle)
                    wheel.selectedChoice = static_cast<int>(wheel.choices.size()) - 1;
                else if (angle > util::Tau - wheel.overshootAngle)
                    wheel.selectedChoice = 0;
                else
                    wheel.selectedChoice = -1;
            }
            wheel.repaint();
        }
        void mouseUp(const juce::MouseEvent& evt) override {
            wheel.state = State::Def;
            if (evt.mouseWasDraggedSinceMouseDown() && wheel.selectedChoice != -1)
                wheel.attach.setValueAsCompleteGesture(static_cast<float>(wheel.selectedChoice));
            for (auto c = 0; c < wheel.choices.size(); ++c)
                wheel.choices[c].get()->setVisible(false);
            wheel.repaint();
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Button)
    };

    struct Choice :
        public nelG::Comp {
        Choice(QuickAccessWheel& w, const juce::String& vs, int i) :
            nelG::Comp(w.processor, w.utils),
            wheel(w),
            valueString(vs),
            idx(i)
        {
            cFont = utils.font;
            cFont.setHeight(11);
        }
        QuickAccessWheel& wheel;
        juce::String valueString;
        juce::Font cFont;
        int idx;
    private:
        void paint(juce::Graphics& g) override {
            const auto bounds = getLocalBounds().toFloat().reduced(util::Thicc);
            const auto curValue = static_cast<int>(wheel.param->convertFrom0to1(wheel.param->getValue()));
            if (idx != curValue) g.setColour(juce::Colour(0x99ffffff));
            else g.setColour(juce::Colour(util::ColYellow));
            g.fillRoundedRectangle(bounds, 3.f);
            g.setColour(juce::Colour(util::ColBlack));
            if (idx == wheel.selectedChoice) g.drawRoundedRectangle(bounds, util::Rounded, util::Thicc);
            g.setFont(cFont);
            g.drawFittedText(valueString, getLocalBounds(), juce::Justification::centred, 1, 0);
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Choice)
    };
public:
    QuickAccessWheel(Nel19AudioProcessor& p, param::ID id, nelG::Utils& u, juce::String tooltp = "") :
        nelG::Comp(p, u),
        param(p.apvts.getParameter(param::getID(id))),
        attach(*param, [this](float v) { repaint(); }, nullptr),
        button(*this, id, tooltp),
        choices(),
        startAngle(util::Tau * .75f), angleRange(util::Tau * .5f), overshootAngle(0), minDimen(0),
        state(State::Def),
        selectedChoice(-1)
    {
        const auto& valueStrings = param->getAllValueStrings();
        choices.reserve(param->getNumSteps());
        for (auto c = 0; c < param->getNumSteps(); ++c) {
            choices.emplace_back(std::make_unique<Choice>(*this, valueStrings[c], c));
            addAndMakeVisible(choices[c].get());
            choices[c].get()->setVisible(false);
        }
        addAndMakeVisible(button);
        button.setMouseCursor(u.cursors[nelG::Utils::Cursor::Hover]);
        setInterceptsMouseClicks(false, true);
        attach.sendInitialUpdate();
    }
    void init(juce::Rectangle<int> buttonBounds, float angleStart = 0.f, float rangeAngle = 3.14f, float overshoot = 3.14 / 8.f) {
        startAngle = angleStart;
        angleRange = rangeAngle;
        overshootAngle = overshoot;
        buttonBounds.setX(buttonBounds.getX() - getParentComponent()->getX());
        buttonBounds.setY(buttonBounds.getY() - getParentComponent()->getY());
        button.setBounds(buttonBounds);
        setBounds(getParentComponent()->getBounds());
    }
private:
    juce::RangedAudioParameter* param;
    juce::ParameterAttachment attach;
    Button button;
    std::vector<std::unique_ptr<Choice>> choices;
    float startAngle, angleRange, overshootAngle, minDimen;
    State state;
    int selectedChoice;

    void resized() override {
        const auto choicesMax = choices.size() - 1;
        const auto choicesMaxInv = 1.f / choicesMax;

        minDimen = static_cast<float>(std::min(button.getWidth(), button.getHeight()));
        const auto choiceDistance = minDimen * 1.5f;
        const auto choiceWidth = minDimen * .75f;
        const auto choiceRadius = choiceWidth * .5f;
        juce::Point<float> centre(
            static_cast<float>(button.getX()) + static_cast<float>(button.getWidth()) * .5f,
            static_cast<float>(button.getY()) + static_cast<float>(button.getHeight()) * .5f
        );
        const auto translation = juce::AffineTransform::translation(centre);
        for (auto c = 0; c < choices.size(); ++c) {
            const auto x = c * choicesMaxInv;
            const auto angle = startAngle + x * angleRange;
            juce::Line<float> tick(0.f, 0.f, 0.f, -choiceDistance);
            const auto rotation = juce::AffineTransform::rotation(angle);
            tick.applyTransform(rotation.followedBy(translation));
            const juce::Rectangle<float> cArea(
                tick.getEndX() - choiceRadius,
                tick.getEndY() - choiceRadius,
                choiceWidth,
                choiceWidth
            );
            //DBG(cArea.toString());
            choices[c].get()->setBounds(cArea.toNearestInt());
        }
    }
    void paint(juce::Graphics& g) override {
#if DebugLayout
        g.setColour(juce::Colours::red);
        g.drawRect(getLocalBounds());
#endif
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QuickAccessWheel)
};

struct Knob :
    public nelG::CompParam
{
    struct Dial :
        public nelG::CompParam
    {
        Dial(Nel19AudioProcessor& p, param::ID id, nelG::Utils& u, const juce::String& s) :
            nelG::CompParam(p, u, s),
            param(p.apvts.getParameter(param::getID(id))),
            attach(*param, [this](float) { repaint(); }, nullptr),
            lastDistance(0)
        { setMouseCursor(utils.cursors[nelG::Utils::Cursor::Hover]); }
        void updateValue(float v) {
            attach.setValueAsPartOfGesture(v);
            repaint();
        }
        juce::RangedAudioParameter* param;
        juce::ParameterAttachment attach;
    private:
        float lastDistance;
        void mouseDown(const juce::MouseEvent&) override {
            if (!enabled) return;
            attach.beginGesture();
            lastDistance = 0;
        }
        void mouseUp(const juce::MouseEvent&) override {
            if (!enabled) return;
            attach.endGesture();
        }
        void mouseDrag(const juce::MouseEvent& evt) override {
            if (!enabled) return;
            const auto height = static_cast<float>(getHeight());
            const auto dist = static_cast<float>(-evt.getDistanceFromDragStartY()) / height;
            const auto dRate = dist - lastDistance;
            const auto speed = evt.mods.isShiftDown() ? .05f : 1.f;
            const auto newValue = juce::jlimit(0.f, 1.f, param->getValue() + dRate * speed);
            updateValue(param->convertFrom0to1(newValue));
            lastDistance = dist;
        }
        void mouseWheelMove(const juce::MouseEvent& evt, const juce::MouseWheelDetails& wheel) override {
            if (!enabled) return;
            else if (evt.mods.isLeftButtonDown() || evt.mods.isRightButtonDown())
                return;
            const auto speed = evt.mods.isShiftDown() ? .005f : .05f;
            const auto gestr = wheel.deltaY > 0 ? speed : -speed;
            attach.setValueAsCompleteGesture(
                param->convertFrom0to1(param->getValue() + gestr)
            );
            repaint();
        }
        void paint(juce::Graphics& g) override {
#if DebugLayout
            g.setColour(juce::Colours::red);
            g.drawRect(getLocalBounds());
#endif
            const auto bounds = getLocalBounds().toFloat().reduced(util::Thicc);
            const auto width = bounds.getWidth();
            const auto height = bounds.getHeight();
            const auto diameter = std::min(width, height);
            juce::Point<float> centre(width * .5f, height * .5f);
            const auto radius = diameter * .5f;
            const auto tickLength = radius - util::Thicc2;
            juce::Line<float> tick(0, 0, 0, tickLength);
            const auto normalValue = param->getValue();
            const auto centreWithOffset = centre + juce::Point<float>(bounds.getX(), bounds.getY());
            const auto translation = juce::AffineTransform::translation(centreWithOffset);
            const auto startAngle = util::Pi * .25f;
            const auto angleLength = util::Tau - startAngle;
            const auto knobRadius = radius;
            const auto knobX = bounds.getX() + centre.x - knobRadius;
            const auto knobY = bounds.getY() + centre.y - knobRadius;
            const auto knobWidth = 2.f * knobRadius;
            juce::Rectangle<float> knobArea(knobX, knobY, knobWidth, knobWidth);
            if(enabled) g.setColour(juce::Colour(util::ColGreen));
            else g.setColour(juce::Colour(util::ColGrey));
            g.drawEllipse(knobArea, util::Thicc);
            const auto thicc = normalValue + util::Thicc;
            const auto tickCountInv = 1.f / util::DialTickCount;
            for (auto x = 0.f; x <= 1.f; x += tickCountInv) {
                auto tickT = tick;
                const auto angle = startAngle + (x * normalValue) * (angleLength - startAngle);
                const auto rotation = juce::AffineTransform::rotation(angle);
                tickT.applyTransform(rotation.followedBy(translation));
                g.drawLine(tickT, thicc);
            }
        }
    };

    Knob(Nel19AudioProcessor& p, param::ID id, nelG::Utils& u, juce::String tooltp = "") :
        nelG::CompParam(p, u, tooltp),
        param(p.apvts.getParameter(param::getID(id))),
        attach(*param, [this](float v) { updateValue(v); }, nullptr),
        dial(p, id, u, tooltp),
        name("", param::getName(id)),
        textbox("", "-")
    {
        name.setMouseCursor(u.cursors[nelG::Utils::Cursor::Norm]);
        textbox.setMouseCursor(u.cursors[nelG::Utils::Cursor::Norm]);
        auto font = utils.font;
        font.setExtraKerningFactor(.1f);
        name.setFont(font);
        textbox.setFont(utils.font);
        name.setColour(juce::Label::ColourIds::textColourId, juce::Colour(util::ColGreen));
        name.setJustificationType(juce::Justification::centred);
        textbox.setColour(juce::Label::ColourIds::textColourId, juce::Colour(util::ColGreen));
        textbox.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(name);
        addAndMakeVisible(dial);
        addAndMakeVisible(textbox);
        attach.sendInitialUpdate();
    }
    void setEnabled(bool e) override {
        dial.setEnabled(e);
        enabled = e;
        auto colour = enabled ? juce::Colour(util::ColGreen) : juce::Colour(util::ColGrey);
        name.setColour(juce::Label::ColourIds::textColourId, colour);
        textbox.setColour(juce::Label::ColourIds::textColourId, colour);
    }
    void updateValue(float v) {
        textbox.setText(param->getCurrentValueAsText(), juce::NotificationType::sendNotificationAsync);
    }

    void setQBounds(juce::Rectangle<int> nameBounds, juce::Rectangle<int> dialBounds, juce::Rectangle<int> tbBounds) {
        const auto x = std::min(nameBounds.getX(), std::min(dialBounds.getX(), tbBounds.getX()));
        const auto y = nameBounds.getY();
        const auto width = std::max(nameBounds.getWidth(), std::max(dialBounds.getWidth(), tbBounds.getWidth()));
        const auto height = nameBounds.getHeight() + dialBounds.getHeight() + tbBounds.getHeight();
        juce::Rectangle<int> bounds(x, y, width, height);
        setBounds(bounds);
        nameBounds.setX(nameBounds.getX() - x);
        nameBounds.setY(nameBounds.getY() - y);
        name.setBounds(nameBounds);
        dialBounds.setX(dialBounds.getX() - x);
        dialBounds.setY(dialBounds.getY() - y);
        dial.setBounds(dialBounds);
        tbBounds.setX(tbBounds.getX() - x);
        tbBounds.setY(tbBounds.getY() - y);
        textbox.setBounds(tbBounds);
    }
private:
    juce::RangedAudioParameter* param;
    juce::ParameterAttachment attach;
    Dial dial;
    juce::Label name, textbox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Knob)
};

struct Slider :
    public nelG::Comp {

    struct Handle :
        public nelG::CompParam
    {
        Handle(Nel19AudioProcessor& p, param::ID id, nelG::Utils& u, const juce::String& s, bool vert) :
            nelG::CompParam(p, u, s),
            param(p.apvts.getParameter(param::getID(id))),
            attach(*param, [this](float v) { repaint(); }, nullptr),
            isVertical(vert)
        { setMouseCursor(u.cursors[nelG::Utils::Cursor::Hover]); }
    private:
        juce::RangedAudioParameter* param;
        juce::ParameterAttachment attach;
        float dragStartValue;
        bool isVertical;

        void paint(juce::Graphics& g) override {
#if DebugLayout
            g.setColour(juce::Colours::red);
            g.drawRect(getLocalBounds());
#endif
            const auto bounds = getLocalBounds().toFloat().reduced(util::Thicc);
            const auto width = bounds.getWidth();
            const auto height = bounds.getHeight();
            const auto value = param->getValue();
            const auto thiccHalf = util::Thicc * .5f;
            if (isVertical) {
                const auto scaled = value * height;
                const auto x = bounds.getX();
                const auto y = bounds.getY();
                if (enabled) g.setColour(juce::Colour(util::ColGreen));
                else g.setColour(juce::Colour(util::ColGrey));
                const auto tickCountInv = 1.f / util::DialTickCount;
                for (auto i = 0.f; i < 1.f; i += tickCountInv) {
                    const auto i0 = height - i * scaled;
                    g.drawLine(x, i0, width, i0, util::Thicc);
                }
                const auto lineY = y + y;
                g.drawLine({ x,lineY,x,height }, util::Thicc);
                g.drawLine({ width,lineY,width,height }, util::Thicc);
            }
            else {
                const auto scaled = value * width;
                const auto x = bounds.getX();
                const auto y = bounds.getY();
                if (enabled) g.setColour(juce::Colour(util::ColGreen));
                else g.setColour(juce::Colour(util::ColGrey));
                const auto tickCountInv = 1.f / util::DialTickCount;
                for (auto i = 0.f; i <= 1.f; i += tickCountInv) {
                    const auto i0 = x + i * scaled;
                    g.drawLine(i0, y, i0, height, util::Thicc);
                }
                const auto lineWidth = width + x;
                g.drawLine({ x, y, x, y }, util::Thicc);
                g.drawLine({ x, height, lineWidth, height }, util::Thicc);
            }
        }

        void mouseDown(const juce::MouseEvent& evt) override {
            attach.beginGesture();
            dragStartValue = param->getValue();
        }
        void mouseDrag(const juce::MouseEvent& evt) override {
            const auto sensitivity = evt.mods.isShiftDown() ? .1f : .8f;
            float bounds, distType;
            if (isVertical) {
                bounds = static_cast<float>(getHeight());
                distType = static_cast<float>(-evt.getDistanceFromDragStartY());
            }
            else {
                bounds = static_cast<float>(getWidth());
                distType = static_cast<float>(evt.getDistanceFromDragStartX());
            }
            const auto distance = distType / bounds;
            const auto value = juce::jlimit(0.f, 1.f, dragStartValue + distance * sensitivity);
            attach.setValueAsPartOfGesture(param->convertFrom0to1(value));
        }
        void mouseUp(const juce::MouseEvent& evt) override {
            attach.endGesture();
        }
        void mouseWheelMove(const juce::MouseEvent& evt, const juce::MouseWheelDetails& wheel) override {
            if (evt.mods.isLeftButtonDown() || evt.mods.isRightButtonDown())
                return;
            const auto change = wheel.deltaY > 0 ? 1 : -1;
            const auto sensitivity = evt.mods.isShiftDown() ? .01f : .05f;
            const auto curValue = param->getValue();
            const auto nValue = curValue + change * sensitivity;
            const auto limited = juce::jlimit(0.f, 1.f, nValue);
            attach.setValueAsCompleteGesture(param->convertFrom0to1(limited));
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Handle)
    };

    Slider(Nel19AudioProcessor& p, param::ID id, nelG::Utils& u, const juce::String& s, bool vert = true) :
        nelG::Comp(p, u, s),
        imgBottom(), imgTop(),
        param(p.apvts.getParameter(param::getID(id))),
        attach(*param, [this](float v) { repaint(); }, nullptr),
        handle(p, id, u, s, vert),
        isVertical(vert)
    {
        addAndMakeVisible(handle);
        attach.sendInitialUpdate();
    }
    virtual void setImages() = 0;
    void setEnabled(bool e) { handle.setEnabled(e); }
protected:
    nelG::Image imgBottom, imgTop;
private:
    juce::RangedAudioParameter* param;
    juce::ParameterAttachment attach;
    Handle handle;
    bool isVertical;
    
    void resized() override {
        auto x = 0.f;
        auto y = 0.f;
        const auto width = static_cast<float>(getWidth());
        const auto height = static_cast<float>(getHeight());
        const auto minDimen = std::min(width, height);
        const auto imgPadding = minDimen * .05f;
        if (isVertical) {
            nelG::Ratio ratio({ 2,9,2 }, height);
            const auto imgTopHeight = ratio[0];
            const auto handleHeight = ratio[1];
            const auto imgBottomHeight = ratio[2];
            imgTop.setBounds(juce::Rectangle<float>(x, y, width, imgTopHeight).reduced(imgPadding).toNearestInt());
            y += imgTopHeight;
            handle.setBounds(juce::Rectangle<float>(x, y, width, handleHeight).toNearestInt());
            y += handleHeight;
            imgBottom.setBounds(juce::Rectangle<float>(x, y, width, imgBottomHeight).reduced(imgPadding).toNearestInt());
        }
        else {
            nelG::Ratio ratio({ 2,9,2 }, width);
            const auto imgTopWidth = ratio[0];
            const auto handleWidth = ratio[1];
            const auto imgBottomWidth = ratio[2];
            imgTop.setBounds(juce::Rectangle<float>(x, y, imgTopWidth, height).reduced(imgPadding).toNearestInt());
            x += imgTopWidth;
            handle.setBounds(juce::Rectangle<float>(x, y, handleWidth, height).toNearestInt());
            x += handleWidth;
            imgBottom.setBounds(juce::Rectangle<float>(x, y, imgBottomWidth, height).reduced(imgPadding).toNearestInt());
        }
        setImages();
    }
    void paint(juce::Graphics& g) override {
#if DebugLayout
        g.setColour(juce::Colours::red);
        g.drawRect(getLocalBounds());
#endif
        imgTop.paint(g);
        imgBottom.paint(g);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Slider)
};

struct SliderSplineMix :
    public Slider
{
    SliderSplineMix(Nel19AudioProcessor& p, param::ID id, nelG::Utils& u, const juce::String& s) :
        Slider(p, id, u, s, false)
    {}
    void setImages() override {
        const auto bounds = imgTop.getBounds().toFloat().reduced(util::Thicc);
        const auto heightHalf = bounds.getHeight() * .5f;
        const auto widthHalf = bounds.getWidth() * .5f;
        const auto black = juce::Colour(util::ColBlack);
        const auto green = juce::Colour(util::ColGreen);
        juce::Graphics gT{ imgTop.img };
        juce::Graphics gB{ imgBottom.img };
        gB.setColour(black);
        gB.fillRoundedRectangle(bounds, util::Rounded);

        gT.setColour(green);
        gB.setColour(green);
        gT.drawFittedText("RAND", imgTop.getBounds(), juce::Justification::centred, 1, 0);
        gB.drawFittedText("SPLINE", imgBottom.getBounds(), juce::Justification::centred, 1, 0);
    }
};

struct ButtonSwitch :
    public nelG::CompParam {

    ButtonSwitch(Nel19AudioProcessor& p, param::ID id, nelG::Utils& u, juce::String tooltp = "") :
        nelG::CompParam(p, u, tooltp),
        param(p.apvts.getParameter(param::getID(id))),
        attach(*param, [this](float) { repaint(); })
    { setMouseCursor(u.cursors[nelG::Utils::Cursor::Hover]); }
private:
    juce::RangedAudioParameter* param;
    juce::ParameterAttachment attach;

    void paint(juce::Graphics& g) override {
#if DebugLayout
        g.setColour(juce::Colours::red);
        g.drawRect(getLocalBounds());
#endif
        const auto bounds = getLocalBounds().toFloat().reduced(util::Thicc);
        if(enabled) g.setColour(juce::Colour(util::ColGreen));
        else g.setColour(juce::Colour(util::ColGrey));
        g.drawRoundedRectangle(bounds, util::Rounded, util::Thicc);
        g.setFont(utils.font);
        g.drawFittedText(param->getCurrentValueAsText(), getLocalBounds(), juce::Justification::centred, 1);
    }
    void mouseUp(const juce::MouseEvent& evt) override {
        if (!enabled) return;
        else if (evt.mouseWasDraggedSinceMouseDown()) return;
        attach.setValueAsCompleteGesture(1.f - param->getValue());
        repaint();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ButtonSwitch)
};