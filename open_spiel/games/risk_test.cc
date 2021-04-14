

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
    std::vector<bool> abstraction0 = { false,false,false,false };
	std::vector<bool> abstraction1 = { true,true,true,true };
	std::vector<int> action_q0 = { 55,1000,1000,1000 };
	std::vector<int> action_q1 = { 5,1,1,1 };
	for (int i = 2; i < 7; ++i) {
		int max_turns=90-(i-2)*5;
		if(i<5){
		testing::RandomSimTest(
			*LoadGame("risk", { {"players", GameParameter(i)},{"map",GameParameter(0)},{"max_turns",GameParameter(max_turns)},{"dep_abs",GameParameter(abstraction0[0])} ,{"atk_abs",GameParameter(abstraction0[1])},{"redist_abs",GameParameter(abstraction0[2])},{"fort_abs",GameParameter(abstraction0[3])},{"dep_q",GameParameter(action_q0[0])},{"atk_q",GameParameter(action_q0[1])},{"redist_q",GameParameter(action_q0[2])},{"fort_q",GameParameter(action_q0[3])} }), 1,true,true);
		
		testing::RandomSimTest(
			*LoadGame("risk", { {"players", GameParameter(i)},{"map",GameParameter(0)},{"max_turns",GameParameter(max_turns)},{"dep_abs",GameParameter(abstraction1[0])} ,{"atk_abs",GameParameter(abstraction1[1])},{"redist_abs",GameParameter(abstraction1[2])},{"fort_abs",GameParameter(abstraction1[3])},{"dep_q",GameParameter(action_q1[0])},{"atk_q",GameParameter(action_q1[1])},{"redist_q",GameParameter(action_q1[2])},{"fort_q",GameParameter(action_q1[3])}}), 1,true,true);
		}
		testing::RandomSimTest(
			*LoadGame("risk", { {"players", GameParameter(i)},{"map",GameParameter(1)},{"max_turns",GameParameter(max_turns)},{"dep_abs",GameParameter(abstraction0[0])} ,{"atk_abs",GameParameter(abstraction0[1])},{"redist_abs",GameParameter(abstraction0[2])},{"fort_abs",GameParameter(abstraction0[3])},{"dep_q",GameParameter(action_q0[0])},{"atk_q",GameParameter(action_q0[1])},{"redist_q",GameParameter(action_q0[2])},{"fort_q",GameParameter(action_q0[3])} }), 1,true,true);
		
		testing::RandomSimTest(
			*LoadGame("risk", { {"players", GameParameter(i)},{"map",GameParameter(1)},{"max_turns",GameParameter(max_turns)},{"dep_abs",GameParameter(abstraction1[0])} ,{"atk_abs",GameParameter(abstraction1[1])},{"redist_abs",GameParameter(abstraction1[2])},{"fort_abs",GameParameter(abstraction1[3])},{"dep_q",GameParameter(action_q1[0])},{"atk_q",GameParameter(action_q1[1])},{"redist_q",GameParameter(action_q1[2])},{"fort_q",GameParameter(action_q1[3])} }), 1,true,true);
		
		}
}
}  // namespace
}  // namespace risk
}  // namespace open_spiel

int main(int argc, char **argv) {
  open_spiel::risk::BasicRiskTests();
}
