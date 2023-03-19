#include "PluginProcessor.h"
#include "PluginEditor.h"

modSys6::gui::Notify makeNotify(juce::Component* comp)
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

Nel19AudioProcessorEditor::Nel19AudioProcessorEditor(Nel19AudioProcessor& p) :
    AudioProcessorEditor(&p),
    audioProcessor(p),
    layout(
        { 25, 150, 50 },
        { 50, 15, 150, 150, 30 }
    ),
    layoutMacros(
        { 1 },
        { 25, 12, 25, 12, 25, 12, 25, 12 }
    ),
    layoutMainParams(
        { 50, 80 },
        { 90, 90, 90, 40 }
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
        { 30, 30, 30, 270, 70 },
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
	
    macro0Dragger(utils, modSys6::ModType::Macro, 0, modulatables),
    macro1Dragger(utils, modSys6::ModType::Macro, 1, modulatables),
    macro2Dragger(utils, modSys6::ModType::Macro, 2, modulatables),
    macro3Dragger(utils, modSys6::ModType::Macro, 3, modulatables),

    visualizer(utils, "Visualizes the sum of the vibrato's modulators.", p.getChannelCountOfBus(false, 0), 1),

    paramRandomizer(utils, modulatables, "MainRandomizer"),

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
    addAndMakeVisible(menuButton);

    addAndMakeVisible(visualizer);

    addAndMakeVisible(modsDepth);
    addAndMakeVisible(modsMix);
    addAndMakeVisible(dryWetMix);
    addAndMakeVisible(gainWet);
    addAndMakeVisible(stereoConfig);

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
            const auto val = rand.nextFloat() > .5f ? true : false;
            audioProcessor.oversamplingEnabled.store(val);
        }
        {
            const auto range = modSys6::makeRange::biasXL(1.f, 10000.f, -.999f);
            const auto val = range.convertFrom0to1(rand.nextFloat());
            const juce::Identifier id(vibrato::toString(vibrato::ObjType::DelaySize));
            audioProcessor.modSys.state.setProperty(id, val, nullptr);
        }
        {
            const auto numTypes = static_cast<float>(vibrato::InterpolationType::NumInterpolationTypes);
            const auto val = rand.nextFloat() * (numTypes - .1f);
            const auto type = static_cast<vibrato::InterpolationType>(val);
            audioProcessor.vibrat.interpolationType.store(type);
        }

        audioProcessor.forcePrepare();
    });

    presetBrowser.savePatch = [this]()
    {
        juce::MessageManagerLock lock;
        audioProcessor.savePatch();
        return audioProcessor.modSys.state;
    };

    setBufferedToImage(true);
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
    layoutTopBar.setBounds(layout.topBar());
    layoutMacros.setBounds(layout(0, 2, 1, 2).reduced(thicc));
    layoutMainParams.setBounds(layout(2, 2, 1, 2).reduced(thicc));
    
    layoutBottomBar.place(tooltips, 0, 0, 1, 1, thicc, false);
    layoutBottomBar.place(buildDate, 1, 0, 1, 1, thicc, false);

    layoutMacros.place(macro0, 0, 0, 1, 1, thicc);
    layoutMacros.place(macro1, 0, 2, 1, 1, thicc);
    layoutMacros.place(macro2, 0, 4, 1, 1, thicc);
    layoutMacros.place(macro3, 0, 6, 1, 1, thicc);
    macro0Dragger.setQBounds(layoutMacros(0, 1, 1, 1, true).reduced(thicc));
    macro1Dragger.setQBounds(layoutMacros(0, 3, 1, 1, true).reduced(thicc));
    macro2Dragger.setQBounds(layoutMacros(0, 5, 1, 1, true).reduced(thicc));
    macro3Dragger.setQBounds(layoutMacros(0, 7, 1, 1, true).reduced(thicc));

    layoutTopBar.place(paramRandomizer, 1, 0, 1, 1, thicc, true);
    layoutTopBar.place(menuButton,      0, 0, 1, 1, thicc, true);
    layoutTopBar.place(presetBrowser.getOpenCloseButton(), 2, 0, 1, 1, thicc, true);
    layoutTopBar.place(nelLabel,        3, 0, 1, 1, thicc, false);
    layoutTopBar.place(visualizer,      4, 0, 1, 1, thicc, false);

    popUp.setBounds({ 0, 0, 100, 50 });
    enterValue.setBounds({ 0, 0, 100, 50 });

    layout.place(modComps[0], 1, 2, 1, 1, thicc, false);
    layout.place(modComps[1], 1, 3, 1, 1, thicc, false);
    
    layoutMainParams.place(modsDepth,    0, 0, 2, 1, thicc, true);
    layoutMainParams.place(modsMix,      0, 1, 2, 1, thicc, true);
    layoutMainParams.place(dryWetMix,    1, 2, 1, 1, thicc, true);
    layoutMainParams.place(gainWet,      0, 2, 1, 1, thicc, true);
    layoutMainParams.place(stereoConfig, 0, 3, 2, 1, thicc, true);

    layout.place(presetBrowser, 1, 1, 2, 3, thicc, false);

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
    modSys6::gui::visualizeGroup
    (
        g,
        "Mods",
        layout(1, 1, 1, 3, 0.f, false),
        utils.colour(modSys6::gui::ColourID::Hover),
        modSys6::gui::Shared::shared.thicc
    );
    modSys6::gui::visualizeGroup
    (
        g,
        "Main",
        layout(2, 1, 1, 3, 0.f, false),
        utils.colour(modSys6::gui::ColourID::Hover),
        modSys6::gui::Shared::shared.thicc,
        false, false
    );
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