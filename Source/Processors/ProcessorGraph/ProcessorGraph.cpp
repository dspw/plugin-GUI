/*
------------------------------------------------------------------

    This file is part of the Open Ephys GUI
    Copyright (C) 2014 Open Ephys

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

#include <stdio.h>
#include <utility>
#include <vector>
#include <map>

#include "ProcessorGraph.h"
#include "../GenericProcessor/GenericProcessor.h"

#include "../AudioNode/AudioNode.h"
#include "../RecordNode/RecordNode.h"
#include "../MessageCenter/MessageCenter.h"
#include "../Merger/Merger.h"
#include "../Splitter/Splitter.h"
#include "../../UI/UIComponent.h"
#include "../../UI/EditorViewport.h"
#include "../../UI/TimestampSourceSelection.h"
#include "../../UI/GraphViewer.h"

#include "../ProcessorManager/ProcessorManager.h"

ProcessorGraph::ProcessorGraph() : currentNodeId(100), isLoadingSignalChain(false)
{

    // The ProcessorGraph will always have 0 inputs (all content is generated within graph)
    // but it will have N outputs, where N is the number of channels for the audio monitor
    setPlayConfigDetails(0, // number of inputs
                         2, // number of outputs
                         44100.0, // sampleRate
                         1024);    // blockSize

}

ProcessorGraph::~ProcessorGraph()
{

}

void ProcessorGraph::createDefaultNodes()
{

    // add output node -- sends output to the audio card
    AudioProcessorGraph::AudioGraphIOProcessor* on =
        new AudioProcessorGraph::AudioGraphIOProcessor(AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode);

    // add record node -- sends output to disk
    //RecordNode* recn = new RecordNode();
    //recn->setNodeId(RECORD_NODE_ID);

    // add audio node -- takes all inputs and selects those to be used for audio monitoring
    AudioNode* an = new AudioNode();
    an->setNodeId(AUDIO_NODE_ID);

    // add message center
    MessageCenter* msgCenter = new MessageCenter();
    msgCenter->setNodeId(MESSAGE_CENTER_ID);

    addNode(on, OUTPUT_NODE_ID);
    //addNode(recn, RECORD_NODE_ID);
    addNode(an, AUDIO_NODE_ID);
    addNode(msgCenter, MESSAGE_CENTER_ID);

}

void ProcessorGraph::updatePointers()
{
    getAudioNode()->updateBufferSize();
}

void ProcessorGraph::moveProcessor(GenericProcessor* processor,
                                    GenericProcessor* newSource,
                                   GenericProcessor* newDest,
                                   bool moveDownstream)
{
    GenericProcessor* originalSource = processor->getSourceNode();
    GenericProcessor* originalDest = processor->getDestNode();
    
    if (originalSource != nullptr)
        originalSource->setDestNode(originalDest);
    
    if (originalDest != nullptr)
        originalDest->setSourceNode(originalSource);
    
    std::cout << "Processor to move: " << processor->getName() << std::endl;
    if (originalSource != nullptr)
        std::cout << "Original source: " << originalSource->getName() << std::endl;
    if (originalDest != nullptr)
        std::cout << "Original dest: " << originalDest->getName() << std::endl;
    if (newSource != nullptr)
        std::cout << "New source: " << newSource->getName() << std::endl;
    if (newDest != nullptr)
        std::cout << "New dest: " << newDest->getName() << std::endl;
    
    processor->setSourceNode(nullptr);
    processor->setDestNode(nullptr);
    
    if (newSource != nullptr)
    {
        if (!processor->isSource())
        {
            processor->setSourceNode(newSource);
            newSource->setDestNode(processor);
        } else {
            processor->setSourceNode(nullptr);
            newSource->setDestNode(nullptr);
            rootNodes.add(newSource);
        }
        
    }
    
    if (newDest != nullptr)
    {
        if (!newDest->isSource())
        {
            processor->setDestNode(newDest);
            newDest->setSourceNode(processor);
            if (rootNodes.contains(newDest))
            {
                rootNodes.set(rootNodes.indexOf(newDest), processor);
            }
        } else {
            processor->setDestNode(nullptr);
            rootNodes.add(processor);
        }
    }
    
    if (moveDownstream) // processor is further down the signal chain, its original dest may have changed
        updateSettings(originalDest);
    else // processor is upstream of its original dest, so we can just update that
        updateSettings(processor);
}

void ProcessorGraph::createProcessor(ProcessorDescription& description,
                                      GenericProcessor* sourceNode,
                                      GenericProcessor* destNode,
                                      bool signalChainIsLoading)
{
	GenericProcessor* processor = nullptr;
    
	try {
		processor = createProcessorFromDescription(description);
	}
	catch (std::exception& e) {
		NativeMessageBox::showMessageBoxAsync(AlertWindow::WarningIcon, "OpenEphys", e.what());
	}

	if (processor != nullptr)
	{
        //NativeMessageBox::showMessageBoxAsync(AlertWindow::WarningIcon, "OpenEphys", "HALP");
        
		if (processor->isSource()) // if we are adding a source processor
        {
            
            if (sourceNode != nullptr)
            {
                // if there's a source feeding into source, form a new signal chain
                processor->setDestNode(sourceNode->getDestNode());
                sourceNode->setDestNode(nullptr);
            }
            
            if (sourceNode == nullptr && destNode != nullptr)
            {
                // if we're adding it upstream of another processor
                if (!destNode->isSource())
                {
                    // if it's not a source, connect them
                    processor->setDestNode(destNode);
                    destNode->setSourceNode(processor);
                }
                else
                {
                    // if it's in front of a source, start a new signal chain
                    processor->setDestNode(nullptr);
                }
            }
            
			if (processor->isGeneratesTimestamps())
			{ // If there are no source processors and we add one,
              //  set it as default for global timestamps and sample rates
				m_validTimestampSources.add(processor);
				if (m_timestampSource == nullptr)
				{
					m_timestampSource = processor;
					m_timestampSourceSubIdx = 0;
				}
				if (m_timestampWindow)
					m_timestampWindow->updateProcessorList();
			}
            
        } else {
            // a source node was not dropped
            if (sourceNode != nullptr)
            {
                // if there's a source available, connect them
                processor->setSourceNode(sourceNode);
                sourceNode->setDestNode(processor);
            }
                
            if (destNode != nullptr)
            {
                if (!destNode->isSource())
                {
                    // if it's not behind a source node, connect them
                    processor->setDestNode(destNode);
                    destNode->setSourceNode(processor);
                } else {
                    // if there's a source downstream, start a new signalchain
                    processor->setDestNode(nullptr);
                }
            }
        }
        
        
        // assuming we get here
        int id = currentNodeId++;
        processor->setNodeId(id); // identifier within processor graph
        addNode(processor, id); // have to add it so it can be deleted by the graph
        GenericEditor* editor = (GenericEditor*) processor->createEditor();

        editor->refreshColors();
        
        if (!checkForNewRootNodes(processor))
        {
            removeProcessor(processor);
        }
        
	}
	else
	{
		CoreServices::sendStatusMessage("Not a valid processor.");
	}
    
    if (!signalChainIsLoading)
    {
        updateSettings(processor);
    } else {
        updateViews(processor);
    }
}

bool ProcessorGraph::checkForNewRootNodes(GenericProcessor* processor, bool processorBeingAdded)
{
    if (processorBeingAdded)
    {
        if (processor->getSourceNode() == nullptr)
        {
            if (processor->getDestNode() != nullptr)
            {
                
                if (rootNodes.indexOf(processor->getDestNode()) > -1)
                {
                    rootNodes.set(rootNodes.indexOf(processor->getDestNode()), processor);
                    return true;
                }
                
                if (processor->getDestNode()->isMerger())
                {
                    if (rootNodes.size() == 8)
                    {
                        NativeMessageBox::showMessageBoxAsync(AlertWindow::WarningIcon, "Signal chain error",
                                                              "Maximum of 8 signal chains.");
                        return false;
                    } else {
                        rootNodes.add(processor);
                        return true;
                    }
                    
                }
            } else {
                if (rootNodes.size() == 8)
                {
                    NativeMessageBox::showMessageBoxAsync(AlertWindow::WarningIcon, "Signal chain error",
                                                          "Maximum of 8 signal chains.");
                    return false;
                    
                } else {
                    rootNodes.add(processor);
                    return true;
                }
            }
        }
    } else {
        
        if (rootNodes.indexOf(processor))
        {
            rootNodes.remove(rootNodes.indexOf(processor));
        }
    }
    
    return true;
}

void ProcessorGraph::updateSettings(GenericProcessor* processor, bool signalChainIsLoading)
{
    // prevents calls from within processors from triggering full update during loading
    if (signalChainIsLoading != isLoadingSignalChain)
    {
        //updateViews(processor);
        return;
    }
        
    GenericProcessor* processorToUpdate = processor;
    
    Array<Splitter*> splitters;
    
    while ((processor != nullptr) || (splitters.size() > 0))
    {
        if (processor != nullptr)
        {
            processor->update();
            
            if (signalChainIsLoading)
            {
                processor->loadFromXml();
                processor->update();
            }
            
            if (processor->getSourceNode() != nullptr)
            {
                if (processor->isMerger())
                {
                    processor->setEnabledState(processor->isEnabled);
                } else {
                    processor->setEnabledState(processor->getSourceNode()->isEnabledState());
                }
            }
                
            else
            {
                if (processor->isSource())
                    processor->setEnabledState(processor->isEnabledState());
                else
                    processor->setEnabledState(false);
            }
                
                
            if (processor->isSplitter())
            {
                splitters.add((Splitter*) processor);
                processor = splitters.getLast()->getDestNode(0); // travel down chain 0 first
            } else {
                processor = processor->getDestNode();
            }
        }
        else {
            Splitter* splitter = splitters.getFirst();
            processor = splitter->getDestNode(1); // then come back to chain 1
            splitters.remove(0);
        }
    }
    
    updateViews(processorToUpdate);
    
}

void ProcessorGraph::updateViews(GenericProcessor* processor)
{
    AccessClass::getGraphViewer()->updateNodes(rootNodes);
    
    int tabIndex;
    
    if (processor == nullptr && rootNodes.size() > 0)
    {
        processor = rootNodes.getFirst();
    }
    
    Array<GenericEditor*> editorArray;
    GenericProcessor* rootProcessor = processor;
    
    while (processor != nullptr)
    {
        rootProcessor = processor;
        processor = processor->getSourceNode();
    }
    
    processor = rootProcessor;

    while (processor != nullptr)
    {
        editorArray.add(processor->getEditor());
        
        if (processor->getDestNode() != nullptr)
        {
            if (processor->getDestNode()->isMerger())
            {
                if (processor->getDestNode()->getSourceNode() != processor)
                {
                    MergerEditor* editor = (MergerEditor*) processor->getDestNode()->getEditor();
                    editor->switchSource();
                }
                    
            }
        }
        
        processor = processor->getDestNode();
    }
    
    AccessClass::getEditorViewport()->updateVisibleEditors(editorArray,
                                                           rootNodes.size(),
                                                           rootNodes.indexOf(rootProcessor));
    
}

void ProcessorGraph::viewSignalChain(int index)
{
    updateViews(rootNodes[index]);
}

void ProcessorGraph::deleteNodes(Array<GenericProcessor*> processorsToDelete)
{
    GenericProcessor* sourceNode = nullptr;
    GenericProcessor* destNode = nullptr;
    
    for (auto processor : processorsToDelete)
    {
        
        sourceNode = processor->getSourceNode();
        destNode = processor->getDestNode();
        
        if (sourceNode != nullptr)
        {
            sourceNode->setDestNode(destNode);
        }
        
        if (destNode != nullptr)
        {
            destNode->setSourceNode(sourceNode);
        }
        
        if (rootNodes.indexOf(processor) > -1)
        {
            if (destNode != nullptr)
                rootNodes.set(rootNodes.indexOf(processor), destNode);
            else
                rootNodes.remove(rootNodes.indexOf(processor));
        }
        
        removeProcessor(processor);
    }
    
    if (destNode != nullptr)
    {
        updateViews(destNode);
    } else {
        updateViews(sourceNode);
    }
        
}

void ProcessorGraph::clearSignalChain()
{

    Array<GenericProcessor*> processors = getListOfProcessors();

    for (int i = 0; i < processors.size(); i++)
    {
        removeProcessor(processors[i]);
    }
    
    rootNodes.clear();
    currentNodeId = 100;
    
    AccessClass::getGraphViewer()->removeAllNodes();

    updateViews(nullptr);
}

void ProcessorGraph::changeListenerCallback(ChangeBroadcaster* source)
{
    refreshColors();
}

void ProcessorGraph::refreshColors()
{
    for (auto p : getListOfProcessors())
    {
        GenericEditor* e = (GenericEditor*) p->getEditor();
        e->refreshColors();
    }
}

/* Set parameters based on XML.
void ProcessorGraph::loadParametersFromXml(GenericProcessor* processor)
{
    // DEPRECATED
    // Should probably do some error checking to make sure XML is valid, depending on how it treats errors (will likely just not update parameters, but error message could be nice.)
    int numberParameters = processor->getNumParameters();
    // Ditto channels. Not sure how to handle different channel sizes when variable sources (file reader etc. change). Maybe I should check number of channels vs source, but that requires hardcoding when source matters.
    //int numChannels=(targetProcessor->channels).size();
    //int numEventChannels=(targetProcessor->eventChannels).size();

    // Sets channel in for loop
    int currentChannel;

    // What the parameter name to change is.
    String parameterNameForXML;
    String parameterValue;
    float parameterFloat;

    forEachXmlChildElementWithTagName(*processor->parametersAsXml,
                                      channelXML,
                                      "CHANNEL")
    {
        currentChannel=channelXML->getIntAttribute("name");

        // std::cout <<"currentChannel:"<< currentChannel  << std::endl;
        // Sets channel to change parameter on
        processor->setCurrentChannel(currentChannel-1);

        forEachXmlChildElement(*channelXML, parameterXML)
        {

            for (int j = 0; j < numberParameters; ++j)
            {
                parameterNameForXML = processor->getParameterName(j);

                if (parameterXML->getStringAttribute("name")==parameterNameForXML)
                {
                    parameterValue=parameterXML->getAllSubText();
                    parameterFloat=parameterValue.getFloatValue();
                    processor->setParameter(j, parameterFloat);
                    // testGrab=targetProcessor->getParameterVar(j, currentChannel);
                    std::cout <<"Channel:" <<currentChannel<<"Parameter:" << parameterNameForXML << "Intended Value:" << parameterFloat << std::endl;
                }

            }
        }
    }
}*/


