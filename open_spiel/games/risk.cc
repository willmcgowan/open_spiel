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

#include <algorithm>
#include <array>
#include <string>
#include <utility>

#include "open_spiel/abseil-cpp/absl/strings/str_cat.h"
#include "open_spiel/fog/fog_constants.h"
#include "open_spiel/game_parameters.h"
#include "open_spiel/observer.h"
#include "open_spiel/policy.h"
#include "open_spiel/spiel.h"
#include "open_spiel/spiel_utils.h"

namespace open_spiel {
namespace risk {
namespace {

// Default parameters.
constexpr int kDefaultPlayers = 2;
constexpr double kAnte = 1;

// Facts about the game
const GameType kGameType{/*short_name=*/"risk",
                         /*long_name=*/"Risk",
                         GameType::Dynamics::kSequential,
                         GameType::ChanceMode::kSampledStochastic,
                         GameType::Information::kImperfectInformation,
                         GameType::Utility::kZeroSum,
                         GameType::RewardModel::kTerminal,
                         /*max_num_players=*/kNumPlayers,
                         /*min_num_players=*/kNumPlayers,
                         /*provides_information_state_string=*/false,
                         /*provides_information_state_tensor=*/false,
                         /*provides_observation_string=*/false,
                         /*provides_observation_tensor=*/true,
                         /*parameter_specification=*/
                          {{"players", GameParameter(kDefaultPlayers)}}  ,
                         /*default_loadable=*/true,
                         /*provides_factored_observation_string=*/true,
                        };

std::shared_ptr<const Game> Factory(const GameParameters& params) {
  return std::shared_ptr<const Game>(new RiskGame(params));
}

REGISTER_SPIEL_GAME(kGameType, Factory);
}  // namespace

class RiskObserver : public Observer {
 public:
  RiskObserver(IIGObservationType iig_obs_type)
      : Observer(/*has_string=*/true, /*has_tensor=*/true),
        iig_obs_type_(iig_obs_type) {}

  void WriteTensor(const State& observed_state, int player,
                   Allocator* allocator) const override {
    const RiskState& state =
        open_spiel::down_cast<const RiskState&>(observed_state);
    SPIEL_CHECK_GE(player, 0);
    SPIEL_CHECK_LT(player, state.num_players_);
    const int num_players = state.num_players_;
    const int num_cards = num_players + 1;

    if (iig_obs_type_.private_info == PrivateInfoType::kSinglePlayer) {
      {  // Observing player.
        auto out = allocator->Get("player", {num_players});
        out.at(player) = 1;
      }
      {  // The player's card, if one has been dealt.
        auto out = allocator->Get("private_card", {num_cards});
        if (state.history_.size() > player)
          out.at(state.history_[player].action) = 1;
      }
    }

    // Betting sequence.
    if (iig_obs_type_.public_info) {
      if (iig_obs_type_.perfect_recall) {
        auto out = allocator->Get("betting", {2 * num_players - 1, 2});
        for (int i = num_players; i < state.history_.size(); ++i) {
          out.at(i - num_players, state.history_[i].action) = 1;
        }
      } else {
        auto out = allocator->Get("pot_contribution", {num_players});
        for (auto p = Player{0}; p < state.num_players_; p++) {
          out.at(p) = state.ante_[p];
        }
      }
    }
  }

