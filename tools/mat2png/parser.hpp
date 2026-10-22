#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <iostream>

namespace spritex::tools {

struct Matrix {
    size_t width{0};
    size_t height{0};
    std::vector<uint8_t> data;

    uint8_t at(size_t x, size_t y) const {
        return data[y * width + x];
    }

    bool empty() const {
        return data.empty() || width == 0 || height == 0;
    }
};

struct ParseResult {
    bool success{false};
    bool wasRepaired{false};
    std::string error;
    std::string warning;
    Matrix matrix;
};

class MatrixParser {
public:
    static ParseResult parse(const std::string& input, bool autoRepair = true) {
        ParseResult result;

        if (input.empty()) {
            result.error = "Input string is empty.";
            return result;
        }

        // Check if input contains brackets '['
        bool hasBrackets = (input.find('[') != std::string::npos);

        std::vector<std::vector<uint8_t>> rows;

        if (hasBrackets) {
            std::string err;
            if (!parse_bracketed(input, rows, err)) {
                result.error = err;
                return result;
            }
        } else {
            std::string err;
            if (!parse_plain(input, rows, err)) {
                result.error = err;
                return result;
            }
        }

        if (rows.empty()) {
            result.error = "No valid matrix rows found in input.";
            return result;
        }

        // Determine target width by modal (most common) row size
        std::vector<size_t> rowSizes;
        rowSizes.reserve(rows.size());
        size_t maxLen = 0;
        for (const auto& r : rows) {
            rowSizes.push_back(r.size());
            if (r.size() > maxLen) maxLen = r.size();
        }

        // Count frequency of each row size
        std::vector<size_t> freq(maxLen + 1, 0);
        for (size_t s : rowSizes) {
            freq[s]++;
        }

        size_t targetWidth = 0;
        size_t maxFreq = 0;
        for (size_t s = 1; s <= maxLen; ++s) {
            if (freq[s] > maxFreq) {
                maxFreq = freq[s];
                targetWidth = s;
            }
        }

        if (targetWidth == 0) {
            result.error = "First row is empty or invalid.";
            return result;
        }

        size_t mismatchCount = 0;
        int firstMismatchRow = -1;
        size_t firstMismatchSize = 0;

        for (size_t r = 0; r < rows.size(); ++r) {
            if (rows[r].size() != targetWidth) {
                if (firstMismatchRow == -1) {
                    firstMismatchRow = static_cast<int>(r);
                    firstMismatchSize = rows[r].size();
                }
                mismatchCount++;

                if (autoRepair) {
                    if (rows[r].size() < targetWidth) {
                        rows[r].resize(targetWidth, 0); // Pad missing tokens with 0 (transparent)
                    } else {
                        rows[r].resize(targetWidth);    // Truncate excess tokens
                    }
                }
            }
        }

        if (mismatchCount > 0) {
            if (!autoRepair) {
                result.error = "Matrix row dimension mismatch: expected " +
                               std::to_string(targetWidth) +
                               " elements, but row " + std::to_string(firstMismatchRow) +
                               " has " + std::to_string(firstMismatchSize) + " elements.";
                return result;
            } else {
                result.wasRepaired = true;
                if (mismatchCount == 1) {
                    result.warning = "Linha " + std::to_string(firstMismatchRow) +
                                     " tinha " + std::to_string(firstMismatchSize) +
                                     " tokens (auto-ajustada para " + std::to_string(targetWidth) + ")";
                } else {
                    result.warning = std::to_string(mismatchCount) +
                                     " linhas auto-ajustadas para " + std::to_string(targetWidth) +
                                     " tokens (ex: linha " + std::to_string(firstMismatchRow) + ")";
                }
            }
        }

        result.matrix.width = targetWidth;
        result.matrix.height = rows.size();
        result.matrix.data.reserve(result.matrix.width * result.matrix.height);
        for (const auto& row : rows) {
            result.matrix.data.insert(result.matrix.data.end(), row.begin(), row.end());
        }

        result.success = true;
        return result;
    }

