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

/** Defines the class Anti_dictionary.
 * It assumes alphabet (for internal representation) to be binary: 0 and 1.
 */

#ifndef ANTIDICTIONARY_HPP
#define ANTIDICTIONARY_HPP

#include <cmath>
#include <cstdlib>
#include <math.h>
#include <numeric>
#include <sstream>
#include <unordered_set>
// NOTE: SDSL SA Construction doesn,t work with 0 as a character
#include <sdsl/suffix_arrays.hpp>

#include "Parser.hpp"
#include "globalDefs.hpp"
#include "utilDefs.hpp"
namespace mawcd {

// TODO: Handle endian-ness
class Anti_dictionary {

public:
  /** @brief Creates anti-dictionary in the form of two hash-tables (aka sets): READS
  ONE SEQUENCE
   * For a single sequence, following is the format:
   * Two hash-tables - ad_0 and ad_1: One for (encoded) maws ending with '0';
  other for those ending in '1'
   * Since, key is same as value, internally it is unordered_map which
  corresponds to sets.
   * For each maw of length k, we are saving its prefix of length (k-1).
   * The integer corresponding to k-1 bits is stored in the correspondonding
  table:
   *  If the last bit of this maw is 0, it is saved in ad_0; otherwise ad_1

   * The type of the key of hash-tables is KEY_TYPE (see globalDefs.h).
  Currently
  it is 32-bits unsigned int.
   *
   * Input file is assumed to be in Fasta Format (requirement for maw-tool).
   *  - It calls maw-tool to generate a temporary file (deleted at the end).
   *  - Choses the length of the key to be used from these maws (@see function
  choose_maw_size).
   *  - Goes through the file again to load (select, encode, and insert into
  corresponding hash-table) the maws of chosen length.
   *
   * @param filename name of the file containing the sequence with respect to
  which the anti-dictionary will be created.
   * @param parser reference to the Parser instance given for encoding/decoding
  the sequence to/from internal representation.
   *
   * @return execution status // SUCCESS if input is valid, otherwise
  corresponding error code after logging the error.
  */
  ReturnStatus create(const std::string filename, const Parser &parser);

  /** @brief Reads AD in text format (corresponding to one sequence).
  * First-line gives the length of keys used;
  * newline-seperated keys.
  * Empty line demarcation between keys corresponding to 0 and 1.
  */
  ReturnStatus read(std::ifstream &adfile);

  /** @brief Writes AD in text format (corresponding to one sequence).
  * First-line gives the length of keys used;
  * newline-seperated keys.
  * Empty line demarcation between keys corresponding to 0 and 1.
  */
  ReturnStatus write(std::ofstream &adfile) const;

  /** @brief Reading anti-dictionary in the binary format: READS ONE SEQUENCE
   * For a single sequence, following is the format:
   * First one byte: 0: Actual Key_size: from 1 to 32 (space used by each key is
   * given by KEY_SIZE)
   * Next one byte: 1: number of keys of one byte in ad_0
   * Next one byte: 2: number of keys of one byte in ad_1
   * Next two bytes: 3 and 4: number of keys of two bytes in ad_0
   * Next two bytes: 5 and 6: number of keys of two bytes in ad_1
   * Next four bytes: 7 to 10: number of keys of four bytes in ad_0
   * Next four bytes: 11 to 14: number of keys of four bytes in ad_1
   * From there on, the keys of ad_0 start: keys in ad_0 of size 1, then size 2,
   * then 3, and then 4 (size in bytes, number as above)
   * Following them are the keys for ad_1: keys in ad_1 of size 1, then size 2,
   * then 3, and then 4 (size in bytes, number as above)
   *
   * The same repeats for all the sequences.
   */
  ReturnStatus read_binary(std::ifstream &adfile);

