#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "rasterizer.hpp"
#include "parser.hpp"
#include "palette.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <iomanip>

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
    std::cout << "   spritex mat2png - Matrix to Pixel Art Rasterizer   \n";
    std::cout << "======================================================\n";
}

void print_help(const char* progName) {
    print_banner();
    std::cout << "Uso: " << progName << " [opcoes]\n\n"
              << "Opcoes:\n"
              << "  -i, --input <arquivo>     Arquivo de matriz (texto/JSON) ou '-' para stdin\n"
              << "  -o, --output <arquivo>    Caminho do arquivo PNG gerado (padrao: output.png)\n"
              << "  -s, --scale <fator>       Fator de ampliacao nearest-neighbor (padrao: 16)\n"
              << "  -p, --palette             Exibe a tabela de 32 cores (DB32) e finaliza\n"
              << "      --info                Exibe dimensoes e estatisticas da matriz\n"
              << "  -h, --help                Exibe este menu de ajuda\n\n"
              << "Exemplos:\n"
              << "  " << progName << " -i cavaleiro_32x32.txt -o cavaleiro.png -s 16\n"
              << "  " << progName << " -i sprite.txt --info\n"
              << "  cat matriz.txt | " << progName << " -o out.png\n";
}

void print_palette() {
    print_banner();
    std::cout << "Paleta Master: DawnBringer 32 (DB32)\n";
    std::cout << "------------------------------------------------------\n";
    std::cout << std::left << std::setw(6)  << "Idx"
              << std::left << std::setw(22) << "Nome"
              << std::left << std::setw(18) << "RGBA"
              << "Hex\n";
    std::cout << "------------------------------------------------------\n";

    for (uint8_t i = 0; i < spritex::tools::Palette32::SIZE; ++i) {
        auto c = spritex::tools::Palette32::get(i);
        std::string rgbaStr = "(" + std::to_string(c.r) + "," +
                              std::to_string(c.g) + "," +
                              std::to_string(c.b) + "," +
                              std::to_string(c.a) + ")";

        std::stringstream hexStream;
        if (c.a == 0) {
            hexStream << "(transparente)";
        } else {
            hexStream << "#"
                      << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)c.r
                      << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)c.g
                      << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << (int)c.b;
        }

        std::cout << std::left << std::setw(6)  << (int)i
                  << std::left << std::setw(22) << spritex::tools::Palette32::get_name(i)
                  << std::left << std::setw(18) << rgbaStr
                  << hexStream.str() << "\n";
    }
    std::cout << "------------------------------------------------------\n";
}

} // namespace

int main(int argc, char* argv[]) {
    setup_console();

    std::string inputFile;
    std::string outputFile = "output.png";
    int scale = 16;
    bool showInfoOnly = false;

    // Parse command-line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            print_help(argv[0]);
            return 0;
        } else if (arg == "-p" || arg == "--palette") {
            print_palette();
            return 0;
        } else if (arg == "--info") {
            showInfoOnly = true;
        } else if (arg == "-i" || arg == "--input") {
            if (i + 1 < argc) {
                inputFile = argv[++i];
            } else {
                std::cerr << "[ERRO] Argumento -i/--input requer um caminho de arquivo.\n";
                return 1;
            }
        } else if (arg == "-o" || arg == "--output") {
            if (i + 1 < argc) {
                outputFile = argv[++i];
            } else {
                std::cerr << "[ERRO] Argumento -o/--output requer um caminho de saida.\n";
                return 1;
            }
        } else if (arg == "-s" || arg == "--scale") {
            if (i + 1 < argc) {
                try {
                    scale = std::stoi(argv[++i]);
                    if (scale <= 0) scale = 1;
                } catch (...) {
                    std::cerr << "[ERRO] Fator de escala invalido: " << argv[i] << "\n";
                    return 1;
                }
            } else {
                std::cerr << "[ERRO] Argumento -s/--scale requer um numero inteiro.\n";
                return 1;
            }
        } else {
            // Positional input file if not yet specified
            if (inputFile.empty()) {
                inputFile = arg;
            } else {
                std::cerr << "[ERRO] Parametro desconhecido: " << arg << "\n";
                return 1;
            }
        }
    }

    // Read input content
    std::string matrixContent;
    if (inputFile.empty() || inputFile == "-") {
        std::string line;
        while (std::getline(std::cin, line)) {
            matrixContent += line + "\n";
        }
        if (matrixContent.empty()) {
            std::cerr << "[AVISO] Nenhuma entrada fornecida via stdin.\n";
            print_help(argv[0]);
            return 1;
        }
    } else {
        std::ifstream file(inputFile);
        if (!file.is_open()) {
            std::cerr << "[ERRO] Nao foi possivel abrir o arquivo de entrada: " << inputFile << "\n";
            return 1;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        matrixContent = buffer.str();
    }

    auto startParse = std::chrono::high_resolution_clock::now();

    // Parse matrix
    auto parseResult = spritex::tools::MatrixParser::parse(matrixContent);
    if (!parseResult.success) {
        std::cerr << "[ERRO] Falha ao processar matriz: " << parseResult.error << "\n";
        return 1;
    }

    const auto& matrix = parseResult.matrix;

    auto endParse = std::chrono::high_resolution_clock::now();
    double parseMs = std::chrono::duration<double, std::milli>(endParse - startParse).count();

    print_banner();
    std::cout << "[+] Entrada:         " << (inputFile.empty() ? "(stdin)" : inputFile) << "\n";
    std::cout << "[+] Dimensoes:       " << matrix.width << "x" << matrix.height << " pixels\n";
    std::cout << "[+] Total Tokens:    " << (matrix.width * matrix.height) << "\n";
    std::cout << "[+] Parse concluido: " << std::fixed << std::setprecision(2) << parseMs << " ms\n";

    if (showInfoOnly) {
        // Tally colors and exit
        std::array<bool, 256> seen{};
        size_t count = 0;
        for (uint8_t t : matrix.data) {
            if (!seen[t]) {
                seen[t] = true;
                count++;
            }
        }
        std::cout << "[+] Cores unicas:    " << count << " / 32\n";
        std::cout << "======================================================\n";
        return 0;
    }

    // Rasterize to PNG
    spritex::tools::Rasterizer::Options opt;
    opt.scale = scale;

    auto startRaster = std::chrono::high_resolution_clock::now();
    auto renderResult = spritex::tools::Rasterizer::to_png(matrix, outputFile, opt);
    auto endRaster = std::chrono::high_resolution_clock::now();

    double rasterMs = std::chrono::duration<double, std::milli>(endRaster - startRaster).count();

    if (!renderResult.success) {
        std::cerr << "[ERRO] Falha na rasterizacao: " << renderResult.errorMessage << "\n";
        return 1;
    }

    std::cout << "[+] Cores unicas:    " << renderResult.uniqueColors << " / 32\n";
    std::cout << "[+] Fator de Escala: " << scale << "x ("
              << renderResult.outputWidth << "x" << renderResult.outputHeight << " px)\n";
    std::cout << "[+] Imagem de Saida: " << outputFile << "\n";
    std::cout << "[+] Rasterizacao:    " << std::fixed << std::setprecision(2) << rasterMs << " ms\n";
    std::cout << "======================================================\n";
    std::cout << "[OK] PNG gerado com sucesso em "
              << std::fixed << std::setprecision(2) << (parseMs + rasterMs) << " ms!\n";

    return 0;
}
