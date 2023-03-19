#include "PluginProcessor.h"
#include "PluginEditor.h"
#define RemoveValueTree false
#define OversamplingEnabled true
#define DebugModsBuffer false

Nel19AudioProcessor::Nel19AudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     :
    AudioProcessor
    (
        BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
    ),
    Timer(),
    appProperties(),
    dryWet(),
    modSys(*this, [this]() { loadPatch(); }),
    oversampling(),
    oversamplingEnabled(false),
    modulators
    {
        vibrato::Modulator(modSys.getBeatsData()),
        vibrato::Modulator(modSys.getBeatsData())
    },
    modsBuffer(),
    modType(),
    vibrat(),
    visualizerValues(),
    lookaheadEnabled(true),
    depthSmooth(0.f), modsMixSmooth(0.f),
    depthBuf(), modsMixBuf()
#endif
{
    appProperties.setStorageParameters(makeOptions());
    auto user = appProperties.getUserSettings();

    { // MAKE PRESETS
        auto file = user->getFile();
        file = file.getParentDirectory();
        file = file.getChildFile("Presets");
        if (!file.exists())
            file.createDirectory();
        {
            const auto make = [&f = file](juce::String name, const char* data, const int size)
            {
                const auto txt = juce::String::fromUTF8(data, size);
                auto nFile = f.getChildFile(name + ".nel");
                if (nFile.existsAsFile())
                    nFile.deleteFile();
                nFile.create();
                nFile.appendText(txt, false, false);
            };

            make("AudioRate Arp (midi)", BinaryData::AudioRate_Arp_midi_nel, BinaryData::AudioRate_Arp_midi_nelSize);
            make("Broken Tape", BinaryData::Broken_Tape_nel, BinaryData::Broken_Tape_nelSize);
            make("Dream Arp EnvFol", BinaryData::Dream_Arp_EnvFol_nel, BinaryData::Dream_Arp_EnvFol_nelSize);
            make("Flanger", BinaryData::Flanger_nel, BinaryData::Flanger_nelSize);
            make("Init", BinaryData::Init_nel, BinaryData::Init_nelSize);
            make("Psychosis", BinaryData::Psychosis_nel, BinaryData::Psychosis_nelSize);
            make("Resample", BinaryData::Resample_nel, BinaryData::Resample_nelSize);
            make("Shoegaze", BinaryData::Shoegaze_nel, BinaryData::Shoegaze_nelSize);
            make("Sines", BinaryData::Sines_nel, BinaryData::Sines_nelSize);
            make("Thicc", BinaryData::Thicc_nel, BinaryData::Thicc_nelSize);
        }
    }

    visualizerValues.resize(2, 0.f);

    modType[0] = vibrato::ModType::Perlin;
    modType[1] = vibrato::ModType::LFO;
    
    {
        const auto defVal = vibrato::toString(vibrato::InterpolationType::Lerp);
        const auto id = vibrato::toString(vibrato::ObjType::InterpolationType);
        const auto idType = user->getValue(id, defVal);
        const auto type = vibrato::toType(idType);
        vibrat.interpolationType.store(type);
    }
    {
        const auto id = getLookaheadID();
        const auto e = user->getBoolValue(id, true);
    }
    {
        std::array<vibrato::ModType, NumActiveMods> defVals
        {
            vibrato::ModType::Perlin,
            vibrato::ModType::LFO
        };
        const auto objType = vibrato::ObjType::ModType;
        for (auto m = 0; m < NumActiveMods; ++m)
        {
            const auto objStr = vibrato::with(objType, m);
            const juce::Identifier id(objStr);
            const auto mID = user->getValue(id, vibrato::toString(defVals[m]));
            const auto type = vibrato::getModType(mID);
            modType[m] = type;
        }
    }

    startTimerHz(4);
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

void Nel19AudioProcessor::changeProgramName(int, const juce::String&)
{}

void Nel19AudioProcessor::prepareToPlay(double sampleRate, int maxBufferSize)
{
    auto sampleRateF = static_cast<float>(sampleRate);

    auto user = appProperties.getUserSettings();

    float dSize;
    {
        static constexpr double defaultDlySize = 13.;
        const juce::String id(vibrato::toString(vibrato::ObjType::DelaySize));
        dSize = static_cast<float>(modSys.state.getProperty(id, -1.f));
        if (dSize <= 0.f || std::isnan(dSize) || std::isinf(dSize))
            dSize = static_cast<float>(user->getDoubleValue(id, defaultDlySize));
    }
    const auto vibSizeSamplesHalf = static_cast<int>(std::rint(sampleRateF * dSize * .001f * .5f));
    
    dryWet.prepare(sampleRateF, maxBufferSize, vibSizeSamplesHalf);

    const auto lGate = lookaheadEnabled.load() ? 1 : 0;
    auto latency = vibSizeSamplesHalf;
#if OversamplingEnabled
    const auto osEnabled = oversamplingEnabled.load();
    oversampling.prepareToPlay(sampleRate, maxBufferSize, osEnabled);

    sampleRate = oversampling.getSampleRateUpsampled();
    maxBufferSize = oversampling.getBlockSizeUp();
    latency += oversampling.getLatency();

    sampleRateF = static_cast<float>(sampleRate);
#endif
    latency *= lGate;
    depthSmooth.makeFromDecayInMs(24.f, sampleRateF);
    modsMixSmooth.makeFromDecayInMs(24.f, sampleRateF);
    depthBuf.resize(maxBufferSize);
    modsMixBuf.resize(maxBufferSize);

    modsBuffer.setSize(2, maxBufferSize, false, true, false);
    
    for (auto m = 0; m < NumActiveMods; ++m)
        modulators[m].prepare(sampleRateF, maxBufferSize, latency);
        
    // UPDATE LFO WAVETABLE
    const auto vds = static_cast<int>(sampleRateF * dSize * .001f);
    vibrat.prepare(maxBufferSize, vds);
    {
        const auto id = vibrato::toString(vibrato::ObjType::InterpolationType);
        const auto typeStr = modSys.state.getProperty(id, "").toString();
        if (typeStr.isNotEmpty())
        {
            const auto type = vibrato::toType(typeStr);
            vibrat.interpolationType.store(type);
        }
    }

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

    return true;
}

void Nel19AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const auto numSamples = buffer.getNumSamples();
    {
        const auto numChannelsIn = getTotalNumInputChannels();
		const auto numChannelsOut = getTotalNumOutputChannels();
        for (auto ch = numChannelsIn; ch < numChannelsOut; ++ch)
            buffer.clear(ch, 0, numSamples);
    }
	const auto numChannels = buffer.getNumChannels();
    const auto playHead = getPlayHead();
    
    modSys.processBlock(numSamples, playHead);
    
    if (numSamples == 0)
    {
        for (auto& v : visualizerValues)
            v = 0.f;
        return;
    }

    const auto samplesRead = buffer.getArrayOfReadPointers();
    auto samples = buffer.getArrayOfWritePointers();

	const auto dryWetMix = modSys.getParam(modSys6::PID::DryWetMix)->getValueSum();
    dryWet.saveDry(samplesRead, dryWetMix, numChannels, numSamples);

    
#if !DebugModsBuffer
    const auto midSideEnabled = modSys.getParam(modSys6::PID::StereoConfig)->getValueSum() > .5f;
    if (midSideEnabled && numChannels == 2)
    {
        midSide::encode(samples, numSamples);
        processBlockVibrato(buffer, midi);
        midSide::decode(samples, numSamples);
    }
    else
#endif
    processBlockVibrato(buffer, midi);

    const auto gainWet = modSys.getParam(modSys6::PID::WetGain)->getValSumDenorm();
    dryWet.processWet(samples, gainWet, numChannels, numSamples);
}

