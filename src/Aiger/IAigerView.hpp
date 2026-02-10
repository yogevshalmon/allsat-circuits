#pragma once

#include <vector>

#include "Globals/AllSatGloblas.hpp"
#include "Aiger/AigAndGate.hpp"

class IAigerView
{
public:
    virtual ~IAigerView() = default;

    virtual const std::vector<AIGLIT>& GetInputs() const = 0;
    virtual const std::vector<AIGLIT>& GetOutputs() const = 0;
    virtual const std::vector<AigAndGate>& GetAndGated() const = 0;
    virtual const std::vector<bool>& GetIsIndexRef() const = 0;
    virtual const AIGINDEX GetMaxIndex() const = 0;
};
