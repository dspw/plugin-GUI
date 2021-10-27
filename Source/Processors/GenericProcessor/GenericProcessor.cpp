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
#include "GenericProcessor.h"

#include "../../AccessClass.h"
#include "../../Utils/Utils.h"
#include "../Editors/GenericEditor.h"

#include "../Settings/DataStream.h"
#include "../Settings/ProcessorInfo.h"
#include "../Settings/ConfigurationObject.h"
#include "../Settings/DeviceInfo.h"

#include "../Splitter/Splitter.h"

#include "../Events/Event.h"
#include "../Events/Spike.h"

#include "../../Processors/MessageCenter/MessageCenterEditor.h"

#include <exception>

const String GenericProcessor::m_unusedNameString("xxx-UNUSED-OPEN-EPHYS-xxx");

GenericProcessor::GenericProcessor(const String& name)
	: GenericProcessorBase(name)
	, sourceNode(nullptr)
	, destNode(nullptr)
	, isEnabled(true)
	, wasConnected(false)
	, nextAvailableChannel(0)
	, saveOrder(-1)
	, loadOrder(-1)
	, currentChannel(-1)
	, editor(nullptr)
	, parametersAsXml(nullptr)
	, ttlEventChannel(nullptr)
	, sendSampleCount(true)
	, m_processorType(PROCESSOR_TYPE_UTILITY)
	, m_name(name)
	, m_paramsWereLoaded(false)

{
	latencyMeter = std::make_unique<LatencyMeter>(this);

	//addBooleanParameter("enable_stream",
	//	"Determines whether or not processing is enabled for a particular stream",
	//	true, true);
}


GenericProcessor::~GenericProcessor()
{
}


AudioProcessorEditor* GenericProcessor::createEditor()
{
	editor = std::make_unique<GenericEditor>(this);

	return editor.get();
}


void GenericProcessor::setNodeId(int id)
{
	nodeId = id;

	if (editor != 0)
	{
		editor->updateName();
	}
}



Parameter* GenericProcessor::getParameter(const String& name)
{
	for (auto param : availableParameters)
	{
		if (param->getName().equalsIgnoreCase(name))
			return param;
	}

	std::cout << "Could not find parameter named " << name << std::endl;

	return nullptr;
}

Parameter* GenericProcessor::getParameter(uint16 streamId, const String& name)
{

    std::cout << "Getting " << name << " for streamId " << streamId << std::endl;

    // no checking, so it's fast; but take care to provide a valid stream / name
    return streamParameterMap[streamId][name];

}

Parameter* GenericProcessor::getParameter(EventChannel* eventChannel, const String& name)
{
    // no checking, so it's fast; but take care to provide a valid stream / name
    return eventChannelParameterMap[eventChannel][name];

}


Parameter* GenericProcessor::getParameter(ContinuousChannel* continuousChannel, const String& name)
{
    // no checking, so it's fast; but take care to provide a valid stream / name
    return continuousChannelParameterMap[continuousChannel][name];

}


Parameter* GenericProcessor::getParameter(SpikeChannel* spikeChannel, const String& name)
{
    // no checking, so it's fast; but take care to provide a valid stream / name
    return spikeChannelParameterMap[spikeChannel][name];

}

Array<Parameter*> GenericProcessor::getParameters()
{
    Array<Parameter*> params;
    
    for (const auto & [key, value] : globalParameterMap)
    {
        params.add(value);
    }
    
    return params;
}

Array<Parameter*> GenericProcessor::getParameters(uint16 streamId)
{
    Array<Parameter*> params;
    
    for (const auto & [key, value] : streamParameterMap[streamId])
    {
        params.add(value);
    }
    
    return params;
}


Array<Parameter*> GenericProcessor::getParameters(EventChannel* eventChannel)
{
    Array<Parameter*> params;
    
    for (const auto & [key, value] : eventChannelParameterMap[eventChannel])
    {
        params.add(value);
    }
    
    return params;
}

Array<Parameter*> GenericProcessor::getParameters(ContinuousChannel* continuousChannel)
{
    Array<Parameter*> params;
    
    for (const auto & [key, value] : continuousChannelParameterMap[continuousChannel])
    {
        params.add(value);
    }
    
    return params;
}

Array<Parameter*> GenericProcessor::getParameters(SpikeChannel* spikeChannel)
{
    Array<Parameter*> params;
    
    for (const auto & [key, value] : spikeChannelParameterMap[spikeChannel])
    {
        params.add(value);
    }
    
    return params;
}



void GenericProcessor::addBooleanParameter(
    Parameter::ParameterScope scope,
    const String& name,
	const String& description,
	bool defaultValue,
	bool deactivateDuringAcquisition)
{

	BooleanParameter* p = new BooleanParameter(
		this, 
		scope,
		name, 
		description, 
		defaultValue, 
		deactivateDuringAcquisition);

	availableParameters.add(p);

	if (scope == Parameter::GLOBAL_SCOPE)
	{
		globalParameterMap[p->getName()] = p;
	}

}

void GenericProcessor::addCategoricalParameter(
    Parameter::ParameterScope scope,
    const String& name,
	const String& description,
	StringArray categories,
	int defaultIndex,
	bool deactivateDuringAcquisition)
{

	CategoricalParameter* p = new CategoricalParameter(
		this, 
		scope,
		name, 
		description, 
		categories, 
		defaultIndex, 
		deactivateDuringAcquisition);

	availableParameters.add(p);
	
	if (scope == Parameter::GLOBAL_SCOPE)
	{
		globalParameterMap[p->getName()] = p;
	}

}

void GenericProcessor::addIntParameter(
    Parameter::ParameterScope scope,
    const String& name,
    const String& description,
	int defaultValue,
	int minValue,
	int maxValue,
	bool deactivateDuringAcquisition)
{

	IntParameter* p = 
		new IntParameter(this, 
			scope,
			name, 
			description, 
			defaultValue, 
			minValue, 
			maxValue, 
			deactivateDuringAcquisition);

	availableParameters.add(p);

	if (scope == Parameter::GLOBAL_SCOPE)
	{
		globalParameterMap[p->getName()] = p;
	}

}

