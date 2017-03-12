/*
  ==============================================================================

    strechotron.cpp
    Created: 11 Mar 2017 1:10:34pm
    Author:  manny

  ==============================================================================
*/

#include "strechotron.h"
#include "ReferenceCountedBuffer.h"

class stretchotron : public Thread {
public:
        //---------------c'tors-------------------//
        stretchotron(ReferenceCountedBuffer &inputBuffer,
                     size_t sampleRate,
                     size_t numChannels,
                     int options =
                     RubberBand::RubberBandStretcher::OptionProcessRealTime) :
                Thread("stretching-thread"),
                stretcher(sampleRate,
                          numChannels,
                          options),
                workingBuf(inputBuffer)
        {}

        //--------------methods-------------------//

        void run() override{
                // process up to boundary, (need reference to atomic limiter...
        }

private:
        RubberBand::RubberBandStretcher stretcher;
        ReferenceCountedBuffer& workingBuf; 
};