void Nel19AudioProcessor::processBlockVibrato(juce::AudioBuffer<float>& bufferOut, const juce::MidiBuffer& midi) noexcept
{
    const auto numChannels = bufferOut.getNumChannels();
#if OversamplingEnabled
    auto& buffer = oversampling.upsample(bufferOut);
#endif
    const auto samplesRead = buffer.getArrayOfReadPointers();
    const auto numSamples = buffer.getNumSamples();

    auto curPlayHead = getPlayHead();
    using namespace modSys6;
    
    // PROCESS MODULATORS
    for(auto m = 0; m < NumActiveMods; ++m)
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
                modSys.getParam(withOffset(PID::AudioRate0Oct, offset))->getValSumDenorm(),
                modSys.getParam(withOffset(PID::AudioRate0Semi, offset))->getValSumDenorm(),
                modSys.getParam(withOffset(PID::AudioRate0Fine, offset))->getValSumDenorm(),
                modSys.getParam(withOffset(PID::AudioRate0Width, offset))->getValSumDenorm(),
                modSys.getParam(withOffset(PID::AudioRate0RetuneSpeed, offset))->getValSumDenorm(),
                modSys.getParam(withOffset(PID::AudioRate0Atk, offset))->getValSumDenorm(),
                modSys.getParam(withOffset(PID::AudioRate0Dcy, offset))->getValSumDenorm(),
                modSys.getParam(withOffset(PID::AudioRate0Sus, offset))->getValSumDenorm(),
                modSys.getParam(withOffset(PID::AudioRate0Rls, offset))->getValSumDenorm()
            );
            break;
        case vibrato::ModType::Perlin:
            mod.setParametersPerlin
            (
                static_cast<double>(modSys.getParam(withOffset(PID::Perlin0RateHz, offset))->getValSumDenorm()),
                static_cast<double>(modSys.getParam(withOffset(PID::Perlin0RateBeats, offset))->getValSumDenorm()),
                modSys.getParam(withOffset(PID::Perlin0Octaves, offset))->getValSumDenorm(),
                modSys.getParam(withOffset(PID::Perlin0Width, offset))->getValSumDenorm(),
                modSys.getParam(withOffset(PID::Perlin0Phase, offset))->getValSumDenorm(),
				perlin::Shape(std::round(modSys.getParam(withOffset(PID::Perlin0Shape, offset))->getValSumDenorm())),
                modSys.getParam(withOffset(PID::Perlin0RateType, offset))->getValueSum() > .5f,
				modSys.getParam(withOffset(PID::Perlin0RandType, offset))->getValSumDenorm() > .5f
            );
            break;
        case vibrato::ModType::Dropout:
            mod.setParametersDropout
            (
                modSys.getParam(withOffset(PID::Dropout0Decay, offset))->getValSumDenorm(),
                modSys.getParam(withOffset(PID::Dropout0Spin, offset))->getValSumDenorm(),
                modSys.getParam(withOffset(PID::Dropout0Chance, offset))->getValSumDenorm(),
                modSys.getParam(withOffset(PID::Dropout0Smooth, offset))->getValSumDenorm(),
                modSys.getParam(withOffset(PID::Dropout0Width, offset))->getValueSum()
            );
            break;
        case vibrato::ModType::EnvFol:
            mod.setParametersEnvFol
            (
                modSys.getParam(withOffset(PID::EnvFol0Attack, offset))->getValSumDenorm(),
                modSys.getParam(withOffset(PID::EnvFol0Release, offset))->getValSumDenorm(),
                modSys.getParam(withOffset(PID::EnvFol0Gain, offset))->getValSumDenorm(),
                modSys.getParam(withOffset(PID::EnvFol0Width, offset))->getValueSum()
            );
            break;
        case vibrato::ModType::Macro:
            mod.setParametersMacro
            (
                modSys.getParam(withOffset(PID::Macro0, offset))->getValSumDenorm()
            );
            break;
        case vibrato::ModType::Pitchwheel:
            mod.setParametersPitchbend
            (
                modSys.getParam(withOffset(PID::Pitchbend0Smooth, offset))->getValSumDenorm()
            );
            break;
        case vibrato::ModType::LFO:
            mod.setParametersLFO
            (
                modSys.getParam(withOffset(PID::LFO0FreeSync, offset))->getValueSum() > .5f,
                modSys.getParam(withOffset(PID::LFO0RateFree, offset))->getValSumDenorm(),
                modSys.getParam(withOffset(PID::LFO0RateSync, offset))->getValSumDenorm(),
                modSys.getParam(withOffset(PID::LFO0Waveform, offset))->getValueSum(),
                modSys.getParam(withOffset(PID::LFO0Phase, offset))->getValSumDenorm(),
                modSys.getParam(withOffset(PID::LFO0Width, offset))->getValSumDenorm()
            );
            break;
        }
        
        mod.processBlock(samplesRead, midi, curPlayHead, numChannels, numSamples);
    }

    auto modsBuf = modsBuffer.getArrayOfWritePointers();

    // FILL MODBUFFER WITH MODULATORS
    {
        const auto modsMix = modSys.getParam(modSys6::PID::ModsMix)->getValueSum();
        const auto depth = modSys.getParam(modSys6::PID::Depth)->getValueSum();
        
        auto modsMixSmoothing = modsMixSmooth(modsMixBuf.data(), modsMix, numSamples);
		auto depthSmoothing = depthSmooth(depthBuf.data(), depth, numSamples);
        
        if(!modsMixSmoothing)
			juce::FloatVectorOperations::fill(modsMixBuf.data(), modsMix, numSamples);
		if (!depthSmoothing)
			juce::FloatVectorOperations::fill(depthBuf.data(), depth, numSamples);
        
        for (auto ch = 0; ch < numChannels; ++ch)
        {
            const auto m0 = modulators[0].buffer[ch].data();
            const auto m1 = modulators[1].buffer[ch].data();
            auto mAll = modsBuf[ch];
            for (auto s = 0; s < numSamples; ++s)
                mAll[s] = (m0[s] + modsMixBuf[s] * (m1[s] - m0[s])) * depthBuf[s];
            visualizerValues[ch] = mAll[numSamples - 1];
        }
    }