void ProcessorGraph::restoreParameters()
{
    
    isLoadingSignalChain = true;

    std::cout << "Restoring parameters for each processor..." << std::endl;
    
    for (auto p : rootNodes)
    {
        updateSettings(p, true);
    }
    
    isLoadingSignalChain = false;
    
}

bool ProcessorGraph::hasRecordNode()
{
    
    Array<GenericProcessor*> processors = getListOfProcessors();
    
    for (auto p : processors)
    {
        if (p->isRecordNode())
        {
            return true;
        }
    }
    return false;
    
}

Array<GenericProcessor*> ProcessorGraph::getListOfProcessors()
{

    Array<GenericProcessor*> allProcessors;

    for (int i = 0; i < getNumNodes(); i++)
    {
        Node* node = getNode(i);

        int nodeId = node->nodeId;

        if (nodeId != OUTPUT_NODE_ID &&
            nodeId != AUDIO_NODE_ID &&
            nodeId != RECORD_NODE_ID &&
            nodeId != MESSAGE_CENTER_ID)
        {
            GenericProcessor* p =(GenericProcessor*) node->getProcessor();
            allProcessors.add(p);
        }
    }

    return allProcessors;

}

void ProcessorGraph::clearConnections()
{

    for (int i = 0; i < getNumNodes(); i++)
    {
        Node* node = getNode(i);
        int nodeId = node->nodeId;

        if (nodeId != OUTPUT_NODE_ID)
        {

            if (nodeId != RECORD_NODE_ID && nodeId != AUDIO_NODE_ID)
            {
                disconnectNode(node->nodeId);
            }

            GenericProcessor* p = (GenericProcessor*) node->getProcessor();
            p->resetConnections();

        }
    }

    // connect audio subnetwork
    for (int n = 0; n < 2; n++)
    {

        addConnection(AUDIO_NODE_ID, n,
                      OUTPUT_NODE_ID, n);

    }

    for (auto& recordNode : getRecordNodes())
        addConnection(MESSAGE_CENTER_ID, midiChannelIndex,
                  recordNode->getNodeId(), midiChannelIndex);

}


