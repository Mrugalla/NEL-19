#include "PluginProcessor.h"
#include "PluginEditor.h"
#define RemoveValueTree false

Nel19AudioProcessor::Nel19AudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
    appProperties(),
    modRateRanges(),
    midiLearn(),
    midiSignal(),
    apvts(*this, nullptr, "parameters", param::createParameters(apvts, modRateRanges)),

    oversampling(getTotalNumInputChannels(), this),

    matrix(apvts),
    mtrxParams(),
    modsIDs(),
    modulatorsID("ModulatorsIdx"),

    midSideProcessor(getTotalNumInputChannels()),
    vibDelay(), vibDelayUp(),
    vibrato(this, vibDelayUp, matrix->getParameter(param::getID(param::ID::DryWetMix))->data(), getTotalNumInputChannels()),
    vibDelayVisualizerValue(),
    mutex()
#endif
{
    for (auto& ml : midiLearn) ml.store(false);
    for (auto& m : midiSignal) m = .5f;

    vibDelayVisualizerValue.resize(getTotalNumInputChannels(), 0.f);

    appProperties.setStorageParameters(makeOptions());

    bool isMono = getTotalNumInputChannels() == 1;

    // activate all used non-modulator parameters
    matrix->activateParameter(param::getID(param::ID::Depth), true);
    matrix->activateParameter(param::getID(param::ID::ModulatorsMix), true);
    matrix->activateParameter(param::getID(param::ID::DryWetMix), true);
    matrix->activateParameter(param::getID(param::ID::Voices), true);
    if(!isMono)
        matrix->activateParameter(param::getID(param::ID::StereoConfig), true);
    
    // add and activate all macro mods and params
    auto macro = matrix->addMacroModulator(param::getID(param::ID::Macro0));
    macro->setActive(true);
    macro = matrix->addMacroModulator(param::getID(param::ID::Macro1));
    macro->setActive(true);
    macro = matrix->addMacroModulator(param::getID(param::ID::Macro2));
    macro->setActive(true);
    macro = matrix->addMacroModulator(param::getID(param::ID::Macro3));
    macro->setActive(true);

    // prepare wavetables for lfos
    const auto numTables = static_cast<size_t>(apvts.getParameter(param::getID(param::ID::LFOWaveTable0))->getNormalisableRange().end + 1.f);
    constexpr int samplesPerCycle = 1 << 11;
    std::vector<std::function<float(float)>> wavetables;
    wavetables.resize(numTables);
    enum wtIdx { Sin, Tri, Sqr, Saw };
    wavetables[Sin] = [t = modSys2::tau](float x) { return .5f * std::sin(x * t) + .5f; };
    wavetables[Tri] = [p = modSys2::pi](float x) {
        return x < .25f ? 2.f * x + .5f :
            x < .75f ? -2.f * (x - .75f) :
            2.f * (x - .75f);
    };
    wavetables[Sqr] = [p = modSys2::pi](float x) {
        const auto K = x < .5f ? 0.f : 1.f;
        const auto W = .5f - .5f * std::cos(4.f * p * x);
        auto sum = 0.f;
        const auto N = 12.f;
        for (auto n = 1.f; n < N; ++n)
            sum += std::fmod(n, 2.f) * std::sin(2.f * x * n * p) / n;
        sum *= 1.69f / p;
        const auto S = .5f - sum;
        return (1.f - W) * S + W * K;
    };
    wavetables[Saw] = [p = modSys2::pi](float x) {
        const auto tau = p * 2.f;
        const auto N = 7.f;
        auto sum = 0.f;
        for (auto n = 1.f; n <= N; ++n)
            sum += std::sin(tau * x * n) / n;
        sum *= .95f / p;
        sum += .5f;
        sum = 1.f - sum;
        const auto window = .5f + .5f * std::cos(tau * x);
        return x + window * (sum - x);
    };
    matrix->setWavetables(wavetables, samplesPerCycle);

    // add all other modulators and their parameters
    // add all other modulators and their parameters
    modsIDs[0].push_back(matrix->addEnvelopeFollowerModulator(
        param::getID(param::ID::EnvFolGain0),
        param::getID(param::ID::EnvFolAtk0),
        param::getID(param::ID::EnvFolRls0),
        param::getID(param::ID::EnvFolBias0),
        param::getID(param::ID::EnvFolWidth0),
        0
    )->id);
    modsIDs[0].push_back(matrix->addLFOModulator(
        param::getID(param::ID::LFOSync0),
        param::getID(param::ID::LFORate0),
        param::getID(param::ID::LFOWidth0),
        param::getID(param::ID::LFOWaveTable0),
        param::getID(param::ID::LFOPolarity0),
        param::getID(param::ID::LFOPhase0),
        modRateRanges,
        0
    )->id);
    modsIDs[0].push_back(matrix->addRandomModulator(
        param::getID(param::ID::RandSync0),
        param::getID(param::ID::RandRate0),
        param::getID(param::ID::RandBias0),
        param::getID(param::ID::RandWidth0),
        param::getID(param::ID::RandSmooth0),
        modRateRanges,
        0
    )->id);
    modsIDs[0].push_back(matrix->addPerlinModulator(
        param::getID(param::ID::PerlinRate0),
        param::getID(param::ID::PerlinOctaves0),
        param::getID(param::ID::PerlinWidth0),
        modRateRanges,
        param::PerlinMaxOctaves,
        0
    )->id);
    modsIDs[0].push_back(matrix->addMIDIPitchbendModulator(
        midiSignal[0], 0
    )->id);
    modsIDs[0].push_back(matrix->addNoteModulator(
        param::getID(param::ID::NoteOct0),
        param::getID(param::ID::NoteSemi0),
        param::getID(param::ID::NoteFine0),
        param::getID(param::ID::NotePhaseDist0),
        param::getID(param::ID::NoteRetune0),
        0
    )->id);

    modsIDs[1].push_back(matrix->addEnvelopeFollowerModulator(
        param::getID(param::ID::EnvFolGain1),
        param::getID(param::ID::EnvFolAtk1),
        param::getID(param::ID::EnvFolRls1),
        param::getID(param::ID::EnvFolBias1),
        param::getID(param::ID::EnvFolWidth1),
        1
    )->id);
    modsIDs[1].push_back(matrix->addLFOModulator(
        param::getID(param::ID::LFOSync1),
        param::getID(param::ID::LFORate1),
        param::getID(param::ID::LFOWidth1),
        param::getID(param::ID::LFOWaveTable1),
        param::getID(param::ID::LFOPolarity1),
        param::getID(param::ID::LFOPhase1),
        modRateRanges,
        1
    )->id);
    modsIDs[1].push_back(matrix->addRandomModulator(
        param::getID(param::ID::RandSync1),
        param::getID(param::ID::RandRate1),
        param::getID(param::ID::RandBias1),
        param::getID(param::ID::RandWidth1),
        param::getID(param::ID::RandSmooth1),
        modRateRanges,
        1
    )->id);
    modsIDs[1].push_back(matrix->addPerlinModulator(
        param::getID(param::ID::PerlinRate1),
        param::getID(param::ID::PerlinOctaves1),
        param::getID(param::ID::PerlinWidth1),
        modRateRanges,
        param::PerlinMaxOctaves,
        1
    )->id);
    modsIDs[1].push_back(matrix->addMIDIPitchbendModulator(
        midiSignal[1], 1
    )->id);
    modsIDs[1].push_back(matrix->addNoteModulator(
        param::getID(param::ID::NoteOct1),
        param::getID(param::ID::NoteSemi1),
        param::getID(param::ID::NoteFine1),
        param::getID(param::ID::NotePhaseDist1),
        param::getID(param::ID::NoteRetune1),
        1
    )->id);

    for (auto i = 0; i < modsIDs.size(); ++i)
        for (auto j = 0; j < modsIDs[i].size(); ++j) {
            auto md = matrix->getModulator(modsIDs[i][j]);
            juce::String mdID = "vd" + juce::String(i) + "_" + juce::String(j);
            if(modsIDs[i][j].toString().contains("EnvFol"))
                md->addDestination(mdID, vibDelay[i], 1.f, false);
            else
                md->addDestination(mdID, vibDelay[i], 1.f, true);
        }

    // activate default modulators and their parameters
    matrix->setModulatorActive(modsIDs[0][Mods::Perlin], true);
    matrix->setModulatorActive(modsIDs[1][Mods::LFO], true);

    // get parameter indexes for later accessing their values
    mtrxParams.resize(PID::EnumSize);
    mtrxParams[PID::Depth] = matrix->getParameterIndex(param::getID(param::ID::Depth));
    mtrxParams[PID::ModsMix] = matrix->getParameterIndex(param::getID(param::ID::ModulatorsMix));
    mtrxParams[PID::StereoConfig] = matrix->getParameterIndex(param::getID(param::ID::StereoConfig));
}

