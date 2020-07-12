/*
  ==============================================================================

   This file is part of the JUCE examples.
   Copyright (c) 2020 - Raw Material Software Limited

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES,
   WHETHER EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR
   PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

/*******************************************************************************
 The block below describes the properties of this PIP. A PIP is a short snippet
 of code that can be read by the Projucer and used to generate a JUCE project.

 BEGIN_JUCE_PIP_METADATA

 name:                  BeatPeggiatorPlugin
 version:               1.0.0
 vendor:                JUCE
 website:               http://juce.com
 description:           BeatPeggiator audio plugin.

 dependencies:          juce_audio_basics, juce_audio_devices, juce_audio_formats,
                        juce_audio_plugin_client, juce_audio_processors,
                        juce_audio_utils, juce_core, juce_data_structures,
                        juce_events, juce_graphics, juce_gui_basics, juce_gui_extra
 exporters:             xcode_mac, vs2019

 moduleFlags:           JUCE_STRICT_REFCOUNTEDPOINTER=1

 type:                  AudioProcessor
 mainClass:             BeatPeggiator

 useLocalCopy:          1

 pluginCharacteristics: pluginWantsMidiIn, pluginProducesMidiOut, pluginIsMidiEffectPlugin

 END_JUCE_PIP_METADATA

*******************************************************************************/

#pragma once
#include <iostream>
#include <array>

class BeatPeggiatorEditor : public AudioProcessorEditor
{
public:
    BeatPeggiatorEditor (AudioProcessor& p, AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor (p),
      parameters (vts)
    {
        numNotesSlider.setSliderStyle (Slider::SliderStyle::LinearHorizontal);
        numNotesSlider.setTextBoxStyle (Slider::TextEntryBoxPosition::TextBoxBelow, true, 50, 10);
        numNotesSlider.setRange (0, 10, 1);
        addAndMakeVisible (numNotesSlider);
        
        beatDivisionSlider.setSliderStyle (Slider::SliderStyle::LinearHorizontal);
        beatDivisionSlider.setTextBoxStyle (Slider::TextEntryBoxPosition::TextBoxBelow, true, 50, 10);
        beatDivisionSlider.setRange (0, 10, 1);
        addAndMakeVisible (beatDivisionSlider);
        
        beatsSlider.setSliderStyle (Slider::SliderStyle::LinearHorizontal);
        beatsSlider.setTextBoxStyle (Slider::TextEntryBoxPosition::TextBoxBelow, true, 50, 10);
        beatsSlider.setRange (0, 10, 1);
        addAndMakeVisible (beatsSlider);

        numNotesAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (parameters, "numNotes", numNotesSlider);
        beatDivisionAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (parameters, "beatDivision", beatDivisionSlider);

        beatsAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (parameters, "beats", beatsSlider);


        setSize (400, 800);

    }
    
    void paint (Graphics& g) override
    {
        g.fillAll (Colours::black);
                    
    }
    
    void resized () override
    {
        auto bounds = getLocalBounds();
        const int componentSize { 100 };
        
        numNotesSlider.setBounds (bounds.removeFromTop (200).withSizeKeepingCentre (componentSize, componentSize));
        beatDivisionSlider.setBounds (bounds.removeFromTop (200).withSizeKeepingCentre (componentSize, componentSize));
        beatsSlider.setBounds (bounds.removeFromTop (200).withSizeKeepingCentre (componentSize, componentSize));
    }
    
private:
    AudioProcessorValueTreeState& parameters;
    
    Slider beatsSlider;
    Slider beatDivisionSlider;
    Slider numNotesSlider;
    
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> beatsAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> beatDivisionAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> numNotesAttachment;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BeatPeggiatorEditor)
};


//==============================================================================
class BeatPeggiatorProcessor  : public AudioProcessor //, private AudioProcessorValueTreeState::Listener
{
public:

    //==============================================================================
    BeatPeggiatorProcessor()
        : AudioProcessor (BusesProperties().withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                           .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
          parameters(*this, nullptr, "BeatPeggiator", createParameters())
    {
//        parameters.addParameterListener("numNotes", this);
//        parameters.addParameterListener("beatDivision", this);
//        parameters.addParameterListener("beats", this);
        std::srand(std::time(NULL));
        
        numNotesParameter = parameters.getRawParameterValue("numNotes");
        beatDivisionParameter = parameters.getRawParameterValue("beatDivision");
        beatsParameter = parameters.getRawParameterValue("beats");
        

        
    }
    //==============================================================================
    AudioProcessorValueTreeState::ParameterLayout createParameters()
        {
            std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
            
            parameters.push_back (std::make_unique<AudioParameterInt> ("beats", "Beats", 1, 10, 1));
            parameters.push_back (std::make_unique<AudioParameterInt> ("beatDivision", "Beat Division", 1, 10, 1));
            parameters.push_back (std::make_unique<AudioParameterInt> ("numNotes", "Number Of Notes", 1, 10, 1));

            return { parameters.begin(), parameters.end() };
        }

       
    //==============================================================================
     void generateBeatMap(int numNotes, int beatDivision, std::vector<int> &beatMap)
    {
        beatMap = std::vector<int>(beatDivision, 0);
        
         for (int i = 0; i < numNotes; i++)
         {
             int x = std::rand() % beatDivision;
             while (beatMap[x] != 0)
             {
                 x = rand() % beatDivision;
             }
             beatMap[x] = 1;
         }
    }
     
    //==============================================================================
    void generateNoteDurations(int beatDivision)
    {
        double offset = (double) 1 / beatDivision;
        for (int i = 0; i < beatMap.size(); i++)
        {
            if (beatMap[i] == 1)
            {
                noteDurations.push_back(offset * i);
            }
        }
    }
    
    //==============================================================================
    void generateBeatPositions(AudioPlayHead::CurrentPositionInfo& info)
    {
        for (double duration : noteDurations)
        {
            beatPositions.push_back(std::ceil(info.ppqPosition) + duration);
        }
    }
    
    //==============================================================================
    void logDoubleVector(std::vector<double> arr)
    {
        std::cout << "[";
        for (const auto item : arr)
        {
            std::cout << item;
            std::cout << ",";
        }
        std::cout << "]" << std::endl;
    }
    
    void logIntVector(std::vector<int> arr)
    {
        std::cout << "[";
        for (const auto item : arr)
        {
            std::cout << item;
            std::cout << ",";
        }
        std::cout << "]" << std::endl;
    }
    
    
    //==============================================================================
    void Reset()
    {
        currentPosition = 0;
        beatMap.clear();
        noteDurations.clear();
        beatPositions.clear();
        newBeat = true;
    }
    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        DBG("PREPARE");
        ignoreUnused (samplesPerBlock);

        notes.clear();
//        currentNote = 0;
//        lastNoteValue = -1;
        time = 0;
        currentPosition = 0;
        newBeat = true;
        noteSent = false;
        rate = sampleRate;
//        prevNumNotes = numNotes->get();
//        prevBeatDivision = beatDivision->get();
    }

    void releaseResources() override {}
    
    //==============================================================================
    void sendNotes(MidiBuffer& midi, AudioPlayHead::CurrentPositionInfo& info, double numSamples)
    {
        int idx = notes.size() == 0 ? 0 : std::rand() % notes.size();
//        DBG("idx: " + std::to_string(idx));
        int noteNumber = notes[idx];
        MidiMessage noteOn = MidiMessage::noteOn(1, noteNumber, (uint8) 127);
        MidiMessage noteOff = MidiMessage::noteOff(1, noteNumber);
        
        // adjust note start to be in correct position
        int noteStart = std::round(nextBeat * (rate * (double) 60.0/info.bpm)) - info.timeInSamples;

        midi.addEvent(noteOn, noteStart);
        midi.addEvent(noteOff, numSamples - 1);
    }
    
    //==============================================================================