void GenericProcessor::addStringParameter(
    Parameter::ParameterScope scope,
    const String& name,
    const String& description,
    String defaultValue,
    bool deactivateDuringAcquisition)
{
    StringParameter* p =
        new StringParameter(this,
            scope,
            name,
            description,
            defaultValue,
            deactivateDuringAcquisition);

    availableParameters.add(p);

    if (scope == Parameter::GLOBAL_SCOPE)
    {
        globalParameterMap[p->getName()] = p;
    }
}

void GenericProcessor::addFloatParameter(
	Parameter::ParameterScope scope,
    const String& name,
    const String& description,
	float defaultValue,
	float minValue,
	float maxValue,
	float stepSize,
	bool deactivateDuringAcquisition)
{

	FloatParameter* p =
		new FloatParameter(this,
			scope,
			name,
			description,
			defaultValue,
			minValue,
			maxValue,
			stepSize,
			deactivateDuringAcquisition);

	availableParameters.add(p);

	if (scope == Parameter::GLOBAL_SCOPE)
	{
		globalParameterMap[p->getName()] = p;
	}

}

void GenericProcessor::addSelectedChannelsParameter(
	Parameter::ParameterScope scope,
    const String& name,
    const String& description,
	int maxSelectedChannels,
	bool deactivateDuringAcquisition)
{

	Array<var> defaultValue;

	SelectedChannelsParameter* p =
		new SelectedChannelsParameter(this,
			scope,
			name,
			description,
			defaultValue,
			maxSelectedChannels,
			deactivateDuringAcquisition);

	availableParameters.add(p);

	if (scope == Parameter::GLOBAL_SCOPE)
	{
		globalParameterMap[p->getName()] = p;
	}
}

void GenericProcessor::parameterChangeRequest(Parameter* param)
{
	currentParameter = param;

	setParameter(-1, 0.0f);

	getEditor()->updateView();
}

void GenericProcessor::setParameter(int parameterIndex, float newValue)
{
	if (currentParameter != nullptr)
	{
		currentParameter->updateValue();
		parameterValueChanged(currentParameter);
	}
	
}


int GenericProcessor::getNextChannel(bool increment)
{
	int chan = nextAvailableChannel;

	LOGDD("Next channel: ", chan, ", num inputs: ", getNumInputs());

	if (increment)
		nextAvailableChannel++;

	if (chan < getNumInputs())
		return chan;
	else
		return -1;
}


void GenericProcessor::resetConnections()
{
	nextAvailableChannel = 0;

	wasConnected = false;
}


void GenericProcessor::setSourceNode(GenericProcessor* sn)
{
    if (this->isMerger())
        setMergerSourceNode(sn);
    else
        sourceNode = sn;
}


void GenericProcessor::setDestNode(GenericProcessor* dn)
{
	
    if (isSplitter())
    {
        setSplitterDestNode(dn);
    }
    else
    {
        destNode = dn;
    }

}


void GenericProcessor::clearSettings()
{
	LOGDD("Generic processor clearing settings.");

	continuousChannels.clear();
	eventChannels.clear();
	spikeChannels.clear();
	configurationObjects.clear();
	dataStreams.clear();

	timestamps.clear();
	numSamples.clear();
	processStartTimes.clear();

}

void GenericProcessor::copyDataStreamSettings(const DataStream* stream)
{

	if (false)
	{
		std::cout << "Copying stream: " << std::endl;
		std::cout << "  Source Node ID: " << stream->getSourceNodeId() << std::endl;
		std::cout << "  Source Node Name: " << stream->getSourceNodeName() << std::endl;
		std::cout << "  Last Node ID: " << stream->getNodeId() << std::endl;
		std::cout << "  Last Node Name: " << stream->getNodeName() << std::endl;
		std::cout << "  Name: " << stream->getName() << std::endl;
		std::cout << "  Stream ID: " << stream->getStreamId() << std::endl;
		std::cout << "  Sample rate: " << stream->getSampleRate() << std::endl;
		std::cout << "  Channel count: " << stream->getChannelCount() << std::endl;
		std::cout << "  " << std::endl;
	}
	

	dataStreams.add(new DataStream(*stream)); 

	dataStreams.getLast()->clearChannels();
	dataStreams.getLast()->addProcessor(processorInfo.get());
	
	for (auto continuousChannel : stream->getContinuousChannels())
	{

		/*std::cout << "Copying continuous channel: " << std::endl;
		std::cout << "  Source Node ID: " << continuousChannel->getSourceNodeId() << std::endl;
		std::cout << "  Source Node Name: " << continuousChannel->getSourceNodeName() << std::endl;
		std::cout << "  Last Node ID: " << continuousChannel->getNodeId() << std::endl;
		std::cout << "  Last Node Name: " << continuousChannel->getNodeName() << std::endl;
		std::cout << "  Name: " << continuousChannel->getName() << std::endl;
		std::cout << "  Stream ID: " << continuousChannel->getStreamId() << std::endl;
		std::cout << "  Sample rate: " << continuousChannel->getSampleRate() << std::endl;*/

		//std::cout << "Input channel: " << continuousChannel->getUniqueId().toString() << std::endl;

		continuousChannels.add(new ContinuousChannel(*continuousChannel));
		continuousChannels.getLast()->addProcessor(processorInfo.get());
		dataStreams.getLast()->addChannel(continuousChannels.getLast());

		//std::cout << "Output channel: " << continuousChannels.getLast()->getUniqueId().toString() << std::endl;

		//std::cout << "Copying " << continuousChannel->getNodeName() << " " << continuousChannel->getName() << std::endl;
	}

	for (auto eventChannel : stream->getEventChannels())
	{

		if (false)
		{
			std::cout << "Copying event channel: " << std::endl;
			std::cout << "  Source Node ID: " << eventChannel->getSourceNodeId() << std::endl;
			std::cout << "  Source Node Name: " << eventChannel->getSourceNodeName() << std::endl;
			std::cout << "  Last Node ID: " << eventChannel->getNodeId() << std::endl;
			std::cout << "  Last Node Name: " << eventChannel->getNodeName() << std::endl;
			std::cout << "  Name: " << eventChannel->getName() << std::endl;
			std::cout << "  ID: " << eventChannel->getStreamId() << std::endl;
			std::cout << "  Sample rate: " << eventChannel->getSampleRate() << std::endl;
		}

		eventChannels.add(new EventChannel(*eventChannel));
		eventChannels.getLast()->addProcessor(processorInfo.get());
		dataStreams.getLast()->addChannel(eventChannels.getLast());
	}

	for (auto spikeChannel : stream->getSpikeChannels())
	{

		std::cout << "Copying spike channel: " << std::endl;
		std::cout << "  Source Node ID: " << spikeChannel->getSourceNodeId() << std::endl;
		std::cout << "  Source Node Name: " << spikeChannel->getSourceNodeName() << std::endl;
		std::cout << "  Last Node ID: " << spikeChannel->getNodeId() << std::endl;
		std::cout << "  Last Node Name: " << spikeChannel->getNodeName() << std::endl;
		std::cout << "  Name: " << spikeChannel->getName() << std::endl;
		std::cout << "  ID: " << spikeChannel->getStreamId() << std::endl;
		std::cout << "  Sample rate: " << spikeChannel->getSampleRate() << std::endl;

		spikeChannels.add(new SpikeChannel(*spikeChannel));
		spikeChannels.getLast()->addProcessor(processorInfo.get());
		dataStreams.getLast()->addChannel(spikeChannels.getLast());
	}
}

