# DrikSynth

Sintetizador VST3 para o Reaper (Windows), com uma biblioteca de presets que imitam teclados clássicos.
É o irmão azul do [DrikSampler](https://github.com/adrik29/DrikSampler), que é laranja.

## Baixar

Em **Releases**, à direita desta página, fica sempre a última compilação:
`DrikSynth-Windows.zip`. As instruções de instalação estão no `LEIA-ME-instalacao.txt` dentro do zip.

## O motor

- Dois osciladores (serra, pulso, triângulo, seno), sub-oscilador e ruído
- Largura de pulso com modulação (PWM), sync do oscilador 2
- Unison de até 4 vozes, com desafino e abertura estéreo
- Filtro passa-baixa de 24 dB (família Moog/Roland) ou 12 dB (família Oberheim), mais passa-alta
- Envelopes separados para filtro e volume, com curvas exponenciais
- LFO para vibrato, PWM e filtro; a roda de modulação acrescenta vibrato
- Drive: distorção sobre a soma das vozes, com controle de tom
- Chorus I e II (família Juno) e Ensemble de três linhas (string machines)
- Modo mono com glide
- 16 vozes

## Os presets

42 presets, agrupados pela máquina que imitam: string machines (Solina, Elka Rhapsody, Yamaha SS-30,
ARP Omni, Polymoog), Roland Juno e Jupiter, Oberheim, Moog, Sequential Prophet, Yamaha CS-80, ARP Odyssey,
Roland SH-101 e outros.

São aproximações da arquitetura e dos patches típicos de cada instrumento, não cópias do circuito.
Os nomes das máquinas pertencem aos seus fabricantes e aparecem só para dizer o que cada preset imita.
A lista está em `Source/Presets.h`, num formato fácil de ler e editar.

## Para aprender mexendo

- Pare o mouse em cima de qualquer controle: aparece uma explicação do que ele faz.
- O botão **Segurar** prende as notas tocadas (elas ficam acesas no teclado da tela) até serem tocadas
  de novo. Assim dá para deixar um acorde soando e girar os botões com o mouse.

## Os teus sons

O botão **Salvar...** guarda o som atual com o nome que você der. Ele passa a aparecer no alto da lista,
em **Meus sons**. Cada som é um arquivo `.drikpreset` em `Documentos\DrikSynth\Presets`; para apagar ou
renomear um som, apague ou renomeie o arquivo.

## O que ainda não faz

- Legato de verdade no modo mono
- Envelope de afinação (para varrer o sync)
- Síntese FM (DX7): é outro motor; para isso existe o Dexed

## Compilar

O GitHub compila sozinho a cada alteração (`.github/workflows/build.yml`). Para compilar à mão:

```
cmake -B build
cmake --build build --config Release
```

Testes do motor de áudio, sem interface:

```
cmake -B build -DDRIK_BUILD_TESTS=ON
cmake --build build --target DrikEngineTest
```

Feito com [JUCE](https://juce.com) 8.
