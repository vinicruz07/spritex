# 🖼️ img2mat — Image to Pixel Art Quantizer

Ferramenta de alta performance desenvolvida em **C++17** para transformar qualquer imagem normal (PNG, JPG, BMP geradas por Midjourney, DALL-E, Stable Diffusion ou fotos) em matrizes numéricas $32\times 32$ (ou qualquer dimensão) perfeitamente quantizadas na paleta **DawnBringer 32 (DB32)**.

---

## ✨ Como Funciona o Pipeline

```text
[Imagem HD (ex: 512x512)]
       │
       ▼
1. Grid-Block Pooling (Amostragem Modal: cor dominante por bloco)
       │
       ▼
2. Quantização Perceptual DB32 (Distância Humana CompuPhase CIELAB)
       │
       ▼
3. Filtros Pós-Processamento (Despeckle / Remoção de Ruído 1x1)
       │
       ▼
4. Contorno Retrô Opcional (--outline para contorno preto)
       │
       ▼
[Matriz 32x32 de Tokens 0..31] + [Preview PNG Opcional]
```

---

## 🚀 Compilação

### Via g++ (MinGW)
```bash
g++ -std=c++17 -O2 -I../../third_party -I../mat2png -I. main.cpp -o img2mat.exe
```

### Via CMake
```bash
cmake -B build
cmake --build build --target img2mat
```

---

## 💻 Uso e Parâmetros

```text
Uso: img2mat.exe -i <imagem> [opcoes]

Opcoes Obrigatorias:
  -i, --input <caminho>     Imagem normal (PNG, JPG, BMP, etc.)

Opcoes de Dimensoes e Amostragem:
  -w, --width <int>         Largura da matriz gerada (padrao: 32)
  -h, --height <int>        Altura da matriz gerada (padrao: 32)
  --mode <modal|center|avg> Metodo de pixelizacao (padrao: modal)
                            modal: cor mais frequente (ideal p/ IA/Midjourney)
                            center: pixel central (ideal p/ upscaled art)
                            avg: media RGB antes da quantizacao

Opcoes de Tratamento de Cor & Fundo:
  --key-white               Trata fundo branco (#F0F0F0+) como transparente
  --key-magenta             Trata magenta (#FF00FF) como transparente
  --alpha <0-255>           Limiar de corte alfa (padrao: 128)
  --outline                 Aplica contorno preto retro (token 1) nas bordas
  --no-clean                Desativa filtro de remocao de ruido isolado

Opcoes de Saida:
  -o, --output <arquivo>    Salva matriz em arquivo .txt (padrao: <input>_mat.txt)
  -p, --png <arquivo.png>   Gera imagem PNG rasterizada de preview
  -s, --scale <int>         Escala da imagem PNG gerada (padrao: 16)
  --stdout                  Imprime a matriz gerada no terminal (stdout)
  --help                    Exibe este menu de ajuda
```

---

## 🎯 Exemplos Práticos

### 1. Converter Imagem do Midjourney para Matriz 32x32 com Fundo Branco Removido
```bash
.\img2mat.exe -i "arte_ia.png" -w 32 -h 32 --key-white -p "preview.png"
```

### 2. Converter Foto com Contorno Retrô Ativado
```bash
.\img2mat.exe -i "personagem.png" -o "matriz.txt" --outline
```

### 3. Integração Direta com a Interface Gráfica Studio
Abra o **`mat2png_gui.exe`** e clique no botão **`[Importar Imagem (PNG/JPG)]`**: você pode escolher qualquer imagem do seu computador e ela será pixelizada, quantizada para DB32 e exibida na tela em tempo real!
