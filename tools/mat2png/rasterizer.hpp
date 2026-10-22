#pragma once
#include "palette.hpp"
#include "parser.hpp"
#include <string>
#include <vector>
#include <array>
#include <filesystem>
#include <iostream>

#if __has_include("stb_image_write.h")
#include "stb_image_write.h"
#elif __has_include("../../third_party/stb_image_write.h")
#include "../../third_party/stb_image_write.h"
#else
#error "stb_image_write.h not found"
#endif

namespace spritex::tools {

struct RasterizerOptions {
    int scale{16};             // Nearest-neighbor scaling factor
    bool verbose{false};
};

class Rasterizer {
public:
    using Options = RasterizerOptions;

    struct RenderResult {
        bool success{false};
        std::string errorMessage;
        size_t outputWidth{0};
        size_t outputHeight{0};
        size_t uniqueColors{0};
    };

    static RenderResult to_png(const Matrix& matrix, const std::string& outputPath, const Options& opt = Options()) {
        RenderResult res;
        if (matrix.empty()) {
            res.errorMessage = "Cannot render an empty matrix.";
            return res;
        }

        int scale = opt.scale > 0 ? opt.scale : 1;
        size_t outW = matrix.width * scale;
        size_t outH = matrix.height * scale;
        res.outputWidth = outW;
        res.outputHeight = outH;

        // Tally unique colors used in this sprite
        std::array<bool, 256> seenColors{};
        size_t colorCount = 0;
        for (uint8_t token : matrix.data) {
            if (!seenColors[token]) {
                seenColors[token] = true;
                colorCount++;
            }
        }
        res.uniqueColors = colorCount;

        // Allocate RGBA pixel buffer
        std::vector<uint8_t> rgba(outW * outH * 4, 0);

        for (size_t y = 0; y < matrix.height; ++y) {
            for (size_t x = 0; x < matrix.width; ++x) {
                uint8_t token = matrix.at(x, y);
                Color c = Palette32::get(token);

                for (int sy = 0; sy < scale; ++sy) {
                    size_t py = y * scale + sy;
                    for (int sx = 0; sx < scale; ++sx) {
                        size_t px = x * scale + sx;
                        size_t idx = (py * outW + px) * 4;
                        rgba[idx + 0] = c.r;
                        rgba[idx + 1] = c.g;
                        rgba[idx + 2] = c.b;
                        rgba[idx + 3] = c.a;
                    }
                }
            }
        }

        // Ensure parent directory exists before writing
        try {
            std::filesystem::path p(outputPath);
            if (p.has_parent_path()) {
                std::filesystem::create_directories(p.parent_path());
            }
        } catch (const std::exception& e) {
            res.errorMessage = std::string("Failed to create destination directory: ") + e.what();
            return res;
        }

        // Write PNG using stb_image_write
        int writeResult = stbi_write_png(
            outputPath.c_str(),
            static_cast<int>(outW),
            static_cast<int>(outH),
            4,
            rgba.data(),
            static_cast<int>(outW * 4)
        );

        if (writeResult == 0) {
            res.errorMessage = "stbi_write_png failed to save file to: " + outputPath;
            return res;
        }

        res.success = true;
        return res;
    }
};

} // namespace spritex::tools
