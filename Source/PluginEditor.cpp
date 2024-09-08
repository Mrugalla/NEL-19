#include "PluginProcessor.h"
#include "PluginEditor.h"

gui::Notify makeNotify(Nel19AudioProcessorEditor* comp)
{
    return [c = comp](int t, const void*)
    {
        if (t == gui::NotificationType::ColourChanged)
        {
            gui::makeCursor(*c, gui::CursorType::Default);
            c->repaint();
        }
        return false;
    };
}

Nel19AudioProcessorEditor::Nel19AudioProcessorEditor(Nel19AudioProcessor& p) :
    AudioProcessorEditor(&p),
    audioProcessor(p),
    layout
    (
        { 3, 21, 8 },
        { 2, 21, 1 }
    ),
    layoutMacros
    (
        { 1 },
        { 8, 3, 8, 3, 8, 3, 8, 3 }
    ),
    layoutMainParams
    (
        { 8, 13 },
        { 2, 5, 5, 5, 5, 2 }
    ),
    layoutBottomBar
    (
        { 21, 8 },
        { 1 }
    ),
    layoutTopBar
    (
        { 2, 2, 2, 2, 2, 13 },
        { 1 }
    ),
    utils(*this, p.appProperties.getUserSettings(), p),
    notify(utils.events, makeNotify(this)),
    nelLabel(utils, "<< NEL >>", gui::ColourID::Transp, gui::ColourID::Transp, gui::ColourID::Txt),
    tooltips(utils),
    buildDate(utils),
    modulatables(),
    macro0(utils, "M0", "Modulate parameters with this macro.", modSys6::PID::MSMacro0, modulatables, gui::ParameterType::Knob),
    macro1(utils, "M1", "Modulate parameters with this macro.", modSys6::PID::MSMacro1, modulatables, gui::ParameterType::Knob),
    macro2(utils, "M2", "Modulate parameters with this macro.", modSys6::PID::MSMacro2, modulatables, gui::ParameterType::Knob),
    macro3(utils, "M3", "Modulate parameters with this macro.", modSys6::PID::MSMacro3, modulatables, gui::ParameterType::Knob),
    modComps
    {
        gui::ModComp(utils, modulatables, audioProcessor.modulators[0].getTables(), 0),
        gui::ModComp(utils, modulatables, audioProcessor.modulators[1].getTables(), modSys6::NumParamsPerMod)
    },
    visualizer(utils, "Visualizes the sum of the vibrato's modulators.", p.getChannelCountOfBus(false, 0), 1),
	bufferSizes(utils, "Buffer", "Switch between different buffer sizes for the vibrato.", modSys6::PID::BufferSize, modulatables, gui::ParameterType::Knob),
    modsDepth(utils, "Depth", "Modulate the depth of the vibrato.", modSys6::PID::Depth, modulatables, gui::ParameterType::Knob),
    modsMix(utils, "Mods", "Interpolate between the vibrato's modulators.", modSys6::PID::ModsMix, modulatables, gui::ParameterType::Knob),
    dryWetMix(utils, "Mix", "Define the dry/wet ratio of the effect.", modSys6::PID::DryWetMix, modulatables, gui::ParameterType::Knob),
    gainWet(utils, "Gain", "The output gain of the wet signal.", modSys6::PID::WetGain, modulatables, gui::ParameterType::Knob),
    stereoConfig(utils, "StereoConfig", "Configurate if effect is applied to l/r or m/s", modSys6::PID::StereoConfig, modulatables, gui::ParameterType::Switch),
	feedback(utils, "FB", "Dial in some feedback to this vibrato's delay.", modSys6::PID::Feedback, modulatables, gui::ParameterType::Knob),
	damp(utils, "Damp", "This is a lowpass filter in the feedback path.", modSys6::PID::Damp, modulatables, gui::ParameterType::Knob),
    macro0Dragger(utils, 0, modulatables),
    macro1Dragger(utils, 1, modulatables),
    macro2Dragger(utils, 2, modulatables),
    macro3Dragger(utils, 3, modulatables),
    paramRandomizer(utils, modulatables),
	hq(utils, "HQ", "Strong vibrato causes less 'grainy' sidelobes with 4x oversampling.", modSys6::PID::HQ, modulatables, gui::ParameterType::Switch),
	lookahead(utils, "Lookahead", "Lookahead aligns the average position of the vibrato with the dry signal.", modSys6::PID::Lookahead, modulatables, gui::ParameterType::Switch),
    popUp(utils),
    enterValue(utils),
#if DebugMenuExists
    menu(nullptr),
    menuButton
    (
        utils,
        "All the extra stuff."
    )
#endif
#if PresetsExist
    ,presetBrowser(utils, ".nel", "presets")
