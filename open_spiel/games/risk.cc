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



//might need to change the current chance action id to zero etc for reasons//
namespace open_spiel {
    namespace risk {
        namespace {

            // Default parameters.
            constexpr int kDefaultPlayers = 2;
            constexpr int kDefaultMap = 1;
            constexpr int kDefaultMaxTurns = 90;
            constexpr std::vector<int> kDefaultRewards = { -1,1 };
            constexpr std::vector<int> kDefaultAssist = { 0,3 };
            constexpr std::vector<bool> kDefaultAbstraction = { false,false,false,false };
            constexpr std::vector<int> kDefaultAbstractionQ = { 55,1000,1000,1000 };



            //couldnt get these to work with templates so here is this ugly stupid code//
            std::array<std::array<bool, 19>, 19> SimpleAdjMatrixer(std::vector<std::vector<int>>dict) {
                std::array<std::array<bool, 19>, 19> res = { 0 }
                for (int i = 0; i < dict.size(); ++i) {
                    for (int j = 0; j < dict[i].size(); ++j) {
                        res[i][dict[i][j]] == 1;
                    }
                }
                return res;
            }
            std::array<std::array<bool, 42>, 42> ClassicAdjMatrixer(std::vector<std::vector<int>>dict) {
                std::array<std::array<bool, 42>, 42> res = { 0 }
                for (int i = 0; i < dict.size(); ++i) {
                    for (int j = 0; j < dict[i].size(); ++j) {
                        res[i][dict[i][j]] == 1;
                    }
                }
                return res;
            }

            std::array<std::array<bool, 19>, 6> SimpleContMatrixer(std::vector<std::vector<int>>dict) {
                std::array<std::array<bool, 19>, 6> res = { 0 };
                for (int i = 0; i < dict.size(); ++i) {
                    for (int j = 0; j < dict[i].size(); ++j) {
                        res[i][dict[i][j]] == 1;
                    }
                }
                return res;
            }
            std::array<std::array<bool, 42>, 6> ClassicContMatrixer(std::vector<std::vector<int>>dict) {
                std::array<std::array<bool, 42>, 6> res = { 0 };
                for (int i = 0; i < dict.size(); ++i) {
                    for (int j = 0; j < dict[i].size(); ++j) {
                        res[i][dict[i][j]] == 1;
                    }
                }
                return res;
            }



            // Facts about the game
            const GameType kGameType{/*short_name=*/"risk",
                /*long_name=*/"Risk",
                GameType::Dynamics::kSequential,
                GameType::ChanceMode::kSampledStochastic,
                GameType::Information::kImperfectInformation,
                GameType::Utility::kZeroSum,
                GameType::RewardModel::kTerminal,
                /*max_num_players=*/2,
                /*min_num_players=*/6,
                /*provides_information_state_string=*/false,
                /*provides_information_state_tensor=*/false,
                /*provides_observation_string=*/false,
                /*provides_observation_tensor=*/true,
                /*parameter_specification=*/
                 {{"players", GameParameter(kDefaultPlayers)},{"map",GameParameter(kDefaultMap)},{"max_turns",GameParameter(kDefaultMaxTurns)},{"rewards",GameParameter(kDefaultRewards)},{"assist",GameParameter(kDefaultAssist)},{"abstraction",GameParameter(kDefaultAbstraction)},{"action_q",GameParameter(kDefaultActionQ}},
                     /*default_loadable=*/true,
                     /*provides_factored_observation_string=*/false,
                     };

            std::shared_ptr<const Game> Factory(const GameParameters& params) {
                return std::shared_ptr<const Game>(new RiskGame(params));
            }

            REGISTER_SPIEL_GAME(kGameType, Factory);
        }  // namespace

        class RiskObserver : public Observer {
        public:
            RiskObserver(IIGObservationType iig_obs_type)
                : Observer(/*has_string=*/false, /*has_tensor=*/true),
                iig_obs_type_(iig_obs_type) {}

