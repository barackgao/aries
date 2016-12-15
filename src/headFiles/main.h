//
// Created by 高炜 on 16/12/9.
//

#ifndef ARIES_MAIN_H_H
#define ARIES_MAIN_H_H

#include <iostream>

#include "../../thirdParty/usr/include/gflags/gflags.h"
#include "../../thirdParty/usr/include/gflags/gflags_declare.h"

DECLARE_string(outputFileCoefficient);
DECLARE_string(logFile);
DECLARE_int64(logFrequency);
DECLARE_double(learningRate);
DECLARE_int64(maxIteration);
DECLARE_string(dataTrainFile);
DECLARE_string(dataLabelFile);
DECLARE_int64(columns);
DECLARE_int64(samples);
DECLARE_string(algorithm);

#endif //ARIES_MAIN_H_H
