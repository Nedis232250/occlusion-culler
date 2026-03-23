#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <fstream>

std::vector<float> load_vertices(const char* path) {
    std::vector<float> result;
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cout << "failed to open file\n";
        return result;
    }

    float x, y, z, r, g, b, a;
    char comma;
    while (file >> x >> comma >> y >> comma >> z >> comma >> r >> comma >> g >> comma >> b >> comma >> a) {
        result.push_back(x);
        result.push_back(y);
        result.push_back(z);
        result.push_back(r);
        result.push_back(g);
        result.push_back(b);
        result.push_back(a);
        file >> comma; // consume trailing comma if present, harmless if not
    }
    return result;
}

std::vector<float> load_indices(std::vector<float> input) {
    unsigned int offset, index;
    std::vector<float> result = input;

    for (unsigned int i = 0; i < input.size() / 7; i++) {
        offset = i * 7;
        index = i / 3;

        uint8_t r = (index >> 16) & 0xFF;
        uint8_t g = (index >> 8) & 0xFF;
        uint8_t b = (index >> 0) & 0xFF;

        result[offset + 3] = r / 255.0f;
        result[offset + 4] = g / 255.0f;
        result[offset + 5] = b / 255.0f;
        result[offset + 6] = 1.0f;
    }

    return result;
}
