#include "PluginProcessor.h"
#include "PluginEditor.h"

Nel19AudioProcessorEditor::Nel19AudioProcessorEditor(Nel19AudioProcessor& p) :
    AudioProcessorEditor(&p),
    audioProcessor(p),
    utils(p),
    layout(
        { 30, 150, 50, 50, 100 },
        { 50, 50, 150, 150, 50, 30 }
    ),
    layoutMacros(
        { 1 },
        { 25, 12, 25, 12, 25, 12, 25, 12 }
    ),
    layoutDepthMix(
        { 50, 120, 50 },
        { 50, 120, 50 }
    ),
    layoutBottomBar(
        { 250, 90 },
        { 1 }
    ),
    layoutParams(
        { 50, 50, 50 },
        { 20, 80, 20 }
    ),
    layoutTopBar(
        { 30, 150, 100, 100 },
        { 1 }
    ),
    buildDate(p, utils),
    tooltips(p, utils),
    macro0(p, utils, "modulate parameters with a macro.", param::ID::Macro0, param::getID(param::ID::Macro0)),
    macro1(p, utils, "modulate parameters with a macro.", param::ID::Macro1, param::getID(param::ID::Macro1)),
    macro2(p, utils, "modulate parameters with a macro.", param::ID::Macro2, param::getID(param::ID::Macro2)),
    macro3(p, utils, "modulate parameters with a macro.", param::ID::Macro3, param::getID(param::ID::Macro3)),
    modulatorsMix(p, utils, "mix the modulators.", param::ID::ModulatorsMix),
    depth(p, utils, "set the depth of the vibrato.", param::ID::Depth, true),
    dryWetMix(p, utils, "mix the dry signal with the vibrated one.", param::ID::DryWetMix),
    modulatables({ &depth, &modulatorsMix, &dryWetMix }) ,
    macroDragger0(p, utils, "drag this to modulate a parameter.", "macro0", modulatables),
    macroDragger1(p, utils, "drag this to modulate a parameter.", "macro1", modulatables),
    macroDragger2(p, utils, "drag this to modulate a parameter.", "macro2", modulatables),
    macroDragger3(p, utils, "drag this to modulate a parameter.", "macro3", modulatables),
    modSelector(),
    popUp(p, utils),
    modulatorComps(),
    visualizer(p, utils)
{
    layout.addNamedLocation("top bar", 0, 0, 5, 1, false, false, false);
    layout.addNamedLocation("macros", 0, 1, 1, 4, false, false, false);
    layout.addNamedLocation("modulator 0", 1, 1, 1, 2, false, false, false);
    layout.addNamedLocation("m0 select", 2, 1, 1, 1, false, false, false);
    layout.addNamedLocation("modulator 1", 1, 3, 1, 2, false, false, false);
    layout.addNamedLocation("m1 select", 2, 4, 1, 1, false, false, false);
    layout.addNamedLocation("paramcs", 3, 1, 2, 4, false, false, false);
    layout.addNamedLocation("depth&ModMix", 2, 2, 2, 2, false, false, false);
    layout.addNamedLocation("bottom bar", 0, 5, 5, 1, false, false, false);

    layoutMacros.addNamedLocation("macro 0", 0, 0, 1, 1, true, true, false);
    layoutMacros.addNamedLocation("macro 1", 0, 2, 1, 1, true, true, false);
    layoutMacros.addNamedLocation("macro 2", 0, 4, 1, 1, true, true, false);
    layoutMacros.addNamedLocation("macro 3", 0, 6, 1, 1, true, true, false);
    layoutMacros.addNamedLocation("m0 drag", 0, 1, 1, 1, false, false, false);
    layoutMacros.addNamedLocation("m1 drag", 0, 3, 1, 1, false, false, false);
    layoutMacros.addNamedLocation("m2 drag", 0, 5, 1, 1, false, false, false);
    layoutMacros.addNamedLocation("m3 drag", 0, 7, 1, 1, false, false, false);

    layoutDepthMix.addNamedLocation("depth", 1, 1, 1, 1, true, true, false);
    layoutDepthMix.addNamedLocation("mods mix", 0, 0, 3, 3, true, true, false);

    layoutBottomBar.addNamedLocation("tooltip", 0, 0, 1, 1);
    layoutBottomBar.addNamedLocation("build num", 1, 0, 1, 1);

    layoutParams.addNamedLocation("visualizer", 0, 0, 3, 1, false, false);
    layoutParams.addNamedLocation("dry/wet mix", 0, 1, 2, 1, true, true, false);
    layoutParams.addNamedLocation("voices", 0, 2, 1, 1, true, false);
    layoutParams.addNamedLocation("stereo config", 1, 2, 1, 1, true, false);
    layoutParams.addNamedLocation("depth max", 2, 2, 1, 1, true, false);

    layoutTopBar.addNamedLocation("menu", 0, 0, 1, 1, true, false);
    layoutTopBar.addNamedLocation("presets", 1, 0, 1, 1, false, false);
    layoutTopBar.addNamedLocation("title", 2, 0, 1, 1, false, false);

    addAndMakeVisible(buildDate);
    addAndMakeVisible(tooltips);
    addAndMakeVisible(macro0);
    addAndMakeVisible(macro1);
    addAndMakeVisible(macro2);
    addAndMakeVisible(macro3);
    addAndMakeVisible(visualizer);
    addAndMakeVisible(modulatorsMix);
    addAndMakeVisible(depth);
    addAndMakeVisible(dryWetMix);

    // modulation selector stuff init:
    auto matrix = audioProcessor.matrix.getCopyOfUpdatedPtr();
    const auto numModulatorComps = modulatorComps.size();
    for (auto mcs = 0; mcs < numModulatorComps; ++mcs) {
        // create and add modulator components + their modulatables
        enum { EnvFol, LFO, Rand, Perlin, NumMods };
        modulatorComps[mcs][EnvFol] = std::make_unique<ModulatorEnvelopeFollowerComp>(
            p, utils, audioProcessor.modsIDs[mcs][EnvFol], modulatables, mcs
        );
        modulatorComps[mcs][LFO] = std::make_unique<ModulatorLFOComp>(
            p, utils, audioProcessor.modsIDs[mcs][LFO], modulatables, mcs
        );
        modulatorComps[mcs][Rand] = std::make_unique<ModulatorComp>(
            p, utils, audioProcessor.modsIDs[mcs][Rand], modulatables, mcs
        );
        modulatorComps[mcs][Perlin] = std::make_unique<ModulatorPerlinComp>(
            p, utils, audioProcessor.modsIDs[mcs][Perlin], modulatables, mcs
        );
        for (auto ms = 0; ms < NumMods; ++ms) {
            addChildComponent(*modulatorComps[mcs][ms]);
            modulatorComps[mcs][ms]->initModulatables();
        }

        modSelector.push_back(std::make_unique<DropDownMenu>(p, utils, "select a modulator."));

        addAndMakeVisible(*modSelector[mcs]);
        addChildComponent(modSelector[mcs]->list);
        modSelector[mcs]->addEntry("Envelope Follower");
        modSelector[mcs]->addEntry("LFO");
        modSelector[mcs]->addEntry("Random Classic");
        modSelector[mcs]->addEntry("Random Perlin");

        // setting up functionality for the modulator-selector dropdown
        modSelector[mcs]->setOnSelect([this, sel = mcs](int idx) {
            auto matrixCopy = audioProcessor.matrix.getCopyOfUpdatedPtr();
            for (const auto& id : audioProcessor.modsIDs[sel])
                matrixCopy->setModulatorActive(id, false); // dangerous!
            matrixCopy->setModulatorActive(audioProcessor.modsIDs[sel][idx], true); // also dangerous!
            audioProcessor.matrix.replaceUpdatedPtrWith(matrixCopy);
            resetModulatorComponent(sel, idx);
        });

        modSelector[mcs]->setOnDown([this, sel = mcs]() {
            auto modsChild = audioProcessor.apvts.state.getChildWithName(audioProcessor.modulatorsID);
            for (auto i = 0; i < audioProcessor.modsIDs[sel].size(); ++i) {
                auto& id = audioProcessor.modsIDs[sel][i];
                auto mod = audioProcessor.matrix->getModulator(id);
                auto active = mod->isActive();
                if (active)
                    return modSelector[sel]->setActive(i);
            }
        });

        // loading current modulatorselectors' states
        for (auto m = 0; m < audioProcessor.modsIDs[mcs].size(); ++m) {
            const auto& id = audioProcessor.modsIDs[mcs][m];
            const auto mod = matrix->getModulator(id);
            bool modValid = mod != nullptr && mod->isActive();
            if (modValid) {
                modSelector[mcs]->setActive(m);
                resetModulatorComponent(mcs, m);
                break;
            }
        }
    }
    audioProcessor.matrix.replaceUpdatedPtrWith(matrix);

    addAndMakeVisible(macroDragger0);
    addAndMakeVisible(macroDragger1);
    addAndMakeVisible(macroDragger2);
    addAndMakeVisible(macroDragger3);
    addAndMakeVisible(popUp);

    setOpaque(true);
    setMouseCursor(utils.cursors[utils.Cursor::Norm]);

    setResizable(true, true);
    startTimerHz(16);
    setSize(nelG::Width, nelG::Height);
}
void Nel19AudioProcessorEditor::resized() {
    layout.setBounds(getLocalBounds().toFloat().reduced(8.f));
    layoutMacros.setBounds(layout(0, 1, 1, 4));
    layoutDepthMix.setBounds(layout(2, 2, 2, 2));
    layoutBottomBar.setBounds(layout.bottomBar());
    layoutParams.setBounds(layout(4, 1, 1, 4));
    layoutTopBar.setBounds(layout.topBar());

    layoutBottomBar.place(buildDate, 1, 0, 1, 1, false);
    layoutBottomBar.place(tooltips, 0, 0, 1, 1, false);
    layoutMacros.place(macro0, 0, 0, 1, 1, true); macroDragger0.setQBounds(layoutMacros(0, 1, 1, 1));
    layoutMacros.place(macro1, 0, 2, 1, 1, true); macroDragger1.setQBounds(layoutMacros(0, 3, 1, 1));
    layoutMacros.place(macro2, 0, 4, 1, 1, true); macroDragger2.setQBounds(layoutMacros(0, 5, 1, 1));
    layoutMacros.place(macro3, 0, 6, 1, 1, true); macroDragger3.setQBounds(layoutMacros(0, 7, 1, 1));

    for (auto mcs = 0; mcs < 2; ++mcs) {
        layout.place(*modSelector[mcs], 2, 1 + mcs * 3, 1, 1, false);
        layout.place(modSelector[mcs]->list, 2, 2, 1, 2, false);
        for(auto mm = 0; mm < modulatorComps[mcs].size(); ++mm)
            layout.place(*modulatorComps[mcs][mm], 1, 1 + mcs * 2, 1, 2, false);
    }   

    layoutDepthMix.place(modulatorsMix, 0, 0, 3, 3, true);
    layoutDepthMix.place(depth, 1, 1, 1, 1, true);
    layoutParams.place(dryWetMix, 0, 1, 2, 1, true);
    layoutParams.place(visualizer, 0, 0, 3, 1, false);

    popUp.setBounds({0, 0, 100, 50});
}
void Nel19AudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(nelG::ColBlack));
    layout.paintGrid(g);
    layoutMacros.paintGrid(g);
    layoutDepthMix.paintGrid(g);
    layoutBottomBar.paintGrid(g);
    layoutParams.paintGrid(g);
    layoutTopBar.paintGrid(g);
}
void Nel19AudioProcessorEditor::mouseEnter(const juce::MouseEvent&) { utils.tooltip = nullptr; }
void Nel19AudioProcessorEditor::mouseMove(const juce::MouseEvent& evt) {
    /*
    layout.mouseMove(evt.position);
    layoutMacros.mouseMove(evt.position);
    layoutDepthMix.mouseMove(evt.position);
    layoutBottomBar.mouseMove(evt.position);
    layoutParams.mouseMove(evt.position);
    layoutTopBar.mouseMove(evt.position);
    repaint();
    */
}
void Nel19AudioProcessorEditor::timerCallback() {
    const auto matrix = audioProcessor.matrix.getUpdatedPtr();
    macroDragger0.timerCallback(matrix); macroDragger1.timerCallback(matrix);
    macroDragger2.timerCallback(matrix); macroDragger3.timerCallback(matrix);

    for(auto modulatable : modulatables)
        modulatable->updateModSys(matrix);
}
void Nel19AudioProcessorEditor::resetModulatorComponent(int modIdx, int modTypeIdx) {
    for (auto& mc : modulatorComps[modIdx])
        mc->setActive(false);
    modulatorComps[modIdx][modTypeIdx]->setActive(true);
}