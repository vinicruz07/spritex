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
    bool autoCrop{false};       // Automatically detect bounding box and fit character to canvas
    int  cropMargin{1};         // Aesthetic border margin (in tokens) when autoCrop is enabled
    bool celShade{false};       // Pre-pass: flatten gradients and textures using Kuwahara filter
    int  celShadeRadius{2};     // Kuwahara radius (2 = 5x5 window)
    bool posterize{false};      // Pre-pass: reduce color steps to create flat color planes
    float contrast{1.0f};       // Contrast factor (1.0 = normal, 1.25 = punchy)
    float saturation{1.0f};     // Saturation factor (1.0 = normal, 1.30 = vibrant for DB32)
    float brightness{0.0f};     // Brightness offset (-0.5 to +0.5)
    DownsampleMode mode{DownsampleMode::Modal};
};

class ColorQuantizer {
public:
    struct BoundingBox {
        int minX{0};
        int maxX{-1};
        int minY{0};
        int maxY{-1};
        bool valid{false};
    };

    // Detect non-background bounding box for auto-crop & fit
    static BoundingBox detect_bounding_box(const uint8_t* rgba, int inW, int inH, const QuantizerOptions& opt) {
        BoundingBox bbox;
        if (!rgba || inW <= 0 || inH <= 0) return bbox;

        uint8_t bgR = rgba[0], bgG = rgba[1], bgB = rgba[2], bgA = rgba[3];

        int minX = inW, maxX = -1;
        int minY = inH, maxY = -1;

        for (int y = 0; y < inH; ++y) {
            for (int x = 0; x < inW; ++x) {
                size_t idx = (y * inW + x) * 4;
                uint8_t r = rgba[idx + 0];
                uint8_t g = rgba[idx + 1];
                uint8_t b = rgba[idx + 2];
                uint8_t a = rgba[idx + 3];

                bool isBg = false;
                if (a < opt.alphaThreshold) {
                    isBg = true;
                } else if (opt.keyWhite && r >= 235 && g >= 235 && b >= 235) {
                    isBg = true;
                } else if (opt.keyMagenta && r >= 230 && g <= 35 && b >= 230) {
                    isBg = true;
                } else if (bgA >= opt.alphaThreshold) {
                    int dr = std::abs((int)r - (int)bgR);
                    int dg = std::abs((int)g - (int)bgG);
                    int db = std::abs((int)b - (int)bgB);
                    if (dr <= 25 && dg <= 25 && db <= 25) {
                        isBg = true;
                    }
                }

                if (!isBg) {
                    if (x < minX) minX = x;
                    if (x > maxX) maxX = x;
                    if (y < minY) minY = y;
                    if (y > maxY) maxY = y;
                }
            }
        }

        if (minX <= maxX && minY <= maxY) {
            bbox.minX = minX;
            bbox.maxX = maxX;
            bbox.minY = minY;
            bbox.maxY = maxY;
            bbox.valid = true;
        }
        return bbox;
    }

    // Color Grading: contrast & saturation boost before downsampling
    static void apply_color_grading(uint8_t* rgba, int w, int h, float contrast, float saturation, float brightness = 0.0f) {
        if (!rgba || w <= 0 || h <= 0) return;
        for (int i = 0; i < w * h; ++i) {
            uint8_t* p = &rgba[i * 4];
            if (p[3] == 0) continue;
            float r = p[0], g = p[1], b = p[2];

            if (brightness != 0.0f) {
                r += brightness * 255.0f;
                g += brightness * 255.0f;
                b += brightness * 255.0f;
            }

            if (std::abs(saturation - 1.0f) >= 0.01f) {
                float lum = 0.299f * r + 0.587f * g + 0.114f * b;
                r = lum + (r - lum) * saturation;
                g = lum + (g - lum) * saturation;
                b = lum + (b - lum) * saturation;
            }

            if (std::abs(contrast - 1.0f) >= 0.01f) {
                r = (r - 128.0f) * contrast + 128.0f;
                g = (g - 128.0f) * contrast + 128.0f;
                b = (b - 128.0f) * contrast + 128.0f;
            }

            p[0] = static_cast<uint8_t>(std::clamp(r, 0.0f, 255.0f));
            p[1] = static_cast<uint8_t>(std::clamp(g, 0.0f, 255.0f));
            p[2] = static_cast<uint8_t>(std::clamp(b, 0.0f, 255.0f));
        }
    }

