#pragma once

static void paintKnob(juce::Graphics& g, const Utils& utils, juce::RangedAudioParameter& rap, juce::Rectangle<int> bounds,
    float* attenuvertor = nullptr, float sumValue = 0.f, bool modDialIsTryingToRemove = false, bool isSelected = false) {
    const auto width = static_cast<float>(bounds.getWidth());
    const auto height = static_cast<float>(bounds.getHeight());
    const auto value = rap.getValue();
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

    bool knobIsModulatable = attenuvertor != nullptr;
    if (knobIsModulatable) {
        const auto modCol = !modDialIsTryingToRemove ? utils.colours[Utils::Modulation] : utils.colours[Utils::Abort];
        juce::Path pathMod;
        const auto modAngle = juce::jlimit(startAngle, endAngle, valueAngle + *attenuvertor * angleRange);
        pathMod.addCentredArc(centre.x, centre.y, radius, radius,
            0.f, valueAngle, modAngle,
            true
        );
        g.setColour(modCol);
        g.strokePath(pathMod, strokeType);

        const auto sumValueAngle = sumValue * angleRange + startAngle;
        const auto sumValueLine = juce::Line<float>::fromStartAndAngle(centre, radius + 1.f, sumValueAngle);

        if (!isSelected) {
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

struct Parameter {
    Parameter(Nel19AudioProcessor& p, param::ID pID, Comp& comp,
        std::function<void(float)> attachLambda = nullptr) :
        rap(*p.apvts.getParameter(param::getID(pID))),
        attach(rap, attachLambda == nullptr ? [&c = comp](float) { c.repaint(); } : attachLambda, nullptr),
        itself(comp)
    {
        if(attachLambda == nullptr)
            attach.sendInitialUpdate();
    }
    void setActive(bool e) { itself.setVisible(e); }
    bool isActive() const noexcept { return itself.isVisible(); }
    void setValueNormalized(const float v) {
        attach.setValueAsCompleteGesture(rap.convertFrom0to1(v));
    }
    void setValueDenormalized(const float v) {
        attach.setValueAsCompleteGesture(v);
    }
    const float getDenormalized() const noexcept { return rap.convertFrom0to1(rap.getValue()); }
    const float getNormalized() const noexcept { return rap.getValue(); }
    juce::RangedAudioParameter& rap;
    juce::ParameterAttachment attach;
protected:
    Comp& itself;
};

struct Knob :
    public Comp,
    public Parameter
{
    Knob(Nel19AudioProcessor& p, Utils& u, const juce::String& tooltp, param::ID pID, juce::Identifier mID = nullptr, bool withRadialHittest = false) :
        Comp(p, u, tooltp, Utils::Cursor::Hover),
        Parameter(p, pID, *this),
        modID(mID),
        dragStartValue(0.f), scrollSpeed(0.f),
        radialHitTest(withRadialHittest)
    {
    }
protected:
    juce::Identifier modID;
    float dragStartValue, scrollSpeed;
    bool radialHitTest;

    void updatePopUp() {
        utils.updatePopUp(rap.name + "\n" + rap.getCurrentValueAsText());
    }

    void paint(juce::Graphics& g) override {
        paintKnob(g, utils, rap, getLocalBounds());
    }
    
    bool hitTest(int x, int y) override {
        if (!radialHitTest) return true;
        juce::Point<float> midPoint(static_cast<float>(getWidth()) * .5f, static_cast<float>(getHeight()) * .5f);
        const auto circleRad = std::min(midPoint.x, midPoint.y) - nelG::Thicc;
        juce::Point<float> mousePos(static_cast<float>(x), static_cast<float>(y));
        juce::Line<float> rad(midPoint, mousePos);
        return rad.getLength() < circleRad;
    }

    void mouseEnter(const juce::MouseEvent& evt) override {
        Comp::mouseEnter(evt);
        updatePopUp();
    }
    void mouseDown(const juce::MouseEvent& evt) override {
        dragStartValue = rap.getValue();
        attach.beginGesture();
        if(modID.isValid())
            processor.matrix->selectModulatorOf(modID);
    }
    void mouseDrag(const juce::MouseEvent& evt) override {
        const auto height = static_cast<float>(getHeight());
        const auto distY = static_cast<float>(evt.getDistanceFromDragStartY());
        const auto distRatio = distY / height;
        const auto shift = evt.mods.isShiftDown();
        const auto speed = shift ? .1f : .4f;
        const auto newValue = juce::jlimit(0.f, 1.f, dragStartValue - distRatio * speed);
        const auto denormValue = rap.convertFrom0to1(newValue);
        updatePopUp();
        attach.setValueAsPartOfGesture(denormValue);
        repaint();
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
        repaint();
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
        updatePopUp();
        repaint();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Knob)
};

struct Modulatable :
    public Comp,
    public modSys2::Identifiable,
    public Parameter
{
    Modulatable(Nel19AudioProcessor& p, Utils& u, const juce::String& tooltp, param::ID pID) :
        Comp(p, u, tooltp, Utils::Cursor::Hover),
        Parameter(p, pID, *this),
        modSys2::Identifiable(param::getID(pID))
    {

    }
    virtual void updateModSys(const std::shared_ptr<modSys2::Matrix>& matrix) = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Modulatable)
};

struct Switch :
    public Comp,
    public Parameter
{
    Switch(Nel19AudioProcessor& p, Utils& u, const juce::String& tooltp, param::ID pID) :
        Comp(p, u, tooltp, Utils::Cursor::Hover),
        Parameter(p, pID, *this)
    {
    }
    virtual void updateModSys(const std::shared_ptr<modSys2::Matrix>& matrix) {
        // not needed atm
    }
protected:

    void updatePopUp() {
        utils.updatePopUp(rap.name + "\n" + rap.getCurrentValueAsText());
    }

    void paint(juce::Graphics& g) override {
        const auto value = rap.getValue();
        bool interacting = isMouseButtonDown();
        auto bounds = getLocalBounds().toFloat().reduced(nelG::Thicc);

        g.fillAll(utils.colours[Utils::Background]);
        if (interacting) {
            bounds = bounds.reduced(nelG::Thicc);
            g.setColour(juce::Colour(0x77ffffff));
            g.fillRoundedRectangle(bounds, nelG::Thicc);
        }
        else if (isMouseOver()) { // hovering
            g.setColour(juce::Colour(0x44ffffff));
            g.fillRoundedRectangle(bounds, nelG::Thicc);
        }
        g.setColour(utils.colours[Utils::Interactable]);
        g.drawText(rap.getCurrentValueAsText(), bounds, juce::Justification::centred, false);
    }
    void mouseEnter(const juce::MouseEvent& evt) override {
        updatePopUp();
        repaint();
    }
    void mouseExit(const juce::MouseEvent& evt) override {
        repaint();
    }
    void mouseMove(const juce::MouseEvent& evt) override {
        Comp::mouseMove(evt);
    }
    void mouseDown(const juce::MouseEvent& evt) override {
        repaint();
    }
    void mouseUp(const juce::MouseEvent& evt) override {
        if (evt.mouseWasDraggedSinceMouseDown()) return repaint();
        const auto value = 1.f - rap.getValue();
        attach.setValueAsCompleteGesture(value);
        repaint();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Switch)
};

struct WaveformChooser :
    public modSys2::Identifiable,
    public Comp,
    public Parameter
{
    WaveformChooser(Nel19AudioProcessor& p, Utils& u, const juce::String& tooltp, param::ID pID) :
        modSys2::Identifiable(param::getID(pID)),
        Comp(p, u, tooltp, Utils::Cursor::Hover),
        Parameter(p, pID, *this, [this](float value) { updateWaveForm(); repaint(); }),
        waveForm(),
        waveFormIdx(-1)
    {
        attach.sendInitialUpdate();
    }
    virtual void updateModSys(const std::shared_ptr<modSys2::Matrix>& matrix) {}
protected:
    std::array<float, 24> waveForm;
    int waveFormIdx;

    void updatePopUp() {
        utils.updatePopUp(rap.name + "\n" + rap.getCurrentValueAsText());
    }

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
        updatePopUp();
        repaint();
    }
    void mouseExit(const juce::MouseEvent& evt) override {
        repaint();
    }
    void mouseDown(const juce::MouseEvent& evt) override {
        repaint();
    }
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

struct ModulatableKnob :
    public Modulatable
{
    struct ModulatorDial :
        public Comp
    {
        ModulatorDial(Nel19AudioProcessor& p, Utils& u, ModulatableKnob& par) :
            Comp(p, u, "modulation attenuvertor", Utils::Cursor::Hover),
            param(par),
            startAttenuvertor(0.f),
            selected(false),
            tryRemove(false)
        {
        }
        inline bool select(bool e) noexcept {
            if (selected != e) {
                selected = e;
                return true;
            }
            return false;
        }
        const inline bool isSelected() const noexcept { return selected; }
        const inline bool isTryingToRemove() const noexcept { return tryRemove; }
    protected:
        ModulatableKnob& param;
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

    ModulatableKnob(Nel19AudioProcessor& p, Utils& u, const juce::String& tooltp, param::ID pID, bool withRadialHittest = false) :
        Modulatable(p, u, tooltp, pID),
        modulatorDial(p, u, *this),
        attenuvertor(0.f), sumValue(0.f), dragStartValue(0.f), scrollSpeed(0.f),
        radialHitTest(withRadialHittest)
    {
        addAndMakeVisible(modulatorDial);
    }
    void updateModSys(const std::shared_ptr<modSys2::Matrix>& matrix) override {
        if (!isVisible()) return;
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
    ModulatorDial modulatorDial;
    float attenuvertor, sumValue, dragStartValue, scrollSpeed;
    bool radialHitTest;

    void resized() override {
        const auto width = static_cast<float>(getWidth());
        const auto height = static_cast<float>(getHeight());
        const auto dialWidth = width / nelG::Pi - nelG::Thicc;
        const auto dialHeight = height / nelG::Pi - nelG::Thicc;
        const auto dialX = (width - dialWidth) * .5f;
        const auto dialY = (height - dialHeight);
        juce::Rectangle<float> dialArea(dialX, dialY, dialWidth, dialHeight);
        modulatorDial.setBounds(dialArea.toNearestInt());
    }
    
    void paint(juce::Graphics& g) override {
        paintKnob(g, utils, rap, getLocalBounds(), &attenuvertor, sumValue, modulatorDial.isTryingToRemove(), modulatorDial.isSelected());
    }
    
    bool hitTest(int x, int y) override {
        if (!radialHitTest) return true;
        juce::Point<float> midPoint(static_cast<float>(getWidth()) * .5f, static_cast<float>(getHeight()) * .5f);
        const auto circleRad = std::min(midPoint.x, midPoint.y) - nelG::Thicc;
        juce::Point<float> mousePos(static_cast<float>(x), static_cast<float>(y));
        juce::Line<float> rad(midPoint, mousePos);
        return rad.getLength() < circleRad;
    }

    void mouseEnter(const juce::MouseEvent& evt) override {
        Comp::mouseEnter(evt);
        updatePopUp();
    }
    void mouseDown(const juce::MouseEvent& evt) override {
        dragStartValue = rap.getValue();
        attach.beginGesture();
        //processor.matrix->selectModulatorOf(id);
    }
    void mouseDrag(const juce::MouseEvent& evt) override {
        const auto height = static_cast<float>(getHeight());
        const auto distY = static_cast<float>(evt.getDistanceFromDragStartY());
        const auto distRatio = distY / height;
        const auto shift = evt.mods.isShiftDown();
        const auto speed = shift ? .1f : .4f;
        const auto newValue = juce::jlimit(0.f, 1.f, dragStartValue - distRatio * speed);
        const auto denormValue = rap.convertFrom0to1(newValue);
        updatePopUp();
        attach.setValueAsPartOfGesture(denormValue);
        repaint();
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
        repaint();
    }
    void mouseWheelMove(const juce::MouseEvent& evt, const juce::MouseWheelDetails& wheel) override {
        if (evt.mods.isAnyMouseButtonDown()) return;
        const auto direction = wheel.isReversed ? -1 : 1;
        const auto change = wheel.deltaY * direction;
        const auto usualMouse = !wheel.isSmooth;
        const auto minInterval = rap.getNormalisableRange().interval / rap.getNormalisableRange().getRange().getLength();
        if (usualMouse) {
            const bool sensitive = evt.mods.isShiftDown();
            const auto speed = sensitive ? .002f : .01f;
            const auto speedStepped = speed < minInterval ?  minInterval : speed;
            const auto nChange = change < 0 ? -speedStepped : speedStepped;
            const auto nValue = juce::jlimit(0.f, 1.f, rap.getValue() + nChange);
            const auto denormValue = rap.convertFrom0to1(nValue);
            attach.setValueAsCompleteGesture(denormValue);
        }
        else { // track pad or touch screen i guess
            const bool released = wheel.isInertial;
            scrollSpeed = released ? scrollSpeed * .5f : change;
            const auto speedStepped = scrollSpeed < minInterval ? minInterval : scrollSpeed;
            const auto nValue = juce::jlimit(0.f, 1.f, rap.getValue() + speedStepped);
            const auto denormValue = rap.convertFrom0to1(nValue);
            attach.setValueAsCompleteGesture(denormValue);
        }
        updatePopUp();
        repaint();
    }

    void updatePopUp() {
        utils.updatePopUp(rap.name + "\n" + rap.getCurrentValueAsText());
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatableKnob)
};

struct ModDragger :
    public Comp
{
    ModDragger(Nel19AudioProcessor& p, Utils& u, const juce::String& tooltp, const juce::String&& mID, std::vector<Modulatable*>& modulatableParameters) :
        Comp(p, u, tooltp, Utils::Cursor::Hover),
        id(mID),
        draggerfall(),
        bounds(0, 0, 0, 0),
        modulatables(modulatableParameters),
        hoveredParameter(nullptr),
        selected(false)
    {}
    void setQBounds(juce::Rectangle<float> b) {
        bounds = nelG::maxQuadIn(b).toNearestInt();
        setBounds(bounds);
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
protected:
    juce::Identifier id;
    juce::ComponentDragger draggerfall;
    juce::Rectangle<int> bounds;
    std::vector<Modulatable*>& modulatables;
    Modulatable* hoveredParameter;
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

    Modulatable* getHoveredParameter() const {
        for (const auto p : modulatables)
            if (p->isVisible() && !getBounds().getIntersection(p->getBounds()).isEmpty())
                return p;
        return nullptr;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModDragger)
};

/*

waveform chooser
    display wavetable playhead
    interpolatable tables?

knobs
    switch between norm and sensitive drag in the middle of drag

*/