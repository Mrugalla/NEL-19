#include "PluginProcessor.h"
#include "PluginEditor.h"

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
    apvts(*this, nullptr, "parameters", nelDSP::param::createParameters()),
    params(),
    nel19(this),
    curDepthMaxIdx(0)
#endif
{
    params.push_back(apvts.getRawParameterValue(nelDSP::param::getID(nelDSP::param::ID::DepthMax)));
    params.push_back(apvts.getRawParameterValue(nelDSP::param::getID(nelDSP::param::ID::Depth)));
    params.push_back(apvts.getRawParameterValue(nelDSP::param::getID(nelDSP::param::ID::Freq)));
    params.push_back(apvts.getRawParameterValue(nelDSP::param::getID(nelDSP::param::ID::LRMS)));
    params.push_back(apvts.getRawParameterValue(nelDSP::param::getID(nelDSP::param::ID::Width)));
    params.push_back(apvts.getRawParameterValue(nelDSP::param::getID(nelDSP::param::ID::Mix)));
}
Nel19AudioProcessor::~Nel19AudioProcessor(){}
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
    nel19.prepareToPlay(sampleRate, maxBufferSize, getChannelCountOfBus(true, 0));
    /*
    auto state = static_cast<int>(
        apvts.state.getChildWithName("MIDILearn").getProperty("MIDILearn", 0)
    );
    auto& ml = nel19.getMidiLearn();
    ml.setState(static_cast<nelDSP::MidiLearn::State>(state));
    auto type = static_cast<int>(
        apvts.state.getChildWithName("MIDILearn").getProperty("Type", 0)
    );
    ml.type = static_cast<nelDSP::MidiLearn::Type>(type);
    auto cc = static_cast<int>(
        apvts.state.getChildWithName("MIDILearn").getProperty("CC", 0)
    );
    ml.controllerNumber = cc; */
}
void Nel19AudioProcessor::releaseResources() {}
#ifndef JucePlugin_PreferredChannelConfigurations
bool Nel19AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif
void Nel19AudioProcessor::processParameters() {
    const auto depthMaxIdx = static_cast<int>(params[static_cast<int>(nelDSP::param::ID::DepthMax)]->load());
    if (depthMaxIdx != curDepthMaxIdx) {
        curDepthMaxIdx = depthMaxIdx;
        const auto p = apvts.getParameter(nelDSP::param::getID(nelDSP::param::ID::DepthMax));
        const auto depthMaxInMs = p->getCurrentValueAsText().getFloatValue();
        nel19.setDepthMax(depthMaxInMs);
    }
    nel19.setDepth(params[static_cast<int>(nelDSP::param::ID::Depth)]->load());
    nel19.setFreq(params[static_cast<int>(nelDSP::param::ID::Freq)]->load());
    nel19.setLRMS(params[static_cast<int>(nelDSP::param::ID::LRMS)]->load());
    nel19.setWidth(params[static_cast<int>(nelDSP::param::ID::Width)]->load());
    nel19.setMix(params[static_cast<int>(nelDSP::param::ID::Mix)]->load());
}
void Nel19AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    if (buffer.getNumSamples() == 0) return;
    juce::ScopedNoDenormals noDenormals;
    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    processParameters();
    nel19.processBlock(buffer, midi);
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
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}
void Nel19AudioProcessor::setStateInformation (const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));

    // flush value tree
    //apvts.state.removeAllChildren(nullptr);
    //apvts.state.removeAllProperties(nullptr);
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Nel19AudioProcessor(); }
