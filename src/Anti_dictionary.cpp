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

/** Implements class Anti_dictionary
 */
#include "../include/Anti_dictionary.hpp"

namespace mawcd {

ReturnStatus Anti_dictionary::create(const std::string filename,
                                     const Parser &parser) {
  /* Call maw tool to create output file containing maws of size from k to K */
  std::string temp_output_file = "tmp_out_" + filename;

  const std::string k = "2";
  // int max_maw_size = static_cast<int>(std::ceil(log2(seq_len + 1))); // Max
  // MAW size can be log (s+1)
  int max_maw_size = cMax_key_size; // Currently 32
  std::string K = std::to_string(max_maw_size);
  std::string cmd = cPath_maw + "maw -a DNA -i " + filename + " -o " +
                    temp_output_file + " -k " + k + " -K " + K;

  if (system(cmd.c_str()) != 0) {
    std::cerr << "Could not run MAW Tool \n";
    return ReturnStatus::ERR_EXTERNAL;
  }

  /* Choose the length of the maws */
  choose_maw_size(filename, temp_output_file, max_maw_size);
  /* Read the output (maws) and store them in hash-table after encoding them */
  load_chosen_maws(temp_output_file, parser);
  /* Delete temp temp file */
  remove(temp_output_file.c_str());

  std::cout << "Anti-dictionary created successfully of size: " << _ad_size
            << std::endl;
  return ReturnStatus::SUCCESS;
}

ReturnStatus Anti_dictionary::read(std::ifstream &adfile) {

  std::string line;
  int letter = 0;
  // First line gives the key size;
  getline(adfile, line);
  if (line.empty()) {
    std::cerr << "Invalid Input: Empty Anti-dictionary File: " << std::endl;
    return ReturnStatus::ERR_INVALID_INPUT;
  }
  _key_size = std::stoi(line); // key is only the first k-1 bits
  /* Read the keys and store them in hash-table */
  while (getline(adfile, line)) {
    if (line.empty()) {
      ++letter; // empty line => keys corresponding to next letter
      if (letter == cNum_table) {
        break;
      }
    } else {
      KEY_TYPE key = static_cast<KEY_TYPE>(std::stoi(line));
      _ad[letter].insert(key);
      ++_ad_size;
    }
  }
  if (letter < cNum_table) {
    std::cerr << "Invalid Input: Anti-dictionary File does not have hash-table "
                 "corresponding to 0 and 1: "
              << std::endl;
    return ReturnStatus::ERR_INVALID_INPUT;
  }
  std::cout << "Anti-dictionary created successfully of size: " << _ad_size
            << std::endl;
  return ReturnStatus::SUCCESS;
}

ReturnStatus Anti_dictionary::write(std::ofstream &adfile) const {
  // First line gives the key size;
  adfile << _key_size << std::endl;
  for (auto i = 0; i < cNum_table; ++i) {
    for (auto k : _ad[i]) {
      adfile << k << std::endl;
    }
    adfile << std::endl;
  }
  std::cout << "Anti-dictionary saved successfully. " << std::endl;
  return ReturnStatus::SUCCESS;
}

ReturnStatus Anti_dictionary::read_binary(std::ifstream &adfile) {

  std::vector<std::vector<UINT_8>> keys_1B{{}, {}};
  std::vector<std::vector<UINT_16>> keys_2B{{}, {}};
  std::vector<std::vector<UINT_32>> keys_4B{{}, {}};

  /* Start reading */
  // Actual key length
  adfile.read((char *)(&_key_size), sizeof(_key_size));

  // Number of keys of length 1B in ad_0, then in ad_1; folllowed by info for
  // 2B; then 4B
  std::vector<std::vector<UINT_32>> num_keys{{}, {}};
  for (auto ad = 0; ad < cNum_table; ++ad) {
    UINT_8 num;
    adfile.read((char *)(&num), sizeof(num));
    num_keys[ad].push_back(num);
  }
  for (auto ad = 0; ad < cNum_table; ++ad) {
    UINT_16 num;
    adfile.read((char *)(&num), sizeof(num));
    num_keys[ad].push_back(num);
  }
  for (auto ad = 0; ad < cNum_table; ++ad) {
    UINT_32 num;
    adfile.read((char *)(&num), sizeof(num));
    num_keys[ad].push_back(num);
  }

  // Read keys of length 1B, 2B, 4B in ad_0, then in ad_1;
  for (auto ad = 0; ad < cNum_table; ++ad) {
    keys_1B[ad].resize(num_keys[ad][0]);
    keys_2B[ad].resize(num_keys[ad][1]);
    keys_4B[ad].resize(num_keys[ad][2]);
    adfile.read((char *)(keys_1B[ad].data()), num_keys[ad][0] * sizeof(UINT_8));
    adfile.read((char *)(keys_2B[ad].data()),
                num_keys[ad][1] * sizeof(UINT_16));
    adfile.read((char *)(keys_4B[ad].data()),
                num_keys[ad][2] * sizeof(UINT_32));
  }
  // Read keys of length 1B, 2B, 4B in ad_0, then in ad_1;
  for (auto ad = 0; ad < cNum_table; ++ad) {
    for (auto k : keys_1B[ad]) {
      KEY_TYPE key = static_cast<KEY_TYPE>(k);
      _ad[ad].insert(key);
      ++_ad_size;
    }
    for (auto k : keys_2B[ad]) {
      KEY_TYPE key = static_cast<KEY_TYPE>(k);
      _ad[ad].insert(key);
      ++_ad_size;
    }
    for (auto k : keys_4B[ad]) {
      KEY_TYPE key = static_cast<KEY_TYPE>(k);
      _ad[ad].insert(key);
      ++_ad_size;
    }
  }
  std::cout << "Anti-dictionary created successfully of size: " << _ad_size
            << std::endl;
  return ReturnStatus::SUCCESS;
}

ReturnStatus Anti_dictionary::write_binary(std::ofstream &adfile) const {
  // Highest unsigned ints of size (in Bytes) 1, 2, 4
  std::vector<UINT_64> thresholds = {0xff, 0xffff, 0xffffffff};

  std::vector<std::vector<UINT_8>> keys_1B{{}, {}};
  std::vector<std::vector<UINT_16>> keys_2B{{}, {}};
  std::vector<std::vector<UINT_32>> keys_4B{{}, {}};
  // check the number of bytes of each key in each ad and add in corrsponding
  // vector
  for (auto ad = 0; ad < cNum_table; ++ad) {
    for (auto i : _ad[ad]) {
      if (i <= thresholds[0]) {
        keys_1B[ad].push_back(static_cast<UINT_8>(i));
      } else if (i <= thresholds[1]) {
        keys_2B[ad].push_back(static_cast<UINT_16>(i));
      } else if (i <= thresholds[2]) {
        keys_4B[ad].push_back(static_cast<UINT_32>(i));
      }
    }
  }
  /* Start writing */
  // Actual key length
  adfile.write((char *)(&_key_size), sizeof(_key_size));
  // Number of keys of length 1B in ad_0, then in ad_1; folllowed by info for
  // 2B; then 4B
  for (auto ad = 0; ad < cNum_table; ++ad) {
    const UINT_8 num = static_cast<UINT_8>(keys_1B[ad].size());
    adfile.write((char *)(&num), sizeof(num));
  }
  for (auto ad = 0; ad < cNum_table; ++ad) {
    const UINT_16 num = static_cast<UINT_16>(keys_2B[ad].size());
    adfile.write((char *)&num, sizeof(num));
  }
  for (auto ad = 0; ad < cNum_table; ++ad) {
    const UINT_32 num = static_cast<UINT_32>(keys_4B[ad].size());
    adfile.write((char *)&num, sizeof(num));
  }
  // Write keys of length 1B, 2B, 4B in ad_0, then in ad_1;
  for (auto ad = 0; ad < cNum_table; ++ad) {
    adfile.write((char *)(keys_1B[ad].data()),
                 keys_1B[ad].size() * sizeof(UINT_8));
    adfile.write((char *)(keys_2B[ad].data()),
                 keys_2B[ad].size() * sizeof(UINT_16));
    adfile.write((char *)(keys_4B[ad].data()),
                 keys_4B[ad].size() * sizeof(UINT_32));
  }
  std::cout << "Anti-dictionary saved successfully. " << std::endl;
  return ReturnStatus::SUCCESS;
}

bool Anti_dictionary::find_following_letter(const KEY_TYPE &key,
                                            char &letter) const {
  bool found = false;
  for (auto i = 0; i < cNum_table; ++i) {
    auto it = _ad[i].find(key);
    if (it != _ad[i].end()) { // search successful => this k-1mer is
      // followed by this letter
      if (i == 0) {
        letter = '1';
      } else {
        letter = '0';
      }
      found = true;
      break;
    }
  }
  return found;
}

// Finds the size of the key
int Anti_dictionary::get_key_size() const {
  return static_cast<int>(_key_size);
}

// Finds the size of the anti-dictionary (# keys stored in total)
int Anti_dictionary::get_ad_size() const { return _ad_size; }

void Anti_dictionary::print() const {
  int valid_bits = cMax_key_size - _key_size;
  for (auto i = 0; i < cNum_table; ++i) {
    std::cout << "AD " << i << std::endl;
    for (auto k : _ad[i]) {
      std::string full_key = (std::bitset<cMax_key_size>(k)).to_string();
      std::cout << full_key.substr(valid_bits, _key_size) << std::endl;
    }
  }
}

//////////////////////// private ////////////////////////
ReturnStatus Anti_dictionary::choose_maw_size(const std::string &seqfilename,
                                              const std::string &mawfilename,
                                              const int max_maw_size) {
  std::ifstream infile(mawfilename);
  std::cout << "OPENED:::" << mawfilename << std::endl;
  if (!infile.is_open()) {
    std::cerr << "Cannot open MAW file \n";
    return ReturnStatus::ERR_FILE_OPEN;
  }
  std::string line;
  int lineNum = 0;
  // Read the first line with sequence name
  getline(infile, line);
  if (line.empty()) {
    std::cerr << "Invalid Input: Empty MAW File: " << std::endl;
    return ReturnStatus::ERR_INVALID_INPUT;
  }
  if (line[0] != '>') {
    std::cerr
        << "Invalid Input: Not a proper MAW format: Expected '>' at line number"
        << lineNum << std::endl;
    return ReturnStatus::ERR_INVALID_INPUT;
  }
  /* Read the input sequence and create suffix array to answer frquency of maws
     */
  sdsl::csa_bitcompressed<> csa;
  create_seq_csa(seqfilename, csa);
  /* Read the file once to choose the length of maws. */
  std::vector<int> freq(max_maw_size + 1,
                        0); // freq[i] = total frequencies of maws of length i
  std::vector<int> num_maws(
      max_maw_size + 1,
      0); // num_maws[i] = total number of maws of length i

  // Collect the frequency and number of maws of each length
  int min_maw_size = max_maw_size;
  int line_num = 0;
  while (getline(infile, line)) {
    ++line_num;
    if (!line.empty()) {
      int maw_len = line.size();
      // Find the number of occs of the longest prefix of this maw
      auto f = sdsl::count(csa, line.substr(0, maw_len - 1));
      ++num_maws[maw_len];
      freq[maw_len] += f;
      if (maw_len < min_maw_size) {
        min_maw_size = maw_len;
      }
    }
  }
  // find the length that gives maximum (positive) number of compressed bits
  // (i.e. freq[l] - num_maws[l])
  // If no such positive number then choose the shortest
  int chosen_maw_size = min_maw_size; // length of the chosen maw
  int max_comp_bits = 0;
  for (auto l = max_maw_size; l > 1; --l) {
    if ((freq[l] - num_maws[l]) > max_comp_bits) {
      max_comp_bits = freq[l] - num_maws[l];
      chosen_maw_size = l;
    }
  }
  if (chosen_maw_size > cMax_key_size) {
    std::cerr << "Minimum MAW size exceeds the maximum key size in hash-table: "
                 "Mawsize = "
              << min_maw_size << " and key size allowed = " << cMax_key_size
              << " \n";
    return ReturnStatus::ERR_LIMIT_EXCEEDS;
  }
  _key_size = static_cast<KEY_TYPE>(chosen_maw_size -
                                    1); // key is only the first k-1 bits
#ifdef VERBOSE
  std::cout << "The shortest and chosen maw size: freq: " << min_maw_size
            << " : " << chosen_maw_size << " : " << freq[chosen_maw_size]
            << std::endl;
#endif
}

ReturnStatus Anti_dictionary::load_chosen_maws(const std::string &filename,
                                               const Parser &parser) {
  const int chosen_maw_size = _key_size + 1;
  std::ifstream infile(filename);
  if (!infile.is_open()) {
    std::cerr << "Cannot open MAW file \n";
    return ReturnStatus::ERR_FILE_OPEN;
  }
  std::string line;
  int lineNum = 0;
  // Read the first line with sequence name
  getline(infile, line);
  if (line.empty()) {
    std::cerr << "Invalid Input: Empty MAW File: " << std::endl;
    return ReturnStatus::ERR_INVALID_INPUT;
  }
  if (line[0] != '>') {
    std::cerr
        << "Invalid Input: Not a proper MAW format: Expected '>' at line number"
        << lineNum << std::endl;
    return ReturnStatus::ERR_INVALID_INPUT;
  }

  // Read the file again to get only the chosen maws.
  while (getline(infile, line)) {
    if (line.empty()) {
      break; // empty line => sequence ends
    }
    if (line.size() == chosen_maw_size) {
      SUBSEQUENCE maw = parser.dna_to_binary(line);
      // Store only (k-1) char
      const SUBSEQUENCE k_1mer(maw, 0, maw.size() - 1);
      const int last_letter = std::stoi(maw.substr(maw.size() - 1, 1));
      KEY_TYPE key = static_cast<KEY_TYPE>(std::stoi(k_1mer, nullptr, 2));
      // std::cout << "maw: " << maw<< std::endl;

      // k-1 char must only be followed by this last char
      // Check in the other ad.
      bool found = false;
      for (auto i = 0; i < cNum_table; ++i) {
        if (i != last_letter) {
          auto it = _ad[i].find(key);
          if (it != _ad[i].end()) { // search successful => this k-1mer is
                                    // followed by the other letter
            _ad[i].erase(it);       // Remove it
            found = true;
          }
        }
      }
      if (!found) { // k-1mer is followed only by the last char
        _ad[last_letter].insert(key);
        ++_ad_size;
      }
    }
  } // maws for the sequence end
}

// TODO: Use SDSL to directly construct csa from the file.
ReturnStatus Anti_dictionary::create_seq_csa(const std::string &filename,
                                             sdsl::csa_bitcompressed<> &csa) {
  std::ifstream infile(filename);
  if (!infile.is_open()) {
    std::cerr << "Cannot open input sequence file \n";
    return ReturnStatus::ERR_FILE_OPEN;
  }
  std::string line;
  // Get the sequence
  getline(infile, line);
  if (line.empty()) {
    std::cerr << "No Input: Empty File: " << std::endl;
    return ReturnStatus::ERR_INVALID_INPUT;
  }
  if (line[0] != '>') {
    std::cerr << "Invalid Input: Not a proper MAW format: Expected '>' "
              << std::endl;
    return ReturnStatus::ERR_INVALID_INPUT;
  }
  std::string seq_value = "";
  while (std::getline(infile, line)) {
    if (line.empty()) {
      break; // end of this sequece
    }
    seq_value += line;

  }                                      // sequence ends
  sdsl::construct_im(csa, seq_value, 1); // 1 for alphabet type
}
} // end namespace
