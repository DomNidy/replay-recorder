#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "encoder/encoder.h"

int main(int argc, char *argv[])
{
    // Validate command-line arguments.
    if (argc < 2 || argc > 3)
    {
        std::filesystem::path encPath(argv[0]);

        std::cerr << "Usage: \n"
                  << encPath.stem().generic_string() << " <input_file> [output_file] [option]...\n\n"
                  << "Options:\n"
                  << "\t--remove-special\t Removes special tokens.\n";
        return 1;
    }
    const std::string inputFilename = argv[1];
    std::filesystem::path outputFilename;
    if (argc >= 3)
    {
        outputFilename = argv[2];
    }
    else
    {
        // Derive output file path if one not provided
        std::filesystem::path inPath(inputFilename);
        outputFilename = inPath.parent_path() / (inPath.stem().string() + "_encoded" + inPath.extension().string());
    }

    // Load input file as a string.
    std::ifstream inputFile(inputFilename);
    if (!inputFile.is_open())
    {
        std::cerr << "Error opening input file: " << inputFilename << "\n";
        return 1;
    }
    std::stringstream buffer;
    buffer << inputFile.rdbuf();
    const std::string inputContent = buffer.str();

    // Encode using the rle method.
    const std::string encodedContent = RP::Encoder::rle(inputContent);

    // Compare lengths and compute percent change.
    const size_t originalLength = inputContent.size();
    const size_t encodedLength = encodedContent.size();
    const double percentChange =
        (originalLength == 0)
            ? 0.0
            : (static_cast<int>(encodedLength - originalLength) / static_cast<double>(originalLength)) * 100.0;

    std::cout << "Original length: " << originalLength << "\n";
    std::cout << "Encoded length: " << encodedLength << "\n";
    std::cout << "Percent change: " << std::fixed << std::setprecision(2) << percentChange << "%\n";

    // Write encoded content to output file
    std::ofstream outputFile(outputFilename);
    if (!outputFile.is_open())
    {
        std::cerr << "Error opening output file: " << outputFilename << "\n";
        return 1;
    }
    outputFile << encodedContent;
    outputFile.close();

    std::cout << "Encoded content written to: " << outputFilename << "\n";

    return 0;
}