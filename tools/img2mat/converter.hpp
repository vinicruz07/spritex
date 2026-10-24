#pragma once
#include "quantizer.hpp"
#include "../mat2png/rasterizer.hpp"
#include "../mat2png/parser.hpp"

#if __has_include("stb_image.h")
#include "stb_image.h"
#elif __has_include("../../third_party/stb_image.h")
#include "../../third_party/stb_image.h"
#else
#error "stb_image.h not found"
#endif

#include <string>
#include <fstream>
#include <memory>
#include <array>

namespace spritex::tools {

struct ConvertResult {
    bool success{false};
    std::string error;
    Matrix matrix;
    int originalWidth{0};
    int originalHeight{0};
    size_t uniqueColors{0};
};

class ImageConverter {
public:
    static ConvertResult from_file(const std::string& imagePath, const QuantizerOptions& opt) {
        ConvertResult res;

        int width = 0, height = 0, channels = 0;
        // Force 4 channels (RGBA)
        uint8_t* data = stbi_load(imagePath.c_str(), &width, &height, &channels, 4);

        if (!data) {
            res.error = "Failed to load image file: " + imagePath + " (" + stbi_failure_reason() + ")";
            return res;
        }

        res.originalWidth = width;
        res.originalHeight = height;

        // Process with quantizer
        res.matrix = ColorQuantizer::process_image(data, width, height, opt);
        stbi_image_free(data);

        if (res.matrix.empty()) {
            res.error = "Processing resulted in an empty matrix.";
            return res;
        }

        // Tally unique colors
        std::array<bool, 256> seen{};
        size_t count = 0;
        for (uint8_t t : res.matrix.data) {
            if (!seen[t]) {
                seen[t] = true;
                count++;
            }
        }
        res.uniqueColors = count;
        res.success = true;
        return res;
    }

    static ConvertResult from_memory(const uint8_t* buffer, int len, const QuantizerOptions& opt) {
        ConvertResult res;

        int width = 0, height = 0, channels = 0;
        uint8_t* data = stbi_load_from_memory(buffer, len, &width, &height, &channels, 4);

        if (!data) {
            res.error = "Failed to decode image from memory.";
            return res;
        }

        res.originalWidth = width;
        res.originalHeight = height;
        res.matrix = ColorQuantizer::process_image(data, width, height, opt);
        stbi_image_free(data);

        if (res.matrix.empty()) {
            res.error = "Processing resulted in an empty matrix.";
            return res;
        }

        std::array<bool, 256> seen{};
        size_t count = 0;
        for (uint8_t t : res.matrix.data) {
            if (!seen[t]) {
                seen[t] = true;
                count++;
            }
        }
        res.uniqueColors = count;
        res.success = true;
        return res;
    }

    static bool save_matrix_file(const Matrix& mat, const std::string& outputPath, std::string& err) {
        std::string content = MatrixParser::format_matrix(mat);
        std::ofstream file(outputPath);
        if (!file.is_open()) {
            err = "Could not open output file for writing: " + outputPath;
            return false;
        }
        file << content;
        return true;
    }
};

} // namespace spritex::tools
