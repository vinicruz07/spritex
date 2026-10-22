#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <algorithm>
#include "palette.hpp"
#include "rasterizer.hpp"
#include "../datasets/dataset.hpp"

namespace spritex::prototype {

class MaskGITSimulator {
public:
    static void generate_from_sample(const SpriteSample& sample, const std::string& output_path) {
        std::cout << "\n=======================================================\n";
        std::cout << " [Spritex Prototype] Interpretando Prompt: \"" << sample.prompt << "\"\n";
        std::cout << " Categoria Detectada: " << sample.category << "\n";
        std::cout << " Paleta Selecionada:  " << sample.default_palette << "\n";
        std::cout << "=======================================================\n";

        // Obter paleta apropriada
        Palette palette = Palette::make_iron();
        if (sample.default_palette == "wood") palette = Palette::make_wood();
        else if (sample.default_palette == "fire") palette = Palette::make_fire();
        else if (sample.default_palette == "blonde") palette = Palette::make_blonde();

        const auto& target = sample.matrix;
        int h = static_cast<int>(target.size());
        int w = static_cast<int>(target[0].size());

        // Grade inicial 100% MASCARADA (-1 = [MASK])
        std::vector<std::vector<int>> canvas(h, std::vector<int>(w, -1));

        std::cout << "[*] Iniciando Decodificacao Paralela MaskGIT (Simulacao de 3 Passos)...\n\n";

        // Passo 1: Desmascarar Fundo (0) e Contorno Externo (1)
        std::cout << "--- [Passo 1/3]: Reconhecimento de Silhueta e Fundo ---\n";
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                if (target[y][x] == 0 || target[y][x] == 1) {
                    canvas[y][x] = target[y][x];
                }
            }
        }
        print_grid(canvas);

        // Passo 2: Desmascarar Corpo, Massa Principal e Pele (tokens intermediários)
        std::cout << "--- [Passo 2/3]: Preenchimento de Volume e Massas Centrais ---\n";
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                if (target[y][x] >= 2 && target[y][x] <= 8) {
                    canvas[y][x] = target[y][x];
                }
            }
        }
        print_grid(canvas);

        // Passo 3: Desmascarar Detalhes Finos, Luzes Altas e Efeitos FX
        std::cout << "--- [Passo 3/3]: Sintese de Highlights, Reflexos e Efeitos FX ---\n";
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                canvas[y][x] = target[y][x];
            }
        }
        print_grid(canvas);

        // Renderizar para PNG com escala 16x (256x256 pixels)
        Rasterizer::save_png(output_path, canvas, palette, 16);
    }

private:
    static void print_grid(const std::vector<std::vector<int>>& grid) {
        for (const auto& row : grid) {
            std::cout << "  ";
            for (int val : row) {
                if (val == -1) std::cout << "::"; // Máscara não revelada (compatível com todos os consoles)
                else if (val == 0) std::cout << ".."; // Vazio
                else if (val == 1) std::cout << "##"; // Contorno
                else if (val < 10) std::cout << " " << val;
                else std::cout << val;
            }
            std::cout << "\n";
        }
        std::cout << "\n";
    }
};

} // namespace spritex::prototype
