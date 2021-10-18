#pragma once

namespace pComp {
    static std::function<void(juce::Graphics&, const juce::Rectangle<float>&, const Utils&, bool)> onPaintPolarity() {
        return [](juce::Graphics& g, const juce::Rectangle<float>& bounds, const Utils& utils, bool enabled) {
            g.setColour(enabled ? utils.colours[Utils::ColourID::Interactable] : utils.colours[Utils::ColourID::Interactable].withMultipliedAlpha(.2f));
            const auto b = nelG::maxQuadIn(bounds.reduced(nelG::Thicc));
            g.drawEllipse(b, nelG::Thicc);
            g.drawLine(juce::Line<float>(b.getBottomLeft(), b.getTopRight()), nelG::Thicc);
        };
    }

    struct Parameter :
        public Comp,
        public modSys2::Identifiable
    {
        Parameter(Nel19AudioProcessor& p, Utils& u, juce::String&& tooltp, juce::String&& _name, param::ID pID,
            std::function<void(float)> onParameterChange, bool _modulatable, bool _active = true) :
            Comp(p, u, tooltp, Utils::Cursor::Hover),
            modSys2::Identifiable(param::getID(pID)),
            rap(*p.apvts.getParameter(this->id)),
            attach(rap, onParameterChange),
            modulatable(_modulatable),
            active(_active)
        {
            setName(_name);
            setBufferedToImage(true);
        }
        /* only implemented by modulatables */
        virtual void updateModSys(const std::shared_ptr<modSys2::Matrix>& matrix){}
        void setValueNormalized(const float v) { attach.setValueAsCompleteGesture(rap.convertFrom0to1(v)); }
        void setValueDenormalized(const float v) { attach.setValueAsCompleteGesture(v); }
        const float getDenormalized() const noexcept { return rap.convertFrom0to1(rap.getValue()); }
        const float getNormalized() const noexcept { return rap.getValue(); }
        juce::RangedAudioParameter& rap;
        juce::ParameterAttachment attach;
        bool modulatable, active;
    protected:
        void updatePopUp(Utils& u) { u.updatePopUp(rap.getCurrentValueAsText()); }
        void mouseDown(const juce::MouseEvent& evt) override {
            utils.updatePopUpPoint(evt.getScreenPosition());
        }
        void mouseUp(const juce::MouseEvent&) override {
            utils.updatePopUpPoint();
        }
    };

    class Knob :
        public Parameter
    {
        struct ModulatorDial :
            public Comp
        {
            ModulatorDial(Nel19AudioProcessor& p, Utils& u, Knob& par) :
                Comp(p, u, "modulation attenuvertor", Utils::Cursor::Hover),
                param(par),
                startAttenuvertor(0.f),
                selected(false),
                tryRemove(false)
            {
                setBufferedToImage(true);
            }
            bool select(bool e) noexcept {
                if (selected != e) {
                    selected = e;
                    repaint();
                    return true;
                }
                return false;
            }
            const inline bool isSelected() const noexcept { return selected; }
            const inline bool isTryingToRemove() const noexcept { return tryRemove; }
        protected:
            Knob& param;
            float startAttenuvertor;
            bool selected, tryRemove;

            void paint(juce::Graphics& g) override {
                const auto bounds = getLocalBounds().toFloat().reduced(nelG::Thicc);
                g.setColour(utils.colours[Utils::Background]);
                g.fillEllipse(bounds);
                {
                    if (selected)
                        if (tryRemove) {
                            g.setColour(utils.colours[Utils::Abort]);
                            g.drawFittedText("!", getLocalBounds(), juce::Justification::centred, 1);
                        }
                        else
                            g.setColour(utils.colours[Utils::Modulation]);
                    else
                        g.setColour(utils.colours[Utils::Normal]);
                }
                g.drawEllipse(bounds, nelG::Thicc);
            }

            void mouseDown(const juce::MouseEvent& evt) override {
                const auto selectedMod = processor.matrix->getSelectedModulator();
                if (selectedMod == nullptr) return;
                auto dest = selectedMod->getDestination(param.id);
                selected = dest != nullptr;
                if (!selected) return;
                startAttenuvertor = dest->getValue();
            }
            void mouseDrag(const juce::MouseEvent& evt) override {
                if (!selected) return;
                const auto height = static_cast<float>(getHeight());
                const auto dist = -evt.getDistanceFromDragStartY();
                const auto speed = evt.mods.isShiftDown() ? .01f : .1f;
                const auto distRel = speed * dist / height;
                const auto atten = juce::jlimit(-1.f, 1.f, startAttenuvertor + distRel);
                auto selectedMod = processor.matrix->getSelectedModulator();
                selectedMod->setAttenuvertor(param.id, atten);
                param.attenuvertor = atten;
                param.repaint();
            }
            void mouseUp(const juce::MouseEvent& evt) override {
                if (evt.mouseWasDraggedSinceMouseDown()) return;
                else if (evt.mods.isLeftButtonDown()) return;
                // right clicks only
                const auto selectedMod = processor.matrix->getSelectedModulator();
                if (selectedMod == nullptr) return;
                auto dest = selectedMod->getDestination(param.id);
                selected = dest != nullptr;
                if (!selected) return;
                if (!tryRemove) {
                    tryRemove = true;
                    return param.repaint();
                }
                auto mtrx = processor.matrix.getCopyOfUpdatedPtr();
                mtrx->removeDestination(selectedMod->id, dest->id);
                processor.matrix.replaceUpdatedPtrWith(mtrx);
                tryRemove = selected = false;
                param.repaint();
            }
            void mouseExit(const juce::MouseEvent& evt) override {
                tryRemove = false;
                param.repaint();
            }

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatorDial)
        };