            void WriteTensor(const State& observed_state, int player,
                Allocator* allocator) const override {
                const RiskState& state =
                    open_spiel::down_cast<const RiskState&>(observed_state);
                SPIEL_CHECK_GE(player, 0);
                SPIEL_CHECK_LT(player, state.num_players_);
                const int num_players = state.num_players_;
                const int num_terrs = state.num_terrs_;

                if (iig_obs_type_.private_info == PrivateInfoType::kSinglePlayer) {
                    // The player's card, if one has been dealt.
                        auto out = allocator->Get("private_cards", { num_terrs + 2 });
                        auto hand = state.GetHand(player);
                        for (int i = 0; i < num_terrs + 2; ++i) {
                            out.at(i) = hand[i];
                        }
                    }

                    // Betting sequence.
                if (iig_obs_type_.public_info) {
                    if (!iig_obs_type_.perfect_recall) {
                        auto out = allocator->Get("board", { num_players * num_terrs + 2 * num_players + 13 + 5 * num_terrs + num_players * (num_players - 1) });
                        std::array<int, 2 * num_players * num_terrs + 5 * num_terrs + 2 * num_players + num_players * num_players + 14> board = state.Board();
                        for (int i = 0; i < num_players * num_terrs + num_players + 14 + 5 * num_terrs; ++i) {
                            out.at(i) = board[i];
                        }
                        for (int i = 0; i < num_players; ++i) {
                            out.at(i + num_players * num_terrs + num_players + 14 + 5 * num_terrs) = state.GetHandSum(i);
                        }
                        for (int i = 0; i < num_players * (num_players - 1); ++i) {
                            out.at(i + num_players * num_terrs + 2 * num_players + 14 + 5 * num_terrs) = board[2 * num_players * num_terrs + 14 + 5 * num_terrs + 3 * num_players];
                        }
                    }
                }
            }

        private:
            IIGObservationType iig_obs_type_;
            };

        RiskState::RiskState(std::shared_ptr<const Game> game)
            : State(game),
            num_terrs_(game->NumTerrs()),
            rewards(game->Rewards()),
            assist(game->Assist()),
            abstraction(game->Abstraction()),
            action_q(game->ActionQ()) {
            switch (num_players_) {
            case 2:
                starting_troops = 40;
                break;
            case 3:
                starting_troops = 35;
                break;
            case 4:
                starting_troops = 30;
                break;
            case 5:
                starting_troops = 25;
                break;
            case 6:
                starting_troops = 20;
                break;
            }
            std::array<int, 2 * num_players_ * num_terrs_ + 5 * num_terrs_ + 2 * num_players_ + num_players_ * num_players_ + 14> board = { 0 };
            std::array<std::array<bool, num_terrs_>, 6> cont_matrix = ContMatrixer(std::vector < std::vector<int>>)
                if (game->map_type_ == 0) {
                    card_arr = ClassicCardArr;
                    adj_matrix = std::array<std::array<bool, num_terrs_>, num_terrs_> ClassicAdjMatrixer(std::vector<std::vector<int>>ClassicAdjVect);
                    std::array<std::array<bool, num_terrs_>, 6> cont_matrix = ClassicContMatrixer(std::vector<std::vector<int>>ClassicContVect);
                    cont_bonus = ClassicContBonus;
                }
                else if (game->map_type_ == 1) {
                    card_arr = SimpleCardArr;
                    adj_matrix = std::array<std::array<bool, num_terrs_>, num_terrs_> SimpleAdjMatrixer(std::vector<std::vector<int>>SimpleAdjVect);
                    cont_matrix = std::array<std::array<bool, num_terrs_>, 6>SimpleContMatrixer(std::vector<std::vector<int>>SimpleContVect);
                    cont_bonus = SimpleContBonus;
                }
            std::array<int, 10> phse_constants = { 0,num_terrs_ + 1,num_terrs_ + 1 + action_q[0],2 * num_terrs_ + 2 + action_q[0],3 * num_terrs_ + 2 + action_q[0], 3 * num_terrs_ + 2 + action_q[0] + action_q[1], 3 * num_terrs_ + 3 + action_q[0] + action_q[1] + action_q[2], 4 * num_terrs_ + 4 + action_q[0] + action_q[1] + action_q[2], 5 * num_terrs_ + 4 + action_q[0] + action_q[1] + action_q[2], 5 * num_terrs_ + 4 + action_q[0] + action_q[1] + action_q[2] + action_q[3] };
            SetIncome(1);
            SetPlayer(0);
            SetPhse(0);
        }


