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

 name:                  ArpeggiatorPlugin
 version:               1.0.0
 vendor:                JUCE
 website:               http://juce.com
 description:           Arpeggiator audio plugin.

 dependencies:          juce_audio_basics, juce_audio_devices, juce_audio_formats,
                        juce_audio_plugin_client, juce_audio_processors,
                        juce_audio_utils, juce_core, juce_data_structures,
                        juce_events, juce_graphics, juce_gui_basics, juce_gui_extra
 exporters:             xcode_mac, vs2019

 moduleFlags:           JUCE_STRICT_REFCOUNTEDPOINTER=1

 type:                  AudioProcessor
 mainClass:             Arpeggiator

 useLocalCopy:          1

 pluginCharacteristics: pluginWantsMidiIn, pluginProducesMidiOut, pluginIsMidiEffectPlugin

 END_JUCE_PIP_METADATA

*******************************************************************************/

#pragma once
#include <iostream>
#include <array>


//==============================================================================
class Arpeggiator  : public AudioProcessor
{
public:

    //==============================================================================
    Arpeggiator()
        : AudioProcessor (BusesProperties()) // add no audio buses at all
    {
        std::srand(std::time(NULL));
        addParameter (speed = new AudioParameterFloat ("speed", "Arpeggiator Speed", 0.0, 1.0, 0.5));
    }

       
    //==============================================================================
     void generateBeatMap(int &numNotes, int &beatDivision, std::vector<int> &beatMap)
    {
        
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
     void generateNoteDurations(double &sampleRate, int &tempo, int &beats, int &beatDivision, std::vector<int> &beatMap)
     {
         
         const int noteLength = sampleRate * (60.0/tempo) * ((double)beats/beatDivision);
         int sum = noteLength;
         
         for (int i = 0; i < beatMap.size(); i++)
         {
             if (i == 0 && beatMap[i] == 0)
             {
                 continue;
             }
             else if (i == 0 && beatMap[i] == 1)
             {
                 noteStartTimes.push_back(sum);
             }
             else if (beatMap[i] == 0)
             {
                 sum = sum + noteLength;
             }
             else if (beatMap[i] == 1)
             {
                 sum = sum + noteLength;
                 noteStartTimes.push_back(sum);
                 sum = noteLength;
             }
         }
     }
    
    //==============================================================================
    void log(SortedSet<int> arr)
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
    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        ignoreUnused (samplesPerBlock);

        notes.clear();
//        currentNote = 0;
//        lastNoteValue = -1;
        time = 0;
//        currentStartTimeIdx = 0;
//        noteStartTime = 0;
        noteSent = false;
        rate = static_cast<float> (sampleRate);
        
        generateBeatMap(numNotes, beatDivision, beatMap);
        generateNoteDurations(sampleRate, tempo, beats, beatDivision, beatMap);
        
    }

    void releaseResources() override {}
    
    float nextBeat = 0;
    int noteNumber = 50;
    void processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi) override
    {
        AudioPlayHead::CurrentPositionInfo info;
        getPlayHead()->getCurrentPosition(info);
        
//        if (!info.isPlaying)
//        {
//            nextBeat = 0;
//            time = 0;
//            midi.clear();
//        }
        
        jassert (buffer.getNumChannels() == 0);
        auto numSamples = buffer.getNumSamples();
        
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                notes.add(msg.getNoteNumber());
                //DBG(msg.getDescription());
            }
            if (msg.isNoteOff())
            {
                notes.removeValue(msg.getNoteNumber());
            }
        }
        
        //midi.clear();
        
        // nextBeat is initialized to 0
        // numSamples = buffer.getNumSamples();
        // info is AudioPlayHead::CurrentPositionInfo
        
        int beatDivision = 5;
        double noteLength = rate * ((double) 60.0/info.bpm) * ((double) 1/beatDivision);
        double blockStart = info.ppqPosition;
        double blockEnd = (info.timeInSamples + numSamples) / (rate * ((double) 60.0/info.bpm));
        
        while (nextBeat >= blockStart && nextBeat < blockEnd)
        {
            MidiMessage noteOn = MidiMessage::noteOn(1, noteNumber, (uint8) 127);
            MidiMessage noteOff = MidiMessage::noteOff(1, noteNumber);

            midi.addEvent(noteOn, 0);
            midi.addEvent(noteOff, 1000);

            DBG("blockStart: " + std::to_string(blockStart));
            DBG("nextBeat: " + std::to_string(nextBeat));
            DBG("blockEnd: " + std::to_string(blockEnd));
            DBG("--------------------");
            nextBeat += .2;

            break;
        }
        
//        while (nextBeat >= info.timeInSamples && nextBeat < info.timeInSamples + numSamples)
//        {
//            MidiMessage noteOn = MidiMessage::noteOn(1, noteNumber, (uint8) 127);
//            MidiMessage noteOff = MidiMessage::noteOff(1, noteNumber);
//
//            midi.addEvent(noteOn, 0);
//            //midi.addEvent(noteOff, 1000);
//
//            DBG("blockStart: " + std::to_string(info.timeInSamples));
//            DBG("nextBeat: " + std::to_string(nextBeat));
//            DBG("blockEnd: " + std::to_string(info.timeInSamples + numSamples));
//            DBG("numEvents: " + std::to_string(midi.getNumEvents()));
//            DBG("--------------------");
//            nextBeat += noteLength;
//
//            break;
//        }
        

       
    }

    using AudioProcessor::processBlock;

    //==============================================================================
    bool isMidiEffect() const override                     { return true; }

    //==============================================================================
    AudioProcessorEditor* createEditor() override          { return new GenericAudioProcessorEditor (*this); }
    bool hasEditor() const override                        { return true; }

    //==============================================================================
    const String getName() const override                  { return "Arpeggiator"; }

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
    void getStateInformation (MemoryBlock& destData) override
    {
        MemoryOutputStream (destData, true).writeFloat (*speed);
    }

    void setStateInformation (const void* data, int sizeInBytes) override
    {
        speed->setValueNotifyingHost (MemoryInputStream (data, static_cast<size_t> (sizeInBytes), false).readFloat());
    }

private:
   //==============================================================================
    AudioParameterFloat* speed;
    int currentNote, lastNoteValue;
    int time;
    float rate;
    SortedSet<int> notes;
    
    // my vars
    int beatDivision = 5;
    int numNotes = 2;
    int beats = 1;
    int tempo = 120;
    int currentStartTimeIdx;
    int noteStartTime;
    bool noteSent;
    std::vector<int> beatMap = std::vector<int>(beatDivision, 0);
    std::vector<int> noteStartTimes;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Arpeggiator)
};
