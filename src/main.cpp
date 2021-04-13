#include <iostream>
#include <string>
#include "simulation.h"

int main(int argc, char **argv)
{
    if(argc <= 1)
    {
        std::cerr << "No arguments used. Use -h for help." << std::endl;
        return EXIT_FAILURE;
    }
    else if (std::string(argv[1]) == "-h")
    {
        std::cerr << "Usage: lfPlayer file.mkv" << std::endl;
        return EXIT_FAILURE;
    }

	try
	{
		Simulation simulation((std::string(argv[1])));
		simulation.run();
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
