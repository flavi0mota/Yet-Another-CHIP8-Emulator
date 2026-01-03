# Tracua-CHIP8: A C99 Cycle-Accurate VM Implementation

![Language](https://img.shields.io/badge/language-C99-00599C?style=flat-square&logo=c&logoColor=white)
![Build System](https://img.shields.io/badge/build-Meson-green?style=flat-square&logo=meson&logoColor=white)
![Library](https://img.shields.io/badge/backend-SDL2-FF3D00?style=flat-square&logo=sdl&logoColor=white)
![License](https://img.shields.io/badge/license-MIT-blue?style=flat-square)

Uma implementação de intepretador para CHIP-8, desenvolvida em C com foco em **precisão de ciclo**, **gerenciamento manual de memória** e **modularidade arquitetural**.

## Arquitetura Básica

O projeto foi estruturado para desacoplar a lógica da CPU da camada de apresentação. Isso permite que o núcleo da emulação seja portado para outros backends (como OpenGL ou Raylib) sem alteração na lógica de opcode.

### O Ciclo de Execução (Fetch-Decode-Execute)
A CPU opera em um loop síncrono que simula a frequência de 500-700hz original (settado para 600hz no código):

1.  **Fetch:** O Opcode de 16-bits é recuperado da memória (Big-endian) combinando dois bytes adjacentes: `opcode = memory[pc] << 8 | memory[pc + 1]`.
2.  **Decode:** Utiliza-se mascaramento de bits (Bitwise AND) para isolar os nibbles de instrução.
3.  **Execute:** Um *Switch Dispatch* roteia para a função correspondente.
4.  **Timers:** Os registradores de *Delay* e *Sound* são decrementados a 60Hz, independentemente do clock da CPU.

### Decisões e casos

* **Renderização via XOR:** A lógica de desenho implementa o comportamento de *sprite wrapping* e detecção de colisão por operações de XOR, essencial para jogos que dependem do "glitch".
* **Áudio Procedural:** O áudio é sintetizado em tempo real gerando uma Onda Quadrada (Square Wave) pura via buffer de áudio da SDL2, reduzindo o *footprint* do binário.
* **Tratamento de "Quirks":** A arquitetura COSMAC VIP original e as implementações modernas (SuperChip) tratam instruções como `8xy6` (Bit shift) e `Fx55` (Memory Dump) de formas diferentes. Este emulador implementa *flags* de configuração para alternar comportamentos em tempo de execução.

---

## 📂 Estrutura do Código

A separação de responsabilidades segue o padrão de headers públicos e implementações privadas:

```text
.
├── include/        # Definições de interfaces e macros (CPU, Bus, Graphics)
├── src/            # Implementação da lógica de emulação
│   ├── main.c      # Entry point e loop principal
│   ├── cpu.c       # Lógica de processamento de Opcodes
│   └── platform.c  # Camada de abstração SDL2 (Vídeo/Áudio/Input)
├── meson.build     # Configuração de build (Cross-platform friendly)
```
---

## Screenshots
<img width="1282" height="681" alt="image" src="https://github.com/user-attachments/assets/653417dd-eae5-4cda-80d9-97af984f8f80" />

---

## Instalação e Compilação

### Pré-requisitos

Você precisará de:

 - Compilador (gcc ou clang)

 - Meson e Ninja (sistema de build)

 - Biblioteca SDL2.

**Debian/Ubuntu:**

```bash
sudo apt-get install build-essential libsdl2-dev meson ninja-build
```

**Compilando**

```bash
meson setup build

meson compile -C build
```
Também é possível utilizar o script build.sh

ESTE PROJETO NÃO SERIA POSSÍVEL SEM OS SEGUINTES RECURSOS:

Cowgod's CHIP-8 Technical Reference: http://devernay.free.fr/hacks/chip8/C8TECH10.HTM
Tobias V. Langhoff Guide to Write a CHIP-8 Emulator: https://tobiasvl.github.io/blog/write-a-chip-8-emulator/
