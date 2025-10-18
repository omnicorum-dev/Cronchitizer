/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

void LookAndFeel::drawRotarySlider(juce::Graphics & g,
                                   int x,
                                   int y,
                                   int width,
                                   int height,
                                   float sliderPosProportional,
                                   float rotaryStartAngle,
                                   float rotaryEndAngle,
                                   juce::Slider & slider)
{
    using namespace juce;
    
    auto bounds = Rectangle<float>(x, y, width, height);
    
    g.setColour(Colour(31u, 95u, 135u));
    g.fillEllipse(bounds);
    
    g.setColour(Colour(25u, 162u, 239u));
    g.drawEllipse(bounds, 2.5f);
    
    if (auto* rswl = dynamic_cast<CustomRotarySlider*>(&slider))
    {
        auto center = bounds.getCentre();
        
        Path p;
        
        Rectangle<float> r;
        r.setLeft(center.getX() - 2);
        r.setRight(center.getX() + 2);
        r.setTop(bounds.getY());
        r.setBottom(center.getY() - rswl->getTextHeight() * 1.5);
        
        p.addRoundedRectangle(r, 2.f);
        
        jassert(rotaryStartAngle < rotaryEndAngle);
        
        auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);
        
        p.applyTransform(AffineTransform().rotated(sliderAngRad, center.getX(), center.getY()));
        
        g.setColour(Colour(0u, 193u, 239u));
        g.fillPath(p);
        
        g.setFont(rswl->getTextHeight());
        auto text = rswl->getDisplayString();
        auto strWidth = g.getCurrentFont().getStringWidth(text);
        
        r.setSize(strWidth + 4, rswl->getTextHeight() + 2);
        r.setCentre(bounds.getCentre());
        
        g.setColour(Colours::black);
        g.fillRect(r);
        
        g.setColour(Colours::white);
        g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
    }
    
    
    
}

void CustomRotarySlider::paint(juce::Graphics &g)
{
    using namespace juce;
    
    auto startAng = degreesToRadians(180.f + 35.f);
    auto endAng = degreesToRadians(180.f - 35.f) + MathConstants<float>::twoPi;
    
    auto range = getRange();
    
    auto sliderBounds = getSliderBounds();
    
    //g.setColour(Colours::red);
    //g.drawRect(getLocalBounds());
    //g.setColour(Colours::yellow);
    //g.drawRect(sliderBounds);
    
    getLookAndFeel().drawRotarySlider(g,
                                      sliderBounds.getX(),
                                      sliderBounds.getY(),
                                      sliderBounds.getWidth(),
                                      sliderBounds.getHeight(),
                                      jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0),
                                      startAng,
                                      endAng,
                                      *this);
    
    
    g.setColour(Colour(0u, 193u, 239u));
    g.setFont(getTextHeight());
    
    Rectangle<float> r;
    auto str = label;
    
    r.setTop(sliderBounds.getBottom());
    r.setBottom(getLocalBounds().getBottom());
    r.setRight(getLocalBounds().getRight());
    r.setLeft(getLocalBounds().getRight() - getLocalBounds().getWidth());
    
    //g.drawRect(r);
    
    g.drawFittedText(str, r.toNearestInt(), juce::Justification::centred, 1);
}

juce::Rectangle<int> CustomRotarySlider::getSliderBounds() const
{
    auto bounds = getLocalBounds();
    
    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());
    
    size -= getTextHeight() * 2;
    juce::Rectangle<int> r;
    r.setSize(size, size);
    r.setCentre(bounds.getCentreX(), 0);
    r.setY(5);
    
    return r;
}

juce::String CustomRotarySlider::getDisplayString() const
{
    juce::String str;
    
    if ( auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param))
    {
        float val = getValue();
        
        str = juce::String(val, 2, false);
    }
    else
    {
        jassertfalse;
    }
    
    if (suffix.isNotEmpty()) {
        str << " ";
        str << suffix;
    }
    
    return str;
}

