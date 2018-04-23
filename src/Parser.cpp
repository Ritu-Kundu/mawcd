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

/** Implements class Parser
 */
#include "../include/Parser.hpp"

namespace mawcd {

Parser::Parser(const AlphabetType alphabetType, const std::string &alphabet)
    : _cAlphabetType(alphabetType), _cAlphabet(alphabet),
      _cOriginal_alphbet_size(alphabet.size()),
      _cEncoded_char_len(static_cast<int>(ceil(log2(alphabet.size())))) {}

Parser::Parser()
    : _cAlphabetType(AlphabetType::GEN), _cAlphabet(""),
      _cOriginal_alphbet_size(255), _cEncoded_char_len(8) {}

ReturnStatus Parser::encode_from_string(const std::string &str,
                                        SEQUENCE &sequence) const {
  FCheckValidity fCheckValidity = &Parser::is_valid_char_general;
  FMapChar fMapChar = &Parser::map_char_general;

  if (_cAlphabetType == AlphabetType::DNA) {
    fCheckValidity = &Parser::is_valid_char_dna;
    fMapChar = &Parser::map_char_dna;
  } else if (_cAlphabetType == AlphabetType::PROT) {
    fCheckValidity = &Parser::is_valid_char_prot;
    fMapChar = &Parser::map_char_prot;
  } else if (_cAlphabetType == AlphabetType::SEL) {
    fCheckValidity = &Parser::is_valid_char_select;
    fMapChar = &Parser::map_char_select;
  }

  sequence.reserve(str.size() * _cEncoded_char_len);

  // Get the encoded string
  for (char c : str) {
    if (_cAlphabetType != AlphabetType::GEN && isspace(c)) {
      // Ignore
    } else if ((this->*fCheckValidity)(c)) {
      sequence += (this->*fMapChar)(c);
    } else {
      std::cerr << "Invalid Input: Invalid character: " << c << std::endl;
      return ReturnStatus::ERR_INVALID_INPUT;
    }
  }
  return ReturnStatus::SUCCESS;
}

ReturnStatus Parser::pack_sequence(const SEQUENCE &seq,
                                   std::string &pvs_hanging,
                                   PACKED_SEQUENCE &packed_sequence) const {

  auto seq_len = seq.size();
  int seq_start_index = 0;

  UINT_8 c;
  /* Adjust last hanging bits from the last chunk */
  if (!pvs_hanging.empty()) {
    int num_haging_bits = pvs_hanging.size();
    int remaining_bits = cByte_Size - num_haging_bits;
    seq_start_index = remaining_bits;

    /* fill the remaining */
    std::string pack;
    if (seq.empty()) { // pack only the hanging bits padding with zeros
      std::string zeroes(remaining_bits, '0');
      pack = pvs_hanging + zeroes;
#ifdef VERBOSE
      std::cout << "Padded pack: " << pack << std::endl;
#endif
    } else { // pack the hanging bts with those from the sequence
      pack = pvs_hanging + seq.substr(0, remaining_bits);
    }

    if (pack.size() < cByte_Size) { // unfilled byte
      pvs_hanging.assign(pack);
    } else {                  // add to pack
      pvs_hanging.assign(""); // pvs_hanging used. nothing left
      c = static_cast<UINT_8>(std::stoi(pack, nullptr,
                                        2)); // change to int of base 2
      packed_sequence.push_back(c); // add the valid last byte with the new
#ifdef VERBOSE
      std::cout << "Adding the hanging: " << std::bitset<8>(c) << std::endl;
#endif
    }
  } // last hanging bits from the previous chunk handled
  if (seq_len > seq_start_index) { // sequence is left
    auto bits_to_pack = (seq_len - seq_start_index);
    auto max_bytes_in_pack = (bits_to_pack / cByte_Size) +
                             1; // 1 (possibly) for last bytes from last chunk
    packed_sequence.reserve(max_bytes_in_pack);

    for (auto ind = seq_start_index; ind < seq_len; ind += cByte_Size) {
      std::string pack = seq.substr(ind, cByte_Size);
      if (pack.size() < cByte_Size) { // unfilled byte
        pvs_hanging.assign(pack);
      } else { // add to pack
        int i = (std::stoi(
            pack, nullptr,
            2)); // change to int of base 2 and keep only the last byte
        c = static_cast<UINT_8>(i);
#ifdef VERBOSE
        std::cout << "CONVERTED: " << std::bitset<8>(c) << std::endl;
#endif
        packed_sequence.push_back(c); // add the valid last byte with the new
      }
    }
  }

  return ReturnStatus::SUCCESS;
}

// Assumes string to have valid characters
// Ideally in the last chunk pvs_remaining will produce nothing
ReturnStatus Parser::decode_to_string(const SEQUENCE &sequence,
                                      std::string &pvs_hanging,
                                      std::string &str) const {
  FRevMapChar fRevMapChar = &Parser::reverse_map_char_general;

  if (_cAlphabetType == AlphabetType::DNA) {
    fRevMapChar = &Parser::reverse_map_char_dna;
  } else if (_cAlphabetType == AlphabetType::PROT) {
    fRevMapChar = &Parser::reverse_map_char_prot;
  } else if (_cAlphabetType == AlphabetType::SEL) {
    fRevMapChar = &Parser::reverse_map_char_select;
  }
  int start_ind = 0;
  ENCODED_CHAR c;
  if (!pvs_hanging.empty()) { // something carried on from the previous chunk
    int already_filled = pvs_hanging.size();
    int unfilled = _cEncoded_char_len - already_filled;
    c = pvs_hanging + sequence.substr(0, unfilled);
    if (c.size() == _cEncoded_char_len) { // Add to string if can be decoded
      str += (this->*fRevMapChar)(c);
      pvs_hanging.assign("");
    } else {
      pvs_hanging.assign(c);
    }
    start_ind = unfilled;
  }
  auto str_size = (sequence.size() - start_ind) / _cEncoded_char_len;
  str.reserve(str_size + 1); // +1 for the possible pvs
  for (auto i = start_ind; i < sequence.size(); i += _cEncoded_char_len) {
    c = sequence.substr(i, _cEncoded_char_len);
    if (c.size() == _cEncoded_char_len) { // Add to string if can be decoded
      str += (this->*fRevMapChar)(c);
    } else {
      pvs_hanging.assign(c);
    }
  }
  return ReturnStatus::SUCCESS;
}

std::string Parser::binary_to_dna(const std::string &str) const {
  std::string dna_seq;
  dna_seq.resize(str.size());

  std::transform(str.begin(), str.end(), dna_seq.begin(),
                 [](char c) { return (c == '0') ? ('A') : ('C'); });
  return dna_seq;
}

std::string Parser::dna_to_binary(const std::string &str) const {
  std::string bin_seq;
  bin_seq.resize(str.size());
  std::transform(str.begin(), str.end(), bin_seq.begin(),
                 [](char c) { return (c == 'A') ? ('0') : ('1'); });
  return bin_seq;
}

//////////////////////// private ////////////////////////

bool Parser::is_valid_char_general(const char c) const {
  return static_cast<bool>(std::isprint(c));
  return true;
}

// Assumes will always be a valid character
ENCODED_CHAR Parser::map_char_general(const char c) const {
  int pos = static_cast<int>(c);
  // std::cout << "INT CHAR : c i: " << c << " " << pos << std::endl;
  std::string str = (std::bitset<8>(pos)).to_string();
  ENCODED_CHAR coded_char = str;
  return coded_char;
}

char Parser::reverse_map_char_general(const ENCODED_CHAR c) const {
  int index = std::stoi(c, nullptr, 2); // change to int of base 2
  char decoded_char = static_cast<char>(index);
  return decoded_char;
}

bool Parser::is_valid_char_select(const char c) const {
  auto pos = _cAlphabet.find(c);
  if (pos != std::string::npos) {
    return true;
  }
  return false;
}

// Assumes will always be a valid character
ENCODED_CHAR Parser::map_char_select(const char c) const {
  int pos = _cAlphabet.find(c);
  std::string str = (std::bitset<8>(pos)).to_string();
  ENCODED_CHAR coded_char = str.substr(8 - _cEncoded_char_len);
  return coded_char;
}

char Parser::reverse_map_char_select(const ENCODED_CHAR c) const {
  int index = std::stoi(c, nullptr, 2); // change to int of base 2
  char decoded_char = _cAlphabet[index];
  return decoded_char;
}

bool Parser::is_valid_char_dna(const char c) const {
  bool result = false;
  switch (std::toupper(c)) {
  case 'A':
  case 'C':
  case 'G':
  case 'T':
  case 'N':
    result = true;
  }
  return result;
}

// Assumes will always be a valid character
ENCODED_CHAR Parser::map_char_dna(const char c) const {
  ENCODED_CHAR result;
  switch (std::toupper(c)) {
  case 'A':
    result = "000";
    break;
  case 'C':
    result = "001";
    break;
  case 'G':
    result = "010";
    break;
  case 'T':
    result = "011";
    break;
  case 'N':
    result = "100";
    break;
  }
  return result;
}

char Parser::reverse_map_char_dna(const ENCODED_CHAR c) const {
  char result;
  if (c == "000") {
    result = 'A';
  } else if (c == "001") {
    result = 'C';
  } else if (c == "010") {
    result = 'G';
  } else if (c == "011") {
    result = 'T';
  } else if (c == "100") {
    result = 'N';
  }
  return result;
}

bool Parser::is_valid_char_prot(const char c) const {
  bool result = false;
  switch (std::toupper(c)) {
  case 'A':
  case 'C':
  case 'D':
  case 'E':
  case 'F':
  case 'G':
  case 'H':
  case 'I':
  case 'K':
  case 'L':
  case 'M':
  case 'N':
  case 'O':
  case 'P':
  case 'Q':
  case 'R':
  case 'S':
  case 'T':
  case 'U':
  case 'V':
  case 'W':
  case 'Y':
    result = true;
    break;
  }
  return result;
}

ENCODED_CHAR Parser::map_char_prot(const char c) const {
  ENCODED_CHAR result;
  switch (std::toupper(c)) {
  case 'A':
    result = "00000";
    break;
  case 'C':
    result = "00001";
    break;
  case 'D':
    result = "00010";
    break;
  case 'E':
    result = "00011";
    break;
  case 'F':
    result = "00100";
    break;
  case 'G':
    result = "00101";
    break;
  case 'H':
    result = "00110";
    break;
  case 'I':
    result = "00111";
    break;
  case 'K':
    result = "01000";
    break;
  case 'L':
    result = "01001";
    break;
  case 'M':
    result = "01010";
    break;
  case 'N':
    result = "01011";
    break;
  case 'O':
    result = "01100";
    break;
  case 'P':
    result = "01101";
    break;
  case 'Q':
    result = "01110";
    break;
  case 'R':
    result = "01111";
    break;
  case 'S':
    result = "10000";
    break;
  case 'T':
    result = "10001";
    break;
  case 'U':
    result = "10010";
    break;
  case 'V':
    result = "10011";
    break;
  case 'W':
    result = "10100";
    break;
  case 'Y':
    result = "10101";
    break;
  }
  return result;
}

// TODO: Take care of codes not in the actual alphabet
char Parser::reverse_map_char_prot(const ENCODED_CHAR c) const {
  char result;
  if (c == "00000") {
    result = 'A';
  } else if (c == "00001") {
    result = 'C';
  } else if (c == "00010") {
    result = 'D';
  } else if (c == "00011") {
    result = 'E';
  } else if (c == "00100") {
    result = 'F';
  } else if (c == "00101") {
    result = 'G';
  } else if (c == "00110") {
    result = 'H';
  } else if (c == "00111") {
    result = 'I';
  } else if (c == "01000") {
    result = 'K';
  } else if (c == "01001") {
    result = 'L';
  } else if (c == "01010") {
    result = 'M';
  } else if (c == "01011") {
    result = 'N';
  } else if (c == "01100") {
    result = 'O';
  } else if (c == "01101") {
    result = 'P';
  } else if (c == "01110") {
    result = 'Q';
  } else if (c == "01111") {
    result = 'R';
  } else if (c == "10000") {
    result = 'S';
  } else if (c == "10001") {
    result = 'T';
  } else if (c == "10010") {
    result = 'U';
  } else if (c == "10011") {
    result = 'V';
  } else if (c == "10100") {
    result = 'W';
  } else if (c == "10101") {
    result = 'Y';
  }

  return result;
}

} // end namespace
