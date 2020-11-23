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
                       ), apvts(*this, nullptr, "parameters", tape::param::createParameters()),
    param({
        apvts.getRawParameterValue(tape::param::getID(tape::param::ID::WowDepth)),
        apvts.getRawParameterValue(tape::param::getID(tape::param::ID::WowFreq)),
        apvts.getRawParameterValue(tape::param::getID(tape::param::ID::WowWidth)),
        apvts.getRawParameterValue(tape::param::getID(tape::param::ID::Lookahead))
        }),
    paramRange({
        apvts.getParameter(tape::param::getID(tape::param::ID::WowDepth)),
        apvts.getParameter(tape::param::getID(tape::param::ID::WowFreq)),
        apvts.getParameter(tape::param::getID(tape::param::ID::WowWidth))
        }),
    paramNormalized({0,0,0}),
    paramValue({-1, -1, -1, -1}),
    tape()
#endif
{
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
const juce::String Nel19AudioProcessor::getProgramName (int index) { return {}; }
void Nel19AudioProcessor::changeProgramName (int, const juce::String&){}

void Nel19AudioProcessor::prepareToPlay(double sampleRate, int maxBufferSize) {
    tape.prepareToPlay(sampleRate, this, maxBufferSize, getChannelCountOfBus(true, 0));
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
void Nel19AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    if (buffer.getNumSamples() == 0) return;
    juce::ScopedNoDenormals noDenormals;
    const auto totalNumInputChannels  = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    paramValue[0] = param[0]->load(); // depth
    tape.setWowDepth(paramValue[0]);
    paramNormalized[0] = paramRange[0]->convertTo0to1(paramValue[0]);
    paramValue[1] = param[1]->load(); // freq
    tape.setWowFreq(paramValue[1]);
    paramNormalized[1] = paramRange[1]->convertTo0to1(paramValue[1]);
    paramValue[2] = param[2]->load(); // width
    tape.setWowWidth(paramValue[2]);
    paramNormalized[2] = paramRange[2]->convertTo0to1(paramValue[2]);
    paramValue[3] = param[3]->load(); // lookahead
    tape.setLookaheadEnabled((bool)paramValue[3]);

    tape.processBlock(buffer);
}
void Nel19AudioProcessor::processBlockBypassed(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    tape.processBlockBypassed(buffer);
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
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Nel19AudioProcessor(); }