 private:
  IIGObservationType iig_obs_type_;
};

RiskState::RiskState(std::shared_ptr<const Game> game)
    : State(game),
      first_bettor_(kInvalidPlayer),
      card_dealt_(game->NumPlayers() + 1, kInvalidPlayer),
      winner_(kInvalidPlayer),
      pot_(kAnte * game->NumPlayers()),
      // How much each player has contributed to the pot, indexed by pid.
      ante_(game->NumPlayers(), kAnte) {}

int RiskState::CurrentPlayer() const {
  if (IsTerminal()) {
    return kTerminalPlayerId;
  } else {
    return (history_.size() < num_players_) ? kChancePlayerId
                                            : history_.size() % num_players_;
  }
}

void RiskState::DoApplyAction(Action action){
  // Additional book-keeping
	int player = GetPlayer();
	if (action_id == kPhseConstants[9]) {
		switch (GetPhse()) {
		case 4:
			Attack();
			break;
		case 6:
			Deal();
			break;
		case 8:
			Deal();
			break;
		}
		return;
	}
	else {
		int t_action = action_id - kPhseConstants[GetPhse()];
		switch (GetPhse()) {
		case 0: {
			if (t_action == 0) {
				Cash();
			}
			else {
				SetCoord(t_action - 1);
			}
			break;
		}
		case 1:
			if (kDepAbs == false) {

				Deploy(t_action + 1);
			}
			else {
				Deploy(RetAbstraction(t_action, kDepAbs));
			}
			break;
		case 2:
			if (t_action == 0) {
				SetFortify();
			}
			else {
				SetCoord(t_action - 1);
				auto troops = GetTroopArr(player);
			}
			break;
		case 3:
			SetCoord(t_action);
			break;
		case 4:
			if (kAtkAbs == false) {
				SetAtkNum(t_action + 1);
			}
			else {
				SetAtkNum(RetAbstraction(t_action, kAtkAbs));
			}
			SetChance(1);
			break;
		case 5:
			if (t_action == 0) {
				SetAttack();
			}
			else if (kRedistAbs == false) {
				Redistribute(t_action);
			}
			else {
				Redistribute(RetAbstraction(t_action - 1, kRedistAbs));
			}
			break;
		case 6:
			if (t_action == 0) {
				EndTurn();
			}
			else {
				SetCoord(t_action - 1);
			}
			break;
		case 7:
			SetCoord(t_action);
			break;
		case 8:
			if (kFortAbs == false) {
				Fortify(t_action + 1);
			}
			else {
				Fortify(RetAbstraction(t_action, kFortAbs));
			}
			break;
		}
	}
	history_.push_back(PlayerAction{ CurrentPlayer(), action_id });
	move_number_ += 1;
}

std::vector<Action> RiskState::LegalActions() const {
	std::vector<Action> res = {};
	int player = GetPlayer();
	if (GetChance() == 1) {
		res.push_back(kPhseConstants[9]);
	}
	else {
		switch (GetPhse()) {
		case 0: {//returns legal dep_to's
			if (GetCashable(player)) {
				res.push_back(0);
			}
			bool map_occupied = true;
			for (int i = 0; i < kNumTerrs; ++i)
				if (GetOwner(i) == kNumPlayers) {
					map_occupied = false;
					res.push_back(i + 1);
				}
			if (map_occupied) {
				for (int i = 0; i < kNumTerrs; ++i)
					if (board[player * kNumTerrs + i] != 0) {
						res.push_back(i + 1);
					}
			}
			break;
		}
		case 1: {
			if (kDepAbs == false) {
				for (int i = 0; i < GetIncome(); ++i) {
					res.push_back(i + kPhseConstants[1]);
				}
			}
			else {
				std::vector<int> abs = GetAbstraction(GetIncome(), kDepAbs);
				for (size_t i = 0; i < abs.size(); ++i) {
					res.push_back(abs[i] + kPhseConstants[1]);
				}

			}
			break;
		}
			  //returns legal dep_q's
		case 2: {//returns legal atk_from's
			std::array<bool, kNumTerrs> attackable_mask = { 0 };
			std::array<int, kNumTerrs> troop_arr = GetTroopArr(player);
			std::array<bool, kNumTerrs> anti_mask = { 0 };
			for (int i = 0; i < kNumTerrs; ++i) {
				if (troop_arr[i] > 1) {
					attackable_mask[i] = 1;
				}
				else if (troop_arr[i] == 0) {
					anti_mask[i] = 1;
				}
			}
			res.push_back(kPhseConstants[2]);
			std::array<bool, kNumTerrs> atk_from = { 0 };
			for (int i = 0; i < kNumTerrs; ++i) {
				for (int j = 0; j < kNumTerrs; ++j) {
					if (attackable_mask[i] && anti_mask[j] && kAdjMatrix[i][j]) {
						atk_from[i] = 1;
						res.push_back(kPhseConstants[2] + 1 + i);
						break;
					}
				}
			}
			break;
		}
		case 3: {
			int from_coord = GetCoord(2);
			std::array<int, kNumTerrs> troop_arr = GetTroopArr(player);
			for (int i = 0; i < kNumTerrs; ++i) {
				if (kAdjMatrix[i][from_coord] && troop_arr[i] == 0) {
					res.push_back(i + kPhseConstants[3]);
				}
			}
			break;
		}
			  //returns legal atk_to's
		case 4: {
			int coord_from = GetCoord(2);
			int atk_num = board[player * kNumTerrs + coord_from];
			if (kAtkAbs == false) {
				for (int i = 0; i < atk_num - 1; ++i) {
					res.push_back(i + kPhseConstants[4]);
				}
			}
			else {
				std::vector<int> abs = GetAbstraction(atk_num - 1, kAtkAbs);
				for (size_t i = 0; i < abs.size(); ++i) {
					res.push_back(abs[i] + kPhseConstants[4]);
				}
			}
			break;
		}
			  //returns legal atk_q's
		case 5: {
			int coord_from = GetCoord(3);
			int redist_num = board[player * kNumTerrs + coord_from];
			res.push_back(0 + kPhseConstants[5]);
			if (kRedistAbs == false) {
				for (int i = 0; i < redist_num - 1; ++i) {
					res.push_back(i + 1 + kPhseConstants[5]);
				}
			}
			else {
				std::vector<int> abs = GetAbstraction(redist_num - 1, kRedistAbs);
				for (size_t i = 0; i < abs.size(); ++i) {
					res.push_back(abs[i] + 1 + kPhseConstants[5]);
				}
			}
			break;
		}//returns legal redist_q's

		case 6: {
			std::array<int, kNumTerrs> troop_arr = GetTroopArr(player);
			res.push_back(kPhseConstants[6]);
			for (int i = 0; i < kNumTerrs; ++i) {
				if (troop_arr[i] > 1) {
					for (int j = 0; j < kNumTerrs; ++j) {
						if (troop_arr[j] && kAdjMatrix[i][j] && i != j) {
							res.push_back(kPhseConstants[6] + 1 + i);
							std::cout << kTerrNames[i] + "\n";
							break;
						}
					}
				}
			}
			break;
		}//returns legal fort_from's
		case 7: {
			int from_coord = GetCoord(6);
			std::array<bool, kNumTerrs> dfs_arr = { 0 };
			DepthFirstSearch(player, from_coord, &dfs_arr);
			for (int i = 0; i < kNumTerrs; ++i) {
				if (dfs_arr[i] && i != from_coord) {
					res.push_back(kPhseConstants[7] + i);
				}
			}
			break;
		}
			  //returns legal fort_to's needs to use depth first search
		case 8: {
			int from_coord = GetCoord(6);
			int to_coord = GetCoord(7);
			std::array<int, kNumTerrs> troop_arr = GetTroopArr(player);
			if (kFortAbs == false) {
				for (int i = 0; i < troop_arr[from_coord]; ++i) {
					res.push_back(kPhseConstants[8] + i);
				}
			}
			else {
				std::vector<int> abs = GetAbstraction(troop_arr[from_coord] - 1, kFortAbs);
				for (int i = 0; i != abs.size(); ++i) {
					res.push_back(abs[i] + kPhseConstants[8]);
				}
			}
			break;
		}
		}

	}
	return res;
}


bool RiskState::IsTerminal() const { return winner_ != kInvalidPlayer; }

std::vector<double> RiskState::Returns() const {
  if (!IsTerminal()) {
    return std::vector<double>(num_players_, 0.0);
  }

  std::vector<double> returns(num_players_);
  for (auto player = Player{0}; player < num_players_; ++player) {
    const int bet = DidBet(player) ? 2 : 1;
    returns[player] = (player == winner_) ? (pot_ - bet) : -bet;
  }
  return returns;
}


void RiskState::ObservationTensor(Player player,
                                  absl::Span<float> values) const {
  ContiguousAllocator allocator(values);
  const RiskGame& game = open_spiel::down_cast<const RiskGame&>(*game_);
  game.default_observer_->WriteTensor(*this, player, &allocator);
}

std::unique_ptr<State> RiskState::Clone() const {
  return std::unique_ptr<State>(new RiskState(*this));
}


std::vector<std::pair<Action, double>> RiskState::ChanceOutcomes() const {
  SPIEL_CHECK_TRUE(IsChanceNode());
  std::vector<std::pair<Action, double>> outcomes;
  const double p = 1.0 / (num_players_ + 1 - history_.size());
  for (int card = 0; card < card_dealt_.size(); ++card) {
    if (card_dealt_[card] == kInvalidPlayer) outcomes.push_back({card, p});
  }
  return outcomes;
}


RiskGame::RiskGame(const GameParameters& params)
    : Game(kGameType, params), num_players_(ParameterValue<int>("players")) {
  SPIEL_CHECK_GE(num_players_, kGameType.min_num_players);
  SPIEL_CHECK_LE(num_players_, kGameType.max_num_players);
  default_observer_ = std::make_shared<RiskObserver>(kDefaultObsType);
  info_state_observer_ = std::make_shared<RiskObserver>(kInfoStateObsType);
  private_observer_ = std::make_shared<RiskObserver>(
      IIGObservationType{.public_info = false,
                         .perfect_recall = false,
                         .private_info = PrivateInfoType::kSinglePlayer});
  public_observer_ = std::make_shared<RiskObserver>(
      IIGObservationType{.public_info = true,
                         .perfect_recall = false,
                         .private_info = PrivateInfoType::kNone});
}

std::unique_ptr<State> RiskGame::NewInitialState() const {
  return std::unique_ptr<State>(new RiskState(shared_from_this()));
}

std::vector<int> RiskGame::ObservationTensorShape() const {
  // One-hot for whose turn it is.
  // One-hot encoding for the single private card. (n+1 cards = n+1 bits)
  // Followed by the contribution of each player to the pot (n).
  // n + n + 1 + n = 3n + 1.
  return {3 * num_players_ + 1};
}

double RiskGame::MaxUtility() const {
  // In poker, the utility is defined as the money a player has at the end
  // of the game minus then money the player had before starting the game.
  // Everyone puts a chip in at the start, and then they each have one more
  // chip. Most that a player can gain is (#opponents)*2.
  return (num_players_ - 1) * 2;
}

double RiskGame::MinUtility() const {
  // In poker, the utility is defined as the money a player has at the end
  // of the game minus then money the player had before starting the game.
  // In Kuhn, the most any one player can lose is the single chip they paid
  // to play and the single chip they paid to raise/call.
  return -2;
}

std::shared_ptr<Observer> RiskGame::MakeObserver(
    absl::optional<IIGObservationType> iig_obs_type,
    const GameParameters& params) const {
  if (!params.empty()) SpielFatalError("Observation params not supported");
  return std::make_shared<RiskObserver>(iig_obs_type.value_or(kDefaultObsType));
}

}  // namespace risk
}  // namespace open_spiel
