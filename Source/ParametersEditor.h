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
            auto angle = dragLine.getAngle();
            angle -= wheel.startAngle;
            while (angle < 0) angle += nelG::Tau;
            if (angle < wheel.angleRange) {
                const auto size = static_cast<float>(wheel.choices.size());
                const auto value = size * angle / wheel.angleRange;
                const auto rValue = static_cast<int>(value);
                wheel.selectedChoice = juce::jlimit(0, static_cast<int>(wheel.choices.size() - 1), rValue);
            }
            else if (angle < wheel.angleRange + wheel.overshootAngle)
                wheel.selectedChoice = wheel.choices.size() - 1;
            else if (angle > nelG::Tau - wheel.overshootAngle)
                wheel.selectedChoice = 0;
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
        attach(*param, [this](float v) { attach.setValueAsCompleteGesture(v); repaint(); }, nullptr),
        button(*this, id, tooltp),
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
        attach.sendInitialUpdate();
    }
    void init(juce::Rectangle<int> buttonBounds, float angleStart = 0.f, float rangeAngle = 3.14f, float overshoot = 3.14 / 8.f) {
        startAngle = angleStart;
        angleRange = rangeAngle;
        overshootAngle = overshoot;
        button.setBounds(buttonBounds);
    }
private:
    juce::RangedAudioParameter* param;
    juce::ParameterAttachment attach;
    Button button;
    std::vector<std::unique_ptr<Choice>> choices;
    float startAngle, angleRange, overshootAngle;
    State state;
    int selectedChoice;

    void resized() override {
        const auto bounds = getLocalBounds().toFloat();
        const auto minDimen = std::min(bounds.getWidth(), bounds.getHeight());
        const auto radius = minDimen * .5f;
        const auto choiceDistance = radius * .7f;
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
            attach(*param, [this](float v) { updateValue(v); }, nullptr),
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
            if (evt.mouseWasClicked()) return; // why not drag? idk
            const auto speed = evt.mods.isShiftDown() ? .005f : .05f;
            const auto gestr = wheel.deltaY > 0 ? speed : -speed;
            attach.setValueAsCompleteGesture(
                param->convertFrom0to1(param->getValue() + gestr)
            );
            repaint();
        }
        void paint(juce::Graphics& g) override {
            const auto bounds = getLocalBounds().toFloat().reduced(nelG::Thicc);
            const auto width = bounds.getWidth();
            const auto height = bounds.getHeight();
            const auto diameter = std::min(width, height);
            juce::Point<float> centre(width * .5f, height * .5f);
            const auto radius = diameter * .5f;
            const auto tickLength = radius - nelG::Thicc * 2.f;
            juce::Line<float> tick(0, 0, 0, tickLength);
            const auto normalValue = param->getValue();
            const auto translation = juce::AffineTransform::translation(centre);
            const auto startAngle = juce::MathConstants<float>::pi * .25f;
            const auto angleLength = juce::MathConstants<float>::twoPi - startAngle;
            const auto knobRadius = radius;
            const auto knobX = centre.x - knobRadius;
            const auto knobY = centre.y - knobRadius;
            const auto knobWidth = 2.f * knobRadius;
            juce::Rectangle<float> knobArea(knobX, knobY, knobWidth, knobWidth);
            g.setColour(juce::Colour(nelG::ColBlack));
            g.fillEllipse(knobArea);
            g.setColour(juce::Colour(nelG::ColGreen));
            g.drawEllipse(knobArea, nelG::Thicc);
            for (auto x = 0.f; x <= 1.f; x += 1.f / 12.f) {
                auto tickT = tick;
                const auto angle = startAngle + (x * normalValue) * (angleLength - startAngle);
                const auto rotation = juce::AffineTransform::rotation(angle);
                tickT.applyTransform(rotation.followedBy(translation));
                g.drawLine(tickT, nelG::Thicc);
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
        name.setFont(utils.font);
        textbox.setFont(utils.font);
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
    public NELComp {

    ButtonSwitch(Nel19AudioProcessor& p, nelDSP::param::ID id, NELGUtil& u, juce::String tooltp = "") :
        NELComp(p, u, tooltp),
        param(p.apvts.getParameter(nelDSP::param::getID(id))),
        attach(*param, [this](float v) { attach.setValueAsCompleteGesture(v); repaint(); })
    {
        setMouseCursor(u.cursors[NELGUtil::Cursor::Hover]);
    }
private:
    juce::RangedAudioParameter* param;
    juce::ParameterAttachment attach;

    void paint(juce::Graphics& g) override {
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
        shapeK(p, nelDSP::param::ID::Shape, utils, "Defines the shape of the randomizer."),
        widthK(p, nelDSP::param::ID::Width, utils, "The offset of each channel's Random LFO."),
        mixK(p, nelDSP::param::ID::Mix, utils, "Turn Mix down for Chorus/Flanger-Sounds.")
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
    void resized() override {
        const auto bX = static_cast<float>(bounds.getX());
        const auto bY = static_cast<float>(bounds.getY());
        const auto width = static_cast<float>(bounds.getWidth());
        const auto height = static_cast<float>(bounds.getHeight());
        auto x = bX;
        const auto y = bY;
        const auto knobCount = 5.f;
        const auto objWidth = width / knobCount;
        const auto knobHeight = height * .75f;
        depthK.setBounds(juce::Rectangle<float>(x, y, objWidth, knobHeight).toNearestInt());
        depthMaxK.init(
            juce::Rectangle<int>(
                static_cast<int>(objWidth + depthK.dial.getX()),
                depthK.getBottom(),
                depthK.dial.getWidth(),
                (getHeight() - depthK.getBottom()) * 2 / 3
                ),
            nelG::Tau * .75f,
            nelG::Tau * .5f,
            nelG::PiQuart * .25f
        );
        depthMaxK.setBounds(juce::Rectangle<float>(x - objWidth, y, objWidth * 3, height).toNearestInt());
        x += objWidth;
        freqK.setBounds(juce::Rectangle<float>(x, y, objWidth, knobHeight).toNearestInt());
        x += objWidth;
        shapeK.setBounds(juce::Rectangle<float>(x, y, objWidth, knobHeight).toNearestInt());
        x += objWidth;
        widthK.setBounds(juce::Rectangle<float>(x, y, objWidth, knobHeight).toNearestInt());
        lrmsK.setBounds(juce::Rectangle<float>(x + static_cast<float>(depthK.dial.getX()), knobHeight, static_cast<float>(depthK.dial.getWidth()), static_cast<float>(getHeight() - depthK.getBottom()) * 2.f / 3.f).toNearestInt());
        x += objWidth;
        mixK.setBounds(juce::Rectangle<float>(x, y, objWidth, knobHeight).toNearestInt());
    }
private:
    QuickAccessWheel depthMaxK;
    ButtonSwitch lrmsK;
    Knob depthK, freqK, shapeK, widthK, mixK;
    juce::Rectangle<int> bounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParametersEditor)
};