#pragma once
#define NOMINMAX
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

#define PI 3.14159
float radians(float degrees) {
    return PI * (degrees / 180.0f);
}

std::vector<unsigned int> set_occlusion_viability(std::vector<float> vertices, unsigned int width, unsigned int height, float rx) {
    std::vector<unsigned int> result;

    unsigned int offset;
    float maxx, minx, maxy, miny, aabb_width, aabb_height;
    unsigned int x = 0;

    for (unsigned int i = 0; i < vertices.size() / 21; i++) {
        offset = i * 21;

        maxx = max(vertices[offset] / (10 * (vertices[offset + 2] + 10.0f)), max(vertices[offset + 7] / (10 * (vertices[offset + 9] + 10.0f)), vertices[offset + 14] / (10 * (vertices[offset + 16] + 10.0f))));
        minx = min(vertices[offset] / (10 * (vertices[offset + 2] + 10.0f)), min(vertices[offset + 7] / (10 * (vertices[offset + 9] + 10.0f)), vertices[offset + 14] / (10 * (vertices[offset + 16] + 10.0f))));
        maxy = max(vertices[offset + 1] / (10 * (vertices[offset + 2] + 10.0f)), max(vertices[offset + 8] / (10 * (vertices[offset + 9] + 10.0f)), vertices[offset + 15] / (10 * (vertices[offset + 16] + 10.0f))));
        miny = min(vertices[offset + 1] / (10 * (vertices[offset + 2] + 10.0f)), min(vertices[offset + 8] / (10 * (vertices[offset + 9] + 10.0f)), vertices[offset + 15] / (10 * (vertices[offset + 16] + 10.0f))));

        aabb_width = width * (maxx - minx);
        aabb_height = height * (maxy - miny);

        if (i == 0) {
            std::cout << aabb_width << "\n";
            std::cout << cos(radians(rx)) << "\n";
        }

        if (aabb_height * cos(radians(rx)) < 8.0f || aabb_width < 8.0f) {
            result.push_back(0);
            x++;
            continue;
        }

        result.push_back(1);
    }

    std::cout << x << "\n";

    return result;
}

std::vector<float> load_terrain_triangulation(unsigned int nx, unsigned int nz) {
    std::vector<float> vertices;
    for (float z = 1.0f; z < 16.0f; z += 16.0f / nz) {
        for (float x = -1.0f; x < 1.0f; x += 2.0f / nx) {
            float centerx = x + 1.0f / nx;
            float centerz = z + 8.0f / nz;
            vertices.push_back(centerx);
            vertices.push_back(0);
            vertices.push_back(10 * centerz - 10);
            vertices.push_back(1);
            vertices.push_back(1);
            vertices.push_back(1);
            vertices.push_back(1);

            vertices.push_back(x);
            vertices.push_back(0);
            vertices.push_back(10 * z - 10);
            vertices.push_back(1);
            vertices.push_back(1);
            vertices.push_back(1);
            vertices.push_back(1);

            vertices.push_back(x + 2.0f / nx);
            vertices.push_back(0);
            vertices.push_back(10 * z - 10);
            vertices.push_back(1);
            vertices.push_back(1);
            vertices.push_back(1);
            vertices.push_back(1);



            vertices.push_back(centerx);
            vertices.push_back(0);
            vertices.push_back(10 * centerz - 10);
            vertices.push_back(1);
            vertices.push_back(1);
            vertices.push_back(1);
            vertices.push_back(1);

            vertices.push_back(x);
            vertices.push_back(0);
            vertices.push_back(10 * (z + 16.0f / nz) - 10);
            vertices.push_back(1);
            vertices.push_back(1);
            vertices.push_back(1);
            vertices.push_back(1);

            vertices.push_back(x + 2.0f / nx);
            vertices.push_back(0);
            vertices.push_back(10 * (z + 16.0f / nz) - 10);
            vertices.push_back(1);
            vertices.push_back(1);
            vertices.push_back(1);
            vertices.push_back(1);



            vertices.push_back(centerx);
            vertices.push_back(0);
            vertices.push_back(10 * centerz - 10);
            vertices.push_back(1);
            vertices.push_back(1);
            vertices.push_back(1);
            vertices.push_back(1);

            vertices.push_back(x);
            vertices.push_back(0);
            vertices.push_back(10 * z - 10);
            vertices.push_back(1);
            vertices.push_back(1);
            vertices.push_back(1);
            vertices.push_back(1);

            vertices.push_back(x);
            vertices.push_back(0);
            vertices.push_back(10 * (z + 16.0f / nz) - 10);
            vertices.push_back(1);
            vertices.push_back(1);
            vertices.push_back(1);
            vertices.push_back(1);



            vertices.push_back(centerx);
            vertices.push_back(0);
            vertices.push_back(10 * centerz - 10);
            vertices.push_back(1);
            vertices.push_back(1);
            vertices.push_back(1);
            vertices.push_back(1);

            vertices.push_back(x + 2.0f / nx);
            vertices.push_back(0);
            vertices.push_back(10 * z - 10);
            vertices.push_back(1);
            vertices.push_back(1);
            vertices.push_back(1);
            vertices.push_back(1);

            vertices.push_back(x + 2.0f / nx);
            vertices.push_back(0);
            vertices.push_back(10 * (z + 16.0f / nz) - 10);
            vertices.push_back(1);
            vertices.push_back(1);
            vertices.push_back(1);
            vertices.push_back(1);
        }
    }

    return vertices;
}
