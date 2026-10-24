#pragma once
#include "../mat2png/palette.hpp"
#include "../mat2png/parser.hpp"

#include <vector>
#include <array>
#include <cmath>
#include <algorithm>
#include <cstdint>

namespace spritex::tools {

enum class DownsampleMode {
    Modal,    // Picks the most frequent DB32 color token in the block (ideal for Midjourney/AI art)
    Center,   // Picks the center pixel of the block (ideal for clean upscaled pixel art)
    Average   // Averages RGB values in the block before quantizing
};

struct QuantizerOptions {
    int targetWidth{32};
    int targetHeight{32};
    uint8_t alphaThreshold{128};
    bool keyWhite{false};       // Treat near-white (#F0F0F0+) as transparent
    bool keyMagenta{false};     // Treat bright magenta as transparent (#FF00FF)
    bool outline{false};        // Automatically add black contour (token 1) around character
    bool despeckle{true};       // Remove orphan/isolated 1x1 noise pixels
    DownsampleMode mode{DownsampleMode::Modal};
};

class ColorQuantizer {
public:
    // Fast CompuPhase Perceptual Color Distance
    // Matches human eye sensitivity (CIE76 approximation)
    static inline long color_distance(uint8_t r1, uint8_t g1, uint8_t b1,
                                     uint8_t r2, uint8_t g2, uint8_t b2) {
        long rmean = ((long)r1 + (long)r2) / 2;
        long dr = (long)r1 - (long)r2;
        long dg = (long)g1 - (long)g2;
        long db = (long)b1 - (long)b2;
        return (((512 + rmean) * dr * dr) >> 8) + 4 * dg * dg + (((767 - rmean) * db * db) >> 8);
    }

    // Maps RGBA to closest DawnBringer 32 (DB32) token (0 to 31)
    static uint8_t map_to_db32(uint8_t r, uint8_t g, uint8_t b, uint8_t a, const QuantizerOptions& opt) {
        if (a < opt.alphaThreshold) {
            return 0; // Transparent
        }

        if (opt.keyWhite && r >= 240 && g >= 240 && b >= 240) {
            return 0; // Key out white background
        }

        if (opt.keyMagenta && r >= 230 && g <= 35 && b >= 230) {
            return 0; // Key out magenta chroma key
        }

        // Find closest color among DB32 tokens 1 to 31
        uint8_t bestToken = 1;
        long minDistance = -1;

        for (uint8_t i = 1; i < Palette32::SIZE; ++i) {
            auto c = Palette32::get(i);
            long d = color_distance(r, g, b, c.r, c.g, c.b);
            if (minDistance < 0 || d < minDistance) {
                minDistance = d;
                bestToken = i;
            }
        }

        return bestToken;
    }

