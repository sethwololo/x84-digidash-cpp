# x84-digidash-cpp

Painel de instrumentos digital para um Renault Mégane II (plataforma X84), em C++ com LVGL, numa Waveshare
ESP32-P4 com tela DSI de 8,8" (480x1920, retrato) usada em paisagem (1920x480). É a versão C++ do projeto Rust +
Slint `x84-digidash`: mesma tela, mesma leitura do CAN.

## Comandos

Tudo pelo `./idf.sh`, que carrega o ESP-IDF 6.0.2 e chama o `idf.py`:

```sh
./idf.sh build                         # firmware lendo o CAN do carro
./idf.sh -p /dev/cu.usbmodem… flash    # grava (a porta muda ao trocar de cabo: ls /dev/cu.usbmodem*)
./idf.sh -p /dev/cu.usbmodem… monitor  # log serial (Ctrl+] sai)
```

Na bancada, sem o carro, a tela mostra `--` e "SEM CAN". Para ver os dados simulados (os três cenários do design,
alternando a cada 12 s), use a variante de simulação, que compila em `build-sim/`:

```sh
./idf.sh -B build-sim -D SDKCONFIG=build-sim/sdkconfig -D "SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.simulation" build flash
```

No Mac, sem a placa:

```sh
cmake -S sim -B sim/build && cmake --build sim/build      # simulador (precisa do SDL2: brew install sdl2)
sim/build/dashboard-sim                                    # janela com a simulação
sim/build/dashboard-sim --screenshot out.png 1             # cenário 1 (0 normal, 1 alerta, 2 combustível) em PNG

cmake -S components/x84_can/test -B components/x84_can/test/build
cmake --build components/x84_can/test/build && components/x84_can/test/build/x84_can_test   # testes do CAN
```

## Onde está cada coisa

- `components/ui/`: a tela, em LVGL. Só ela desenha; o resto do firmware entrega um `Screen` (`screen.h`) ao
  `Dashboard::show` (`dashboard.h`).
  - `theme.h`: cores, fontes (escala tipográfica), escalas dos medidores e as réguas do layout. É onde se muda a
    aparência.
  - `base.h`: os blocos de montar (caixa, contorno, texto, imagem) e os setters que só mexem no LVGL quando o
    valor muda.
  - `widgets/`: peças reutilizáveis (barras, caixas de valor, luzes de aviso, moldura de alerta, portas, setas).
  - `panels/`: o quadro (cantos, cabeçalho, rodapé) e as três colunas, montados com os widgets.
  - `simulation.cpp`: dados falsos para a bancada e o simulador.
  - `fonts/` e `images/`: gerados por `tools/` (não editar à mão).
- `components/x84_can/`: o CAN do carro, em C++ puro (compila também no Mac, para os testes).
  - `frames.h`: posição de cada sinal e um decodificador por frame. `view.cpp`: o que a tela mostra a partir dos
    frames (lâmpadas, avisos, marcha). `trip.h`: distância e combustível do trip.
  - `test/`: os testes, conferidos contra `can/signals-x84.json`.
- `main/`: a placa. `display.cpp` (tela e LVGL), `can.cpp` (TWAI em listen-only, TX GPIO3 / RX GPIO2),
  `vehicle.cpp` (frames → `Screen`, trip no NVS), `intro.cpp` (fade do backlight), `main.cpp` (liga tudo).
- `sim/`: o simulador de desktop. `tools/`: conversores de fontes e imagens.

## Mudanças comuns

- **Uma cor ou um tamanho de texto:** `components/ui/theme.h`.
- **Um tamanho de fonte novo:** uma linha em `tools/fonts.sh`, rodar o script, declarar em `components/ui/fonts.h`
  e criar o `theme::Font` em `theme.h`.
- **Uma imagem nova:** o SVG em `assets/images/`, depois `cd tools && npm install && node images.mjs`, e declarar em
  `components/ui/images.h`.
- **Mover algo na tela:** as posições estão no topo de cada arquivo de `panels/`, como constantes com nome.
- Antes de gravar, confira no simulador: `cmake --build sim/build && sim/build/dashboard-sim --screenshot out.png 2`.

## Editor

O `.clangd` já remove os flags do GCC da placa que o clang não conhece. Para o clangd achar os headers do
compilador da placa, no VS Code acrescente às configurações do usuário
`"clangd.arguments": ["--query-driver=**/riscv32-esp-elf-*"]` e rode um `./idf.sh build` (ele gera
`build/compile_commands.json`).
