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

#include "open_spiel/games/risk.h"

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
		for (int i = 2; i < 7; ++i) {
		std::vector<int> rewards;
		std::vector<int> assist;
		std::vector<int> abstraction0 = { false,false,false,false };
		std::vector<int> abstraction1 = { true,true,true,true };
		std::vector<int> action_q0 = { 55,1000,1000,1000 };
		std::vector<int> action_q1 = { 5,1,1,1 };
		int max_turns=90-(i-2)*5;
		testing::RandomSimTest(
			*LoadGame("risk", { {"players", GameParameter(i)},{"map",GameParameter(0)},{"max_turns",GameParameter(max_turns)},{"dep_abs",GameParameter(abstraction0[0])} ,{"atk_abs",GameParameter(abstraction0[1])},{"redist_abs",GameParameter(abstraction0[2])},{"fort_abs",GameParameter(abstraction0[3])},{"dep_q",GameParameter(action_q0[0])},{"atk_q",GameParameter(action_q0[1])},{"redist_q",GameParameter(action_q0[2])},{"fort_q",GameParameter(action_q0[3])} }), 100);

		testing::RandomSimTest(
			*LoadGame("risk", { {"players", GameParameter(i)},{"map",GameParameter(0)},{"max_turns",GameParameter(max_turns)},{"dep_abs",GameParameter(abstraction1[0])} ,{"atk_abs",GameParameter(abstraction1[1])},{"redist_abs",GameParameter(abstraction1[2])},{"fort_abs",GameParameter(abstraction1[3])},{"dep_q",GameParameter(action_q1[0])},{"atk_q",GameParameter(action_q1[1])},{"redist_q",GameParameter(action_q1[2])},{"fort_q",GameParameter(action_q1[3])}}), 100);

		testing::RandomSimTest(
			*LoadGame("risk", { {"players", GameParameter(i)},{"map",GameParameter(1)},{"max_turns",GameParameter(max_turns)},{"dep_abs",GameParameter(abstraction0[0])} ,{"atk_abs",GameParameter(abstraction0[1])},{"redist_abs",GameParameter(abstraction0[2])},{"fort_abs",GameParameter(abstraction0[3])},{"dep_q",GameParameter(action_q0[0])},{"atk_q",GameParameter(action_q0[1])},{"redist_q",GameParameter(action_q0[2])},{"fort_q",GameParameter(action_q0[3])} }), 100);

		testing::RandomSimTest(
			*LoadGame("risk", { {"players", GameParameter(i)},{"map",GameParameter(0)},{"max_turns",GameParameter(max_turns)},{"dep_abs",GameParameter(abstraction1[0])} ,{"atk_abs",GameParameter(abstraction1[1])},{"redist_abs",GameParameter(abstraction1[2])},{"fort_abs",GameParameter(abstraction1[3])},{"dep_q",GameParameter(action_q1[0])},{"atk_q",GameParameter(action_q1[1])},{"redist_q",GameParameter(action_q1[2])},{"fort_q",GameParameter(action_q1[3])} }), 100);
		}
		
	}
}  // namespace
}  // namespace risk
}  // namespace open_spiel

int main(int argc, char **argv) {
  open_spiel::risk::BasicRiskTests();
}
