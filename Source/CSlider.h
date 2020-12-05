#pragma once
#include <JuceHeader.h>

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
    void setCursors(juce::MouseCursor* hover, juce::MouseCursor* invisible) {
        setMouseCursor(*hover);
        cursors[0] = hover;
        cursors[1] = invisible;
    }
    // GET
    const float getValue() const { return denormalize(param->getValue()); }
    const float getValueNormalized() const { return param->getValue(); }

    CSlider* otherSlider;
    ParameterTextFields* paramText;
private:
    juce::RangedAudioParameter* param;
    juce::ParameterAttachment attach;
    std::array<juce::MouseCursor*, 2> cursors; // 0 = hover, 1 = invisible
    float dragStartValue;
    bool sensitiveDrag, linkedDrag, linkedDragInv, drag;
    
    // for getValue
    const float denormalize(const float normalized) const { return param->getNormalisableRange().convertFrom0to1(normalized); }

    // mouse event handling
    void mouseDown(const juce::MouseEvent& evt) override {
        attach.beginGesture();
        drag = true;
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
    void mouseDoubleClick(const juce::MouseEvent& evt) override { attach.setValueAsCompleteGesture(param->getDefaultValue()); }

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
        auto y = juce::jlimit(0, getHeight(), evt.getPosition().getY());
        auto value = 1.f - (float)y / getHeight();
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
        if (linkedDrag) {
            paramText->disable();
            return;
        }
        paramText->enable();
        if (!sensitiveDrag) paramText->setPosition(getBounds().getTopLeft() + evt.getPosition());
        else paramText->setPosition(getPosition());
    }
    const bool cursorInvisible() { return &getMouseCursor() != cursors[0]; }
    void setCursorVisible(){ setMouseCursor(*cursors[0]); }
    void setCursorInvisible() { setMouseCursor(*cursors[1]); }
    void resetCursor(const juce::MouseEvent& evt) {
        auto& desktop = juce::Desktop::getInstance();
        for (int i = 0; i < desktop.getNumMouseSources(); ++i) {
            auto mouse = desktop.getMouseSource(i);
            if (*mouse == evt.source) {
                juce::Point<float> respawnPoint(
                    getScreenX() + getWidth() / 2,
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

/*
todo:

- sudden jumps when drag mode changes within drag
- crash on rare condition (mouseUp, endGesture)

*/