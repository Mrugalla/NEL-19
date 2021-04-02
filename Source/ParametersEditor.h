#pragma once
#include <JuceHeader.h>

class QuickAccessWheel :
    public NELComp
{
    enum class State { Def, Drag };
    
    struct Button :
        public NELComp
    {
        Button(QuickAccessWheel& w, nelDSP::param::ID id, const juce::String& s) :
            NELComp(w.processor, w.utils, s),
            wheel(w),
            name(nelDSP::param::getName(id))
        {
            qaFont = utils.font;
            qaFont.setHeight(11);
        }
    private:
        QuickAccessWheel& wheel;
        juce::String name;
        juce::Font qaFont;
        void paint(juce::Graphics& g) {
#if DebugLayout
            g.setColour(juce::Colours::red);
            g.drawRect(getLocalBounds());
#endif
            const auto bounds = getLocalBounds().toFloat().reduced(nelG::Thicc);
            g.setColour(juce::Colour(nelG::ColBlack));
            g.fillRoundedRectangle(bounds, nelG::Rounded);
            g.setFont(qaFont);
            if (wheel.state == State::Def) {
                g.setColour(juce::Colour(nelG::ColGreen));
                g.drawRoundedRectangle(bounds, nelG::Rounded, nelG::Thicc);
                g.drawFittedText(name + "\n" + wheel.param->getCurrentValueAsText(), bounds.toNearestInt(), juce::Justification::centred, 1, 1);
            }
            else {
                if (wheel.selectedChoice == -1) {
                    g.setColour(juce::Colour(nelG::ColRed));
                    g.drawFittedText("X", getLocalBounds(), juce::Justification::centred, 1, 1);
                    g.drawRoundedRectangle(bounds, nelG::Rounded, nelG::Thicc);
                    return;
                }
                g.setColour(juce::Colour(nelG::ColGreen));
                g.drawRoundedRectangle(bounds, nelG::Rounded, nelG::Thicc);
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
                while (angle < 0) angle += nelG::Tau;
                if (angle < wheel.angleRange) {
                    const auto size = static_cast<float>(wheel.choices.size());
                    const auto value = size * angle / wheel.angleRange;
                    const auto rValue = static_cast<int>(value);
                    wheel.selectedChoice = juce::jlimit(0, static_cast<int>(wheel.choices.size()) - 1, rValue);
                }
                else if (angle < wheel.angleRange + wheel.overshootAngle)
                    wheel.selectedChoice = static_cast<int>(wheel.choices.size()) - 1;
                else if (angle > nelG::Tau - wheel.overshootAngle)
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
        public NELComp {
        Choice(QuickAccessWheel& w, const juce::String& vs, int i) :
            NELComp(w.processor, w.utils),
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
#if DebugLayout
            g.setColour(juce::Colours::red);
            g.drawRect(getLocalBounds());
#endif
            const auto bounds = getLocalBounds().toFloat().reduced(nelG::Thicc);
            const auto curValue = static_cast<int>(wheel.param->convertFrom0to1(wheel.param->getValue()));
            if (idx != curValue) g.setColour(juce::Colour(0x99ffffff));
            else g.setColour(juce::Colour(nelG::ColYellow));
            g.fillRoundedRectangle(bounds, 3.f);
            g.setColour(juce::Colour(nelG::ColBlack));
            if (idx == wheel.selectedChoice) g.drawRoundedRectangle(bounds, nelG::Rounded, nelG::Thicc);
            g.setFont(cFont);
            g.drawFittedText(valueString, getLocalBounds(), juce::Justification::centred, 1, 0);
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Choice)
    };
public:
    QuickAccessWheel(Nel19AudioProcessor& p, nelDSP::param::ID id, NELGUtil& u, juce::String tooltp = "") :
        NELComp(p, u),
        param(p.apvts.getParameter(nelDSP::param::getID(id))),
        attach(*param, [this](float v) { repaint(); }, nullptr),
        button(*this, id, tooltp),
        choices(),
        startAngle(nelG::Tau * .75f), angleRange(nelG::Tau * .5f), overshootAngle(0), minDimen(0),
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
        button.setMouseCursor(u.cursors[NELGUtil::Cursor::Hover]);
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

        minDimen = std::min(button.getWidth(), button.getHeight());
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
    public NELComp
{
    struct Dial :
        public NELComp
    {
        Dial(Nel19AudioProcessor& p, nelDSP::param::ID id, NELGUtil& u, const juce::String& s) :
            NELComp(p, u, s),
            param(p.apvts.getParameter(nelDSP::param::getID(id))),
            attach(*param, [this](float) { repaint(); }, nullptr),
            lastDistance(0)
        {
            setMouseCursor(utils.cursors[NELGUtil::Cursor::Hover]);
        }
        void updateValue(float v) {
            attach.setValueAsPartOfGesture(v);
            repaint();
        }
        juce::RangedAudioParameter* param;
        juce::ParameterAttachment attach;
    private:
        float lastDistance;
        void mouseEnter(const juce::MouseEvent&) override {
            setMouseCursor(utils.cursors[NELGUtil::Cursor::Hover]);
        }
        void mouseExit(const juce::MouseEvent&) override {
            setMouseCursor(utils.cursors[NELGUtil::Cursor::Norm]);
        }
        void mouseDown(const juce::MouseEvent&) override {
            attach.beginGesture();
            lastDistance = 0;
        }
        void mouseUp(const juce::MouseEvent&) override {
            attach.endGesture();
        }
        void mouseDrag(const juce::MouseEvent& evt) override {
            const auto height = static_cast<float>(getHeight());
            const auto dist = static_cast<float>(-evt.getDistanceFromDragStartY()) / height;
            const auto dRate = dist - lastDistance;
            const auto speed = evt.mods.isShiftDown() ? .05f : 1.f;
            const auto newValue = juce::jlimit(0.f, 1.f, param->getValue() + dRate * speed);
            updateValue(param->convertFrom0to1(newValue));
            lastDistance = dist;
        }
        void mouseWheelMove(const juce::MouseEvent& evt, const juce::MouseWheelDetails& wheel) override {
            if (evt.mods.isLeftButtonDown() || evt.mods.isRightButtonDown())
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
            const auto bounds = getLocalBounds().toFloat().reduced(nelG::Thicc);
            const auto width = bounds.getWidth();
            const auto height = bounds.getHeight();
            const auto diameter = std::min(width, height);
            juce::Point<float> centre(width * .5f, height * .5f);
            const auto radius = diameter * .5f;
            const auto tickLength = radius - nelG::Thicc * 2.f;
            juce::Line<float> tick(0, 0, 0, tickLength);
            const auto normalValue = param->getValue();
            const auto centreWithOffset = centre + juce::Point<float>(bounds.getX(), bounds.getY());
            const auto translation = juce::AffineTransform::translation(centreWithOffset);
            const auto startAngle = nelG::Pi * .25f;
            const auto angleLength = nelG::Tau - startAngle;
            const auto knobRadius = radius;
            const auto knobX = bounds.getX() + centre.x - knobRadius;
            const auto knobY = bounds.getY() + centre.y - knobRadius;
            const auto knobWidth = 2.f * knobRadius;
            juce::Rectangle<float> knobArea(knobX, knobY, knobWidth, knobWidth);
            g.setColour(juce::Colour(nelG::ColBlack));
            g.fillEllipse(knobArea);
            g.setColour(juce::Colour(nelG::ColGreen));
            g.drawEllipse(knobArea, nelG::Thicc);
            const auto thicc = normalValue + nelG::Thicc;
            const auto tickCount = 16.f;
            for (auto x = 0.f; x <= 1.f; x += 1.f / tickCount) {
                auto tickT = tick;
                const auto angle = startAngle + (x * normalValue) * (angleLength - startAngle);
                const auto rotation = juce::AffineTransform::rotation(angle);
                tickT.applyTransform(rotation.followedBy(translation));
                g.drawLine(tickT, thicc);
            }
        }
    };

    Knob(Nel19AudioProcessor& p, nelDSP::param::ID id, NELGUtil& u, juce::String tooltp = "") :
        NELComp(p, u, tooltp),
        param(p.apvts.getParameter(nelDSP::param::getID(id))),
        attach(*param, [this](float v) { updateValue(v); }, nullptr),
        dial(p, id, u, tooltp),
        name("", nelDSP::param::getName(id)),
        textbox("", "-")
    {
        name.setMouseCursor(u.cursors[NELGUtil::Cursor::Norm]);
        textbox.setMouseCursor(u.cursors[NELGUtil::Cursor::Norm]);
        auto font = utils.font;
        font.setExtraKerningFactor(.1f);
        name.setFont(font);
        textbox.setFont(utils.font);
        name.setColour(juce::Label::ColourIds::textColourId, juce::Colour(nelG::ColGreen));
        name.setJustificationType(juce::Justification::centred);
        textbox.setColour(juce::Label::ColourIds::textColourId, juce::Colour(nelG::ColGreen));
        textbox.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(name);
        addAndMakeVisible(dial);
        addAndMakeVisible(textbox);
        attach.sendInitialUpdate();
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
    juce::RangedAudioParameter* param;
    juce::ParameterAttachment attach;
    Dial dial;
    juce::Label name, textbox;
private:
    void paint(juce::Graphics& g) override {
#if DebugLayout
        g.setColour(juce::Colours::red);
        g.drawRect(getLocalBounds());
#endif
    }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Knob)
};

struct Slider :
    public NELComp {

    struct Handle :
        public NELComp
    {
        Handle(Nel19AudioProcessor& p, nelDSP::param::ID id, NELGUtil& u, const juce::String& s) :
            NELComp(p, u, s),
            param(p.apvts.getParameter(nelDSP::param::getID(id))),
            attach(*param, [this](float v) { repaint(); }, nullptr)
        { setMouseCursor(u.cursors[NELGUtil::Cursor::Hover]); }
    private:
        juce::RangedAudioParameter* param;
        juce::ParameterAttachment attach;
        float dragStartValue;

        void paint(juce::Graphics& g) override {
#if DebugLayout
            g.setColour(juce::Colours::red);
            g.drawRect(getLocalBounds());
#endif
            const auto bounds = getLocalBounds().toFloat().reduced(nelG::Thicc);
            const auto width = bounds.getWidth();
            const auto height = bounds.getHeight();
            const auto value = param->getValue();
            const auto scaled = value * height;
            const auto x = bounds.getX();
            const auto y = height - scaled + nelG::Thicc;
            g.setColour(juce::Colour(nelG::ColBlack));
            g.fillRoundedRectangle(bounds, nelG::Rounded);
            g.setColour(juce::Colour(nelG::ColGreen));
            g.fillRoundedRectangle({x, y, width, scaled}, nelG::Rounded);
            const auto thiccHalf = nelG::Thicc * .5f;
            const auto lineX = x + thiccHalf;
            const auto lineY = bounds.getY() + nelG::Thicc;
            const auto lineWidth = width + thiccHalf;
            const auto lineHeight = height + nelG::Thicc;
            g.drawLine({ lineX,lineY,lineX,lineHeight }, nelG::Thicc);
            g.drawLine({ lineWidth,lineY,lineWidth,lineHeight }, nelG::Thicc);
            //g.setColour(juce::Colour(nelG::ColBlack));
            //g.drawRoundedRectangle(bounds, nelG::Rounded, nelG::Thicc);
        }

        void mouseDown(const juce::MouseEvent& evt) override {
            attach.beginGesture();
            dragStartValue = param->getValue();
        }
        void mouseDrag(const juce::MouseEvent& evt) override {
            const auto sensitivity = evt.mods.isShiftDown() ? .1f : .8f;
            const auto height = static_cast<float>(getHeight());
            const auto distance = -evt.getDistanceFromDragStartY() / height;
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

    Slider(Nel19AudioProcessor& p, nelDSP::param::ID id, NELGUtil& u, const juce::String& s) :
        NELComp(p, u, s),
        imgBottom(), imgTop(),
        param(p.apvts.getParameter(nelDSP::param::getID(id))),
        attach(*param, [this](float v) { repaint(); }, nullptr),
        handle(p, id, u, s)
    {
        addAndMakeVisible(handle);
        attach.sendInitialUpdate();
    }
    virtual void setImages() = 0;
protected:
    nelG::Image imgBottom, imgTop;
private:
    juce::RangedAudioParameter* param;
    juce::ParameterAttachment attach;
    Handle handle;
    
    void resized() override {
        auto x = 0.f;
        auto y = 0.f;
        const auto width = static_cast<float>(getWidth());
        const auto height = static_cast<float>(getHeight());
        const auto minDimen = std::min(width, height);
        const auto imgPadding = minDimen * .05f;
        nelG::Ratio ratio({ 2,9,2 });
        const auto imgTopHeight = ratio[0] * height;
        const auto handleHeight = ratio[1] * height;
        const auto imgBottomHeight = ratio[2] * height;
        imgTop.setBounds(juce::Rectangle<float>(x, y, width, imgTopHeight).reduced(imgPadding).toNearestInt());
        y += imgTopHeight;
        handle.setBounds(juce::Rectangle<float>(x, y, width, handleHeight).toNearestInt());
        y += handleHeight;
        imgBottom.setBounds(juce::Rectangle<float>(x, y, width, imgBottomHeight).reduced(imgPadding).toNearestInt());
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

struct SliderShape :
    public Slider {

    SliderShape(Nel19AudioProcessor& p, nelDSP::param::ID id, NELGUtil& u, const juce::String& s) :
        Slider(p, id, u, s)
    {}
    void setImages() override {
        const auto bounds = imgTop.getBounds().toFloat().reduced(nelG::Thicc);
        const auto heightHalf = bounds.getHeight() * .5f;
        const auto widthHalf = bounds.getWidth() * .5f;
        const auto black = juce::Colour(nelG::ColBlack);
        const auto green = juce::Colour(nelG::ColGreen);
        juce::Graphics gT{ imgTop.img };
        juce::Graphics gB{ imgBottom.img };
        gT.setColour(black);
        gT.fillRoundedRectangle(bounds, nelG::Rounded);
        gB.setColour(black);
        gB.fillRoundedRectangle(bounds, nelG::Rounded);
        
        gT.setColour(green);
        gB.setColour(green);
        juce::Path path;
        const auto xStart = bounds.getX() + nelG::Thicc;
        const auto bottom = bounds.getBottom() - nelG::Thicc;
        const auto midX = bounds.getX() + widthHalf;
        const auto top = bounds.getY() + nelG::Thicc;
        const auto xEnd = bounds.getRight() - nelG::Thicc;
        const juce::PathStrokeType pathStrokeType(nelG::Thicc, juce::PathStrokeType::JointStyle::curved, juce::PathStrokeType::EndCapStyle::rounded);
        path.startNewSubPath(xStart, bottom);
        path.lineTo(midX, bottom);
        path.lineTo(midX, top);
        path.lineTo(xEnd, top);
        gT.strokePath(path, pathStrokeType);
        path.clear();
        const auto yStart = bounds.getY() + nelG::Thicc;
        const auto doubleThicc = nelG::Thicc * 2;
        const auto nWidth = bounds.getWidth() - doubleThicc;
        const auto nHeight = bounds.getHeight() - doubleThicc;
        path.startNewSubPath(0, .5f);
        for (auto i = 0; i < nWidth; ++i) {
            const auto x = static_cast<float>(i) / nWidth;
            const auto sinusoid = std::sin(x * nelG::Tau);
            const auto y = .5f * (sinusoid + 1.f);
            path.lineTo(i, y);
        }
        const auto scale = juce::AffineTransform::scale(1.f, nHeight);
        const auto translate = juce::AffineTransform::translation(xStart, yStart);
        gB.strokePath(path, pathStrokeType, scale.followedBy(translate));
    }
};

struct ButtonSwitch :
    public NELComp {

    ButtonSwitch(Nel19AudioProcessor& p, nelDSP::param::ID id, NELGUtil& u, juce::String tooltp = "") :
        NELComp(p, u, tooltp),
        param(p.apvts.getParameter(nelDSP::param::getID(id))),
        attach(*param, [this](float) { repaint(); })
    {
        setMouseCursor(u.cursors[NELGUtil::Cursor::Hover]);
    }
private:
    juce::RangedAudioParameter* param;
    juce::ParameterAttachment attach;

    void paint(juce::Graphics& g) override {
#if DebugLayout
        g.setColour(juce::Colours::red);
        g.drawRect(getLocalBounds());
#endif
        const auto bounds = getLocalBounds().toFloat().reduced(nelG::Thicc);
        g.setColour(juce::Colour(nelG::ColBlack));
        g.fillRoundedRectangle(bounds, nelG::Rounded);
        g.setColour(juce::Colour(nelG::ColGreen));
        g.drawRoundedRectangle(bounds, nelG::Rounded, nelG::Thicc);
        g.setFont(utils.font);
        g.drawFittedText(param->getCurrentValueAsText(), getLocalBounds(), juce::Justification::centred, 1);
    }
    void mouseUp(const juce::MouseEvent& evt) override {
        if (evt.mouseWasDraggedSinceMouseDown()) return;
        attach.setValueAsCompleteGesture(1.f - param->getValue());
        repaint();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ButtonSwitch)
};

struct ParametersEditor :
    public NELComp
{
    ParametersEditor(Nel19AudioProcessor& p, NELGUtil& u) :
        NELComp(p, u),
        depthMaxK(p, nelDSP::param::ID::DepthMax, utils, "Higher DepthMax values sacrifice latency for a stronger vibrato."),
        lrmsK(p, nelDSP::param::ID::LRMS, utils, "L/R for traditional stereo width, M/S for mono compatibility."),
        depthK(p, nelDSP::param::ID::Depth, utils, "Defines the depth of the vibrato."),
        freqK(p, nelDSP::param::ID::Freq, utils, "The frequency in which random values are picked for the LFO."),
        widthK(p, nelDSP::param::ID::Width, utils, "The offset of each channel's Random LFO."),
        mixK(p, nelDSP::param::ID::Mix, utils, "Turn Mix down for Chorus/Flanger-Sounds."),
        shapeK(p, nelDSP::param::ID::Shape, utils, "Defines the shape of the randomizer."),
        bounds()
    {
        setInterceptsMouseClicks(false, true);
        addAndMakeVisible(depthK);
        addAndMakeVisible(freqK);
        addAndMakeVisible(shapeK);
        addAndMakeVisible(widthK);
        addAndMakeVisible(mixK);
        addAndMakeVisible(depthMaxK);
        addAndMakeVisible(lrmsK);
    }
    void setQBounds(const juce::Rectangle<int> b) {
        bounds = b;
        setBounds(getParentComponent()->getBounds());
    }
private:
    QuickAccessWheel depthMaxK;
    ButtonSwitch lrmsK;
    Knob depthK, freqK, widthK, mixK;
    SliderShape shapeK;
    juce::Rectangle<int> bounds;
    
    void resized() override {
        const auto width = static_cast<float>(bounds.getWidth());
        const auto height = static_cast<float>(bounds.getHeight());
        const auto x = static_cast<float>(bounds.getX());
        nelG::Ratio ratioX({ 13, 13, 4, 8, 8 }, width, x);
        nelG::Ratio ratioY({ 5, 9, 5, 5 }, height);
        nelG::RatioBounds ratioBounds(ratioX, ratioY);
        depthK.setQBounds(ratioBounds(0, 0), ratioBounds(0, 1), ratioBounds(0, 2));
        depthMaxK.init(
            ratioBounds(0, 3),
            nelG::Tau * .75f,
            nelG::Tau * .5f,
            nelG::PiQuart * .25f
        );
        freqK.setQBounds(ratioBounds(1, 0), ratioBounds(1, 1), ratioBounds(1, 2));
        shapeK.setBounds(ratioBounds(2, 1, 2, 3));
        widthK.setQBounds(ratioBounds(3, 0), ratioBounds(3, 1), ratioBounds(3, 2));
        lrmsK.setBounds(ratioBounds(3, 3));
        mixK.setQBounds(ratioBounds(4, 0), ratioBounds(4, 1), ratioBounds(4, 2));
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParametersEditor)
};