#if DebugModsBuffer
    const auto depth = modSys.getParam(modSys6::PID::Depth)->getValueSum();
    for (auto ch = 0; ch < numChannelsOut; ++ch)
    {
        const auto mAll = modsBuffer[ch].data();
        auto samples = buffer->getWritePointer(ch);
        for (auto s = 0; s < numSamples; ++s)
            samples[s] = mAll[s] * depth;
        visualizerValues[ch] = mAll[numSamples - 1];
    }
#else
    vibrat
    (
        buffer.getArrayOfWritePointers(),
        numChannels,
        numSamples,
        modsBuf
    );
#endif
    
#if OversamplingEnabled
    if(oversampling.isEnabled())
        oversampling.downsample(bufferOut);
#endif
}

void Nel19AudioProcessor::processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const auto numSamples = buffer.getNumSamples();
    modSys.processBlock(numSamples, getPlayHead());
    if (numSamples == 0)
        return;
	auto numChannels = buffer.getNumChannels();
    auto samples = buffer.getArrayOfWritePointers();
    dryWet.processBypass(samples, numChannels, numSamples);
}

bool Nel19AudioProcessor::hasEditor() const { return true; }

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
    std::unique_ptr<juce::XmlElement> xml(modSys.state.createXml());
    copyXmlToBinary(*xml, destData);
}

