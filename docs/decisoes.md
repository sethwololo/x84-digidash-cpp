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
| Manual elétrico 2003 | `~/Documents/Megane Workshop Repairs 2003/MR364MEGANE8.pdf`, seção 83A (PDF p. 90 a 110) | lógica do próprio painel: luzes, mensagens, defect modes |
| Injeção "petrol" (vale para o Hi-Flex) | `~/Documents/Renault-Megane-2-workshop-manual-2003-2009/Engine & Peripherals/MR366X8417B*.pdf` | falha de injeção nível 1 e 2 |
| ABS Bosch 8.0 | `.../Chassis/MR366X8438C000.pdf`, p. 38C-7 e 38C-8 | tabela das luzes do ABS |
| Central do motor | `.../Electrical Equipment/MR366X8487G050.pdf` | falha de carga da bateria |
| Direção elétrica | `.../Chassis/MR366X8436B000.pdf` | SERVICE e "Check steering" |
| Manual do proprietário | `~/Downloads/Manual de Proprietário Megane Sedan e Gran Tour.pdf`, p. 1.31 a 1.39 | descrição genérica das luzes |

A seção 83A do manual de 2003 é a referência principal: descreve o painel, não cada central.

## STOP e SERVICE

O painel original acende o STOP (vermelho, com bipe) ou o SERVICE (laranja) junto com a luz própria do sistema
(83A-18, itens II e III) ou com uma mensagem (tabela de 83A-19 e 83A-20).

| Luz | Condição | Frame | Fonte |
|---|---|---|---|
| STOP | `Défaut DAE` = 10 (falha maior da direção) | `5E4` | base de sinais |
| STOP | `Défaillance verrouillage colonne` = 001 (UCH/STOP) | `60D` | base de sinais |
| STOP | falha de injeção gravidade 2 | `551` | 17B; 83A-19 "Faulty injection", "Engine overheating" |
| STOP | `Incident circuit frein` (nível do fluido de freio) | `5C5` | 83A-18 III; 38C "Brake faults" |
| STOP | luz da pressão do óleo | `625` | 83A-18 III |
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

Com a ignição ligada e o motor parado, a luz do óleo acende e leva o STOP junto. É o comportamento do original: o
manual do proprietário diz que o STOP "apaga-se com o motor em funcionamento".

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

A trava das crianças e o limpador não estão na tabela, mas 83A-18 II diz que acendem o SERVICE com bipe.

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

## Computador de bordo

- **Consumo médio:** aparece depois de 400 m desde o reset, como o original (83A-17). Antes era 1 km.

## Divergências de propósito

| Original | Painel novo | Por quê |
|---|---|---|
| ponteiros vão a zero com o frame ausente (velocímetro, conta-giros, temperatura) | `--` | zero parece um valor real; `--` diz que não há dado |
| sem a UCH: símbolo de portas aceso e setas acesas | não mostra | são estados inventados; o SERVICE já avisa da falha |
| boia em falha: luz de reserva piscando | só o SERVICE e o nível em `--` | a moldura RESERVA diria que falta combustível, o que não se sabe |
| frame da direção ausente: "faulty steering" com SERVICE | FALHA NA DIREÇÃO na cor de falha, com SERVICE | a mensagem do manual é a da falha maior; a cor segue a mensagem |

## A conferir no carro

1. Com a ignição ligada e tudo normal, o log diz `no expected frame missing`. Se o `0x5E4` ou o `0x651` não chegarem
   pela porta OBD, a regra deles precisa sair.
2. Na partida, nenhum aviso pisca: se piscar, aumentar `IGNITION_SETTLE_MS`.
3. Ignição ligada e motor parado: o STOP acende junto com a luz do óleo, como no original.
4. A polaridade da luz do óleo (`OIL_LAMP_WHEN_SWITCH_CLOSED`) continua a conferir, e agora também leva o STOP.