void GenericProcessor::updateDisplayName(String name)
{
	m_name = name;
}


void GenericProcessor::update()
{
	LOGD(getName(), " updating settings.");

	clearSettings();

	processorInfo.reset();
	processorInfo = std::unique_ptr<ProcessorInfoObject>(new ProcessorInfoObject(this));
   
	if (sourceNode != nullptr)
	{
		// copy settings from source node
		const EventChannel* messageChannel = sourceNode->getMessageChannel();

		if (messageChannel != nullptr)
		{
			eventChannels.add(new EventChannel(*messageChannel));
			eventChannels.getLast()->addProcessor(processorInfo.get());
		}

		if (!isMerger())
		{

			if (sourceNode->isSplitter())
			{
				Splitter* splitter = (Splitter*)sourceNode;

				for (auto stream : splitter->getStreamsForDestNode(this))
				{
					copyDataStreamSettings(stream);
				}
			}
			else {
				for (auto stream : sourceNode->getStreamsForDestNode(this))
				{
					copyDataStreamSettings(stream);
				}
			}

			for (auto configurationObject : sourceNode->configurationObjects)
			{
				configurationObjects.add(new ConfigurationObject(*configurationObject));
			}

			isEnabled = sourceNode->isEnabled;
		}
	}
	else
	{
		// connect first processor in signal chain to message center

		const EventChannel* messageChannel = AccessClass::getMessageCenter()->messageCenter->getMessageChannel();

		eventChannels.add(new EventChannel(*messageChannel));
		eventChannels.getLast()->addProcessor(processorInfo.get());

		LOGD(getNodeId(), " connected to Message Center");
	}

	updateChannelIndexMaps();

    /// UPDATE PARAMETERS FOR STREAMS
	for (auto stream : getDataStreams())
	{
		LOGD("Stream ", stream->getStreamId(), " num channels: ", stream->getChannelCount());

		if (streamParameterMap.find(stream->getStreamId()) == streamParameterMap.end())
		{

			streamParameterMap[stream->getStreamId()] = std::map<String, Parameter*>();

			for (auto param : availableParameters)
			{
				if (param->getType() == Parameter::BOOLEAN_PARAM)
				{
					if (param->getScope() == Parameter::STREAM_SCOPE)
					{
						BooleanParameter* p = (BooleanParameter*)param;
						parameters.add(new BooleanParameter(
							this,
							param->getScope(),
							p->getName(),
							p->getDescription(),
							p->getBoolValue(),
							p->shouldDeactivateDuringAcquisition()
						));

						streamParameterMap[stream->getStreamId()][param->getName()] = parameters.getLast();
                        parameters.getLast()->setStreamId(stream->getStreamId());
					}

				}
				else if (param->getType() == Parameter::INT_PARAM)
				{

					if (param->getScope() == Parameter::STREAM_SCOPE)
                    {
						IntParameter* p = (IntParameter*)param;
						parameters.add(new IntParameter(
							this,
							param->getScope(),
							p->getName(),
							p->getDescription(),
							p->getIntValue(),
							p->getMinValue(),
							p->getMaxValue(),
							p->shouldDeactivateDuringAcquisition()
						));

						streamParameterMap[stream->getStreamId()][param->getName()] = parameters.getLast();
                        parameters.getLast()->setStreamId(stream->getStreamId());
					}
				}
                else if (param->getType() == Parameter::STRING_PARAM)
                {

                    if (param->getScope() == Parameter::STREAM_SCOPE)
                    {
                        StringParameter* p = (StringParameter*)param;
                        parameters.add(new StringParameter(
                            this,
                            param->getScope(),
                            p->getName(),
                            p->getDescription(),
                            p->getStringValue(),
                            p->shouldDeactivateDuringAcquisition()
                        ));

                        streamParameterMap[stream->getStreamId()][param->getName()] = parameters.getLast();
                        parameters.getLast()->setStreamId(stream->getStreamId());
                    }
                }
				else if (param->getType() == Parameter::FLOAT_PARAM)
				{

					if (param->getScope() == Parameter::STREAM_SCOPE)
                    {
						FloatParameter* p = (FloatParameter*)param;
						parameters.add(new FloatParameter(
							this,
							param->getScope(),
							p->getName(),
							p->getDescription(),
							p->getFloatValue(),
							p->getMinValue(),
							p->getMaxValue(),
							p->getStepSize(),
							p->shouldDeactivateDuringAcquisition()
						));

						streamParameterMap[stream->getStreamId()][param->getName()] = parameters.getLast();
                        parameters.getLast()->setStreamId(stream->getStreamId());
					}

				}
				else if (param->getType() == Parameter::CATEGORICAL_PARAM)
				{

					if (param->getScope() == Parameter::STREAM_SCOPE)
					{
						CategoricalParameter* p = (CategoricalParameter*)param;
						parameters.add(new CategoricalParameter(
							this,
							param->getScope(),
							p->getName(),
							p->getDescription(),
							p->getCategories(),
							p->getSelectedIndex(),
							p->shouldDeactivateDuringAcquisition()
						));

						streamParameterMap[stream->getStreamId()][param->getName()] = parameters.getLast();
                        parameters.getLast()->setStreamId(stream->getStreamId());
					}

				}
				else if (param->getType() == Parameter::SELECTED_CHANNELS_PARAM)
				{

					if (param->getScope() == Parameter::STREAM_SCOPE)
                    {
						SelectedChannelsParameter* p = (SelectedChannelsParameter*)param;

						SelectedChannelsParameter* newParam = new SelectedChannelsParameter(
							this,
							param->getScope(),
							p->getName(),
							p->getDescription(),
							p->getValue(),
							p->getMaxSelectableChannels(),
							p->shouldDeactivateDuringAcquisition()
						);

						newParam->setChannelCount(stream->getChannelCount());

						parameters.add(newParam);

						streamParameterMap[stream->getStreamId()][param->getName()] = parameters.getLast();
                        parameters.getLast()->setStreamId(stream->getStreamId());

					}
				}
			}
		}
		else {
			for (auto param : availableParameters)
			{
				if (param->getType() == Parameter::SELECTED_CHANNELS_PARAM)
				{
					if (param->getScope() == Parameter::STREAM_SCOPE)
                    {
                        SelectedChannelsParameter* p = (SelectedChannelsParameter*) getParameter(stream->getStreamId(), param->getName());
                        p->setChannelCount(stream->getChannelCount());
                    }
					else
                    {
                        SelectedChannelsParameter* p = (SelectedChannelsParameter*) getParameter(param->getName());
                        p->setChannelCount(getNumOutputs());
                    }
						
				}
			}
		}
	}


	updateSettings(); // allow processors to change custom settings, 
					  // including creation of streams / channels and
					  // setting isEnabled variable

	LOGD("Updated custom settings.");

	updateChannelIndexMaps();

	m_needsToSendTimestampMessages.clear();
	for (auto stream : getDataStreams())
		m_needsToSendTimestampMessages[stream->getStreamId()] = true;

	// required for the ProcessorGraph to know the
	// details of this processor:
	setPlayConfigDetails(getNumInputs(),  // numIns
		getNumOutputs(), // numOuts
		44100.0,         // sampleRate (always 44100 Hz, default audio card rate)
		128);            // blockSize

	editor->update(isEnabled); // allow the editor to update its settings
}

