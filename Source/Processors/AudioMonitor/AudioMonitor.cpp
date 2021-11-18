/*
    ------------------------------------------------------------------

    This file is part of the Open Ephys GUI
    Copyright (C) 2016 Open Ephys

    ------------------------------------------------------------------

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "AudioMonitor.h"
#include "AudioMonitorEditor.h"
#include <stdio.h>


AudioMonitor::AudioMonitor()
    : GenericProcessor ("Audio Monitor"),
      destBufferSampleRate(44100.0f),
      estimatedSamples(1024)
{

    tempBuffer = std::make_unique<AudioSampleBuffer>();
    
    addBooleanParameter(Parameter::GLOBAL_SCOPE,
                        String("mute_audio"),
                        "Mute audio for this Audio Monitor",
                        false);
    
    addCategoricalParameter(Parameter::GLOBAL_SCOPE,
                            String("audio_output"),
                            "Select L/R or both",
                            { "LEFT", "BOTH", "RIGHT" },
                            1);
    
    addSelectedChannelsParameter(Parameter::STREAM_SCOPE,
                                 String("selected_channels"),
                                 "Channels to monitor",
                                 4);

    for (int i = 0; i < MAX_CHANNELS; i++)
    {
        
        bandpassfilters.add (new Dsp::SmoothedFilterDesign
                         <Dsp::Butterworth::Design::BandPass    // design type
                         <2>,                                   // order
                         1,                                     // number of channels (must be const)
                         Dsp::DirectFormII> (1));               // realization
        
        antialiasingfilters.add (new Dsp::SmoothedFilterDesign<Dsp::RBJ::Design::LowPass, 1> (1024));
    }
}


AudioProcessorEditor* AudioMonitor::createEditor()
{
    editor = std::make_unique<AudioMonitorEditor>(this);

    return editor.get();
}


void AudioMonitor::updateSettings()
{
    updatePlaybackBuffer();
    
    for (auto stream : dataStreams)
    {

        Array<var>* activeChannels = stream->getParameter("selected_channels")->getValue().getArray();
        
        if (activeChannels->size() > 0)
        {
            selectedStream = stream->getStreamId();
            
            for (int i = 0; i < activeChannels->size(); i++)
            {
                updateFilter(i, selectedStream);
            }
        }
    }
}


void AudioMonitor::resetConnections()
{
    GenericProcessor::resetConnections();

    updatePlaybackBuffer();
}


void AudioMonitor::updatePlaybackBuffer()
{
	setPlayConfigDetails(getNumInputs(), getNumOutputs() + 2, 44100.0, 128);
}


void AudioMonitor::prepareToPlay(double sampleRate_, int estimatedSamplesPerBlock)
{
    //("Processor sample rate: ", getSampleRate());
    std::cout << "Audio card sample rate: " << sampleRate_ << std::endl;
    std::cout << "Samples per block: " << estimatedSamplesPerBlock << std::endl;
    
	if (sampleRate_ != destBufferSampleRate || estimatedSamplesPerBlock != estimatedSamples)
	{
		destBufferSampleRate = sampleRate_;
		estimatedSamples = estimatedSamplesPerBlock;
		recreateBuffers();
	}
}


void AudioMonitor::recreateBuffers()
{
	numSamplesExpected.clear();
    sourceBufferSampleRate.clear();
    
    ratio.clear();
    
    for (int i = 0; i < getNumInputs(); i++)
    {
        numSamplesExpected.emplace(i,
                                   continuousChannels[i]->getSampleRate()
                                   / destBufferSampleRate
                                   * estimatedSamples);
        
        sourceBufferSampleRate.emplace(i, continuousChannels[i]->getSampleRate());
        
        ratio.emplace(i, sourceBufferSampleRate[i]/destBufferSampleRate);
        
        std::cout << "Incoming sample rate " << i << ": " << sourceBufferSampleRate[i] << std::endl;
        std::cout << "Ratio " << i << ": " << ratio[i] << std::endl;
        std::cout << "Samples expected " << i << ": " << numSamplesExpected[i] << std::endl;
    }

    samplesInBackupBuffer.clear();
    samplesInOverflowBuffer.clear();
    
    bufferA.clear();
    bufferB.clear();
    bufferSwap.clear();
    
    for (int i = 0; i < MAX_CHANNELS; i++)
    {

        samplesInBackupBuffer.emplace(i, 0.0f);
        samplesInOverflowBuffer.emplace(i, 0.0f);

        bufferA.emplace(i, std::make_unique<AudioBuffer<float>>(1,44100));
        bufferB.emplace(i, std::make_unique<AudioBuffer<float>>(1,44100));
        bufferSwap.emplace(i, false);

    }

    tempBuffer->setSize(1, 4096);
}


void AudioMonitor::parameterValueChanged(Parameter* param)
{
    std::cout << "---> Value changed for " << param->getName() << " : " << (int) param->getValue() << std::endl;


    if (param->getName().equalsIgnoreCase("selected_channels"))
    {
        
        selectedStream = param->getStreamId();
        
        Array<var>* activeChannels = param->getValue().getArray();

        for (int i = 0; i < activeChannels->size(); i++)
        {
            
            int localIndex =(int) activeChannels->getReference(i);
            
            int globalIndex = getDataStream(selectedStream)->getContinuousChannels()[localIndex]->getGlobalIndex();
            
             std::cout << "CHANNEL " << i << " stream " << selectedStream <<  " : " << localIndex << " : " << globalIndex << std::endl;
            
            updateFilter(i, selectedStream);
        }
        
        // clear monitored channels on all other streams
        for (auto stream : dataStreams)
        {
            if (stream->getStreamId() != selectedStream)
            {
                stream->getParameter("selected_channels")->currentValue = Array<var>();
            }
        }
    }
}


void AudioMonitor::updateFilter(int i, uint16 streamId)
{

    Dsp::Params params1;
    params1[0] = getDataStream(streamId)->getSampleRate(); // sample rate
    params1[1] = 2;                          // order
    params1[2] = (7000 + 300) / 2;     // center frequency
    params1[3] = 7000 - 300;           // bandwidth

    bandpassfilters[i]->setParams (params1);
    
    double cutoffFreq = destBufferSampleRate / 2; // upsample

    double sampleFreq = destBufferSampleRate;  // upsample

    Dsp::Params params2;
    params2[0] = sampleFreq; // sample rate
    params2[1] = cutoffFreq; // cutoff frequency
    params2[2] = 1.25; //Q //

    antialiasingfilters[i]->setParams(params2);

}

void AudioMonitor::handleBroadcastMessage(String msg)
{
    
    StringArray parts = StringArray::fromTokens(msg, " ", "");

    if (parts[0].equalsIgnoreCase("AUDIO"))
    {
        if (parts.size() > 1)
        {
            String command = parts[1];

            if (command.equalsIgnoreCase("SELECT"))
            {
                if (parts.size() == 4)
                {
                    uint16 streamId = parts[2].getIntValue();
                    
                    DataStream* stream = getDataStream(streamId);
                    
                    if (stream != nullptr)
                    {
                        
                        int localChannel = parts[3].getIntValue() - 1;
                        
                        if (localChannel >= 0 && localChannel < stream->getContinuousChannels().size())
                        {
                            Array<var> ch;
                            ch.add(localChannel);
                            
                            stream->getParameter("selected_channels")->setNextValue(ch);
                        }
                    }
                }
            }
        }
    }
}

void AudioMonitor::process (AudioBuffer<float>& buffer)
{
    
    int valuesNeeded = buffer.getNumSamples(); // samples needed to fill the complete buffer
    
    int totalBufferChannels = buffer.getNumChannels();

    // clear the left and right channels (last two channels)
    buffer.clear(totalBufferChannels - 2, 0, buffer.getNumSamples());
    buffer.clear(totalBufferChannels - 1, 0, buffer.getNumSamples());

    if (!getParameter("mute_audio")->getValue())
    {

        for (auto stream : dataStreams)
        {
            
            if (stream->getStreamId() == selectedStream)
            {
                
                AudioSampleBuffer* overflowBuffer;
                AudioSampleBuffer* backupBuffer;

                Array<var>* activeChannels = stream->getParameter("selected_channels")->getValue().getArray();

                for (int i = 0; i < activeChannels->size(); i++)
                {

                    int localIndex = (int) activeChannels->getReference(i);
                    
                    int globalIndex = getDataStream(selectedStream)->getContinuousChannels()[localIndex]->getGlobalIndex();
                    
                    tempBuffer->clear();

                    if (!bufferSwap[i])
                    {
                        overflowBuffer = bufferA[i].get();
                        backupBuffer = bufferB[i].get();

                        bufferSwap[i] = true;
                    }
                    else
                    {
                        overflowBuffer = bufferB[i].get();
                        backupBuffer = bufferA[i].get();

                        bufferSwap[i] = false;
                    }

                    backupBuffer->clear();

                    samplesInOverflowBuffer[i] = samplesInBackupBuffer[i]; // size of buffer after last round
                    samplesInBackupBuffer[i] = 0;

                    double orphanedSamples = 0;

                    // 1. copy overflow buffer

                    double samplesToCopyFromOverflowBuffer =
                        ((samplesInOverflowBuffer[i] <= numSamplesExpected[globalIndex]) ?
                            samplesInOverflowBuffer[i] :
                            numSamplesExpected[globalIndex]);

                    // LOGD("Number of samples to copy: ", samplesToCopyFromOverflowBuffer);
                    
                    //std::cout << "Copying from overflow buffer: " << samplesToCopyFromOverflowBuffer << std::endl;

                    if (samplesToCopyFromOverflowBuffer > 0) // need to re-add samples from backup buffer
                    {

                        tempBuffer->addFrom(0,    // destination channel
                            0,                // destination start sample
                            *overflowBuffer,  // source
                            i,                // source channel
                            0,                // source start sample
                            (int) samplesToCopyFromOverflowBuffer,    // number of samples
                            1.0f              // gain to apply
                        );

                        double leftoverSamples = samplesInOverflowBuffer[i] - samplesToCopyFromOverflowBuffer;
                        
                        //std::cout << "Copying to backup buffer: " << leftoverSamples << std::endl;

                        if (leftoverSamples > 0) // move remaining samples to the backup buffer
                        {

                            backupBuffer->addFrom(0, // destination channel
                                0,                     // destination start sample
                                *overflowBuffer,       // source
                                i,                     // source channel
                                (int) samplesToCopyFromOverflowBuffer,         // source start sample
                                (int) leftoverSamples,       // number of samples
                                1.0f                   // gain to apply
                            );
                        }

                        samplesInBackupBuffer[i] = leftoverSamples;
                    }
                    
                    double remainingSamples = double(numSamplesExpected[globalIndex]) - samplesToCopyFromOverflowBuffer;

                    double samplesAvailable = double(getNumSourceSamples(selectedStream));
                    
                    //std::cout << "Remaining samples: " << remainingSamples << std::endl;
                    
                    //std::cout << "Samples available: " << samplesAvailable << std::endl;


                    double samplesToCopyFromIncomingBuffer = ((remainingSamples <= samplesAvailable) ?
                        remainingSamples :
                        samplesAvailable);
                    
                    //std::cout << "Copying from incoming buffer: " << samplesToCopyFromIncomingBuffer << std::endl;

                    if (samplesToCopyFromIncomingBuffer > 0)
                    {

                        tempBuffer->addFrom(0,                  // destination channel
                            (int) samplesToCopyFromOverflowBuffer,    // destination start sample
                            buffer,                             // source
                            globalIndex,                        // source channel
                            0,                                  // source start sample
                            (int) samplesToCopyFromIncomingBuffer,    //  number of samples
                            1.0f                                // gain to apply
                        );

                    }

                    orphanedSamples = samplesAvailable - samplesToCopyFromIncomingBuffer;
                    
                    //std::cout << "Orphaned samples: " << orphanedSamples << std::endl;

                    if (orphanedSamples > 0 && (samplesInBackupBuffer[i] + orphanedSamples < backupBuffer->getNumSamples()))
                    {

                        backupBuffer->addFrom(0,          // destination channel
                            samplesInBackupBuffer[i],     // destination start sample
                            buffer,                       // source
                            globalIndex,                  // source channel
                            (int) remainingSamples,             // source start sample
                            (int) orphanedSamples,              //  number of samples
                            1.0f                          // gain to apply
                        );

                        samplesInBackupBuffer[i] = samplesInBackupBuffer[i] + orphanedSamples;

                    }
                    
                   // std::cout << "Total copied: " << samplesToCopyFromOverflowBuffer + samplesToCopyFromIncomingBuffer << std::endl;

                    // now that our tempBuffer is ready, we can filter it and copy it into the
                    // original buffer

                   // LOGD("Ratio = ", ratio[channelIndex], ", gain = ", gain);
                   // LOGD("Values needed = ", valuesNeeded, ", channel = ", channelIndex);

                    float* ptr = tempBuffer->getWritePointer(0);
                    
                    int totalCopied = int(samplesToCopyFromOverflowBuffer + samplesToCopyFromIncomingBuffer);
                    
                    bandpassfilters[i]->process(totalCopied, &ptr);

                    // initialize variables
                    int sourceBufferPos = 0;
                    int sourceBufferSize = totalCopied;
                    double subSampleOffset = 0.0;
                    int nextPos = (sourceBufferPos + 1) % sourceBufferSize;

                    double destBufferPos;
                    int targetChannel;

                    if (int(getParameter("audio_output")->getValue()) == 0 || int(getParameter("audio_output")->getValue()) == 2)
                        targetChannel = totalBufferChannels - 2;
                    else
                        targetChannel = totalBufferChannels - 1;

                    // code modified from "juce_ResamplingAudioSource.cpp":
                    for (destBufferPos = 0; destBufferPos < valuesNeeded; destBufferPos++)
                    {
                        float alpha = (float)subSampleOffset;
                        float invAlpha = 1.0f - alpha;

                        buffer.addFrom(targetChannel,    // destChannel
                            destBufferPos,               // destSampleOffset
                            *tempBuffer,                 // source
                            0,                           // sourceChannel
                            sourceBufferPos,             // sourceSampleOffset
                            1,                           // number of samples
                            invAlpha);                   // gain to apply to source
                        
                        buffer.addFrom(targetChannel,    // destChannel
                            destBufferPos,               // destSampleOffset
                            *tempBuffer,                 // source
                            0,                           // sourceChannel
                            nextPos,                     // sourceSampleOffset
                            1,                           // number of samples
                            alpha);                      // gain to apply to source

                        subSampleOffset += ratio[i];

                        while (subSampleOffset >= 1.0)
                        {
                            if (++sourceBufferPos >= sourceBufferSize)
                                sourceBufferPos = 0;

                            nextPos = (sourceBufferPos + 1) % sourceBufferSize;
                            subSampleOffset -= 1.0;
                        }
                    }
                    
                    //std::cout << "After upsampling: " << valuesNeeded << std::endl;
                    
                    //std::cout << std::endl;

                    ptr = buffer.getWritePointer(targetChannel);
                    antialiasingfilters[i]->process(destBufferPos, &ptr);
                    
                } // end cycling through channels

                if (int(getParameter("audio_output")->getValue()) == 2)
                {
                    // copy the signal into the right channel
                    buffer.addFrom(totalBufferChannels - 1,    // destChannel
                        0,                                     // destSampleOffset
                        buffer,                                // source
                        totalBufferChannels - 2,               // sourceChannel
                        0,                                     // sourceSampleOffset
                        valuesNeeded,                          // number of samples
                        1.0);                                  // gain to apply to source

                }
                
            } // stream is selected
        
        } // loop through streams

    } // not muted

} // process