        struct Dial :
            public Comp
        {
            Dial(Nel19AudioProcessor& p, Utils& u, juce::String&& tooltp, Knob& _knob) :
                Comp(p, u, tooltp, Utils::Cursor::Hover),
                knob(_knob)
            {
                setInterceptsMouseClicks(false, false);
            }
        protected:
            const Knob& knob;
            void paint(juce::Graphics& g) override {
                const auto width = static_cast<float>(getWidth());
                const auto height = static_cast<float>(getHeight());
                const auto value = knob.rap.getValue();
                juce::PathStrokeType strokeType(nelG::Thicc, juce::PathStrokeType::JointStyle::curved, juce::PathStrokeType::EndCapStyle::rounded);
                const juce::Point<float> centre(width * .5f, height * .5f);
                const auto radius = std::min(centre.x, centre.y) - nelG::Thicc;
                const auto startAngle = -nelG::PiQuart * 3.f;
                const auto endAngle = nelG::PiQuart * 3.f;
                const auto angleRange = endAngle - startAngle;
                const auto valueAngle = startAngle + angleRange * value;

                g.setColour(utils.colours[Utils::Normal]);
                juce::Path pathNorm;
                pathNorm.addCentredArc(centre.x, centre.y, radius, radius,
                    0.f, startAngle, endAngle,
                    true
                );
                const auto innerRad = radius - nelG::Thicc2;
                pathNorm.addCentredArc(centre.x, centre.y, innerRad, innerRad,
                    0.f, startAngle, endAngle,
                    true
                );
                g.strokePath(pathNorm, strokeType);

                if (knob.modulatable) {
                    const auto modDialIsTryingToRemove = knob.modulatorDial.isTryingToRemove();
                    const auto modCol = !modDialIsTryingToRemove ? utils.colours[Utils::Modulation] : utils.colours[Utils::Abort];
                    juce::Path pathMod;
                    const auto modAngle = juce::jlimit(startAngle, endAngle, valueAngle + knob.attenuvertor * angleRange);
                    pathMod.addCentredArc(centre.x, centre.y, radius, radius,
                        0.f, valueAngle, modAngle,
                        true
                    );
                    g.setColour(modCol);
                    g.strokePath(pathMod, strokeType);

                    const auto sumValueAngle = knob.sumValue * angleRange + startAngle;
                    const auto sumValueLine = juce::Line<float>::fromStartAndAngle(centre, radius + 1.f, sumValueAngle);

                    const auto isDestOfSelectedMod = knob.modulatorDial.isSelected();
                    if (!isDestOfSelectedMod) {
                        const auto shortedSumValLine = sumValueLine.withShortenedStart(radius - nelG::Thicc);
                        g.setColour(utils.colours[Utils::Background]);
                        g.drawLine(shortedSumValLine, nelG::Thicc2);
                        g.setColour(modCol);
                        g.drawLine(shortedSumValLine, nelG::Thicc2);
                    }
                    else {
                        const auto shortedSumValLine = sumValueLine.withShortenedStart(radius - nelG::Thicc * 3.f);
                        g.setColour(modCol);
                        g.drawLine(shortedSumValLine, nelG::Thicc2);
                    }
                }

                const auto valueLine = juce::Line<float>::fromStartAndAngle(centre, radius + 1.f, valueAngle);
                const auto tickBGThiccness = nelG::Thicc2 * 2.f;
                g.setColour(utils.colours[Utils::Background]);
                g.drawLine(valueLine, tickBGThiccness);
                g.setColour(utils.colours[Utils::Interactable]);
                g.drawLine(valueLine.withShortenedStart(radius - nelG::Thicc * 3.f), nelG::Thicc2);
            }
            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Dial)
        };

        struct Label :
            public Comp
        {
            Label(Nel19AudioProcessor& p, Utils& u, juce::String&& tooltp, juce::String&& _name) :
                Comp(p, u, tooltp, Utils::Cursor::Hover)
            {
                setName(_name);
            }
        protected:
            void paint(juce::Graphics& g) override {
                g.setColour(utils.colours[Utils::ColourID::Normal]);
                //g.setFont(utils.font);
                g.drawFittedText(getName(), getLocalBounds(), juce::Justification::centred, 1);
            }

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Label)
        };
    public:
        Knob(Nel19AudioProcessor& p, Utils& u, param::ID pID, juce::String&& tooltp, juce::String&& _name,
            juce::Identifier _attachedModID = juce::Identifier(), bool _modulatable = true, bool showLabel = true) :
            Parameter(p, u, std::move(tooltp), std::move(_name), pID, onParamChange(_modulatable), _modulatable),
            dial(p, u, std::move(tooltp), *this),
            label(p, u, std::move(tooltp), std::move(_name)),
            modulatorDial(p, u, *this),
            attachedModID(_attachedModID),
            dragStartValue(0.f),
            scrollSpeed(0.f),
            attenuvertor(0.f),
            sumValue(0.f)
        {
            addAndMakeVisible(dial);
            if(showLabel)
                addAndMakeVisible(label);
            attach.sendInitialUpdate();
            if (modulatable)
                addAndMakeVisible(modulatorDial);
        }
        void updateModSys(const std::shared_ptr<modSys2::Matrix>& matrix) override {
            if (!modulatable || !isVisible()) return;
            auto selectedMod = matrix->getSelectedModulator();
            bool shallRepaint = false;
            if (selectedMod != nullptr) {
                auto dest = selectedMod->getDestination(id);
                bool paramIsDestination = dest != nullptr;
                bool isSelectedChanged = modulatorDial.select(paramIsDestination);
                const auto atten = paramIsDestination ? dest->getValue() : 0.f;
                bool attenChanged = attenuvertor != atten;
                attenuvertor = atten;
                shallRepaint = attenChanged || isSelectedChanged;
            }
            const auto sv = processor.matrix->getParameter(id)->getSumValue();
            bool sumValueChanged = sumValue != sv;
            sumValue = sv;
            if (shallRepaint || sumValueChanged)
                repaint();
        }
    protected:
        Dial dial;
        Label label;
        ModulatorDial modulatorDial;
        juce::Identifier attachedModID;
        float dragStartValue, scrollSpeed, attenuvertor, sumValue;

        void mouseEnter(const juce::MouseEvent& evt) override {
            Comp::mouseEnter(evt);
            updatePopUp(utils);
        }
        void mouseDown(const juce::MouseEvent& evt) override {
            dragStartValue = rap.getValue();
            attach.beginGesture();
            if (attachedModID.isValid())
                processor.matrix->selectModulatorOf(attachedModID);
            Parameter::mouseDown(evt);
        }
        void mouseDrag(const juce::MouseEvent& evt) override {
            const auto height = static_cast<float>(getHeight());
            const auto distY = static_cast<float>(evt.getDistanceFromDragStartY());
            const auto distRatio = distY / height;
            const auto shift = evt.mods.isShiftDown();
            const auto speed = shift ? .1f : .4f;
            const auto newValue = juce::jlimit(0.f, 1.f, dragStartValue - distRatio * speed);
            const auto denormValue = rap.convertFrom0to1(newValue);
            attach.setValueAsPartOfGesture(denormValue);
            updatePopUp(utils);
        }
        void mouseUp(const juce::MouseEvent& evt) override {
            bool mouseClicked = !evt.mouseWasDraggedSinceMouseDown();
            if (mouseClicked) {
                bool revertToDefault = evt.mods.isAltDown();
                float value;
                if (revertToDefault)
                    value = rap.getDefaultValue();
                else {
                    juce::Point<int> centre(getWidth() / 2, getHeight() / 2);
                    const auto angle = centre.getAngleToPoint(evt.getPosition());
                    const auto startAngle = nelG::PiQuart * -3.f;
                    const auto endAngle = nelG::PiQuart * 3.f;
                    const auto angleRange = endAngle - startAngle;
                    value = juce::jlimit(0.f, 1.f, (angle - startAngle) / angleRange);
                }
                const auto denormValue = rap.convertFrom0to1(value);
                attach.setValueAsPartOfGesture(denormValue);
            }
            attach.endGesture();
            Parameter::mouseUp(evt);
        }
        void mouseWheelMove(const juce::MouseEvent& evt, const juce::MouseWheelDetails& wheel) override {
            if (evt.mods.isAnyMouseButtonDown()) return;
            const auto direction = wheel.isReversed ? -1 : 1;
            const auto change = wheel.deltaY * direction;
            const auto usualMouse = !wheel.isSmooth;
            if (usualMouse) {
                const bool sensitive = evt.mods.isShiftDown();
                const auto speed = sensitive ? .002f : .01f;
                const auto nChange = change < 0 ? -speed : speed;
                const auto nValue = juce::jlimit(0.f, 1.f, rap.getValue() + nChange);
                const auto denormValue = rap.convertFrom0to1(nValue);
                attach.setValueAsCompleteGesture(denormValue);
            }
            else { // track pad or touch screen i guess
                const bool released = wheel.isInertial;
                scrollSpeed = released ? scrollSpeed * .5f : change;
                const auto nValue = juce::jlimit(0.f, 1.f, rap.getValue() + scrollSpeed);
                const auto denormValue = rap.convertFrom0to1(nValue);
                attach.setValueAsCompleteGesture(denormValue);
            }
            updatePopUp(utils);
        }

    private:
        void resized() override {
            auto bounds = getLocalBounds().toFloat();
            if (label.isVisible()) {
                const auto x = 0.f;
                const auto y = 0.f;
                const auto w = bounds.getWidth();
                const auto h = bounds.getHeight() * .2f;
                const juce::Rectangle<float> labelBounds(x, y, w, h);
                label.setBounds(labelBounds.toNearestInt());
                bounds.setY(h);
                bounds.setHeight(bounds.getHeight() - h);
            }
            const auto dialBounds = nelG::maxQuadIn(bounds);
            dial.setBounds(dialBounds.toNearestInt());
            if (modulatable) {
                const auto width = static_cast<float>(dialBounds.getWidth());
                const auto height = static_cast<float>(dialBounds.getHeight());
                const auto w = width / nelG::Pi - nelG::Thicc;
                const auto h = height / nelG::Pi - nelG::Thicc;
                const auto x = dialBounds.getX() + (width - w) * .5f;
                const auto y = dialBounds.getY() + (height - h);
                const juce::Rectangle<float> dialArea(x, y, w, h);
                modulatorDial.setBounds(dialArea.toNearestInt());
            }
        }

        std::function<void(float)> onParamChange(bool _modulatable) {
            if (_modulatable)
                return [](float) {};
            return [this](float) { repaint(); };
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Knob)
    };

    struct Switch :
        public Parameter
    {
        Switch(Nel19AudioProcessor& p, Utils& u, juce::String&& tooltp, juce::String&& _name, param::ID pID) :
            Parameter(p, u, std::move(tooltp), std::move(_name), pID, [this](float) { repaint(); }, false),
            onPaint(nullptr)
        {
            attach.sendInitialUpdate();
        }
        std::function<void(juce::Graphics&, const juce::Rectangle<float>&, const Utils&, bool)> onPaint;
    protected:
        void paint(juce::Graphics& g) override {
            const auto value = rap.getValue();
            auto bounds = getLocalBounds().toFloat().reduced(nelG::Thicc);

            g.fillAll(utils.colours[Utils::Background]);
            if (isMouseOver()) {
                g.setColour(utils.colours[Utils::ColourID::HoverButton]);
                if (isMouseButtonDown()) {
                    bounds.reduce(nelG::Thicc, nelG::Thicc);
                    g.fillRoundedRectangle(bounds, nelG::Thicc);
                }
                g.fillRoundedRectangle(bounds, nelG::Thicc);
            }
            if (onPaint != nullptr)
                return onPaint(g, bounds, utils, rap.getValue() > .5f);
            g.setColour(utils.colours[Utils::Interactable]);
            g.drawText(rap.getCurrentValueAsText(), bounds, juce::Justification::centred, false);
        }
        
        void mouseEnter(const juce::MouseEvent& evt) override {
            Comp::mouseEnter(evt);
            updatePopUp(utils);
            repaint();
        }
        void mouseExit(const juce::MouseEvent& evt) override { repaint(); }
        void mouseDown(const juce::MouseEvent& evt) override { repaint(); }
        void mouseUp(const juce::MouseEvent& evt) override {
            if (evt.mouseWasDraggedSinceMouseDown()) return repaint();
            const auto value = 1.f - rap.getValue();
            attach.setValueAsCompleteGesture(value);
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Switch)
    };

    struct WaveformChooser :
        public Parameter
    {
        WaveformChooser(Nel19AudioProcessor& p, Utils& u, juce::String&& tooltp, juce::String&& _name, param::ID pID) :
            Parameter(p, u, std::move(tooltp), std::move(_name), pID, [this](float value) { updateWaveForm(); repaint(); }, false),
            waveForm(),
            waveFormIdx(-1)
        {
            attach.sendInitialUpdate();
        }
    protected:
        std::array<float, 24> waveForm;
        int waveFormIdx;

        void paint(juce::Graphics& g) override {
            g.fillAll(utils.colours[Utils::Background]);
            g.setColour(utils.colours[Utils::Normal]);

            const auto width = static_cast<float>(getWidth());
            const auto height = static_cast<float>(getHeight());
            const auto midY = height * .5f;
            const auto sizeInv = 1.f / static_cast<float>(waveForm.size());
            const auto valWidth = width * sizeInv;
            for (auto i = 0; i < waveForm.size(); ++i) {
                const auto valX = static_cast<float>(i) * sizeInv * width;
                const auto wfHeight = waveForm[i] * height; // [0,1] > [0, height]
                float valY, valHeight;
                if (midY < wfHeight) {
                    valY = midY;
                    valHeight = wfHeight - midY;
                }
                else {
                    valY = wfHeight;
                    valHeight = midY - wfHeight;
                }
                g.fillRect(valX, valY, valWidth, valHeight);
            }
            g.setFont(utils.font);
            g.setColour(utils.colours[Utils::Interactable]);
            g.drawText(rap.getCurrentValueAsText(), getLocalBounds().toFloat(), juce::Justification::centred, false);
        }
        void mouseEnter(const juce::MouseEvent& evt) override {
            Comp::mouseEnter(evt);
            updatePopUp(utils);
            repaint();
        }
        void mouseExit(const juce::MouseEvent& evt) override { repaint(); }
        void mouseDown(const juce::MouseEvent& evt) override { repaint(); }
        void mouseUp(const juce::MouseEvent& evt) override {
            if (evt.mouseWasDraggedSinceMouseDown()) return repaint();
            const auto disWaveForm = rap.convertFrom0to1(rap.getValue());
            float newWaveForm;
            if (evt.mods.isLeftButtonDown())
                newWaveForm = std::fmod(disWaveForm + 1, rap.getNormalisableRange().end + 1.f);
            else {
                newWaveForm = disWaveForm - 1;
                if (newWaveForm < 0)
                    newWaveForm += rap.getNormalisableRange().end + 1;
            }
            attach.setValueAsCompleteGesture(newWaveForm);
        }

        void updateWaveForm() {
            const auto disWaveForm = rap.convertFrom0to1(rap.getValue());
            const auto tableIdx = static_cast<int>(disWaveForm);
            updateWaveForm(tableIdx);
        }
        void updateWaveForm(const int tableIdx) {
            const auto sizeInv = 1.f / static_cast<float>(waveForm.size());
            const auto& wt = processor.matrix->getWaveTables();
            for (auto i = 0; i < waveForm.size(); ++i) {
                const auto phase = static_cast<float>(i) * sizeInv;
                waveForm[i] = 1.f - wt(phase, tableIdx);
            }
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformChooser)
    };

    struct ModDragger :
        public Comp
    {
        ModDragger(Nel19AudioProcessor& p, Utils& u, juce::String&& tooltp,
            const juce::String&& mID, std::vector<Parameter*>& modulatableParameters) :
            Comp(p, u, std::move(tooltp), Utils::Cursor::Hover),
            id(mID),
            draggerfall(),
            bounds(0, 0, 0, 0),
            modulatables(modulatableParameters),
            hoveredParameter(nullptr),
            selected(false)
        {
            setBufferedToImage(true);
        }
        void timerCallback(const std::shared_ptr<modSys2::Matrix>& matrix) {
            const auto mod = matrix->getModulator(id);
            const auto selection = matrix->getSelectedModulator();
            auto s = mod == selection;
            if (selected != s) {
                selected = s;
                repaint();
            }
        }
        void setQBounds(juce::Rectangle<int> b) {
            bounds = b;
            setBounds(b);
        }
    protected:
        juce::Identifier id;
        juce::ComponentDragger draggerfall;
        juce::Rectangle<int> bounds;
        std::vector<Parameter*>& modulatables;
        Parameter* hoveredParameter;
        bool selected;

        void mouseDown(const juce::MouseEvent& evt) override {
            auto matrix = processor.matrix.getUpdatedPtr();
            matrix->selectModulator(id);
            selected = true;
            draggerfall.startDraggingComponent(this, evt);
        }
        void mouseDrag(const juce::MouseEvent& evt) override {
            draggerfall.dragComponent(this, evt, nullptr);
            hoveredParameter = getHoveredParameter();
            repaint();
        }
        void mouseUp(const juce::MouseEvent&) override {
            if (hoveredParameter != nullptr) {
                auto matrix = processor.matrix.getCopyOfUpdatedPtr();
                const auto m = matrix->getModulator(id);
                const auto p = matrix->getParameter(hoveredParameter->id);
                auto* rap = processor.apvts.getParameter(p->id);
                const auto pValue = rap->getValue();
                const auto atten = 1.f - pValue;
                matrix->addDestination(m->id, p->id, atten, false);
                processor.matrix.replaceUpdatedPtrWith(matrix);
                hoveredParameter = nullptr;
            }
            setBounds(bounds);
            repaint();
        }
        void paint(juce::Graphics& g) override {
            if (hoveredParameter == nullptr)
                if (selected)
                    g.setColour(utils.colours[Utils::Modulation]);
                else
                    g.setColour(utils.colours[Utils::Inactive]);
            else
                g.setColour(utils.colours[Utils::Interactable]);

            const auto tBounds = getLocalBounds().toFloat();
            g.drawRoundedRectangle(tBounds, 2, nelG::Thicc);
            juce::Point<float> centre(tBounds.getX() + tBounds.getWidth() * .5f, tBounds.getY() + tBounds.getHeight() * .5f);
            const auto arrowHead = tBounds.getWidth() * .25f;
            g.drawArrow(juce::Line<float>(centre, { centre.x, tBounds.getBottom() }), nelG::Thicc, arrowHead, arrowHead);
            g.drawArrow(juce::Line<float>(centre, { tBounds.getX(), centre.y }), nelG::Thicc, arrowHead, arrowHead);
            g.drawArrow(juce::Line<float>(centre, { centre.x, tBounds.getY() }), nelG::Thicc, arrowHead, arrowHead);
            g.drawArrow(juce::Line<float>(centre, { tBounds.getRight(), centre.y }), nelG::Thicc, arrowHead, arrowHead);
        }

        Parameter* getHoveredParameter() const {
            for (const auto p : modulatables)
                if (p->isVisible() && !getBounds().getIntersection(p->getBounds()).isEmpty())
                    return p;
            return nullptr;
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModDragger)
    };
}

/*

waveform chooser
    display wavetable playhead
    interpolatable tables?

knobs
    switch between norm and sensitive drag in the middle of drag

*/