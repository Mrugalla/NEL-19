#include "PluginProcessor.h"
#include "PluginEditor.h"
#define RemoveValueTree false
#define OversamplingEnabled true
#define DebugModsBuffer false
#define PPDHasSidechain true

Nel19AudioProcessor::BusesProps Nel19AudioProcessor::makeBusesProps()
{
    BusesProps bp;
    bp.addBus(true, "Input", ChannelSet::stereo(), true);
    bp.addBus(false, "Output", ChannelSet::stereo(), true);
#if PPDHasSidechain
    if (!juce::JUCEApplicationBase::isStandaloneApp())
        bp.addBus(true, "Sidechain", ChannelSet::stereo(), true);
#endif
    return bp;
}

Nel19AudioProcessor::Nel19AudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     :
    AudioProcessor(makeBusesProps()),
    Timer(),
    sidechain(),
    appProperties(),
    standalonePlayHead(),
    audioBufferD(),
    dryWet(),
    params(*this),
    oversampling(),
    modulators(),
    modsBuffer(),
    modType
    {
        vibrato::ModType::LFO,
        vibrato::ModType::Perlin
    },
    vibrat(),
    visualizerValues{ 0., 0. },
    depth(1.), modsMix(0.)
#endif
{
    appProperties.setStorageParameters(makeOptions());

    startTimerHz(4);
}

Nel19AudioProcessor::~Nel19AudioProcessor()
{
    auto user = appProperties.getUserSettings();
    user->setValue("firstTimeUwU", false);
    user->save();
}

bool Nel19AudioProcessor::canAddBus(bool isInput) const
{
    if (wrapperType == wrapperType_Standalone)
        return false;

    return PPDHasSidechain ? isInput : false;
}

juce::PropertiesFile::Options Nel19AudioProcessor::makeOptions()
{
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

const juce::String Nel19AudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool Nel19AudioProcessor::acceptsMidi() const
{
    return true;
}

bool Nel19AudioProcessor::producesMidi() const
{
    return false;
}

bool Nel19AudioProcessor::isMidiEffect() const
{
    return false;
}

double Nel19AudioProcessor::getTailLengthSeconds() const
{
    return 0.;
}

int Nel19AudioProcessor::getNumPrograms()
{
    return 1;
}

int Nel19AudioProcessor::getCurrentProgram()
{
    return 0;
}

void Nel19AudioProcessor::setCurrentProgram(int)
{
}

const juce::String Nel19AudioProcessor::getProgramName(int)
{
    return {};
}

void Nel19AudioProcessor::changeProgramName(int, const String&)
{}

void Nel19AudioProcessor::prepareToPlay(double sampleRate, int maxBufferSize)
{
    standalonePlayHead.prepare(sampleRate);
    
    audioBufferD.setSize(4, maxBufferSize, false, true, false);

    using PID = modSys6::PID;

    //const auto delaySizeMs = 13.;
    const auto delaySizeMs = static_cast<double>(params(PID::BufferSize).getValSumDenorm());
    const auto delaySizeD = std::round(sampleRate * delaySizeMs / 1000.);
	auto delaySize = static_cast<int>(delaySizeD);
    if (delaySize % 2 != 0)
		delaySize += 1;
    const auto delaySizeHalf = delaySize / 2;
    
    dryWet.prepare(sampleRate, maxBufferSize, delaySizeHalf);

    const auto lookaheadEnabled = params(PID::Lookahead).getValueSum() > .5f;

	auto latency = delaySizeHalf * (lookaheadEnabled ? 1 : 0);
    
    bool osEnabled = false;
#if OversamplingEnabled && !DebugModsBuffer
	osEnabled = params(PID::HQ).getValueSum() > .5f;
    oversampling.prepareToPlay(sampleRate, maxBufferSize, osEnabled);

    const auto sampleRateUpD = oversampling.getSampleRateUpsampled();
    const auto blockSizeUp = oversampling.getBlockSizeUp();
    latency += oversampling.getLatency();
#else
    const auto sampleRateUpD = sampleRate;
	const auto blockSizeUp = maxBufferSize;
#endif
    depth.prepare(sampleRate, blockSizeUp, 24.);
	modsMix.prepare(sampleRate, blockSizeUp, 24.);

    modsBuffer.setSize(2, blockSizeUp, false, true, false);
    
    for (auto m = 0; m < NumActiveMods; ++m)
        modulators[m].prepare(sampleRateUpD, blockSizeUp, latency, osEnabled ? 4 : 1);
        
    vibrat.prepare
    (
        sampleRateUpD,
        blockSizeUp,
        delaySize * (osEnabled ? 4 : 1)
    );

    setLatencySamples(latency);
}

void Nel19AudioProcessor::releaseResources()
{}

bool Nel19AudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto mono = ChannelSet::mono();
    const auto stereo = ChannelSet::stereo();

    const auto mainIn = layouts.getMainInputChannelSet();
    const auto mainOut = layouts.getMainOutputChannelSet();

    if (mainIn != mainOut)
        return false;

    if (mainOut != stereo && mainOut != mono)
        return false;

#if PPDHasSidechain
    if (wrapperType != wrapperType_Standalone)
    {
        const auto scIn = layouts.getChannelSet(true, 1);
        if (!scIn.isDisabled())
            if (scIn != mono && scIn != stereo)
                return false;
    }
#endif

    return true;
}