void GenericProcessor::updateChannelIndexMaps()
{
	continuousChannelMap.clear();
	eventChannelMap.clear();
	spikeChannelMap.clear();
	dataStreamMap.clear();

	for (int i = 0; i < continuousChannels.size(); i++)
	{
		ContinuousChannel* chan = continuousChannels[i];
		chan->setGlobalIndex(i);

		uint16 processorId = chan->getSourceNodeId();
		uint16 streamId = chan->getStreamId();
		uint16 localIndex = chan->getLocalIndex();

		continuousChannelMap[processorId][streamId][localIndex] = chan;
	}

	for (int i = 0; i < eventChannels.size(); i++)
	{
		EventChannel* chan = eventChannels[i];
		chan->setGlobalIndex(i);

		uint16 processorId = chan->getSourceNodeId();
		uint16 streamId = chan->getStreamId();
		uint16 localIndex = chan->getLocalIndex();

		eventChannelMap[processorId][streamId][localIndex] = chan;
	}

	for (int i = 0; i < spikeChannels.size(); i++)
	{
		SpikeChannel* chan = spikeChannels[i];
		chan->setGlobalIndex(i);

		uint16 processorId = chan->getSourceNodeId();
		uint16 streamId = chan->getStreamId();
		uint16 localIndex = chan->getLocalIndex();

		spikeChannelMap[processorId][streamId][localIndex] = chan;
	}

	for (int i = 0; i < dataStreams.size(); i++)
	{
		DataStream* stream = dataStreams[i];

		uint16 streamId = stream->getStreamId();

		dataStreamMap[streamId] = stream;
	}
	
	latencyMeter->update(getDataStreams());
}

String GenericProcessor::handleConfigMessage(String msg)
{
	return "";
}


void GenericProcessor::getDefaultEventInfo(Array<DefaultEventInfo>& events, int subproc) const
{
	events.clear();
}


/** Used to get the number of samples in a given buffer, for a given channel. */
uint32 GenericProcessor::getNumSamples(int channelNum) const
{
	if (channelNum >= 0
		&& channelNum < continuousChannels.size())
	{
		uint16 streamId = continuousChannels[channelNum]->getStreamId();

		try
		{
			return numSamples.at(streamId);;
		}
		catch (...)
		{
			return 0;
		}
	}

	return 0;
}


/** Used to get the timestamp for a given buffer, for a given source node. */
juce::uint64 GenericProcessor::getTimestamp(int channelNum) const
{

	if (channelNum >= 0
		&& channelNum < continuousChannels.size())
	{
		uint16 streamId = continuousChannels[channelNum]->getStreamId();

		try
		{
			return timestamps.at(streamId);;
		}
		catch (...)
		{
			return 0;
		}
	}

	return 0;
}

uint32 GenericProcessor::getNumSourceSamples(uint16 streamId) const
{
	return numSamples.at(streamId);
}

juce::uint64 GenericProcessor::getSourceTimestamp(uint16 streamId) const
{
	return timestamps.at(streamId);
}


void GenericProcessor::setTimestampAndSamples(juce::uint64 timestamp, uint32 nSamples, uint16 streamId)
{

	MidiBuffer& eventBuffer = *m_currentMidiBuffer;
	LOGDD("Setting timestamp to ", timestamp);

	HeapBlock<char> data;
	size_t dataSize = SystemEvent::fillTimestampAndSamplesData(data, 
		this, 
		streamId, 
		timestamp, 
		nSamples,
		m_initialProcessTime);

	eventBuffer.addEvent(data, dataSize, 0);

	//since the processor generating the timestamp won't get the event, add it to the map
	timestamps[streamId] = timestamp;
	numSamples[streamId] = nSamples;
	processStartTimes[streamId] = m_initialProcessTime;

	if (m_needsToSendTimestampMessages[streamId] && nSamples > 0)
	{
		HeapBlock<char> data;
		size_t dataSize = SystemEvent::fillTimestampSyncTextData(data, this, streamId, timestamp, false);

		eventBuffer.addEvent(data, dataSize, 0);

		m_needsToSendTimestampMessages[streamId] = false;
	}
}


