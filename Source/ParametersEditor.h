#pragma once
#include <JuceHeader.h>

class QuickAccessWheel :
    public juce::Component {
    enum class State { Def, Drag };
    
    struct Button :
        public juce::Component {
        Button(QuickAccessWheel& w, nelDSP::param::ID id) :
            wheel(w),
            name(nelDSP::param::getName(id))
        {}
    private:
        QuickAccessWheel& wheel;
        juce::String name;
        void paint(juce::Graphics& g) {
            const auto thicc = 3.f;
            const auto rounded = 3.f;
            const auto bounds = getLocalBounds().toFloat().reduced(thicc);
            g.setColour(juce::Colour(nelG::ColBlack));
            g.fillRoundedRectangle(bounds, rounded);
            if (wheel.state == State::Def) {
                g.setColour(juce::Colour(nelG::ColGreen));
                g.drawRoundedRectangle(bounds, rounded, thicc);
                g.drawFittedText(name + "\n" + wheel.param->getCurrentValueAsText(), getLocalBounds(), juce::Justification::centred, 1, 1);
            }
            else {
                if (wheel.selectedChoice == -1) {
                    g.setColour(juce::Colour(nelG::ColRed));
                    g.drawFittedText("X", getLocalBounds(), juce::Justification::centred, 1, 1);
                    g.drawRoundedRectangle(bounds, rounded, thicc);
                    return;
                }
                g.setColour(juce::Colour(nelG::ColGreen));
                g.drawRoundedRectangle(bounds, rounded, thicc);
                g.drawFittedText(name + "\n" + wheel.param->getCurrentValueAsText(), getLocalBounds(), juce::Justification::centred, 1, 1);
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
            auto angle = dragLine.getAngle();
            angle -= wheel.startAngle;
            while (angle < 0) angle += nelG::Tau;
            
            if (angle < wheel.angleRange) {
                const auto size = static_cast<float>(wheel.choices.size());
                const auto value = size * angle / wheel.angleRange;
                const auto rValue = static_cast<int>(value);
                wheel.selectedChoice = juce::jlimit(0, static_cast<int>(wheel.choices.size()), rValue);
            }
            else
                wheel.selectedChoice = -1;
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
    };

    struct Choice :
        public juce::Component {
        Choice(QuickAccessWheel& w, const juce::String& vs, int i) :
            wheel(w),
            valueString(vs),
            idx(i)
        {
        }
        QuickAccessWheel& wheel;
        juce::String valueString;
        int idx;
    private:
        void paint(juce::Graphics& g) override {
            const auto thicc = 3.f;
            const auto rounded = 3.f;
            const auto bounds = getLocalBounds().toFloat().reduced(thicc);
            const auto curValue = static_cast<int>(wheel.param->convertFrom0to1(wheel.param->getValue()));
            if (idx != curValue) g.setColour(juce::Colour(0x99ffffff));
            else g.setColour(juce::Colour(nelG::ColYellow));
            g.fillRoundedRectangle(bounds, 3.f);
            g.setColour(juce::Colour(nelG::ColBlack));
            if (idx == wheel.selectedChoice) g.drawRoundedRectangle(bounds, rounded, thicc);
            g.drawFittedText(valueString, getLocalBounds(), juce::Justification::centred, 1, 0);
        }
    };
public:
    QuickAccessWheel(Nel19AudioProcessor& p, nelDSP::param::ID id, const NELGUtil& u) :
        nelGUtil(u),
        param(p.apvts.getParameter(nelDSP::param::getID(id))),
        attach(*param, [this](float v) { attach.setValueAsCompleteGesture(v); }, nullptr),
        button(*this, id),
        choices(),
        startAngle(nelG::Tau * .75f), angleRange(nelG::Tau * .5f),
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
        setMouseCursor(nelGUtil.cursors[NELGUtil::Cursor::Norm]);
        attach.sendInitialUpdate();
    }
    void init(juce::Rectangle<int> buttonBounds, float angleStart = 0.f, float rangeAngle = 3.14f) {
        startAngle = angleStart;
        angleRange = rangeAngle;
        button.setBounds(buttonBounds);
    }
private:
    const NELGUtil& nelGUtil;
    juce::RangedAudioParameter* param;
    juce::ParameterAttachment attach;
    Button button;
    std::vector<std::unique_ptr<Choice>> choices;
    float startAngle, angleRange;
    State state;
    int selectedChoice;

    void resized() override {
        const auto bounds = getLocalBounds().toFloat();
        const auto minDimen = std::min(bounds.getWidth(), bounds.getHeight());
        const auto radius = minDimen * .5f;
        const auto choiceDistance = radius * .75f;
        const auto choiceWidth = radius - choiceDistance;
        const auto choiceRadius = choiceWidth * .5f;

        const auto choicesMax = choices.size() - 1;
        const auto choicesMaxInv = 1.f / choicesMax;
        const auto centre = button.getBounds().getCentre().toFloat();
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
            choices[c].get()->setBounds(cArea.toNearestInt());
        }
    }
    void paint(juce::Graphics& g) override {
        //g.setColour(juce::Colours::red);
        //g.drawRect(getLocalBounds(), 2);
    }
};

