#include <iostream>
#include <signal.h>
#include <sstream>

#include "Globals/AllSatGloblas.hpp"
#include "Globals/AllSatAlgoGlobals.hpp"
#include "Globals/AllSatConfig.hpp"
#include "Globals/AllSatPresets.hpp"
#include "AllSatAlgo/Algorithms.hpp"
#include "Utilities/InputParser.hpp"


using namespace std;

// define global algo for sigHandling
AllSatAlgoBase* allSatAlgo = nullptr;

// function for handling sig
// just print the result with wasInterrupted = true
void sigHandler(int s){
    printf("Caught signal %d\n",s);
    if(allSatAlgo != nullptr)
    {
        allSatAlgo->PrintResult(true);
        delete allSatAlgo;
    }
    exit(1); 
}

void PrintUsage()
{
    // TODO add additonal param like outfile, max models etc..
    cout << "USAGE: ./hall_tool <input_file_name> [</mode> <mode_name>] [additonal parameters]" << endl;
    cout << "where <input_file_name> is the path to a .aag or .aig instance in AIGER format" << endl << endl;
    // cout << "you can provide a pre-configured algorithm <mode_name> where " << DEF_ALG << " is the defualt one" << endl;
    cout << "\t accepted <mode_name> are [";
    for (size_t i = 0; i < MODES.size(); i++) {
        if (i != 0) {
            std::cout << ", ";
        }
        std::cout << MODES[i];
    }
    cout << "]" << endl;
    cout << "\t for example: ./hall_tool <input_file_name> /mode " << TERSIM_ALG << endl;
    cout << "\t default mode is: " << ROC_ALG << endl;

    // additonal parameters
    cout << endl;
    cout << "additonal parameters can be provided in [additonal parameters]:" << endl;
    cout << "Runnig example: \n\t ./hall_tool ../benchmarks/AND.aag /mode core /general/timeout 60 /general/print_enumer 1" << endl;

    cout << endl;
    cout << "General:" << endl;
    cout << "[</general/timeout> <value>] provide timeout in seconds, if <value> not provided use default of 3600 sec" << endl;
    cout << "[</general/print_enumer> <0|1>] represent if to print the enumerations found" << endl;
    cout << "[</general/projection_vars> <indices>] comma-separated AIGER input indices for projected enumeration" << endl;
    cout << "   Example: /general/projection_vars 1,3,5 - only enumerate over inputs with AIGINDEX 1, 3, and 5" << endl;

    cout << endl;
    cout << "Algorithm parameters:" << endl;

    cout << "[</alg/blocking/use_cirsim> <0|1>] if to use ternary simulation in the generalization" << endl;
    cout << "[</alg/blocking/use_top_to_bot_sim> <0|1>] if to use top to bottom simulation instead of bottom to top" << endl;
    cout << "[</alg/blocking/use_ucore> <0|1>] if to use unSAT-core in the generalization" << endl;
    cout << "[</alg/blocking/use_lit_drop> <0|1>] if to use unSAT-core minimazation with literal dropping" << endl;   
}

static bool ApplyBoolOverride(const InputParser& input, const string& key, bool& target)
{
    if (!input.cmdOptionExists(key))
    {
        return false;
    }

    target = input.getBoolCmdOption(key, target);
    return true;
}

static bool ApplyUIntOverride(const InputParser& input, const string& key, unsigned& target)
{
    if (!input.cmdOptionExists(key))
    {
        return false;
    }

    target = input.getUintCmdOption(key, target);
    return true;
}

static vector<AIGINDEX> ParseProjectionIndices(const string& projectionIndices)
{
    vector<AIGINDEX> indices;
    if (projectionIndices.empty())
    {
        return indices;
    }

    stringstream ss(projectionIndices);
    string token;
    while (getline(ss, token, ','))
    {
        size_t start = token.find_first_not_of(" \t");
        size_t end = token.find_last_not_of(" \t");
        if (start == string::npos)
        {
            continue;
        }
        token = token.substr(start, end - start + 1);

        try
        {
            indices.push_back((AIGINDEX)stoul(token));
        }
        catch (const exception& e)
        {
            throw runtime_error("Invalid projection AIGINDEX '" + token + "': " + e.what());
        }
    }

    return indices;
}

static bool TryGetPresetForMode(const string& mode, allsat::EnumerateOptions::Preset& outPreset)
{
    if (mode == TERSIM_ALG)
    {
        outPreset = allsat::EnumerateOptions::Preset::Tale;
        return true;
    }
    if (mode == DRMS_DISJOINT_ALG)
    {
        outPreset = allsat::EnumerateOptions::Preset::MarsDisjoint;
        return true;
    }
    if (mode == DRMS_NON_DISJOINT_ALG)
    {
        outPreset = allsat::EnumerateOptions::Preset::MarsNonDisjoint;
        return true;
    }
    if (mode == COMB_DISJOINT_BLOCK_ALG)
    {
        outPreset = allsat::EnumerateOptions::Preset::Duty;
        return true;
    }
    if (mode == CORE_ALG)
    {
        outPreset = allsat::EnumerateOptions::Preset::Core;
        return true;
    }
    if (mode == ROC_ALG)
    {
        outPreset = allsat::EnumerateOptions::Preset::Roc;
        return true;
    }
    if (mode == CARMA_ALG)
    {
        outPreset = allsat::EnumerateOptions::Preset::Carma;
        return true;
    }

    return false;
}