#endif
{
    nelLabel.font = gui::Shared::shared.font;

    paramRandomizer.add(&stereoConfig);

    visualizer.onPaint = gui::makeVibratoVisualizerOnPaint2();
    visualizer.onUpdate = [&p = audioProcessor](gui::Visualizer::Buffer& b)
    {
        const auto& vals = p.visualizerValues;
        bool needsUpdate = false;
        for (auto ch = 0; ch < b.size(); ++ch)
        {
            const auto val = static_cast<float>(vals[ch]);

            if (b[ch][0] != val)
            {
                b[ch][0] = val;
                needsUpdate = true;
            }
        }
        return needsUpdate;
    };

    addAndMakeVisible(nelLabel);

    addAndMakeVisible(tooltips);
    addAndMakeVisible(buildDate);

    addAndMakeVisible(macro0);
    addAndMakeVisible(macro1);
    addAndMakeVisible(macro2);
    addAndMakeVisible(macro3);

    for(auto& m: modComps)
        addAndMakeVisible(m);

    addAndMakeVisible(paramRandomizer);
    addAndMakeVisible(hq);
	addAndMakeVisible(lookahead);
#if DebugMenuExists
    addAndMakeVisible(menuButton);
#endif
    
    addAndMakeVisible(visualizer);

	addAndMakeVisible(bufferSizes);
    addAndMakeVisible(modsDepth);
    addAndMakeVisible(modsMix);
    addAndMakeVisible(dryWetMix);
    addAndMakeVisible(gainWet);
    addAndMakeVisible(stereoConfig);
	addAndMakeVisible(feedback);
	addAndMakeVisible(damp);

    addAndMakeVisible(macro0Dragger);
    addAndMakeVisible(macro1Dragger);
    addAndMakeVisible(macro2Dragger);
    addAndMakeVisible(macro3Dragger);

    addAndMakeVisible(popUp);
    addChildComponent(enterValue);

#if PresetsExist
    presetBrowser.init(*this);
    presetBrowser.saveFunc = [&]()
    {
        utils.audioProcessor.savePatch();
        return utils.audioProcessor.params.state;
    };

    presetBrowser.loadFunc = [&](const juce::ValueTree& vt)
    {
        utils.updatePatch(vt);
        notify(gui::NotificationType::PatchUpdated);
    };
#endif

#if DebugMenuExists
    menuButton.onClick = [this]()
    {
        menu2::openMenu(menu, audioProcessor, utils, *this, layout(1, 1, 2, 1).toNearestInt(), menuButton);
    };
    
    menuButton.onPaint = [this](juce::Graphics& g, menu2::Button&)
    {
        menu2::paintMenuButton(g, menuButton, utils, menu.get());
    };
#endif

    setOpaque(true);
    gui::makeCursor(*this, gui::CursorType::Default);

    for (auto m = 0; m < modComps.size(); ++m)
    {
        auto& modComp = modComps[m];
        modComp.onModChange = [this, m](vibrato::ModType t)
        {
            audioProcessor.modType[m] = t;
        };
        modComp.setMod(p.modType[m]);
        modComp.addButtonsToRandomizer(paramRandomizer);
        modComp.getModType = [this, m]()
        {
            return audioProcessor.modType[m];
        };
    }

    paramRandomizer.add([this](juce::Random& rand)
    {
        audioProcessor.suspendProcessing(true);

        const auto numMods = static_cast<float>(vibrato::ModType::NumMods);
        for (auto m = 0; m < modComps.size(); ++m)
        {
            const auto val = rand.nextFloat() * (numMods - .1f);
            const auto type = static_cast<vibrato::ModType>(val);
            modComps[m].setMod(type);
        }

        audioProcessor.forcePrepare();
    });

    setResizable(true, true);
    {
        const auto user = p.appProperties.getUserSettings();
        const auto w = user->getIntValue("BoundsWidth", gui::Width);
        const auto h = user->getIntValue("BoundsHeight", gui::Height);
        setSize(w, h);
    }

    startTimerHz(60);
}

void Nel19AudioProcessorEditor::timerCallback()
{
    nelLabel.updateTimer();
    tooltips.updateTimer();
    buildDate.updateTimer();
    for (auto modulatable : modulatables)
        modulatable->updateTimer();
    for (auto& modComp : modComps)
        modComp.updateTimer();
    visualizer.updateTimer();
    macro0Dragger.updateTimer();
	macro1Dragger.updateTimer();
	macro2Dragger.updateTimer();
	macro3Dragger.updateTimer();
    paramRandomizer.updateTimer();
    popUp.updateTimer();
    enterValue.updateTimer();
#if DebugMenuExists
    if (menu != nullptr)
        menu->updateTimer();
    menuButton.updateTimer();
#endif
#if PresetsExist
    presetBrowser.updateTimer();
#endif
}

