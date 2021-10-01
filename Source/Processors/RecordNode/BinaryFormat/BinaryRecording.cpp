#include "BinaryRecording.h"

#include "../../Settings/InfoObject.h"
#include "../../Settings/DataStream.h"

#include "../../Events/Spike.h"

#define TIC std::chrono::high_resolution_clock::now()

#define MAX_BUFFER_SIZE 40960

BinaryRecording::BinaryRecording()
{
    m_bufferSize = MAX_BUFFER_SIZE;
	m_scaledBuffer.malloc(MAX_BUFFER_SIZE);
	m_intBuffer.malloc(MAX_BUFFER_SIZE);
	m_tsBuffer.malloc(MAX_BUFFER_SIZE);		
}

BinaryRecording::~BinaryRecording() {}

String BinaryRecording::getEngineID() const
{
	return "RAWBINARY";
}

String BinaryRecording::getProcessorString(const InfoObject* channelInfo)
{
    String fName = channelInfo->getSourceNodeName().replaceCharacter(' ', '_') + "-";
    fName += String(((ChannelInfoObject*)channelInfo)->getStreamId());
	fName += File::getSeparatorString();
	return fName;
}

void BinaryRecording::openFiles(File rootFolder, int experimentNumber, int recordingNumber)
{

	String basepath = rootFolder.getFullPathName() + rootFolder.getSeparatorString() + "experiment" + String(experimentNumber)
        + File::getSeparatorString() + "recording" + String(recordingNumber + 1) + File::getSeparatorString();
    String contPath = basepath + "continuous" + File::getSeparatorString();

    m_channelIndexes.insertMultiple(0, 0, getNumRecordedContinuousChannels());
    m_fileIndexes.insertMultiple(0, 0, getNumRecordedContinuousChannels());

    Array<const ContinuousChannel*> indexedContinuousChannels;
    Array<unsigned int> indexedChannelCount;
    Array<var> jsonContinuousfiles;
    Array<var> jsonChannels;
    StringArray continuousFileNames;
    int lastId = 0;

    int lastStreamId = -1;

    for (int proc = 0; proc < getNumRecordedProcessors(); proc++)
    {

        const RecordProcessorInfo& pInfo = getProcessorInfo(proc);
        int recChans = pInfo.recordedChannels.size();

        for (int chan = 0; chan < recChans; chan++)
        {
            int recordedChan = pInfo.recordedChannels[chan];
            int realChan = getRealChannel(recordedChan);
            const ContinuousChannel* channelInfo = getContinuousChannel(realChan);
            int sourceId = channelInfo->getSourceNodeId();
            int currentStreamId = channelInfo->getStreamId();

            int nInfoArrays = indexedContinuousChannels.size();

            bool found = false;
            DynamicObject::Ptr jsonChan = new DynamicObject();


            jsonChan->setProperty("channel_name", channelInfo->getName());
            jsonChan->setProperty("description", channelInfo->getDescription());
            jsonChan->setProperty("identifier", channelInfo->getIdentifier());
            jsonChan->setProperty("history", channelInfo->getHistoryString());
            jsonChan->setProperty("bit_volts", channelInfo->getBitVolts());
            jsonChan->setProperty("units", channelInfo->getUnits());
            jsonChan->setProperty("source_processor_index", channelInfo->getSourceNodeId());
            //jsonChan->setProperty("recorded_processor_index", channelInfo->getCurrentNodeChannelIdx());
            createChannelMetadata(channelInfo, jsonChan);
            for (int i = lastId; i < nInfoArrays; i++)
            {

                if (indexedContinuousChannels[i]->getStreamId() == currentStreamId)
                {
                    unsigned int count = indexedChannelCount[i];
                    m_channelIndexes.set(recordedChan, count);
                    m_fileIndexes.set(recordedChan, i);
                    indexedChannelCount.set(i, count + 1);
                    jsonChannels.getReference(i).append(var(jsonChan));
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                String datPath = getProcessorString(channelInfo);
                continuousFileNames.add(contPath + datPath + "continuous.dat");

                LOGDD("Creating file: ", contPath, datPath, "timestamps.npy");
                ScopedPointer<NpyFile> tFile = new NpyFile(contPath + datPath + "timestamps.npy", NpyType(BaseType::INT64,1));
                m_dataTimestampFiles.add(tFile.release());

                ScopedPointer<NpyFile> ftsFile = new NpyFile(contPath + datPath + "synchronized_timestamps.npy", NpyType(BaseType::DOUBLE,1));
                m_dataFloatTimestampFiles.add(ftsFile.release());

                m_fileIndexes.set(recordedChan, nInfoArrays);
                m_channelIndexes.set(recordedChan, 0);
                indexedChannelCount.add(1);
                indexedContinuousChannels.add(channelInfo);

                Array<var> jsonChanArray;
                jsonChanArray.add(var(jsonChan));
                jsonChannels.add(var(jsonChanArray));
                DynamicObject::Ptr jsonFile = new DynamicObject();
                jsonFile->setProperty("folder_name", datPath.replace(File::getSeparatorString(), "/")); //to make it more system agnostic, replace separator with only one slash
                jsonFile->setProperty("sample_rate", channelInfo->getSampleRate());
                jsonFile->setProperty("source_processor_name", channelInfo->getSourceNodeName());
                jsonFile->setProperty("source_processor_id", channelInfo->getSourceNodeId());
                jsonFile->setProperty("source_processor_sub_idx", channelInfo->getStreamId());
                jsonFile->setProperty("recorded_processor", channelInfo->getNodeName());
                jsonFile->setProperty("recorded_processor_id", channelInfo->getNodeId());
                jsonContinuousfiles.add(var(jsonFile));
            }
        }
        lastId = 0; // indexedDataChannels.size();
    }

    int nFiles = continuousFileNames.size();
    for (int i = 0; i < nFiles; i++)
    {
        int numChannels = jsonChannels.getReference(i).size();
        ScopedPointer<SequentialBlockFile> bFile = new SequentialBlockFile(numChannels, samplesPerBlock);
        if (bFile->openFile(continuousFileNames[i]))
            m_DataFiles.add(bFile.release());
        else
            m_DataFiles.add(nullptr);
        DynamicObject::Ptr jsonFile = jsonContinuousfiles.getReference(i).getDynamicObject(); 
        jsonFile->setProperty("num_channels", numChannels);
        jsonFile->setProperty("channels", jsonChannels.getReference(i));
    }

    int nChans = getNumRecordedContinuousChannels();
    //Timestamps
    Array<uint32> procIDs;
    for (int i = 0; i < nChans; i++)

    {
        m_startTS.add(getTimestamp(i));
    }

    int nEvents = getNumRecordedEventChannels();
    String eventPath(basepath + "events" + File::getSeparatorString());
    Array<var> jsonEventFiles;

    for (int ev = 0; ev < nEvents; ev++)
    {

        const EventChannel* chan = getEventChannel(ev);
        String eventName = getProcessorString(chan);
        NpyType type;
        String dataFileName;

        switch (chan->getType())
        {
        case EventChannel::TEXT:
            LOGDD("Got TEXT channel");
            eventName += "TEXT_group";
            type = NpyType(BaseType::CHAR, chan->getLength());
            dataFileName = "text";
            break;
        case EventChannel::TTL:
            LOGDD("Got TTL channel");
            eventName += "TTL";
            type = NpyType(BaseType::INT16, 1);
            dataFileName = "channel_states";
            break;
        default:
            LOGDD("Got BINARY group");
            eventName += "BINARY_group";
            type = NpyType(chan->getEquivalentMetadataType(), chan->getLength());
            dataFileName = "data_array";
            break;
        }

        // FIXME eventName += "_" + String(chan->getSourceIndex() + 1) + File::getSeparatorString();

        ScopedPointer<EventRecording> rec = new EventRecording();

        rec->mainFile = std::make_unique<NpyFile>(eventPath + eventName + dataFileName + ".npy", type);
        rec->timestampFile = std::make_unique<NpyFile>(eventPath + eventName + "timestamps.npy", NpyType(BaseType::INT64, 1));
        rec->channelFile = std::make_unique<NpyFile>(eventPath + eventName + "channels.npy", NpyType(BaseType::UINT16, 1));
        if (chan->getType() == EventChannel::TTL && m_saveTTLWords)
        {
            rec->extraFile = std::make_unique<NpyFile>(eventPath + eventName + "full_words.npy", NpyType(BaseType::UINT8, chan->getDataSize()));
        }

        DynamicObject::Ptr jsonChannel = new DynamicObject();
        jsonChannel->setProperty("folder_name", eventName.replace(File::getSeparatorString(), "/"));
        jsonChannel->setProperty("channel_name", chan->getName());
        jsonChannel->setProperty("description", chan->getDescription());
        jsonChannel->setProperty("identifier", chan->getIdentifier());
        jsonChannel->setProperty("sample_rate", chan->getSampleRate());
        jsonChannel->setProperty("type", jsonTypeValue(type.getType()));
        //jsonChannel->setProperty("num_channels", (int) chan->stream->getNumChannels());
        jsonChannel->setProperty("source_processor", chan->getSourceNodeName());
        createChannelMetadata(chan, jsonChannel);

        rec->metaDataFile = createEventMetadataFile(chan, eventPath + eventName + "metadata.npy", jsonChannel);
        m_eventFiles.add(rec.release());
        jsonEventFiles.add(var(jsonChannel));
    }

    int nSpikes = getNumRecordedSpikeChannels();

    Array<const SpikeChannel*> indexedSpikes;
    Array<uint16> indexedChannels;
    m_spikeFileIndexes.insertMultiple(0, 0, nSpikes);
    m_spikeChannelIndexes.insertMultiple(0, 0, nSpikes);
    String spikePath(basepath + "spikes" + File::getSeparatorString());
    Array<var> jsonSpikeFiles;
    Array<var> jsonSpikeChannels;
    std::map<uint32, int> groupMap;
    for (int sp = 0; sp < nSpikes; sp++)
    {
        const SpikeChannel* ch = getSpikeChannel(sp);
        DynamicObject::Ptr jsonChannel = new DynamicObject();
        unsigned int numSpikeChannels = ch->getNumChannels();
        jsonChannel->setProperty("channel_name", ch->getName());
        jsonChannel->setProperty("description", ch->getDescription());
        jsonChannel->setProperty("identifier", ch->getIdentifier());
        Array<var> jsonChannelInfo;
        for (int i = 0; i < numSpikeChannels; i++)
        {
            DynamicObject::Ptr jsonSpikeChInfo = new DynamicObject();
            jsonSpikeChInfo->setProperty("source_processor_id", ch->getSourceNodeId());
            jsonSpikeChInfo->setProperty("source_processor_stream_id", ch->getStreamId());
            jsonSpikeChInfo->setProperty("source_processor_channel", ch->getLocalIndex());
            jsonChannelInfo.add(var(jsonSpikeChInfo));
        }
        jsonChannel->setProperty("source_channel_info", jsonChannelInfo);
        createChannelMetadata(ch, jsonChannel);

        int nIndexed = indexedSpikes.size();
        bool found = false;
        for (int i = 0; i < nIndexed; i++)
        {
            const SpikeChannel* ich = indexedSpikes[i];
            //identical channels (same data and metadata) from the same processor go to the same file
            if (ch->getSourceNodeId() == ich->getSourceNodeId() && ch->getStreamId() == ich->getStreamId() && *ch == *ich)
            {
                found = true;
                m_spikeFileIndexes.set(sp, i);
                unsigned int numChans = indexedChannels[i];
                indexedChannels.set(i, numChans + 1);
                m_spikeChannelIndexes.set(sp, numChans + 1);
                jsonSpikeChannels.getReference(i).append(var(jsonChannel));
                break;
            }
        }

        if (!found)
        {
            int fileIndex = m_spikeFiles.size();
            m_spikeFileIndexes.set(sp, fileIndex);
            indexedSpikes.add(ch);
            m_spikeChannelIndexes.set(sp, 1);
            indexedChannels.add(1);
            ScopedPointer<EventRecording> rec = new EventRecording();

            uint32 procID = 0; // FIXME GenericProcessor::getProcessorFullId(ch->getSourceNodeID(), ch->getSubProcessorIdx());
            int groupIndex = ++groupMap[procID];

            String spikeName = getProcessorString(ch) + "spike_group_" + String(groupIndex) + File::getSeparatorString();

            rec->mainFile = std::make_unique<NpyFile>(spikePath + spikeName + "spike_waveforms.npy", NpyType(BaseType::INT16, ch->getTotalSamples()), ch->getNumChannels());
            rec->timestampFile = std::make_unique<NpyFile>(spikePath + spikeName + "spike_times.npy", NpyType(BaseType::INT64, 1));
            rec->channelFile = std::make_unique<NpyFile>(spikePath + spikeName + "spike_electrode_indices.npy", NpyType(BaseType::UINT16, 1));
            rec->extraFile = std::make_unique<NpyFile>(spikePath + spikeName + "spike_clusters.npy", NpyType(BaseType::UINT16, 1));
            Array<NpyType> tsTypes;

            Array<var> jsonChanArray;
            jsonChanArray.add(var(jsonChannel));
            jsonSpikeChannels.add(var(jsonChanArray));
            DynamicObject::Ptr jsonFile = new DynamicObject();

            jsonFile->setProperty("folder_name", spikeName.replace(File::getSeparatorString(),"/"));
            jsonFile->setProperty("sample_rate", ch->getSampleRate());
            jsonFile->setProperty("source_processor", ch->getSourceNodeName());
            jsonFile->setProperty("num_channels", (int)numSpikeChannels);
            jsonFile->setProperty("pre_peak_samples", (int)ch->getPrePeakSamples());
            jsonFile->setProperty("post_peak_samples", (int)ch->getPostPeakSamples());

            rec->metaDataFile = createEventMetadataFile(ch, spikePath + spikeName + "metadata.npy", jsonFile);
            m_spikeFiles.add(rec.release());
            jsonSpikeFiles.add(var(jsonFile));
        }
    }
    int nSpikeFiles = jsonSpikeFiles.size();
    for (int i = 0; i < nSpikeFiles; i++)
    {
        int size = jsonSpikeChannels.getReference(i).size();
        DynamicObject::Ptr jsonFile = jsonSpikeFiles.getReference(i).getDynamicObject();
        jsonFile->setProperty("num_channels", size);
        jsonFile->setProperty("channels", jsonSpikeChannels.getReference(i));
    }

    File syncFile = File(basepath + "sync_messages.txt");
    Result res = syncFile.create();
    if (res.failed())
    {
        std::cerr << "Error creating sync text file:" << res.getErrorMessage() << std::endl;
    }
    else
    {
        m_syncTextFile = syncFile.createOutputStream();
    }

    m_recordingNum = recordingNumber;

    DynamicObject::Ptr jsonSettingsFile = new DynamicObject();
    jsonSettingsFile->setProperty("GUI version", CoreServices::getGUIVersion());
    jsonSettingsFile->setProperty("continuous", jsonContinuousfiles);
    jsonSettingsFile->setProperty("events", jsonEventFiles);
    jsonSettingsFile->setProperty("spikes", jsonSpikeFiles);
    FileOutputStream settingsFileStream(File(basepath + "structure.oebin"));

    jsonSettingsFile->writeAsJSON(settingsFileStream, 2, false, 3);

}

std::unique_ptr<NpyFile> BinaryRecording::createEventMetadataFile(const MetadataEventObject* channel, String filename, DynamicObject* jsonFile)
{
    int nMetadata = channel->getEventMetadataCount();
    if (nMetadata < 1) return nullptr;

    Array<NpyType> types;
    Array<var> jsonMetadata;
    for (int i = 0; i < nMetadata; i++)
    {
        const MetadataDescriptor* md = channel->getEventMetadataDescriptor(i);
        types.add(NpyType(md->getName(), md->getType(), md->getLength()));
        DynamicObject::Ptr jsonValues = new DynamicObject();
        jsonValues->setProperty("name", md->getName());
        jsonValues->setProperty("description", md->getDescription());
        jsonValues->setProperty("identifier", md->getIdentifier());
        jsonValues->setProperty("type", jsonTypeValue(md->getType()));
        jsonValues->setProperty("length", (int)md->getLength());
        jsonMetadata.add(var(jsonValues));
    }
    if (jsonFile)
        jsonFile->setProperty("event_metadata", jsonMetadata);
    return std::make_unique<NpyFile>(filename, types);
}

template <typename TO, typename FROM>
void dataToVar(var& dataTo, const void* dataFrom, int length)
{
	const FROM* buffer = reinterpret_cast<const FROM*>(dataFrom);
	for (int i = 0; i < length; i++)
	{
		dataTo.append(static_cast<TO>(*(buffer + i)));
	}
}

String BinaryRecording::jsonTypeValue(BaseType type)
{
	switch (type)
	{
	case BaseType::CHAR:
		return "string";
	case BaseType::INT8:
		return "int8";
	case BaseType::UINT8:
		return "uint8";
	case BaseType::INT16:
		return "int16";
	case BaseType::UINT16:
		return "uint16";
	case BaseType::INT32:
		return "int32";
	case BaseType::UINT32:
		return "uint32";
	case BaseType::INT64:
		return "int64";
	case BaseType::UINT64:
		return "uint64";
	case BaseType::FLOAT:
		return "float";
	case BaseType::DOUBLE:
		return "double";
	default:
		return String();
	}
}

void BinaryRecording::createChannelMetadata(const MetadataObject* channel, DynamicObject* jsonFile)
{
	int nMetadata = channel->getMetadataCount();
	if (nMetadata < 1) return;

	Array<var> jsonMetadata;
	for (int i = 0; i < nMetadata; i++)
	{
		const MetadataDescriptor* md = channel->getMetadataDescriptor(i);
		const MetadataValue* mv = channel->getMetadataValue(i);
		DynamicObject::Ptr jsonValues = new DynamicObject();
		MetadataDescriptor::MetadataTypes type = md->getType();
		unsigned int length = md->getLength();
		jsonValues->setProperty("name", md->getName());
		jsonValues->setProperty("description", md->getDescription());
		jsonValues->setProperty("identifier", md->getIdentifier());
		jsonValues->setProperty("type", jsonTypeValue(type));
		jsonValues->setProperty("length", (int)length);
		var val;
		if (type == MetadataDescriptor::CHAR)
		{
			String tmp;
			mv->getValue(tmp);
			val = tmp;
		}
		else
		{
			const void* buf = mv->getRawValuePointer();
			switch (type)
			{
			case MetadataDescriptor::INT8:
				dataToVar<int, int8>(val, buf, length);
				break;
			case MetadataDescriptor::UINT8:
				dataToVar<int, uint8>(val, buf, length);
				break;
			case MetadataDescriptor::INT16:
				dataToVar<int, int16>(val, buf, length);
				break;
			case MetadataDescriptor::UINT16:
				dataToVar<int, uint16>(val, buf, length);
				break;
			case MetadataDescriptor::INT32:
				dataToVar<int, int32>(val, buf, length);
				break;
				//A full uint32 doesn't fit in a regular int, so we increase size
			case MetadataDescriptor::UINT32:
				dataToVar<int64, uint8>(val, buf, length);
				break;
			case MetadataDescriptor::INT64:
				dataToVar<int64, int64>(val, buf, length);
				break;
				//This might overrun and end negative if the uint64 is really big, but there is no way to store a full uint64 in a var
			case MetadataDescriptor::UINT64:
				dataToVar<int64, uint64>(val, buf, length);
				break;
			case MetadataDescriptor::FLOAT:
				dataToVar<float, float>(val, buf, length);
				break;
			case MetadataDescriptor::DOUBLE:
				dataToVar<double, double>(val, buf, length);
				break;
			default:
				val = "invalid";
			}
		}
		jsonValues->setProperty("value", val);
		jsonMetadata.add(var(jsonValues));
	}
	jsonFile->setProperty("channel_metadata", jsonMetadata);
}

void BinaryRecording::closeFiles()
{
	resetChannels();
}

void BinaryRecording::resetChannels()
{
	m_DataFiles.clear();
	m_channelIndexes.clear();
	m_fileIndexes.clear();
	m_dataTimestampFiles.clear();
    m_dataFloatTimestampFiles.clear();
	m_eventFiles.clear();
	m_spikeChannelIndexes.clear();
	m_spikeFileIndexes.clear();
	m_spikeFiles.clear();
	//m_syncTextFile = nullptr;

	m_scaledBuffer.malloc(MAX_BUFFER_SIZE);
	m_intBuffer.malloc(MAX_BUFFER_SIZE);
	m_tsBuffer.malloc(MAX_BUFFER_SIZE);
	m_bufferSize = MAX_BUFFER_SIZE;
	m_startTS.clear();
}

void BinaryRecording::writeEventMetadata(const MetadataEvent* event, NpyFile* file)
{
    if (!file || !event) return;
    int nMetadata = event->getMetadataValueCount();
    for (int i = 0; i < nMetadata; i++)
    {
        const MetadataValue* val = event->getMetadataValue(i);
        file->writeData(val->getRawValuePointer(), val->getDataSize());
    }
}

void BinaryRecording::increaseEventCounts(EventRecording* rec)
{
    rec->mainFile->increaseRecordCount();
    rec->timestampFile->increaseRecordCount();
    if (rec->extraFile) rec->extraFile->increaseRecordCount();
    if (rec->channelFile) rec->channelFile->increaseRecordCount();
    if (rec->metaDataFile) rec->metaDataFile->increaseRecordCount();
}

void BinaryRecording::writeSynchronizedData(int writeChannel, int realChannel, const float* dataBuffer, const double* ftsBuffer, int size)
{

    if (!size)  
        return;

    /* If our internal buffer is too small to hold the data... */
	if (size > m_bufferSize) //shouldn't happen, but if does, this prevents crash...
	{
		std::cerr << "[RN] Write buffer overrun, resizing from: " << m_bufferSize << " to: " << size << std::endl;
		m_scaledBuffer.malloc(size);
		m_intBuffer.malloc(size);
		m_tsBuffer.malloc(size);
		m_bufferSize = size;
	}

    /* Convert signal from float to int w/ bitVolts scaling */
	double multFactor = 1 / (float(0x7fff) * getContinuousChannel(realChannel)->getBitVolts());
	FloatVectorOperations::copyWithMultiply(m_scaledBuffer.getData(), dataBuffer, multFactor, size);
	AudioDataConverters::convertFloatToInt16LE(m_scaledBuffer.getData(), m_intBuffer.getData(), size);

    /* Get the file index that belongs to the current recording channel */
	int fileIndex = m_fileIndexes[writeChannel];

    /* Write the data to that file */
	m_DataFiles[fileIndex]->writeChannel(
		getTimestamp(writeChannel) - m_startTS[writeChannel],
		m_channelIndexes[writeChannel],
		m_intBuffer.getData(), size);

    /* If is first channel in subprocessor */
	if (m_channelIndexes[writeChannel] == 0)
    {

		int64 baseTS = getTimestamp(writeChannel);
		for (int i = 0; i < size; i++)
            /* Generate int timestamp */ 
            m_tsBuffer[i] = baseTS + i;     

        /* Write int timestamps to disc */
		m_dataTimestampFiles[fileIndex]->writeData(m_tsBuffer, size*sizeof(int64));
		m_dataTimestampFiles[fileIndex]->increaseRecordCount(size);

        //LOGD("BinaryRecording::writeSynchronizedData: ", *ftsBuffer);

        m_dataFloatTimestampFiles[fileIndex]->writeData(ftsBuffer, size*sizeof(double));
        m_dataFloatTimestampFiles[fileIndex]->increaseRecordCount(size);
        
	}
}

void BinaryRecording::writeData(int writeChannel, int realChannel, const float* buffer, int size)
{

    if (!size)
        return;

    /* If our internal buffer is too small to hold the data... */
	if (size > m_bufferSize) //shouldn't happen, but if does, this prevents crash...
	{
		std::cerr << "[RN] Write buffer overrun, resizing from: " << m_bufferSize << " to: " << size << std::endl;
		m_scaledBuffer.malloc(size);
		m_intBuffer.malloc(size);
		m_tsBuffer.malloc(size);
		m_bufferSize = size;
	}

    /* Convert signal from float to int w/ bitVolts scaling */
	double multFactor = 1 / (float(0x7fff) * getContinuousChannel(realChannel)->getBitVolts());
	FloatVectorOperations::copyWithMultiply(m_scaledBuffer.getData(), buffer, multFactor, size);
	AudioDataConverters::convertFloatToInt16LE(m_scaledBuffer.getData(), m_intBuffer.getData(), size);

    /* Get the file index that belongs to the current recording channel */
	int fileIndex = m_fileIndexes[writeChannel];

    /* Write the data to that file */
	m_DataFiles[fileIndex]->writeChannel(
		getTimestamp(writeChannel) - m_startTS[writeChannel],
		m_channelIndexes[writeChannel],
		m_intBuffer.getData(), size);

    /* If is first channel in subprocessor */
	if (m_channelIndexes[writeChannel] == 0)
    {

		int64 baseTS = getTimestamp(writeChannel);
		for (int i = 0; i < size; i++)
            /* Generate int timestamp */ 
            m_tsBuffer[i] = baseTS + i;

        /* Write int timestamps to disc */
		m_dataTimestampFiles[fileIndex]->writeData(m_tsBuffer, size*sizeof(int64));
		m_dataTimestampFiles[fileIndex]->increaseRecordCount(size);
        
	}

}

void BinaryRecording::writeEvent(int eventIndex, const MidiMessage& event)
{

    const EventChannel* info = getEventChannel(eventIndex);

	EventPtr ev = Event::deserialize(event, info);
	EventRecording* rec = m_eventFiles[eventIndex];
	if (!rec) return;

	if (ev->getEventType() == EventChannel::TTL)
	{

        TTLEvent* ttl = static_cast<TTLEvent*>(ev.get());

        int64 ts = ev->getTimestamp();
        rec->timestampFile->writeData(&ts, sizeof(int64));

        uint16 chan = ttl->getBit();;
        rec->channelFile->writeData(&chan, sizeof(uint16));
		
		int16 data = (ttl->getBit() + 1) * (ttl->getState() ? 1 : -1);
		rec->mainFile->writeData(&data, sizeof(int16));
        /*
		if (rec->extraFile)
			rec->extraFile->writeData(ttl->getTTLWordPointer(), info->getDataSize());
        */
	}
	else if (ev->getEventType() == EventChannel::TEXT)
	{

        TextEvent* text = static_cast<TextEvent*>(ev.get());

        int64 ts = text->getTimestamp();
        rec->timestampFile->writeData(&ts, sizeof(int64));

		rec->mainFile->writeData(ev->getRawDataPointer(), info->getDataSize());
	}

	writeEventMetadata(ev.get(), rec->metaDataFile.get());
	increaseEventCounts(rec);
	
}

void BinaryRecording::writeSpike(int electrodeIndex, const Spike* spike)
{

	const SpikeChannel* channel = getSpikeChannel(electrodeIndex);
    LOGDD("Got spike channel");
	EventRecording* rec = m_spikeFiles[m_spikeFileIndexes[electrodeIndex]];
    LOGDD("Got event recording");
	uint16 spikeChannel = m_spikeChannelIndexes[electrodeIndex];
    LOGDD("Got real spike channel");

	int totalSamples = channel->getTotalSamples() * channel->getNumChannels();
    LOGDD("Got total number of samples: ", totalSamples);

	if (totalSamples > m_bufferSize) //Shouldn't happen, and if it happens it'll be slow, but better this than crashing. Will be reset on file close and reset.
	{
		std::cerr << "(spike) Write buffer overrun, resizing to" << totalSamples << std::endl;
		m_bufferSize = totalSamples;
		m_scaledBuffer.malloc(totalSamples);
		m_intBuffer.malloc(totalSamples);
	}

	double multFactor = 1 / (float(0x7fff) * channel->getChannelBitVolts(0));
	FloatVectorOperations::copyWithMultiply(m_scaledBuffer.getData(), spike->getDataPointer(), multFactor, totalSamples);
	AudioDataConverters::convertFloatToInt16LE(m_scaledBuffer.getData(), m_intBuffer.getData(), totalSamples);
	rec->mainFile->writeData(m_intBuffer.getData(), totalSamples*sizeof(int16));

	int64 ts = spike->getTimestamp();
	rec->timestampFile->writeData(&ts, sizeof(int64));
	rec->channelFile->writeData(&spikeChannel, sizeof(uint16));

	uint16 sortedID = spike->getSortedID();
	rec->extraFile->writeData(&sortedID, sizeof(uint16));
	writeEventMetadata(spike, rec->metaDataFile.get());

	increaseEventCounts(rec);
	
}

void BinaryRecording::writeTimestampSyncText(uint64 streamId, int64 timestamp, float sourceSampleRate, String text)
{
    bool inHere = true;
	if (!m_syncTextFile)
		return;
	m_syncTextFile->writeText(text + "\n", false, false, nullptr);
    m_syncTextFile->flush();
}

RecordEngineManager* BinaryRecording::getEngineManager()
{
    RecordEngineManager* man = new RecordEngineManager("RAWBINARY", "Binary",
                                                       &(engineFactory<BinaryRecording>));
    EngineParameter* param;
    param = new EngineParameter(EngineParameter::BOOL, 0, "Record TTL full words", true);
    man->addParameter(param);
    return man;
}

void BinaryRecording::setParameter(EngineParameter& parameter)
{
	boolParameter(0, m_saveTTLWords);
}