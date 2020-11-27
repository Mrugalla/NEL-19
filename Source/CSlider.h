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
        linkedDrag(false)
    {
        attach.sendInitialUpdate();
        setOpaque(false);
        setAlpha(0); // invisible because drawing is handled by UI in space.h
    }
    // GET
    const float getValue() const { return denormalize(param->getValue()); }
    const float getValueNormalized() const { return param->getValue(); }

    CSlider* otherSlider;
    ParameterTextFields* paramText;
private:
    juce::RangedAudioParameter* param;
    juce::ParameterAttachment attach;
    float dragStartValue;
    bool sensitiveDrag, linkedDrag;
    
    void mouseDown(const juce::MouseEvent& evt) override {
        updateMods(evt.mods);
        attach.beginGesture();
        if (linkedDrag) {
            dragStartValue = param->getValue();
            otherSlider->dragStartValue = otherSlider->param->getValue();
            otherSlider->attach.beginGesture();
        }
        else if (sensitiveDrag) dragStartValue = param->getValue();
    }
    void mouseDrag(const juce::MouseEvent& evt) override {
        updateMods(evt.mods);
        if (!sensitiveDrag) {
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
        else {
            auto yOff = (float)evt.getDistanceFromDragStartY();
            auto yOffNorm = SensitiveSpeed * yOff / getHeight();
            auto value = juce::jlimit(0.f, 1.f, dragStartValue - yOffNorm);
            attach.setValueAsPartOfGesture(denormalize(value));
            if (linkedDrag) {
                value = juce::jlimit(0.f, 1.f, otherSlider->dragStartValue + yOffNorm);
                otherSlider->attach.setValueAsPartOfGesture(otherSlider->denormalize(value));
            }
        }
        updateParamText(evt);
    }
    void mouseUp(const juce::MouseEvent& evt) override {
        updateMods(evt.mods);
        attach.endGesture();
        if (linkedDrag) otherSlider->attach.endGesture();
    }
    void mouseWheelMove(const juce::MouseEvent& evt, const juce::MouseWheelDetails& wheel) override {
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
        sensitiveDrag = mods.isShiftDown();
        linkedDrag = mods.isAltDown() && otherSlider != nullptr;
    }
    const float denormalize(const float normalized) const { return param->getNormalisableRange().convertFrom0to1(normalized); }
    void updateValue(float value) {
        attach.setValueAsPartOfGesture(value);
    }
    void updateParamText(const juce::MouseEvent& evt) {
        paramText->setPosition(getBounds().getTopLeft() + evt.getPosition());
        paramText->enable();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CSlider)
};
