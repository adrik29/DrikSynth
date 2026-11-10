#pragma once
// Biblioteca de presets do DrikSynth.
//
// Cada preset parte dos valores padrao e muda so o que esta listado.
// Os nomes dizem qual maquina cada um IMITA: sao aproximacoes da arquitetura
// e dos patches tipicos de cada uma, nao copias do circuito.
//
// Ondas: 0 serra, 1 pulso, 2 triangulo, 3 seno.   Filtro (fmode): 0 = 24 dB, 1 = 12 dB.
// Chorus: 0 desligado, 1 Chorus I, 2 Chorus II, 3 Ensemble.   LFO: 0 tri, 1 seno, 2 quadrada, 3 aleatorio.
#include <utility>
#include <vector>

namespace drik
{
struct Preset
{
    const char* name;
    const char* group;
    std::vector<std::pair<const char*, float>> values;
};

inline const std::vector<Preset>& presets()
{
    static const std::vector<Preset> list = {

    // ------------------------------------------------------------------ String machines
    { "Solina Strings", "String machines",
      { {"o2wave",0},{"o2semi",12},{"mix2",0.5f},{"fmode",1},{"cutoff",5200},{"reso",0.05f},{"hpf",180},
        {"keytrack",0.3f},{"aa",0.12f},{"as",1},{"ar",0.7f},{"chorus",3},{"chomix",0.85f},{"spread",0.3f} } },

    { "Elka Rhapsody Strings", "String machines",
      { {"o2wave",0},{"o2semi",12},{"mix2",0.8f},{"fmode",1},{"cutoff",9000},{"reso",0.1f},{"hpf",350},
        {"keytrack",0.3f},{"aa",0.08f},{"as",1},{"ar",0.5f},{"chorus",3},{"chomix",0.9f},{"spread",0.3f} } },

    { "Yamaha SS-30 Strings", "String machines",
      { {"o2wave",0},{"o2semi",12},{"mix2",0.4f},{"unison",2},{"detune",9},{"spread",0.6f},{"fmode",1},
        {"cutoff",4500},{"reso",0.05f},{"hpf",220},{"keytrack",0.3f},{"aa",0.2f},{"as",1},{"ar",0.9f},
        {"chorus",3},{"chomix",0.7f} } },

    { "String Machine Cello", "String machines",
      { {"o1oct",-1},{"fmode",1},{"cutoff",2200},{"reso",0.1f},{"hpf",60},{"keytrack",0.4f},
        {"aa",0.1f},{"as",1},{"ar",0.5f},{"chorus",3},{"chomix",0.7f},{"spread",0.2f} } },

    { "ARP Omni Strings", "String machines",
      { {"o1wave",1},{"pw",0.35f},{"o2wave",0},{"o2semi",12},{"mix2",0.6f},{"fmode",1},{"cutoff",4200},
        {"hpf",150},{"keytrack",0.3f},{"aa",0.3f},{"as",1},{"ar",1.2f},{"chorus",3},{"chomix",0.8f},{"spread",0.3f} } },

    { "Polymoog Vox Humana", "String machines",
      { {"o1wave",1},{"pw",0.5f},{"pwm",0.55f},{"lforate",5.5f},{"o2wave",0},{"o2fine",8},{"mix2",0.6f},
        {"cutoff",2400},{"reso",0.1f},{"keytrack",0.5f},{"aa",0.15f},{"as",1},{"ar",0.6f},
        {"chorus",2},{"chomix",0.6f},{"spread",0.4f} } },

    // ------------------------------------------------------------------ Tirados de discos
    // Ajustado por medida (espectro, abertura estereo e oscilacao de altura) sobre um trecho
    // de 1,2 s de "New Europeans". E o ponto mais proximo que a medida achou, nao uma copia.
    { "Ultravox New Europeans (aprox.)", "Tirados de discos",
      { {"o2wave",0},{"o2semi",12},{"mix2",0.6f},{"unison",2},{"detune",9},{"spread",0.6f},{"fmode",1},
        {"cutoff",2000},{"reso",0.05f},{"hpf",120},{"keytrack",0.3f},{"aa",0.2f},{"as",1},{"ar",0.9f},
        {"chorus",3},{"chomix",0.5f} } },

    // ------------------------------------------------------------------ Roland Juno
    { "Juno Pad", "Roland Juno",
      { {"o1wave",1},{"pwm",0.6f},{"lforate",0.4f},{"sub",0.5f},{"cutoff",1800},{"reso",0.15f},{"envamt",0.25f},
        {"fa",0.6f},{"fd",1.2f},{"fs",0.6f},{"fr",1.2f},{"aa",0.5f},{"ad",1.0f},{"as",0.9f},{"ar",1.4f},
        {"chorus",1},{"chomix",0.7f},{"spread",0.2f} } },

    { "Juno Strings", "Roland Juno",
      { {"cutoff",3200},{"reso",0.05f},{"hpf",120},{"aa",0.35f},{"as",1},{"ar",0.9f},
        {"chorus",2},{"chomix",0.75f},{"spread",0.2f} } },

    { "Juno Brass", "Roland Juno",
      { {"cutoff",600},{"reso",0.1f},{"envamt",0.55f},{"fa",0.06f},{"fd",0.5f},{"fs",0.5f},{"fr",0.3f},
        {"aa",0.03f},{"as",1},{"ar",0.3f},{"chorus",1},{"chomix",0.6f},{"spread",0.2f} } },

    { "Juno Bass", "Roland Juno",
      { {"o1oct",-1},{"sub",0.7f},{"cutoff",420},{"reso",0.25f},{"envamt",0.45f},{"keytrack",0.3f},
        {"fa",0.001f},{"fd",0.25f},{"fs",0.1f},{"fr",0.15f},{"aa",0.001f},{"ad",0.3f},{"as",0.7f},{"ar",0.12f},{"spread",0} } },

    { "Juno Pluck", "Roland Juno",
      { {"o1wave",1},{"pw",0.4f},{"cutoff",500},{"reso",0.2f},{"envamt",0.6f},{"fa",0.001f},{"fd",0.25f},{"fs",0},
        {"fr",0.25f},{"aa",0.001f},{"ad",0.6f},{"as",0.3f},{"ar",0.35f},{"chorus",1},{"chomix",0.6f},{"spread",0.2f} } },

    { "Juno PWM Organ", "Roland Juno",
      { {"o1wave",1},{"pw",0.3f},{"pwm",0.3f},{"lforate",1.2f},{"sub",0.4f},{"cutoff",2600},{"hpf",90},
        {"aa",0.003f},{"as",1},{"ar",0.08f},{"chorus",2},{"chomix",0.7f},{"spread",0.2f} } },

    // ------------------------------------------------------------------ Roland Jupiter
    { "Jupiter Brass", "Roland Jupiter",
      { {"o2wave",0},{"o2fine",8},{"mix2",1},{"cutoff",500},{"reso",0.1f},{"envamt",0.6f},
        {"fa",0.05f},{"fd",0.6f},{"fs",0.45f},{"fr",0.4f},{"aa",0.02f},{"as",1},{"ar",0.35f},{"spread",0.4f} } },

    { "Jupiter Strings", "Roland Jupiter",
      { {"o2wave",0},{"o2fine",10},{"mix2",1},{"fmode",1},{"cutoff",4000},{"hpf",100},
        {"aa",0.4f},{"as",1},{"ar",1.0f},{"chorus",1},{"chomix",0.4f},{"spread",0.5f} } },

    { "Jupiter Soft Pad", "Roland Jupiter",
      { {"o1wave",2},{"o2wave",0},{"o2fine",-7},{"mix2",0.5f},{"cutoff",1400},{"reso",0.1f},{"envamt",0.2f},
        {"fa",1.2f},{"fd",2.0f},{"fs",0.6f},{"fr",1.5f},{"aa",0.8f},{"as",1},{"ar",1.8f},{"spread",0.6f} } },

    { "Jupiter Arp Pluck", "Roland Jupiter",
      { {"o1wave",1},{"pw",0.45f},{"o2wave",0},{"o2semi",12},{"mix2",0.5f},{"cutoff",900},{"reso",0.25f},
        {"envamt",0.5f},{"fa",0.001f},{"fd",0.18f},{"fs",0},{"fr",0.18f},{"aa",0.001f},{"ad",0.4f},{"as",0.2f},
        {"ar",0.2f},{"spread",0.5f} } },

    { "Jupiter Sync Lead", "Roland Jupiter",
      { {"sync",1},{"mix1",0},{"mix2",1},{"o2wave",0},{"o2semi",19},{"cutoff",5000},{"reso",0.15f},
        {"mono",1},{"glide",0.06f},{"aa",0.003f},{"as",1},{"ar",0.2f},{"lforate",5.5f},{"spread",0} } },

    // ------------------------------------------------------------------ Oberheim
    { "OB-X Brass", "Oberheim",
      { {"o2wave",0},{"o2fine",12},{"mix2",1},{"unison",2},{"detune",10},{"spread",0.8f},{"fmode",1},
        {"cutoff",900},{"reso",0.1f},{"envamt",0.45f},{"fa",0.03f},{"fd",0.7f},{"fs",0.6f},{"fr",0.4f},
        {"aa",0.01f},{"as",1},{"ar",0.3f} } },

    { "OB Strings Pad", "Oberheim",
      { {"o2wave",0},{"o2fine",14},{"mix2",1},{"unison",2},{"detune",14},{"spread",1},{"fmode",1},
        {"cutoff",2600},{"reso",0.1f},{"aa",0.5f},{"as",1},{"ar",1.5f} } },

    { "OB Sweep Pad", "Oberheim",
      { {"o2wave",1},{"pw",0.4f},{"o2fine",9},{"mix2",0.8f},{"spread",0.9f},{"fmode",1},{"cutoff",500},
        {"reso",0.45f},{"envamt",0.6f},{"fa",2.5f},{"fd",3.0f},{"fs",0.4f},{"fr",2.0f},{"aa",0.3f},{"as",1},{"ar",2.0f} } },

    { "OB Unison Lead", "Oberheim",
      { {"o2wave",0},{"o2fine",6},{"mix2",1},{"unison",4},{"detune",18},{"spread",0.7f},{"fmode",1},
        {"cutoff",3000},{"reso",0.15f},{"mono",1},{"glide",0.08f},{"aa",0.003f},{"as",1},{"ar",0.25f} } },

    { "SEM Bass", "Oberheim",
      { {"o1oct",-1},{"o2wave",1},{"pw",0.4f},{"o2semi",-12},{"mix2",0.7f},{"fmode",1},{"cutoff",350},
        {"reso",0.3f},{"envamt",0.45f},{"keytrack",0.3f},{"fa",0.001f},{"fd",0.3f},{"fs",0.2f},{"fr",0.2f},
        {"aa",0.001f},{"as",1},{"ar",0.15f},{"mono",1},{"spread",0} } },

    // ------------------------------------------------------------------ Moog
    { "Minimoog Bass", "Moog",
      { {"o1oct",-1},{"o2wave",0},{"o2semi",-12},{"o2fine",6},{"mix2",0.8f},{"sub",0.5f},{"cutoff",260},
        {"reso",0.2f},{"envamt",0.5f},{"keytrack",0.3f},{"fa",0.001f},{"fd",0.28f},{"fs",0.15f},{"fr",0.2f},
        {"aa",0.001f},{"as",1},{"ar",0.15f},{"mono",1},{"spread",0} } },

    { "Minimoog Lead", "Moog",
      { {"o2wave",0},{"o2fine",7},{"mix2",0.9f},{"cutoff",1800},{"reso",0.15f},{"envamt",0.3f},
        {"fa",0.01f},{"fd",0.5f},{"fs",0.6f},{"fr",0.3f},{"aa",0.005f},{"as",1},{"ar",0.25f},
        {"mono",1},{"glide",0.09f},{"lforate",5.8f},{"spread",0} } },

    { "Lucky Man Lead", "Moog",
      { {"o1wave",1},{"pw",0.5f},{"o2wave",1},{"o2fine",5},{"mix2",0.8f},{"cutoff",2500},{"reso",0.1f},
        {"aa",0.01f},{"as",1},{"ar",0.4f},{"mono",1},{"glide",0.25f},{"lforate",5.5f},{"spread",0} } },

    { "Taurus Bass", "Moog",
      { {"o1oct",-2},{"o2wave",0},{"o2semi",-24},{"o2fine",9},{"mix2",1},{"cutoff",300},{"reso",0.1f},
        {"envamt",0.25f},{"keytrack",0.2f},{"fa",0.005f},{"fd",0.8f},{"fs",0.4f},{"fr",0.5f},
        {"aa",0.005f},{"as",1},{"ar",0.5f},{"mono",1},{"spread",0} } },

    // ------------------------------------------------------------------ Sequential Prophet
    { "Prophet Brass", "Sequential Prophet",
      { {"o2wave",0},{"o2fine",5},{"mix2",1},{"cutoff",700},{"reso",0.15f},{"envamt",0.5f},
        {"fa",0.04f},{"fd",0.45f},{"fs",0.5f},{"fr",0.3f},{"aa",0.01f},{"as",1},{"ar",0.3f},{"spread",0.3f} } },

    { "Prophet Pad", "Sequential Prophet",
      { {"o2wave",1},{"pw",0.4f},{"pwm",0.25f},{"lforate",0.7f},{"o2fine",-6},{"mix2",0.8f},{"cutoff",1500},
        {"reso",0.2f},{"envamt",0.3f},{"fa",0.9f},{"fd",1.5f},{"fs",0.5f},{"fr",1.2f},{"aa",0.6f},{"as",1},
        {"ar",1.5f},{"spread",0.5f} } },

    { "Prophet Poly Keys", "Sequential Prophet",
      { {"o1wave",1},{"pw",0.3f},{"o2wave",0},{"o2fine",4},{"mix2",0.7f},{"cutoff",1200},{"reso",0.1f},
        {"envamt",0.4f},{"fa",0.001f},{"fd",0.6f},{"fs",0.2f},{"fr",0.4f},{"aa",0.001f},{"ad",1.5f},{"as",0.4f},
        {"ar",0.4f},{"spread",0.4f} } },

    // ------------------------------------------------------------------ Yamaha CS-80
    { "CS-80 Brass", "Yamaha CS-80",
      { {"o2wave",0},{"o2fine",7},{"mix2",1},{"unison",2},{"detune",14},{"spread",0.7f},{"fmode",1},
        {"cutoff",450},{"reso",0.15f},{"envamt",0.5f},{"fa",0.25f},{"fd",1.5f},{"fs",0.5f},{"fr",1.2f},
        {"aa",0.08f},{"as",1},{"ar",1.8f},{"lforate",5.5f},{"lfopitch",6} } },

    { "CS-80 Strings", "Yamaha CS-80",
      { {"o2wave",1},{"pw",0.45f},{"pwm",0.2f},{"lforate",0.9f},{"o2fine",9},{"mix2",0.8f},{"unison",2},
        {"detune",11},{"spread",0.8f},{"fmode",1},{"cutoff",3500},{"hpf",110},{"aa",0.45f},{"as",1},{"ar",1.4f} } },

    // ------------------------------------------------------------------ ARP
    { "Odyssey Sync Lead", "ARP",
      { {"sync",1},{"mix1",0},{"mix2",1},{"o2wave",0},{"o2semi",7},{"o2fine",20},{"cutoff",6000},{"reso",0.2f},
        {"mono",1},{"glide",0.1f},{"aa",0.003f},{"as",1},{"ar",0.3f},{"lforate",6.0f},{"chorus",2},{"chomix",0.35f},{"spread",0} } },

    { "Odyssey Square Lead", "ARP",
      { {"o1wave",1},{"pw",0.5f},{"cutoff",2200},{"reso",0.3f},{"envamt",0.2f},{"fa",0.005f},{"fd",0.4f},
        {"fs",0.5f},{"aa",0.005f},{"as",1},{"ar",0.2f},{"mono",1},{"glide",0.05f},{"spread",0} } },

    // ------------------------------------------------------------------ Roland mono
    { "SH-101 Bass", "Roland mono",
      { {"o1wave",1},{"pw",0.5f},{"o1oct",-1},{"sub",0.6f},{"cutoff",380},{"reso",0.35f},{"envamt",0.5f},
        {"keytrack",0.3f},{"fa",0.001f},{"fd",0.2f},{"fs",0.1f},{"fr",0.15f},{"aa",0.001f},{"as",1},{"ar",0.1f},
        {"mono",1},{"spread",0} } },

    { "SH-101 Lead", "Roland mono",
      { {"o1wave",0},{"sub",0.3f},{"cutoff",1600},{"reso",0.4f},{"envamt",0.3f},{"fa",0.001f},{"fd",0.35f},
        {"fs",0.4f},{"aa",0.001f},{"as",1},{"ar",0.2f},{"mono",1},{"glide",0.04f},{"spread",0} } },

    { "Acid Bass", "Roland mono",
      { {"o1oct",-1},{"cutoff",300},{"reso",0.8f},{"envamt",0.55f},{"keytrack",0.2f},{"fa",0.001f},{"fd",0.2f},
        {"fs",0},{"fr",0.15f},{"aa",0.001f},{"ad",0.5f},{"as",0.6f},{"ar",0.08f},{"mono",1},{"glide",0.05f},{"spread",0} } },

    // ------------------------------------------------------------------ Diversos
    { "PWM Pad", "Diversos",
      { {"o1wave",1},{"pwm",0.8f},{"lforate",0.5f},{"o2wave",1},{"o2fine",11},{"mix2",0.8f},{"cutoff",2200},
        {"reso",0.1f},{"aa",0.7f},{"as",1},{"ar",1.6f},{"chorus",1},{"chomix",0.5f},{"spread",0.6f} } },

    { "Organ", "Diversos",
      { {"o1wave",3},{"o2wave",3},{"o2semi",12},{"mix2",0.6f},{"sub",0.25f},{"cutoff",6000},
        {"aa",0.003f},{"as",1},{"ar",0.05f},{"chorus",2},{"chomix",0.5f},{"spread",0.2f} } },

    { "Hollow Flute", "Diversos",
      { {"o1wave",2},{"noise",0.06f},{"cutoff",2800},{"aa",0.07f},{"as",1},{"ar",0.25f},
        {"lforate",5.0f},{"lfopitch",5},{"mono",1},{"spread",0} } },

    { "Noise Sweep", "Diversos",
      { {"mix1",0},{"noise",1},{"cutoff",300},{"reso",0.6f},{"envamt",0.7f},{"keytrack",0},{"fa",3.0f},{"fd",3.0f},
        {"fs",0},{"fr",2.0f},{"aa",0.5f},{"as",1},{"ar",2.0f} } },

    { "Init (serra pura)", "Diversos", {} },
    };

    return list;
}

} // namespace drik