void Nel19AudioProcessor::processBlock(AudioBufferF& buffer, MidiBuffer& midi)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();
    audioBufferD.setSize(numChannels, numSamples, true, false, true);
    
    auto samplesD = audioBufferD.getArrayOfWritePointers();
    auto samplesF = buffer.getArrayOfWritePointers();

    for(auto ch = 0; ch < numChannels; ++ch)
        for (auto s = 0; s < numSamples; ++s)
        {
            const auto smplF = samplesF[ch][s];
            const auto smplD = static_cast<double>(smplF);
			samplesD[ch][s] = smplD;
        }
    
    processBlock(audioBufferD, midi);

    for (auto ch = 0; ch < numChannels; ++ch)
        for (auto s = 0; s < numSamples; ++s)
			samplesF[ch][s] = static_cast<float>(samplesD[ch][s]);
}

void Nel19AudioProcessor::processBlockBypassed(AudioBufferF& buffer, MidiBuffer&)
{
    auto samples = buffer.getArrayOfWritePointers();
	auto numSamples = buffer.getNumSamples();

    for (auto ch = 0; ch < buffer.getNumChannels(); ++ch)
        SIMD::clear(samples[ch], buffer.getNumSamples());

    params.processMacros();

    if (numSamples == 0)
    {
        for (auto& v : visualizerValues)
            v = 0.;
        return;
    }
}

void Nel19AudioProcessor::processBlock(AudioBufferD& buffer, MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const auto numSamples = buffer.getNumSamples();
    {
        const auto numChannelsIn = getTotalNumInputChannels();
        const auto numChannelsOut = getTotalNumOutputChannels();
        for (auto ch = numChannelsIn; ch < numChannelsOut; ++ch)
            buffer.clear(ch, 0, numSamples);
    }

    bool standalone = wrapperType == wrapperType_Standalone;
    sidechain.updateBuffers(*this, buffer, standalone);

    dsp::synthesizeTransport
    (
        getPlayHead(),
        standalonePlayHead,
        numSamples
    );

    params.processMacros();
    
    if (numSamples == 0)
    {
        for (auto& v : visualizerValues)
            v = 0.;
        return;
    }

    const auto samplesMainRead = sidechain.samplesMainRead;
    const auto numChannels = sidechain.numChannels;
    const auto dryWetMix = params(modSys6::PID::DryWetMix).getValueSum();
    const auto lookaheadEnabled = params(modSys6::PID::Lookahead).getValueSum() > .5f;
    dryWet.saveDry(samplesMainRead, dryWetMix, numChannels, numSamples, lookaheadEnabled);

    auto samplesMain = sidechain.samplesMain;
    
    const auto midSideEnabled = params(modSys6::PID::StereoConfig).getValueSum() > .5f;
    bool shallMidSide = midSideEnabled && numChannels == 2;
#if !DebugModsBuffer
    if (shallMidSide)
    {
        midSide::encode(samplesMain, numSamples);
        if (sidechain.enabled)
            midSide::encode(sidechain.samplesSC, numSamples);
        processBlockVibrato(buffer, midi, lookaheadEnabled);
        midSide::decode(samplesMain, numSamples);
    }
    else
#endif
    {
        processBlockVibrato(buffer, midi, lookaheadEnabled);
    }
    
    const auto gainWet = params(modSys6::PID::WetGain).getValSumDenorm();
    dryWet.processWet(samplesMain, gainWet, numChannels, numSamples);
}

