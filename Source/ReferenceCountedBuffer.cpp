/*
  ==============================================================================

  ReferenceCountedBuffer.cpp
  Created: 10 Mar 2017 12:48:12pm
  Author:  manny

  ==============================================================================
*/

#include "ReferenceCountedBuffer.h"

ReferenceCountedBuffer::ReferenceCountedBuffer(const String& nameToUse,
                                               int numChannels,
                                               int numSamples)
        :   position (0),
            name (nameToUse),
            buffer (numChannels, numSamples)
{
        DBG (String ("Buffer named '")
             + name
             + "' constructed. numChannels = "
             + String (numChannels)
             + ", numSamples = "
             + String (numSamples));
}

ReferenceCountedBuffer::~ReferenceCountedBuffer()
{
        DBG (String ("Buffer named '") + name + "' destroyed");
}

AudioSampleBuffer* ReferenceCountedBuffer::getAudioSampleBuffer()
{
        return &buffer;
}