void Nel19AudioProcessorEditor::resized()
{
    if (getWidth() < MinEditorBounds)
        return setBounds(0, 0, MinEditorBounds, getHeight());
    else if (getHeight() < MinEditorBounds)
        return setBounds(0, 0, getWidth(), MinEditorBounds);

    utils.resized();

    const auto thicc = utils.thicc;

    layout.setBounds(getLocalBounds().toFloat().reduced(thicc));
    layoutBottomBar.setBounds(layout.bottomBar());
    layoutTopBar.setBounds(layout(0, 0, 2, 1));
    layoutMacros.setBounds(layout(0, 1, 1, 1));
    layoutMainParams.setBounds(layout(2, 1, 1, 1));
    
    layoutBottomBar.place(tooltips,  0, 0, 1, 1);
    layoutBottomBar.place(buildDate, 1, 0, 1, 1);

    layoutMacros.place(macro0, 0, 0, 1, 1);
    layoutMacros.place(macro1, 0, 2, 1, 1);
    layoutMacros.place(macro2, 0, 4, 1, 1);
    layoutMacros.place(macro3, 0, 6, 1, 1);
    macro0Dragger.setQBounds(layoutMacros(0, 1, 1, 1, true));
    macro1Dragger.setQBounds(layoutMacros(0, 3, 1, 1, true));
    macro2Dragger.setQBounds(layoutMacros(0, 5, 1, 1, true));
    macro3Dragger.setQBounds(layoutMacros(0, 7, 1, 1, true));

    {
        auto area = layout(1, 1, 1, 1);
        const auto x = area.getX();
        const auto w = area.getWidth();
		const auto h = area.getHeight() * .5f;
        auto y = area.getY();
        for (auto i = 0; i < 2; ++i)
        {
            auto& modComp = modComps[i];
            modComp.setBounds(juce::Rectangle<float>(x, y, w, h).toNearestInt());
            y += h;
        }
    }
    
	layoutMainParams.place(bufferSizes, 0, 0, 2, 1);
    layoutMainParams.place(modsDepth, 0, 1, 2, 1);
    layoutMainParams.place(modsMix, 0, 2, 2, 1);
    layoutMainParams.place(gainWet, 0, 3, 1, 1);
    layoutMainParams.place(dryWetMix, 1, 3, 1, 1);
    layoutMainParams.place(damp, 0, 4, 1, 1);
    layoutMainParams.place(feedback, 1, 4, 1, 1);
    layoutMainParams.place(stereoConfig, 0, 5, 1, 1, 0.f, true);
    
    layoutTopBar.place(paramRandomizer, 1, 0, 1, 1, 0.f, true);
#if DebugMenuExists
    layoutTopBar.place(menuButton,      0, 0, 1, 1, 0.f, true);
#endif
#if PresetsExist
    layoutTopBar.place(presetBrowser.getOpenCloseButton(), 2, 0, 1, 1, 0.f, true);
#endif
    layoutTopBar.place(hq,              3, 0, 1, 1, 0.f, true);
	layoutTopBar.place(lookahead,       4, 0, 1, 1, 0.f, true);
    layoutTopBar.place(nelLabel,        5, 0, 1, 1, thicc * 4.f);
    {
        auto area = layoutMainParams(0, 0, 2, 1);
        area.setY(layoutTopBar.getY(0));
		area.setHeight(static_cast<float>(hq.getHeight()));

        visualizer.setBounds(area.toNearestInt());
    }
    
    {
        const auto w = static_cast<int>(thicc * 25.f);
        const auto h = w * 75 / 100;

        popUp.setBounds({ 0, 0, w, h });
        enterValue.setBounds({ 0, 0, w, h });
    }

#if PresetsExist
    layout.place(presetBrowser, 1, 1, 2, 1, 0.f);
#endif

#if DebugMenuExists
    if(menu != nullptr)
        layout.place(*menu, 1, 1, 2, 1);
#endif

    {
        auto user = audioProcessor.appProperties.getUserSettings();
        user->setValue("BoundsWidth", getWidth());
        user->setValue("BoundsHeight", getHeight());
    }
}

void Nel19AudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(gui::Shared::shared.colour(gui::ColourID::Bg).withAlpha(1.f));
}

void Nel19AudioProcessorEditor::mouseEnter(const juce::MouseEvent&)
{
    utils.setTooltip(nullptr);
}

void Nel19AudioProcessorEditor::mouseDown(const juce::MouseEvent&)
{
    utils.killEnterValue();
}

#undef DebugMenuExists