    // Kuwahara filter: edge-preserving smoothing that flattens textures into cel-shaded bands
    static void apply_kuwahara(uint8_t* rgba, int w, int h, int r) {
        if (!rgba || w <= 0 || h <= 0 || r <= 0) return;
        std::vector<uint8_t> src(rgba, rgba + w * h * 4);

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                size_t pIdx = (y * w + x) * 4;
                if (src[pIdx + 3] == 0) continue;

                const int qX0[4] = {std::max(0, x - r), x, std::max(0, x - r), x};
                const int qX1[4] = {x, std::min(w - 1, x + r), x, std::min(w - 1, x + r)};
                const int qY0[4] = {std::max(0, y - r), std::max(0, y - r), y, y};
                const int qY1[4] = {y, y, std::min(h - 1, y + r), std::min(h - 1, y + r)};

                float minVar = 1e20f;
                int bestR = src[pIdx], bestG = src[pIdx + 1], bestB = src[pIdx + 2];

                for (int q = 0; q < 4; ++q) {
                    long sumR = 0, sumG = 0, sumB = 0;
                    long sumSqR = 0, sumSqG = 0, sumSqB = 0;
                    int count = 0;

                    for (int qy = qY0[q]; qy <= qY1[q]; ++qy) {
                        for (int qx = qX0[q]; qx <= qX1[q]; ++qx) {
                            size_t idx = (qy * w + qx) * 4;
                            if (src[idx + 3] == 0) continue;
                            int pr = src[idx + 0];
                            int pg = src[idx + 1];
                            int pb = src[idx + 2];
                            sumR += pr; sumG += pg; sumB += pb;
                            sumSqR += pr * pr; sumSqG += pg * pg; sumSqB += pb * pb;
                            count++;
                        }
                    }

                    if (count == 0) continue;
                    float meanR = (float)sumR / count;
                    float meanG = (float)sumG / count;
                    float meanB = (float)sumB / count;
                    float var = ((float)sumSqR / count - meanR * meanR) +
                                ((float)sumSqG / count - meanG * meanG) +
                                ((float)sumSqB / count - meanB * meanB);

                    if (var < minVar) {
                        minVar = var;
                        bestR = (int)meanR;
                        bestG = (int)meanG;
                        bestB = (int)meanB;
                    }
                }

                rgba[pIdx + 0] = (uint8_t)bestR;
                rgba[pIdx + 1] = (uint8_t)bestG;
                rgba[pIdx + 2] = (uint8_t)bestB;
            }
        }
    }

    // Posterize color channels to discrete steps
    static void apply_posterize(uint8_t* rgba, int w, int h, int levels = 8) {
        if (!rgba || w <= 0 || h <= 0 || levels <= 0) return;
        int step = 256 / levels;
        for (int i = 0; i < w * h; ++i) {
            uint8_t* p = &rgba[i * 4];
            if (p[3] == 0) continue;
            p[0] = std::clamp((p[0] / step) * step + step / 2, 0, 255);
            p[1] = std::clamp((p[1] / step) * step + step / 2, 0, 255);
            p[2] = std::clamp((p[2] / step) * step + step / 2, 0, 255);
        }
    }

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

    // Downsamples raw RGBA image buffer to target Matrix using configured options
    static Matrix process_image(const uint8_t* rgba, int inW, int inH, const QuantizerOptions& opt) {
        Matrix out;
        if (!rgba || inW <= 0 || inH <= 0 || opt.targetWidth <= 0 || opt.targetHeight <= 0) {
            return out;
        }

        out.width = static_cast<size_t>(opt.targetWidth);
        out.height = static_cast<size_t>(opt.targetHeight);
        out.data.resize(out.width * out.height, 0);

        // Pre-processing pipeline
        std::vector<uint8_t> localBuf;
        const uint8_t* activeRgba = rgba;

        bool needsPreprocess = (opt.celShade && opt.celShadeRadius > 0) ||
                               (std::abs(opt.contrast - 1.0f) >= 0.01f) ||
                               (std::abs(opt.saturation - 1.0f) >= 0.01f) ||
                               (std::abs(opt.brightness) >= 0.01f) ||
                               opt.posterize;

        if (needsPreprocess) {
            localBuf.assign(rgba, rgba + inW * inH * 4);
            if (std::abs(opt.contrast - 1.0f) >= 0.01f || std::abs(opt.saturation - 1.0f) >= 0.01f || std::abs(opt.brightness) >= 0.01f) {
                apply_color_grading(localBuf.data(), inW, inH, opt.contrast, opt.saturation, opt.brightness);
            }
            if (opt.celShade && opt.celShadeRadius > 0) {
                apply_kuwahara(localBuf.data(), inW, inH, opt.celShadeRadius);
            }
            if (opt.posterize) {
                apply_posterize(localBuf.data(), inW, inH, 8);
            }
            activeRgba = localBuf.data();
        }

        BoundingBox bbox;
        double step = 0.0;
        double centerX = 0.0, centerY = 0.0;
        bool useCrop = false;

        if (opt.autoCrop) {
            bbox = detect_bounding_box(activeRgba, inW, inH, opt);
            if (bbox.valid) {
                int bboxW = bbox.maxX - bbox.minX + 1;
                int bboxH = bbox.maxY - bbox.minY + 1;
                int availW = std::max(1, opt.targetWidth - 2 * opt.cropMargin);
                int availH = std::max(1, opt.targetHeight - 2 * opt.cropMargin);

                step = std::max((double)bboxW / availW, (double)bboxH / availH);
                centerX = (bbox.minX + bbox.maxX) / 2.0;
                centerY = (bbox.minY + bbox.maxY) / 2.0;
                useCrop = true;
            }
        }

        for (int ty = 0; ty < opt.targetHeight; ++ty) {
            int y0, y1;
            if (useCrop) {
                double sy = centerY + (ty - (opt.targetHeight - 1) / 2.0) * step;
                y0 = (int)std::round(sy - step / 2.0);
                y1 = (int)std::round(sy + step / 2.0);
                if (y1 <= y0) y1 = y0 + 1;
            } else {
                y0 = ty * inH / opt.targetHeight;
                y1 = (ty + 1) * inH / opt.targetHeight;
                if (y1 <= y0) y1 = y0 + 1;
            }

            for (int tx = 0; tx < opt.targetWidth; ++tx) {
                int x0, x1;
                if (useCrop) {
                    double sx = centerX + (tx - (opt.targetWidth - 1) / 2.0) * step;
                    x0 = (int)std::round(sx - step / 2.0);
                    x1 = (int)std::round(sx + step / 2.0);
                    if (x1 <= x0) x1 = x0 + 1;

                    if (x1 <= 0 || x0 >= inW || y1 <= 0 || y0 >= inH) {
                        out.data[ty * out.width + tx] = 0;
                        continue;
                    }
                    x0 = std::clamp(x0, 0, inW - 1);
                    x1 = std::clamp(x1, x0 + 1, inW);
                    y0 = std::clamp(y0, 0, inH - 1);
                    y1 = std::clamp(y1, y0 + 1, inH);
                } else {
                    x0 = tx * inW / opt.targetWidth;
                    x1 = (tx + 1) * inW / opt.targetWidth;
                    if (x1 <= x0) x1 = x0 + 1;
                }

                uint8_t token = 0;

                if (opt.mode == DownsampleMode::Center) {
                    int cx = (x0 + x1) / 2;
                    int cy = (y0 + y1) / 2;
                    size_t idx = (cy * inW + cx) * 4;
                    token = map_to_db32(activeRgba[idx + 0], activeRgba[idx + 1], activeRgba[idx + 2], activeRgba[idx + 3], opt);

                } else if (opt.mode == DownsampleMode::Average) {
                    long sumR = 0, sumG = 0, sumB = 0;
                    int nonTransCount = 0;
                    int totalCount = 0;

                    for (int y = y0; y < y1; ++y) {
                        for (int x = x0; x < x1; ++x) {
                            size_t idx = (y * inW + x) * 4;
                            totalCount++;
                            if (activeRgba[idx + 3] >= opt.alphaThreshold) {
                                if (!(opt.keyWhite && activeRgba[idx + 0] >= 240 && activeRgba[idx + 1] >= 240 && activeRgba[idx + 2] >= 240)) {
                                    sumR += activeRgba[idx + 0];
                                    sumG += activeRgba[idx + 1];
                                    sumB += activeRgba[idx + 2];
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

                    int stepX = std::max(1, (x1 - x0) / 8);
                    int stepY = std::max(1, (y1 - y0) / 8);

                    for (int y = y0; y < y1; y += stepY) {
                        for (int x = x0; x < x1; x += stepX) {
                            size_t idx = (y * inW + x) * 4;
                            uint8_t t = map_to_db32(activeRgba[idx + 0], activeRgba[idx + 1], activeRgba[idx + 2], activeRgba[idx + 3], opt);
                            counts[t]++;
                            totalSampled++;
                        }
                    }

                    if (counts[0] * 2 >= totalSampled) {
                        token = 0;
                    } else {
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

        // Post-processing filter 1: Smart Despeckle (Remove single-pixel orphan noise)
        if (opt.despeckle && out.width > 2 && out.height > 2) {
            apply_despeckle(out, 2);
        }

        // Post-processing filter 2: Outline enhancement
        if (opt.outline) {
            apply_outline(out, 1);
        }

        return out;
    }

    // Advanced Despeckle: removes orphan 1x1 noise pixels without neighbor connections
    static bool apply_despeckle(Matrix& mat, int passes = 2) {
        if (mat.width < 3 || mat.height < 3) return false;
        bool changedAny = false;
        for (int p = 0; p < passes; ++p) {
            Matrix copy = mat;
            bool passChanged = false;
            for (size_t y = 0; y < mat.height; ++y) {
                for (size_t x = 0; x < mat.width; ++x) {
                    uint8_t cur = copy.at(x, y);
                    std::array<int, 32> neighborCounts{};
                    int validNeighbors = 0;
                    int sameCount = 0;

                    for (int dy = -1; dy <= 1; ++dy) {
                        for (int dx = -1; dx <= 1; ++dx) {
                            if (dx == 0 && dy == 0) continue;
                            int nx = (int)x + dx;
                            int ny = (int)y + dy;
                            if (nx >= 0 && nx < (int)mat.width && ny >= 0 && ny < (int)mat.height) {
                                uint8_t nToken = copy.at(nx, ny);
                                neighborCounts[nToken]++;
                                validNeighbors++;
                                if (nToken == cur) sameCount++;
                            }
                        }
                    }

                    if (sameCount <= (p == 0 ? 0 : 1) && validNeighbors > 0) {
                        int maxCount = -1;
                        uint8_t bestNeighbor = cur;
                        for (uint8_t t = 0; t < 32; ++t) {
                            if (neighborCounts[t] > maxCount) {
                                maxCount = neighborCounts[t];
                                bestNeighbor = t;
                            }
                        }
                        if (maxCount >= 3 && bestNeighbor != cur) {
                            mat.data[y * mat.width + x] = bestNeighbor;
                            passChanged = true;
                            changedAny = true;
                        }
                    }
                }
            }
            if (!passChanged) break;
        }
        return changedAny;
    }

    // Applies classic black retro pixel art contour (token 1) around sprite silhouette
    static bool apply_outline(Matrix& mat, uint8_t outlineToken = 1) {
        Matrix copy = mat;
        bool changed = false;
        for (size_t y = 0; y < mat.height; ++y) {
            for (size_t x = 0; x < mat.width; ++x) {
                uint8_t cur = copy.at(x, y);
                if (cur == 0 || cur == outlineToken) continue;

                bool touchesTransparent = false;
                const int dx[] = {0, 0, -1, 1};
                const int dy[] = {-1, 1, 0, 0};
                for (int i = 0; i < 4; ++i) {
                    int nx = (int)x + dx[i];
                    int ny = (int)y + dy[i];
                    if (nx < 0 || nx >= (int)mat.width || ny < 0 || ny >= (int)mat.height || copy.at(nx, ny) == 0) {
                        touchesTransparent = true;
                        break;
                    }
                }

                if (touchesTransparent) {
                    mat.data[y * mat.width + x] = outlineToken;
                    changed = true;
                }
            }
        }
        return changed;
    }
};

} // namespace spritex::tools
