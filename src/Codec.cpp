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

/** Implements class Codec
 */
#include "../include/Codec.hpp"

namespace mawcd {
Codec::Codec(const Anti_dictionary &ad)
    : _cAd(ad), _cSuff_len(ad.get_key_size()) {}

ReturnStatus Codec::compress_file(const Parser &parser,
                                  const std::string &in_filename) const {
  /* Open input file */
  std::ifstream infile(in_filename, std::ios::binary);
  std::cout << "################ Compressing file: " << in_filename << std::endl;
  if (!infile.is_open()) {
    std::cerr << "Cannot open file to be compressed " << in_filename << " \n";
    return ReturnStatus::ERR_FILE_OPEN;
  }
  /* Open output file (with extension .com) */
  std::string out_filename(in_filename + cExt_com);
  std::ofstream outfile(out_filename, std::ios::binary);
  if (!outfile.is_open()) {
    std::cerr << "Cannot open output file " << out_filename << " \n";
    return ReturnStatus::ERR_FILE_OPEN;
  }
  UINT_64 orig_seq_size = 0;
  /* Write length of the original sequence */
  // First 4 bytes represent the length of the original sequence
  // Write dummy to rewrite the correct value at the end
  outfile.write((char *)(&orig_seq_size), sizeof(orig_seq_size));

  /* Preapare to read fil in chunks */
  // find file size
  struct stat filestatus;
  stat(in_filename.c_str(), &filestatus);

  std::size_t totalSize = filestatus.st_size;
  constexpr std::size_t bufferSize =1024 * 1024 * 1024; // Read in chunks of
  // 1MB
  
  std::size_t totalChunks = totalSize / bufferSize;
  std::size_t last_chunk_size = totalSize % bufferSize;
  if (last_chunk_size != 0) {
    ++totalChunks;
  } else {
    last_chunk_size = bufferSize;
  }
  // Flag to indicate whether the first chunk
  bool is_initial = true;  // First chunk is initials
  KEY_TYPE pvs_suffix = 0; // suffix from previous chunk (initially 0)
  // Part of packed representation remained hanging from the previous chunk
  std::string pvs_hanging = ""; // initially empty

  /* Start reading file in chunks */
  for (std::size_t chunk = 0; chunk < totalChunks; ++chunk) {
    std::size_t this_chunk_size = bufferSize;
    if (chunk == totalChunks - 1) { // last chunk
      this_chunk_size = last_chunk_size;
    }
#ifdef VERBOSE
    std::cout << "BUFFER SIZE: " << this_chunk_size << std::endl;
#endif
    std::string buffer(this_chunk_size, 0);
    /* Read the chunk in buffer */
    infile.read(&buffer[0], this_chunk_size);
#ifdef VERBOSE
    std::cout << "BUFFER str: " << buffer << std::endl;
#endif
    /* Encode data in buffer */
    SEQUENCE encoded_sequence;
    auto result = parser.encode_from_string(buffer, encoded_sequence);
    orig_seq_size += encoded_sequence.size();
    if (result != ReturnStatus::SUCCESS) {
      return result;
    }
#ifdef VERBOSE
    std::cout << "ENCODED str: " << encoded_sequence << std::endl;
#endif
    /* Compress the sequence */
    SEQUENCE compressed_seq =
        compress(encoded_sequence, is_initial, pvs_suffix);
    if (is_initial) { // turn the flag off for the other chunks than the first
      is_initial = false;
    }
#ifdef VERBOSE
    std::cout << "COMPRESSED str: " << compressed_seq << std::endl;
#endif
    /* Pack the sequence */
    PACKED_SEQUENCE packed;
    parser.pack_sequence(compressed_seq, pvs_hanging, packed);
#ifdef VERBOSE
    std::cout << "PACKED size: " << packed.size() << "\n ";

    for (auto p : packed) {
      std::cout << "PACKED str: "
                << " " << std::hex << (int)p << " ";
    }
    std::cout << "\n pvs_hanging: " << pvs_hanging << std::endl;
#endif
    /* Write to output */
    outfile.write((char *)packed.data(),
                  packed.size()); // leave out last hanging bits
  }

  /* Write the hanging bits from the last chunk (if any) */
  if (!pvs_hanging.empty()) { // pad hanging bits with zeroes
    /* Pack the sequence */
    PACKED_SEQUENCE packed;
    parser.pack_sequence("", pvs_hanging, packed);
    outfile.write((char *)packed.data(), packed.size());
  }

  // First 4 bytes represent the length of the original sequence
  // ReWrite dummy written in the beginning with the correct value
  outfile.seekp(std::ios::beg);
  outfile.write((char *)(&orig_seq_size), sizeof(orig_seq_size));

  std::cout << "File compressed successfully: " << in_filename << std::endl;
  return ReturnStatus::SUCCESS;
}

ReturnStatus Codec::decompress_file(const Parser &parser,
                                    const std::string &in_filename) const {
  /* Open input file  */
  std::ifstream infile(in_filename, std::ios::binary);
  std::cout << "################ Decompressing file: " << in_filename << std::endl;
  if (!infile.is_open()) {
    std::cerr << "Cannot open input file " << in_filename << " \n";
    return ReturnStatus::ERR_FILE_OPEN;
  }
  /* Open output file (with extension .decom) */
  std::string out_filename(in_filename + cExt_decom);
  std::ofstream outfile(out_filename);
  if (!infile.is_open()) {
    std::cerr << "Cannot open output file " << out_filename << " \n";
    return ReturnStatus::ERR_FILE_OPEN;
  }
  /* Read length of the original sequence */
  UINT_64 orig_seq_size = 0;
  // First 4 bytes represent the length of the original sequence
  infile.read((char *)(&orig_seq_size), sizeof(orig_seq_size));
#ifdef VERBOSE
  std::cout << "Len of original string: " << orig_seq_size << std::endl;
#endif

  /* Preapare to read file in chunks */
  // find file size
  struct stat filestatus;
  stat(in_filename.c_str(), &filestatus);
  // FExclude the first bytes represent length of the original size
  std::size_t totalSize = filestatus.st_size - sizeof(orig_seq_size);
  constexpr std::size_t bufferSize = 1024 * 1024 * 1024; // Read in chunks of
  // 1MB
  std::size_t totalChunks = totalSize / bufferSize;
  std::size_t last_chunk_size = totalSize % bufferSize;
  if (last_chunk_size != 0) {
    ++totalChunks;
  } else {
    last_chunk_size = bufferSize;
  }

  // Flag to indicate whether the first chunk
  bool is_initial = true;  // First chunk is initials
  KEY_TYPE pvs_suffix = 0; // suffix from previous chunk (initially 0)
  // Part of encoded representation of a character remained hanging from the
  // previous chunk
  std::string pvs_remaining = ""; // initially empty

  for (std::size_t chunk = 0; chunk < totalChunks; ++chunk) {
    std::size_t this_chunk_size = bufferSize;
    if (chunk == totalChunks - 1) { // last chunk
      this_chunk_size = last_chunk_size;
    }
    std::vector<UINT_8> buffer(this_chunk_size, 0);
    /* Read the chunk in buffer */
    infile.read((char *)buffer.data(), this_chunk_size);
#ifdef VERBOSE
    std::cout << "Read # of bytes: " << buffer.size() << std::endl;
#endif

    /* Decompress data in buffer */
    SEQUENCE decompressed_seq =
        decompress(orig_seq_size, buffer, is_initial, pvs_suffix);

    orig_seq_size -= decompressed_seq.size();
    if (is_initial) { // turn the flag off for the other chunks than the first
      is_initial = false;
    }
#ifdef VERBOSE
    std::cout << "DECOMPRESSED str: " << decompressed_seq << std::endl;
#endif
    /* Decode the decompressed sequence */
    std::string decoded_str;
    parser.decode_to_string(decompressed_seq, pvs_remaining, decoded_str);
#ifdef VERBOSE
    std::cout << "DECODED str: " << decoded_str << std::endl;
#endif
    /* Save in the file (except possibly the last character) */
    outfile.write(decoded_str.data(), decoded_str.size());
  }

  std::cout << "File decompressed successfully: " << in_filename << std::endl;
  return ReturnStatus::SUCCESS;
}

// pvs_suffix is seq initially
SEQUENCE Codec::compress(const SEQUENCE &seq, bool is_initial,
                         KEY_TYPE &pvs_suffix) const {
#ifdef VERBOSE
  std::cout << "Compression starts." << seq << std::endl;
#endif
  auto n = seq.size();
  SEQUENCE compressed_seq;
  KEY_TYPE suffix = pvs_suffix;
  KEY_TYPE mask = ~((~1) << (_cSuff_len - 1));
  int start_ind = 0;
  if (is_initial) {
    start_ind = _cSuff_len;
    // handle until the suffix is collected
    for (auto i = 0; i < _cSuff_len && i < n; ++i) {
      suffix = suffix << 1;
      if (seq[i] == '1') {
        suffix = suffix | 1;
      }
      compressed_seq.push_back(seq[i]);
#ifdef VERBOSE
      std::cout << "Initial : Added: " << seq[i] << std::endl;
#endif
    }
  }
#ifdef VERBOSE
  std::cout << "Suffix collected: " << std::bitset<cMax_key_size>(suffix)
            << std::endl;
#endif
  for (auto i = start_ind; i < n; ++i) {
    char following_char;
    // test if the current char can be figured out from the ad
    if (_cAd.find_following_letter(
            suffix, following_char)) { // found the key => following char
// Do nothing => compress current symbol
#ifdef VERBOSE
      std::cout << "Skipped: \n";
#endif
    } else {
      compressed_seq.push_back(seq[i]);
#ifdef VERBOSE
      std::cout << "Rem : Added: " << seq[i] << std::endl;
#endif
    }
    suffix = suffix << 1;
    if (seq[i] == '1') {
      suffix = suffix | 1;
    }
    suffix = suffix & mask;
#ifdef VERBOSE
    std::cout << "New Suffix collected: " << std::bitset<cMax_key_size>(suffix)
              << std::endl;
#endif
  }
  pvs_suffix = suffix; // save suffix for the next chunk
#ifdef VERBOSE
  std::cout << "Pvs Suffix returned: " << std::bitset<cMax_key_size>(suffix)
            << std::endl;
#endif
  compressed_seq.shrink_to_fit();
  return compressed_seq;
}

SEQUENCE Codec::decompress(const UINT_64 n,
                           const PACKED_SEQUENCE &comp_packed_seq,
                           bool is_initial, KEY_TYPE &pvs_suffix) const {
  SEQUENCE seq;
  seq.reserve(n);
  auto comp_packed_seq_len = comp_packed_seq.size();

  KEY_TYPE suff_mask = ~((~1) << (_cSuff_len - 1));
  KEY_TYPE suffix = pvs_suffix;

  int curr_byte_ind = 0;
  UINT_8 curr_byte = comp_packed_seq[curr_byte_ind];
  int curr_bit_ind = 0;
  UINT_8 byte_mask = 0x80;
  int start_ind = 0;
  bool is_hit_end = false;
#ifdef VERBOSE
  std::cout << "BYTE MASK: " << std::bitset<cMax_key_size>(byte_mask)
            << std::endl;
#endif
  if (is_initial) {
    start_ind = _cSuff_len;
    // handle until the suffix is collected
    for (auto i = 0; i < _cSuff_len && i < n; ++i) {
#ifdef VERBOSE
      std::cout << "Initial : byte num: bt num: byte: " << curr_byte_ind
                << " : " << curr_bit_ind << " : "
                << std::bitset<cMax_key_size>(curr_byte) << std::endl;
#endif
      suffix = suffix << 1;
      if (curr_byte & byte_mask) { // bit is 1
        seq.push_back('1');
        suffix = suffix | 1;
#ifdef VERBOSE
        std::cout << "Added 1\n";
#endif
      } else { // bit is 0
        seq.push_back('0');
#ifdef VERBOSE
        std::cout << "Added 0\n";
#endif
      }
      curr_byte = curr_byte << 1;
#ifdef VERBOSE
      std::cout << "Suffix collected: " << std::bitset<cMax_key_size>(suffix)
                << std::endl;
#endif
      if ((++curr_bit_ind) == cByte_Size) { // this byte done
        curr_bit_ind = 0;
        ++curr_byte_ind;
        if (curr_byte_ind < comp_packed_seq_len) {
          curr_byte = comp_packed_seq[curr_byte_ind];
        } else {
          is_hit_end = true;
        }
      }
    }
  }
  bool isCharAdded = true;
  for (auto i = start_ind; i < n; ++i) {
    char following_char;
    // test if the current char can be figured out from the ad
    if (_cAd.find_following_letter(
            suffix, following_char)) { // found the key => following char
      seq.push_back(following_char);
#ifdef VERBOSE
      std::cout << "Following: Added " << following_char << std::endl;
      ;
#endif
    } else {            // try to extract character from the com_seq
      if (is_hit_end) { // if it has already been exhausted
        isCharAdded = false;
        break;
      } else {
#ifdef VERBOSE
        std::cout << "Rem : byte num: bt num: byte: " << curr_byte_ind << " : "
                  << curr_bit_ind << " : " << std::bitset<8>(curr_byte)
                  << std::endl;
#endif
        if (curr_byte & byte_mask) { // bit is 1
          seq.push_back('1');
          #ifdef VERBOSE
          std::cout << "Added 1\n";
          #endif
        } else { // bit is 1
          seq.push_back('0');
          #ifdef VERBOSE
          std::cout << "Added 0\n";
          #endif
        }
        curr_byte = curr_byte << 1;
        if ((++curr_bit_ind) == cByte_Size) { // this byte done
          curr_bit_ind = 0;
          ++curr_byte_ind;
          if (curr_byte_ind < comp_packed_seq_len) {
            curr_byte = comp_packed_seq[curr_byte_ind];
          } else {
            is_hit_end = true;
          }
        }
      }
    }
    if (isCharAdded) { // we added something in the sequence
      suffix = suffix << 1;
      if (seq[i] == '1') {
        suffix = suffix | 1;
      }
      suffix = suffix & suff_mask;
#ifdef VERBOSE
      std::cout << "Suffix collected: " << std::bitset<cMax_key_size>(suffix)
                << std::endl;
#endif
    }
  }
  pvs_suffix = suffix;
  return seq;
}
} // end namespace