void ProcessorGraph::updateConnections()
{

    clearConnections(); // clear processor graph

    Array<GenericProcessor*> splitters;

    // keep track of which splitter is currently being explored, in case there's another
    // splitter between the one being explored and its source.
    GenericProcessor* activeSplitter = nullptr;

    // stores the pointer to a source leading into a particular dest node
    // along with a boolean vector indicating the position of this source
    // relative to other sources entering the dest via mergers
    // (when the mergerOrder vectors of all incoming nodes to a dest are
    // lexicographically sorted, the sources will be in the correct order)
    struct ConnectionInfo
    {
        GenericProcessor* source;
        std::vector<int> mergerOrder;
        bool connectContinuous;
        bool connectEvents;

        // for SortedSet sorting:
        bool operator<(const ConnectionInfo& other) const
        {
            return mergerOrder < other.mergerOrder;
        }

        bool operator==(const ConnectionInfo& other) const
        {
            return mergerOrder == other.mergerOrder;
        }
    };

    // each destination node gets a set of sources, sorted by their order as dictated by mergers
    std::unordered_map<GenericProcessor*, SortedSet<ConnectionInfo>> sourceMap;

    for (int n = 0; n < rootNodes.size(); n++) // cycle through the tabs
    {
        std::cout << "Signal chain: " << n << std::endl;
        std::cout << std::endl;

        //GenericEditor* sourceEditor = (GenericEditor*) tabs[n]->getEditor();
        GenericProcessor* source = rootNodes[n];

        while (source != nullptr)// && destEditor->isEnabled())
        {
            std::cout << "Source node: " << source->getName() << "." << std::endl;
            GenericProcessor* dest = (GenericProcessor*) source->getDestNode();

            if (source->isReady())
            {
                //TODO: This is will be removed when probe based audio node added. 
                connectProcessorToAudioNode(source);

                //if (source->isRecordNode())
                 //   connectProcessorToMessageCenter(source);

                // find the next dest that's not a merger or splitter
                GenericProcessor* prev = source;

                ConnectionInfo conn;
                conn.source = source;
                conn.connectContinuous = true;
                conn.connectEvents = true;

                while (dest != nullptr && (dest->isMerger() || dest->isSplitter()))
                {
                    if (dest->isSplitter() && dest != activeSplitter && !splitters.contains(dest))
                    {
                        // add to stack of splitters to explore
                        splitters.add(dest);
                        dest->switchIO(0); // go down first path
                    }
                    else if (dest->isMerger())
                    {
                        auto merger = static_cast<Merger*>(dest);

                        // keep the input aligned with the current path
                        int path = merger->switchToSourceNode(prev);
                        jassert(path != -1); // merger not connected to prev?
                        
                        conn.mergerOrder.insert(conn.mergerOrder.begin(), path);
                        conn.connectContinuous &= merger->sendContinuousForSource(prev);
                        conn.connectEvents &= merger->sendEventsForSource(prev);
                    }

                    prev = dest;
                    dest = dest->getDestNode();
                }

                if (dest != nullptr)
                {
                    if (dest->isReady())
                    {
                        sourceMap[dest].add(conn);
                    }
                }
                else
                {
                    std::cout << "     No dest node." << std::endl;
                }
            }

            std::cout << std::endl;

            source->wasConnected = true;

            if (dest != nullptr && dest->wasConnected)
            {
                // don't bother retraversing downstream of a dest that has already been connected
                // (but if it leads to a splitter that is still in the stack, it may still be
                // used as a source for the unexplored branch.)

                std::cout << dest->getName() << " " << dest->getNodeId() <<
                    " has already been connected." << std::endl;
                std::cout << std::endl;
                dest = nullptr;
            }

            source = dest; // switch source and dest

            if (source == nullptr)
            {
                if (splitters.size() > 0)
                {
                    activeSplitter = splitters.getLast();
                    splitters.removeLast();
                    activeSplitter->switchIO(1);

                    source = activeSplitter;
                    GenericProcessor* newSource;
                    while (source->isSplitter() || source->isMerger())
                    {
                        newSource = source->getSourceNode();
                        newSource->setPathToProcessor(source);
                        source = newSource;
                    }
                }
                else
                {
                    activeSplitter = nullptr;
                }
            }

        } // end while source != 0
    } // end "tabs" for loop

    // actually connect sources to each dest processor,
    // in correct order by merger topography
    for (const auto& destSources : sourceMap)
    {
        GenericProcessor* dest = destSources.first;

        for (const ConnectionInfo& conn : destSources.second)
        {
            connectProcessors(conn.source, dest, conn.connectContinuous, conn.connectEvents);
        }
    }

    //OwnedArray<EventChannel> extraChannels;
    getMessageCenter()->addSpecialProcessorChannels();
	
	getAudioNode()->updatePlaybackBuffer();

    /*
    for (auto& recordNode : getRecordNodes())
        recordNode->addSpecialProcessorChannels(extraChannels);
    */

} // end method