juce::PropertiesFile::Options Nel19AudioProcessor::makeOptions() {
    juce::PropertiesFile::Options options;
    options.applicationName = JucePlugin_Name;
    options.filenameSuffix = ".settings";
    options.folderName = "Mrugalla" + juce::File::getSeparatorString() + JucePlugin_Name;
    options.osxLibrarySubFolder = "Application Support";
    options.commonToAllUsers = false;
    options.ignoreCaseOfKeyNames = true;
    options.doNotSave = false;
    options.millisecondsBeforeSaving = 0;
    options.storageFormat = juce::PropertiesFile::storeAsXML;
    return options;
}

const juce::String Nel19AudioProcessor::getName() const { return JucePlugin_Name; }
bool Nel19AudioProcessor::acceptsMidi() const{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}
bool Nel19AudioProcessor::producesMidi() const {
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}
bool Nel19AudioProcessor::isMidiEffect() const {
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}
double Nel19AudioProcessor::getTailLengthSeconds() const { return 0.; }
int Nel19AudioProcessor::getNumPrograms() { return 1; }
int Nel19AudioProcessor::getCurrentProgram() { return 0; }
void Nel19AudioProcessor::setCurrentProgram (int) {}
const juce::String Nel19AudioProcessor::getProgramName (int) { return {}; }
void Nel19AudioProcessor::changeProgramName (int, const juce::String&){}
void Nel19AudioProcessor::prepareToPlay(double sampleRate, int maxBufferSize) {
    const auto channelCount = getTotalNumInputChannels();
    
    const auto sampleRateDown = sampleRate;
    const auto maxBufferSizeDown = maxBufferSize;

    oversampling.prepareToPlay(sampleRate, maxBufferSize);
    sampleRate = oversampling.getSampleRateUpsampled();
    maxBufferSize = oversampling.getBlockSizeUp();
    auto latency = oversampling.getLatency();

    const auto sec = static_cast<float>(sampleRate);
    const auto slow = sec / 4.f;
    const auto medium = sec / 16.f;
    const auto quick = sec / 64.f;
    const auto instant = 0.f;

    for (auto& vd : vibDelay)
        vd.setSize(channelCount, maxBufferSizeDown);
    vibDelayUp.setSize(channelCount, maxBufferSize);

    vibrato.prepareToPlay(maxBufferSize);
    
    auto user = appProperties.getUserSettings();

    juce::String vibDelaySizeID("vibDelaySize");
    auto vibDelaySize = static_cast<float>(apvts.state.getProperty(vibDelaySizeID, "-1"));
    if(vibDelaySize <= 0.f)
        vibDelaySize = static_cast<float>(user->getDoubleValue(vibDelaySizeID, 4.));
    const size_t vds = static_cast<size_t>(vibDelaySize * sec / 1000.f);
    vibrato.resizeDelay(vds);
    latency += vibrato.getLatency();
    
    juce::String vibInterpolationID("vibInterpolation");
    auto vibInterpolation = static_cast<int>(apvts.state.getProperty(vibInterpolationID, "-1"));
    if (vibInterpolation < 0.f)
        vibInterpolation = user->getIntValue(vibInterpolationID, vibrato::InterpolationType::Spline);
    vibrato.setInterpolationType(static_cast<vibrato::InterpolationType>(vibInterpolation));

    auto m = matrix.getCopyOfUpdatedPtr();
    m->prepareToPlay(
        channelCount,
        maxBufferSizeDown,
        sampleRateDown,
        latency
    );

    m->setSmoothingLengthInSamples(param::getID(param::ID::Macro0), quick);
    m->setSmoothingLengthInSamples(param::getID(param::ID::Macro1), quick);
    m->setSmoothingLengthInSamples(param::getID(param::ID::Macro2), quick);
    m->setSmoothingLengthInSamples(param::getID(param::ID::Macro3), quick);
    m->setSmoothingLengthInSamples(param::getID(param::ID::Depth), medium);
    m->setSmoothingLengthInSamples(param::getID(param::ID::ModulatorsMix), medium);
    m->setSmoothingLengthInSamples(param::getID(param::ID::DryWetMix), quick);
    m->setSmoothingLengthInSamples(param::getID(param::ID::Voices), instant);
    m->setSmoothingLengthInSamples(param::getID(param::ID::StereoConfig), instant);
    m->setSmoothingLengthInSamples(param::getID(param::ID::LFOPhase0), medium);
    m->setSmoothingLengthInSamples(param::getID(param::ID::LFOPhase1), medium);
    
    matrix.replaceUpdatedPtrWith(m);

    setLatencySamples(latency);
}
void Nel19AudioProcessor::releaseResources() {}
bool Nel19AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const {
    return
        (layouts.getMainInputChannelSet() == juce::AudioChannelSet::disabled()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::disabled())

        ||

        (layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo());
}