    void processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi) override
    {
        AudioPlayHead::CurrentPositionInfo info;
        getPlayHead()->getCurrentPosition(info);
        tempo = info.bpm;
//        jassert (buffer.getNumChannels() == 0);
        auto numSamples = buffer.getNumSamples();
                
        if (!info.isPlaying)
        {
            newBeat = true;
        }
        
        auto numNotesValue = numNotesParameter->load();
        auto beatDivisionValue = beatDivisionParameter->load();
        auto beatsValue = beatsParameter->load();
                                        
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                notes.add(msg.getNoteNumber());
            }
            if (msg.isNoteOff())
            {
                notes.removeValue(msg.getNoteNumber());
            }
        }
        
        midi.clear();
        
        if (!notes.isEmpty() && info.isPlaying)
        {
            if (newBeat)
            {
                
                beatMap.clear();
                noteDurations.clear();
                beatPositions.clear();
                
                generateBeatMap(numNotesValue, beatDivisionValue, beatMap);
                generateNoteDurations(beatDivisionValue);
                generateBeatPositions(info);
                
                newBeat = false;
                
            }
            
            nextBeat = beatPositions[currentPosition];

//            if (blockStart > nextBeat)
//            {
//                nextBeat = blockStart;
//            }
            
            double blockStart = info.ppqPosition;
            double blockEnd = (info.timeInSamples + numSamples) / (rate * ((double) 60.0/info.bpm));

             while (nextBeat >= blockStart && nextBeat < blockEnd)
            {
                
//                DBG("beatDivision: " + std::to_string(*beatDivision));
//                DBG("blockStart: " + std::to_string(info.ppqPosition));
//                DBG("nextBeat:" + std::to_string(nextBeat));
//                DBG("blockEnd: " + std::to_string(blockEnd));
//                DBG("--------------------");

                sendNotes(midi, info, numSamples);
                
                if (numNotesValue == 1)
                {
                    newBeat = true;
                    break;
                }

                if (currentPosition >= noteDurations.size() - 1)
                {
                    currentPosition = 0;
                    newBeat = true;
                    break;
                }
                else
                {
                    currentPosition += 1;
                    nextBeat = noteDurations[currentPosition];
                }
            }
        }
        
        if (notes.isEmpty())
            {
                Reset();
            }
        
    }

    using AudioProcessor::processBlock;

    //==============================================================================
    bool isMidiEffect() const override                     { return true; }

    //==============================================================================
    AudioProcessorEditor* createEditor() override          { return new BeatPeggiatorEditor (*this, parameters); }
    bool hasEditor() const override                        { return true; }

    //==============================================================================
    const String getName() const override                  { return "BeatPeggiator"; }

    bool acceptsMidi() const override                      { return true; }
    bool producesMidi() const override                     { return true; }
    double getTailLengthSeconds() const override           { return 0; }

    //==============================================================================
    int getNumPrograms() override                          { return 1; }
    int getCurrentProgram() override                       { return 0; }
    void setCurrentProgram (int) override                  {}
    const String getProgramName (int) override             { return {}; }
    void changeProgramName (int, const String&) override   {}


    //==============================================================================
    
//    void parameterChanged (const String& parameterID, float newValue) override
//    {
////        DBG("parameterChanged");
////        DBG(parameterID + ": " + std::to_string(newValue));
//    }
    //==============================================================================

    void getStateInformation (MemoryBlock& destData) override
    {
        auto state = parameters.copyState();
        std::unique_ptr<juce::XmlElement> xml (state.createXml());
        copyXmlToBinary (*xml, destData);
    }

    void setStateInformation (const void* data, int sizeInBytes) override
    {
       std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
 
        if (xmlState.get() != nullptr)
            if (xmlState->hasTagName (parameters.state.getType()))
                parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
    }
    

private:
    
   //==============================================================================
    AudioProcessorValueTreeState parameters;
    std::atomic<float>* numNotesParameter = nullptr;
    std::atomic<float>* beatDivisionParameter = nullptr;
    std::atomic<float>* beatsParameter = nullptr;
//    AudioParameterInt* beatDivision;
//    AudioParameterInt* numNotes;
    
    int currentNote, lastNoteValue;
    int prevNumNotes, prevBeatDivision;
    int time;
    double rate;
    SortedSet<int> notes;
    int beats = 1;
    float tempo;
    double nextBeat;
    int currentPosition;
    int noteStartTime;
    bool noteSent;
    bool newBeat;
    std::vector<int> beatMap;
    std::vector<double> noteDurations;
    std::vector<double> beatPositions;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BeatPeggiatorProcessor)
};