int GenericProcessor::processEventBuffer()
{
	//
	// This loops through all events in the buffer, and uses the BUFFER_SIZE
	// events to determine the number of samples in the current buffer. If
	// there are multiple such events, the one with the highest number of
	// samples will be used.
	// This approach is not ideal, as it will become a problem if we allow
	// the sample rate to change at different points in the signal chain.
	//
	int numRead = 0;

	MidiBuffer& eventBuffer = *m_currentMidiBuffer;

	if (eventBuffer.getNumEvents() > 0)
	{
		MidiBuffer::Iterator i(eventBuffer);

		const uint8* dataptr;
		int dataSize;

		int samplePosition = -1;

		while (i.getNextEvent(dataptr, dataSize, samplePosition))
		{

			if (static_cast<Event::Type> (*dataptr) == Event::Type::SYSTEM_EVENT 
				&& static_cast<SystemEvent::Type>(*(dataptr + 1) == SystemEvent::Type::TIMESTAMP_AND_SAMPLES))
			{
				uint16 sourceProcessorId = *reinterpret_cast<const uint16*>(dataptr + 2);
				uint16 sourceStreamId = *reinterpret_cast<const uint16*>(dataptr + 4);
				uint32 sourceChannelIndex = *reinterpret_cast<const uint16*>(dataptr + 6);

				juce::int64 timestamp = *reinterpret_cast<const juce::int64*>(dataptr + 8);
				uint32 nSamples = *reinterpret_cast<const uint32*>(dataptr + 16);
				juce::int64 initialTicks = *reinterpret_cast<const juce::int64*>(dataptr + 20);
				
				numSamples[sourceStreamId] = nSamples;
				timestamps[sourceStreamId] = timestamp;
				processStartTimes[sourceStreamId] = initialTicks;
					
			}
			//else {
			//	std::cout << nodeId << " : " << " received event" << std::endl;
			//}
			//set the "recorded" bit on the first byte. This will go away when the probe system is implemented.
			//doing a const cast is always a bad idea, but there's no better way to do this until whe change the event record system
			//if (nodeId < 900) //If the processor is not a specialized one
			//	*const_cast<uint8*>(dataptr + 0) = *(dataptr + 0) | 0x80;
		}
	}

	return numRead;
}


int GenericProcessor::checkForEvents(bool checkForSpikes)
{
	//std::cout << nodeId << ": check for events" << std::endl;

	if (m_currentMidiBuffer->getNumEvents() > 0)
	{
		//Since adding events to the buffer inside this loop could be dangerous, create a temporary event buffer
		//so any call to addEvent will operate on it;
		MidiBuffer temporaryEventBuffer;
		MidiBuffer* originalEventBuffer = m_currentMidiBuffer;
		m_currentMidiBuffer = &temporaryEventBuffer;

		for (const auto meta : *originalEventBuffer) {

			const auto message = meta.getMessage();

			uint16 sourceProcessorId = EventBase::getProcessorId(message);
			uint16 sourceStreamId = EventBase::getStreamId(message);
			uint16 sourceChannelIdx = EventBase::getChannelIndex(message);

			if (EventBase::getBaseType(message) == Event::Type::PROCESSOR_EVENT)
			{
				const EventChannel* eventChannel = getEventChannel(sourceProcessorId, sourceStreamId, sourceChannelIdx);

				if (eventChannel != nullptr)
				{
					handleEvent(eventChannel, message, meta.samplePosition);

					if (eventChannel->getType() == EventChannel::Type::TTL)
					{
						// FIXME: Crashes for any sources that generate TTL events
						
						//getEditor()->setTTLState(sourceStreamId,
						//		TTLEvent::getBit(message),
						//		TTLEvent::getState(message)
						//	);
						
					}
				}

			}
			else if (EventBase::getBaseType(message) == Event::Type::SYSTEM_EVENT 
				    && SystemEvent::getSystemEventType(message) == SystemEvent::Type::TIMESTAMP_SYNC_TEXT)
			{
				handleTimestampSyncTexts(message);
			}
			else if (checkForSpikes && EventBase::getBaseType(message) == Event::Type::SPIKE_EVENT)
			{
				const SpikeChannel* spikeChannel = getSpikeChannel(sourceProcessorId, sourceStreamId, sourceChannelIdx);

				if (spikeChannel != nullptr)
					handleSpike(spikeChannel, message, meta.samplePosition);
			}
		}
		// Restore the original buffer pointer and, if some new events have 
		// been added here, copy them to the original buffer
		m_currentMidiBuffer = originalEventBuffer;

		if (temporaryEventBuffer.getNumEvents() > 0)
		{
			m_currentMidiBuffer->addEvents(temporaryEventBuffer, 0, -1, 0);
		}
			
		return 0;
	}

	return -1;
}

void GenericProcessor::addEvent(const Event* event, int sampleNum)
{
	size_t size = event->getChannelInfo()->getDataSize() + event->getChannelInfo()->getTotalEventMetadataSize() + EVENT_BASE_SIZE;
	
	HeapBlock<char> buffer(size);

	event->serialize(buffer, size);

	m_currentMidiBuffer->addEvent(buffer, size, sampleNum >= 0 ? sampleNum : 0);
}

void GenericProcessor::addTTLChannel(String name)
{
	if (dataStreams.size() == 0)
		return;

	if (ttlEventChannel != nullptr)
	{
		EventChannel::Settings settings{
			EventChannel::Type::TTL,
			name,
			"Default TTL event channel",
			"ttl.events",
			dataStreams[0]
		};

		eventChannels.add(new EventChannel(settings));
		ttlEventChannel = eventChannels.getLast();

		ttlBitStates.clear();

		for (int i = 0; i < 8; i++)
			ttlBitStates.add(false);
	}
	else {
		jassert(false); // cannot add a second default TTL channel
	}
}


void GenericProcessor::flipTTLBit(int sampleIndex, int bit)
{
	if (bit < 0 || bit >= 8)
		return;

	bool currentState = ttlBitStates[bit];
	ttlBitStates.set(bit, !currentState);

	int64 timestamp = timestamps[ttlEventChannel->getStreamId()] + sampleIndex;

	TTLEventPtr eventPtr = TTLEvent::createTTLEvent(ttlEventChannel, timestamp, bit, !currentState);

	addEvent(eventPtr, sampleIndex);
}

