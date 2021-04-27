#pragma once
#include <fstream>
#include <thread>
#include <iostream>
#include <dsp/agc.h>
#include <dsp/fir_filter.h>
#include <dsp/fir_gen.h>
#include <dsp/costas_loop.h>
#include <dsp/clock_recovery_mm.h>
#include <dsp/dc_blocker.h>
#include <dsp/pipe.h>
#include "dsp/delay_one_imag.h"
#include <functional>
#define BUFFER_SIZE 4096

class QPSKDemodulatorDSP
{

private:
  std::string inputFilePath, outputFilePath;
  size_t data_in_filesize;
  std::ifstream data_in;
  std::ofstream data_out;
  bool aquaMode = false;
  bool hrptMode = false;
  bool f32, i16, i8, w8;
  float samplerate, symbolrate, rrc_alpha, rrc_taps, loop_bw;
  bool hard_symbs;
  bool dc_block;

public:
  void initDSP(
      std::function<void(int)> pProgressCallback,
      std::function<void(void)> pDoneCallback,
      std::function<void(int8_t, int8_t, int)> pConstellationCallback);
  void startDSP(
      std::string inputFilePath,
      std::string outputFilePath,
      bool optionF32,
      bool optionI16,
      bool optionI8,
      bool optionW8,
      int sampleRateEntry,
      int symbolRateEntry,
      float rrcAlphaEntry,
      float rrcTapsEntry,
      float loopBwEntry,
      bool hardSymbolsOption,
      bool dcBlockOption,
      bool aquaModeEntry,
      bool hrptModeEntry);

  void destroyDSP();

private:
  std::function<void(int)> progressCallback;
  std::function<void(void)> doneCallback;
  std::function<void(int8_t, int8_t, int)> constellationCallback;
  // All buffers we use along the way
  std::complex<float> *buffer;
  std::complex<float> *buffer2;
  std::complex<float> *buffer3;
  std::complex<float> *agc_buffer;
  std::complex<float> *agc_buffer2;
  std::complex<float> *filter_buffer;
  std::complex<float> *filter_buffer2;
  std::complex<float> *filter_buffer3;
  std::complex<float> *pll_buffer;
  std::complex<float> *recovery_buffer;
  std::complex<float> *recovered_buffer;
  // Int16 buffer
  int16_t *buffer_i16;
  // Int8 buffer
  int8_t *buffer_i8;
  // Uint8 buffer
  uint8_t *buffer_u8;

private:
  // All blocks used along the way
  libdsp::AgcCC *agc;
  libdsp::FIRFilterCCF *rrc_fir_filter;
  libdsp::CostasLoop *costas_loop;
  libdsp::ClockRecoveryMMCC *clock_recovery;
  libdsp::DCBlocker *dc_blocker;
  DelayOneImag *delay_one_imag;

private:
  // All FIFOs we use along the way
  libdsp::Pipe<std::complex<float>> *dc_pipe;
  libdsp::Pipe<std::complex<float>> *pre_agc_pipe;
  libdsp::Pipe<std::complex<float>> *post_agc_pipe;
  libdsp::Pipe<std::complex<float>> *pre_pll_pipe;
  libdsp::Pipe<std::complex<float>> *post_pll_pipe;
  libdsp::Pipe<std::complex<float>> *post_reco_pipe;

private:
  void fileThreadFunction();
  void agcThreadFunction();
  void rrcThreadFunction();
  void pllThreadFunction();
  void clockrecoveryThreadFunction();
  void dcBlockThreadFunction();
  void finalWriteThreadFunction();

private:
  std::thread *fileThread;
  std::thread *agcThread;
  std::thread *rrcThread;
  std::thread *pllThread;
  std::thread *clockrecoveryThread;
  std::thread *dcBlockThread;
  std::thread *finalWriteThread;
};
