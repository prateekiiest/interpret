// Copyright (c) 2018 Microsoft Corporation
// Licensed under the MIT license.
// Author: Paul Koch <code@koch.ninja>

// !!! NOTE: To add a new loss/objective function in C++, follow the steps listed at the top of the "Loss.cpp" file !!!

#include "Loss.h"

// TFloat could be double, float, or some SIMD intrinsic type
template <typename TFloat>
struct LossMultitaskBinaryLogLoss : public LossMultitaskBinary {
   // this one would more popularily be called LossMultilabelLogLoss.  We're currently calling this
   // LossMultitaskBinaryLogLoss since it fits better into our ontology of Multitask* types having 
   // multiple targets, but consider chaning this to multilabel since it would be more widely recognized that way

   // this one needs to be special cased!
};
