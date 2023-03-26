#include "PluginProcessor.h"
#include "PluginEditor.h"

modSys6::gui::Notify makeNotify(Nel19AudioProcessorEditor* comp)
{
    return [c = comp](int t, const void*)
    {
        if (t == modSys6::gui::NotificationType::ColourChanged)
        {
            modSys6::gui::makeCursor(*c, modSys6::gui::CursorType::Default);
            c->repaint();
        }
        return false;
    };
}

modSys6::gui::Notify makeNotifyDepthModes(std::array<modSys6::gui::Button, 5>& buttons, int i)
{
    return [&btns = buttons, i](int t, const void*)
    {
        for (auto& btn : btns)
            btn.setState(0);

        auto& btn = btns[i];

        if (t == modSys6::gui::NotificationType::PatchUpdated)
        {
            auto& p = btn.utils.audioProcessor;
            const auto fs = static_cast<float>(p.oversampling.getSampleRateUpsampled());
            const auto sizeInMs = std::round(p.vibrat.getSizeInMs(fs));

            btn.setState
            (
                sizeInMs == (i == 0 ? 1.f : i == 1 ? 4.f : i == 2 ? 20.f : i == 3 ? 420.f : 2000.f) ? 1 : 0
            );
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
        { 2, 2, 2, 2, 13 },
        { 1 }
    ),
    utils(p.modSys, *this, p.appProperties.getUserSettings(), p),
    notify(utils.events, makeNotify(this)),

    nelLabel(utils, "<< NEL >>", modSys6::gui::ColourID::Transp, modSys6::gui::ColourID::Transp, modSys6::gui::ColourID::Txt),

    tooltips(utils),
    buildDate(utils),
    
    modulatables(),

    macro0(utils, "M0", "Modulate parameters with this macro.", modSys6::PID::MSMacro0, modulatables, modSys6::gui::ParameterType::Knob),
    macro1(utils, "M1", "Modulate parameters with this macro.", modSys6::PID::MSMacro1, modulatables, modSys6::gui::ParameterType::Knob),
    macro2(utils, "M2", "Modulate parameters with this macro.", modSys6::PID::MSMacro2, modulatables, modSys6::gui::ParameterType::Knob),
    macro3(utils, "M3", "Modulate parameters with this macro.", modSys6::PID::MSMacro3, modulatables, modSys6::gui::ParameterType::Knob),

    modComps
    {
        modSys6::gui::ModComp(utils, modulatables, audioProcessor.modulators[0].getTables(), 0),
        modSys6::gui::ModComp(utils, modulatables, audioProcessor.modulators[1].getTables(), modSys6::NumParamsPerMod)
    },

    modsDepth(utils, "Depth", "Modulate the depth of the vibrato.", modSys6::PID::Depth, modulatables, modSys6::gui::ParameterType::Knob),
    modsMix(utils, "Mods\nMix", "Interpolate between the vibrato's modulators.", modSys6::PID::ModsMix, modulatables, modSys6::gui::ParameterType::Knob),
    dryWetMix(utils, "Mix", "Define the dry/wet ratio of the effect.", modSys6::PID::DryWetMix, modulatables, modSys6::gui::ParameterType::Knob),
    gainWet(utils, "Gain", "The output gain of the wet signal.", modSys6::PID::WetGain, modulatables, modSys6::gui::ParameterType::Knob),
    stereoConfig(utils, "StereoConfig", "Configurate if effect is applied to l/r or m/s", modSys6::PID::StereoConfig, modulatables, modSys6::gui::ParameterType::Switch),
	feedback(utils, "Feedback", "Dial in some feedback to this vibrato's delay.", modSys6::PID::Feedback, modulatables, modSys6::gui::ParameterType::Knob),

    depthModes
    {
        modSys6::gui::Button(utils, makeNotifyDepthModes(depthModes, 0), "Click here to resize the internal delay"),
        modSys6::gui::Button(utils, makeNotifyDepthModes(depthModes, 1), "Click here to resize the internal delay"),
        modSys6::gui::Button(utils, makeNotifyDepthModes(depthModes, 2), "Click here to resize the internal delay"),
        modSys6::gui::Button(utils, makeNotifyDepthModes(depthModes, 3), "Click here to resize the internal delay"),
        modSys6::gui::Button(utils, makeNotifyDepthModes(depthModes, 4), "Click here to resize the internal delay")
    },
	
    macro0Dragger(utils, modSys6::ModType::Macro, 0, modulatables),
    macro1Dragger(utils, modSys6::ModType::Macro, 1, modulatables),
    macro2Dragger(utils, modSys6::ModType::Macro, 2, modulatables),
    macro3Dragger(utils, modSys6::ModType::Macro, 3, modulatables),

    visualizer(utils, "Visualizes the sum of the vibrato's modulators.", p.getChannelCountOfBus(false, 0), 1),

    paramRandomizer(utils, modulatables, "MainRandomizer"),
    hq
    (
        utils,
        [&](int t, const void*)
        {
            if (t == modSys6::gui::NotificationType::PatchUpdated)
            {
                hq.setState(p.oversamplingEnabled.load());
                hq.repaint();
            }
            return false;
        },
        "Enable oversampling to reduce sidelobes during strong modulation."
    ),
    popUp(utils),
    enterValue(utils),

    menu(nullptr),
    menuButton
    (
        utils,
        "All the extra stuff.",
        [this]()
        {
            menu2::openMenu(menu, audioProcessor, utils, *this, layout(1, 1, 2, 3).toNearestInt(), menuButton);
        },
        [this](juce::Graphics& g, menu2::ButtonM&)
        {
            menu2::paintMenuButton(g, menuButton, utils, menu.get());
        }
    ),
    presetBrowser(utils, *p.appProperties.getUserSettings())
{
    nelLabel.font = modSys6::gui::Shared::shared.font;

    paramRandomizer.add(&stereoConfig);

    visualizer.onPaint = modSys6::gui::makeVibratoVisualizerOnPaint2();
    visualizer.onUpdate = [&p = this->audioProcessor](modSys6::gui::Buffer& b)
    {
        const auto& vals = p.visualizerValues;
        bool needsUpdate = false;
        for (auto ch = 0; ch < b.size(); ++ch)
        {
            if (b[ch][0] != vals[ch])
            {
                b[ch][0] = vals[ch];
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
    addAndMakeVisible(menuButton);

    addAndMakeVisible(visualizer);

    addAndMakeVisible(modsDepth);
    addAndMakeVisible(modsMix);
    addAndMakeVisible(dryWetMix);
    addAndMakeVisible(gainWet);
    addAndMakeVisible(stereoConfig);
	addAndMakeVisible(feedback);

    {
        const auto fs = static_cast<float>(audioProcessor.oversampling.getSampleRateUpsampled());
        const auto curDelaySizeMs = std::round(audioProcessor.vibrat.getSizeInMs(fs));

        for (auto i = 0; i < depthModes.size(); ++i)
        {
            auto& dMode = depthModes[i];
            addAndMakeVisible(dMode);

            enum { One, Four, Twenty, FourTwenty, TwoSec, NumModes };
            juce::String txt(i == 0 ? "1" : i == 1 ? "4" : i == 2 ? "20" : i == 3 ? "420" : "2k");
            dMode.onPaint = modSys6::gui::makeTextButtonOnPaint(txt, juce::Justification::centred, 1);

            dMode.setState
            (
                curDelaySizeMs == (i == 0 ? 1.f : i == 1 ? 4.f : i == 2 ? 20.f : i == 3 ? 420.f : 2000.f) ? 1 : 0
            );

            dMode.onClick = [&, i]()
            {
                const auto delaySizeMs = i == 0 ? 1.f : i == 1 ? 4.f : i == 2 ? 20.f : i == 3 ? 420.f : 2000.f;

                juce::Identifier vibDelaySizeID(vibrato::toString(vibrato::ObjType::DelaySize));
                audioProcessor.modSys.state.setProperty(vibDelaySizeID, delaySizeMs, nullptr);
                audioProcessor.forcePrepare();

                for (auto j = 0; j < depthModes.size(); ++j)
                {
                    depthModes[j].setState(i == j ? 1 : 0);
                    depthModes[j].repaint();
                }
            };
        }
    }
    

    addAndMakeVisible(macro0Dragger);
    addAndMakeVisible(macro1Dragger);
    addAndMakeVisible(macro2Dragger);
    addAndMakeVisible(macro3Dragger);

    addAndMakeVisible(popUp);
    addChildComponent(enterValue);

    addAndMakeVisible(presetBrowser);
    presetBrowser.init(this);

    setOpaque(true);
    modSys6::gui::makeCursor(*this, modSys6::gui::CursorType::Default);

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
        {
            const auto val = rand.nextFloat() > .5f;
            audioProcessor.oversamplingEnabled.store(val);
        }
        {
            const auto range = modSys6::makeRange::withCentre(.1f, 10000.f, 4.f);
            const auto val = range.convertFrom0to1(rand.nextFloat());
            const juce::Identifier id(vibrato::toString(vibrato::ObjType::DelaySize));
            audioProcessor.modSys.state.setProperty(id, val, nullptr);
        }

        audioProcessor.forcePrepare();
    });

    presetBrowser.savePatch = [this]()
    {
        juce::MessageManagerLock lock;
        audioProcessor.savePatch();
        return audioProcessor.modSys.state;
    };

    hq.onPaint = modSys6::gui::makeTextButtonOnPaint("HQ", juce::Justification::centred, 1);
    hq.onClick = [&]()
    {
        auto e = !audioProcessor.oversamplingEnabled.load();
        audioProcessor.oversamplingEnabled.store(e);
        hq.setState(e);
        hq.repaint();
    };
	hq.setState(audioProcessor.oversamplingEnabled.load());

    setResizable(true, true);
    {
        const auto user = p.appProperties.getUserSettings();
        const auto w = user->getIntValue("BoundsWidth", nelG::Width);
        const auto h = user->getIntValue("BoundsHeight", nelG::Height);
        setSize(w, h);
    }
}

void Nel19AudioProcessorEditor::resized()
{
    if (getWidth() < MinEditorBounds)
        return setBounds(0, 0, MinEditorBounds, getHeight());
    else if (getHeight() < MinEditorBounds)
        return setBounds(0, 0, getWidth(), MinEditorBounds);

    const auto thicc = modSys6::gui::Shared::shared.thicc;

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

    
    {
        const auto area = layoutMainParams(0, 0, 2, 1);
		const auto w = area.getWidth() / static_cast<float>(depthModes.size());
        const auto h = area.getHeight();
		const auto y = area.getY();
        auto x = area.getX();
		for (auto i = 0; i < depthModes.size(); ++i)
		{
			auto& depthMode = depthModes[i];
            depthMode.setBounds(juce::Rectangle<float>(x, y, w, h).toNearestInt());
			x += w;
		}
    }
    layoutMainParams.place(modsDepth, 0, 1, 2, 1);
    layoutMainParams.place(modsMix, 0, 2, 2, 1);
    layoutMainParams.place(dryWetMix, 0, 3, 2, 1);
    layoutMainParams.place(gainWet, 0, 4, 1, 1);
    layoutMainParams.place(feedback, 1, 4, 1, 1);
    layoutMainParams.place(stereoConfig, 0, 5, 1, 1, 0.f, true);
    
    layoutTopBar.place(paramRandomizer, 1, 0, 1, 1, 0.f, true);
    layoutTopBar.place(menuButton,      0, 0, 1, 1, 0.f, true);
    layoutTopBar.place(presetBrowser.getOpenCloseButton(), 2, 0, 1, 1, 0.f, true);
    layoutTopBar.place(hq,              3, 0, 1, 1, 0.f, true);
    layoutTopBar.place(nelLabel,        4, 0, 1, 1);
    {
        auto area = layoutMainParams(0, 0, 2, 1);
        area.setY(layoutTopBar.getY(0));
		area.setHeight(static_cast<float>(hq.getHeight()));

        visualizer.setBounds(area.toNearestInt());
    }

    popUp.setBounds({ 0, 0, 100, 50 });
    enterValue.setBounds({ 0, 0, 100, 50 });

    layout.place(presetBrowser, 1, 1, 2, 2, 0.f, false);

    {
        auto user = audioProcessor.appProperties.getUserSettings();
        user->setValue("BoundsWidth", getWidth());
        user->setValue("BoundsHeight", getHeight());
    }
    
    layout.place(menu.get(), 1, 1, 2, 3, 0.f, false);
}

void Nel19AudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(utils.colour(modSys6::gui::ColourID::Bg));
}

void Nel19AudioProcessorEditor::mouseEnter(const juce::MouseEvent&)
{
    utils.setTooltip(nullptr);
}

void Nel19AudioProcessorEditor::mouseDown(const juce::MouseEvent&)
{
    utils.killEnterValue();
}

/*
*/