bool GenericProcessor::getTTLBit(int bit)
{
	if (bit < 0 || bit >= 8)
		return false;

	return ttlBitStates[bit];
}

void GenericProcessor::broadcastMessage(String msg)
{
	AccessClass::getMessageCenter()->broadcastMessage(msg);
}

void GenericProcessor::addSpike(int channelIndex, const Spike* spike, int sampleNum)
{
	addSpike(spikeChannels[channelIndex], spike, sampleNum);
}

void GenericProcessor::addSpike(const SpikeChannel* channel, const Spike* spike, int sampleNum)
{
	size_t size = channel->getDataSize() 
		+ channel->getTotalEventMetadataSize() 
		+ SPIKE_BASE_SIZE 
		+ channel->getNumChannels()*sizeof(float);

	HeapBlock<char> buffer(size);

	spike->serialize(buffer, size);

	m_currentMidiBuffer->addEvent(buffer, size, sampleNum >= 0 ? sampleNum : 0);
}


void GenericProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& eventBuffer)
{

	if (isSource())
		m_initialProcessTime = Time::getHighResolutionTicks();

	m_currentMidiBuffer = &eventBuffer;

	processEventBuffer(); // extract buffer sizes and timestamps,

	process(buffer);

	latencyMeter->setLatestLatency(processStartTimes);
}

Array<const EventChannel*> GenericProcessor::getEventChannels()
{
	Array<const EventChannel*> channels;

	for (int i = 0; i < eventChannels.size(); i++)
	{
		channels.add(eventChannels[i]);
	}

	return channels;
}

Array< const DataStream*> GenericProcessor::getStreamsForDestNode(GenericProcessor* p)
{
	Array<const DataStream*> streams;

	for (int i = 0; i < dataStreams.size(); i++)
	{
		streams.add(dataStreams[i]);
	}

	return streams;
}

Array< const DataStream*> GenericProcessor::getDataStreams() const
{
	Array<const DataStream*> streams;

	for (int i = 0; i < dataStreams.size(); i++)
	{
		streams.add(dataStreams[i]);
	}

	return streams;
}

const ContinuousChannel* GenericProcessor::getContinuousChannel(uint16 processorId, uint16 streamId, uint16 localIndex) const
{
	return continuousChannelMap.at(processorId).at(streamId).at(localIndex);
}

int GenericProcessor::getIndexOfMatchingChannel(const ContinuousChannel* channel) const
{

	for (int index = 0; index < continuousChannels.size(); index++)
	{
		if (*continuousChannels[index] == *channel) // check for matching Uuid
		{
			return index;
		}	
	}

	return -1;
}

int GenericProcessor::getIndexOfMatchingChannel(const EventChannel* channel) const
{

	for (int index = 0; index < eventChannels.size(); index++)
	{
		if (*eventChannels[index] == *channel) // check for matching Uuid
		{
			return index;
		}
	}

	return -1;
}

int GenericProcessor::getIndexOfMatchingChannel(const SpikeChannel* channel) const
{

	for (int index = 0; index < spikeChannels.size(); index++)
	{
		if (*spikeChannels[index] == *channel) // check for matching Uuid
		{
			return index;
		}
	}

	return -1;
}

const EventChannel* GenericProcessor::getEventChannel(uint16 processorId, uint16 streamId, uint16 localIndex) const
{
	return eventChannelMap.at(processorId).at(streamId).at(localIndex); 
}

const EventChannel* GenericProcessor::getMessageChannel() const
{
	for (auto eventChannel : eventChannels)
	{
		LOGD("Event channel source node id: ", eventChannel->getSourceNodeId());

		if (eventChannel->getSourceNodeId() == 904)
			return eventChannel;
	}

	return nullptr;
}

const SpikeChannel* GenericProcessor::getSpikeChannel(uint16 processorId, uint16 streamId, uint16 localIndex) const
{
	return spikeChannelMap.at(processorId).at(streamId).at(localIndex);
}

DataStream* GenericProcessor::getDataStream(uint16 streamId) const
{
	return dataStreamMap.at(streamId);
}

const ConfigurationObject* GenericProcessor::getConfigurationObject(int index) const
{
	return configurationObjects[index];
}

int GenericProcessor::getTotalContinuousChannels() const
{
	return continuousChannels.size();
}


int GenericProcessor::getTotalEventChannels() const
{
	return eventChannels.size();
}

int GenericProcessor::getTotalSpikeChannels() const
{
	return spikeChannels.size();
}

int GenericProcessor::getTotalConfigurationObjects() const
{
	return configurationObjects.size();
}

const ContinuousChannel* GenericProcessor::getContinuousChannel(int globalIndex) const
{
	if (globalIndex < continuousChannels.size())
		return continuousChannels[globalIndex];
	else
		return nullptr;
}

const SpikeChannel* GenericProcessor::getSpikeChannel(int globalIndex) const
{
	if (globalIndex < spikeChannels.size())
		return spikeChannels[globalIndex];
	else
		return nullptr;
}

const EventChannel* GenericProcessor::getEventChannel(int globalIndex) const
{
	if (globalIndex < eventChannels.size())
		return eventChannels[globalIndex];
	else
		return nullptr;
}
/////// ---- LOADING AND SAVING ---- //////////


