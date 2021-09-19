#pragma once
#include <array>

class QuickAccessMenu :
    public Comp
{
    struct Option :
        public Comp
    {
        Option(Nel19AudioProcessor& p, Utils& u, std::function<void(juce::Graphics&, Comp*, bool)> _onPaint) :
            Comp(p, u, "", Utils::Cursor::Hover),
            onPaint(_onPaint),
            hovering(false)
        {}
        void update(juce::Point<int> dragPos) {
            hovering = getScreenBounds().contains(dragPos);
            repaint();
        }
    protected:
        std::function<void(juce::Graphics&, Comp*, bool)> onPaint;
        bool hovering;

        void paint(juce::Graphics& g) override { if (onPaint) onPaint(g, this, hovering); }
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Option)
    };
public:
    QuickAccessMenu(Nel19AudioProcessor& p, Utils& u, juce::String&& tooltp,
        std::function<void(int)> _onSelect,
        std::function<void(juce::Graphics&, Comp*)> _onPaint,
        const size_t _numOptions) :
        Comp(p, u, tooltp, Utils::Cursor::Hover),
        onSelect(_onSelect),
        onPaint(_onPaint),
        options(),
        numOptions(_numOptions)
    {
    }
    void init(const std::vector<std::function<void(juce::Graphics&, Comp*, bool)>>& _onPaint)
    {
        options.reserve(numOptions);
        for (auto o = 0; o < numOptions; ++o) {
            options.emplace_back(std::make_unique<Option>(processor, utils, _onPaint[o]));
            getTopLevelComponent()->addChildComponent(options[o].get());
        }
    }

    std::function<void(int)> onSelect;
    std::function<void(juce::Graphics&, Comp*)> onPaint;
protected:
    std::vector<std::unique_ptr<Option>> options;
    const size_t numOptions;

    void paint(juce::Graphics& g) override { onPaint(g, this); }

    void mouseEnter(const juce::MouseEvent& evt) override { Comp::mouseEnter(evt); repaint(); }
    void mouseExit(const juce::MouseEvent& evt) override { repaint(); }
    void mouseDown(const juce::MouseEvent& evt) override { setVisibleOptions(true); }
    void mouseDrag(const juce::MouseEvent& evt) override {
        const auto pos = evt.getScreenPosition();
        for (auto o = 0; o < numOptions; ++o)
            options[o]->update(pos);
    }
    void mouseUp(const juce::MouseEvent& evt) override {
        replaceModulator(evt.getScreenPosition());
        setVisibleOptions(false);
    }

    void setVisibleOptions(bool visible) {
        if (visible) {
            const auto top = getTopLevelComponent();
            const auto bounds = top->getLocalBounds().toFloat().reduced(nelG::Thicc2);
            auto x = bounds.getX();
            const auto y = bounds.getY();
            const auto width = bounds.getWidth() / static_cast<float>(numOptions);
            const auto height = bounds.getHeight();
            for (auto o = 0; o < numOptions; ++o, x += width) {
                const juce::Rectangle<float> oBounds(x, y, width, height);
                options[o]->setBounds(nelG::maxQuadIn(oBounds).toNearestInt());
                options[o]->setVisible(true);
            }
            return repaint();
        }
        for (auto o = 0; o < numOptions; ++o)
            options[o]->setVisible(false);
        repaint();
    }

    void replaceModulator(juce::Point<int> pos) {
        for (auto o = 0; o < numOptions; ++o)
            if (options[o]->getScreenBounds().contains(pos))
                return onSelect(o);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QuickAccessMenu)
};


class ModulatorComp :
    public Comp,
    public modSys2::Identifiable
{
    enum Mods{ EnvFol, LFO, Rand, Perlin, NumMods };

    struct Label :
        public Comp
    {
        Label(Nel19AudioProcessor& p, Utils& u, juce::String&& _name) :
            Comp(p, u)
        {
            setName(_name);
            setBufferedToImage(true);
        }
        void paint(juce::Graphics& g) override {
            g.setFont(utils.font);
            g.setColour(utils.colours[Utils::ColourID::Modulation]);
            g.drawFittedText(getName(), getLocalBounds(), juce::Justification::centredRight, 1);
        }
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Label)
    };
