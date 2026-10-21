# Decisões

O objetivo é, primeiro, replicar o painel original do Mégane II o mais fielmente possível: as mesmas luzes, nas
mesmas condições, com as mesmas mensagens. Depois, melhorar o que der e mostrar mais informação de uma vez. Este
documento registra o que veio de onde e onde o painel novo diverge do original de propósito.

## Contexto

- **Carro:** Mégane II brasileiro, K4M 1.6 16v Hi-Flex (gasolina e etanol), câmbio manual JH3. Não tem ESP, monitor de pressão dos
  pneus, câmbio automático, freio de mão elétrico nem SSPP: as regras desses sistemas ficam de fora.
- **Leitura:** pela porta OBD. O painel original continua no carro e é a fonte de `0x5C5`, `0x645` e `0x715`
  (combustível, freio de mão, nível do fluido de freio, luz do airbag, nível de óleo, hodômetro), que ele lê por fio
  e retransmite.
- **Regras do CAN** (sinais, timeouts, origem de cada dado): `docs/can-plan.md` do projeto Rust
  (`~/dev/rust/x84-digidash`).

## Fontes

| Fonte | Onde | O que tem |
|---|---|---|
| Base de sinais | `can/*.json` do projeto Rust | sinais cuja descrição cita Stop ou Service |
| Bases de CAN do DDT2000 | `~/Downloads/DDT2000data/ecus/`: `v84.04.xml`, `v84.05.xml`, `can_v84_10.xml`, `canv84.12.xml` | todas mapeadas em `can/signals-x84.json` (a `v84.10` traduzida, mais o que só existe nas outras, marcado em `bases`; o tamanho de cada frame por versão em `dlc_by_base`) |
| Painel original no DDT2000 | `~/Downloads/DDT2000data/ecus/Tdb_BC85__EG_MG_84Phase2_Serie_rev6.xml` | configuração e lógica do painel: limiar do alerta de óleo, tabela da boia, frames ausentes |
| Manual elétrico 2003 | `~/Documents/Megane Workshop Repairs 2003/MR364MEGANE8.pdf`, seção 83A (PDF p. 90 a 110) | lógica do próprio painel: luzes, mensagens, defect modes |
| Injeção "petrol" (os níveis valem para o Hi-Flex, ver o pyren abaixo) | `~/Documents/Renault-Megane-2-workshop-manual-2003-2009/Engine & Peripherals/MR366X8417B*.pdf` | falha de injeção nível 1 e 2 |
| ABS Bosch 8.0 | `.../Chassis/MR366X8438C000.pdf`, p. 38C-7 e 38C-8 | tabela das luzes do ABS |
| Central do motor | `.../Electrical Equipment/MR366X8487G050.pdf` | falha de carga da bateria |
| Direção elétrica | `.../Chassis/MR366X8436B000.pdf` | SERVICE e "Check steering" |
| Manual do proprietário | `~/Downloads/Manual de Proprietário Megane Sedan e Gran Tour.pdf`, p. 1.31 a 1.39 | descrição genérica das luzes |
| Diagnóstico (pyren) | `~/Downloads/pyrendata_247_and_Lada_and_OldRenault/`: `EcuRenault/Uces.xml`, `Sessions/FG0110294.xml` e `FG0110792.xml` (injeção S3000 Flex do X84), `Vehicles/TCOM_086.xml` (Mégane II) | centrais do carro e o que a ferramenta de diagnóstico lê de cada uma |

A seção 83A do manual de 2003 é a referência principal: descreve o painel, não cada central.

O banco do pyren descreve o diagnóstico (perguntas da ferramenta e respostas da central), não as mensagens que as
centrais trocam sozinhas na rede. Dele vêm duas confirmações:

- A injeção S3000 Flex do X84 fase 2 (`FG0110792.xml`) tem os estados "DEFAUT INJECTION GRAVITE 1" e "GRAVITE 2":
  os níveis do manual 17B valem para o Hi-Flex.
- O Mégane II tem na mesma rede de diagnóstico, a da porta OBD, a direção elétrica (endereço `04`), o airbag (`2C`),
  a UCH (`26`), a UPC (`27`), o painel (`51`), o ABS (`01`), o ar-condicionado (`29`) e a injeção (`7A`).

Os `.p` do pyren são pickles de Python (abri-los pode executar código) e não foram abertos.

## STOP e SERVICE

O painel original acende o STOP (vermelho, com bipe) ou o SERVICE (laranja) junto com a luz própria do sistema
(83A-18, itens II e III) ou com uma mensagem (tabela de 83A-19 e 83A-20).

