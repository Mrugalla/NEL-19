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
        attach(*param, [this](float v) { updateValue(v); }, nullptr),
        dragStartValue(0),
        sensitiveDrag(false),
        linkedDrag(false),
        drag(false)
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
    bool sensitiveDrag, linkedDrag, drag;
    
    void mouseDown(const juce::MouseEvent& evt) override {
        updateMods(evt.mods);
        attach.beginGesture();
        dragStartValue = param->getValue();
        drag = true;
        if (linkedDrag) {
            otherSlider->dragStartValue = otherSlider->param->getValue();
            otherSlider->attach.beginGesture();
            if (sensitiveDrag) setMouseCursor(*cursors[1]);
        }
        else if (sensitiveDrag)
            setMouseCursor(*cursors[1]);
    }
    void mouseDrag(const juce::MouseEvent& evt) override {
        updateMods(evt.mods);
        if (!sensitiveDrag) mouseDragNormal(evt);
        else mouseDragSensitive(evt);

        updateParamText(evt);
    }
    void mouseUp(const juce::MouseEvent& evt) override {
        updateMods(evt.mods);
        resetCursor(evt);
        attach.endGesture();
        drag = false;
        if (linkedDrag) otherSlider->attach.endGesture();
    }
    void mouseWheelMove(const juce::MouseEvent& evt, const juce::MouseWheelDetails& wheel) override {
        if (drag) return;
        updateMods(evt.mods);
        auto direction = wheel.deltaY < 0 ? -1 : 1;
        auto interval = sensitiveDrag ? ScrollIntervalSensitive : ScrollInterval;
        auto value = juce::jlimit(0.f, 1.f, param->getValue() + direction * interval);
        attach.setValueAsCompleteGesture(denormalize(value));
        if (linkedDrag) {
            direction *= -1;
            value = juce::jlimit(0.f, 1.f, otherSlider->param->getValue() + direction * interval);
            otherSlider->attach.setValueAsCompleteGesture(otherSlider->denormalize(value));
        }
        updateParamText(evt);
    }
    void mouseDoubleClick(const juce::MouseEvent& evt) override {
        attach.setValueAsCompleteGesture(param->getDefaultValue());
    }

    void updateMods(const juce::ModifierKeys& mods) {
        if (linkedDrag && !mods.isAltDown())
            otherSlider->attach.endGesture();
        sensitiveDrag = mods.isShiftDown();
        linkedDrag = mods.isAltDown() && otherSlider != nullptr;
    }
    const float denormalize(const float normalized) const { return param->getNormalisableRange().convertFrom0to1(normalized); }
    void updateValue(float value) {
        attach.setValueAsPartOfGesture(value);
    }
    void updateParamText(const juce::MouseEvent& evt) {
        paramText->enable();
        if (!sensitiveDrag) paramText->setPosition(getBounds().getTopLeft() + evt.getPosition());
        else paramText->setPosition({
            getX(),
            getY()
        });
    }
    void resetCursor(const juce::MouseEvent& evt) {
        if (&getMouseCursor() != cursors[0]) setMouseCursor(*cursors[0]);
        if (!sensitiveDrag) return;
        auto& desktop = juce::Desktop::getInstance();
        for (int i = 0; i < desktop.getNumMouseSources(); ++i) {
            auto mouse = desktop.getMouseSource(i);
            if (*mouse == evt.source) {
                juce::Point<float> respawnPoint(
                    getScreenX() + getWidth() / 2,
                    getScreenY() + getHeight() * (1 - getValueNormalized())
                );
                mouse->setScreenPosition(respawnPoint);
                return;
            }    
        }
    }

    // mouseDragHelpers
    void mouseDragNormal(const juce::MouseEvent& evt) {
        auto y = juce::jlimit(0, getHeight(), evt.getPosition().getY());
        auto value = 1.f - (float)y / getHeight();
        attach.setValueAsPartOfGesture(denormalize(value));
        if (linkedDrag) {
            auto yOff = (float)evt.getDistanceFromDragStartY();
            auto yOffNorm = yOff / getHeight();
            value = juce::jlimit(0.f, 1.f, otherSlider->dragStartValue + yOffNorm);
            otherSlider->attach.setValueAsPartOfGesture(otherSlider->denormalize(value));
        }
    }
    void mouseDragSensitive(const juce::MouseEvent& evt) {
        auto yOff = (float)evt.getDistanceFromDragStartY();
        auto yOffNorm = SensitiveSpeed * yOff / getHeight();
        auto value = juce::jlimit(0.f, 1.f, dragStartValue - yOffNorm);
        attach.setValueAsPartOfGesture(denormalize(value));
        if (linkedDrag) {
            value = juce::jlimit(0.f, 1.f, otherSlider->dragStartValue + yOffNorm);
            otherSlider->attach.setValueAsPartOfGesture(otherSlider->denormalize(value));
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CSlider)
};
