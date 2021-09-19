#include "PluginProcessor.h"
#include "PluginEditor.h"

Nel19AudioProcessorEditor::Nel19AudioProcessorEditor(Nel19AudioProcessor& p) :
    AudioProcessorEditor(&p),
    audioProcessor(p),
    utils(p),
    layout(
        { 30, 150, 30, 30, 120 },
        { 50, 50, 150, 150, 50, 30 }
    ),
    layoutMacros(
        { 1 },
        { 25, 12, 25, 12, 25, 12, 25, 12 }
    ),
    layoutDepthMix(
        { 1 },
        { 70, 50 }
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
    macro0(p, utils, param::ID::Macro0, "modulate parameters with a macro.", "Macro 0", param::getID(param::ID::Macro0), false),
    macro1(p, utils, param::ID::Macro1, "modulate parameters with a macro.", "Macro 1", param::getID(param::ID::Macro1), false),
    macro2(p, utils, param::ID::Macro2, "modulate parameters with a macro.", "Macro 2", param::getID(param::ID::Macro2), false),
    macro3(p, utils, param::ID::Macro3, "modulate parameters with a macro.", "Macro 3", param::getID(param::ID::Macro3), false),
    modulatorsMix(p, utils, param::ID::ModulatorsMix, "mix the modulators.", "Mods Mix"),
    depth(p, utils, param::ID::Depth, "set the depth of the vibrato.", "Depth"),
    dryWetMix(p, utils, param::ID::DryWetMix, "mix the dry signal with the vibrated one.", "Dry/Wet-Mix"),
    midSideSwitch(p, utils, "switch between l/r & m/s processing.", "Stereo-Config", param::ID::StereoConfig),
    modulatables({ &depth, &modulatorsMix, &dryWetMix }) ,
    macroDragger0(p, utils, "drag this to modulate a parameter.", param::getID(param::ID::Macro0), modulatables),
    macroDragger1(p, utils, "drag this to modulate a parameter.", param::getID(param::ID::Macro1), modulatables),
    macroDragger2(p, utils, "drag this to modulate a parameter.", param::getID(param::ID::Macro2), modulatables),
    macroDragger3(p, utils, "drag this to modulate a parameter.", param::getID(param::ID::Macro3), modulatables),
    popUp(p, utils),
    modulatorComps(),
    visualizer(p, utils),
    menu(nullptr),
    menuButton(p, utils, "All the extra stuff.", [this]() { menu2::openMenu(menu, audioProcessor, utils, *this, layout(1, 1, 4, 4).toNearestInt() , menuButton); }, [this](juce::Graphics& g, const menu2::Button& b) { menu2::paintMenuButton(g, menuButton, utils, menu.get()); })
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
    layoutParams.addNamedLocation("stereo config", 1, 2, 1, 1, true, false, false);
    layoutParams.addNamedLocation("depth max", 2, 2, 1, 1, true, false);

    layoutTopBar.addNamedLocation("menu", 0, 0, 1, 1, true, false);
    layoutTopBar.addNamedLocation("presets", 1, 0, 1, 1, false, false);
    layoutTopBar.addNamedLocation("title", 2, 0, 1, 1, false, false);

    auto numChannels = audioProcessor.getChannelCountOfBus(false, 0);

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
    if(numChannels == 2) addAndMakeVisible(midSideSwitch);

    // modulation components init stuff:
    auto matrix = audioProcessor.matrix.getCopyOfUpdatedPtr();
    enum { EnvFol, LFO, Rand, Perlin, NumMods };
    for (auto mcs = 0; mcs < modulatorComps.size(); ++mcs) {
        // behaviour for changing the selected modulator[mcs]
        auto onModReplace = [this, sel = mcs](int idx) {
            auto matrixCopy = audioProcessor.matrix.getCopyOfUpdatedPtr();
            for (const auto& id : audioProcessor.modsIDs[sel])
                matrixCopy->setModulatorActive(id, false);
            matrixCopy->setModulatorActive(audioProcessor.modsIDs[sel][idx], true);
            audioProcessor.matrix.replaceUpdatedPtrWith(matrixCopy);
            resetModulatorComp(sel, idx);
        };

        // create and add modulator components + their modulatables
        modulatorComps[mcs][EnvFol] = std::make_unique<ModulatorEnvelopeFollowerComp>(
            p, utils, audioProcessor.modsIDs[mcs][EnvFol], modulatables, mcs, onModReplace
        );
        modulatorComps[mcs][LFO] = std::make_unique<ModulatorLFOComp>(
            p, utils, audioProcessor.modsIDs[mcs][LFO], modulatables, mcs, onModReplace
        );
        modulatorComps[mcs][Rand] = std::make_unique<ModulatorRandComp>(
            p, utils, audioProcessor.modsIDs[mcs][Rand], modulatables, mcs, onModReplace
        );
        modulatorComps[mcs][Perlin] = std::make_unique<ModulatorPerlinComp>(
            p, utils, audioProcessor.modsIDs[mcs][Perlin], modulatables, mcs, onModReplace
        );

        for (auto ms = 0; ms < NumMods; ++ms) {
            addChildComponent(modulatorComps[mcs][ms].get());
            modulatorComps[mcs][ms]->initModulatables();
        }

        // loading current modulatorselectors' states
        for (auto m = 0; m < audioProcessor.modsIDs[mcs].size(); ++m) {
            const auto& id = audioProcessor.modsIDs[mcs][m];
            const auto mod = matrix->getModulator(id);
            bool modValid = mod != nullptr && mod->isActive();
            if (modValid) {
                resetModulatorComp(mcs, m);
                break;
            }
        }
    }

    addAndMakeVisible(macroDragger0);
    addAndMakeVisible(macroDragger1);
    addAndMakeVisible(macroDragger2);
    addAndMakeVisible(macroDragger3);

    std::vector<std::function<void(juce::Graphics&, Comp*, bool)>> paintModOptions;
    paintModOptions.push_back(ModulatorComp::paintModOption(utils, "Envelope\nFollower"));
    paintModOptions.push_back(ModulatorComp::paintModOption(utils, "LFO"));
    paintModOptions.push_back(ModulatorComp::paintModOption(utils, "Classic\nRand"));
    paintModOptions.push_back(ModulatorComp::paintModOption(utils, "Perlin\nNoise"));
    for (auto mcs = 0; mcs < modulatorComps.size(); ++mcs)
        for (auto ms = 0; ms < NumMods; ++ms)
            modulatorComps[mcs][ms]->init(paintModOptions);

    audioProcessor.matrix.replaceUpdatedPtrWith(matrix);

    addAndMakeVisible(menuButton);
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

    layoutTopBar.place(menuButton, 0, 0, 1, 1, true);
    layout.place(menu.get(), 1, 1, 4, 4);
    
    layoutBottomBar.place(buildDate, 1, 0, 1, 1, false);
    layoutBottomBar.place(tooltips, 0, 0, 1, 1, false);
    layoutMacros.place(macro0, 0, 0, 1, 1);
    layoutMacros.place(macro1, 0, 2, 1, 1);
    layoutMacros.place(macro2, 0, 4, 1, 1);
    layoutMacros.place(macro3, 0, 6, 1, 1);
    macroDragger0.setQBounds(layoutMacros(0, 1, 1, 1, true).toNearestInt());
    macroDragger1.setQBounds(layoutMacros(0, 3, 1, 1, true).toNearestInt());
    macroDragger2.setQBounds(layoutMacros(0, 5, 1, 1, true).toNearestInt());
    macroDragger3.setQBounds(layoutMacros(0, 7, 1, 1, true).toNearestInt());

    layoutParams.place(midSideSwitch, 1, 2, 1, 1, false);

    for (auto mcs = 0; mcs < 2; ++mcs)
        for (auto mm = 0; mm < modulatorComps[mcs].size(); ++mm)
            layout.place(*modulatorComps[mcs][mm], 1, 1 + mcs * 2, 1, 2); 

    layoutDepthMix.place(depth,         0, 0, 1, 1);
    layoutDepthMix.place(modulatorsMix, 0, 1, 1, 1);

    layoutParams.place(dryWetMix,  0, 1, 2, 1);
    layoutParams.place(visualizer, 0, 0, 3, 1);

    popUp.setBounds({0, 0, 100, 30});
}
void Nel19AudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(utils.colours[Utils::Background]);
    layout.paintGrid(g);
    layoutMacros.paintGrid(g);
    layoutDepthMix.paintGrid(g);
    layoutBottomBar.paintGrid(g);
    layoutParams.paintGrid(g);
    layoutTopBar.paintGrid(g);
}
void Nel19AudioProcessorEditor::mouseEnter(const juce::MouseEvent&) { utils.updateTooltip(nullptr); }
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
void Nel19AudioProcessorEditor::resetModulatorComp(int modIdx, int modTypeIdx) {
    for (auto& mc : modulatorComps[modIdx])
        mc->setVisible(false);
    modulatorComps[modIdx][modTypeIdx]->setVisible(true);
}