| Luz | Condição | Frame | Fonte |
|---|---|---|---|
| STOP | `Défaut DAE` = 10 (falha maior da direção) | `5E4` | base de sinais |
| STOP | `Défaillance verrouillage colonne` = 001 (UCH/STOP) | `60D` | base de sinais |
| STOP | falha de injeção gravidade 2 | `551` | 17B; 83A-19 "Faulty injection", "Engine overheating" |
| STOP | `Incident circuit frein` (nível do fluido de freio) | `5C5` | 83A-18 III; 38C "Brake faults" |
| STOP | luz da pressão do óleo com o motor a 1600 rpm ou mais | `625` + `181` | 83A-18 III; DDT2000 do painel (falha de pressão: "contato fechado e rotação > limiar", `CFG_SEUILPH`, 1600 rpm de fábrica) |
| STOP | luz da bateria | `625` | 83A-18 III; 87G |
| SERVICE | `Défaut DAE` = 01 (falha menor da direção) | `5E4` | base de sinais; 36B |
| SERVICE | `Défaillance verrouillage colonne` = 110 (UCH/SERV) | `60D` | base de sinais |
| SERVICE | falha de injeção gravidade 1 | `551` | 17B; 83A-18 II; 83A-19 "Check injection" |
| SERVICE | `Voyant état ABS` = 01 (ABS em falha) | `354` | 83A-18 II; 38C |
| SERVICE | luz do airbag | `5C5` | 83A-18 II (e visto no carro) |
| SERVICE | óleo no mínimo (`0x715` alerta) | `715` | 83A-16; 83A-19 "Adjust oil level" |
| SERVICE | boia do tanque em curto ou aberta | `5C5` | 83A-21 |
| SERVICE | avisos marcados Service na tabela de mensagens | `60D`, `35D` | 83A-19, 83A-20 (ver abaixo) |

Ficam de fora:

- **ABS não programado (10) ou em diagnóstico (11):** as linhas da tabela 38C que piscam o ABS só acontecem na
  oficina.
- **Temperatura do motor e luz MOTOR (MIL):** nenhuma fonte liga ao STOP ou ao SERVICE. A mensagem "Engine
  overheating" (Stop) vem da falha de injeção gravidade 2, já coberta.
- **Imobilizador (SERVICE em 83A-18):** não há sinal no CAN para a falha dele.
- **ESP, pneus, câmbio automático, filtro de partículas:** o carro não tem.

Com a ignição ligada e o motor parado, ou em marcha lenta, a luz do óleo acende sozinha: o painel original só declara
a falha de pressão acima do limiar de rotação. A captura de 21/10/2026 confirmou a polaridade: o pressostato fica
"fechado" com a ignição ligada e o motor parado.

## Mensagens

A cor da mensagem no cabeçalho segue a tabela de 83A-19 e 83A-20: Stop é a cor de falha, Service a de SERVICE, e
"-" informativa. Mudou em relação à versão anterior:

| Mensagem | Antes | Agora |
|---|---|---|
| CARTÃO NÃO DETECTADO | informativa | SERVICE |
| PILHA DO CARTÃO FRACA | SERVICE | informativa |
| MODO CARTÃO RESTRITO | SERVICE | informativa |
| VERIFICAR DIREÇÃO (nova) | — | SERVICE, com a falha menor da direção |
| FALHA NA DIREÇÃO (nova) | — | falha, com a falha maior da direção ou o frame dela ausente |
| FALHA ELETRÔNICA (nova) | — | SERVICE, com frame ausente da injeção ou da UPC |

O limpador não está na tabela, mas 83A-18 II diz que acende o SERVICE com bipe. O mesmo item diz isso da trava das
crianças, mas no carro o painel original só bipa (três vezes, na partida, pelo pedido de buzzer da UCH no `60D`): o
painel novo mostra FALHA TRAVA DE CRIANÇA sem acender o SERVICE.

## Frames ausentes

O original trata a falta de um frame como alerta (83A-22 e 83A-23). O painel novo faz o mesmo para os frames das
centrais que o original espera, depois que a ignição se firma:

| Frame ausente | Luzes e mensagem |
|---|---|
| Airbag (`651`) | airbag + SERVICE |
| ABS (`354`) | freio + ABS |
| Injeção, rotação (`181`) | SERVICE + FALHA ELETRÔNICA; óleo, bateria e temperatura |
| Injeção, geral (`551`) | SERVICE + FALHA ELETRÔNICA |
| Direção (`5E4`) | SERVICE + FALHA NA DIREÇÃO |
| UCH (`60D`) | SERVICE |
| UPC (`625`) | SERVICE + FALHA ELETRÔNICA |

- **Direção:** o manual chama a linha de "ESP frame", mas a mensagem é "faulty steering", a da direção elétrica. O
  carro não tem ESP, então a regra vale para o `0x5E4`.
