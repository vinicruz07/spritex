# 🛠️ mat2png — Matrix to Pixel Art Rasterizer

Ferramenta de alta performance do ecossistema **`spritex`** desenvolvida em C++17 para converter matrizes numéricas arbitrárias (16x16, 32x32, retangulares, de LLMs ou arquivos de texto/JSON) em imagens PNG nítidas usando a paleta master **DawnBringer 32 (DB32)**.

---

## 🎨 Paleta Padrão de 32 Cores (DB32)
* [`PALETTE_32.md`](PALETTE_32.md) — Tabela visual de Índices (0 a 31), Nomes, Hex, RGBA e Famílias.
* [`palette_32.json`](palette_32.json) — Base de dados em JSON para integração com frontends e scripts.
* [`palette.hpp`](palette.hpp) — Implementação em C++17 com `constexpr array<Color, 32>`.

---

## 🚀 Compilação

### Via g++ (MinGW / GCC)
```bash
g++ -std=c++17 -O2 -I../../third_party -I. main.cpp -o mat2png.exe
```

### Via CMake
```bash
cmake -B build
cmake --build build
```

---

## 💻 Uso e Parâmetros

```text
Uso: mat2png.exe [opções]

Opções:
  -i, --input <arquivo>     Arquivo de matriz (texto/JSON) ou '-' para stdin
  -o, --output <arquivo>    Caminho do arquivo PNG gerado (padrão: output.png)
  -s, --scale <fator>       Fator de ampliação nearest-neighbor (padrão: 16)
  -p, --palette             Exibe a tabela de 32 cores (DB32) no terminal e finaliza
      --info                Exibe dimensões e estatísticas da matriz sem gerar imagem
  -h, --help                Exibe o menu de ajuda
```

---

## 🎯 Exemplos Práticos

### 1. Renderizar um Sprite 32x32 com Escala 16x (512x512 px)
```bash
.\mat2png.exe -i samples/sample_32x32.txt -o samples/knight_32x32.png -s 16
```

### 2. Exportar em Escala Nativa 1:1 (32x32 px puro para Jogos / Game Engines)
```bash
.\mat2png.exe -i samples/sample_32x32.txt -o samples/knight_32x32_native.png -s 1
```

### 3. Exibir apenas Informações e Contagem de Cores
```bash
.\mat2png.exe -i samples/sample_32x32.txt --info
```

### 4. Consultar a Tabela de Cores DB32
```bash
.\mat2png.exe -p
```

### 5. Via Pipe / Stdin (Integração Direta com Scripts e IAs)
```bash
cat samples/sample_16x16.txt | .\mat2png.exe -o out.png
```

---

## 🔮 Roadmap Futuro
- [x] CLI nativa em C++ de alta performance (`mat2png.exe`).
- [x] Parser robusto com suporte a arrays aninhados `[[...]]`, listas de linhas e arquivos plain text.
- [x] Suporte à paleta universal DB32 de 32 cores com canal alfa (transparência nativa).
- [ ] Interface Web / GUI Dinâmica: visualizador interativo em tempo real onde o usuário pode colar qualquer matriz e ver a pixel art renderizada instantaneamente.
