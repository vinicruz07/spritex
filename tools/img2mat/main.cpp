#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "converter.hpp"
#include "../mat2png/palette.hpp"
#include "../mat2png/rasterizer.hpp"

#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include <filesystem>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace {

void setup_console() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
}

void print_banner() {
    std::cout << "======================================================\n";
    std::cout << "   spritex img2mat - Image to Pixel Art Quantizer     \n";
    std::cout << "======================================================\n";
}

void print_help(const char* progName) {
    print_banner();
    std::cout << "Uso: " << progName << " -i <imagem> [opcoes]\n\n"
              << "Opcoes Obrigatorias:\n"
              << "  -i, --input <caminho>     Imagem normal (PNG, JPG, BMP, etc.)\n\n"
              << "Opcoes de Dimensoes e Amostragem:\n"
              << "  -w, --width <int>         Largura da matriz gerada (padrao: 32)\n"
              << "  -h, --height <int>        Altura da matriz gerada (padrao: 32)\n"
              << "  --mode <modal|center|avg> Metodo de pixelizacao (padrao: modal)\n"
              << "                            modal: cor mais frequente (ideal p/ IA/Midjourney)\n"
              << "                            center: pixel central (ideal p/ upscaled art)\n"
              << "                            avg: media RGB antes da quantizacao\n\n"
              << "Opcoes de Tratamento de Cor & Fundo:\n"
              << "  --key-white               Trata fundo branco (#F0F0F0+) como transparente\n"
              << "  --key-magenta             Trata magenta (#FF00FF) como transparente\n"
              << "  --alpha <0-255>           Limiar de corte alfa (padrao: 128)\n"
              << "  --outline                 Aplica contorno preto retro (token 1) nas bordas\n"
              << "  --no-clean                Desativa filtro de remocao de ruido isolado\n\n"
              << "Opcoes de Saida:\n"
              << "  -o, --output <arquivo>    Salva matriz em arquivo .txt (padrao: <input>_mat.txt)\n"
              << "  -p, --png <arquivo.png>   Gera imagem PNG rasterizada de preview\n"
              << "  -s, --scale <int>         Escala da imagem PNG gerada (padrao: 16)\n"
              << "  --stdout                  Imprime a matriz gerada no terminal (stdout)\n"
              << "  --help                    Exibe este menu de ajuda\n\n"
              << "Exemplos:\n"
              << "  " << progName << " -i arte_midjourney.png -w 32 -h 32 --key-white -p out.png\n"
              << "  " << progName << " -i foto.jpg -o matriz.txt --outline\n";
}

} // namespace