void Nel19AudioProcessor::processBlockVibrato(AudioBufferD& bufferAll, const MidiBuffer& midi,
    bool lookaheadEnabled) noexcept
{
    
#if OversamplingEnabled && !DebugModsBuffer
    auto& buffer = oversampling.upsample(bufferAll);
    const auto osEnabled = oversampling.isEnabled();
#else
    auto& buffer = bufferAll;
#endif
    sidechain.setBufferUpsampled(&buffer);
    
    const auto numChannels = sidechain.numChannels;
    const auto numSamples = buffer.getNumSamples();
    const auto samplesMainRead = sidechain.samplesMainReadUpsampled;
    const auto samplesSCRead = sidechain.samplesSCReadUpsampled;

    using namespace modSys6;

    // SYNTHESIZE MODULATORS
    for (auto m = 0; m < NumActiveMods; ++m)
    {
        auto& mod = modulators[m];
        const auto type = modType[m];
        mod.setType(type);
        const auto offset = m * NumParamsPerMod;

        switch (type)
        {
        case vibrato::ModType::AudioRate:
            mod.setParametersAudioRate
            (
                params(withOffset(PID::AudioRate0Oct, offset)).getValSumDenorm(),
                params(withOffset(PID::AudioRate0Semi, offset)).getValSumDenorm(),
                params(withOffset(PID::AudioRate0Fine, offset)).getValSumDenorm(),
                params(withOffset(PID::AudioRate0Width, offset)).getValSumDenorm(),
                params(withOffset(PID::AudioRate0RetuneSpeed, offset)).getValSumDenorm(),
                params(withOffset(PID::AudioRate0Atk, offset)).getValSumDenorm(),
                params(withOffset(PID::AudioRate0Dcy, offset)).getValSumDenorm(),
                params(withOffset(PID::AudioRate0Sus, offset)).getValSumDenorm(),
                params(withOffset(PID::AudioRate0Rls, offset)).getValSumDenorm()
            );
            break;
        case vibrato::ModType::Perlin:
            mod.setParametersPerlin
            (
                static_cast<double>(params(withOffset(PID::Perlin0RateHz, offset)).getValSumDenorm()),
                static_cast<double>(params(withOffset(PID::Perlin0RateBeats, offset)).getValSumDenorm()),
                static_cast<double>(params(withOffset(PID::Perlin0Octaves, offset)).getValSumDenorm()),
                static_cast<double>(params(withOffset(PID::Perlin0Width, offset)).getValSumDenorm()),
                static_cast<double>(params(withOffset(PID::Perlin0Phase, offset)).getValSumDenorm()),
                static_cast<double>(params(withOffset(PID::Perlin0Bias, offset)).getValueSum()),
                perlin::Shape(std::round(params(withOffset(PID::Perlin0Shape, offset)).getValSumDenorm())),
                params(withOffset(PID::Perlin0RateType, offset)).getValueSum() > .5f
            );
            break;
        case vibrato::ModType::Dropout:
            mod.setParametersDropout
            (
                params(withOffset(PID::Dropout0Decay, offset)).getValSumDenorm(),
                params(withOffset(PID::Dropout0Spin, offset)).getValSumDenorm(),
                params(withOffset(PID::Dropout0Chance, offset)).getValSumDenorm(),
                params(withOffset(PID::Dropout0Smooth, offset)).getValSumDenorm(),
                params(withOffset(PID::Dropout0Width, offset)).getValueSum()
            );
            break;
        case vibrato::ModType::EnvFol:
            mod.setParametersEnvFol
            (
                params(withOffset(PID::EnvFol0Attack, offset)).getValSumDenorm(),
                params(withOffset(PID::EnvFol0Release, offset)).getValSumDenorm(),
                params(withOffset(PID::EnvFol0Gain, offset)).getValSumDenorm(),
                params(withOffset(PID::EnvFol0Width, offset)).getValueSum(),
                params(withOffset(PID::EnvFol0SC, offset)).getValueSum() > .5f
            );
            break;
        case vibrato::ModType::Macro:
            mod.setParametersMacro
            (
                params(withOffset(PID::Macro0, offset)).getValSumDenorm(),
                params(withOffset(PID::Macro0Smooth, offset)).getValSumDenorm(),
				params(withOffset(PID::Macro0SCGain, offset)).getValSumDenorm()
            );
            break;
        case vibrato::ModType::Pitchwheel:
            mod.setParametersPitchbend
            (
                params(withOffset(PID::Pitchbend0Smooth, offset)).getValSumDenorm()
            );
            break;
        case vibrato::ModType::LFO:
            mod.setParametersLFO
            (
                params(withOffset(PID::LFO0FreeSync, offset)).getValueSum() > .5f,
                params(withOffset(PID::LFO0RateFree, offset)).getValSumDenorm(),
                params(withOffset(PID::LFO0RateSync, offset)).getValSumDenorm(),
                params(withOffset(PID::LFO0Waveform, offset)).getValueSum(),
                params(withOffset(PID::LFO0Phase, offset)).getValSumDenorm(),
                params(withOffset(PID::LFO0Width, offset)).getValSumDenorm()
            );
            break;
        }

        mod.processBlock
        (
            samplesMainRead,
            samplesSCRead,
            midi,
            standalonePlayHead.posInfo,
            numChannels,
            numSamples
        );
    }
    
    auto modsBuf = modsBuffer.getArrayOfWritePointers();

    double* depthBuf;

    // FILL MODBUFFER WITH MODULATORS
    {
        const auto modsMixV = params(modSys6::PID::ModsMix).getValueSum();
        const auto depthV = params(modSys6::PID::Depth).getValueSum();

        auto modsMixInfo = modsMix(modsMixV, numSamples);
        auto depthInfo = depth(depthV, numSamples);
        depthBuf = depthInfo.buf;

        if (!modsMixInfo.smoothing)
            SIMD::fill(modsMixInfo.buf, modsMixV, numSamples);
        if (!depthInfo.smoothing)
            SIMD::fill(depthBuf, depthV, numSamples);

        for (auto ch = 0; ch < numChannels; ++ch)
        {
            const auto mod0 = modulators[0].buffer[ch].data();
            const auto mod1 = modulators[1].buffer[ch].data();
            auto& visualizer = visualizerValues[ch];
            auto mAll = modsBuf[ch];
            for (auto s = 0; s < numSamples; ++s)
            {
                const auto modMixed = mod0[s] + modsMixInfo.buf[s] * (mod1[s] - mod0[s]);
                const auto modGained = modMixed * depthBuf[s];
                const auto modShifted = modGained - 1.f;
                const auto modOut = modShifted + depthInfo.buf[s] * (modGained - modShifted);
                mAll[s] = modOut;
                visualizer = modGained;
            }
        }
    }

#if DebugModsBuffer
    const auto depthV = params(modSys6::PID::Depth).getValueSum();
    for (auto ch = 0; ch < numChannels; ++ch)
    {
        const auto mAll = modsBuf[ch];
        auto samples = buffer.getWritePointer(ch);
        for (auto s = 0; s < numSamples; ++s)
            samples[s] = mAll[s] * depthV;
        visualizerValues[ch] = mAll[numSamples - 1];
    }
#else
    const auto feedback = static_cast<double>(params(modSys6::PID::Feedback).getValSumDenorm());
    const auto dampHz = static_cast<double>(params(modSys6::PID::Damp).getValSumDenorm());
    vibrat
    (
        buffer.getArrayOfWritePointers(),
        numChannels,
        numSamples,
        modsBuf,
        depthBuf,
        feedback,
        dampHz,
        osEnabled ? vibrato::InterpolationType::Lerp : vibrato::InterpolationType::Spline,
        lookaheadEnabled
    );
#endif

#if OversamplingEnabled && !DebugModsBuffer
    if (osEnabled)
        oversampling.downsample(bufferAll);
#endif
}

