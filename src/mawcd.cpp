/**
   mawcd: Codec based on Minimal Absent Words
   Copyright (C) 2017 Ritu Kundu, Panagiotis Charalampopoulos, and Solon P.
Pissis
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

/** Module containing main() method.
 */

#include <cstdlib>
#include <fstream>

#include "../include/Anti_dictionary.hpp"
#include "../include/Codec.hpp"
#include "../include/Parser.hpp"
#include "../include/globalDefs.hpp"
#include "../include/utilDefs.hpp"

/** Module containing main() method.
 */

using namespace mawcd;
/** Function to create anti-dictionary.
 * */
ReturnStatus create_ad(const Parser &parser, const std::string &in_filename,
                       const std::string &ad_filename);

// TODO: Take care of codes not in the actual alphabet
int main(int argc, char **argv) {

  /* Decode arguments */
  struct InputFlags flags;
  if (decodeFlags(argc, argv, flags) != ReturnStatus::SUCCESS) {
    usage();
    return 1;
  }

  /* Create Parser */
  std::string alphabet;
  if (flags.alphabet_type == AlphabetType::DNA) { // DNA
    alphabet = cDNAAlphabet;
  } else if (flags.alphabet_type == AlphabetType::PROT) { // protien
    alphabet = cPROTAlphabet;
  } else if (flags.alphabet_type == AlphabetType::SEL) { // user-defined
    alphabet = flags.selected_alphabet;
  }
  const Parser &parser = (flags.alphabet_type == AlphabetType::GEN)
                             ? (Parser())
                             : (Parser(flags.alphabet_type, alphabet));

  if (flags.mode == Mode::AD) { // Create Anti-dictionary
    create_ad(parser, flags.input_filename, flags.anti_dictionary_filename);
  } else { // compression or decompression
    ReturnStatus status;
    /* Read and initialise Anti-dictionary */
    std::string filename = flags.anti_dictionary_filename;
    std::ifstream adfile(filename, std::ios::binary);
    if (!adfile.is_open()) {
      std::cerr << "Cannot open anti-dictionary file \n";
      return static_cast<int>(ReturnStatus::ERR_FILE_OPEN);
    }
    Anti_dictionary ad{};
    // Initialise AD
    status = ad.read_binary(adfile);
    if (status != ReturnStatus::SUCCESS) {
      return static_cast<int>(status);
    }
#ifdef VERBOSE
    ad.print();
#endif

    /* Create Codec */
    Codec codec(ad);

    if (flags.mode == Mode::COM) { // Compress single file
      status = codec.compress_file(parser, flags.input_filename);
      if (status != ReturnStatus::SUCCESS) {
        return static_cast<int>(status);
      }
    } else if (flags.mode == Mode::DECOM) { // batch compress
      status = codec.decompress_file(parser, flags.input_filename);
      if (status != ReturnStatus::SUCCESS) {
        return static_cast<int>(status);
      }
    } else {
      /* Extract names of the files to be compressed (new line separated) from
       * the input file */

      /* Read Input file */
      std::string filename = flags.input_filename;
      std::ifstream infile(filename);
      if (!infile.is_open()) {
        std::cerr << "Cannot open input file " << filename << " \n";
        return static_cast<int>(ReturnStatus::ERR_FILE_OPEN);
      }
      /* Set the appropriate function to call */
      ReturnStatus (Codec::*fp_file)(const Parser &parser,
                                     const std::string &in_filename) const;
      if (flags.mode == Mode::BCOM) { // Compress single file
        fp_file = &Codec::compress_file;
      } else { // batch compress
        fp_file = &Codec::decompress_file;
      }
      std::string line;
      while (std::getline(infile, line)) {
        if (!line.empty()) {
          status = (codec.*fp_file)(parser, line);
          if (status != ReturnStatus::SUCCESS) {
            return static_cast<int>(status);
          }
        } // one line done
      }   // file read
    }     // batch processing ended
  }       // com/decom ended
}
/** @brief Creates the anti-dictionary of the sequence in the given input file
 * and save it in the given output file.
 *
 * Input file:
 * - Read each line, encodes it, converts into DNA alphabet (0 to A and 1 to
 * C) (as required by maw-tool).
 * - This temporary file (deleted at the end) is given to create
 * Anti_dictionary.
 * - The Anti-dictionary is then saved as the output file.
 *
 * @param parser reference to the Parser instance given for encoding the
 * sequence.
 * @param in_filename name of the input file containing the sequence.
 * @param ad_filename name of the output filein which the anti-dictionary will
 * be saved.
 *
 * @return execution status // SUCCESS if input is valid, otherwise
 * corresponding error code after logging the error.
 *
 */
ReturnStatus create_ad(const Parser &parser, const std::string &in_filename,
                       const std::string &ad_filename) {
  ReturnStatus status;
  /* Open AD file */
  std::ofstream adfile(ad_filename, std::ios::binary);
  if (!adfile.is_open()) {
    std::cerr << "Cannot create anti-dictionary file " << ad_filename << " \n";
    return ReturnStatus::ERR_FILE_OPEN;
  }

  /* Read Input file */
  std::ifstream infile(in_filename);
  if (!infile.is_open()) {
    std::cerr << "Cannot open input file " << in_filename << " \n";
    return ReturnStatus::ERR_FILE_OPEN;
  }
  std::string line;
  // Get the sequence
  std::getline(infile, line);
  if (line.empty()) {
    std::cerr << "No Input: Empty File: " << std::endl;
    return ReturnStatus::ERR_INVALID_INPUT;
  }
  /* Open temporary file to be used by maw tool */
  std::string temp_seq_file = "temp_seq_file";
  std::ofstream tempfile(temp_seq_file);
  tempfile << "> dummy" << std::endl;
  int lineNum = 0;
  do {
    ++lineNum;
    std::string seq_name;
    std::string seq_value;
    if (!line.empty()) {
      line.shrink_to_fit();
      // std::cout << "LINE Read: " << line <<std::endl;
      /* Encode the line */
      SEQUENCE en_sequence;
      parser.encode_from_string(line, en_sequence);
      // std::cout << "ENCODED Read: " << en_sequence <<std::endl;
      /* Transform the sequence to DNA (required for maw tool) */
      auto dna_seq = parser.binary_to_dna(en_sequence);
      // std::cout << "DNA Read: " << dna_seq <<std::endl;
      tempfile << dna_seq << std::endl;
    }
  } while (std::getline(infile, line)); // sequence ends
  Anti_dictionary ad{};
  status = ad.create(temp_seq_file, parser);
  if (status != ReturnStatus::SUCCESS) {
    return status;
  }
  // delete temp temp file
  remove(temp_seq_file.c_str());
  /* Save the anti-dictionary */
  ad.write_binary(adfile);

  std::cout << "AD created successfully: " << std::endl;
  return ReturnStatus::SUCCESS;
}
