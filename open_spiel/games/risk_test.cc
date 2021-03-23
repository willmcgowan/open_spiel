// Copyright 2019 DeepMind Technologies Ltd. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "open_spiel/games/kuhn_poker.h"

#include "open_spiel/algorithms/get_all_states.h"
#include "open_spiel/policy.h"
#include "open_spiel/spiel_utils.h"
#include "open_spiel/tests/basic_tests.h"

namespace open_spiel {
namespace risk {
namespace {

namespace testing = open_spiel::testing;

void BasicRiskTests() {
	testing::LoadGameTest("risk");
	testing::ChanceOutcomesTest(*LoadGame("risk"));
	std::vector<std::vector<int>> SimpleAdj{ {0,2,11} ,{0,2,18},{ 3,16} ,{4,14,16},{5,7,14} ,{4,6,14,15},{5,7},{4,6,8},{7,9} ,{8,10} ,{11,12} ,{0,9,12},{10,11,13},{12,14,17,16},{3,4,5,13,15,16},{5,14},{2,3,13,14,17},{13,16,18},{17,1} };
	std::vector<std::vector<int>> ClassicAdj{ {1, 2},{0, 2, 3},{0, 1, 3},{1, 2, 4},{3, 5, 6},{6, 7, 8, 10, 11},{4, 5, 8, 34},{5, 9, 11, 12, 14},{5, 6, 10, 40},{7, 14},{5, 8, 11, 40},{5, 7, 10, 12, 13},{7, 11, 13, 14},{11, 12, 14},{7, 9, 12, 13, 15},{14, 16, 20},{15, 17, 19, 20},{16, 18, 19, 38},{17, 19, 22},{16, 17, 18, 22, 21, 20},{15, 16, 19, 21},{19, 20, 22, 23},{18, 19, 21, 23},{21, 22, 24},{23, 25, 27},{24, 26, 27},{25, 27},{24, 25, 26, 28},{27, 29, 33, 35, 36},{28, 30, 32},{29, 31, 32},{30, 32},{29, 30, 33, 34},{28, 32, 34, 35},{6, 32, 33, 40},{28, 33, 34, 36, 40, 41},{28, 35, 37, 41},{36, 38, 39, 41},{37, 39},{37, 38, 40, 41},{8, 10, 34, 35, 39, 41},{35, 36, 37, 39, 40} }; ;
	std::vector<std::vector<int>> SimpleCont{ {0,1} ,{2,3,4,16},{5,14,15},{6,7,8,9} , {10,11,12},{13,17,18} };
	std::vector<std::vector<int>> ClassicCont{ {0, 1, 2, 3},{4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 34},{15, 16, 17, 18, 19, 20, 21, 22, 23},{24, 25, 26, 27},{28, 29, 30, 31, 32, 33},{35, 36, 37, 38, 39, 40, 41} };
	std::vector<int> SimpleContBonus = { 1,5,3,3,2,2 };
	std::vector<int> ClassicContBonus = { 2, 7, 5, 2, 3, 5 };
	std::vector<std::string> SimpleTerrNames = { "ALPHA","BRAVO","CHARLIE","DELTA","ECHO","FOXTROT","GOLF","HOTEL","INDIA","JULIETT","KILO","LIMA","MIKE","NOVEMBER","OSCAR","PAPA","QUEBEC","ROMEO","SIERRA" };
	std::vector<std::string> ClassicTerrNames = { "East Aus.", "West Aus.", "New Guinea", "Indonesia", "Siam", "China", "India", "Mongolia", "Afghanistan", "Japan", "Ural", "Siberia", "Irkutsk", "Yakutsk", "Kamchatka", "Alaska", "N.W. Territory", "Greenland", "Quebec", "Ontario", "Alberta", "West U.S", "East U.S", "C. America", "Venezuela", "Peru", "Argentina", "Brazil", "N. Africa", "C. Africa", "S. Africa", "Madagascar", "E. Africa", "Egypt", "Middle East", "S. Europe", "W. Europe", "Great Britain", "Iceland", "Scandinavia", "Ukraine", "N. Europe" };
	std::array<int, 4> SimpleCardArr = { 6,12,19,21 };
	std::array<int, 4> ClassicCardArr = { 14,28,42,44 };
		for (int i = 2; i < 7; ++i) {
		std::vector<int> rewards;
		std::vector<int> assist;
		std::vector<int> abstraction0 = { false,false,false,false };
		std::vector<int> abstraction1 = { true,true,true,true };
		std::vector<int> action_q0 = { 55,1000,1000,1000 };
		std::vector<int> action_q1 = { 5,1,1,1 };
		int max_turns;
		switch (i) {
		case 2:
			max_turns = 90;
			rewards = { -1,1 };
			assist = { 0,3 };
			break;
		case 3:
			max_turns = 85;
			rewards = { -1,-1,2 };
			assist = { 0,0,3 };
			break;
		case 4:
			max_turns = 80;
			rewards = { -1,-1,-1,3 };
			assist = { 0,0,0,3 };
			break;
		case 5:
			max_turns = 75;
			rewards = { -1,-1,-1,-1,4 };
			assist = { 0,0,0,1,3 };
			break;

		case 6:
			max_turns = 70;
			rewards = { -1,-1,-1,-1,-1,5 };
			assist = { 0,0,0,1,2,3 };
			break;
		}
		testing:::RandomSimTest(
			*LoadGame("risk", { {"players", GameParameter(i)},{"adj",GameParameter(SimpleAdj)},{"cont",GameParameter(SimpleCont)},{"cont_bonus",GameParameter(SimpleContBonus)} {"max_turns",GameParameter(max_turns)},{"rewards",GameParameter(rewards)},{"assist",GameParameter(assist)},{"abstraction",GameParameter(abstraction0)},{"action_q",GameParameter(action_q0)},{"card_arr",GameParameter(SimpleCardArr)},{"terr_names",GameParameter(SimpleTerrNames)} }), 100);
		testing:::RandomSimTest(
			*LoadGame("risk", { {"players", GameParameter(i)},{"adj",GameParameter(SimpleAdj)},{"cont",GameParameter(SimpleCont)},{"cont_bonus",GameParameter(SimpleContBonus)} {"max_turns",GameParameter(max_turns)},{"rewards",GameParameter(rewards)},{"assist",GameParameter(assist)},{"abstraction",GameParameter(abstraction1)},{"action_q",GameParameter(action_q1)},{"card_arr",GameParameter(SimpleCardArr)},{"terr_names",GameParameter(SimpleTerrNames)} }), 100);

		testing:::RandomSimTest(
			*LoadGame("risk", { {"players", GameParameter(i)},{"adj",GameParameter(ClassicAdj)},{"cont",GameParameter(ClassicCont)},{"cont_bonus",GameParameter(ClassicContBonus)} {"max_turns",GameParameter(max_turns)},{"rewards",GameParameter(rewards)},{"assist",GameParameter(assist)},{"abstraction",GameParameter(abstraction0)},{"action_q",GameParameter(action_q0)},{"card_arr",GameParameter(ClassicCardArr)},{"terr_names",GameParameter(ClassicTerrNames)} }), 100);
		testing:::RandomSimTest(
			*LoadGame("risk", { {"players", GameParameter(i)},{"adj",GameParameter(ClassicAdj)},{"cont",GameParameter(ClassicCont)},{"cont_bonus",GameParameter(ClassicContBonus)} {"max_turns",GameParameter(max_turns)},{"rewards",GameParameter(rewards)},{"assist",GameParameter(assist)},{"abstraction",GameParameter(abstraction1)},{"action_q",GameParameter(action_q1)},{"card_arr",GameParameter(ClassicCardArr)},{"terr_names",GameParameter(ClassicTerrNames)} }), 100);
		}
		
	}
}  // namespace
}  // namespace risk
}  // namespace open_spiel

int main(int argc, char **argv) {
  open_spiel::kuhn_poker::BasicRiskTests();
}
