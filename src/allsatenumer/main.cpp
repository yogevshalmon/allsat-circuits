#include <iostream>
#include <signal.h>

#include "AllSatGloblas.hpp"
#include "AllSatEnumerDualRail.hpp"
#include "AllSatEnumer.hpp"

using namespace std;

// define global algo for sigHandling
AllSatEnumerBase* allSatAlgo = nullptr;

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
    // TODO
    // timeout
    // max ...
    // outfile
    cout << "USAGE: ./hall_tool <input_file_name> [<-mode> <'MODE'>] [additonal parameters]" << endl;
    cout << "where <input_file_name> is the path to a aag or aig instance in AIGER format" << endl;
    cout << "you can provide a pre-configured mode <'MODE'> from the list [";
    for (size_t i = 0; i < MODES.size(); i++) {
        if (i != 0) {
            std::cout << ", ";
        }
        std::cout << MODES[i];
    }
    cout << "]" << endl;
    cout << "\t for example: ./hall_tool <input_file_name> -mode comb-dis-block " << endl;

    // additonal parameters
    cout << endl;
    cout << "additonal parameters can be provided in [additonal parameters]:" << endl;
    cout << "Runnig example: \n\t ./allsatenumer-aig ../benchmarks/halfadder.aag --no_rep -satsolver_mode 6" << endl;

    cout << endl;
    cout << "General:" << endl;
    cout << "<--print_models> represent if to print the enumerations found" << endl;
    cout << "<-satsolver_mode value> represent the sat solver mode" << endl;
    cout << "\t Accepeted Values: [0,1,2,3,4,5,6,7] \n\t defualt value: 5" << endl;

    cout << endl;
    cout << "Algorithm parameters:" << endl;

    cout << "[--no_dr] disable dual-rail encoding" << endl;
    cout << "[--no_tersim] disable teranry simulation mode" << endl;
    cout << "[<-dr_block_mode> <mode>] dual-rail blocking mode" << endl;
    cout << "\t Accepeted Values: [0 (non-disoint), 1 (disjoint)] \n\t defualt value: 1" << endl;
    cout << "[--dr_no_force_pol] disable dual-rail force polarity" << endl;
    cout << "[--dr_no_boost] disable dual-rail boos score" << endl;    
}


int main(int argc, char **argv) 
{
    InputParser cmdInput(argc, argv);

    if(argc < 2 || cmdInput.cmdOptionExists("-h") || cmdInput.cmdOptionExists("--h") || cmdInput.cmdOptionExists("-help") || cmdInput.cmdOptionExists("--help"))
    {
        PrintUsage();
        return 1;
    }

    string mode = cmdInput.getCmdOption("-mode");

    if (!mode.empty())
    {
        auto it = std::find(MODES.begin(), MODES.end(), mode);
        if (it == MODES.end())
        {
            cout << "Error, unkown mode provided" << endl;
            return -1;
        }
    }

    // no dual-rail if cmd option or mode is tersim
    bool noUseDRAlgo = cmdInput.cmdOptionExists("--no_dr") || (mode == TERSIM_ALG);

    try
    { 
        if (noUseDRAlgo)
        {
 	        allSatAlgo = new AllSatEnumer(cmdInput);
        }
        else
        {
            allSatAlgo = new AllSatEnumerDualRail(cmdInput);
        }
        allSatAlgo->InitializeSolver(argv[1]);    
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