void Nel19AudioProcessor::processBlockBypassed(AudioBufferD& buffer, MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const auto numSamples = buffer.getNumSamples();
    params.processMacros();
    if (numSamples == 0)
        return;
	auto numChannels = buffer.getNumChannels();
    auto samples = buffer.getArrayOfWritePointers();
    dryWet.processBypass
    (
        samples,
        numChannels,
        numSamples,
        params(modSys6::PID::Lookahead).getValueSum() > .5f
    );
}

bool Nel19AudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* Nel19AudioProcessor::createEditor()
{
    return new Nel19AudioProcessorEditor(*this);
}

void Nel19AudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    savePatch();
#if RemoveValueTree
    modSys.state.removeAllChildren(nullptr);
    modSys.state.removeAllProperties(nullptr);
#endif
    std::unique_ptr<juce::XmlElement> xml(params.state.createXml());
    copyXmlToBinary(*xml, destData);
}

void Nel19AudioProcessor::savePatch()
{
    params.savePatch();
    {
        const auto modTypeID = vibrato::toString(vibrato::ObjType::ModType);
        const juce::Identifier id(modTypeID);
        auto modTypeState = params.state.getChildWithName(id);
        if (!modTypeState.isValid())
        {
            modTypeState = juce::ValueTree(id);
            params.state.appendChild(modTypeState, nullptr);
        }
        for (auto m = 0; m < NumActiveMods; ++m)
        {
            const auto typeID = modTypeID + static_cast<String>(m);
			const auto typeStr = vibrato::toString(modType[m]);
            modTypeState.setProperty(typeID, typeStr, nullptr);
        }
    }
    for (auto m = 0; m < NumActiveMods; ++m)
        modulators[m].savePatch(params.state, m);
    
    params.state.setProperty("firstTimeUwU", false, nullptr);
}