void Nel19AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    if (!processBlockReady(buffer)) return;
    midSideProcessor.setEnabled(matrix->getParameterValue(mtrxParams[StereoConfig]));
    midSideProcessor.processBlockEncode(buffer);
    for (auto& vb : vibDelay)
        vb.clear(0, buffer.getNumSamples());
    const auto mtrx = processBlockModSys(buffer, midi);
    processBlockVibDelay(buffer, mtrx);
    {
        auto bufferUp = oversampling.upsample(buffer);
        if (bufferUp == nullptr) return;

        if (oversampling.isEnabled()) {
            for (auto ch = 0; ch < buffer.getNumChannels(); ++ch) {
                const auto vb = vibDelay[0].getReadPointer(ch);
                auto vbUp = vibDelayUp.getWritePointer(ch);
                for (auto s = 0; s < buffer.getNumSamples(); ++s) {
                    const auto idx = oversampling::MaxOrder * s;
                    for (auto o = 0; o < oversampling::MaxOrder; ++o)
                        vbUp[idx + o] = vb[s];
                }
            }
        }
        else
            for (auto ch = 0; ch < buffer.getNumChannels(); ++ch) {
                const auto vb = vibDelay[0].getReadPointer(ch);
                auto vbUp = vibDelayUp.getWritePointer(ch);
                juce::FloatVectorOperations::copy(vbUp, vb, buffer.getNumSamples());
            }

        vibrato.processBlock(*bufferUp);
        oversampling.downsample(buffer);
    }
    midSideProcessor.processBlockDecode(buffer);
}
void Nel19AudioProcessor::processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    
}
void Nel19AudioProcessor::processBlockEmpty() {
    
}
bool Nel19AudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* Nel19AudioProcessor::createEditor() {
    return new Nel19AudioProcessorEditor (*this);
}