//==============================================================================
AM_SidechainAudioProcessorEditor::AM_SidechainAudioProcessorEditor (AM_SidechainAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
multSlider(*audioProcessor.apvts.getParameter("Input Mult")),
outGainSlider(*audioProcessor.apvts.getParameter("Output Gain")),
dcOffsetSlider(*audioProcessor.apvts.getParameter("DC Offset")),
smoothnessSlider(*audioProcessor.apvts.getParameter("Smoothness")),
multSliderAttachment(audioProcessor.apvts, "Input Mult", multSlider),
outGainSliderAttachment(audioProcessor.apvts, "Output Gain", outGainSlider),
dcOffsetSliderAttachment(audioProcessor.apvts, "DC Offset", dcOffsetSlider),
smoothnessSliderAttachment(audioProcessor.apvts, "Smoothness", smoothnessSlider)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    
    //multSlider.label = "CRONCHITIZE";
    //outGainSlider.label = "Output Gain";
    //dcOffsetSlider.label = "DC Offset";
    //smoothnessSlider.label = "Smoothness";
    
    for( auto* comp : getComps() )
    {
        addAndMakeVisible(comp);
    }
    
    addAndMakeVisible(audioProcessor.waveViewerSC);
    
    audioProcessor.waveViewerSC.setColours(juce::Colours::black, juce::Colours::green);
    
    
    addAndMakeVisible(audioProcessor.waveViewerMain);
    audioProcessor.waveViewerMain.setColours(juce::Colours::transparentWhite, juce::Colours::blue);
    audioProcessor.waveViewerMain.setAlwaysOnTop(true);
    audioProcessor.waveViewerMain.setOpaque(false);
    
    setSize (1920/2, 814/2);
}

AM_SidechainAudioProcessorEditor::~AM_SidechainAudioProcessorEditor()
{
}

//==============================================================================
void AM_SidechainAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (juce::Colour(40u, 40u, 40u));

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (15.0f));
    
    g.drawImage(juce::ImageCache::getFromMemory(BinaryData::Background_png, BinaryData::Background_pngSize), getLocalBounds().toFloat());
    //g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void AM_SidechainAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    
    auto bounds = getLocalBounds();
    auto crunchArea = bounds.removeFromLeft(bounds.getWidth() * 0.5);
    auto outGainArea = bounds.removeFromBottom(bounds.getHeight() * 0.45);
    auto dcOffsetArea = bounds.removeFromLeft(bounds.getWidth() * 0.5);
    auto smoothnessArea = bounds;
    
    multSlider.setBounds(crunchArea);
    outGainSlider.setBounds(outGainArea);
    dcOffsetSlider.setBounds(dcOffsetArea);
    smoothnessSlider.setBounds(smoothnessArea);
    
    multSlider.setBounds(145/2, 130/2, 455/2, 460/2);
    multSlider.mainSliderLookAndFeel.image = juce::ImageCache::getFromMemory(BinaryData::Cronch_Sheet_png, BinaryData::Cronch_Sheet_pngSize);
    
    dcOffsetSlider.setBounds(660/2, 50/2, 190/2, 200/2);
    dcOffsetSlider.mainSliderLookAndFeel.image = juce::ImageCache::getFromMemory(BinaryData::DC_Sheet_png, BinaryData::DC_Sheet_pngSize);
    
    smoothnessSlider.setBounds(660/2, 310/2, 190/2, 200/2);
    smoothnessSlider.mainSliderLookAndFeel.image = juce::ImageCache::getFromMemory(BinaryData::Smoothe_Sheet_png, BinaryData::Smoothe_Sheet_pngSize);
    
    outGainSlider.setBounds(660/2, 570/2, 190/2, 200/2);
    outGainSlider.mainSliderLookAndFeel.image = juce::ImageCache::getFromMemory(BinaryData::Gain_Sheet_png, BinaryData::Gain_Sheet_pngSize);
    
    audioProcessor.waveViewerMain.setBounds(1165/2, 115/2, 560/2, 500/2);
    audioProcessor.waveViewerSC.setBounds(1165/2, 115/2, 560/2, 500/2);
    
}

std::vector<juce::Component*> AM_SidechainAudioProcessorEditor::getComps()
{
    return
    {
        &multSlider,
        &outGainSlider,
        &dcOffsetSlider,
        &smoothnessSlider
    };
}
