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

/** Defines the class Codec.
 * It contains the method for comrpression and decomprssion of a file and a
 * sequence.
 */

#ifndef CODEC_HPP
#define CODEC_HPP

#include "Anti_dictionary.hpp"
#include "Parser.hpp"
#include "globalDefs.hpp"

namespace mawcd {
/** Class Codec
 * A Codec contains methods for compressing and decompressing a file or
 * sequence.
 * - It makes use of the Parser instance given to encode/decode sequences
 * to/from internal representation.
 * - It is tied to an Anti_dictionary which it uses for
 * compression/decompression.
 * - It reads the files in blocks (of 1MB currently).
 *
 */
class Codec {
public:
  /** @brief Constructs the codec (that uses the given anti_dictionary).
   *
   * @param ad Anti_dictionary which will be used for
   * compression/decompression.
   *
   */
  Codec(const Anti_dictionary &ad);

  /** @brief Compresses the given input file and save it in the corresponding
   * output file.
   *
   *
   * Input file is read in blocks (currently 1MB).
   * - Each block is encoded, compressed, packed, and stored in output file.
   *
   * Output file (compressed) : same name as that of input file with an added
   * extension cExt_decom ('.com' currently).
   * Compressed File Format (binary):
   * - First 4 bytes represent the length of the original sequence.
   * - Following which are compressed encoded sequence (of '0' and '1') packed
   * into bytes.
   *
   * @param parser reference to the Parser instance given for encoding/decoding
   * blocks of the sequence to/from internal representation.
   * @param in_filename name of the input file. Save the compressed file in
   * <in_filename><cExt_com>
   *
   * @return execution status // SUCCESS if input is valid, otherwise
   * corresponding error code after logging the error. 
   *
   * @see cExt_com
   */
  ReturnStatus compress_file(const Parser &parser,
                             const std::string &in_filename) const;

  /** @brief Decompresses the given input file and save it in the corresponding
   * output file.
   *
   *
   * Input file (assumed to be in compressed format) is read in blocks
   * (currently 1MB).
   * - Each block is decompressed, decoded, and stored in output file.
   * Decompression is done of the packed byte sequence (without exapnding into
   * binary string. Thus memory efficient.)
   * Input (Compressed) File Format (binary) is assumed to be as follows:
   * - First 4 bytes represent the length of the original sequence.
   * - Following which are compressed encoded sequence (of '0' and '1') packed
   * into bytes.
   *
   * Output file (decompressed) : same name as that of input file with an added
   * extension cExt_decom ('.decom' currently).
   *
   * @param parser reference to the Parser instance given for encoding/decoding
   * blocks of the sequence to/from internal representation.
   * @param in_filename name of the input file. Save the decompressed file in
   * <in_filename><cExt_decom>
   *
   * @return execution status // SUCCESS if input is valid, otherwise
   * corresponding error code after logging the error.   *
   *
   * @see cExt_decom
   */
  ReturnStatus decompress_file(const Parser &parser,
                               const std::string &in_filename) const;

  /** @brief Compresses the encoded string (of '0' and '1').
   *
   * If the first block is to be compressed (indicated by is_initial), initial
   * bits corresponding to the length of the key (of anti_dictionary) are copied
   * in the compressed sequence as is.
   * After that, the collected suffix (key) is used to infer the character from
   * the anti_dictionary.
   * - If it can be inferred, it is skipped, otherwise it is copied.
   * If it is the not the first block, suffix of the end of the previous block
   * (given as pvs_suffix) is used in the beginning.
   * - The suffix at the end of this block will also be returned in pvs_suffix.
   *
   * @param seq reference to the (block of) encoded sequence to be compressed.
   * @param is_initial boolean representing if it is the initial (first) block.
   * @param pvs_suffix reference to the suffix (key) from the previous block.
   * The last suffix of this block will also be returned in it.
   *
   * @return the compressed sequence (corresponding to this block).
   */
  SEQUENCE compress(const SEQUENCE &seq, bool is_initial,
                    KEY_TYPE &pvs_suffix) const;

  /** @brief Decompresses the packed encoded string.
   *
   * If the first block is to be compressed (indicated by is_initial), initial
   * bits corresponding to the length of the key (of anti_dictionary) are copied
   * in the decompressed sequence as is.
   * After that, the collected suffix (key) is used to infer the character from
   * the anti_dictionary.
   * - If it can be inferred, it is added, otherwise we move on.
   * If it is the not the first block, suffix of the end of the previous block
   * (given as pvs_suffix) is used in the beginning.
   * - The suffix at the end of this block will also be returned in pvs_suffix.
   *
   * @param n size of the original (encoded) sequence.
   * @param comp_seq reference to the (block of) compressed (packed and encoded)
   * sequence to be decompressed.
   * @param is_initial boolean representing if it is the initial (first) block.
   * @param pvs_suffix reference to the suffix (key) from the previous block.
   * The last suffix of this block will also be returned in it.
   *
   * @return the decompressed sequence (corresponding to this block).
   */
  SEQUENCE decompress(const UINT_64 n, const PACKED_SEQUENCE &comp_seq,
                      bool is_initial, KEY_TYPE &pvs_suffix) const;

  //////////////////////// private ////////////////////////
private:
  /** reference to the anti_dictionary that will be used for
   * compressing/decompressing */
  const Anti_dictionary &_cAd;
  /** length of the suffix (key) used for the inference of the next character
   * from the anti-dictionary */
  const int _cSuff_len;
};

} // end namespace
#endif
