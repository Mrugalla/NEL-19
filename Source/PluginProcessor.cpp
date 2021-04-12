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
    apvts(*this, nullptr, "parameters", param::createParameters()),
    params(),
    nel19(this),
    curDepthMaxIdx(-1)
#endif
{
    appProperties.setStorageParameters(makeOptions());

    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::DepthMax)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::Depth)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::Freq)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::Smooth)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::LRMS)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::Width)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::Mix)));
    params.push_back(apvts.getRawParameterValue(param::getID(param::ID::SplineMix)));
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
    nel19.prepareToPlay(sampleRate, maxBufferSize, getChannelCountOfBus(false, 0));
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
    const auto depthMaxIdx = static_cast<int>(params[static_cast<int>(param::ID::DepthMax)]->load());
    if (depthMaxIdx != curDepthMaxIdx) {
        curDepthMaxIdx = depthMaxIdx;
        const auto p = apvts.getParameter(param::getID(param::ID::DepthMax));
        const auto depthMaxInMs = p->getCurrentValueAsText().getFloatValue();
        nel19.setDepthMax(depthMaxInMs);
    }
    nel19.setDepth(params[static_cast<int>(param::ID::Depth)]->load());
    nel19.setFreq(params[static_cast<int>(param::ID::Freq)]->load());
    nel19.setShape(1.f - params[static_cast<int>(param::ID::Smooth)]->load());
    nel19.setLRMS(params[static_cast<int>(param::ID::LRMS)]->load());
    nel19.setWidth(params[static_cast<int>(param::ID::Width)]->load());
    nel19.setMix(params[static_cast<int>(param::ID::Mix)]->load());
    nel19.setSplineMix(params[static_cast<int>(param::ID::SplineMix)]->load());
}
void Nel19AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    if (buffer.getNumSamples() == 0) return;
    juce::ScopedNoDenormals noDenormals;
    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    processParameters();
    nel19.processBlock(buffer);
}
void Nel19AudioProcessor::processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    if (buffer.getNumSamples() == 0) return;
    juce::ScopedNoDenormals noDenormals;
    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());
    processParameters();
    nel19.processBlockBypassed(buffer);
}
bool Nel19AudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* Nel19AudioProcessor::createEditor() { return new Nel19AudioProcessorEditor (*this); }
void Nel19AudioProcessor::getStateInformation (juce::MemoryBlock& destData) {
    auto& spline = nel19.getSplineCreator();
    spline.getState(apvts.state);
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
    auto& spline = nel19.getSplineCreator();
    spline.setState(apvts.state);
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Nel19AudioProcessor(); }