struct Knob :
    public juce::Component
{
    struct Dial :
        public juce::Component
    {
        Dial(Nel19AudioProcessor& p, nelDSP::param::ID id, const NELGUtil& u) :
            nelGUtil(u),
            param(p.apvts.getParameter(nelDSP::param::getID(id))),
            attach(*param, [this](float v) { updateValue(v); }, nullptr),
            lastDistance(0)
        {
            setMouseCursor(nelGUtil.cursors[NELGUtil::Cursor::Norm]);
        }
        
        void updateValue(float v) {
            attach.setValueAsPartOfGesture(v);
        }
        juce::RangedAudioParameter* param;
        juce::ParameterAttachment attach;
    private:
        const NELGUtil& nelGUtil;
        float lastDistance;
        void mouseEnter(const juce::MouseEvent&) override {
            setMouseCursor(nelGUtil.cursors[NELGUtil::Cursor::Hover]);
        }
        void mouseExit(const juce::MouseEvent&) override {
            setMouseCursor(nelGUtil.cursors[NELGUtil::Cursor::Norm]);
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
            repaint();
            lastDistance = dist;
        }
        void mouseWheelMove(const juce::MouseEvent& evt, const juce::MouseWheelDetails& wheel) override {
            const auto speed = evt.mods.isShiftDown() ? .005f : .05f;
            const auto gestr = wheel.deltaY > 0 ? speed : -speed;
            attach.setValueAsCompleteGesture(
                param->convertFrom0to1(param->getValue() + gestr)
            );
            repaint();
        }
        void paint(juce::Graphics& g) override {
            const auto thicc = 3.f;
            const auto width = static_cast<float>(getWidth());
            const auto height = static_cast<float>(getHeight());
            const auto diameter = std::min(width, height);
            juce::Point<float> centre(width * .5f, height * .5f);
            const auto radius = diameter * .5f;
            const auto tickLength = radius - thicc * 4.f;
            juce::Line<float> tick(0, 0, 0, tickLength);
            const auto normalValue = param->getValue();
            const auto translation = juce::AffineTransform::translation(centre);
            const auto startAngle = juce::MathConstants<float>::pi * .25f;
            const auto angleLength = juce::MathConstants<float>::twoPi - startAngle;
            const auto knobRadius = radius - thicc * 2.f;
            const auto knobX = centre.x - knobRadius;
            const auto knobY = centre.y - knobRadius;
            const auto knobWidth = 2.f * knobRadius;
            juce::Rectangle<float> knobArea(knobX, knobY, knobWidth, knobWidth);
            g.setColour(juce::Colour(nelG::ColBlack));
            g.fillEllipse(knobArea);
            g.setColour(juce::Colour(nelG::ColGreen));
            g.drawEllipse(knobArea, thicc);
            for (auto x = 0.f; x <= 1.f; x += 1.f / 16.f) {
                auto tickT = tick;
                const auto angle = startAngle + (x * normalValue) * (angleLength - startAngle);
                const auto rotation = juce::AffineTransform::rotation(angle);
                tickT.applyTransform(rotation.followedBy(translation));
                g.drawLine(tickT, thicc);
            }
        }
    };

    Knob(Nel19AudioProcessor& p, nelDSP::param::ID id, const NELGUtil& u) :
        nelGUtil(u),
        param(p.apvts.getParameter(nelDSP::param::getID(id))),
        attach(*param, [this](float v) { updateValue(v); }, nullptr),
        dial(p, id, u),
        name("", nelDSP::param::getName(id)),
        textbox("", "-")
    {
        name.setMouseCursor(nelGUtil.cursors[NELGUtil::Cursor::Norm]);
        textbox.setMouseCursor(nelGUtil.cursors[NELGUtil::Cursor::Norm]);
        name.setColour(juce::Label::ColourIds::textColourId, juce::Colour(nelG::ColBlack));
        name.setJustificationType(juce::Justification::centredBottom);
        textbox.setColour(juce::Label::ColourIds::textColourId, juce::Colour(nelG::ColBlack));
        textbox.setJustificationType(juce::Justification::centredTop);
        addAndMakeVisible(name);
        addAndMakeVisible(dial);
        addAndMakeVisible(textbox);
        attach.sendInitialUpdate();
    }
    
    void updateValue(float v) {
        textbox.setText(param->getCurrentValueAsText(), juce::NotificationType::sendNotificationAsync);
        attach.setValueAsPartOfGesture(v);
    }

    const NELGUtil& nelGUtil;
    juce::RangedAudioParameter* param;
    juce::ParameterAttachment attach;
    Dial dial;
    juce::Label name, textbox;
private:
    void resized() override {
        auto x = 0.f;
        const auto width = static_cast<float>(getWidth());
        const auto height = static_cast<float>(getHeight());
        const auto sliderHeight = height * .6f;
        const auto labelHeight = (height - sliderHeight) * .5f;
        auto y = 0.f;

        const auto dialWidth = width * .625f;
        const auto dialX = (width - dialWidth) * .5f;
        name.setBounds(juce::Rectangle<float>(x, y, width, labelHeight).toNearestInt());
        y += labelHeight;
        dial.setBounds(juce::Rectangle<float>(dialX, y, dialWidth, sliderHeight).toNearestInt());
        y += sliderHeight;
        textbox.setBounds(juce::Rectangle<float>(x, y, width, labelHeight).toNearestInt());
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Knob)
};