void ProcessorGraph::connectProcessors(GenericProcessor* source, GenericProcessor* dest,
    bool connectContinuous, bool connectEvents)
{

    if (source == nullptr || dest == nullptr)
        return;

    std::cout << "     Connecting " << source->getName() << " " << source->getNodeId(); //" channel ";
    std::cout << " to " << dest->getName() << " " << dest->getNodeId() << std::endl;

    // 1. connect continuous channels
    if (connectContinuous)
    {
        for (int chan = 0; chan < source->getNumOutputs(); chan++)
        {
            //std::cout << chan << " ";

            addConnection(source->getNodeId(),         // sourceNodeID
                          chan,                        // sourceNodeChannelIndex
                          dest->getNodeId(),           // destNodeID
                          dest->getNextChannel(true)); // destNodeChannelIndex
        }
    }

    // 2. connect event channel
    if (connectEvents)
    {
        addConnection(source->getNodeId(),    // sourceNodeID
                      midiChannelIndex,       // sourceNodeChannelIndex
                      dest->getNodeId(),      // destNodeID
                      midiChannelIndex);      // destNodeChannelIndex
    }

    //3. If dest is a record node, register the processor
    if (dest->isRecordNode())
    {
        ((RecordNode*)dest)->registerProcessor(source);
    }

}

