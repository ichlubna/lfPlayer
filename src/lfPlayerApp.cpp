#include <iostream>
#include <string>
#include <algorithm>
#include "arguments/arguments.hpp"
#include "simulation.h"

int main(int argc, char **argv)
{
    constexpr float DEFAULT_SCALE{1.0};
    constexpr size_t DEFAULT_ITERATIONS{256};

    Arguments args(argc, argv);
    std::string path = static_cast<std::string>(args["-i"]);
    size_t iterations = static_cast<size_t>(args["-q"]);
    float scale = static_cast<float>(args["-s"]);

    std::string helpText{ "Usage:\n"
             "Example: lfPlayer -i /MyAmazingMachine/thatScene.lf -s 1.0 -q 256\n"  
             "-i - folder with lf images or video-encoded .lf file using lfEncoder app\n"
             "-- folder with lf grid images with row_column (start with zero) naming such as: 0_0.jpg, 0_1.jpg, ...\n"
             "-s - scale of focus map (0.0-1.0> where 1.0 is full resolution (same as the input LF)\n"
             "-q - number of scanned focusing distances\n"
             "-- higher -s and -q values mean better quality but worse performance\n"
            };
    if(args.printHelpIfPresent(helpText))
        return 0;

    if(path == "") 
    {
        std::cerr << "No path specified. Use -h for help." << std::endl;
        return EXIT_FAILURE;
    }

    if(scale == 0)
        scale = DEFAULT_SCALE;
    if(iterations == 0)
        iterations = DEFAULT_ITERATIONS;

    try
	{
		Simulation simulation(path, scale, iterations);
		simulation.run();
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
