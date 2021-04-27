#include <iostream>
#include <getopt.h>
#include <unistd.h>
#include <string.h>
#include "dsp.h"
#include <functional>

using namespace std;

bool doneProcessing = 0;

enum presets
{
  m2lrpt = 0,
  metopahrpt,
  fy3bahrpt,
  fy3cahrpt,
  fy3bmpt,
  fy3dahrpt,
  nppjpsshrd,
  aquadb
};

void usage(FILE *fp, const char *path)
{
  /* take only the last portion of the path */
  const char *basename = strrchr(path, '/');
  basename = basename ? basename + 1 : path;

  fprintf(fp, "usage: %s [OPTION]\n", basename);
  fprintf(fp, "  -h, --help\t\t"
              "Print this help and exit.\n");
  fprintf(fp, "  -i FILENAME, --input FILENAME \t"
              "Input file, - for STDIN (defaults to -)\n");
  fprintf(fp, "  -o FILENAME, --output FILENAME\t"
              "Output file, - for STDOUT (defaults to -)\n");

  fprintf(fp, "  -s SAMPLERATE, --samplereate SAMPLERATE\t"
              "Input sample rate. Defaults to 6000000 (6MS/s).\n");
  fprintf(fp, "  -p PRESET, --preset PRESET (required)\t"
              "Satellite preset. One of: m2lrpt metopahrpt fy3bahrpt fy3cahrpt fy3bmpt fy3dahrpt nppjpsshrd aquadb'\n");
}

void done()
{
  std::cerr << "Finished\n\n";
  doneProcessing = 1;
}

void progress(int pct)
{
  std::cerr << pct << "                     \r";
}

int main(int argc, char **argv)
{
  int c;
  int help_flag = 0;
  char infile[256] = "-";
  char outfile[256] = "-";
  int preset = -1;
  int samplerate = 6000000;
  while (1)
  {
    int this_option_optind = optind ? optind : 1;
    int option_index = 0;
    static struct option long_options[] = {
        {"input", required_argument, NULL, 'i'},
        {"output", required_argument, NULL, 'o'},
        {"preset", required_argument, NULL, 'p'},
        {"samplerate", required_argument, NULL, 's'},
        {"help", no_argument, &help_flag, 1},
        {0, 0, 0, 0}};

    c = getopt_long(argc, argv, "s:i:o:p:h",
                    long_options, &option_index);
    if (c == -1)
      break;

    switch (c)
    {
    case 'h':
      help_flag = 1;
      break;

    case 'i':
      strncpy(infile, optarg ? optarg : "-", sizeof(infile));
      /* strncpy does not fully guarantee null-termination */
      infile[sizeof(infile) - 1] = '\0';
      break;

    case 'o':
      strncpy(outfile, optarg ? optarg : "-", sizeof(outfile));
      /* strncpy does not fully guarantee null-termination */
      outfile[sizeof(outfile) - 1] = '\0';
      break;

    case 's':
      samplerate = atoi(optarg);
      break;

    case 'p':
      if (strcmp(optarg, "m2lrpt") == 0)
      {
        preset = m2lrpt;
        break;
      }
      else if (strcmp(optarg, "metopahrpt") == 0)
      {
        preset = metopahrpt;
        break;
      }
      else if (strcmp(optarg, "fy3bahrpt") == 0)
      {
        preset = fy3bahrpt;
        break;
      }
      else if (strcmp(optarg, "fy3cahrpt") == 0)
      {
        preset = fy3cahrpt;
        break;
      }
      else if (strcmp(optarg, "fy3bmpt") == 0)
      {
        preset = fy3bmpt;
        break;
      }
      else if (strcmp(optarg, "fy3dahrpt") == 0)
      {
        preset = fy3dahrpt;
        break;
      }
      else if (strcmp(optarg, "nppjpsshrd") == 0)
      {
        preset = nppjpsshrd;
        break;
      }
      else if (strcmp(optarg, "aquadb") == 0)
      {
        preset = aquadb;
        break;
      }
      else
      {
        //don't break, fall through
      }

    default:
      fprintf(stderr, "Invalid arguments specified\n");
      help_flag = 1;
    }
  }

  if (help_flag || preset < 0)
  {
    usage(stderr, argv[0]);
    exit(EXIT_FAILURE);
  }
  fprintf(stderr, "Input: %s\n", infile);
  fprintf(stderr, "Output: %s\n", outfile);
  fprintf(stderr, "Preset: %d\n\n", preset);
  fprintf(stderr, "Samplerate: %d\n\n", samplerate);

  auto dsp = new QPSKDemodulatorDSP();
  dsp->initDSP(progress, done, NULL);

  bool optionF32 = 0;
  bool optionI16 = 1;
  bool optionI8 = 0;
  bool optionW8 = 0;
  int sampleRateEntry = 6000000;
  int symbolRateEntry = 2800000;
  float rrcAlphaEntry = 0.5;
  float rrcTapsEntry = 361;
  float loopBwEntry = 0.005;
  bool hardSymbolsOption = 0;
  bool dcBlockOption = 0;
  bool aquaModeEntry = 0;
  bool hrptModeEntry = 0;

  switch (preset)
  {
  case m2lrpt:
    symbolRateEntry = 72000;
    rrcTapsEntry = 511;
    hrptModeEntry = 1;
    break;
  case metopahrpt:
    symbolRateEntry = 2333333;
    rrcTapsEntry = 31;
    loopBwEntry = 0.002;
    hrptModeEntry = 1;
    break;
  case fy3bahrpt:
    symbolRateEntry = 2800000;
    rrcTapsEntry = 31;
    loopBwEntry = 0.002;
    hrptModeEntry = 1;
    break;
  case fy3cahrpt:
    symbolRateEntry = 2600000;
    rrcAlphaEntry = 0.5;
    rrcTapsEntry = 31;
    loopBwEntry = 0.002;
    hrptModeEntry = 1;
    break;
  case fy3bmpt:
    symbolRateEntry = 18700000;
    rrcAlphaEntry = 0.35;
    rrcTapsEntry = 31;
    loopBwEntry = 0.0063;
    break;
  case fy3dahrpt:
    symbolRateEntry = 30000000;
    rrcAlphaEntry = 0.35;
    rrcTapsEntry = 31;
    loopBwEntry = 0.0063;
    break;
  case nppjpsshrd:
    symbolRateEntry = 15000000;
    rrcAlphaEntry = 0.7;
    rrcTapsEntry = 31;
    loopBwEntry = 0.001;
    break;
  case aquadb:
    symbolRateEntry = 7500000;
    rrcTapsEntry = 31;
    loopBwEntry = 0.006;
    hardSymbolsOption = 0;
    dcBlockOption = 1;
    aquaModeEntry = 1;
    break;
  default:
    exit(EXIT_FAILURE);
  }

  dsp->startDSP(std::string(infile),
                std::string(outfile),
                optionF32,
                optionI16,
                optionI8,
                optionW8,
                sampleRateEntry,
                symbolRateEntry,
                rrcAlphaEntry,
                rrcTapsEntry,
                loopBwEntry,
                hardSymbolsOption,
                dcBlockOption,
                aquaModeEntry,
                hrptModeEntry);

  while (!doneProcessing)
  {
    usleep(100000);
  }
  exit(EXIT_SUCCESS);
}