public:
    static std::function<void(juce::Graphics&, Comp*, bool)> paintModOption(const Utils& u, const juce::String&& modName) {
        return [&utils = u, name = modName](juce::Graphics& g, Comp* comp, bool hover) {
            const auto bounds = comp->getLocalBounds().toFloat().reduced(nelG::Thicc);
            g.setColour(utils.colours[Utils::ColourID::Background]);
            g.fillRoundedRectangle(bounds, nelG::Thicc);
            if (hover) {
                g.setColour(utils.colours[Utils::ColourID::HoverButton]);
                g.fillRoundedRectangle(bounds, nelG::Thicc);
                g.setColour(utils.colours[Utils::ColourID::Interactable]);
            }
            else
                g.setColour(utils.colours[Utils::ColourID::Normal]);
            g.drawRoundedRectangle(bounds, nelG::Thicc, nelG::Thicc);
            g.setFont(utils.font);
            g.drawFittedText(name, comp->getLocalBounds(), juce::Justification::centred, 1);
        };
    }

    ModulatorComp(Nel19AudioProcessor& p, Utils& u, const juce::Identifier& mID, std::vector<pComp::Parameter*>& _modulatables, int modulatorsIdx,
        std::function<void(int)> onModReplace, juce::String&& _name) :
        Comp(p, u),
        modSys2::Identifiable(mID),
        modulatables(_modulatables),
        randButton(processor, utils),
        replaceModButton(p, u, "drag to replace this modulator.", onModReplace, paintReplaceModButton(), NumMods),
        label(p, u, std::move(_name)),
        modsIdx(modulatorsIdx)
    {
        addAndMakeVisible(randButton);
        addAndMakeVisible(replaceModButton);
        addAndMakeVisible(label);
    }
    void init(const std::vector<std::function<void(juce::Graphics&, Comp*, bool)>>& modPaints) {
        replaceModButton.init(modPaints);
    }

    virtual void initModulatables() {}
protected:
    std::vector<pComp::Parameter*>& modulatables;
    RandomizerButton randButton;
    QuickAccessMenu replaceModButton;
    Label label;
    int modsIdx;

    void resized() override {
        const auto width = static_cast<float>(getWidth());
        const auto height = static_cast<float>(getHeight());
        auto rbX = width * .85f;
        const auto rbY = height * .05f;
        const auto margin = .2f;
        const auto rbWidth = std::min(width, height) * margin;
        randButton.setBounds(juce::Rectangle<float>(rbX, rbY, rbWidth, rbWidth).toNearestInt());
        rbX -= rbWidth * (margin + 1.f);
        replaceModButton.setBounds(juce::Rectangle<float>(rbX, rbY, rbWidth, rbWidth).toNearestInt());
        {
            const auto w = rbX - rbWidth * margin;
            const auto x = 0.f;
            const auto y = rbY;
            const auto h = rbWidth;
            label.setBounds(juce::Rectangle<float>(x,y,w,h).toNearestInt());
        }
        
    }
private:
    std::function<void(juce::Graphics& g, Comp* comp)> paintReplaceModButton() {
        return [this](juce::Graphics& g, Comp* comp) {
            const auto bounds = comp->getLocalBounds().toFloat().reduced(nelG::Thicc);
            g.setColour(utils.colours[Utils::ColourID::Background]);
            g.fillRoundedRectangle(bounds, nelG::Thicc);
            if (comp->isMouseOver()) {
                g.setColour(utils.colours[Utils::ColourID::HoverButton]);
                g.fillRoundedRectangle(bounds, nelG::Thicc);
                g.setColour(utils.colours[Utils::ColourID::Interactable]);
            }
            else
                g.setColour(utils.colours[Utils::ColourID::Normal]);
            g.drawRoundedRectangle(bounds, nelG::Thicc, nelG::Thicc);
            g.setFont(utils.font);
            g.drawFittedText("<<", comp->getLocalBounds(), juce::Justification::centred, 1);
        };
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatorComp)
};

