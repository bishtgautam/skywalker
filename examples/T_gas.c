// ************************************************************************
// Skywalker: Copyright 2021, Cohere Consulting, LLC and
//            National Technology & Engineering Solutions of Sandia, LLC (NTESS)
//
// Copyright pending. Under provisional terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Sandia Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact Jeffrey Johnson (jeff@cohere-llc.com)
// ************************************************************************

#include <skywalker.h>

#include <string.h>
#include <tgmath.h>

void usage(const char *prog_name) {
  fprintf(stderr, "%s: usage:\n", prog_name);
  fprintf(stderr, "%s <input.yaml>\n", prog_name);
  exit(-1);
}

// Retrieves the value with the given name from the given input, exiting
// on failure.
double get_value(sw_input_t *input, const char *name) {
  sw_input_result_t in_result = sw_input_get(input, name);
  if (in_result.error_code != SW_SUCCESS) {
    fprintf(stderr, "T_gas: %s", in_result.error_message);
    exit(-1);
  }
  return in_result.value;
}

// Places the value with the given name into the given output, exiting
// on failure.
void put_value(sw_output_t *output, const char *name, double value) {
  sw_output_result_t out_result = sw_output_set(output, name, value);
  if (out_result.error_code != SW_SUCCESS) {
    fprintf(stderr, "T_gas: %s", out_result.error_message);
    exit(-1);
  }
}

// Determines the output_file name corresponding to the given name of the
// input file.
const char* output_file_name(const char *input_file) {
  static char output_file_[FILENAME_MAX];
  size_t dot_index = strlen(input_file);
  char *dot = strstr(input_file, ".");
  if (dot) {
    dot_index = (const char*)dot - input_file;
  }
  memcpy(output_file_, input_file, sizeof(char) * dot_index);
  memcpy(&output_file_[dot_index], ".py", sizeof(char) * 3);
  return (const char*)output_file_;
}

int main(int argc, char **argv) {

  if (argc == 1) {
    usage((const char*)argv[0]);
  }
  const char *input_file = argv[1];

  // Load the ensemble. Any error encountered is fatal.
  fprintf(stderr, "T_gas: Loading ensemble from %s\n", input_file);
  sw_ensemble_result_t load_result = sw_load_ensemble(input_file, NULL);
  if (load_result.error_code != SW_SUCCESS) {
    fprintf(stderr, "T_gas: %s", load_result.error_message);
    exit(-1);
  }

  // Ensemble data
  sw_ensemble_t *ensemble = load_result.ensemble;
  sw_input_t *input;
  sw_output_t *output;
  while (sw_ensemble_next(ensemble, &input, &output)) {
    // Fetch inputs.
    double V = get_value(input, "V"); // gas volume
    double p = get_value(input, "p"); // gas pressure

    double a = 0.0, b = 0.0;
    if (sw_input_has(input, "a")) {
      a = get_value(input, "a");
    }
    if (sw_input_has(input, "b")) {
      a = get_value(input, "b");
    }

    // Compute the gas temperature using the Van der Waals equation.
    static const double R = 8.31446261815324;
    double T = ((p - a/(V*V)) * (V - b)) / R;

    // Stash it.
    put_value(output, "T", T);
  }

  // Write out a Python module.
  const char *output_file = output_file_name(input_file);
  sw_ensemble_write(ensemble, output_file);

  // Clean up.
  sw_ensemble_free(ensemble);
}
