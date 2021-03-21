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
	testing::ChanceOutcomesTest(*LoadGame("risk"))
		for (int i = 0; i< 2; ++i) {
			for (int j = 2; i < 7; ++j) {
				std::vector<int> rewards;
				std::vector<int> assist;
				std::vector<int> abstraction0 = { false,false,false,false };
				std::vector<int> abstraction1 = { true,true,true,true };
				std::vector<int> action_q0 = { 55,1000,1000,1000 };
				std::vector<int> action_q1 = { 5,1,1,1 };
				int max_turns;
				switch (j) {
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
					*LoadGame("risk", { {"players", GameParameter(j)},{"map",GameParameter(i)},{"max_turns",GameParameter(max_turns)},{"rewards",GameParameter(rewards)},{"assist",GameParameter(assist)},{"abstraction",GameParameter(abstraction0)},{"action_q",GameParameter(action_q0)} }), 100);
				testing:::RandomSimTest(
					*LoadGame("risk", { {"players", GameParameter(j)},{"map",GameParameter(i)},{"max_turns",GameParameter(max_turns)},{"rewards",GameParameter(rewards)},{"assist",GameParameter(assist)},{"abstraction",GameParameter(abstraction1)},{"action_q",GameParameter(action_q1)} }), 100);
			}
		}
	}
}
}  // namespace
}  // namespace risk
}  // namespace open_spiel

int main(int argc, char **argv) {
  open_spiel::kuhn_poker::BasicRiskTests();
}
