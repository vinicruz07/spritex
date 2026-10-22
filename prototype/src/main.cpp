#include <iostream>
#include <string>
#include <sstream>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "generator.hpp"
#include "router.hpp"

using namespace spritex::prototype;

void process_prompt(const std::string& user_prompt, int id = 0) {
    SpriteSample sample = SemanticRouter::resolve_sample(user_prompt);

    // Gerar nome de arquivo limpo baseado no prompt do usuario
    std::string safe_name = "";
    for (char c : user_prompt) {
        if (std::isalnum(static_cast<unsigned char>(c))) safe_name += std::tolower(c);
        else if (c == ' ' || c == '_' || c == '-') {
            if (!safe_name.empty() && safe_name.back() != '_') safe_name += '_';
        }
    }
    if (safe_name.empty()) safe_name = "custom_sprite_" + std::to_string(id);
    
    std::string output_path = "prototype/output/" + safe_name + ".png";

    MaskGITSimulator::generate_from_sample(sample, output_path);
}

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char** argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    std::cout << "\n";
    std::cout << "#########################################################\n";
    std::cout << "#          spritex - PROTOTIPO INTERATIVO C++           #\n";
    std::cout << "#      Digite um prompt ou passe como argumento CLI      #\n";
    std::cout << "#########################################################\n";

    // 1. Se passou argumentos por linha de comando
    if (argc > 1) {
        std::stringstream ss;
        for (int i = 1; i < argc; ++i) {
            if (i > 1) ss << " ";
            ss << argv[i];
        }
        std::string prompt = ss.str();
        process_prompt(prompt, 1);
        return 0;
    }

    // 2. Modo Interativo no Console (REPL)
    std::cout << "\nExemplos de prompts que voce pode testar:\n";
    std::cout << "  - 'mulher loira'\n";
    std::cout << "  - 'um cavaleiro de fogo'\n";
    std::cout << "  - 'uma espada de fogo'\n";
    std::cout << "  - 'um machado de madeira'\n";
    std::cout << "  - 'uma bola de fogo magica'\n\n";

    std::string line;
    int counter = 1;
    while (true) {
        std::cout << "spritex (digite um prompt ou 'sair') > ";
        if (!std::getline(std::cin, line) || line.empty()) break;
        if (line == "sair" || line == "exit" || line == "quit") break;

        process_prompt(line, counter++);
        std::cout << "\n---------------------------------------------------------\n";
    }

    std::cout << "\nEncerrando prototipo do spritex. Ate mais!\n";
    return 0;
}
