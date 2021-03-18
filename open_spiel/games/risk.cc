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
      : Observer(/*has_string=*/false, /*has_tensor=*/true),
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

void RiskState::ResetPlayer() {
    for (int i = 0; i < kNumPlayers; ++i) {
        board[kNumPlayers * kNumTerrs + i] = 0;
    }
}

void RiskState::SetPlayer(int player) {
    ResetPlayer();
    board[kNumPlayers * kNumTerrs + player] = 1;
}

void RiskState::SetNextPlayer() {
    int cur_player = GetPlayer();
    int new_player = (cur_player + 1) % kNumPlayers;
    board[kNumPlayers * kNumTerrs + cur_player] = 0;
    board[kNumPlayers * kNumTerrs + new_player] = 1;
    if (new_player == 0) {
        IncrementTurns(1);
    }
}

int RiskState::GetPlayer() const {
    for (int i = 0; i < kNumPlayers; ++i) {
        if (board[kNumPlayers * kNumTerrs + i] == 1) {
            return i;
        }
    }
    return kNumPlayers;
}

int RiskState::GetTurns() const{
    return board[kNumPlayers * kNumTerrs + kNumPlayers];
}

void RiskState::IncrementTurns(int increment) {
    board[kNumPlayers * kNumTerrs + kNumPlayers] += 1;
}

void RiskState::ResetPhse() {
    for (int i = 0; i < 9; ++i) {
        board[kNumPlayers * kNumTerrs + kNumPlayers + 1 + i] = 0;
    }
}

void RiskState::SetPhse(int phse) {
    ResetPhse();
    board[kNumPlayers * kNumTerrs + kNumPlayers + 1 + phse] = 1;
}

int RiskState::GetPhse() const{
    for (int i = 0; i < 9; ++i) {
        if (board[kNumPlayers * kNumTerrs + kNumPlayers + 1 + i] == 1) {
            return i;
        }
    }
    return 9;
}

void RiskState::SetIncome(int income) {
    board[kNumPlayers * kNumTerrs + kNumPlayers + 2 + 9] = income;
}

int RiskState::GetIncome() const {
    return board[kNumPlayers * kNumTerrs + kNumPlayers + 2 + 9];
}

void RiskState::IncrementIncome(int increment) {
    board[kNumPlayers * kNumTerrs + kNumPlayers + 2 + 9] += increment;
}

void RiskState::SetChance(int chance) {
    board[kNumPlayers * kNumTerrs + kNumPlayers + 1 + 9] = chance;
}

int RiskState::GetChance() const {
    return board[kNumPlayers * kNumTerrs + kNumPlayers + 1 + 9];
}

void RiskState::SetTerr(int coord,int player,int troops) {
    board[player * kNumTerrs + coord] = troops;
}

void RiskState::IncrementTerr(int coord,int player, int increment) {
    board[player*kNumTerrs + coord] += increment;
}

void RiskState::SetSucc(int val) {
    board[kNumPlayers * kNumTerrs + kNumPlayers + 4 + 9 + 5 * kNumTerrs] = val;
}

int RiskState::GetSucc() const{
    return board[kNumPlayers * kNumTerrs + kNumPlayers + 4 + 9 + 5 * kNumTerrs];
}

void RiskState::ResetActions() {
    for (int i = kNumPlayers * kNumTerrs + kNumPlayers + 3 + 9; i < kNumPlayers * kNumTerrs + kNumPlayers + 4 + 9 + 5 * kNumTerrs; ++i) {
        board[i] = 0;
    }
}

void RiskState::SetCoord(int coord) {
    switch (GetPhse()) {
    case 0:
        //dep_to//
        board[kNumPlayers * kNumTerrs + kNumPlayers + 3 +9+ coord]=1;
        SetPhse(1);
        break;
    case 2:
        //atk_from//
        board[kNumPlayers * kNumTerrs + kNumPlayers + 3 + 9 + kNumTerrs+coord] = 1;
        SetPhse(3);
        break;
    case 3:
        //atk_to/
        board[kNumPlayers * kNumTerrs + kNumPlayers + 3 + 9 + 2*kNumTerrs + coord] = 1;
        SetPhse(4);
        break;
    case 6:
        //fort_from//
        board[kNumPlayers * kNumTerrs + kNumPlayers + 4 + 9 + 3 * kNumTerrs + coord] = 1;
        SetPhse(7);
        break;
    case 7:
        //fort_to//
        board[kNumPlayers * kNumTerrs + kNumPlayers + 4 + 9 + 4 * kNumTerrs + coord] = 1;
        SetPhse(8);
        break;
    }
}