void ProcessorGraph::connectProcessorToAudioNode(GenericProcessor* source)
{

    /*
    std::cout << "#########SKIPPING CONNECT TO RECORD NODE" << std::endl;

    if (source == nullptr)
        return;
    */

    getAudioNode()->registerProcessor(source);
    //getRecordNode()->registerProcessor(source);

    for (int chan = 0; chan < source->getNumOutputs(); chan++)
    {

        getAudioNode()->addInputChannel(source, chan);

        addConnection(source->getNodeId(),                   // sourceNodeID
                      chan,                                  // sourceNodeChannelIndex
                      AUDIO_NODE_ID,                         // destNodeID
                      getAudioNode()->getNextChannel(true)); // destNodeChannelIndex
   

        /*
        getRecordNode()->addInputChannel(source, chan);

        addConnection(source->getNodeId(),                    // sourceNodeID
                      chan,                                   // sourceNodeChannelIndex
                      RECORD_NODE_ID,                         // destNodeID
                      getRecordNode()->getNextChannel(true)); // destNodeChannelIndex
        */
 
    }

    /*
    // connect event channel
    addConnection(source->getNodeId(),    // sourceNodeID
                  midiChannelIndex,       // sourceNodeChannelIndex
                  RECORD_NODE_ID,         // destNodeID
                  midiChannelIndex);      // destNodeChannelIndex
    */

    // connect event channel
    addConnection(source->getNodeId(),    // sourceNodeID
                  midiChannelIndex,       // sourceNodeChannelIndex
                  AUDIO_NODE_ID,          // destNodeID
                  midiChannelIndex);      // destNodeChannelIndex


    //getRecordNode()->addInputChannel(source, midiChannelIndex);

}


