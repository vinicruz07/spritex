#include <iostream>
#include <vector>

void print_banner() {
    std::cout << "========================================\n";
    std::cout << "       spritex - Pixel Art Gen AI       \n";
    std::cout << "========================================\n";
}

int main(int argc, char** argv) {
    print_banner();

    // Exemplo conceitual da matriz de tokens/índices
    std::vector<std::vector<int>> sample_sprite = {
        {0, 0, 1, 1, 1, 0, 0},
        {0, 1, 2, 2, 2, 1, 0},
        {1, 2, 3, 2, 3, 2, 1},
        {1, 2, 2, 2, 2, 2, 1},
        {0, 1, 2, 1, 2, 1, 0},
        {0, 0, 1, 0, 1, 0, 0}
    };

    std::cout << "[*] Matriz de entrada conceitual carregada ("
              << sample_sprite.size() << "x" << sample_sprite[0].size() << "):\n\n";

    for (const auto& row : sample_sprite) {
        std::cout << "  ";
        for (int val : row) {
            if (val == 0) std::cout << ". ";
            else std::cout << val << " ";
        }
        std::cout << "\n";
    }

    std::cout << "\n[*] Inicializacao concluida. Pronto para processamento.\n";
    return 0;
}