- **Ignição firme:** a ignição vem do estado APC da UPC (`0x625`) ou, sem ele, de qualquer frame que o lado do
  motor só manda com a ignição ligada. Um frame só conta como ausente 3 s depois de a ignição ligar, para cada
  central ter tempo de começar. O valor de 3 s não está no manual.
- **Só o que o manual lista:** as luzes de óleo e bateria acesas pela falta do `0x181` não levam o STOP junto,
  porque a tabela não cita o STOP.
- **Rede inteira ausente:** mostra "SEM CAN" no rodapé. Sem nenhum frame, não dá para saber se a ignição está
  ligada, e o original também só alerta com a ignição ligada.
- **Frames do painel original** (`5C5`, `645`, `715`): não estão na tabela, porque o original é quem os manda.
  Ausentes, os valores deles mostram `--`.
- **Log:** o firmware registra no log os frames esperados que faltam sempre que a lista muda (`missing frames:
  0x5E4`). É o que conferir no carro: com tudo funcionando, a lista deve ficar vazia.

## Backlight

À noite (farol baixo ou alto), o backlight segue o reostato do painel original (`0x645`, porcentagem máxima de noite),
arredondado e limitado entre 10 % e 70 %; sem o frame, 70 %. A captura de 21/10/2026 mostrou o reostato indo de 95 %
a 4 % (o byte de dia fica em 255). De dia, 100 %.

## Computador de bordo

- **Consumo médio:** aparece depois de 400 m desde o reset, como o original (83A-17). Antes era 1 km.

## Diagnóstico ativo (possibilidade, não implementado)

Alguns dados que o painel original não mostra só saem perguntando à injeção. Ela não os manda sozinha na rede, e a
base de sinais não tem nenhum sinal de álcool ou etanol. Os pedidos estão no banco do pyren (`SG0110294.p`, um
pickle só com strings, sem código; conferido com `pickletools` antes de carregar):

| Dado | Pedido (KWP2000, `0x7E0`, resposta em `0x7E8`) | Na resposta | Conversão |
|---|---|---|---|
| Taxa de álcool estimada no tanque | `21 A0` | bytes 27 e 28 (16 bits) | valor / 655,35 = % |
| Consumo instantâneo | `21 A9` | bytes 3 e 4 (16 bits) | valor / 100 = L/h |

A taxa de álcool só existe na versão `S3000_AD_CAN_1_X84_FLEXFUEL` (`FG0110294`); a de 2012 (`FG0110792`) não tem.

Caminhos possíveis:

1. **Escutar:** quando um scanner na porta OBD faz o pedido, o painel lê a resposta em `0x7E8` sem sair do modo
   listen-only. Só funciona com o scanner ligado.
2. **Perguntar:** o painel abre uma sessão de diagnóstico (`10 C0`), mantém-na viva (`3E 01` a cada 3 s, como o
   pyren) e pede `21 A0` de tempos em tempos. Exige transmitir na rede do carro, o que muda a natureza do projeto:
   sair do listen-only, conviver com a ferramenta da oficina ligada à mesma porta e garantir que um erro não
   perturbe as centrais.

O consumo instantâneo não precisa disso: sai dos contadores do `0x551` e do `0x354`, os mesmos do KM/L.

## Divergências de propósito

| Original | Painel novo | Por quê |
|---|---|---|
| ponteiros vão a zero com o frame ausente (velocímetro, conta-giros, temperatura) | `--` | zero parece um valor real; `--` diz que não há dado |
| sem a UCH: símbolo de portas aceso e setas acesas | não mostra | são estados inventados; o SERVICE já avisa da falha |
| boia em falha: luz de reserva piscando | só o SERVICE e o nível em `--` | a moldura RESERVA diria que falta combustível, o que não se sabe |
| falha da trava das crianças: só os bipes | a mensagem FALHA TRAVA DE CRIANÇA, sem SERVICE | mostra o que a UCH já sabe, sem um alerta que o original não dá |
| frame da direção ausente: "faulty steering" com SERVICE | FALHA NA DIREÇÃO na cor de falha, com SERVICE | a mensagem do manual é a da falha maior; a cor segue a mensagem |

## A conferir no carro

1. Com a ignição ligada e tudo normal, o log diz `no expected frame missing`. A direção e o airbag estão na rede da
   porta OBD (banco do pyren); se mesmo assim o `0x5E4` ou o `0x651` não chegarem, a regra deles precisa sair.
2. Na partida, nenhum aviso pisca: se piscar, aumentar `IGNITION_SETTLE_MS`.
3. Ignição ligada e motor parado: a luz do óleo acende sem o STOP, como no original.