void ProcessorGraph::connectProcessorToMessageCenter(GenericProcessor* source)
{

    // connect event channel
    addConnection(getMessageCenter()->getNodeId(),    // sourceNodeID
                  midiChannelIndex,       // sourceNodeChannelIndex
                  source->getNodeId(),          // destNodeID
                  midiChannelIndex);      // destNodeChannelIndex

}


GenericProcessor* ProcessorGraph::createProcessorFromDescription(ProcessorDescription& description)
{
	GenericProcessor* processor = nullptr;

	if (description.fromProcessorList)
	{

		std::cout << "Creating from description..." << std::endl;
		std::cout << description.libName << "::" << description.processorName <<
        " (" << description.processorType << "-" << description.processorIndex << ")" << std::endl;

		processor = ProcessorManager::createProcessor((ProcessorClasses) description.processorType,
                                                      description.processorIndex);
	}
	else
	{
		std::cout << "Creating from plugin info..." << std::endl;
		std::cout << description.libName << "(" << description.libVersion << ")::" << description.processorName << std::endl;

		processor = ProcessorManager::createProcessorFromPluginInfo((Plugin::PluginType)
                                                                    description.processorType,
                                                                    description.processorIndex,
                                                                    description.processorName,
                                                                    description.libName,
                                                                    description.libVersion,
                                                                    description.isSource,
                                                                    description.isSink);
	}

	String msg = "New " + description.processorName + " created";
	CoreServices::sendStatusMessage(msg);

    return processor;
}


