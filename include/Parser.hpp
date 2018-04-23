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

/** @file Parser.hpp
 * @brief Defines the class Parser.
 * It provides methods for parsing files and sequences.
 */

#ifndef PARSER_HPP
#define PARSER_HPP

#include <algorithm>
#include <bitset>
#include <cctype>
#include <clocale>
#include <cmath>
#include <fstream>
#include <functional>

#include <sys/stat.h>
//#include <sys/types.h>

#include "globalDefs.hpp"
#include "utilDefs.hpp"

namespace mawcd {
/** Class Parser
 * A Parser contains methods for parsing a sequence  (file or string) to/from
 * encoded (or packed-encoded) sequence.
 * - A sequence is string of characters from the corresponding (original)
 * alphabet.
 * - An encoded sequence is a string of encoded characters:
 *   -- An encoded character is a string of '0' and '1' (characters)
 * - A packed-encoded sequence is when encoded sequence (string of '1' and '0')
 * is converted and into corresponding bits and the bits are packed as bytes.
 *
 * Note that encoding/decoding here refers to converting to/from the internal
 * representation.
 */
class Parser {
  /** Function Pointers for checking validity of a character, encoding an
   * orginal alphabet character, and decoding an encoded character
   * */
  using FCheckValidity = bool (Parser::*)(const char) const;
  using FMapChar = ENCODED_CHAR (Parser::*)(const char) const;
  using FRevMapChar = char (Parser::*)(const ENCODED_CHAR) const;

public:
  /** @brief Constructs the parser (for specialised alphabet).
   *
   * @param alphabetType Alphabet type: DNA for genomic sequences, PROT for
   proteins. SEL for user-defined.
   * @param alphabet string containing all the letters of the chosen alphabet.

   * @see AlphabetType
   * @see cDegenerate_PROTAlphabet
   * @see cDegenerate_DNAAlphabet

   */
  Parser(const AlphabetType alphabetType, const std::string &alphabet);

  /** @brief Constructs the parser for general (graphical and characters)
   * alphabet.
   *
   * @see AlphabetType
   */
  Parser();

  /** @brief Parses the string (containing single sequence in original alphabet)
     into an encoded sequence. It ignores space-characters and new-lines for the
     specialised alphabet (DNA, Protein, user-defined(SEL))
     *
     * @param str reference to the string (sequence in original alphabet) to be
     encoded
     * @param sequence reference in which encoded sequence will be stored.
     * @see AlphabetType

     * @return execution status // SUCCESS if input is valid, otherwise
     corresponding error code after logging the error

     */
  ReturnStatus encode_from_string(const std::string &str,
                                  SEQUENCE &sequence) const;

  /** @brief Packs the encoded string (of '0' and '1') into a sequence of bytes.
   *
   * The last bits of the sequence may not fill the whole byte. Those
   (hanging) bits are returned in the 'pvs_hanging' to be used with the next
   call (for the next block/part of the sequence)
   *
   * Similarly, the part of the sequence not utilised in the previous call
   (hanging bits from the last call) are given in 'pvs_hanging' which is
   prepended (conceptually) to this sequence before packing.
   *
   * If only pvs_hanging is given (sequence is empty), it is padded with zeroes
   to make a full byte and returned as the packed sequence.
   *
   * @param sequence reference to the encoded sequence to be packed.
   * @param pvs_hanging reference to the string containing remaining bits (last
   bits which could not fill a byte) from the previous call (previous part of
   the sequence). Remaining bits of this part of the sequence will be returned
   in it.
   * @param packed_sequence reference in which packed encoded sequence will be
   stored.
   *
   * @return execution status // SUCCESS if input is valid, otherwise
     corresponding error code after logging the error.

     */
  ReturnStatus pack_sequence(const SEQUENCE &seq, std::string &pvs_hanging,
                             PACKED_SEQUENCE &packed_sequence) const;

