#pragma once
#include <array>

struct ModulatorComp :
    public Component,
    public modSys2::Identifiable
{
    ModulatorComp(Nel19AudioProcessor& p, Utils& u, const juce::Identifier& mID, std::vector<Modulatable*>& modulatableParameters, int modulatorsIdx) :
        Component(p, u),
        modSys2::Identifiable(mID),
        modulatables(modulatableParameters),
        randButton(processor, utils),
        modsIdx(modulatorsIdx)
    {
        addAndMakeVisible(randButton);
    }
    virtual void setActive(bool e) { setVisible(e); }
    virtual void initModulatables() {}
protected:
    std::vector<Modulatable*>& modulatables;
    RandomizerButton randButton;
    int modsIdx;

    void resized() override {
        const auto width = static_cast<float>(getWidth());
        const auto height = static_cast<float>(getHeight());

        const auto rbX = width * .85f;
        const auto rbY = height * .05f;
        const auto rbWidth = std::min(width, height) * .2f;
        randButton.setBounds(juce::Rectangle<float>(rbX, rbY, rbWidth, rbWidth).toNearestInt());
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatorComp)
};

struct ModulatorPerlinComp :
    public ModulatorComp
{
    ModulatorPerlinComp(Nel19AudioProcessor& p, Utils& u, const juce::Identifier& mID, std::vector<Modulatable*>& modulatableParameters, int modulatorsIdx) :
        ModulatorComp(p, u, mID, modulatableParameters, modulatorsIdx),
        layout(
            { 30, 70, 130, 70, 30 },
            { 70, 90, 30 }
        ),
        rateP(processor, utils, "The rate at which new values are picked.", modsIdx == 0 ? param::ID::PerlinRate0 : param::ID::PerlinRate1),
        octavesP(processor, utils, "More Octaves increase the complexity of the modulation.", modsIdx == 0 ? param::ID::PerlinOctaves0 : param::ID::PerlinOctaves1),
        widthP(processor, utils, "Increases the stereo-width of the modulation.", modsIdx == 0 ? param::ID::PerlinWidth0 : param::ID::PerlinWidth1)    
    {
        modulatableParameters.push_back(&rateP);
        modulatableParameters.push_back(&octavesP);
        modulatableParameters.push_back(&widthP);
        this->randButton.addRandomizable(&rateP);
        this->randButton.addRandomizable(&octavesP);
        this->randButton.addRandomizable(&widthP);
    }
    void setActive(bool e) override {
        rateP.setActive(e);
        octavesP.setActive(e);
        widthP.setActive(e);
        Component::setVisible(e);
    }
    void initModulatables() override {
        auto top = getTopLevelComponent();
        top->addChildComponent(rateP);
        top->addChildComponent(octavesP);
        top->addChildComponent(widthP);
    }
protected:
    nelG::Layout layout;
    ModulatableKnob rateP, octavesP, widthP;

    void paint(juce::Graphics& g) override {
        //layout.paintGrid(g);
        g.fillAll(utils.colours[Utils::Background]);
        //outtake::drawRandGrid(g, getLocalBounds(), 32, 16, juce::Colour(nelG::ColDarkGrey), .1f);
    }
    void resized() override {
        layout.setBounds(getBoundsInParent().toFloat());
        layout.place(rateP, 1, 1, 1, 1, true);
        layout.place(octavesP, 2, 1, 1, 1, true);
        layout.place(widthP, 3, 1, 1, 1, true);
        ModulatorComp::resized();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatorPerlinComp)
};

