#include "PluginProcessor.h"
#include "PluginEditor.h"

Nel19AudioProcessorEditor::Nel19AudioProcessorEditor(Nel19AudioProcessor& p) :
    AudioProcessorEditor(&p),
    audioProcessor(p),
    utils(p),
    layout(
        { 25, 150, 50, 150 },
        { 50, 50, 150, 150, 50, 30 }
    ),
    layoutMacros(
        { 1 },
        { 25, 12, 25, 12, 25, 12, 25, 12 }
    ),
    layoutMainParams(
        { 1 },
        { 80, 80, 80, 30 }
    ),
    layoutBottomBar(
        { 250, 90 },
        { 1 }
    ),
    layoutMiscs(
        { 1 },
        { 50, 200 }
    ),
    layoutTopBar(
        { 30, 250, 70 },
        { 1 }
    ),
    buildDate(p, utils),
    tooltips(p, utils),
    macro0(p, utils, param::ID::Macro0, "modulate parameters with a macro.", "Macro 0", param::getID(param::ID::Macro0), false, false),
    macro1(p, utils, param::ID::Macro1, "modulate parameters with a macro.", "Macro 1", param::getID(param::ID::Macro1), false, false),
    macro2(p, utils, param::ID::Macro2, "modulate parameters with a macro.", "Macro 2", param::getID(param::ID::Macro2), false, false),
    macro3(p, utils, param::ID::Macro3, "modulate parameters with a macro.", "Macro 3", param::getID(param::ID::Macro3), false, false),
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
    shuttle(p, utils, "Lionel's space shuttle vibrates the universe to your music :>", BinaryData::shuttle_png, BinaryData::shuttle_pngSize, pxl::colourize(utils, Utils::ColourID::Background, Utils::ColourID::Modulation)),
    menu(nullptr),
    menuButton(p, utils, "All the extra stuff.", [this]() { menu2::openMenu(menu, audioProcessor, utils, *this, layout(1, 1, 3, 4).toNearestInt() , menuButton); }, [this](juce::Graphics& g, const menu2::Button& b) { menu2::paintMenuButton(g, menuButton, utils, menu.get()); })
{
    auto numChannels = audioProcessor.getChannelCountOfBus(false, 0);

    addAndMakeVisible(buildDate);
    addAndMakeVisible(tooltips);
    addAndMakeVisible(macro0);
    addAndMakeVisible(macro1);
    addAndMakeVisible(macro2);
    addAndMakeVisible(macro3);
    addAndMakeVisible(shuttle); shuttle.setAlpha(.3f);

    addAndMakeVisible(modulatorsMix);
    addAndMakeVisible(depth);
    addAndMakeVisible(dryWetMix);
    if (numChannels == 2) addAndMakeVisible(midSideSwitch);

    // modulation components init stuff:
    auto matrix = audioProcessor.matrix.getCopyOfUpdatedPtr();
    for (auto mcs = 0; mcs < modulatorComps.size(); ++mcs) {
        // behaviour for changing the selected modulator[mcs]
        auto onModReplace = [this, sel = mcs](int idx) {
            for (auto& mc : modulatorComps[sel])
                mc->setVisible(false);
            auto matrixCopy = audioProcessor.matrix.getCopyOfUpdatedPtr();
            for (const auto& id : audioProcessor.modsIDs[sel])
                matrixCopy->setModulatorActive(id, false);
            matrixCopy->setModulatorActive(audioProcessor.modsIDs[sel][idx], true);
            audioProcessor.matrix.replaceUpdatedPtrWith(matrixCopy);
            modulatorComps[sel][idx]->setVisible(true);
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
        modulatorComps[mcs][Pitchbend] = std::make_unique<ModulatorPitchbendComp>(
            p, utils, audioProcessor.modsIDs[mcs][Pitchbend], modulatables, mcs, onModReplace
            );
        modulatorComps[mcs][Note] = std::make_unique<ModulatorNoteComp>(
            p, utils, audioProcessor.modsIDs[mcs][Note], modulatables, mcs, onModReplace
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
    paintModOptions.push_back(ModulatorComp::paintModOption(utils, "Pitchbend"));
    paintModOptions.push_back(ModulatorComp::paintModOption(utils, "Note"));
    for (auto mcs = 0; mcs < modulatorComps.size(); ++mcs)
        for (auto ms = 0; ms < NumMods; ++ms)
            modulatorComps[mcs][ms]->init(paintModOptions);

    audioProcessor.matrix.replaceUpdatedPtrWith(matrix);

    addAndMakeVisible(menuButton);
    addAndMakeVisible(popUp);

    setOpaque(true);
    setMouseCursor(utils.cursors[utils.Cursor::Norm]);

    setBufferedToImage(true);
    setResizable(true, true);
    startTimerHz(16);
    setSize(nelG::Width, nelG::Height);
}
void Nel19AudioProcessorEditor::resized() {
    layout.setBounds(getLocalBounds().toFloat().reduced(nelG::Thicc));
    layoutBottomBar.setBounds(layout.bottomBar());
    layoutTopBar.setBounds(layout.topBar());
    layoutMacros.setBounds(layout(0, 1, 1, 4).reduced(nelG::Thicc));
    layoutMainParams.setBounds(layout(2, 1, 1, 4).reduced(nelG::Thicc));
    layoutMiscs.setBounds(layout(3, 1, 1, 4).reduced(nelG::Thicc));

    layoutTopBar.place(menuButton,  0, 0, 1, 1, nelG::Thicc, true);

    layout.place(menu.get(), 1, 1, 4, 4);
    
    layoutBottomBar.place(buildDate, 1, 0, 1, 1, nelG::Thicc, false);
    layoutBottomBar.place(tooltips, 0, 0, 1, 1, nelG::Thicc, false);
    layoutMacros.place(macro0, 0, 0, 1, 1, nelG::Thicc);
    layoutMacros.place(macro1, 0, 2, 1, 1, nelG::Thicc);
    layoutMacros.place(macro2, 0, 4, 1, 1, nelG::Thicc);
    layoutMacros.place(macro3, 0, 6, 1, 1, nelG::Thicc);
    macroDragger0.setQBounds(layoutMacros(0, 1, 1, 1, true).reduced(nelG::Thicc).toNearestInt());
    macroDragger1.setQBounds(layoutMacros(0, 3, 1, 1, true).reduced(nelG::Thicc).toNearestInt());
    macroDragger2.setQBounds(layoutMacros(0, 5, 1, 1, true).reduced(nelG::Thicc).toNearestInt());
    macroDragger3.setQBounds(layoutMacros(0, 7, 1, 1, true).reduced(nelG::Thicc).toNearestInt());

    for (auto mcs = 0; mcs < 2; ++mcs)
        for (auto mm = 0; mm < modulatorComps[mcs].size(); ++mm)
            layout.place(*modulatorComps[mcs][mm], 1, 1 + mcs * 2, 1, 2, nelG::Thicc, false); 

    layoutMainParams.place(depth,         0, 0, 1, 1, nelG::Thicc);
    layoutMainParams.place(modulatorsMix, 0, 1, 1, 1, nelG::Thicc);
    layoutMainParams.place(dryWetMix,     0, 2, 1, 1, nelG::Thicc);
    layoutMainParams.place(midSideSwitch, 0, 3, 1, 1, nelG::Thicc, true);

    shuttle.setBounds(layoutMiscs().reduced(nelG::Thicc).toNearestInt());

    popUp.setBounds({0, 0, 100, 30});
}
void Nel19AudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::black);
    //g.fillAll(utils.colours[Utils::Background]);
    nelG::fillAndOutline(g, layoutMacros, utils.colours[Utils::ColourID::Background], utils.colours[Utils::ColourID::Normal]);
    for (auto& modComp : modulatorComps)
        nelG::fillAndOutline(g, *modComp[0].get(), utils.colours[Utils::ColourID::Background], utils.colours[Utils::ColourID::Normal]);
    nelG::fillAndOutline(g, layoutMainParams, utils.colours[Utils::ColourID::Background], utils.colours[Utils::ColourID::Normal]);
    nelG::fillAndOutline(g, layoutMiscs, utils.colours[Utils::ColourID::Background], utils.colours[Utils::ColourID::Normal]);
}
void Nel19AudioProcessorEditor::mouseEnter(const juce::MouseEvent&) { utils.updateTooltip(nullptr); }
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

/*

onModReplace can trigger some thread issue. why
    seems to create -nan in processor vibBuffer
    changed way it setVisible the modulatorComps (works now?)

*/