int main(int argc, char **argv) 
{
    InputParser cmdInput(argc, argv);

    if(argc < 2 || cmdInput.cmdOptionExists("-h") || cmdInput.cmdOptionExists("--h") || cmdInput.cmdOptionExists("-help") || cmdInput.cmdOptionExists("--help"))
    {
        PrintUsage();
        return 1;
    }

    string mode = cmdInput.getCmdOptionWDef("/mode", "roc");
    if (!mode.empty())
    {
        allsat::EnumerateOptions::Preset preset = allsat::EnumerateOptions::Preset::None;
        if (!TryGetPresetForMode(mode, preset))
        {
            cout << "Error, unkown mode provided" << endl;
            return -1;
        }

        AllSatConfig config;
        ApplyPreset(config, preset);

        if (cmdInput.cmdOptionExists("/alg"))
        {
            string alg = cmdInput.getCmdOptionWDef("/alg", "blocking");
            if (alg != "blocking")
            {
                cout << "Error, unkown algorithm type provided" << endl;
                return -1;
            }
        }

        if (cmdInput.cmdOptionExists("/alg/blocking/enc"))
        {
            string blockingEnc = cmdInput.getCmdOptionWDef("/alg/blocking/enc", "tseitin");
            if (blockingEnc == "tseitin")
            {
                config.encoding = AllSatConfig::Encoding::Tseitin;
            }
            else if (blockingEnc == "dual_rail")
            {
                config.encoding = AllSatConfig::Encoding::DualRail;
            }
            else
            {
                cout << "Error, unkown blocking enc type provided" << endl;
                return -1;
            }
        }

        ApplyBoolOverride(cmdInput, "/alg/blocking/use_cirsim", config.useCirSim);
        ApplyBoolOverride(cmdInput, "/alg/blocking/use_top_to_bot_sim", config.useTopToBottomSim);
        ApplyBoolOverride(cmdInput, "/alg/blocking/use_ucore", config.useUcore);
        ApplyBoolOverride(cmdInput, "/alg/blocking/use_lit_drop", config.useLitDrop);
        ApplyUIntOverride(cmdInput, "/alg/blocking/lit_drop_conflict_limit", config.litDropConflictLimit);
        ApplyBoolOverride(cmdInput, "/alg/blocking/lit_drop_recur_ucore", config.useLitDropRecur);

        ApplyBoolOverride(cmdInput, "/alg/blocking/use_ipasir_for_plain", config.useIpasirForPlain);
        ApplyBoolOverride(cmdInput, "/alg/blocking/use_ipasir_for_dual", config.useIpasirForDual);

        ApplyBoolOverride(cmdInput, "/alg/blocking/dual_rail/block_no_rep", config.dualBlockNoRep);
        ApplyBoolOverride(cmdInput, "/alg/blocking/dual_rail/force_pol", config.dualForcePolarity);
        ApplyBoolOverride(cmdInput, "/alg/blocking/dual_rail/boost_score", config.dualBoostScore);
        ApplyBoolOverride(cmdInput, "/alg/blocking/dual_rail/use_tseitin_for_dual", config.dualUseTseitinForDual);

        if (cmdInput.cmdOptionExists("/general/timeout"))
        {
            config.useTimeout = true;
            config.timeoutSeconds = cmdInput.getUintCmdOption("/general/timeout", config.timeoutSeconds);
        }

        ApplyBoolOverride(cmdInput, "/general/print_enumer", config.printEnumerations);
        ApplyBoolOverride(cmdInput, "/general/print_info", config.printInfo);

        if (cmdInput.cmdOptionExists("/general/projection_vars"))
        {
            config.projectionIndices = ParseProjectionIndices(cmdInput.getCmdOption("/general/projection_vars"));
        }

        ApplyUIntOverride(cmdInput, "/sat_solver/intel_sat/mode", config.intelSatMode);

        try
        {
            if (config.encoding == AllSatConfig::Encoding::Tseitin)
            {
                allSatAlgo = new AllSatAlgoTseitinEnc(config);
            }
            else
            {
                allSatAlgo = new AllSatAlgoDualRailEnc(config);
            }

            allSatAlgo->InitializeWithAIGFile(argv[1]);
        }
        catch (exception& ex)
        {
            delete allSatAlgo;
            cout << "Error while initilize the solver: " << ex.what() << endl;
            return -1;
        }
    }
    else
    {
        cout << "Error, please provide valid mode with \"/mode\" parameter" << endl;
        return -1;
    }


    // define sigaction for catchin ctr+c etc..
    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = sigHandler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    sigaction(SIGINT, &sigIntHandler, NULL);

    try
    { 
        allSatAlgo->FindAllEnumer();
        allSatAlgo->PrintResult();
    }
    catch (exception& ex)
    {
        delete allSatAlgo;
        cout << "Error acord: " << ex.what() << endl;
        return -1;
    }

    delete allSatAlgo;

    return 0;
}