void GenericProcessor::saveToXml(XmlElement* xml)
{
	xml->setAttribute("NodeId", nodeId);
    
    XmlElement* paramsXml = xml->createNewChildElement("GLOBAL_PARAMETERS");
    
    for (auto param : getParameters())
    {
        param->toXml(paramsXml);
    }

	for (auto stream : getDataStreams())
	{
		XmlElement* streamXml = xml->createNewChildElement("STREAM");
        
        streamXml->setAttribute("name",stream->getName());
        streamXml->setAttribute("description", stream->getDescription());
        streamXml->setAttribute("sample_rate", stream->getSampleRate());
        streamXml->setAttribute("channel_count", stream->getChannelCount());

        XmlElement* streamParamsXml = xml->createNewChildElement("PARAMETERS");
        
		for (auto param : getParameters(stream->getStreamId()))
			streamParameterMap[stream->getStreamId()][param->getName()]->toXml(streamParamsXml);
        
        for (auto eventChannel : stream->getEventChannels())
        {
            if (getParameters(eventChannel).size() > 0)
            {
                XmlElement* eventParamsXml = xml->createNewChildElement("EVENT_CHANNEL");
                eventParamsXml->setAttribute("name",eventChannel->getName());
                eventParamsXml->setAttribute("description", eventChannel->getDescription());
                
                for (auto param : getParameters(eventChannel))
                {
                    eventChannelParameterMap[eventChannel][param->getName()]->toXml(eventParamsXml);
                }
            }
        }
        
        for (auto continuousChannel : stream->getContinuousChannels())
        {
            if (getParameters(continuousChannel).size() > 0)
            {
                XmlElement* continuousParamsXml = xml->createNewChildElement("CONTINUOUS_CHANNEL");
                continuousParamsXml->setAttribute("name",continuousChannel->getName());
                continuousParamsXml->setAttribute("description", continuousChannel->getDescription());
                
                for (auto param : getParameters(continuousChannel))
                {
                    continuousChannelParameterMap[continuousChannel][param->getName()]->toXml(continuousParamsXml);
                }
            }
        }
        
        for (auto spikeChannel : stream->getSpikeChannels())
        {
            if (getParameters(spikeChannel).size() > 0)
            {
                XmlElement* spikeParamsXml = xml->createNewChildElement("SPIKE_CHANNEL");
                spikeParamsXml->setAttribute("name",spikeChannel->getName());
                spikeParamsXml->setAttribute("description", spikeChannel->getDescription());
                
                for (auto param : getParameters(spikeChannel))
                {
                    spikeChannelParameterMap[spikeChannel][param->getName()]->toXml(spikeParamsXml);
                }
            }
        }
	}

	saveCustomParametersToXml(xml->createNewChildElement("CUSTOM_PARAMETERS"));

	getEditor()->saveToXml(xml->createNewChildElement("EDITOR"));
}


void GenericProcessor::saveCustomParametersToXml(XmlElement* parentElement)
{
}

void GenericProcessor::saveChannelParametersToXml(XmlElement* parentElement, InfoObject* channel)
{
	XmlElement* channelInfoXml;

	if (channel->getType() == InfoObject::Type::CONTINUOUS_CHANNEL)
	{
		channelInfoXml = parentElement->createNewChildElement("CHANNEL");
		channelInfoXml->setAttribute("name", channel->getName());
		channelInfoXml->setAttribute("number", channel->getGlobalIndex());
	}
	else if (channel->getType() == InfoObject::Type::EVENT_CHANNEL)
	{
		channelInfoXml = parentElement->createNewChildElement("EVENTCHANNEL");
		channelInfoXml->setAttribute("name", channel->getName());
		channelInfoXml->setAttribute("number", channel->getGlobalIndex());

	}
	else if (channel->getType() == InfoObject::Type::SPIKE_CHANNEL)
	{
		channelInfoXml = parentElement->createNewChildElement("SPIKECHANNEL");
		channelInfoXml->setAttribute("name", String(channel->getName()));
		channelInfoXml->setAttribute("number", channel->getGlobalIndex());
	}

	saveCustomChannelParametersToXml(channelInfoXml, channel);
}

void GenericProcessor::saveCustomChannelParametersToXml(XmlElement* channelInfo, InfoObject* channel)
{
}


void GenericProcessor::loadFromXml()
{

	if (parametersAsXml != nullptr)
	{
       // if (!m_paramsWereLoaded)
       // {
		LOGD("Loading parameters for ", m_name);

		int streamIndex = 0;
		Array<const DataStream*> availableStreams = getDataStreams();

		forEachXmlChildElement(*parametersAsXml, xmlNode)
		{
            if (xmlNode->hasTagName("GLOBAL_PARAMETERS"))
            {
                for (int i = 0; i < xmlNode->getNumAttributes(); i++)
                    getParameter(xmlNode->getAttributeName(i))->fromXml(xmlNode);
            }
            
			if (xmlNode->hasTagName("STREAM"))
			{
				if (availableStreams.size() > streamIndex)
				{
					LOGD("FOUND IT!");
                    
                    forEachXmlChildElement(*xmlNode, streamParams)
                    {
                        if (streamParams->hasTagName("PARAMETERS"))
                        {
                            for (int i = 0; i < xmlNode->getNumAttributes(); i++)
                                getParameter(availableStreams[streamIndex]->getStreamId(),
                                         xmlNode->getAttributeName(i))->fromXml(xmlNode);
                        }
                    }
				}
				else {
					LOGD("DID NOT FIND IT!");
				}

				streamIndex++;
			}
		}


        forEachXmlChildElement(*parametersAsXml, xmlNode)
        {
            if (xmlNode->hasTagName("CUSTOM_PARAMETERS"))
            {
                loadCustomParametersFromXml(xmlNode);
            }
        }
        // use parametersAsXml to restore state
        
        // load editor parameters
        forEachXmlChildElement(*parametersAsXml, xmlNode)
        {
            if (xmlNode->hasTagName("EDITOR"))
            {
                getEditor()->loadFromXml(xmlNode);
            }
        }

        forEachXmlChildElement(*parametersAsXml, xmlNode)
        {
            if (xmlNode->hasTagName("CHANNEL"))
            {
                loadChannelParametersFromXml(xmlNode, InfoObject::Type::CONTINUOUS_CHANNEL);
            }
            else if (xmlNode->hasTagName("EVENTCHANNEL"))
            {
                loadChannelParametersFromXml(xmlNode, InfoObject::Type::EVENT_CHANNEL);
            }
            else if (xmlNode->hasTagName("SPIKECHANNEL"))
            {
                loadChannelParametersFromXml(xmlNode, InfoObject::Type::SPIKE_CHANNEL);
            }
        }
	}

	m_paramsWereLoaded = true;
}


void GenericProcessor::loadChannelParametersFromXml(XmlElement* channelInfo, InfoObject::Type type)
{
	int channelNum = channelInfo->getIntAttribute("number");

	if (type == InfoObject::Type::CONTINUOUS_CHANNEL)
	{
		forEachXmlChildElement(*channelInfo, subNode)
		{
			if (subNode->hasTagName("SELECTIONSTATE"))
			{
				//getEditor()->setChannelSelectionState(channelNum,
				//	subNode->getBoolAttribute("param"),
				//	subNode->getBoolAttribute("record"),
				//	subNode->getBoolAttribute("audio"));
			}
		}
	}

	loadCustomChannelParametersFromXml(channelInfo, type);
}


