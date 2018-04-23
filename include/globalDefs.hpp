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

/** Declarations used by each of the other modules
 */

#ifndef GLOBAL_DEFS
#define GLOBAL_DEFS

#include <cstdint>
#include <iostream>
#include <iterator>
#include <list>
#include <ios>
#include <string>
#include <vector>

namespace mawcd {
#define DEBUG

// TODO: Might try c-style fwrite() to fasten writing of binary files
using UINT_64 = uint64_t;
using UINT_32 = uint32_t;
using UINT_16 = uint16_t;
using UINT_8 = std::uint8_t;

using KEY_SIZE = UINT_8;   //< Length of the key to be used (should fit in a byte as it is no more than 32 currently)
using KEY_TYPE = uint32_t; //< data-type of the key; currently unsigned integer of (32-bits maximum)
const int cMax_key_size =
    32; //< Maximum size of key in the hash-table(anti-dictionary)
const int cNum_table = 2; //< Two hash tables in AD; one each for '0' and '1'

/** Path to the maw tool (which is generating MAWs) */
const std::string cPath_maw = "./external/maw-master/";

/** Constants defining various alphabets */
const std::string cPROTAlphabet = "ACDEFGHIKLMNPQRSTUVWY";
const std::string cDNAAlphabet = "ACGTN";

/** Constants defining various extensions of the output files */
const std::string cExt_com = ".com"; //< extension for compressed file
const std::string cExt_decom = ".decom"; //< extension for decompressed file


/** Enum for various possible states (success or errors) rsturned from a
 * function */
enum class ReturnStatus {
  SUCCESS,
  ERR_ARGS,
  ERR_FILE_OPEN,
  ERR_INVALID_INPUT,
  ERR_INVALID_INDEX,
  ERR_LIMIT_EXCEEDS,
  ERR_EXTERNAL,
  HELP
};

/** Enum to define various alphabet that can be used .
* DNA : ACGT
* PROT: ACDEFGHIKLMNPQRSTUVWY
* GEN: All graphical or space characters
* SEL: User given case-sensitive alphabet
*/
enum class AlphabetType { DNA, PROT, GEN, SEL };

/** Various modes of operation of the tools.
 * AD: Creating anti-dictionary
 * COM: Compression
 * DECOM: Decompression
 * BCOM: Batch compression
 * BDECOM: Batch decompression
 */
enum class Mode { AD, COM, DECOM, BCOM, BDECOM };

/** Types for the internal representation (encoded) of the sequence.
 * */
/** Assumes alphabet letters are encoded in 0s and 1s ; each of which is
* represented by an 8 bit char
*/
using SEQUENCE = std::string; //< encoded sequence
using SUBSEQUENCE =
    SEQUENCE; //< subsequence or substring is the same type as that of sequence
using ENCODED_CHAR =
    std::string; //< a character is encoded as a string of 0s and 1s

/** Type for the encoded seuquence  when bits are packed into bytes
*/
using PACKED_SEQUENCE = std::vector<UINT_8>; //< internal representation of the encoded sequence
const UINT_8 cByte_Size = 8;


} // end namespace

#endif