int RiskState::GetCoord(int phse) const {
    int constant = 0;
    switch (phse) {
    case 0:
        constant = kNumPlayers * kNumTerrs + kNumPlayers + 3 + 9;
        break;
    case 2:
        constant = kNumPlayers * kNumTerrs + kNumPlayers + 3 + 9 + kNumTerrs;
        break;
    case 3:
        constant = kNumPlayers * kNumTerrs + kNumPlayers + 3 + 9 + 2 * kNumTerrs;
        break;
    case 6:
        constant = kNumPlayers * kNumTerrs + kNumPlayers + 4 + 9 + 3 * kNumTerrs;
        break;
    case 7:
        constant = kNumPlayers * kNumTerrs + kNumPlayers + 4 + 9 + 4 * kNumTerrs;
        break;
    }
    for (int i = 0; i < kNumTerrs; ++i) {
        if (board[constant + i] == 1) {
            return i;
        }
    }
    return kNumTerrs;
}

void RiskState::SetAtkNum(int num) {
    board[kNumPlayers * kNumTerrs + kNumPlayers + 3 + 9 + 3 * kNumTerrs] = num;
    SetChance(1);
}

int RiskState::GetAtkNum() const {
    return board[kNumPlayers * kNumTerrs + kNumPlayers + 3 + 9 + 3 * kNumTerrs];
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
    board[kNumPlayers * kNumTerrs + kNumPlayers + 5 + 9 + 5 * kNumTerrs + (kNumTerrs + 2) * player + card_coord] = amount;
}

bool RiskState::GetCard(int player, int card_coord) const{
    return board[kNumPlayers * kNumTerrs + kNumPlayers + 5 + 9 + 5 * kNumTerrs + (kNumTerrs + 2) * player + card_coord];
}

void RiskState::ResetHand(int player) {
    for (int i = 0; i < kNumTerrs + 2; ++i) {
        SetCard(player, i, 0);
    }
}

std::array<bool, kNumTerrs + 2> RiskState::GetHand(int player) const{
    std::array<bool, kNumTerrs + 2> res = { 0 };
    for (int i = 0; i < kNumTerrs + 2; ++i) {
        res[i] = GetCard(player,i);
    }
    return res;
}

