#include <iostream>
#include <signal.h>

#include "AllSatGloblas.hpp"
#include "AllSatEnumerDualRail.hpp"

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
    cout << "USAGE: ./allsatenumr-aig <input_file_name> <--no_rep> <--print_model> <-topor_mode value>" << endl;
    cout << "where <input_file_name> is the path to a aag instance in Aiger format" << endl;
    cout << "where <--no_rep> represent if to not use repetition" << endl;
    cout << "where <--print_model> represent if to print the enumerations found" << endl;
    cout << "where <-topor_mode value> represent the topor mode" << endl;
    cout << "\t Accepeted Values: [0,1,2,3,4,5,6,7] \n\t defualt value: 5" << endl;
    cout << "Runnig example: \n\t ./allsatenumer-aig ../benchmarks/halfadder.aag --no_rep -topor_mode 6" << endl;
}


int main(int argc, char **argv) 
{
    InputParser cmdInput(argc, argv);

    if(argc < 2 || cmdInput.cmdOptionExists("-h") || cmdInput.cmdOptionExists("--h"))
    {
        PrintUsage();
        return 1;
    }

	allSatAlgo = new AllSatEnumerDualRail(cmdInput);

    // define sigaction for catchin ctr+c etc..
    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = sigHandler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    sigaction(SIGINT, &sigIntHandler, NULL);

    try
    { 
        allSatAlgo->InitializeSolver(argv[1]);
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