void Nel19AudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(params.state.getType()))
            params.state = juce::ValueTree::fromXml(*xmlState);
    loadPatch();
#if RemoveValueTree
    modSys.state.removeAllChildren(nullptr);
    modSys.state.removeAllProperties(nullptr);
#endif
}

void Nel19AudioProcessor::loadPatch()
{
    suspendProcessing(true);
    params.loadPatch();
    {
        const auto modTypeID = vibrato::toString(vibrato::ObjType::ModType);
        const auto modTypeState = params.state.getChildWithName(modTypeID);
        if (modTypeState.isValid())
        {
            for (auto m = 0; m < NumActiveMods; ++m)
            {
                const auto propID = modTypeID + static_cast<String>(m);
                const auto typeProp = modTypeState.getProperty(propID).toString();
                for (auto i = 0; i < static_cast<int>(vibrato::ModType::NumMods); ++i)
                {
                    const auto type = static_cast<vibrato::ModType>(i);
                    if (typeProp == vibrato::toString(type))
                        modType[m] = type;
                }
            }
        }
    }
    for (auto m = 0; m < modulators.size(); ++m)
        modulators[m].loadPatch(params.state, m);
    
    prepareToPlay(getSampleRate(), getBlockSize());

    //benchmark::processBlock(*this);

    suspendProcessing(false);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Nel19AudioProcessor();
}

void Nel19AudioProcessor::timerCallback()
{
    using PID = modSys6::PID;
#if OversamplingEnabled && !DebugModsBuffer
    const bool oversamplingChanged = (params(PID::HQ).getValueSum() > .5f) != oversampling.isEnabled();
#else
	const bool oversamplingChanged = false;
#endif
	const auto bufferSize = static_cast<int>(std::round(params(PID::BufferSize).getValSumDenorm()));
    const auto bufferSizeVibrato = static_cast<int>(std::round(vibrat.getSizeInMs(oversampling.getSampleRateUpsampled())));
    //DBG(bufferSize << " :: " << bufferSizeVibrato);
    const bool bufferSizeChanged = bufferSize != bufferSizeVibrato;
    
    const auto curLatency = getLatencySamples();
    const auto latencyWithoutOversampling = curLatency - oversampling.getLatency();
    const auto hasLatency = latencyWithoutOversampling != 0;
    const bool lookaheadChanged = (params(PID::Lookahead).getValueSum() > .5f) != hasLatency;
    
    if (oversamplingChanged || lookaheadChanged || bufferSizeChanged)
        forcePrepare();
}

void Nel19AudioProcessor::forcePrepare()
{
    suspendProcessing(true);
	prepareToPlay(getSampleRate(), getBlockSize());
	suspendProcessing(false);
}

#undef RemoveValueTree
#undef OversamplingEnabled
#undef DebugModsBuffer
#undef PPDHasSidechain