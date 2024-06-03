#include <iostream>
#include <signal.h>

#include "Globals/AllSatGloblas.hpp"
#include "Globals/AllSatAlgoGlobals.hpp"
#include "AllSatAlgo/Algorithms.hpp"


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

    cout << endl;
    cout << "Algorithm parameters:" << endl;

    cout << "[</alg/blocking/use_cirsim> <0|1>] if to use ternary simulation in the generalization" << endl;
    cout << "[</alg/blocking/use_top_to_bot_sim> <0|1>] if to use top to bottom simulation instead of bottom to top" << endl;
    cout << "[</alg/blocking/use_ucore> <0|1>] if to use unSAT-core in the generalization" << endl;
    cout << "[</alg/blocking/use_lit_drop> <0|1>] if to use unSAT-core minimazation with literal dropping" << endl;   
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
        auto itMap = MODE_PARAMS.find(mode);

		if (itMap == MODE_PARAMS.end())
		{
			cout << "Error, unkown mode provided" << endl;
            return -1;
		}

        cmdInput.AppendParams(itMap->second);
    }
    else
    {
        cout << "Error, please provide valid mode with \"/mode\" parameter" << endl;
        return -1;
    }

    // no dual-rail if cmd option or mode is tersim
    string alg = cmdInput.getCmdOptionWDef("/alg", "blocking");

    string blockingEnc = cmdInput.getCmdOptionWDef("/alg/blocking/enc", "tseitin");

    try
    {
        if (alg == "blocking")
        {
            if (blockingEnc == "tseitin")
            {
                allSatAlgo = new AllSatAlgoTseitinEnc(cmdInput);
            }
            else if (blockingEnc == "dual_rail")
            {  
                allSatAlgo = new AllSatAlgoDualRailEnc(cmdInput);
            }
            else
            {
                throw runtime_error("unkown blocking enc type provided");
                return -1;
            }
        }
        else
        {
            throw runtime_error("unkown algorithm type provided");
            return -1;
        }
        
        allSatAlgo->InitializeWithAIGFile(argv[1]);    
    }
    catch (exception& ex)
    {
        delete allSatAlgo;
        cout << "Error while initilize the solver: " << ex.what() << endl;
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
