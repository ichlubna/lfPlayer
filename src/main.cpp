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
        std::cerr << "Usage:" << std::endl <<
                 "lfPlayer file.mkv" << std::endl <<
                 "- video with h265 stream" << std::endl <<
                 "or" << std::endl <<
                 "lfPlayer folderWithImages" << std::endl <<
                 "- folder with lf grid images with row_column naming such as: 0_0.jpg, 0_1.jpg, ..." << std::endl;
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
