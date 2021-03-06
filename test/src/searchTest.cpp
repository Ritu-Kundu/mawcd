#include <limits.h>
#include <vector>
#include <string>
#include <iterator>
#include "Parser.hpp"
#include "utilDefs.hpp"
#include "globalDefs.hpp"
#include "Elastic_string.hpp"
#include "Search.hpp"
#include "gtest/gtest.h"

// TODO: Add test invoving empty seed and/or string in a symbol

const std::vector<int> num_occ = {5,8,5,7,15};
const std::vector<std::vector<std::pair<int, int>>> indices = {
{std::pair<int, int>(0,2),std::pair<int, int>(4,4),std::pair<int, int>(4,6),std::pair<int, int>(10,11),std::pair<int, int>(10,12)},
{std::pair<int, int>(0,2) , std::pair<int, int>(4,4) , std::pair<int, int>(4,5) , std::pair<int, int>(4,6) , std::pair<int, int>(9,10) , std::pair<int, int>(9,11) , std::pair<int, int>(10,11) , std::pair<int, int>(10,12)},
{std::pair<int, int>(0,2),std::pair<int, int>(4,4),std::pair<int, int>(4,6),std::pair<int, int>(10,11),std::pair<int, int>(10,12)},
{std::pair<int, int>(0,2) , std::pair<int, int>(4,4) , std::pair<int, int>(4,5) , std::pair<int, int>(4,6) , std::pair<int, int>(5,7) , std::pair<int, int>(10,11) , std::pair<int, int>(10,12)},
{std::pair<int, int>(0,2) , std::pair<int, int>(1,3) , std::pair<int, int>(2,4) , std::pair<int, int>(3,4) , std::pair<int, int>(4,4) , std::pair<int, int>(4,5) , std::pair<int, int>(4,6) , std::pair<int, int>(5,7) , std::pair<int, int>(6,8) , std::pair<int, int>(7,9) , std::pair<int, int>(8,10) , std::pair<int, int>(9,10) , std::pair<int, int>(9,11) , std::pair<int, int>(10,11) , std::pair<int, int>(10,12)}
};
TEST(nonEmptySeedElTest, ManyPatterns) {
  /* Parse the input */
  eldes::Elastic_string els;
  eldes::Parser::parse_elastic(eldes::AlphabetType::DNA, "test_files/sampleText.txt", els);
  eldes::PATTERNS patterns;
  eldes::Parser::parse_degenerate(eldes::AlphabetType::DNA, "test_files/samplePattern.txt", patterns);
  eldes::Search search(els, patterns);

  /* Find the occurrences of each of the pattern */
  eldes::ALL_PATTERNS_OCCURRENCES apo(patterns.size());
  search.find(apo);

  for (int i=0; i < patterns.size(); ++i) {
    EXPECT_EQ(apo[i].size(), num_occ[i]);
    int j = 0;
    for (auto const& oc: apo[i]) {
      EXPECT_EQ(indices[i][j].first, oc.start);
      EXPECT_EQ(indices[i][j].second, oc.end);
	  ++j;
    }
   } 
}