      // How much each player has contributed to the pot, indexed by pid.

std::array<int, 2 * num_players_ * num_terrs_ + 5 * num_terrs_ + 2 * num_players_ + num_players_ * num_players_ + 14> RiskState::Board() {
     return board;
}

void RiskState::ResetPlayer() {
    for (int i = 0; i < num_players_; ++i) {
        board[num_players_ * num_terrs_ + i] = 0;
    }
}

void RiskState::SetPlayer(int player) {
    ResetPlayer();
    board[num_players_ * num_terrs_ + player] = 1;
}

void RiskState::SetNextPlayer() {
    int cur_player = GetPlayer();
    int new_player = (cur_player + 1) % num_players_;
    board[num_players_ * num_terrs_ + cur_player] = 0;
    board[num_players_ * num_terrs_ + new_player] = 1;
    if (new_player == 0) {
        IncrementTurns(1);
    }
}

int RiskState::GetPlayer() const {
    for (int i = 0; i < num_players_; ++i) {
        if (board[num_players_ * num_terrs_ + i] == 1) {
            return i;
        }
    }
    return num_players_;
}

int RiskState::GetTurns() const{
    return board[num_players_ * num_terrs_ + num_players_];
}

void RiskState::IncrementTurns(int increment) {
    board[num_players_ * num_terrs_ + num_players_] += 1;
}

void RiskState::ResetPhse() {
    for (int i = 0; i < 9; ++i) {
        board[num_players_ * num_terrs_ + num_players_ + 1 + i] = 0;
    }
}

void RiskState::SetPhse(int phse) {
    ResetPhse();
    board[num_players_ * num_terrs_ + num_players_ + 1 + phse] = 1;
}

int RiskState::GetPhse() const{
    for (int i = 0; i < 9; ++i) {
        if (board[num_players_ * num_terrs_ + num_players_ + 1 + i] == 1) {
            return i;
        }
    }
    return 9;
}

void RiskState::SetIncome(int income) {
    board[num_players_ * num_terrs_ + num_players_ + 2 + 9] = income;
}

int RiskState::GetIncome() const {
    return board[num_players_ * num_terrs_ + num_players_ + 2 + 9];
}

void RiskState::IncrementIncome(int increment) {
    board[num_players_ * num_terrs_ + num_players_ + 2 + 9] += increment;
}

void RiskState::SetChance(int chance) {
    board[num_players_ * num_terrs_ + num_players_ + 1 + 9] = chance;
}

int RiskState::GetChance() const {
    return board[num_players_ * num_terrs_ + num_players_ + 1 + 9];
}

void RiskState::SetTerr(int coord,int player,int troops) {
    board[player * num_terrs_ + coord] = troops;
}

void RiskState::IncrementTerr(int coord,int player, int increment) {
    board[player*num_terrs_ + coord] += increment;
}

void RiskState::SetSucc(int val) {
    board[num_players_ * num_terrs_ + num_players_ + 4 + 9 + 5 * num_terrs_] = val;
}

int RiskState::GetSucc() const{
    return board[num_players_ * num_terrs_ + num_players_ + 4 + 9 + 5 * num_terrs_];
}

void RiskState::ResetActions() {
    for (int i = num_players_ * num_terrs_ + num_players_ + 3 + 9; i < num_players_ * num_terrs_ + num_players_ + 4 + 9 + 5 * num_terrs_; ++i) {
        board[i] = 0;
    }
}

void RiskState::SetCoord(int coord) {
    switch (GetPhse()) {
    case 0:
        //dep_to//
        board[num_players_ * num_terrs_ + num_players_ + 3 +9+ coord]=1;
        SetPhse(1);
        break;
    case 2:
        //atk_from//
        board[num_players_ * num_terrs_ + num_players_ + 3 + 9 + num_terrs_+coord] = 1;
        SetPhse(3);
        break;
    case 3:
        //atk_to/
        board[num_players_ *num_terrs_ + num_players_ + 3 + 9 + 2*num_terrs_ + coord] = 1;
        SetPhse(4);
        break;
    case 6:
        //fort_from//
        board[num_players_ * num_terrs_ + num_players_ + 4 + 9 + 3 * num_terrs_ + coord] = 1;
        SetPhse(7);
        break;
    case 7:
        //fort_to//
        board[num_players_ *num_terrs_ + num_players_ + 4 + 9 + 4 * num_terrs_ + coord] = 1;
        SetPhse(8);
        break;
    }
}

int RiskState::GetCoord(int phse) const {
    int constant = 0;
    switch (phse) {
    case 0:
        constant = num_players_ * num_terrs_ + num_players_ + 3 + 9;
        break;
    case 2:
        constant = num_players_ * num_terrs_ + num_players_ + 3 + 9 + num_terrs_;
        break;
    case 3:
        constant = num_players_ * num_terrs_ + num_players_ + 3 + 9 + 2 * num_terrs_;
        break;
    case 6:
        constant = num_players_ * num_terrs_ + num_players_ + 4 + 9 + 3 * num_terrs_;
        break;
    case 7:
        constant = num_players_ * num_terrs_ + num_players_ + 4 + 9 + 4 * num_terrs_;
        break;
    }
    for (int i = 0; i < num_terrs_; ++i) {
        if (board[constant + i] == 1) {
            return i;
        }
    }
    return num_terrs_;
}

void RiskState::SetAtkNum(int num) {
    board[num_players_ * num_terrs_ + num_players_ + 3 + 9 + 3 * num_terrs_] = num;
    SetChance(1);
}

int RiskState::GetAtkNum() const {
    return board[num_players_ * num_terrs_ + num_players_ + 3 + 9 + 3 * num_terrs_];
}

void RiskState::SetDeploy() {
    ResetActions();
    SetPhse(0);
}

void RiskState::SetAttack() {
    ResetActions();
    SetPhse(2);
}

void RiskState::SetRedistribute() {
    SetPhse(5);
}

void RiskState::SetFortify() {
    ResetActions();
    SetPhse(6);
}

void RiskState::SetCard(int player, int card_coord,int amount){
    board[num_players_ * num_terrs_ + num_players_ + 5 + 9 + 5 * num_terrs_ + (num_terrs_ + 2) * player + card_coord] = amount;
}

bool RiskState::GetCard(int player, int card_coord) const{
    return board[num_players_ * num_terrs_ + num_players_ + 5 + 9 + 5 * num_terrs_ + (num_terrs_ + 2) * player + card_coord];
}

void RiskState::ResetHand(int player) {
    for (int i = 0; i < num_terrs_ + 2; ++i) {
        SetCard(player, i, 0);
    }
}

std::array<bool, num_terrs_ + 2> RiskState::GetHand(int player) const{
    std::array<bool, num_terrs_ + 2> res = { 0 };
    for (int i = 0; i < num_terrs_ + 2; ++i) {
        res[i] = GetCard(player,i);
    }
    return res;
}

std::array<bool, (num_terrs_ + 2)> RiskState::GetCards() const {
    std::array<bool, (num_terrs_ + 2)> res = { 0 };
    for (int i = 0; i < num_players_; ++i) {
        auto hand = GetHand(i);
        for (int j = 0; j < num_terrs_ + 2; ++j) {
            res[j] = hand[j];
        }
    }
    return res;
}

int RiskState::GetCardSig(int card_coord) const{
    int arr[4] = { 2,3,5,7 };
    for (int i = 0; i < 4; ++i) {
        if (kCardArr[i] > card_coord) {
            return arr[i];
        }
    }
    return 0;
}
int RiskState::GetHandSig(int player) const {
    auto hand = GetHand(player);
    int res=1;
    for (int i = 0; i <(int) hand.size(); ++i) {
        if (hand[i] == 1) {
            res *= GetCardSig(i);
        }
    }
    return res;
}
int RiskState::GetHandSum(int player) const {
    auto hand = GetHand(player);
    int sum = 0;
    for (int i = 0; i < (int)hand.size(); ++i) {
        sum += hand[i];
    }
    return sum;
}
bool RiskState::GetCashable(int player) const {
    int sig = GetHandSig(player);
    std::array<int, 10> key_arr = { 30,42,70,105,98,147,245,125,27,8 };
    for (int i = 0; i < 10; ++i) {
        if (sig % key_arr[i] == 0) {
            return true;
        }
    }
    return false;
}
std::array<int,3> RiskState::GetSatCards(std::array<int, 3> component_arr) const{
    std::array<int, 3> arr = { 0 };
    int player = GetPlayer();
    auto hand = GetHand(player);
    for (int i = 0; i < 3; ++i) {
        if (component_arr[i] == 0) {
            for (int j = 0; j < kCardArr[0]; ++j) {
                if (GetCard(player, j) == 1) {
                    arr[i] = j;
                    hand[j] = 0;
                    break;
                }
            }
        }
        else {
            for (int j = kCardArr[component_arr[i]-1]; j < kCardArr[component_arr[i]]; ++j) {
                if (GetCard(player, j) == 1) {
                    arr[i] = j;
                    hand[j] = 0;
                    break;
                }
            }
        }
    }
    return arr;
}

int RiskState::GetOwner(int coord) const{
    for (int i = 0; i < num_players_; ++i) {
        if (board[i * num_terrs_ + coord] != 0) {
            return i;
        }
    }
    return num_players_;
}

std::array<int,num_terrs_> RiskState::GetTroopArr(int player) const{
    std::array<int,num_terrs_> res = { 0 };
    for (int i = 0; i < num_terrs_; ++i) {
        if (board[num_terrs_ * player + i] > 0) {
            res[i] = board[num_terrs_ * player + i];
        }
    }
    return res;
}

std::array<bool, num_terrs_> RiskState::GetTroopMask(int player) const{
    std::array<bool, num_terrs_> res = { 0 };
    for (int i = 0; i < num_terrs_; ++i) {
        if (board[num_terrs_ * player + i] > 0) {
            res[i] = 1;
        }
    }
    return res;
}

int RiskState::GetEliminated(int player) const{
    for (int i = 0; i < num_players_ - 1; ++i) {
        if (board[2 * num_players_ * num_terrs_ + 5 + 9 + 5 * num_terrs_ + 3 * num_players_ +player*(num_players_-1)+i] == 1) {
            return i+1;
        }
    }
    return 0;
}

int RiskState::GetMaxElim() const {
    int max = 0;
    for (int i = 0; i < num_players_; ++i) {   
        int elim = GetEliminated(i);
        if (elim > max) {
            max = elim;
        }
    }
    return max;
}



void RiskState::EndTurn() {
    if (GetSucc() == 1) {
        SetChance(1);
    }
    else {
        
        SetNextPlayer();
        while (GetEliminated(GetPlayer()) !=0) {
            SetNextPlayer();
        }
        SetDeploy();
        Income();
        int sum = GetHandSum(GetPlayer());
        if (sum > 4) {
            Cash();
        }
    }
}

void RiskState::Eliminate(int victim, int victor) {
    auto victim_hand = GetHand(victim);
    for (int i = 0; i < num_terrs_+2; ++i) {
        board[num_players_ * num_terrs_ + num_players_ + 5 + 9 + 5 * num_terrs_ + (num_terrs_ + 2) *victor + i ]=victim_hand[i];
        ResetHand(victim);
    }
    int max = GetMaxElim();
    board[2 * num_players_ * num_terrs_ + 5 + 9 + 5 * num_terrs_ + 3 * num_players_ + victim* (num_players_ - 1) + max] = 1;
}

void RiskState::Deal() {
    int player = GetPlayer();
    assert(GetChance()==1);
    std::vector<int> internal_deck = {};
    std::array<int,num_terrs_+2> cards = GetCards();
    for (int i = 0; i < (int)cards.size(); ++i) {
        if (cards[i] == 0) {
            internal_deck.push_back(i);
        }
    }
    auto sig = GetHandSig(player);
    std::random_device seed;
    std::mt19937 gen(seed());
    std::uniform_int_distribution<>distrib(0, internal_deck.size() - 1);
    int choice = distrib(gen);
    int card = internal_deck[choice];
    SetCard(player, card, 1);
    SetChance(0);
    SetSucc(0);
    EndTurn();
}

void RiskState::Cash() {
    int player = GetPlayer();
    assert(GetCashable(player));
    int key = GetHandSig(player);
    std::array<int,10> key_arr = { 30,42,70,105,98,147,245,125,27,8 };
    std::array<std::array<int, 3>, 10> component_arr = { {{0,1,2},{0,1,3},{0,2,3},{1,2,3},{0,3,3},{1,3,3},{2,3,3},{2,2,2},{1,1,1},{0,0,0}} };
    std::array<int, 10> troop_arr = { 10,10,10,10,10,10,10,8,6,4 };
    int index = 0;
    for (int i = 0; i < 10;++i) {
        if (key % key_arr[i] == 0) {
            index = i;
            break;
        }
    }
    int income = troop_arr[index];
    bool bonus = false;
    auto sat_cards = GetSatCards(component_arr[index]);
    for (int i = 0; i < 3; ++i) {
        if (sat_cards[i] < num_terrs_ &&bonus ==false) {
            if (GetOwner(sat_cards[i]) == player) {
                bonus = true;
                IncrementTerr(sat_cards[i], player, 2);
            }
        }
        SetCard(player, sat_cards[i], 0);

    }
    IncrementIncome(income);
}

void RiskState::Income() {
    int turns = GetTurns();
    int income = 0;
    assert(GetIncome() == 0);
    if (turns< kStartingTroops) {
        income = 1;
    }
    else {
        std::array<bool,num_terrs_> troop_mask = GetTroopMask(GetPlayer());
        int terrs = 0;
        for (int i = 0; i < num_terrs_; ++i) {
            if (troop_mask[i]) {
                terrs += 1;
            }
        }
        int base = std::max((int)std::floor(terrs / 3), 3);
        int continent_bonus = 0;
        int ast_bonus = 0;
        for (int i = 0; i < kNumContinents; ++i) {
            bool sat = true;
            for (int j = 0; j < num_terrs_; ++j) {
                if (!(!troop_mask[j] && kContinentMatrix[i][j])) {
                    sat = false;
                    break;
                }
            }
            if (sat) {
                continent_bonus += kContinentBonusArr[i];
            }
        }
        if (turns == kStartingTroops) {
            ast_bonus = kAstArr[GetPlayer()];
        }
        income = base + continent_bonus + ast_bonus;
    }
    SetIncome(income);
}

void RiskState::Deploy(int amount) {
    int coord = GetCoord(0);
    int player = GetPlayer();
    IncrementTerr(coord,player, amount);
    IncrementIncome(-amount);
    if (GetIncome() == 0) {
        if (GetTurns() < kStartingTroops) {
            EndTurn();
        }
        else {
            SetAttack();
        }

    }
    else {
        SetDeploy();
    }
}

void RiskState::Attack() {
    double prob_arr[6][2] = { {0.2925668724279835, 0.628343621399177},{0.3402777777777778, 1.0},{0.44830246913580246, 0.7723765432098766},{0.4212962962962963, 1.0},{0.7453703703703703, 1.0},{0.5833333333333334, 1.0} };
    int n_atk = GetAtkNum();
    int coord_from = GetCoord(2);
    int coord_to = GetCoord(3);
    int atk_player = GetPlayer();
    int def_player = GetOwner(coord_to);
    int n_def = board[num_terrs_*def_player+coord_to];
    int start_amount = n_atk;
    while (n_atk > 0 && n_def > 0) {
        std::random_device seed;
        std::mt19937 gen(seed());
        std::uniform_real_distribution<>distrib(0, 1);
        double randnum = distrib(gen);
        int atk_dice = std::min(3, n_atk);
        int def_dice = std::min(2, n_def);
        int def_losses=0;
        switch (atk_dice * def_dice + def_dice) {
        case 8:
            if (randnum <= prob_arr[0][0]) {
                def_losses = 0;
            }
            else if (randnum <= prob_arr[0][1]) {
                def_losses = 1;
            }
            else {
                def_losses = 2;
            }
            break;
        case 6:
            if (randnum <= prob_arr[2][0]) {
                def_losses = 0;
            }
            else if (randnum <= prob_arr[2][1]) {
                def_losses = 1;
            }
            else {
                def_losses = 2;
            }
            break;
        case 4:
            if (randnum <= prob_arr[1][0]) {
                def_losses = 0;
            }
            else if (randnum <= prob_arr[1][1]) {
                def_losses = 1;
            }
            else {
                def_losses = 2;
            }
            break;

        case 3:
            if (randnum <= prob_arr[3][0]) {
                def_losses = 0;
            }
            else if (randnum <= prob_arr[3][1]) {
                def_losses = 1;
            }
            else {
                int def_losses = 2;
            }
            break;
        case 2:
            if (randnum <= prob_arr[4][0]) {
                int def_losses = 0;
            }
            else if (randnum <= prob_arr[4][1]) {
                def_losses = 1;
            }
            else {
                def_losses = 2;
            }
            break;
        }
        n_def -= def_losses;
        n_atk -= std::min(atk_dice, def_dice) - def_losses;
    }
    SetChance(0);
    assert(n_def == 0 || n_atk == 0);
    if (n_def == 0) {
        SetTerr(coord_to, GetPlayer(), n_atk);
        SetTerr(coord_to, def_player, 0);
        IncrementTerr(coord_from,atk_player, -start_amount);
        SetSucc(1);
        SetRedistribute();
    
    }
    if (n_atk == 0) {
        SetTerr(coord_to, def_player, n_def);
        IncrementTerr(coord_from,atk_player, -start_amount);
        SetAttack();
    }
    bool def_dead = true;
    for (int i = 0; i < num_terrs_; ++i) {
        if (board[num_terrs_ * def_player + i] != 0) {
            def_dead = false;
            break;
        }
    }
    if (def_dead) {
        Eliminate(def_player, GetPlayer());
    }
}

void RiskState::Redistribute(int amount) {
    int coord_from = GetCoord(3);
    int coord_to = GetCoord(2);
    int player = GetPlayer();
    IncrementTerr(coord_from,player, -amount);
    IncrementTerr(coord_to,player, amount);
    int sum = GetHandSum(player);
    if (sum > 5) {
        SetDeploy();
    }
    else {
        SetAttack();
    }
    while (sum > 5) {
        Cash();
        sum -= 3;
    }
}

void RiskState::Fortify(int amount) {
    int player = GetPlayer();
    int coord_from = GetCoord(6);
    int coord_to = GetCoord(7);
    IncrementTerr(coord_to,player, amount);
    IncrementTerr(coord_from,player, -amount);
    EndTurn();
}

void RiskState::DepthFirstSearch(int player, int vertex,std::array<bool,num_terrs_>* out) const{
    (*out)[vertex]=1;
    for (int i = 0; i < num_terrs_;++i) {
        if (kAdjMatrix[vertex][i] &&board[player*num_terrs_+i]!=0&& !(*out)[i] ){
            DepthFirstSearch(player, i, out);
        }
    }
}

std::vector<int> RiskState::GetAbstraction(int num,int abs) const {
    std::vector<int> res = { 0 };
    for (int i = 0; i < abs; ++i) {
        if ((int)std::floor(num * (i + 1) / (abs)) > 0) {
            //std::cout << std::to_string(std::floor(num * (i + 1) / (abs))) + "\n";
            res.push_back(i);
        }
    }
    return res;
}

int RiskState::RetAbstraction(int action,int abs) const {
    int phse = GetPhse();
    int num=0;
    switch (phse) {
    case 1:
        num = GetIncome();
        break;
    case 4:
        num = GetTroopArr(GetPlayer())[GetCoord(2)]-1;
        break;
    case 5:
        num = GetTroopArr(GetPlayer())[GetCoord(3)]-1;
        break;
    case 8:
        num = GetTroopArr(GetPlayer())[GetCoord(6)]-1;
        break;
    }
    return (int)std::round((action + 1) * num / abs);
}

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
	if (action_id == phse_constants[9]) {
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
		int t_action = action_id - phse_constants[GetPhse()];
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
			if (!abstraction[0]) {

				Deploy(t_action + 1);
			}
			else {
				Deploy(RetAbstraction(t_action, action_q[0]));
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
			if (!abstraction[1]) {
				SetAtkNum(t_action + 1);
			}
			else {
				SetAtkNum(RetAbstraction(t_action, action_q[1]));
			}
			SetChance(1);
			break;
		case 5:
			if (t_action == 0) {
				SetAttack();
			}
			else if (!abstraction[2]) {
				Redistribute(t_action);
			}
			else {
				Redistribute(RetAbstraction(t_action - 1, action_q[2]));
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
			if (!abstraction[3]) {
				Fortify(t_action + 1);
			}
			else {
				Fortify(RetAbstraction(t_action, action_q[3]));
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
		res.push_back(phse_constants[9]);
	}
	else {
		switch (GetPhse()) {
		case 0: {//returns legal dep_to's
			if (GetCashable(player)) {
				res.push_back(0);
			}
			bool map_occupied = true;
			for (int i = 0; i < num_terrs_; ++i)
				if (GetOwner(i) == num_players_) {
					map_occupied = false;
					res.push_back(i + 1);
				}
			if (map_occupied) {
				for (int i = 0; i < num_terrs_; ++i)
					if (board[player * num_terrs_ + i] != 0) {
						res.push_back(i + 1);
					}
			}
			break;
		}
		case 1: {
			if (!abstraction[0]) {
				for (int i = 0; i < GetIncome(); ++i) {
					res.push_back(i + phse_constants[1]);
				}
			}
			else {
				std::vector<int> abs = GetAbstraction(GetIncome(),action_q[0]);
				for (size_t i = 0; i < abs.size(); ++i) {
					res.push_back(abs[i] + phse_constants[1]);
				}

			}
			break;
		}
			  //returns legal dep_q's
		case 2: {//returns legal atk_from's
			std::array<bool, num_terrs_> attackable_mask = { 0 };
			std::array<int, num_terrs_> troop_arr = GetTroopArr(player);
			std::array<bool, num_terrs_> anti_mask = { 0 };
			for (int i = 0; i < num_terrs_; ++i) {
				if (troop_arr[i] > 1) {
					attackable_mask[i] = 1;
				}
				else if (troop_arr[i] == 0) {
					anti_mask[i] = 1;
				}
			}
			res.push_back(phse_constants[2]);
			for (int i = 0; i < num_terrs_; ++i) {
				for (int j = 0; j < num_terrs_; ++j) {
					if (attackable_mask[i] && anti_mask[j] && kAdjMatrix[i][j]) {
						res.push_back(phse_constants[2] + 1 + i);
						break;
					}
				}
			}
			break;
		}
		case 3: {
			int from_coord = GetCoord(2);
			std::array<int, num_terrs_> troop_arr = GetTroopArr(player);
			for (int i = 0; i < num_terrs_; ++i) {
				if (kAdjMatrix[i][from_coord] && troop_arr[i] == 0) {
					res.push_back(i + phse_constants[3]);
				}
			}
			break;
		}
			  //returns legal atk_to's
		case 4: {
			int coord_from = GetCoord(2);
			int atk_num = board[player * num_terrs_ + coord_from];
			if (kAtkAbs == false) {
				for (int i = 0; i < atk_num - 1; ++i) {
					res.push_back(i + phse_constants[4]);
				}
			}
			else {
				std::vector<int> abs = GetAbstraction(atk_num - 1, action_q[1]);
				for (size_t i = 0; i < abs.size(); ++i) {
					res.push_back(abs[i] + phse_constants[4]);
				}
			}
			break;
		}
			  //returns legal atk_q's
		case 5: {
			int coord_from = GetCoord(3);
			int redist_num = board[player * num_terrs_ + coord_from];
			res.push_back(0 + phse_constants[5]);
			if (kRedistAbs == false) {
				for (int i = 0; i < redist_num - 1; ++i) {
					res.push_back(i + 1 + phse_constants[5]);
				}
			}
			else {
				std::vector<int> abs = GetAbstraction(redist_num - 1, action_q[2]);
				for (size_t i = 0; i < abs.size(); ++i) {
					res.push_back(abs[i] + 1 + phse_constants[5]);
				}
			}
			break;
		}//returns legal redist_q's

		case 6: {
			std::array<int, num_terrs_> troop_arr = GetTroopArr(player);
			res.push_back(phse_constants[6]);
			for (int i = 0; i < num_terrs_; ++i) {
				if (troop_arr[i] > 1) {
					for (int j = 0; j < num_terrs_; ++j) {
						if (troop_arr[j] && kAdjMatrix[i][j] && i != j) {
							res.push_back(phse_constants[6] + 1 + i);
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
			std::array<bool, num_terrs_> dfs_arr = { 0 };
			DepthFirstSearch(player, from_coord, &dfs_arr);
			for (int i = 0; i < num_terrs_; ++i) {
				if (dfs_arr[i] && i != from_coord) {
					res.push_back(phse_constants[7] + i);
				}
			}
			break;
		}
			  //returns legal fort_to's needs to use depth first search
		case 8: {
			int from_coord = GetCoord(6);
			int to_coord = GetCoord(7);
			std::array<int, num_terrs_> troop_arr = GetTroopArr(player);
			if (kFortAbs == false) {
				for (int i = 0; i < troop_arr[from_coord]; ++i) {
					res.push_back(phse_constants[8] + i);
				}
			}
			else {
				std::vector<int> abs = GetAbstraction(troop_arr[from_coord] - 1, action_q[3]);
				for (int i = 0; i != abs.size(); ++i) {
					res.push_back(abs[i] + phse_constants[8]);
				}
			}
			break;
		}
		}

	}
	return res;
}


bool RiskState::IsTerminal() const { return winner_ != kInvalidPlayer; }

std::vector<double> RiskState::Rewards() const{
    assert(IsTerminal());
    std::vector<double> res = { };
    std::array<int, num_players_> arr = { 0 };
    int split = 0;
    int split_rewards = 0;
    for (int i = 0; i < num_players_; ++i) {
        arr[i] = GetEliminated(i);
        if (arr[i] == 0) {
            split_rewards += kRewards[num_players_ - split - 1];
            split += 1;
        }
    }
    double split_reward = (double)split_rewards / split;
    for (int i = 0; i < num_players_; ++i) {
        if (arr[i] != 0) {
            res.push_back( kRewards[arr[i] - 1]);
        }
        else {
            res.push_back(split_reward);
        }
    }
    return res;
}

std::vector<double> RiskState::Returns() const {
    if (!IsTerminal()) {
        std::vector<double> res = {};
        for (int i = 0; i < num_players_ ; ++i) {
            res.push_back(0);
        }
        return res;
    }
    else {
        return Rewards();
    }
}

Player RiskState::CurrentPlayer() const{
    if (GetChance()) {
        return kChancePlayerId;
    }
    else {
        return GetPlayer();
    }
}

bool RiskState::IsTerminal() const{
   if (GetMaxElim() == num_players_ || GetTurns() >= kMaxTurns) {
       return true;
   }
   return false;
}

bool RiskState::IsChanceNode() const{
    return(CurrentPlayer() == -1);
}

bool RiskState::IsPlayerNode() const {
    return (CurrentPlayer() >= 0);
}

ActionsAndProbs RiskState::ChanceOutcomes() const {
    return ActionsAndProbs{ std::make_pair(phse_constants[9], 1) };
}

std::vector<Action> RiskState::LegalChanceOutcomes() const{
    if (IsChanceNode()) {
        return { num_distinct_actions_-1 };
   }
    else {
        return {};
    }
}

void RiskState::ObservationTensor(Player player,
                                  absl::Span<float> values) const {
  ContiguousAllocator allocator(values);
  const RiskGame& game = open_spiel::down_cast<const RiskGame&>(*game_);
  game.default_observer_->WriteTensor(*this, player, &allocator);
}

std::unique_ptr<RiskState> RiskState::Clone() const {
    return std::unique_ptr<RiskState>(new RiskState(*this));
}




RiskGame::RiskGame(const GameParameters& params)
    : Game(kGameType, params), num_players_(ParameterValue<int>("players")),
    map_type_(ParameterValue<int>("map_type")),
    max_turns_(ParameterValue<int>("max_turns")),
    rewards_(ParameterValue<std::vector<int>>("rewards")),
    assist_(ParameterValue<std::vector<int>>("assist"))
    abstraction_(ParameterValue<std::array<bool,4>>("abstraction")),
    action_q_(ParameterValue<std::array<int,4>>("action_q"))
{
    if (map_type_ == 0) {
        num_terrs_ =42;
    }
    else if (map_type_ == 1) {
        num_terrs_ = 19;
    }
    num_distinct_actions_ = 5 * num_terrs_ + 4 + action_q_[0] + action_q_[1] + action_q_[2] + action_q_[3];
    max_chance_nodes_in_history_ = 10000;//random big number bc idk
    max_game_length_ = 100000;
  default_observer_ = std::make_shared<RiskObserver>(kDefaultObsType);
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
    return {};
}

   
std::shared_ptr<Observer> RiskGame::MakeObserver(
    absl::optional<IIGObservationType> iig_obs_type,
    const GameParameters& params) const {
  if (!params.empty()) SpielFatalError("Observation params not supported");
  return std::make_shared<RiskObserver>(iig_obs_type.value_or(kDefaultObsType));
}

}  // namespace risk
}  // namespace open_spiel