void Nel19AudioProcessor::savePatch()
{
    modSys.savePatch();
    {
        const auto modTypeID = vibrato::toString(vibrato::ObjType::ModType);
        const juce::Identifier id(modTypeID);
        auto modTypeState = modSys.state.getChildWithName(id);
        if (!modTypeState.isValid())
        {
            modTypeState = juce::ValueTree(id);
            modSys.state.appendChild(modTypeState, nullptr);
        }
        for (auto m = 0; m < NumActiveMods; ++m)
        {
            const auto typeID = modTypeID + static_cast<juce::String>(m);
            modTypeState.setProperty(typeID, vibrato::toString(modType[m]), nullptr);
        }
    }
    for (auto m = 0; m < modulators.size(); ++m)
        modulators[m].savePatch(modSys.state, m);
    {
        const juce::Identifier id(vibrato::toString(vibrato::ObjType::InterpolationType));
        const auto type = vibrat.interpolationType.load();
        const auto typeStr = vibrato::toString(type);
        modSys.state.setProperty(id, typeStr, nullptr);
    }
    {
        const juce::Identifier id(vibrato::toString(vibrato::ObjType::DelaySize));
        const auto bufferSize = vibrat.getSizeInMs(static_cast<float>(oversampling.getSampleRateUpsampled()));
        modSys.state.setProperty(id, bufferSize, nullptr);
    }
    {
        const juce::Identifier id(oversampling::getID());
        const auto oEnabled = oversampling.isEnabled() ? 1 : 0;
        modSys.state.setProperty(id, oEnabled, nullptr);
    }
    {
        const juce::Identifier id(getLookaheadID());
        const auto oEnabled = lookaheadEnabled.load() ? 1 : 0;
        modSys.state.setProperty(id, oEnabled, nullptr);
    }
}