void Nel19AudioProcessor::getStateInformation (juce::MemoryBlock& destData) {
    juce::ScopedLock lock(mutex);
    matrix->getState(apvts);
    auto state = apvts.state;
    auto modulatorsChild = state.getChildWithName(modulatorsID);
    if (!modulatorsChild.isValid()) {
        modulatorsChild = juce::ValueTree(modulatorsID);
        state.appendChild(modulatorsChild, nullptr);
    }
    for (const auto& modsID : modsIDs)
        for (const auto& modID : modsID) {
            auto mod = matrix->getModulator(modID);
            int active = mod->isActive() ? 1 : 0;
            //DBG(modID.toString() << ": " << active);
            modulatorsChild.setProperty(modID, active, nullptr);
        }
    //DBG(state.toXmlString());

#if RemoveValueTree
    apvts.state.removeAllChildren(nullptr);
    apvts.state.removeAllProperties(nullptr);
#endif
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}
void Nel19AudioProcessor::setStateInformation (const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));

    juce::ScopedLock lock(mutex);
    auto m = matrix.getCopyOfUpdatedPtr();
    auto state = apvts.state;
    auto modulatorsChild = state.getChildWithName(modulatorsID);
    if (!modulatorsChild.isValid()) {
        modulatorsChild = juce::ValueTree(modulatorsID);
        state.appendChild(modulatorsChild, nullptr);
    }
    for (const auto& modsID : modsIDs)
        for (const auto& modID : modsID) {
            auto mActive = static_cast<int>(modulatorsChild.getProperty(modID, -1));
            if(mActive != -1)
                m->setModulatorActive(modID, mActive);
        }