  /** @brief Decodes the encoded string (of '0' and '1') into the corresponding
   sequence of characters from original alphabet.
   *
   * The last bits of the encoded sequence may not be enough to decipher the
   reverse_mapping (fewer than the size of an encoded character). Those
   (hanging) bits are returned in the 'pvs_hanging' to be used with the next
   call (for the next block/part of the sequence)
   *
   * Similarly, the bits of the sequence remaining (hanging) from the previous
   part (hanging bits from the last call) are given in 'pvs_hanging' which are
   prepended (conceptually) to this sequence before deciphering (collecting bits
   equal to the encoded character size).
   *
   * @param sequence reference to the encoded sequence to be decoded.
   * @param pvs_hanging reference to the string containing remaining bits (last
   bits which were not sufficient to decipher the encoded char) from the
   previous call (previous part of the sequence). Remaining such bits of this
   part of the sequence will be returned in it.
   * @param str reference to string in which decoded sequence will be
   stored.
   *
   * @return execution status // SUCCESS if input is valid, otherwise
     corresponding error code after logging the error.

     */
  ReturnStatus decode_to_string(const SEQUENCE &sequence,
                                std::string &pvs_hanging,
                                std::string &str) const;

  /** @brief Maps the given sequence of binary characters to that consisting
   * of DNA alphabet.
   * Assumes that the string will always have valid characters ('0' or '1').
   * One to one mapping from  '0' to 'A' and '1' to 'C'.
   * @return mapped sting
   */
  std::string binary_to_dna(const std::string &str) const;

  /** @brief Maps the given sequence of DNA alphabet characters ('A' and 'C')
   * to that consisting of binary ('0' and '1').
   * Assumes that the string will always have valid characters ('A' or 'C').
   * One to one mapping from  'A' to '0' and 'C' to '1'.
   * @return mapped sting
   */
  std::string dna_to_binary(const std::string &str) const;

private:
  const AlphabetType _cAlphabetType; //< Type of alphabet: DNA, PROT, SEL or GEN
  const std::string
      _cAlphabet; //< Original alphabet (string of valid characters)
  // Assumes alphabet size to be not more than 2^8-1 (i.e. 255)
  const int _cOriginal_alphbet_size; // Size of the original alphabet: s
  const int _cEncoded_char_len; // log_2 s (for specialised alphabet) or 8 (for
                                // general alphabet)

  /** Checks the validity of the given character.
   * Tests whether the character is graphical or space character.
   * */
  bool is_valid_char_general(const char c) const;

  /** @brief Maps some character from the alphabet to corresponding encoded
   * representation (string of '0' and '1').
   * Assumes will always be a valid character.
   */
  ENCODED_CHAR map_char_general(const char c) const;

  /** @brief Maps the encoded representation to the corresponding character in
   * the alphabet.
   * Assumes will always be a valid encoded character.
   * Considers the given encoded character to be the binary representation of
   * the corresponding character.
   */
  char reverse_map_char_general(const ENCODED_CHAR c) const;

  /** Checks the validity of the given character.
   * Tries to find the character in the alphabet string .
    * */
  bool is_valid_char_select(const char c) const;

  /** @brief Maps some character from the alphabet to corresponding encoded
   * representation.
   * Assumes will always be a valid character.
   * Uses the binary representation of the corresponding position of the
   * character in the alphabet.
   */
  ENCODED_CHAR map_char_select(const char c) const;

  /** @brief Maps the encoded representation to the corresponding character in
   * the alphabet.
   * Assumes will always be a valid encoded character.
   * Considers the given encoded character to be the binary representation of
   * some position in the alphabet and maps it to the corresponding character.
   */
  char reverse_map_char_select(const ENCODED_CHAR c) const;

  /** Checks the validity of the given DNA character.
   * Hard-coded.
   * */
  bool is_valid_char_dna(const char c) const;

  /** @brief Maps the DNA character to corresponding encoded representation.
   * Assumes will always be a valid character.
   */
  ENCODED_CHAR map_char_dna(const char c) const;

  /** @brief Maps the encoded representation to the corresponding DNA character.
   * Assumes will always be a valid encoded character.
   */
  char reverse_map_char_dna(const ENCODED_CHAR c) const;

  /** Checks the validity of the given PROT character.
     * Hard-coded.
     * */
  bool is_valid_char_prot(const char c) const;

  /** @brief Maps the PROT character to corresponding encoded representation.
     * Assumes will always be a valid character.
     */
  ENCODED_CHAR map_char_prot(const char c) const;

  /** @brief Maps the encoded representation to the corresponding PROT
   * character.
   * Assumes will always be a valid encoded character.
   */
  char reverse_map_char_prot(const ENCODED_CHAR c) const;
};

} // end namespace
#endif