void Nel19AudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(modSys.state.getType()))
            modSys.state = juce::ValueTree::fromXml(*xmlState);
    loadPatch();
#if RemoveValueTree
    modSys.state.removeAllChildren(nullptr);
    modSys.state.removeAllProperties(nullptr);
#endif
}

void Nel19AudioProcessor::loadPatch()
{
    suspendProcessing(true);
    modSys.loadPatch();
    {
        const auto modTypeID = vibrato::toString(vibrato::ObjType::ModType);
        const auto modTypeState = modSys.state.getChildWithName(modTypeID);
        if (modTypeState.isValid())
        {
            for (auto m = 0; m < NumActiveMods; ++m)
            {
                const auto propID = modTypeID + static_cast<juce::String>(m);
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
        modulators[m].loadPatch(modSys.state, m);
    {
        const juce::Identifier id(vibrato::toString(vibrato::ObjType::InterpolationType));
        const auto typeStr = modSys.state.getProperty(id, "").toString();
        if (typeStr.isNotEmpty())
        {
            const auto type = vibrato::toType(typeStr);
            vibrat.interpolationType.store(type);
        }
    }
    {
        const juce::Identifier id(vibrato::toString(vibrato::ObjType::DelaySize));
        const auto sizeStr = modSys.state.getProperty(id, "").toString();
        if (sizeStr.isNotEmpty())
        {
            const auto bufferSize = sizeStr.getFloatValue();
            modSys.state.setProperty(id, bufferSize, nullptr);
        }
    }
    {
        const juce::Identifier id(oversampling::getID());
        const auto oEnabledStr = modSys.state.getProperty(id, "").toString();
        if (oEnabledStr.isNotEmpty())
        {
            const auto oEnabled = oEnabledStr.getIntValue() == 1 ? true : false;
            oversamplingEnabled.store(oEnabled);
        }
    }
    {
        const juce::Identifier id(getLookaheadID());
        const auto oEnabledStr = modSys.state.getProperty(id, "").toString();
        if (oEnabledStr.isNotEmpty())
        {
            const auto oEnabled = oEnabledStr.getIntValue() == 1 ? true : false;
            lookaheadEnabled.store(oEnabled);
        }
    }
    prepareToPlay(getSampleRate(), getBlockSize());
    suspendProcessing(false);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Nel19AudioProcessor();
}

void Nel19AudioProcessor::timerCallback()
{
    if (oversamplingEnabled.load() != oversampling.isEnabled() ||
        lookaheadEnabled.load() != getLatencySamples() - oversampling.getLatency() != 0)
    {
        forcePrepare();
    }
}

void Nel19AudioProcessor::forcePrepare()
{
    suspendProcessing(true);
	prepareToPlay(getSampleRate(), getBlockSize());
	suspendProcessing(false);
}

juce::String Nel19AudioProcessor::getLookaheadID()
{
    return "lookahead";
}

#undef RemoveValueTree
#undef OversamplingEnabled
#undef DebugModsBuffer