#if RemoveValueTree
    apvts.state.removeAllChildren(nullptr);
    apvts.state.removeAllProperties(nullptr);
#endif
    m->setState(apvts);
    matrix.replaceUpdatedPtrWith(m);
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Nel19AudioProcessor(); }

/////////// PROCESS BLOCK EXTRA STEPS ///////////////////
bool Nel19AudioProcessor::processBlockReady(juce::AudioBuffer<float>& buffer)  {
    if (buffer.getNumSamples() == 0) {
        processBlockEmpty();
        return false;
    }  
    juce::ScopedNoDenormals noDenormals;
    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());
    return true;
}
const std::shared_ptr<modSys2::Matrix> Nel19AudioProcessor::processBlockModSys(juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi) {
    auto mtrx = matrix.updateAndLoadCurrentPtr();
    mtrx->processBlock(buffer, getPlayHead(), midi);
    return mtrx;
}
void Nel19AudioProcessor::processBlockVibDelay(juce::AudioBuffer<float>& buffer, const std::shared_ptr<modSys2::Matrix>& mtrx)  {
    auto vd0 = vibDelay[0].getArrayOfWritePointers();
    const auto vd1 = vibDelay[1].getArrayOfReadPointers();
    const auto back = buffer.getNumSamples() - 1;
    for (auto ch = 0; ch < buffer.getNumChannels(); ++ch) {
        for (auto s = 0; s < buffer.getNumSamples(); ++s) {
            const auto mixV = mtrx->getParameterValue(mtrxParams[PID::ModsMix], s);
            const auto depthV = mtrx->getParameterValue(mtrxParams[PID::Depth], s);

            const auto val = vd0[ch][s] + mixV * (vd1[ch][s] - vd0[ch][s]);
            vd0[ch][s] = juce::jlimit(
                -1.f + std::numeric_limits<float>::epsilon(),
                1.f - std::numeric_limits<float>::epsilon(),
                val * depthV
            );
        }
        const auto lastVal = vd0[ch][back];
        vibDelayVisualizerValue[ch].set(lastVal);
    }
}
/////////////////////////////////////////////////////////

/*

processBlockBypassed has some issue idk

if plugin didn't get music for a while
    parameters don't repaint correctly
    improve processBlockEmpty method!

*/