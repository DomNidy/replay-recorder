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
    std::string inputFilePath = argv[1];
    std::string outputFilePath;
    if (argc >= 3)
    {
        outputFilePath = argv[2];
    }
    else
    {
        // Derive output file path if one not provided
        std::filesystem::path inPath(inputFilePath);
        outputFilePath =
            (inPath.parent_path() / (inPath.stem().string() + "_encoded" + inPath.extension().string())).string();
    }

    // Load input file as a string.
    std::ifstream inFile(inputFilePath);
    if (!inFile)
    {
        std::cerr << "Error opening input file: " << inputFilePath << "\n";
        return 1;
    }
    std::stringstream buffer;
    buffer << inFile.rdbuf();
    std::string inputContent = buffer.str();

    // Encode using the rle method.
    std::string encodedContent = RP::Encoder::rle(inputContent);

    // Compare lengths and compute percent change.
    size_t originalLength = inputContent.size();
    size_t encodedLength = encodedContent.size();
    double percentChange =
        (originalLength == 0)
            ? 0.0
            : (static_cast<int>(encodedLength - originalLength) / static_cast<float>(originalLength)) * 100.0;

    std::cout << "Original length: " << originalLength << "\n";
    std::cout << "Encoded length: " << encodedLength << "\n";
    std::cout << "Percent difference: " << std::fixed << std::setprecision(3) << percentChange << "% "
              << (percentChange >= 0 ? "increase" : "decrease") << "\n";

    // Write the encoded content to file.
    std::ofstream outFile(outputFilePath);
    if (!outFile)
    {
        std::cerr << "Error opening output file: " << outputFilePath << "\n";
        return 1;
    }
    outFile << encodedContent;
    outFile.close();
    std::cout << "Encoded file written to: " << outputFilePath << "\n";

    return 0;
}