struct ModulatorPerlinComp :
    public ModulatorComp
{
    ModulatorPerlinComp(Nel19AudioProcessor& p, Utils& u, const juce::Identifier& mID, std::vector<pComp::Parameter*>& mods, int modulatorsIdx,
        std::function<void(int)> onModReplace) :
        ModulatorComp(p, u, mID, mods, modulatorsIdx, onModReplace, "Perlin Noise"),
        layout(
            { 30, 70, 130, 70, 30 },
            { 70, 90, 30 }
        ),
        rateP(processor, utils, modsIdx == 0 ? param::ID::PerlinRate0 : param::ID::PerlinRate1, "The rate at which new values are picked.", "Rate"),
        octavesP(processor, utils, modsIdx == 0 ? param::ID::PerlinOctaves0 : param::ID::PerlinOctaves1, "More Octaves increase the complexity of the modulation.", "Octaves"),
        widthP(processor, utils, modsIdx == 0 ? param::ID::PerlinWidth0 : param::ID::PerlinWidth1, "Increases the Stereo-Width of the modulation.", "Width")
    {
        this->modulatables.push_back(&rateP);
        this->modulatables.push_back(&octavesP);
        this->modulatables.push_back(&widthP);
        this->randButton.addRandomizable(&rateP);
        this->randButton.addRandomizable(&octavesP);
        this->randButton.addRandomizable(&widthP);
    }
    void setVisible(bool e) override {
        rateP.setVisible(e);
        octavesP.setVisible(e);
        widthP.setVisible(e);
        Comp::setVisible(e);
    }
    void initModulatables() override {
        auto top = getTopLevelComponent();
        top->addChildComponent(rateP);
        top->addChildComponent(octavesP);
        top->addChildComponent(widthP);
    }
protected:
    nelG::Layout layout;
    pComp::Knob rateP, octavesP, widthP;

    void paint(juce::Graphics& g) override {
        //layout.paintGrid(g);
        g.fillAll(utils.colours[Utils::Background]);
        //outtake::drawRandGrid(g, getLocalBounds(), 32, 16, juce::Colour(nelG::ColDarkGrey), .1f);
    }
    void resized() override {
        layout.setBounds(getBoundsInParent().toFloat());
        layout.place(rateP, 1, 1, 1, 1);
        layout.place(octavesP, 2, 1, 1, 1);
        layout.place(widthP, 3, 1, 1, 1);
        ModulatorComp::resized();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatorPerlinComp)
};

