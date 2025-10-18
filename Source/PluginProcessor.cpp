/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AM_SidechainAudioProcessor::AM_SidechainAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                        #if ! JucePlugin_IsMidiEffect
                            .withInput("Input",  juce::AudioChannelSet::stereo(), true)
                            #if ! JucePlugin_IsSynth
                                .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                            #endif
                            .withInput ("Sidechain", juce::AudioChannelSet::stereo(), true)
                        #endif
                            ), waveViewerMain(1), waveViewerSC(1)
#endif
{
    waveViewerMain.setRepaintRate(30);
    waveViewerMain.setBufferSize(256);
    waveViewerMain.setSamplesPerBlock(8);
    
    waveViewerSC.setRepaintRate(30);
    waveViewerSC.setBufferSize(256);
    waveViewerSC.setSamplesPerBlock(8);
}

AM_SidechainAudioProcessor::~AM_SidechainAudioProcessor()
{
}

//==============================================================================
const juce::String AM_SidechainAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AM_SidechainAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AM_SidechainAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AM_SidechainAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AM_SidechainAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AM_SidechainAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AM_SidechainAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AM_SidechainAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String AM_SidechainAudioProcessor::getProgramName (int index)
{
    return {};
}

void AM_SidechainAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void AM_SidechainAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    
    
    
}

void AM_SidechainAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AM_SidechainAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
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

void AM_SidechainAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    /*auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        // ..do something to the data...
    }*/
    
    auto chainSettings = getChainSettings(apvts);
    float outputGain = juce::Decibels::decibelsToGain(chainSettings.outputGainInDecibels);
    float inputMult = chainSettings.inputMult;
    float dcOffset = chainSettings.dcOffset;
    float smoothness = chainSettings.smoothness + 1.f;
    
    juce::AudioBuffer<float> mainBuffer = getBusBuffer(buffer, true, 0);
    juce::AudioBuffer<float> sidechainBuffer = getBusBuffer(buffer, true, 1);
    
    //waveViewerMain.pushBuffer(mainBuffer);
    waveViewerSC.pushBuffer(sidechainBuffer);
    
    int side_channels = sidechainBuffer.getNumChannels();
    int main_channels = mainBuffer.getNumChannels();
    
    if (side_channels == 0) {
        
    } else if (main_channels < 2) {
        
    } else {
        for (int channel = 0; channel < 2; ++ channel) {
            auto* channelData = mainBuffer.getWritePointer(channel);
            //auto* sidechainData = sidechainBuffer.getWritePointer(channel);
            
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
                float main_sample = mainBuffer.getSample(channel, sample);
                float sc_sample = sidechainBuffer.getSample(channel, sample);
                
                // apply DC Offset function
                sc_sample += 0.5 * dcOffset * (1 - sc_sample);
                // apply smooth absolute value
                sc_sample = std::pow(abs(sc_sample), smoothness);
                // apply sidechain input gain
                sc_sample *= inputMult;
                // invert
                sc_sample = 1 - sc_sample;
                // clip
                sc_sample = std::max(0.0f, sc_sample);
                
                // apply sidechain gain curve to output sample
                channelData[sample] = (main_sample * sc_sample) * outputGain;
            }
        }
        
    }
    
    waveViewerMain.pushBuffer(mainBuffer);
    
}

//==============================================================================
bool AM_SidechainAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AM_SidechainAudioProcessor::createEditor()
{
    return new AM_SidechainAudioProcessorEditor (*this);
    
    //return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void AM_SidechainAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    
    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void AM_SidechainAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid()) {
        apvts.replaceState(tree);
    }
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState & apvts)
{
    ChainSettings settings;
    
    settings.inputMult = apvts.getRawParameterValue("Input Mult")->load();
    settings.outputGainInDecibels = apvts.getRawParameterValue("Output Gain")->load();
    settings.dcOffset = apvts.getRawParameterValue("DC Offset")->load();
    settings.smoothness = apvts.getRawParameterValue("Smoothness")->load();
    
    return settings;
}

juce::AudioProcessorValueTreeState::ParameterLayout AM_SidechainAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Input Mult",
                                                           "Input Mult",
                                                           juce::NormalisableRange<float>(0.f, 7.5f, 0.05f, 1.f),
                                                           1.f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Output Gain",
                                                           "Output Gain",
                                                           juce::NormalisableRange<float>(-12.f, 12.f, 0.1f, 1.f),
                                                           0.f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("DC Offset",
                                                           "DC Offset",
                                                           juce::NormalisableRange<float>(0.f, 1.f, 0.01f, 1.f),
                                                           0.f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Smoothness",
                                                           "Smoothness",
                                                           juce::NormalisableRange<float>(0.f, 1.f, 0.01, 1.f),
                                                           0.f));
    
    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AM_SidechainAudioProcessor();
}
