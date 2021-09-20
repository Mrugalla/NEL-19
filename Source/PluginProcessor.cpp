#include "PluginProcessor.h"
#include "PluginEditor.h"
#define RemoveValueTree false
#include <chrono>
#include <thread>

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
    apvts(*this, nullptr, "parameters", param::createParameters(apvts, modRateRanges)),
    matrix(apvts),
    mtrxParams(),
    modsIDs(),
    modulatorsID("ModulatorsIdx"),

    midSideProcessor(getChannelCountOfBus(false, 0)),

    vibDelay(),
    vibrato(vibDelay[0], matrix->getParameter(param::getID(param::ID::DryWetMix))->data(), getChannelCountOfBus(false, 0)),
    vibDelayVisualizerValue(0.f),
    mutex()
#endif
{
    appProperties.setStorageParameters(makeOptions());

    bool isMono = getChannelCountOfBus(false, 0) == 1;

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
    const auto channelCount = getChannelCountOfBus(false, 0);
    
    const auto sec = static_cast<float>(sampleRate);
    const auto slow = sec / 4.f;
    const auto medium = sec / 16.f;
    const auto quick = sec / 64.f;
    const auto instant = 0.f;

    vibrato.prepareToPlay(maxBufferSize);
    for(auto& vd: vibDelay)
        vd.setSize(channelCount, maxBufferSize);
    
    auto user = appProperties.getUserSettings();
    const auto vibDelaySize = user->getDoubleValue("vibDelaySize", 4.);
    const size_t vds = size_t(vibDelaySize * sec / 1000.f);
    vibrato.resizeDelay(*this, vds);
    const auto vdLatency = vds / 2;
    
    auto m = matrix.getCopyOfUpdatedPtr();
    m->prepareToPlay(
        channelCount,
        maxBufferSize,
        sampleRate,
        vdLatency
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
    if (buffer.getNumSamples() == 0)
        return processBlockEmpty();
    if (!processBlockReady(buffer)) return;
    midSideProcessor.setEnabled(matrix->getParameterValue(mtrxParams[StereoConfig]));
    midSideProcessor.processBlockEncode(buffer);
    const auto mtrx = processBlockModSys(buffer);
    processBlockVibDelay(buffer, mtrx);
    midSideProcessor.processBlockDecode(buffer);
}
void Nel19AudioProcessor::processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    if (buffer.getNumSamples() == 0)
        return processBlockEmpty();
    if (!processBlockReady(buffer)) return;
    const auto mtrx = processBlockModSys(buffer);
    auto vd0 = vibDelay[0].getArrayOfWritePointers();
    for(auto ch = 0; ch < buffer.getNumChannels(); ++ch)
        juce::FloatVectorOperations::fill(vd0[ch], 0.f, buffer.getNumSamples());
    vibDelayVisualizerValue.store(0.f);
    vibrato.processBlock(*this, buffer);
}
void Nel19AudioProcessor::processBlockEmpty() {
    juce::ScopedNoDenormals noDenormals;
    const auto numSamples = 1;
    for (auto& vb : vibDelay)
        vb.clear(0, numSamples);
    auto mtrx = matrix.updateAndLoadCurrentPtr();
    mtrx->processBlockEmpty();
    vibDelayVisualizerValue.store(0.f);
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
            //DBG(modID.toString() << ": " << mActive);
            if(mActive != -1)
                m->setModulatorActive(modID, mActive);
        } 
    //DBG(state.toXmlString());
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
    juce::ScopedNoDenormals noDenormals;
    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());
    for (auto& vb : vibDelay)
        vb.clear(0, buffer.getNumSamples());
    return true;
}
const std::shared_ptr<modSys2::Matrix> Nel19AudioProcessor::processBlockModSys(juce::AudioBuffer<float>& buffer) {
    auto mtrx = matrix.updateAndLoadCurrentPtr();
    mtrx->processBlock(buffer, getPlayHead());
    return mtrx;
}
void Nel19AudioProcessor::processBlockVibDelay(juce::AudioBuffer<float>& buffer, const std::shared_ptr<modSys2::Matrix>& mtrx)  {
    auto vd0 = vibDelay[0].getArrayOfWritePointers();
    const auto vd1 = vibDelay[1].getArrayOfReadPointers();
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
    }
    const auto lastVal = vd0[0][buffer.getNumSamples() - 1];
    vibDelayVisualizerValue.store(lastVal);
    vibrato.processBlock(*this, buffer);
}
/////////////////////////////////////////////////////////

/*

processBlockBypassed has some issue idk

if plugin didn't get music for a while
    parameters don't repaint correctly
    improve processBlockEmpty method!

*/