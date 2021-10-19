/*
    ------------------------------------------------------------------

    This file is part of the Open Ephys GUI
    Copyright (C) 2013 Open Ephys

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

#ifndef __PHASEDETECTOR_H_F411F29D__
#define __PHASEDETECTOR_H_F411F29D__

#include <ProcessorHeaders.h>

enum PhaseType
{
    NO_PHASE = 0, RISING_POS, FALLING_POS, FALLING_NEG, RISING_NEG
};

enum DetectorType
{
    PEAK = 0, TROUGH, RISING_ZERO, FALLING_ZERO
};

/** Holds settings for one stream's phase detector*/
class PhaseDetectorSettings
{
public:
    /** Constructor -- sets default values*/
    PhaseDetectorSettings();

    ~PhaseDetectorSettings() { }

    /** Creates an event for a particular stream*/
    TTLEventPtr createEvent(int64 timestamp, bool state);

    int samplesSinceTrigger;

    float lastSample;

    bool isActive;
    bool wasTriggered;

    PhaseType currentPhase;
    DetectorType detectorType;

    int triggerChannel;
    int outputBit;
    int gateBit;

    EventChannel* eventChannel;
};

/**
    Uses peaks to estimate the phase of a continuous signal.

    @see GenericProcessor, PhaseDetectorEditor
*/
class PhaseDetector : public GenericProcessor
{
public:
    /** Constructor */
    PhaseDetector();

    /** Destructor*/
    ~PhaseDetector() { }

    /** Creates the custom editor for this plugin */
    AudioProcessorEditor* createEditor() override;

    /** Emits events at peaks, troughs, or zero-crossings*/
    void process (AudioBuffer<float>& buffer) override;

    /** Called when processor needs to update its settings*/
    void updateSettings() override;

    /** Called when a parameter is updated*/
    void parameterValueChanged(Parameter* param) override;

private:
    /** Called whenever a new event arrives*/
    void handleEvent (const EventChannel* channelInfo, const EventPacket& packet, int sampleNum) override;

    StreamSettings<PhaseDetectorSettings> settings;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhaseDetector);
};

#endif  // __PHASEDETECTOR_H_F411F29D__