void GenericProcessor::loadCustomParametersFromXml(XmlElement*) { }

void GenericProcessor::loadCustomChannelParametersFromXml(XmlElement* channelInfo, InfoObject::Type type) { }

void GenericProcessor::setCurrentChannel(int chan)
{
	currentChannel = chan;
}


void GenericProcessor::setProcessorType(PluginProcessorType processorType)
{
	m_processorType = processorType;
}


bool GenericProcessor::canSendSignalTo(GenericProcessor*) const { return true; }

bool GenericProcessor::generatesTimestamps() const { return false; }

bool GenericProcessor::isFilter()        const  { return getProcessorType() == PROCESSOR_TYPE_FILTER; }
bool GenericProcessor::isSource()        const  { return getProcessorType() == PROCESSOR_TYPE_SOURCE; }
bool GenericProcessor::isSink()          const  { return getProcessorType() == PROCESSOR_TYPE_SINK; }
bool GenericProcessor::isSplitter()      const  { return getProcessorType() == PROCESSOR_TYPE_SPLITTER; }
bool GenericProcessor::isMerger()        const  { return getProcessorType() == PROCESSOR_TYPE_MERGER; }
bool GenericProcessor::isAudioMonitor()  const  { return getProcessorType() == PROCESSOR_TYPE_AUDIO_MONITOR; }
bool GenericProcessor::isUtility()       const  { return getProcessorType() == PROCESSOR_TYPE_UTILITY; }
bool GenericProcessor::isRecordNode()    const  { return getProcessorType() == PROCESSOR_TYPE_RECORD_NODE; }

PluginProcessorType GenericProcessor::getProcessorType() const
{
	return m_processorType;
}

int GenericProcessor::getNumInputs() const  
{ 
	if (sourceNode != nullptr)
	{
		return sourceNode->continuousChannels.size();
	}
	else {
		return 0;
	}
}

int GenericProcessor::getNumOutputs() const   
{ 
	return continuousChannels.size(); 
}

int GenericProcessor::getNumOutputsForStream(int streamIdx) const
{
	return dataStreams[streamIdx]->getChannelCount();
}

int GenericProcessor::getNodeId() const                     { return nodeId; }

float GenericProcessor::getDefaultSampleRate() const        { return 44100.0; }
float GenericProcessor::getSampleRate(int) const               { return getDefaultSampleRate(); }

GenericProcessor* GenericProcessor::getSourceNode() const { return sourceNode; }
GenericProcessor* GenericProcessor::getDestNode()   const { return destNode; }

int GenericProcessor::getNumDataStreams() const { return dataStreams.size(); }

GenericEditor* GenericProcessor::getEditor() const { return editor.get(); }

AudioBuffer<float>* GenericProcessor::getContinuousBuffer() const { return 0; }
MidiBuffer* GenericProcessor::getEventBuffer() const             { return 0; }

void GenericProcessor::switchIO(int)   { }
void GenericProcessor::switchIO()       { }

void GenericProcessor::setPathToProcessor(GenericProcessor* p)   { }
void GenericProcessor::setMergerSourceNode(GenericProcessor* sn)  { }
void GenericProcessor::setSplitterDestNode(GenericProcessor* dn)  { }

bool GenericProcessor::startAcquisition() { return true; }
bool GenericProcessor::stopAcquisition() { return true; }

void GenericProcessor::startRecording() {

	m_needsToSendTimestampMessages.clear();
	for (auto stream : getDataStreams())
		m_needsToSendTimestampMessages[stream->getStreamId()] = true;

 }

void GenericProcessor::stopRecording()  { }

void GenericProcessor::updateSettings() { }

void GenericProcessor::handleEvent(const EventChannel* eventInfo, const EventPacket& packet, int samplePosition) {}

void GenericProcessor::handleSpike(const SpikeChannel* spikeInfo, const EventPacket& packet, int samplePosition) {}

void GenericProcessor::handleTimestampSyncTexts(const EventPacket& packet) {};

/*void GenericProcessor::setEnabledState(bool t)
{
	isEnabled = t;
    
    if (isEnabled)
        getEditor()->enable();
    else
        getEditor()->disable();
}*/


GenericProcessor::DefaultEventInfo::DefaultEventInfo(EventChannel::Type t, unsigned int c, unsigned int l, float s)
	:type(t),
	nChannels(c),
	length(l),
	sampleRate(s)
{
}

GenericProcessor::DefaultEventInfo::DefaultEventInfo()
	:type(EventChannel::INVALID),
	nChannels(0),
	length(0),
	sampleRate(44100)
{}


LatencyMeter::LatencyMeter(GenericProcessor* processor_)
	: processor(processor_),
	counter(0)
{

}

void LatencyMeter::update(Array<const DataStream*>dataStreams)
{
	latencies.clear();

	for (auto dataStream : dataStreams)
		latencies[dataStream->getStreamId()].insertMultiple(0, 0, 5);

}

void LatencyMeter::setLatestLatency(std::map<uint16, juce::int64>& processStartTimes)
{

	if (counter % 10 == 0) // update latency estimate every 10 blocks
	{

		std::map<uint16, juce::int64>::iterator it = processStartTimes.begin();

		int64 currentTime = Time::getHighResolutionTicks();

		while (it != processStartTimes.end())
		{
			latencies[it->first].set(counter % 5, currentTime - it->second);
			it++;
		}

		if (counter % 50 == 0)
		{

			std::map<uint16, juce::int64>::iterator it = processStartTimes.begin();

			while (it != processStartTimes.end())
			{
				float totalLatency = 0.0f;

				for (int i = 0; i < 10; i++)
					totalLatency += float(latencies[it->first][i]);

				totalLatency = totalLatency / 5.0f
					/ float(Time::getHighResolutionTicksPerSecond())
					* 1000.0f;

				//std::cout << "Total latency for " << processor->getNodeId() << ": " << totalLatency << " ms" << std::endl;

				processor->getEditor()->setMeanLatencyMs(it->first, totalLatency);

				it++;

			}
			
		}
			
	}

	counter++;

}
