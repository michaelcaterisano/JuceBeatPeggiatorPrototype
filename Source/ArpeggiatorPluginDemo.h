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
        //addParameter (speed = new AudioParameterFloat ("speed", "Arpeggiator Speed", 0.0, 1.0, 0.5));
        addParameter(beatDivision = new AudioParameterInt("beatDivision", "Beat Division", 1, 10, 4));
    }

       
    //==============================================================================
     void generateBeatMap(int &numNotes, int beatDivision, std::vector<int> &beatMap)
    {
//        DBG("generateBeatMap()");
//        DBG("beatDivision: " + std::to_string(beatDivision));
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
//     void generateNoteDurations(float &tempo, int &beats, int beatDivision, std::vector<int> &beatMap)
//     {
//         const double noteLength = (double) beats/beatDivision;
//         double sum = 0.0;
//
//         for (int i = 0; i < beatMap.size(); i++)
//         {
//             if (i == 0 && beatMap[i] == 0)
//             {
//
//                 continue;
//             }
//             else if (i == 0 && beatMap[i] == 1)
//             {
//
//                 noteDurations.push_back(sum);
//             }
//             else if (beatMap[i] == 0)
//             {
//
//                 sum = sum + noteLength;
//             }
//             else if (beatMap[i] == 1)
//             {
//
//                 sum = sum + noteLength;
//                 noteDurations.push_back(sum);
//                 sum = 0.0;
//             }
//         }
//     }
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
        ignoreUnused (samplesPerBlock);

        notes.clear();
//        currentNote = 0;
//        lastNoteValue = -1;
        time = 0;
        currentPosition = 0;
        newBeat = true;
        noteSent = false;
        rate = sampleRate;
        
    }

    void releaseResources() override {}
    
    //==============================================================================
    void sendNotes(MidiBuffer& midi, AudioPlayHead::CurrentPositionInfo& info)
    {
        int idx = notes.size() == 0 ? 0 : std::rand() % notes.size();
        DBG("idx: " + std::to_string(idx));
        int noteNumber = notes[idx];
        MidiMessage noteOn = MidiMessage::noteOn(1, noteNumber, (uint8) 127);
        MidiMessage noteOff = MidiMessage::noteOff(1, noteNumber);

        // adjust note start to be in correct position
        int noteStart = std::round(nextBeat * (rate * (double) 60.0/info.bpm)) - info.timeInSamples;

        midi.addEvent(noteOn, noteStart);
        midi.addEvent(noteOff, 1000);
    }
    
    //==============================================================================

    void processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi) override
    {
        AudioPlayHead::CurrentPositionInfo info;
        getPlayHead()->getCurrentPosition(info);
        
        if (!info.isPlaying)
        {
            newBeat = true;
        }
        
        tempo = info.bpm;
        
        jassert (buffer.getNumChannels() == 0);
        auto numSamples = buffer.getNumSamples();
        
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                notes.add(msg.getNoteNumber());
                DBG("notes: " + std::to_string(notes[0]));
            }
            if (msg.isNoteOff())
            {
                notes.removeValue(msg.getNoteNumber());
            }
        }
        
        midi.clear();
         
        //double noteLength = rate * ((double) 60.0/info.bpm) * ((double) 1/ *beatDivision);
       
        
        
        if (!notes.isEmpty() && info.isPlaying)
        {
            if (newBeat)
            {
                DBG("NEW BEAT/////////////////////");
                
                beatMap.clear();
                noteDurations.clear();
                beatPositions.clear();
                
                generateBeatMap(numNotes, *beatDivision, beatMap);
                generateNoteDurations(*beatDivision);
                generateBeatPositions(info);

                logIntVector(beatMap);
                logDoubleVector(noteDurations);
                logDoubleVector(beatPositions);
                
                
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
                
                DBG("beatDivision: " + std::to_string(*beatDivision));
                DBG("blockStart: " + std::to_string(info.ppqPosition));
                DBG("nextBeat:" + std::to_string(nextBeat));
                DBG("blockEnd: " + std::to_string(blockEnd));
                DBG("--------------------");

                sendNotes(midi, info);
                
                if (numNotes == 1)
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
        MemoryOutputStream (destData, true).writeInt (*beatDivision);
    }

    void setStateInformation (const void* data, int sizeInBytes) override
    {
        beatDivision->setValueNotifyingHost (MemoryInputStream (data, static_cast<size_t> (sizeInBytes), false).readFloat ());
    }

private:
   //==============================================================================
    AudioParameterInt* beatDivision;
    int currentNote, lastNoteValue;
    int time;
    double rate;
    SortedSet<int> notes;
    int numNotes = 3;
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
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Arpeggiator)
};