    // Downsamples raw RGBA image buffer to target Matrix using configured mode
    static Matrix process_image(const uint8_t* rgba, int inW, int inH, const QuantizerOptions& opt) {
        Matrix out;
        if (!rgba || inW <= 0 || inH <= 0 || opt.targetWidth <= 0 || opt.targetHeight <= 0) {
            return out;
        }

        out.width = static_cast<size_t>(opt.targetWidth);
        out.height = static_cast<size_t>(opt.targetHeight);
        out.data.resize(out.width * out.height, 0);

        for (int ty = 0; ty < opt.targetHeight; ++ty) {
            int y0 = ty * inH / opt.targetHeight;
            int y1 = (ty + 1) * inH / opt.targetHeight;
            if (y1 <= y0) y1 = y0 + 1;

            for (int tx = 0; tx < opt.targetWidth; ++tx) {
                int x0 = tx * inW / opt.targetWidth;
                int x1 = (tx + 1) * inW / opt.targetWidth;
                if (x1 <= x0) x1 = x0 + 1;

                uint8_t token = 0;

                if (opt.mode == DownsampleMode::Center) {
                    int cx = (x0 + x1) / 2;
                    int cy = (y0 + y1) / 2;
                    size_t idx = (cy * inW + cx) * 4;
                    token = map_to_db32(rgba[idx + 0], rgba[idx + 1], rgba[idx + 2], rgba[idx + 3], opt);

                } else if (opt.mode == DownsampleMode::Average) {
                    long sumR = 0, sumG = 0, sumB = 0;
                    int nonTransCount = 0;
                    int totalCount = 0;

                    for (int y = y0; y < y1; ++y) {
                        for (int x = x0; x < x1; ++x) {
                            size_t idx = (y * inW + x) * 4;
                            totalCount++;
                            if (rgba[idx + 3] >= opt.alphaThreshold) {
                                if (!(opt.keyWhite && rgba[idx + 0] >= 240 && rgba[idx + 1] >= 240 && rgba[idx + 2] >= 240)) {
                                    sumR += rgba[idx + 0];
                                    sumG += rgba[idx + 1];
                                    sumB += rgba[idx + 2];
                                    nonTransCount++;
                                }
                            }
                        }
                    }

                    if (nonTransCount * 2 >= totalCount && nonTransCount > 0) {
                        uint8_t avgR = static_cast<uint8_t>(sumR / nonTransCount);
                        uint8_t avgG = static_cast<uint8_t>(sumG / nonTransCount);
                        uint8_t avgB = static_cast<uint8_t>(sumB / nonTransCount);
                        token = map_to_db32(avgR, avgG, avgB, 255, opt);
                    } else {
                        token = 0;
                    }

                } else { // DownsampleMode::Modal (Default)
                    std::array<int, 32> counts{};
                    int totalSampled = 0;

                    // Sample block (step by 1 if block is small, step by 2 if large)
                    int stepX = std::max(1, (x1 - x0) / 8);
                    int stepY = std::max(1, (y1 - y0) / 8);

                    for (int y = y0; y < y1; y += stepY) {
                        for (int x = x0; x < x1; x += stepX) {
                            size_t idx = (y * inW + x) * 4;
                            uint8_t t = map_to_db32(rgba[idx + 0], rgba[idx + 1], rgba[idx + 2], rgba[idx + 3], opt);
                            counts[t]++;
                            totalSampled++;
                        }
                    }

                    // If majority is transparent (> 50%), keep transparent
                    if (counts[0] * 2 >= totalSampled) {
                        token = 0;
                    } else {
                        // Find most frequent non-transparent color
                        int maxFreq = -1;
                        uint8_t modalToken = 1;
                        for (uint8_t i = 1; i < 32; ++i) {
                            if (counts[i] > maxFreq) {
                                maxFreq = counts[i];
                                modalToken = i;
                            }
                        }
                        token = modalToken;
                    }
                }

                out.data[ty * out.width + tx] = token;
            }
        }

        // Post-processing filter 1: Despeckle (Remove single-pixel orphan noise)
        if (opt.despeckle && out.width > 2 && out.height > 2) {
            apply_despeckle(out);
        }

        // Post-processing filter 2: Outline enhancement
        if (opt.outline) {
            apply_outline(out);
        }

        return out;
    }

    // Removes orphan 1x1 noise pixels whose orthogonal neighbors all agree on a different color
    static void apply_despeckle(Matrix& mat) {
        Matrix copy = mat;
        for (size_t y = 1; y < mat.height - 1; ++y) {
            for (size_t x = 1; x < mat.width - 1; ++x) {
                uint8_t cur = copy.at(x, y);
                uint8_t up    = copy.at(x, y - 1);
                uint8_t down  = copy.at(x, y + 1);
                uint8_t left  = copy.at(x - 1, y);
                uint8_t right = copy.at(x + 1, y);

                // If all 4 neighbors match each other and differ from current, replace
                if (up == down && up == left && up == right && cur != up) {
                    mat.data[y * mat.width + x] = up;
                }
            }
        }
    }

    // Applies classic black retro pixel art contour (token 1) around sprite silhouette
    static void apply_outline(Matrix& mat) {
        Matrix copy = mat;
        for (size_t y = 0; y < mat.height; ++y) {
            for (size_t x = 0; x < mat.width; ++x) {
                uint8_t cur = copy.at(x, y);
                if (cur == 0 || cur == 1) continue; // Already transparent or black

                // Check 4-way neighbors for transparency
                bool touchesTransparent = false;
                if (x == 0 || copy.at(x - 1, y) == 0) touchesTransparent = true;
                else if (x + 1 == mat.width || copy.at(x + 1, y) == 0) touchesTransparent = true;
                else if (y == 0 || copy.at(x, y - 1) == 0) touchesTransparent = true;
                else if (y + 1 == mat.height || copy.at(x, y + 1) == 0) touchesTransparent = true;

                if (touchesTransparent) {
                    mat.data[y * mat.width + x] = 1; // Contour token 1 (Black)
                }
            }
        }
    }
};

} // namespace spritex::tools
