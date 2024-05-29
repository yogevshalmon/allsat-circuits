#pragma once

#include "Topor.hpp"

// TODO enable this -> change SATLIT size to SOLVER_LIT_SIZE
// define the lit size for topor
// #ifdef SAT_SOLVER_LIT_64
//     typedef int32_t SOLVER_LIT_SIZE;
// #else
//     typedef int32_t SOLVER_LIT_SIZE;
// #endif
typedef int32_t SOLVER_LIT_SIZE;

// define the index size for topor
#ifdef SAT_SOLVER_INDEX_64
	typedef uint64_t SOLVER_INDEX_SIZE;
#else
	typedef uint32_t SOLVER_INDEX_SIZE;
#endif

// define if to use compress mode
#ifdef SAT_SOLVER_COMPRESS
    static constexpr bool SOLVER_COMPRESS = true;
#else
    static constexpr bool SOLVER_COMPRESS = false;
#endif


static constexpr int ToporBadRetVal = -1;
static constexpr int ToporSatRetVal = 10;
static constexpr int ToporUnSatRetVal = 20;
static constexpr int ToporTimeOutRetVal = 0;

static int GetToporResult(const Topor::TToporReturnVal& res)
{
    switch (res)
    {
    case Topor::TToporReturnVal::RET_SAT:
        //std::cout << "c Enumeration found" << std::endl;
        return ToporSatRetVal;
    case Topor::TToporReturnVal::RET_UNSAT:
        //std::cout << "UNSATISFIABLE" << std::endl << "c No more assignments found" << std::endl;
        return ToporUnSatRetVal;
    case Topor::TToporReturnVal::RET_TIMEOUT_LOCAL:
        std::cout << "c TIMEOUT_LOCAL" << std::endl;
        return ToporBadRetVal;
    case Topor::TToporReturnVal::RET_CONFLICT_OUT:
        std::cout << "c CONFLICT_OUT" << std::endl;
        return ToporBadRetVal;
    case Topor::TToporReturnVal::RET_MEM_OUT:
        std::cout << "c MEMORY_OUT" << std::endl;
        return ToporBadRetVal;
    case Topor::TToporReturnVal::RET_USER_INTERRUPT:
        std::cout << "c USER_INTERRUPT" << std::endl;
        return ToporBadRetVal;
    case Topor::TToporReturnVal::RET_INDEX_TOO_NARROW:
        std::cout << "c INDEX_TOO_NARROW" << std::endl;
        return ToporBadRetVal;
    case Topor::TToporReturnVal::RET_PARAM_ERROR:
        std::cout << "c PARAM_ERROR" << std::endl;
        return ToporBadRetVal;
    case Topor::TToporReturnVal::RET_ASSUMPTION_REQUIRED_ERROR:
        std::cout << "c ASSUMPTION_REQUIRED_ERROR" << std::endl;
        return ToporBadRetVal;
    case Topor::TToporReturnVal::RET_TIMEOUT_GLOBAL:
        //std::cout << "c TIMEOUT_GLOBAL" << std::endl;
        return ToporTimeOutRetVal;
    case Topor::TToporReturnVal::RET_DRAT_FILE_PROBLEM:
        std::cout << "c DRAT_FILE_PROBLEM" << std::endl;
        return ToporBadRetVal;
    case Topor::TToporReturnVal::RET_EXOTIC_ERROR:
        std::cout << "c EXOTIC_ERROR" << std::endl;
        return ToporBadRetVal;
    default:
        std::cout << "s UNEXPECTED_ERROR" << std::endl;
        return ToporBadRetVal;
    }
}        