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

/** @file utilDefs.hpp
 * @brief Defines various functions required to parse the input command.
 */

#ifndef UTIL_DEFS
#define UTIL_DEFS

//#include <cassert>
#include <cctype>
#include <getopt.h>
//#include <sys/time.h>

#include "globalDefs.hpp"

namespace mawcd {
/** Structure defining various input flags.
 * */
struct InputFlags {
  Mode mode;
  AlphabetType alphabet_type;
  std::string selected_alphabet;
  std::string input_filename;
  std::string anti_dictionary_filename;
};

/** @brief Prints the usage instructions of the tool.
 * */
void usage(void);

/** @brief Decodes the input parameters to populate the corresponding flags.
 * */
ReturnStatus decodeFlags(int argc, char *argv[], struct InputFlags &flags);

} // end namespace

#endif