struct ModulatorLFOComp :
    public ModulatorComp
{
    ModulatorLFOComp(Nel19AudioProcessor& p, Utils& u, const juce::Identifier& mID, std::vector<pComp::Parameter*>& mods, int modulatorsIdx,
        std::function<void(int)> onModReplace) :
        ModulatorComp(p, u, mID, mods, modulatorsIdx, onModReplace, "LFO"),
        layout(
            { 30, 50, 90, 50, 50, 30 },
            { 50, 60, 30 }
        ),
        rateP(processor, utils, modsIdx == 0 ? param::ID::LFORate0 : param::ID::LFORate1, "The rate at which new values are picked.", "Rate"),
        widthP(processor, utils, modsIdx == 0 ? param::ID::LFOWidth0 : param::ID::LFOWidth1, "Increases the Stereo-Width of the modulation.", "Width"),
        phaseP(processor, utils, modsIdx == 0 ? param::ID::LFOPhase0 : param::ID::LFOPhase1, "Defines the phase of the waveform.", "Phase"),
        waveformP(processor, utils, "Change the waveform of this modulator.", "Waveform", modsIdx == 0 ? param::ID::LFOWaveTable0 : param::ID::LFOWaveTable1),
        tempoSyncP(processor, utils, "Switch between free-running or tempo-sync modulation.", "Tempo-Sync", modsIdx == 0 ? param::ID::LFOSync0 : param::ID::LFOSync1),
        polarityP(processor, utils, "Flip the polarity of this modulator.", "Polarity", modsIdx == 0 ? param::ID::LFOPolarity0 : param::ID::LFOPolarity1)
    {
        this->modulatables.push_back(&rateP);
        this->modulatables.push_back(&widthP);
        this->modulatables.push_back(&phaseP);
        this->randButton.addRandomizable(&rateP);
        this->randButton.addRandomizable(&widthP);
        this->randButton.addRandomizable(&waveformP);
        this->randButton.addRandomizable(&tempoSyncP);
        this->randButton.addRandomizable(&polarityP);
        this->randButton.addRandomizable(&phaseP);
    }
    void setVisible(bool e) override {
        Comp::setVisible(e);
        rateP.setVisible(e);
        widthP.setVisible(e);
        waveformP.setVisible(e);
        tempoSyncP.setVisible(e);
        polarityP.setVisible(e);
        phaseP.setVisible(e);
    }
    void initModulatables() override {
        auto top = getTopLevelComponent();
        top->addChildComponent(tempoSyncP);
        top->addChildComponent(rateP);
        top->addChildComponent(waveformP);
        top->addChildComponent(widthP);
        top->addChildComponent(polarityP);
        top->addChildComponent(phaseP);
    }
protected:
    nelG::Layout layout;
    pComp::Knob rateP, widthP, phaseP;
    pComp::WaveformChooser waveformP;
    pComp::Switch tempoSyncP, polarityP;

    void paint(juce::Graphics& g) override {
        //layout.paintGrid(g);
        g.fillAll(utils.colours[Utils::Background]);
    }
    void resized() override {
        layout.setBounds(getBoundsInParent().toFloat());
        layout.place(tempoSyncP, 1, 2, 1, 1, true);
        layout.place(rateP, 1, 1, 1, 1);
        layout.place(waveformP, 2, 1, 1, 1, true);
        layout.place(widthP, 3, 1, 1, 1);
        layout.place(phaseP, 4, 1, 1, 1);
        layout.place(polarityP, 4, 2, 1, 1, true);
        ModulatorComp::resized();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatorLFOComp)
};

struct ModulatorEnvelopeFollowerComp :
    public ModulatorComp
{
    ModulatorEnvelopeFollowerComp(Nel19AudioProcessor& p, Utils& u, const juce::Identifier& mID, std::vector<pComp::Parameter*>& mods, int modulatorsIdx,
        std::function<void(int)> onModReplace) :
        ModulatorComp(p, u, mID, mods, modulatorsIdx, onModReplace, "Envelope Follower"),
        layout(
            { 30, 40, 80, 80, 50, 50, 30 },
            { 50, 60, 30 }
        ),
        atkP(processor, utils, modsIdx == 0 ? param::ID::EnvFolAtk0 : param::ID::EnvFolAtk1, "Set how fast the modulator reacts to transients.", "Attack"),
        rlsP(processor, utils, modsIdx == 0 ? param::ID::EnvFolRls0 : param::ID::EnvFolRls1, "Set how long the modulator needs to release.", "Release"),
        gainP(processor, utils, modsIdx == 0 ? param::ID::EnvFolGain0 : param::ID::EnvFolGain1, "Define the modulator's input gain.", "Gain"),
        biasP(processor, utils, modsIdx == 0 ? param::ID::EnvFolBias0 : param::ID::EnvFolBias1, "Changes the character of the modulation.", "Bias"),
        widthP(processor, utils, modsIdx == 0 ? param::ID::EnvFolWidth0 : param::ID::EnvFolWidth1, "Changes the Stereo-Width of the modulator.", "Width")
    {
        this->modulatables.push_back(&atkP);
        this->modulatables.push_back(&rlsP);
        this->modulatables.push_back(&gainP);
        this->modulatables.push_back(&biasP);
        this->modulatables.push_back(&widthP);

        this->randButton.addRandomizable(&atkP);
        this->randButton.addRandomizable(&rlsP);
        this->randButton.addRandomizable(&gainP);
        this->randButton.addRandomizable(&biasP);
        this->randButton.addRandomizable(&widthP);
    }
    void setVisible(bool e) override {
        Comp::setVisible(e);
        atkP.setVisible(e);
        rlsP.setVisible(e);
        gainP.setVisible(e);
        biasP.setVisible(e);
        widthP.setVisible(e);
    }
    void initModulatables() override {
        auto top = getTopLevelComponent();
        top->addChildComponent(atkP);
        top->addChildComponent(rlsP);
        top->addChildComponent(gainP);
        top->addChildComponent(biasP);
        top->addChildComponent(widthP);
    }
protected:
    nelG::Layout layout;
    pComp::Knob atkP, rlsP, gainP, biasP, widthP;

    void paint(juce::Graphics& g) override {
        //layout.paintGrid(g);
        g.fillAll(utils.colours[Utils::Background]);
    }
    void resized() override {
        layout.setBounds(getBoundsInParent().toFloat());
        layout.place(gainP, 1, 1, 1, 1);
        layout.place(atkP, 2, 1, 1, 1);
        layout.place(rlsP, 3, 1, 1, 1);
        layout.place(biasP, 4, 1, 1, 1);
        layout.place(widthP, 5, 1, 1, 1);
        ModulatorComp::resized();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatorEnvelopeFollowerComp)
};

