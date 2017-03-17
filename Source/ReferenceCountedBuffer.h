/*
  ==============================================================================

    ReferenceCountedBuffer.h
    Created: 10 Mar 2017 12:48:12pm
    Author:  manny

  ==============================================================================
*/


#ifndef REFERENCECOUNTEDBUFFER_H_INCLUDED
#define REFERENCECOUNTEDBUFFER_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

class ReferenceCountedBuffer : public ReferenceCountedObject
{
public:
        typedef ReferenceCountedObjectPtr<ReferenceCountedBuffer> Ptr;

        ReferenceCountedBuffer (const String& nameToUse,
                                int numChannels,
                                int numSamples);

        ~ReferenceCountedBuffer();

        AudioSampleBuffer* getAudioSampleBuffer();

        int position;

        Atomic<int> writeTo;

        Atomic<int> playFrom;

private:
        String name;
        AudioSampleBuffer buffer;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReferenceCountedBuffer)
};

#endif  // REFERENCECOUNTEDBUFFER_H_INCLUDED