struct ModulatorLFOComp :
    public ModulatorComp
{
    ModulatorLFOComp(Nel19AudioProcessor& p, Utils& u, const juce::Identifier& mID, std::vector<Modulatable*>& modulatableParameters, int modulatorsIdx) :
        ModulatorComp(p, u, mID, modulatableParameters, modulatorsIdx),
        layout(
            { 30, 50, 90, 50, 50, 30 },
            { 50, 60, 30 }
        ),
        rateP(processor, utils, "The rate at which new values are picked.", modsIdx == 0 ? param::ID::LFORate0 : param::ID::LFORate1),
        widthP(processor, utils, "Increases the stereo-width of the modulation.", modsIdx == 0 ? param::ID::LFOWidth0 : param::ID::LFOWidth1),
        phaseP(processor, utils, "PHASE", modsIdx == 0 ? param::ID::LFOPhase0 : param::ID::LFOPhase1),
        waveformP(processor, utils, "Change the waveform of this modulator.", modsIdx == 0 ? param::ID::LFOWaveTable0 : param::ID::LFOWaveTable1),
        tempoSyncP(processor, utils, "Switch between free-running or tempo-sync modulation.", modsIdx == 0 ? param::ID::LFOSync0 : param::ID::LFOSync1),
        polarityP(processor, utils, "POLARITY", modsIdx == 0 ? param::ID::LFOPolarity0 : param::ID::LFOPolarity1)
    {
        modulatableParameters.push_back(&rateP);
        modulatableParameters.push_back(&widthP);
        modulatableParameters.push_back(&phaseP);
        this->randButton.addRandomizable(&rateP);
        this->randButton.addRandomizable(&widthP);
        this->randButton.addRandomizable(&waveformP);
        this->randButton.addRandomizable(&tempoSyncP);
        this->randButton.addRandomizable(&polarityP);
        this->randButton.addRandomizable(&phaseP);
    }
    void setActive(bool e) override {
        Component::setVisible(e);
        rateP.setActive(e);
        widthP.setActive(e);
        waveformP.setActive(e);
        tempoSyncP.setActive(e);
        polarityP.setActive(e);
        phaseP.setActive(e);
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
    ModulatableKnob rateP, widthP, phaseP;
    WaveformChooser waveformP;
    Switch tempoSyncP, polarityP;

    void paint(juce::Graphics& g) override {
        //layout.paintGrid(g);
        g.fillAll(utils.colours[Utils::Background]);
    }
    void resized() override {
        layout.setBounds(getBoundsInParent().toFloat());
        layout.place(tempoSyncP, 1, 2, 1, 1, true);
        layout.place(rateP, 1, 1, 1, 1, true);
        layout.place(waveformP, 2, 1, 1, 1, true);
        layout.place(widthP, 3, 1, 1, 1, true);
        layout.place(phaseP, 4, 1, 1, 1, true);
        layout.place(polarityP, 4, 2, 1, 1, true);
        ModulatorComp::resized();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatorLFOComp)
};

struct ModulatorEnvelopeFollowerComp :
    public ModulatorComp
{
    ModulatorEnvelopeFollowerComp(Nel19AudioProcessor& p, Utils& u, const juce::Identifier& mID, std::vector<Modulatable*>& modulatableParameters, int modulatorsIdx) :
        ModulatorComp(p, u, mID, modulatableParameters, modulatorsIdx),
        layout(
            { 30, 40, 80, 80, 50, 50, 30 },
            { 50, 60, 30 }
        ),
        atkP(processor, utils, "Set how fast the modulator reacts to transients.", modsIdx == 0 ? param::ID::EnvFolAtk0 : param::ID::EnvFolAtk1),
        rlsP(processor, utils, "Set how long the modulator needs to release.", modsIdx == 0 ? param::ID::EnvFolRls0 : param::ID::EnvFolRls1),
        gainP(processor, utils, "Define the modulator's input gain.", modsIdx == 0 ? param::ID::EnvFolGain0 : param::ID::EnvFolGain1),
        biasP(processor, utils, "Changes the character of the modulation.", modsIdx == 0 ? param::ID::EnvFolBias0 : param::ID::EnvFolBias1),
        widthP(processor, utils, "Changes the stereo-width of the modulator.", modsIdx == 0 ? param::ID::EnvFolWidth0 : param::ID::EnvFolWidth1)
    {
        modulatableParameters.push_back(&atkP);
        modulatableParameters.push_back(&rlsP);
        modulatableParameters.push_back(&gainP);
        modulatableParameters.push_back(&biasP);
        modulatableParameters.push_back(&widthP);

        this->randButton.addRandomizable(&atkP);
        this->randButton.addRandomizable(&rlsP);
        this->randButton.addRandomizable(&gainP);
        this->randButton.addRandomizable(&biasP);
        this->randButton.addRandomizable(&widthP);
    }
    void setActive(bool e) override {
        Component::setVisible(e);
        atkP.setActive(e);
        rlsP.setActive(e);
        gainP.setActive(e);
        biasP.setActive(e);
        widthP.setActive(e);
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
    ModulatableKnob atkP, rlsP, gainP, biasP, widthP;

    void paint(juce::Graphics& g) override {
        //layout.paintGrid(g);
        g.fillAll(utils.colours[Utils::Background]);
    }
    void resized() override {
        layout.setBounds(getBoundsInParent().toFloat());
        layout.place(gainP, 1, 1, 1, 1, true);
        layout.place(atkP, 2, 1, 1, 1, true);
        layout.place(rlsP, 3, 1, 1, 1, true);
        layout.place(biasP, 4, 1, 1, 1, true);
        layout.place(widthP, 5, 1, 1, 1, true);
        ModulatorComp::resized();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatorEnvelopeFollowerComp)
};

struct ModulatorRandComp :
    public ModulatorComp
{
    ModulatorRandComp(Nel19AudioProcessor& p, Utils& u, const juce::Identifier& mID, std::vector<Modulatable*>& modulatableParameters, int modulatorsIdx) :
        ModulatorComp(p, u, mID, modulatableParameters, modulatorsIdx),
        layout(
            { 30, 70, 70, 70, 70, 30 },
            { 70, 90, 30 }
        ),
        rateP(processor, utils, "The rate at which new values are picked.", modsIdx == 0 ? param::ID::RandRate0 : param::ID::RandRate1),
        biasP(processor, utils, "The bias of the values picked.", modsIdx == 0 ? param::ID::RandBias0 : param::ID::RandBias1),
        widthP(processor, utils, "Increases the stereo-width of the modulation.", modsIdx == 0 ? param::ID::RandWidth0 : param::ID::RandWidth1),
        smoothP(processor, utils, "Increases the smoothness of the modulation.", modsIdx == 0 ? param::ID::RandSmooth0 : param::ID::RandSmooth1),
        syncP(processor, utils, "Switches between free or tempo-sync modulation.", modsIdx == 0 ? param::ID::RandSync0 : param::ID::RandSync1)
    {
        modulatableParameters.push_back(&rateP);
        modulatableParameters.push_back(&biasP);
        modulatableParameters.push_back(&widthP);
        modulatableParameters.push_back(&smoothP);
        this->randButton.addRandomizable(&rateP);
        this->randButton.addRandomizable(&biasP);
        this->randButton.addRandomizable(&widthP);
        this->randButton.addRandomizable(&smoothP);
        this->randButton.addRandomizable(&syncP);
    }
    void setActive(bool e) override {
        rateP.setActive(e);
        biasP.setActive(e);
        widthP.setActive(e);
        smoothP.setActive(e);
        syncP.setActive(e);
        Component::setVisible(e);
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
    ModulatableKnob rateP, biasP, widthP, smoothP;
    Switch syncP;

    void paint(juce::Graphics& g) override {
        //layout.paintGrid(g);
        g.fillAll(utils.colours[Utils::Background]);
    }
    void resized() override {
        layout.setBounds(getBoundsInParent().toFloat());
        layout.place(rateP, 1, 1, 1, 1, true);
        layout.place(biasP, 2, 1, 1, 1, true);
        layout.place(widthP, 3, 1, 1, 1, true);
        layout.place(smoothP, 4, 1, 1, 1, true);
        layout.place(syncP, 1, 2, 1, 1, true);
        ModulatorComp::resized();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatorRandComp)
};

/*
* 
* to do
* each modulator has
*   name/id             CHECK
*   parameters          CHECK
*       & randomizer    CHECK
*   preset menu?
*   selector            CHECK almost
*
* modulators can be lfo or env
*   triggers: midi, automation, transient detection
*   envelope follower triggered by envelope detection? hm, maybe not
*/