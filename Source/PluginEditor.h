/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================

struct LookAndFeel : juce::LookAndFeel_V4
{
    void drawRotarySlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPosProportional,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider& slider) override;
};

struct CustomRotarySlider : juce::Slider
{
    CustomRotarySlider(juce::RangedAudioParameter& rap, const juce::String& unitSuffix) : juce::Slider(juce::Slider::RotaryVerticalDrag, juce::Slider::TextEntryBoxPosition::NoTextBox),
    param(&rap),
    suffix(unitSuffix)
    {
        setLookAndFeel(&lnf);
    }
    
    ~CustomRotarySlider()
    {
        setLookAndFeel(nullptr);
    }
    
    juce::String label;
    
    void paint(juce::Graphics& g) override;
    juce::Rectangle<int> getSliderBounds() const;
    int getTextHeight() const { return 20; }
    juce::String getDisplayString() const;
    
private:
    LookAndFeel lnf;
    
    juce::RangedAudioParameter* param;
    juce::String suffix;
};



class RasterKnob : public juce::Slider
{
    public:
    RasterKnob(juce::RangedAudioParameter& rap) : Slider(SliderStyle::RotaryHorizontalVerticalDrag, TextEntryBoxPosition::NoTextBox),
    param(&rap)
    {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
        setLookAndFeel(&mainSliderLookAndFeel);
    }

    ~RasterKnob()
    {
        setLookAndFeel(nullptr);
    }
    
    juce::String getDisplayString() const {
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
        
        return str;
    }
    
    class RasterKnobLookAndFeel : public juce::LookAndFeel_V4
        {
        public:
            RasterKnobLookAndFeel()
            {
                image = juce::ImageCache::getFromMemory(BinaryData::Cronch_Sheet_png, BinaryData::Cronch_Sheet_pngSize);
            }

            void drawRotarySlider(
                                  juce::Graphics& g,
                int x,
                int y,
                int width,
                int height,
                float sliderPosProportional,
                float /*rotaryStartAngle*/,
                float /*rotaryEndAngle*/,
                Slider& slider) override
            {
                const auto frames = 127;
                const auto frameId = static_cast<int>(ceil(sliderPosProportional * (static_cast<float>(frames) - 1.0f)));

                g.drawImage(image,
                    x,
                    y,
                    width,
                    height,
                    0,
                    frameId * height*2,
                    width*2,
                    height*2);
                
                if (slider.isMouseButtonDown()) {
                    auto* rswl = dynamic_cast<RasterKnob*>(&slider);
                    g.setColour(juce::Colours::black);
                    g.fillRect(width/2 - 28, height/2 - 10, 50, 20);
                    g.setFont(16.0f);
                    g.setColour(juce::Colours::white);
                    g.drawFittedText(rswl->getDisplayString(), x-4, y-1, width, height, juce::Justification::centred, 1);
                }
            }
            
            juce::Image image;

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RasterKnobLookAndFeel)
        } mainSliderLookAndFeel;
    
    juce::RangedAudioParameter* param;
    
};

//==============================================================================
/**
*/
class AM_SidechainAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    AM_SidechainAudioProcessorEditor (AM_SidechainAudioProcessor&);
    ~AM_SidechainAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AM_SidechainAudioProcessor& audioProcessor;
    
    RasterKnob multSlider,
    outGainSlider,
    dcOffsetSlider,
    smoothnessSlider;
    
    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;
    
    Attachment multSliderAttachment,
    outGainSliderAttachment,
    dcOffsetSliderAttachment,
    smoothnessSliderAttachment;
    
    std::vector<juce::Component*> getComps();
    

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AM_SidechainAudioProcessorEditor)
};