struct ButtonSwitch :
    public juce::Component {

    ButtonSwitch(Nel19AudioProcessor& p, nelDSP::param::ID id, const NELGUtil& u) :
        param(p.apvts.getParameter(nelDSP::param::getID(id))),
        attach(*param, [this](float v) { attach.setValueAsCompleteGesture(v); })
    {
        setMouseCursor(u.cursors[NELGUtil::Cursor::Hover]);
    }

private:
    juce::RangedAudioParameter* param;
    juce::ParameterAttachment attach;

    void paint(juce::Graphics& g) override {
        const auto thicc = 3.f;
        const auto rounded = 3.f;
        const auto bounds = getLocalBounds().toFloat().reduced(thicc);
        g.setColour(juce::Colour(nelG::ColBlack));
        g.fillRoundedRectangle(bounds, rounded);
        g.setColour(juce::Colour(nelG::ColGreen));
        g.drawRoundedRectangle(bounds, rounded, thicc);
        g.drawFittedText(param->getCurrentValueAsText(), getLocalBounds(), juce::Justification::centred, 1);
    }
    void mouseUp(const juce::MouseEvent& evt) override {
        if (evt.mouseWasDraggedSinceMouseDown()) return;
        attach.setValueAsCompleteGesture(1.f - param->getValue());
        repaint();
    }
};

struct ParametersEditor :
    public juce::Component
{
    ParametersEditor(Nel19AudioProcessor& p, const NELGUtil& u) :
        utils(u),
        processor(p),
        depthMaxK(p, nelDSP::param::ID::DepthMax, utils),
        lrmsK(p, nelDSP::param::ID::LRMS, utils),
        depthK(p, nelDSP::param::ID::Depth, utils),
        freqK(p, nelDSP::param::ID::Freq, utils),
        widthK(p, nelDSP::param::ID::Width, utils),
        mixK(p, nelDSP::param::ID::Mix, utils)
    {
        addAndMakeVisible(depthK);
        addAndMakeVisible(freqK);
        addAndMakeVisible(widthK);
        addAndMakeVisible(mixK);
        addAndMakeVisible(depthMaxK);
        addAndMakeVisible(lrmsK);
    }
    void setQBounds(const juce::Rectangle<int> b) {
        bounds = b;
        setBounds(getParentComponent()->getBounds());
    }
    void resized() override {
        const auto bX = static_cast<float>(bounds.getX());
        const auto bY = static_cast<float>(bounds.getY());
        const auto width = static_cast<float>(bounds.getWidth());
        const auto height = static_cast<float>(bounds.getHeight());
        auto x = bX;
        const auto y = bY;
        const auto objWidth = width / 4.f;
        const auto knobHeight = height * .75f;
        depthK.setBounds(juce::Rectangle<float>(x, y, objWidth, knobHeight).toNearestInt());
        depthMaxK.init(
            juce::Rectangle<int>(objWidth + depthK.dial.getX(), depthK.getBottom(), depthK.dial.getWidth(), (getHeight() - depthK.getBottom()) * 2 / 3),
            nelG::Tau * .75f,
            nelG::Tau * .5f
        );
        depthMaxK.setBounds(juce::Rectangle<float>(x - objWidth, y, objWidth * 3, height).toNearestInt());
        x += objWidth;
        freqK.setBounds(juce::Rectangle<float>(x, y, objWidth, knobHeight).toNearestInt());
        x += objWidth;
        widthK.setBounds(juce::Rectangle<float>(x, y, objWidth, knobHeight).toNearestInt());
        lrmsK.setBounds(juce::Rectangle<float>(x + depthK.dial.getX(), knobHeight, depthK.dial.getWidth(), (getHeight() - depthK.getBottom()) * 2 / 3).toNearestInt());
        x += objWidth;
        mixK.setBounds(juce::Rectangle<float>(x, y, objWidth, knobHeight).toNearestInt());
    }
private:
    const NELGUtil& utils;
    Nel19AudioProcessor& processor;
    QuickAccessWheel depthMaxK;
    ButtonSwitch lrmsK;
    Knob depthK, freqK, widthK, mixK;
    juce::Rectangle<int> bounds;
    void mouseEnter(const juce::MouseEvent&) override {
        setMouseCursor(utils.cursors[NELGUtil::Cursor::Norm]);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParametersEditor)
};