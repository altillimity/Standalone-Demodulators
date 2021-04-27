#include <iostream>
#include <getopt.h>
#include <unistd.h>
#include <cstring>
#include "dsp.h"
#include <functional>

using namespace std;

bool doneProcessing = 0;

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
              "Satellite preset. Either 'noaa' or 'meteor'\n");
}

void done()
{
  std::cerr << "Finished\n\n";
  doneProcessing = 1;
}

void progress(size_t c, size_t t, size_t fc)
{
  static int n=0;
  n++;
  if(n==100) {
  std::cerr << c << "/" << t << " - " << fc << "                     \r"; // Changed
  n=0;
  }
}

int main(int argc, char **argv)
{
  int c;
  int help_flag = 0;
  char infile[256] = "-";
  char outfile[256] = "-";
  char preset = -1;
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
      if (strcmp(optarg, "noaa") == 0)
      {
        preset = 0;
        break;
      }
      else if (strcmp(optarg, "meteor") == 0)
      {
        preset = 1;
        break;
      }
      //dont break, do fall through

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

  auto dsp = new CBPSKDemodulatorDsp();
  dsp->initDSP(progress, done, NULL);

  bool optionF32 = 0;
  bool optionI16 = 1;
  bool optionI8 = 0;
  bool optionW8 = 0;

  int symbolrateEntry = 665400;
  float rrcAlphaEntry = 0.5;
  float rrcTapsEntry = 31;
  bool noaaDeframerOption = true;
  bool frontRRCOption = false;

  if (preset == 1)
  {
    rrcAlphaEntry = 0.4;
    noaaDeframerOption = false;
    frontRRCOption = true;
  }

  dsp->startDSP(std::string(infile),
                std::string(outfile),
                optionF32,
                optionI16,
                optionI8,
                optionW8,
                samplerate,
                symbolrateEntry,
                rrcAlphaEntry,
                rrcTapsEntry,
                noaaDeframerOption,
                frontRRCOption);

  while (!doneProcessing)
  {
    usleep(100000);
  }
  exit(EXIT_SUCCESS);
}