std::array<bool, (kNumTerrs + 2)> RiskState::GetCards() const {
    std::array<bool, (kNumTerrs + 2)> res = { 0 };
    for (int i = 0; i < kNumPlayers; ++i) {
        auto hand = GetHand(i);
        for (int j = 0; j < kNumTerrs + 2; ++j) {
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
    for (int i = 0; i < kNumPlayers; ++i) {
        if (board[i * kNumTerrs + coord] != 0) {
            return i;
        }
    }
    return kNumPlayers;
}

std::array<int,kNumTerrs> RiskState::GetTroopArr(int player) const{
    std::array<int, kNumTerrs> res = { 0 };
    for (int i = 0; i < kNumTerrs; ++i) {
        if (board[kNumTerrs * player + i] > 0) {
            res[i] = board[kNumTerrs * player + i];
        }
    }
    return res;
}

std::array<bool, kNumTerrs> RiskState::GetTroopMask(int player) const{
    std::array<bool, kNumTerrs> res = { 0 };
    for (int i = 0; i < kNumTerrs; ++i) {
        if (board[kNumTerrs * player + i] > 0) {
            res[i] = 1;
        }
    }
    return res;
}

int RiskState::GetEliminated(int player) const{
    for (int i = 0; i < kNumPlayers - 1; ++i) {
        if (board[2 * kNumPlayers * kNumTerrs + 5 + 9 + 5 * kNumTerrs + 3 * kNumPlayers +player*(kNumPlayers-1)+i] == 1) {
            return i+1;
        }
    }
    return 0;
}

int RiskState::GetMaxElim() const {
    int max = 0;
    for (int i = 0; i < kNumPlayers; ++i) {
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
    for (int i = 0; i < kNumTerrs+2; ++i) {
        board[kNumPlayers * kNumTerrs + kNumPlayers + 5 + 9 + 5 * kNumTerrs + (kNumTerrs + 2) *victor + i ]=victim_hand[i];
        ResetHand(victim);
    }
    int max = GetMaxElim();
    board[2 * kNumPlayers * kNumTerrs + 5 + 9 + 5 * kNumTerrs + 3 * kNumPlayers + victim* (kNumPlayers - 1) + max] = 1;
}

void RiskState::Deal() {
    int player = GetPlayer();
    assert(GetChance()==1);
    std::vector<int> internal_deck = {};
    std::array<int,kNumTerrs+2> cards = GetCards();
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
        if (sat_cards[i] < kNumTerrs &&bonus ==false) {
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
        std::array<bool,kNumTerrs> troop_mask = GetTroopMask(GetPlayer());
        int terrs = 0;
        for (int i = 0; i < kNumTerrs; ++i) {
            if (troop_mask[i]) {
                terrs += 1;
            }
        }
        int base = std::max((int)std::floor(terrs / 3), 3);
        int continent_bonus = 0;
        int ast_bonus = 0;
        for (int i = 0; i < kNumContinents; ++i) {
            bool sat = true;
            for (int j = 0; j < kNumTerrs; ++j) {
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
    int n_def = board[kNumTerrs*def_player+coord_to];
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
    for (int i = 0; i < kNumTerrs; ++i) {
        if (board[kNumTerrs * def_player + i] != 0) {
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

void RiskState::DepthFirstSearch(int player, int vertex,std::array<bool,kNumTerrs>* out) const{
    (*out)[vertex]=1;
    for (int i = 0; i < kNumTerrs;++i) {
        if (kAdjMatrix[vertex][i] &&board[player*kNumTerrs+i]!=0&& !(*out)[i] ){
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
    return (int)std::floor((action + 1) * num / abs);
}
//this could be very stupid and bad//
RiskState::RiskState(std::shared_ptr<Game>game)
: State(game),board{0},
move_number_(0),
num_distinct_actions(kPhseConstants[9]){
    SetIncome(1);
    SetPlayer(0);
    SetPhse(0);
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
			for (int i = 0; i < kNumTerrs; ++i) {
				for (int j = 0; j < kNumTerrs; ++j) {
					if (attackable_mask[i] && anti_mask[j] && kAdjMatrix[i][j]) {
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

std::vector<double> RiskState::Rewards() const{
    assert(IsTerminal());
    std::vector<double> res = { };
    std::array<int, kNumPlayers> arr = { 0 };
    int split = 0;
    int split_rewards = 0;
    for (int i = 0; i < kNumPlayers; ++i) {
        arr[i] = GetEliminated(i);
        if (arr[i] == 0) {
            split_rewards += kRewards[kNumPlayers - split - 1];
            split += 1;
        }
    }
    double split_reward = (double)split_rewards / split;
    for (int i = 0; i < kNumPlayers; ++i) {
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
        for (int i = 0; i < kNumPlayers; ++i) {
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
   if (GetMaxElim() == kNumPlayers || GetTurns() >= kMaxTurns) {
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
    return ActionsAndProbs{ std::make_pair(kPhseConstants[9], 1) };
}

std::vector<Action> RiskState::LegalChanceOutcomes() const{
    if (IsChanceNode()) {
        return { kPhseConstants[9] };
   }
    else {
        return {};
    }
}

std::unique_ptr<RiskState> RiskState::Clone() const {
    return std::unique_ptr<RiskState>(new RiskState(*this));
}

void RiskState::ObservationTensor(Player player,
                                  absl::Span<float> values) const {
  ContiguousAllocator allocator(values);
  const RiskGame& game = open_spiel::down_cast<const RiskGame&>(*game_);
  game.default_observer_->WriteTensor(*this, player, &allocator);
}



RiskGame::RiskGame(const GameParameters& params)
    : Game(kGameType, params), num_players_(ParameterValue<int>("players")) {
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