struct ModulatorRandComp :
    public ModulatorComp
{
    ModulatorRandComp(Nel19AudioProcessor& p, Utils& u, const juce::Identifier& mID, std::vector<pComp::Parameter*>& mods, int modulatorsIdx,
        std::function<void(int)> onModReplace) :
        ModulatorComp(p, u, mID, mods, modulatorsIdx, onModReplace, "Classic Random"),
        layout(
            { 30, 70, 70, 70, 70, 30 },
            { 70, 90, 30 }
        ),
        rateP(processor, utils, modsIdx == 0 ? param::ID::RandRate0 : param::ID::RandRate1, "The rate at which new values are picked.", "Rate"),
        biasP(processor, utils, modsIdx == 0 ? param::ID::RandBias0 : param::ID::RandBias1, "The bias of the values picked.", "Bias"),
        widthP(processor, utils, modsIdx == 0 ? param::ID::RandWidth0 : param::ID::RandWidth1, "Increases the Stereo-Width of the modulation.", "Width"),
        smoothP(processor, utils, modsIdx == 0 ? param::ID::RandSmooth0 : param::ID::RandSmooth1, "Increases the smoothness of the modulation.", "Smooth"),
        syncP(processor, utils, "Switches between free or tempo-sync modulation.", "Tempo-Sync", modsIdx == 0 ? param::ID::RandSync0 : param::ID::RandSync1)
    {
        this->modulatables.push_back(&rateP);
        this->modulatables.push_back(&biasP);
        this->modulatables.push_back(&widthP);
        this->modulatables.push_back(&smoothP);
        this->randButton.addRandomizable(&rateP);
        this->randButton.addRandomizable(&biasP);
        this->randButton.addRandomizable(&widthP);
        this->randButton.addRandomizable(&smoothP);
        this->randButton.addRandomizable(&syncP);
    }
    void setVisible(bool e) override {
        rateP.setVisible(e);
        biasP.setVisible(e);
        widthP.setVisible(e);
        smoothP.setVisible(e);
        syncP.setVisible(e);
        Comp::setVisible(e);
    }
    void initModulatables() override {
        auto top = getTopLevelComponent();
        top->addChildComponent(rateP);
        top->addChildComponent(biasP);
        top->addChildComponent(widthP);
        top->addChildComponent(smoothP);
        top->addChildComponent(syncP);
    }
protected:
    nelG::Layout layout;
    pComp::Knob rateP, biasP, widthP, smoothP;
    pComp::Switch syncP;

    void paint(juce::Graphics& g) override {
        //layout.paintGrid(g);
        g.fillAll(utils.colours[Utils::Background]);
    }
    void resized() override {
        layout.setBounds(getBoundsInParent().toFloat());
        layout.place(rateP, 1, 1, 1, 1);
        layout.place(biasP, 2, 1, 1, 1);
        layout.place(widthP, 3, 1, 1, 1);
        layout.place(smoothP, 4, 1, 1, 1);
        layout.place(syncP, 1, 2, 1, 1, true);
        ModulatorComp::resized();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatorRandComp)
};

/*
* 
* to do
* each modulator has
*   preset menu
*
* modulators can be lfo or env
*   triggers: midi, automation, transient detection
*   but envelope follower triggered by envelope detection? hm, maybe not
*/