  /** @brief Writing anti-dictionary in the binary format: WRITES ONE SEQUENCE
   * For a single sequence, following is the format:
   * First one byte: 0: Actual Key_size: from 1 to 32 (space used by each key is
   * given by KEY_SIZE)
   * Next one byte: 1: number of keys of one byte in ad_0
   * Next one byte: 2: number of keys of one byte in ad_1
   * Next two bytes: 3 and 4: number of keys of two bytes in ad_0
   * Next two bytes: 5 and 6: number of keys of two bytes in ad_1
   * Next four bytes: 7 to 10: number of keys of four bytes in ad_0
   * Next four bytes: 11 to 14: number of keys of four bytes in ad_1
   * From there on, the keys of ad_0 start: keys in ad_0 of size 1, then size 2,
   * then 3, and then 4 (size in bytes, number as above)
   * Following them are the keys for ad_1: keys in ad_1 of size 1, then size 2,
   * then 3, and then 4 (size in bytes, number as above)
   *
   * The same repeats for all the sequences.
  */
  ReturnStatus write_binary(std::ofstream &adfile) const;

  /** @brief Infers the next character following a given suffix (key).
   * If it finds the key in ad_0, returns true and '1' in letter
   * If it finds the key in ad_1, returns true and '0' in letter
   * Otherwise returns false
   */
  bool find_following_letter(const KEY_TYPE &key, char &letter) const;

  /** @brief Finds the size of the key.
 */
  int get_key_size() const;

  /** @brief Finds the size of the anti-dictionary (# keys stored in total)
 */
  int get_ad_size() const;

  /** @brief Prints the Anti_dictionary in human-readable form.
   */
  void print() const;

  //////////////////////// private ////////////////////////
private:
  // Assumes a binary alphabet
  // anti-dictionary corresponding to each of the binary alphabet: 0 and 1
  std::vector<std::unordered_set<KEY_TYPE>> _ad =
      std::vector<std::unordered_set<KEY_TYPE>>(cNum_table);

  int _ad_size; //< total size (number of maws) in all the ad

  KEY_SIZE _key_size; // length of the keys (longest prefix of the chosen maws)
                      // in ad; Same for all ad.

  /** @brief Chooses the length of the maws to be stored in the anti-dictionary
   * and
   * store l-1 as the key-size.
   * It finds the length that gives maximum (positive) number of compressed bits
   * - The frequency or number of occurrences of the longest proper prefix of
   * that maw (say #occs_m) is same as the bits compressed.
   * - But the maw itself will have to be stored in the AD. Thus effective
   * compression is (#occs_m - 1) for a maw m (because it takes constant # of
   * bytes).
   * - For a specific length l, compression corresponding to the maws of that
   * length = Summation(#occs_m - 1) over all maws m of length l
   * - Thus we choose the length that maximizes c = (freq[l] - num_maws[l])
   * If there are more than one l that gives the chosen c, we use the minimum
   * such l.
   *  If there is no such l that gives a positive c, we choose the shortest
   * length of maws.
   * We use the Compressed Suffix Array to find the number of occurrences of the
   * longest prefix of a maw.
   *
   * @param seqfilename name of the file containing sequence (only seuquence for
   * SDSL)
   * @param mawfilename name of the file containing the maws of the sequence
   * @param max_maw_size length of the longest possible maw
   * */
  ReturnStatus choose_maw_size(const std::string &seqfilename,
                               const std::string &mawfilename,
                               const int max_maw_size);

  /** @brief Loads ad_0 and ad_1 with the corresponding prefixes of the
(encoded) maws
   of selected size.
   * Selects all the maws of length key-size+1 and maps them to binary
   representation.
   * For each maw of length k, save its prefix of length (k-1)
   * The integer corresponding to k-1 bits is stored in the correspondonding
    table:
   *  If the last bit of this maw is 0, it is saved in ad_0; otherwise ad_1

   * The type of the key of hash-tables is KEY_TYPE (@see KEY_TYPE).
   Currently, it is 32-bits unsigned int.
   *
   * @param filename name of the file containing the maws of the sequence
   * @param parser reference to the Parser instance given for encoding maws
   * */

  ReturnStatus load_chosen_maws(const std::string &filename,
                                const Parser &parser);

  /** @brief Reads the sequence file and creates its Compressed Suffix Array:
   * SINGLE SEQUENCE
   * Uses SDSL.
   * */
  ReturnStatus create_seq_csa(const std::string &filename,
                              sdsl::csa_bitcompressed<> &csa);
};

} // end namespace
#endif
