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
    apvts(*this, nullptr, "parameters", param::createParameters(apvts, modRateRanges)),
    params()
#endif
{
    appProperties.setStorageParameters(makeOptions());

    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::Macro0)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::Macro1)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::Macro2)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::Macro3)));

    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::EnvFolGain0)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::EnvFolAtk0)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::EnvFolRls0)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::EnvFolBias0)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::EnvFolWidth0)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::LFOSync0)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::LFORate0)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::LFOWidth0)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::LFOWaveTable0)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::RandSync0)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::RandRate0)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::RandBias0)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::RandWidth0)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::RandSmooth0)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::PerlinSync0)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::PerlinRate0)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::PerlinOctaves0)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::PerlinWidth0)));

    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::EnvFolGain1)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::EnvFolAtk1)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::EnvFolRls1)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::EnvFolBias1)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::EnvFolWidth1)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::LFOSync1)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::LFORate1)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::LFOWidth1)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::LFOWaveTable1)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::RandSync1)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::RandRate1)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::RandBias1)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::RandWidth1)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::RandSmooth1)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::PerlinSync1)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::PerlinRate1)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::PerlinOctaves1)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::PerlinWidth1)));

    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::Depth)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::ModulatorsMix)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::DryWetMix)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::Voices)));
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
void Nel19AudioProcessor::processParameters() {
    
}
void Nel19AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    if (buffer.getNumSamples() == 0) return;
    juce::ScopedNoDenormals noDenormals;
    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    processParameters();
    ////
}
void Nel19AudioProcessor::processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    if (buffer.getNumSamples() == 0) return;
    juce::ScopedNoDenormals noDenormals;
    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());
    processParameters();
    ////
}
bool Nel19AudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* Nel19AudioProcessor::createEditor() { return new Nel19AudioProcessorEditor (*this); }
void Nel19AudioProcessor::getStateInformation (juce::MemoryBlock& destData) {
    //auto& spline = nel19.getSplineCreator();
    //spline.getState(apvts.state);
#if RemoveValueTree
    apvts.state.removeAllChildren(nullptr);
    apvts.state.removeAllProperties(nullptr);
#endif
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}
void Nel19AudioProcessor::setStateInformation (const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
    handleAsyncUpdate();
#if RemoveValueTree
    apvts.state.removeAllChildren(nullptr);
    apvts.state.removeAllProperties(nullptr);
#endif
}
void Nel19AudioProcessor::handleAsyncUpdate() {
    //auto& spline = nel19.getSplineCreator();
    //spline.setState(apvts.state);
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Nel19AudioProcessor(); }