    static std::string format_matrix(const Matrix& m) {
        if (m.empty()) return "";
        std::stringstream ss;
        for (size_t y = 0; y < m.height; ++y) {
            ss << "[";
            for (size_t x = 0; x < m.width; ++x) {
                if (x > 0) ss << ",";
                uint8_t val = m.at(x, y);
                if (val < 10) ss << "0";
                ss << static_cast<int>(val);
            }
            ss << "]\r\n";
        }
        return ss.str();
    }

private:
    static bool parse_bracketed(const std::string& input, std::vector<std::vector<uint8_t>>& rows, std::string& error) {
        size_t i = 0;
        size_t len = input.length();

        while (i < len) {
            // Find next '['
            while (i < len && input[i] != '[') {
                i++;
            }
            if (i >= len) break;
            i++; // Skip '['

            // Skip whitespace
            while (i < len && std::isspace(static_cast<unsigned char>(input[i]))) {
                i++;
            }
            if (i >= len) break;

            // If the next character is another '[', this was an outer container wrapper like `[[...`
            if (input[i] == '[') {
                // Loop continues to process inner '['
                continue;
            }

            // Otherwise, we are inside a row! Read tokens until ']'
            std::vector<uint8_t> currentRow;
            while (i < len && input[i] != ']') {
                // Skip commas and whitespace
                while (i < len && (std::isspace(static_cast<unsigned char>(input[i])) || input[i] == ',')) {
                    i++;
                }
                if (i >= len || input[i] == ']') break;

                // Read number
                if (std::isdigit(static_cast<unsigned char>(input[i]))) {
                    size_t start = i;
                    while (i < len && std::isdigit(static_cast<unsigned char>(input[i]))) {
                        i++;
                    }
                    std::string numStr = input.substr(start, i - start);
                    try {
                        int val = std::stoi(numStr, nullptr, 10);
                        if (val < 0 || val > 255) {
                            error = "Token value out of range (0-255): " + numStr;
                            return false;
                        }
                        currentRow.push_back(static_cast<uint8_t>(val));
                    } catch (const std::exception& e) {
                        error = "Failed to parse number: " + numStr;
                        return false;
                    }
                } else if (input[i] == '#') {
                    // Line comment until newline
                    while (i < len && input[i] != '\n') i++;
                } else {
                    // Skip other characters (spaces, trailing commas, formatting)
                    i++;
                }
            }

            if (i < len && input[i] == ']') {
                i++; // Skip ']'
            }

            if (!currentRow.empty()) {
                rows.push_back(std::move(currentRow));
            }
        }

        return true;
    }

    static bool parse_plain(const std::string& input, std::vector<std::vector<uint8_t>>& rows, std::string& error) {
        std::istringstream stream(input);
        std::string line;
        size_t lineNum = 0;

        while (std::getline(stream, line)) {
            lineNum++;
            size_t start = 0;
            while (start < line.size() && std::isspace(static_cast<unsigned char>(line[start]))) start++;
            if (start >= line.size()) continue;

            if (line[start] == '#' || (start + 1 < line.size() && line[start] == '/' && line[start + 1] == '/')) {
                continue; // comment
            }

            std::vector<uint8_t> currentRow;
            size_t i = start;
            while (i < line.size()) {
                while (i < line.size() && (std::isspace(static_cast<unsigned char>(line[i])) || line[i] == ',')) {
                    i++;
                }
                if (i >= line.size()) break;

                if (std::isdigit(static_cast<unsigned char>(line[i]))) {
                    size_t numStart = i;
                    while (i < line.size() && std::isdigit(static_cast<unsigned char>(line[i]))) {
                        i++;
                    }
                    std::string numStr = line.substr(numStart, i - numStart);
                    try {
                        int val = std::stoi(numStr, nullptr, 10);
                        if (val < 0 || val > 255) {
                            error = "Token value out of range (0-255) on line " + std::to_string(lineNum) + ": " + numStr;
                            return false;
                        }
                        currentRow.push_back(static_cast<uint8_t>(val));
                    } catch (const std::exception& e) {
                        error = "Failed to parse number on line " + std::to_string(lineNum) + ": " + numStr;
                        return false;
                    }
                } else {
                    i++;
                }
            }

            if (!currentRow.empty()) {
                rows.push_back(std::move(currentRow));
            }
        }

        return true;
    }
};

} // namespace spritex::tools
