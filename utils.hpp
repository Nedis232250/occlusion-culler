#pragma once
#define NOMINMAX
#include <iostream>
#include <string>
#include <vector>
#include <fstream>

void load_vertices(std::vector<float>& result, const char* path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cout << "failed to open file\n";
        return;
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

std::vector<unsigned int> set_occlusion_viability(std::vector<float> vertices, unsigned int width, unsigned int height, float rx, float ry) {
    std::vector<unsigned int> result;

    unsigned int offset;
    float maxx, minx, maxy, miny, aabb_width, aabb_height;

    for (unsigned int i = 0; i < vertices.size() / 21; i++) {
        offset = i * 21;

        maxx = max(vertices[offset], max(vertices[offset + 7], vertices[offset + 14]));
        minx = min(vertices[offset], min(vertices[offset + 7], vertices[offset + 14]));
        maxy = max(vertices[offset + 1], max(vertices[offset + 8], vertices[offset + 15]));
        miny = min(vertices[offset + 1], min(vertices[offset + 8], vertices[offset + 15]));

        if (minx > 1.0f || maxx < -1.0f || miny > 1.0f || maxy < -1.0f) {
            result.push_back(2);
            continue;
        }

        aabb_width = width * (maxx - minx);
        aabb_height = height * (maxy - miny);

        if (aabb_height * cos(radians(rx)) < 16.0f || aabb_width < 16.0f || aabb_height < 16.0f || aabb_width * cos(radians(ry)) < 16.0f) {
            result.push_back(0);
            continue;
        }

        result.push_back(1);
    }

    return result;
}

void CPU_vertex_transformation(std::vector<float>& ogp, std::vector<float>& np, unsigned int vertex_count, float thetax, float thetay) {
    for (unsigned int i = 0; i < vertex_count * 7; i += 7) {
        float x = ogp[i];
        float y = ogp[i + 1];
        float z = 0.1f * (ogp[i + 2] + 10);

        float rx = radians(thetax);
        float ry = radians(thetay);

        float y1 = y * cos(rx) - z * sin(rx);
        float z1 = y * sin(rx) + z * cos(rx);
        float x1 = x;

        float xr = x1 * cos(ry) + z1 * sin(ry);
        float yr = y1;
        float zr = -x1 * sin(ry) + z1 * cos(ry);

        zr = max(0.25, zr);

        float invz = 1.0f / zr;
        float xp = (xr * invz);
        float yp = (yr * invz);

        np[i] = xp;
        np[i + 1] = yp;
        np[i + 2] = zr;
    }
}