bool ProcessorGraph::processorWithSameNameExists(const String& name)
{
    for (int i = 0; i < getNumNodes(); i++)
    {
        Node* node = getNode(i);

        if (name.equalsIgnoreCase(node->getProcessor()->getName()))
            return true;

    }

    return false;

}


void ProcessorGraph::removeProcessor(GenericProcessor* processor)
{

    std::cout << "Removing processor with ID " << processor->getNodeId() << std::endl;

    int nodeId = processor->getNodeId();

    disconnectNode(nodeId);
    removeNode(nodeId);

	if (processor->isSource())
	{
		m_validTimestampSources.removeAllInstancesOf(processor);

		if (m_timestampSource == processor)
		{
			const GenericProcessor* newProc = 0;

			//Look for the next source node. If none is found, set the sourceid to 0
			for (int i = 0; i < getNumNodes() && newProc == nullptr; i++)
			{
				if (getNode(i)->nodeId != OUTPUT_NODE_ID)
				{
					GenericProcessor* p = dynamic_cast<GenericProcessor*>(getNode(i)->getProcessor());
					//GenericProcessor* p = static_cast<GenericProcessor*>(getNode(i)->getProcessor());
					if (p && p->isSource() && p->isGeneratesTimestamps())
					{
						newProc = p;
					}
				}
			}
			m_timestampSource = newProc;
			m_timestampSourceSubIdx = 0;
		}
		if (m_timestampWindow)
			m_timestampWindow->updateProcessorList();
	}

    checkForNewRootNodes(processor, false);
}

bool ProcessorGraph::enableProcessors()
{

    updateConnections();

    std::cout << "Enabling processors..." << std::endl;

    bool allClear;

    if (getNumNodes() < 4)
    {
        std::cout << "Not enough processors in signal chain to acquire data" << std::endl;
        AccessClass::getUIComponent()->disableCallbacks();
        return false;
    }

    for (int i = 0; i < getNumNodes(); i++)
    {

        Node* node = getNode(i);

        if (node->nodeId != OUTPUT_NODE_ID)
        {
            GenericProcessor* p = (GenericProcessor*) node->getProcessor();
            allClear = p->isReady();

            if (!allClear)
            {
                std::cout << p->getName() << " said it's not OK." << std::endl;
                //	sendActionMessage("Could not initialize acquisition.");
                AccessClass::getUIComponent()->disableCallbacks();
                return false;

            }
        }
    }

    for (int i = 0; i < getNumNodes(); i++)
    {

        Node* node = getNode(i);

        if (node->nodeId != OUTPUT_NODE_ID)
        {
            GenericProcessor* p = (GenericProcessor*) node->getProcessor();
            p->enableEditor();
            p->enableProcessor();
        }
    }

	//Update special channels indexes, at the end
	//To change, as many other things, when the probe system is implemented
    for (auto& node : getRecordNodes())
    {
        node->updateRecordChannelIndexes();
    }
	getAudioNode()->updateRecordChannelIndexes();

    //	sendActionMessage("Acquisition started.");
	m_startSoftTimestamp = Time::getHighResolutionTicks();
	if (m_timestampWindow)
		m_timestampWindow->setAcquisitionState(true);
    return true;
}

