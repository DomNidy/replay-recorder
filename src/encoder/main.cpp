#include <iostream>
#include "encoder.h"

int main()
{
    std::cout << "RLE TEST: " << RP::Encoder::rle("abc") << "\n";
}