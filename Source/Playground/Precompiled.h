/*
  ==============================================================================

    Precompiled.h
    Created: 28 Sep 2015 10:45:27am
    Author:  Peter Rudenko

  ==============================================================================
*/

#ifndef PRECOMPILED_H_INCLUDED
#define PRECOMPILED_H_INCLUDED

#include "JuceHeader.h"

#define TINYRNN_OPENCL_ACCELERATION 1
#include "TinyRNN.h"

#if defined TRAINING_MODE
#include <algorithm>
#include <math.h>
#include <float.h>
#endif

#endif  // PRECOMPILED_H_INCLUDED
