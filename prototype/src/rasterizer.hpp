#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <filesystem>
#include "palette.hpp"

// Include stb_image_write
#include "../third_party/stb_image_write.h"

namespace spritex::prototype {

class Rasterizer {
public:
    // Exporta uma matriz 16x16 para PNG com nearest-neighbor scaling
    static bool save_png(const std::string& filename,
                         const std::vector<std::vector<int>>& grid,
                         const Palette& palette,
                         int scale = 16) 
    {
        int src_h = static_cast<int>(grid.size());
        if (src_h == 0) return false;
        int src_w = static_cast<int>(grid[0].size());

        int out_w = src_w * scale;
        int out_h = src_h * scale;

        // Buffer RGBA (4 bytes por pixel)
        std::vector<uint8_t> buffer(out_w * out_h * 4, 0);

        // Nearest-neighbor upscale
        for (int y = 0; y < out_h; ++y) {
            int src_y = y / scale;
            for (int x = 0; x < out_w; ++x) {
                int src_x = x / scale;

                int token = grid[src_y][src_x];
                Color c = palette.get(token);

                int idx = (y * out_w + x) * 4;
                buffer[idx + 0] = c.r;
                buffer[idx + 1] = c.g;
                buffer[idx + 2] = c.b;
                buffer[idx + 3] = c.a;
            }
        }

        // Resolver caminho de forma robusta independente do CWD
        namespace fs = std::filesystem;
        fs::path out_path(filename);

        // Se o usuario estiver rodando de dentro de prototype/
        if (!fs::exists("prototype") && filename.rfind("prototype/", 0) == 0) {
            out_path = filename.substr(10); // "output/nome.png"
        }

        // Criar diretorios automaticamente se nao existirem
        if (out_path.has_parent_path()) {
            std::error_code ec;
            fs::create_directories(out_path.parent_path(), ec);
        }

        std::string final_path = out_path.string();
        int result = stbi_write_png(final_path.c_str(), out_w, out_h, 4, buffer.data(), out_w * 4);
        if (result != 0) {
            std::cout << "  [PNG Gerado com Sucesso] -> " << final_path 
                      << " (" << out_w << "x" << out_h << "px, escala " << scale << "x)\n";
            return true;
        } else {
            std::cerr << "  [ERRO] Falha ao gravar arquivo PNG: " << final_path << "\n";
            return false;
        }
    }
};

} // namespace spritex::prototype