int main(int argc, char* argv[]) {
    setup_console();

    if (argc < 2) {
        print_help(argv[0]);
        return 1;
    }

    std::string inputFile;
    std::string outputFile;
    std::string outputPng;
    int targetW = 32;
    int targetH = 32;
    int scale = 16;
    bool printStdout = false;

    spritex::tools::QuantizerOptions opt;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help") {
            print_help(argv[0]);
            return 0;
        } else if (arg == "-i" || arg == "--input") {
            if (i + 1 < argc) inputFile = argv[++i];
        } else if (arg == "-o" || arg == "--output") {
            if (i + 1 < argc) outputFile = argv[++i];
        } else if (arg == "-p" || arg == "--png") {
            if (i + 1 < argc) outputPng = argv[++i];
        } else if (arg == "-w" || arg == "--width") {
            if (i + 1 < argc) targetW = std::stoi(argv[++i]);
        } else if (arg == "-h" || arg == "--height") {
            if (i + 1 < argc) targetH = std::stoi(argv[++i]);
        } else if (arg == "-s" || arg == "--scale") {
            if (i + 1 < argc) scale = std::stoi(argv[++i]);
        } else if (arg == "--key-white") {
            opt.keyWhite = true;
        } else if (arg == "--key-magenta") {
            opt.keyMagenta = true;
        } else if (arg == "--crop" || arg == "--auto-crop") {
            opt.autoCrop = true;
        } else if (arg == "--outline") {
            opt.outline = true;
        } else if (arg == "--no-clean") {
            opt.despeckle = false;
        } else if (arg == "--stdout") {
            printStdout = true;
        } else if (arg == "--alpha") {
            if (i + 1 < argc) opt.alphaThreshold = static_cast<uint8_t>(std::stoi(argv[++i]));
        } else if (arg == "--mode") {
            if (i + 1 < argc) {
                std::string m = argv[++i];
                if (m == "center") opt.mode = spritex::tools::DownsampleMode::Center;
                else if (m == "avg" || m == "average") opt.mode = spritex::tools::DownsampleMode::Average;
                else opt.mode = spritex::tools::DownsampleMode::Modal;
            }
        } else {
            // Positional input file
            if (inputFile.empty()) inputFile = arg;
        }
    }

    if (inputFile.empty()) {
        std::cerr << "[ERRO] Nenhuma imagem de entrada fornecida (-i).\n";
        return 1;
    }

    opt.targetWidth = targetW > 0 ? targetW : 32;
    opt.targetHeight = targetH > 0 ? targetH : 32;

    // Default output path if not specified
    if (outputFile.empty()) {
        std::filesystem::path p(inputFile);
        outputFile = (p.parent_path() / (p.stem().string() + "_matrix.txt")).string();
    }

    auto startT = std::chrono::high_resolution_clock::now();

    // Run conversion
    auto res = spritex::tools::ImageConverter::from_file(inputFile, opt);

    auto endT = std::chrono::high_resolution_clock::now();
    double elapsedMs = std::chrono::duration<double, std::milli>(endT - startT).count();

    if (!res.success) {
        std::cerr << "[ERRO] Falha na conversao: " << res.error << "\n";
        return 1;
    }

    // Save text matrix file
    std::string writeErr;
    if (!spritex::tools::ImageConverter::save_matrix_file(res.matrix, outputFile, writeErr)) {
        std::cerr << "[ERRO] " << writeErr << "\n";
        return 1;
    }

    // Save preview PNG if requested
    bool pngSaved = false;
    if (!outputPng.empty()) {
        spritex::tools::Rasterizer::Options rastOpt;
        rastOpt.scale = scale > 0 ? scale : 16;
        auto rastRes = spritex::tools::Rasterizer::to_png(res.matrix, outputPng, rastOpt);
        pngSaved = rastRes.success;
    }

    if (!printStdout) {
        print_banner();
        std::cout << "[+] Imagem de Entrada:   " << inputFile << " ("
                  << res.originalWidth << "x" << res.originalHeight << " px)\n";
        std::cout << "[+] Resolucao Alvo:      " << opt.targetWidth << "x" << opt.targetHeight << " tokens\n";
        std::cout << "[+] Paleta de Destino:   DawnBringer 32 (DB32)\n";
        std::cout << "[+] Cores Unicas:        " << res.uniqueColors << " / 32\n";
        std::cout << "[+] Metodo Utilizado:    "
                  << (opt.mode == spritex::tools::DownsampleMode::Modal ? "Modal (Dominancia em Bloco)" :
                     (opt.mode == spritex::tools::DownsampleMode::Center ? "Center Pixel" : "Media RGB")) << "\n";
        std::cout << "[+] Matriz Salva:        " << outputFile << "\n";
        if (pngSaved) {
            std::cout << "[+] Preview PNG:         " << outputPng
                      << " (" << (opt.targetWidth * scale) << "x" << (opt.targetHeight * scale) << " px)\n";
        }
        std::cout << "[+] Tempo de Execucao:   " << std::fixed << std::setprecision(2) << elapsedMs << " ms\n";
        std::cout << "======================================================\n";
        std::cout << "[OK] Pixelizacao e mapeamento DB32 concluidos com sucesso!\n";
    } else {
        std::cout << spritex::tools::MatrixParser::format_matrix(res.matrix);
    }

    return 0;
}
