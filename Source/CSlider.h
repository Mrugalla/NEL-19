#pragma once
#include <JuceHeader.h>
#include <array>

class CSlider :
    public juce::Component
{
public:
    static constexpr float ScrollInterval = .05f;
    static constexpr float ScrollIntervalSensitive = .01f;
    static constexpr float SensitiveSpeed = 1.f / 16.f;

    CSlider(juce::AudioProcessorValueTreeState& apvts, juce::String id) :
        otherSlider(nullptr),
        paramText(nullptr),
        param(apvts.getParameter(id)),
        attach(*param, [this](float v) { attach.setValueAsPartOfGesture(v); }, nullptr),
        dragStartValue(0),
        sensitiveDrag(false), linkedDrag(false), linkedDragInv(false), drag(false)
    {
        attach.sendInitialUpdate();
        setOpaque(false);
        setAlpha(0); // invisible because drawing is handled by UI in space.h
    }
    // SET
    void setCursors(const juce::MouseCursor* hover, const juce::MouseCursor* invisible, const juce::MouseCursor* cursorDisabled) {
        setMouseCursor(*hover);
        cursors[0] = hover;
        cursors[1] = invisible;
        cursors[2] = cursorDisabled;
    }
    // GET
    const float getValue() const { return denormalize(param->getValue()); }
    const float getValueNormalized() const { return param->getValue(); }

    CSlider* otherSlider;
    ParameterTextFields* paramText;
private:
    juce::RangedAudioParameter* param;
    juce::ParameterAttachment attach;
    std::array<const juce::MouseCursor*, 3> cursors; // 0 = hover, 1 = invisible, 2 = disabled
    float dragStartValue;
    bool sensitiveDrag, linkedDrag, linkedDragInv, drag;
    
    // for getValue
    const float denormalize(const float normalized) const { return param->getNormalisableRange().convertFrom0to1(normalized); }

    // mouse event handling
    void mouseEnter(const juce::MouseEvent&) override {
        const auto& c = getMouseCursor();
        if(isEnabled() && c == *cursors[2]) setMouseCursor(*cursors[0]);
        else if(!isEnabled() && c == *cursors[0]) setMouseCursor(*cursors[2]);
    }
    void mouseDown(const juce::MouseEvent& evt) override {
        attach.beginGesture();
        drag = true;
        dragStartValue = getValueNormalized();
        updateDragModes(evt);
    }
    void mouseDrag(const juce::MouseEvent& evt) override {
        updateDragModes(evt);
        if (!sensitiveDrag) mouseDragNormal(evt);
        else mouseDragSensitive(evt);
        updateParamText(evt);
    }
    void mouseUp(const juce::MouseEvent& evt) override {
        attach.endGesture();
        drag = false;
        if (!evt.mouseWasDraggedSinceMouseDown()) {
            const auto value = 1.f - (float)evt.getPosition().y / getHeight();
            attach.setValueAsCompleteGesture(denormalize(value));
        }
        if (sensitiveDrag) stopSensitiveDrag(evt);
        if (linkedDrag) stopLinkedDrag();
    }
    void mouseWheelMove(const juce::MouseEvent& evt, const juce::MouseWheelDetails& wheel) override {
        if (drag) return;
        updateDragModes(evt);
        auto direction = wheel.deltaY < 0 ? -1 : 1;
        auto interval = sensitiveDrag ? ScrollIntervalSensitive : ScrollInterval;
        auto value = juce::jlimit(0.f, 1.f, getValueNormalized() + direction * interval);
        attach.setValueAsCompleteGesture(denormalize(value));
        if (linkedDrag) {
            direction *= linkedDragInv ? -1 : 1;
            value = juce::jlimit(0.f, 1.f, otherSlider->getValueNormalized() + direction * interval);
            otherSlider->attach.setValueAsCompleteGesture(otherSlider->denormalize(value));
        }
        updateParamText(evt);
    }
    void mouseDoubleClick(const juce::MouseEvent&) override { attach.setValueAsCompleteGesture(param->getDefaultValue()); }

    // mouse-Drag&Link-Helpers
    const bool linkExists() const { return otherSlider != nullptr; }
    void updateDragModes(const juce::MouseEvent& evt) {
        if (drag) {
            if (evt.mods.isShiftDown() && !sensitiveDrag) startSensitiveDrag();
            else if (!evt.mods.isShiftDown() && sensitiveDrag) stopSensitiveDrag(evt);
            if (linkExists()) {
                if (evt.mods.isAltDown()) {
                    if (!linkedDrag) startLinkedDrag();
                    if (!linkedDragInv) restartLinkedDrag(true);
                }
                else if (evt.mods.isCtrlDown()) {
                    if (!linkedDrag) startLinkedDrag();
                    if (linkedDragInv && evt.mods.isCtrlDown()) restartLinkedDrag(false);
                }
                else if(linkedDrag) stopLinkedDrag();
            }
        }
        else {
            // mousewheel
            sensitiveDrag = evt.mods.isShiftDown();
            linkedDragInv = evt.mods.isAltDown();
            linkedDrag = linkedDragInv || evt.mods.isCtrlDown();
        }
    }
    void startSensitiveDrag() {
        sensitiveDrag = true;
        dragStartValue = getValueNormalized();
        setCursorInvisible();
    }
    void stopSensitiveDrag(const juce::MouseEvent& evt) {
        sensitiveDrag = false;
        resetCursor(evt);
    }
    void startLinkedDrag() {
        linkedDrag = true;
        otherSlider->dragStartValue = otherSlider->getValueNormalized();
        otherSlider->attach.beginGesture();
    }
    void stopLinkedDrag() {
        linkedDrag = false;
        otherSlider->attach.endGesture();
    }
    void restartLinkedDrag(const bool inverted) {
        linkedDragInv = inverted;
        stopLinkedDrag();
        startLinkedDrag();
    }
    void mouseDragNormal(const juce::MouseEvent& evt) {
        const auto distance = evt.getDistanceFromDragStartY();
        const auto distanceRelative = (float)-distance / getHeight();
        auto value = juce::jlimit(0.f, 1.f, dragStartValue + distanceRelative);
        attach.setValueAsPartOfGesture(denormalize(value));
        if (linkedDrag) {
            auto yOff = (float)evt.getDistanceFromDragStartY();
            auto yOffNorm = yOff / getHeight();
            auto direction = linkedDragInv ? 1 : -1;
            value = juce::jlimit(0.f, 1.f, otherSlider->dragStartValue + yOffNorm * direction);
            otherSlider->attach.setValueAsPartOfGesture(otherSlider->denormalize(value));
        }
    }
    void mouseDragSensitive(const juce::MouseEvent& evt) {
        auto yOff = (float)evt.getDistanceFromDragStartY();
        auto yOffNorm = SensitiveSpeed * yOff / getHeight();
        auto value = juce::jlimit(0.f, 1.f, dragStartValue - yOffNorm);
        attach.setValueAsPartOfGesture(denormalize(value));
        if (linkedDrag) {
            auto direction = linkedDragInv ? 1 : -1;
            value = juce::jlimit(0.f, 1.f, otherSlider->dragStartValue + yOffNorm * direction);
            otherSlider->attach.setValueAsPartOfGesture(otherSlider->denormalize(value));
        }
    }
    
    // other stuff
    void updateParamText(const juce::MouseEvent& evt) {
        if (linkedDrag || !isEnabled()) {
            paramText->disable();
            return;
        }
        paramText->enable();
        if (!sensitiveDrag) paramText->setPosition(getBounds().getTopLeft() + evt.getPosition());
        else paramText->setPosition(getPosition());
    }
    bool cursorInvisible() { return getMouseCursor() != *cursors[0]; }
    void setCursorVisible(){ setMouseCursor(*cursors[0]); }
    void setCursorInvisible() { setMouseCursor(*cursors[1]); }
    void resetCursor(const juce::MouseEvent& evt) {
        auto& desktop = juce::Desktop::getInstance();
        for (int i = 0; i < desktop.getNumMouseSources(); ++i) {
            auto mouse = desktop.getMouseSource(i);
            if (*mouse == evt.source) {
                juce::Point<float> respawnPoint(
                    getScreenX() + (float)getWidth() / 2,
                    getScreenY() + getHeight() * (1 - getValueNormalized())
                );
                mouse->setScreenPosition(respawnPoint);
                setCursorVisible();
                return;
            }
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CSlider)
};

template<typename Float>
struct MidiLearnButton :
    public juce::Component
{
    enum ImageID { Learn, Learning, CC, Pitch };

    MidiLearnButton(tape::Tape<Float>& tape, juce::AudioProcessorValueTreeState& apvts, juce::String paramID, int upscale) :
        ml(tape.getMidiLearn()),
        id(paramID), idType("Type"), idCC("CC"),
        valueTree(),
        images {
            juce::ImageCache::getFromMemory(BinaryData::midiLearn_png, BinaryData::midiLearn_pngSize),
            juce::ImageCache::getFromMemory(BinaryData::midiLearning_png, BinaryData::midiLearning_pngSize),
            juce::ImageCache::getFromMemory(BinaryData::midiCC_png, BinaryData::midiCC_pngSize),
            juce::ImageCache::getFromMemory(BinaryData::midiPitch_png, BinaryData::midiPitch_pngSize)
        },
        state(ml.getState())
    {
        for (auto& i : images)
            i = i.rescaled(i.getWidth() * upscale, i.getHeight() * upscale, juce::Graphics::lowResamplingQuality);
        
        valueTree = apvts.state.getChildWithName(id);
        if (!valueTree.isValid()) {
            valueTree = juce::ValueTree(id);
            apvts.state.appendChild(valueTree, nullptr);
            updateVT();
        }
    }
    ~MidiLearnButton() {
        using namespace tape;
        if (ml.state == MidiLearn<Float>::State::Learning) {
            ml.state = MidiLearn<Float>::State::Off;
            updateVT();
        }   
    }
    void update() {
        using namespace tape;
        if(state == MidiLearn<Float>::State::Learning)
            if (ml.state == MidiLearn<Float>::State::Learned) {
                state = ml.state;
                updateVT();
                repaint();
            }
    }

    tape::MidiLearn<Float>& ml;
    juce::Identifier id, idType, idCC;
    juce::ValueTree valueTree;
    std::array<juce::Image, 4> images;
    typename tape::MidiLearn<Float>::State state;
private:
    void mouseUp(const juce::MouseEvent& evt) override {
        using namespace tape;
        if (ml.state == MidiLearn<Float>::State::Learning) ml.state = state = MidiLearn<Float>::State::Off;
        else ml.state = state = MidiLearn<Float>::State::Learning;
        updateVT();
        repaint();
    }
    
    void paint(juce::Graphics& g) override {
        using namespace tape;
        switch (ml.state) {
        case MidiLearn<Float>::State::Off: g.drawImageAt(images[ImageID::Learn], 0, 0, false);
            break;
        case MidiLearn<Float>::State::Learning: g.drawImageAt(images[ImageID::Learning], 0, 0, false);
            break;
        case MidiLearn<Float>::State::Learned:
            switch (ml.type) {
            case MidiLearn<Float>::Type::Controller: g.drawImageAt(images[ImageID::CC], 0, 0, false);
                break;
            case MidiLearn<Float>::Type::PitchWheel: g.drawImageAt(images[ImageID::Pitch], 0, 0, false);
                break;
            }
            break;
        }
    }

    void updateVT() {
        valueTree.removeAllProperties(nullptr);
        valueTree.setProperty(id, static_cast<int>(ml.getState()), nullptr);
        valueTree.setProperty(idType, static_cast<int>(ml.type), nullptr);
        valueTree.setProperty(idCC, ml.controllerNumber, nullptr);
    }
};

template<typename Float>
struct BufferSizeTextField :
    public juce::Component,
    public juce::KeyListener
{
    enum class State{ Fixed, Typing };

    BufferSizeTextField(Nel19AudioProcessor& p, juce::String paramID, int upscale) :
        audioProcessor(p),
        valueTree(),
        id(paramID),
        image(juce::ImageCache::getFromMemory(BinaryData::bufferSize_png, BinaryData::bufferSize_pngSize)),
        txtImage(juce::Image::ARGB, 26, 7, true),
        font(),
        state(State::Fixed),
        text(),
        bufferSizeInMs(tape::DelaySizeInMS),
        upscaleFactor(upscale)
    {
        valueTree = p.apvts.state.getChildWithName(id);
        if (!valueTree.isValid()) {
            valueTree = juce::ValueTree(id);
            p.apvts.state.appendChild(valueTree, nullptr);
            updateBuffers();
        }

        addKeyListener(this);
        setWantsKeyboardFocus(true);
        bufferSizeInMs = static_cast<float>(valueTree.getProperty(id));
        makeTxtImage();
    }

    Nel19AudioProcessor& audioProcessor;
    juce::ValueTree valueTree;
    juce::Identifier id;
    juce::Image image, txtImage;
    Font font;
    State state;
    juce::String text;
    float bufferSizeInMs;
    int upscaleFactor, txtImageX;
private:
    void paint(juce::Graphics& g) override {
        g.setImageResamplingQuality(juce::Graphics::lowResamplingQuality);
        g.drawImage(image, getLocalBounds().toFloat(), juce::RectanglePlacement::fillDestination, false);
        drawTxtImage(g);
    }

    void mouseUp(const juce::MouseEvent& evt) override {
        switch (state) {
        case State::Fixed:
            state = State::Typing;
            text = "";
            makeTxtImageEmpty();
            break;
        case State::Typing:
            state = State::Fixed;
            updateValue();
            break;
        }
        repaint();
    }
    bool updateValue() {
        if (!numeric(text[0])) return false;
        bool decimals = false;
        for (auto i = 1; i < text.length(); ++i)
            if (text[i] == '.')
                if (decimals) return false;
                else decimals = true;
            else if (!numeric(text[i])) return false;
        auto newValue = text.getFloatValue();
        bufferSizeInMs = clamp(newValue);
        updateBuffers();
        return true;
    }
    float clamp(float v) { return juce::jlimit(tape::DelaySizeInMSMin, tape::DelaySizeInMSMax, v); }
    bool numeric(juce::juce_wchar c) {
        return c == '0' || c == '1' || c == '2' || c == '3' || c == '4' || c == '5' || c == '6' || c == '7' || c == '8' || c == '9';
    }

    void makeTxtImage(float value) {
        auto str = static_cast<juce::String>(value);
        txtImage.clear(txtImage.getBounds());
        juce::Graphics gT{ txtImage };
        auto x = 0;
        auto ltr = font.letter[font.Start];
        for (auto i = 0; i < str.length(); ++i) {
            ltr = font.getLetter(str[i]);
            gT.drawImageAt(font.srcImage.getClippedImage({ ltr.x, 1, ltr.width, 7 }), x, 0, false);
            x += ltr.width;
        }
        const auto widthHalf = 14;
        const auto marginX = 2;
        txtImageX = marginX + widthHalf - x / 2 - 1;
        for (auto yy = 0; yy < txtImage.getHeight(); ++yy)
            for (auto xx = 0; xx < txtImage.getWidth(); ++xx)
                if (txtImage.getPixelAt(xx, yy) == juce::Colour(0xff37946e))
                    txtImage.setPixelAt(xx, yy, juce::Colour(0x00000000));
    }
    void makeTxtImageEmpty() {
        txtImage.clear(txtImage.getBounds());
        juce::Graphics gT{ txtImage };
        auto x = 0;
        auto ltr = font.letter[font.Point];
        for (auto i = 0; i < 3; ++i) {
            gT.drawImageAt(font.srcImage.getClippedImage({ ltr.x, 1, ltr.width, 7 }), x, 0, false);
            x += ltr.width;
        }
        const auto widthHalf = 14;
        const auto marginX = 2;
        txtImageX = marginX + widthHalf - x / 2 - 1;
        for (auto yy = 0; yy < txtImage.getHeight(); ++yy)
            for (auto xx = 0; xx < txtImage.getWidth(); ++xx)
                if (txtImage.getPixelAt(xx, yy) == juce::Colour(0xff37946e))
                    txtImage.setPixelAt(xx, yy, juce::Colour(0x00000000));
    }
    void drawTxtImage(juce::Graphics& g) {
        g.drawImage(txtImage, juce::Rectangle<float>(
            txtImageX * upscaleFactor,
            7 * upscaleFactor,
            26 * upscaleFactor,
            7 * upscaleFactor),
            juce::RectanglePlacement::fillDestination);
    }
    void makeTxtImage() { makeTxtImage(bufferSizeInMs); }

    bool keyPressed(const juce::KeyPress& key, Component*) override {
        if (state != State::Typing) return false;
        if (key == juce::KeyPress::returnKey) {
            state = State::Fixed;
            updateValue();
            makeTxtImage();
            repaint();
            return true;
        }
        else if (key == juce::KeyPress::escapeKey) {
            state = State::Fixed;
            makeTxtImage();
            repaint();
            return true;
        }
        else if (key == juce::KeyPress::backspaceKey) {
            text = text.substring(0, text.length() - 1);
            makeTxtImage(text.getFloatValue());
            repaint();
            return true;
        }
        else {
            text += key.getTextDescription();
            makeTxtImage(text.getFloatValue());
            repaint();
            return true;
        }
        return false;
    }
    bool keyStateChanged(bool, Component*) override { return false; }

    void updateBuffers() {
        valueTree.removeAllProperties(nullptr);
        valueTree.setProperty(id, bufferSizeInMs, nullptr);
        audioProcessor.buffersReallocating = true;
        audioProcessor.tape.setWowDelaySize(bufferSizeInMs);
        audioProcessor.buffersReallocating = false;
    }
};

/*
todo:

CSlider
    - sudden jumps when drag mode changes within drag
    - sudden jump on drag start even if not just click

BufferSizeTextField
    - can't write letters
*/