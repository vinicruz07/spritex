# 🖥️ spritex Studio GUI — Visualizador & Editor Interativo de Pixel Art

Interface Gráfica (GUI) nativa em **C++17 puro (Win32 GDI)**, ultrarrápida, sem dependências externas pesadas e com visual moderno *Dark Theme*.

Permite importar qualquer imagem normal (PNG/JPG), colar matrizes numéricas geradas por LLMs ou criadas manualmente, editá-las em tempo real, remover fundos com 1 clique e visualizar a arte final instantaneamente a 60+ FPS.

---

## ✨ Recursos

* **🖼️ Importador de Imagens (PNG/JPG):** Botão para carregar qualquer imagem do computador; ela é pixelizada e quantizada para DB32 em **< 45 ms** e renderizada na hora.
* **🎛️ Seletores de Resolução Alvo (16x16, 32x32, 64x64):**
  * Botões dedicados com indicação do modo ativo (`[ 16x16 ]`, `[ 32x32 ]`, `[ 64x64 ]`).
  * **Re-Pixelização Instantânea de Imagens:** Ao alternar a resolução com uma imagem já carregada, o app reprocessa na hora a partir da imagem original na nova resolução.
  * **Redimensionamento de Matrizes:** Se estiver editando uma matriz colada ou manual, o app redimensiona suavemente via interpolação *nearest-neighbor*.
* **🪄 Removedor de Fundo Inteligente (Botão & Botão Direito):**
  * **Botão `[Remover Fundo]`:** Detecta automaticamente a cor do fundo pelas bordas externas e aplica *Exterior Flood-Fill*, tornando todo o fundo transparente (`token 0`). **Preserva 100% dos detalhes internos** (como reflexos brancos ou contornos da armadura que usem a mesma cor).
  * **Clique com o Botão Direito no Sprite:** Clique em qualquer pixel ou bolsão de cor na tela para convertê-lo imediatamente em Transparente (`token 0`) por inundação conectada!
* **⚡ Atualização em Tempo Real (Zero Lag):** Ao digitar ou colar qualquer matriz (`Ctrl+V`), o parser nativo em C++ processa a entrada em **< 0.3 ms** e atualiza a tela na hora.
* **🎨 Paleta Master DB32 (32 Cores):** Suporte nativo ao canal alfa (token `0` transparente com fundo quadriculado estilo *Photoshop / Aseprite*).
* **🔍 Inspetor de Pixels Interativo:** Ao mover o mouse sobre o sprite, o painel exibe as coordenadas `(X, Y)`, o token e a cor exata (`Nome, Hex e RGBA`).
* **🎯 Régua / Paleta Clicável no Rodapé:** Todos os 32 swatches de cores exibidos com seus respectivos números. Clicar em qualquer cor copia o número do token para a área de transferência.
* **📐 Escalonamento Nearest-Neighbor:** Seleção rápida de escala de visualização (`1x`, `4x`, `8x`, `12x`, `16x HD`, `20x`, `24x`).
* **🏁 Grade de Pixels (Grid):** Checkbox para ativar/desativar linhas sutis demarcando cada pixel.
* **💾 Exportação Completa:**
  * **Salvar PNG (HD):** Exporta o PNG em alta resolução (16x / 512x512 px).
  * **Salvar PNG (1x):** Exporta o PNG na escala nativa exata (32x32 px) para engines como Unity, Godot ou Unreal.
  * **Copiar Imagem:** Coloca o bitmap gerado na Área de Transferência do Windows para colar direto no Discord, Photoshop, Paint, etc.
* **⚡ Amostras Rápidas Embutidas:** Botões de 1 clique para carregar o Paladino 32x32 e o Guerreiro 16x16.

---

## 🚀 Como Executar

Basta abrir o executável diretamente:

```bash
# Pelo terminal dentro da pasta:
cd "tools/mat2png_gui"
.\mat2png_gui.exe
```

Ou dar um duplo clique em `mat2png_gui.exe` no Windows Explorer!

---

## 🛠️ Compilação

### Via g++ (MinGW)
```bash
g++ -std=c++17 -O2 -I../../third_party -I../mat2png -I../img2mat -I. main.cpp -mwindows -lgdi32 -luser32 -lcomdlg32 -lcomctl32 -o mat2png_gui.exe
```

### Via CMake
```bash
cmake -B build
cmake --build build --target mat2png_gui
```
