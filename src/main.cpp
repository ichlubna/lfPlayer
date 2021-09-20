#include <iostream>
#include <string>
#include <algorithm>
#include "simulation.h"

int main(int argc, char **argv)
{
    std::vector<std::string> args{"file"};
    for(int i=1; i<argc; i++)
        args[i-1] = argv[i];

    if(argc <= 1) 
    {
        std::cerr << "No arguments used. Use -h for help." << std::endl;
        return EXIT_FAILURE;
    }
    else if (std::find(args.begin(), args.end(), "-h") != args.end())
    {
        std::cerr << "Usage:" << std::endl <<
                 "lfPlayer path scale iterations" << std::endl << 
                 "-- folder with lf grid images with row_column (start with zero) naming such as: 0_0.jpg, 0_1.jpg, ..." << std::endl;
        return EXIT_FAILURE;
    }

	try
	{
		Simulation simulation(args[0], std::stof(args[1]), std::stoi(args[2]));
		simulation.run();
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