bool ProcessorGraph::disableProcessors()
{

    std::cout << "Disabling processors..." << std::endl;

    bool allClear;

    for (int i = 0; i < getNumNodes(); i++)
    {
        Node* node = getNode(i);
        if (node->nodeId != OUTPUT_NODE_ID )
        {
            GenericProcessor* p = (GenericProcessor*) node->getProcessor();
            std::cout << "Disabling " << p->getName() << std::endl;
			if (node->nodeId != MESSAGE_CENTER_ID)
				p->disableEditor();
            allClear = p->disableProcessor();

            if (!allClear)
            {
                //	sendActionMessage("Could not stop acquisition.");
                return false;
            }
        }
    }

    //AccessClass::getEditorViewport()->signalChainCanBeEdited(true);
	if (m_timestampWindow)
		m_timestampWindow->setAcquisitionState(false);
    //	sendActionMessage("Acquisition ended.");

    return true;
}

void ProcessorGraph::setRecordState(bool isRecording)
{

    for (int i = 0; i < getNumNodes(); i++)
    {
        Node* node = getNode(i);
        if (node->nodeId != OUTPUT_NODE_ID)
        {
            GenericProcessor* p = (GenericProcessor*) node->getProcessor();

            p->setRecording(isRecording);
        }
    }

}


AudioNode* ProcessorGraph::getAudioNode()
{

    Node* node = getNodeForId(AUDIO_NODE_ID);
    return (AudioNode*) node->getProcessor();

}

Array<RecordNode*> ProcessorGraph::getRecordNodes()
{

    Array<RecordNode*> recordNodes;

    Array<GenericProcessor*> processors = getListOfProcessors();

    for (int i = 0; i < processors.size(); i++)
    {
        if (processors[i]->isRecordNode())
            recordNodes.add((RecordNode*)processors[i]);
    }

    return recordNodes;

}


MessageCenter* ProcessorGraph::getMessageCenter()
{

    Node* node = getNodeForId(MESSAGE_CENTER_ID);
    return (MessageCenter*) node->getProcessor();

}


void ProcessorGraph::setTimestampSource(int sourceIndex, int subIdx)
{
	m_timestampSource = m_validTimestampSources[sourceIndex];
	if (m_timestampSource)
	{
		m_timestampSourceSubIdx = subIdx;
	}
	else
	{
		m_timestampSourceSubIdx = 0;
	}
}

void ProcessorGraph::getTimestampSources(Array<const GenericProcessor*>& validSources, int& selectedSource, int& selectedSubId) const
{
	validSources = m_validTimestampSources;
	getTimestampSources(selectedSource, selectedSubId);
}

void ProcessorGraph::getTimestampSources(int& selectedSource, int& selectedSubId) const
{
	if (m_timestampSource)
		selectedSource = m_validTimestampSources.indexOf(m_timestampSource);
	else
		selectedSource = -1;
	selectedSubId = m_timestampSourceSubIdx;
}

int64 ProcessorGraph::getGlobalTimestamp(bool softwareOnly) const
{
	if (softwareOnly || !m_timestampSource)
	{
		return (Time::getHighResolutionTicks() - m_startSoftTimestamp);
	}
	else
	{
		return static_cast<int64>((Time::highResolutionTicksToSeconds(Time::getHighResolutionTicks() - m_timestampSource->getLastProcessedsoftwareTime())
			* m_timestampSource->getSampleRate(m_timestampSourceSubIdx)) + m_timestampSource->getSourceTimestamp(m_timestampSource->getNodeId(), m_timestampSourceSubIdx));
	}
}

float ProcessorGraph::getGlobalSampleRate(bool softwareOnly) const
{
	if (softwareOnly || !m_timestampSource)
	{
		return Time::getHighResolutionTicksPerSecond();
	}
	else
	{
		return m_timestampSource->getSampleRate(m_timestampSourceSubIdx);
	}
}

uint32 ProcessorGraph::getGlobalTimestampSourceFullId() const
{
	if (!m_timestampSource)
		return 0;

	return GenericProcessor::getProcessorFullId(m_timestampSource->getNodeId(), m_timestampSourceSubIdx);
}

void ProcessorGraph::setTimestampWindow(TimestampSourceSelectionWindow* window)
{
	m_timestampWindow = window;
}
