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

#ifndef OPEN_SPIEL_GAMES_RISK_H_
#define OPEN_SPIEL_GAMES_RISK_H_

#include <array>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>

#include "open_spiel/policy.h"
#include "open_spiel/spiel.h"
#include "open_spiel/spiel_utils.h"

// A simple game that includes chance and imperfect information
// http://en.wikipedia.org/wiki/Kuhn_poker
//
// For more information on this game (e.g. equilibrium sets, etc.) see
// http://poker.cs.ualberta.ca/publications/AAAI05.pdf
//
// The multiplayer (n>2) version is the one described in
// http://mlanctot.info/files/papers/aamas14sfrd-cfr-kuhn.pdf
//
// Parameters:
// none rn because am stupid but maybe later all the varst that are used as constants like ast_vect and the map could be params; bn  

//current documentation on form of board attribute ie the world state//
//[0:kNumPlayers*kNumTerrs]=player troops
//[kNumPlayers*kNumTerrs:kNumPlayers*kNumTerrs+kNumPlayers]=whose turn it is
//[kNumPlayers*kNumTerrs+kNumPlayers]=turns elapsed
//[kNumPlayers*kNumTerrs+kNumPlayers+1:kNumPlayers*kNumTerrs+kNumPlayers+1+phses]=cur_phse
//[kNumPlayers*kNumTerrs+kNumPlayers+1+phses]= is chance node
//[kNumPlayers*kNumTerrs+kNumPlayers+2+phses]=current troop income
//[kNumPlayers*kNumTerrs+kNumPlayers+3+phses:kNumPlayers*kNumTerrs+kNumPlayers+3+phses+kNumTerrs]=dep to
//[kNumPlayers*kNumTerrs+kNumPlayers+3+phses+kNumTerrs:kNumPlayers*kNumTerrs+kNumPlayers+3+phses+2*kNumTerrs]=atk from
//[kNumPlayers*kNumTerrs+kNumPlayers+3+phses+2*kNumTerrs:kNumPlayers*kNumTerrs+kNumPlayers+3+phses+3*kNumTerrs]=atk to
//[kNumPlayers*kNumTerrs+kNumPlayers+3+phses+3*kNumTerrs]=atk amount
//[kNumPlayers*kNumTerrs+kNumPlayers+4+phses+3*kNumTerrs:kNumPlayers*kNumTerrs+kNumPlayers+4+phses+4*kNumTerrs]=fort from
//[kNumPlayers*kNumTerrs+kNumPlayers+4+phses+4*kNumTerrs:kNumPlayers*kNumTerrs+kNumPlayers+4+phses+5*kNumTerrs]=fort to
//[kNumPlayers*kNumTerrs+kNumPlayers+4+phses+5*kNumTerrs]= succ atk
//[kNumPlayers*kNumTerrs+kNumPlayers+5+phses+5*kNumTerrs:2*kNumPlayers*kNumTerrs+5+phses+5*kNumTerrs+3*kNumPlayers]=cards
//[2*kNumPlayers*kNumTerrs+5+phses+5*kNumTerrs+3*kNumPlayers:2*kNumPlayers*kNumTerrs+5+phses+5*kNumTerrs+3*kNumPlayers+kNumPlayers(kNumPlayers-1)]=death array note this might need to be modified for the obs as it is a mixture of categorical and ordinal rep


//default observation ie including private and public info
//0:kNumPlayers*kNumTerrs]=player troops
//[kNumPlayers*kNumTerrs:kNumPlayers*kNumTerrs+kNumPlayers]=whose turn it is
//[kNumPlayers*kNumTerrs+kNumPlayers]=turns elapsed
//[kNumPlayers*kNumTerrs+kNumPlayers+1:kNumPlayers*kNumTerrs+kNumPlayers+10]=cur_phse
//[kNumPlayers*kNumTerrs+kNumPlayers+10]= is chance node
//[kNumPlayers*kNumTerrs+kNumPlayers+11]=current troop income
//[kNumPlayers*kNumTerrs+kNumPlayers+12:kNumPlayers*kNumTerrs+kNumPlayers+12+kNumTerrs]=dep to
//[kNumPlayers*kNumTerrs+kNumPlayers+12+kNumTerrs:kNumPlayers*kNumTerrs+kNumPlayers+12+2*kNumTerrs]=atk from
//[kNumPlayers*kNumTerrs+kNumPlayers+12+2*kNumTerrs:kNumPlayers*kNumTerrs+kNumPlayers+12+3*kNumTerrs]=atk to
//[kNumPlayers*kNumTerrs+kNumPlayers+12+3*kNumTerrs]=atk amount
//[kNumPlayers*kNumTerrs+kNumPlayers+13+3*kNumTerrs:kNumPlayers*kNumTerrs+kNumPlayers+13+4*kNumTerrs]=fort from
//[kNumPlayers*kNumTerrs+kNumPlayers+13+4*kNumTerrs:kNumPlayers*kNumTerrs+kNumPlayers+13+5*kNumTerrs]=fort to
//[kNumPlayers*kNumTerrs+kNumPlayers+13+5*kNumTerrs]= succ atk
//[kNumPlayers*kNumTerrs+kNumPlayers+14+5*kNumTerrs:kNumPlayers*kNumTerrs+kNumPlayers+16+6*kNumTerrs] = private cards
//[kNumPlayers*kNumTerrs+kNumPlayers+16+6*kNumTerrs:kNumPlayers*kNumTerrs+2*kNumPlayers+16+6*kNumTerrs]= cards total(public)
//[kNumPlayers*kNumTerrs+2*kNumPlayers+16+6*kNumTerrs:kNumPlayers*kNumTerrs+2*kNumPlayers+16+6*kNumTerrs+kNumPlayers(kNumPlayers-1)]=death_arr


namespace open_spiel {
namespace risk {

//constants
    const std::vector<std::vector<int>> SimpleAdjVect{ {0,2,11} ,{0,2,18},{ 3,16} ,{4,14,16},{5,7,14} ,{4,6,14,15},{5,7},{4,6,8},{7,9} ,{8,10} ,{11,12} ,{0,9,12},{10,11,13},{12,14,17,16},{3,4,5,13,15,16},{5,14},{2,3,13,14,17},{13,16,18},{17,1} };
    const std::vector<std::vector<int>> SimpleContVect{ {0,1} ,{2,3,4,16},{5,14,15},{6,7,8,9} , {10,11,12},{13,17,18} };
    const std::array<int, 4> SimpleCardArr = { 6,12,19,21 };
    const std::vector<int> SimpleContBonus = { 1,5,3,3,2,2 };
    const std::vector<std::string> SimpleTerrNames = { "ALPHA","BRAVO","CHARLIE","DELTA","ECHO","FOXTROT","GOLF","HOTEL","INDIA","JULIETT","KILO","LIMA","MIKE","NOVEMBER","OSCAR","PAPA","QUEBEC","ROMEO","SIERRA" };
    const std::vector<std::vector<int>> ClassicAdjVect{ {1, 2},{0, 2, 3},{0, 1, 3},{1, 2, 4},{3, 5, 6},{6, 7, 8, 10, 11},{4, 5, 8, 34},{5, 9, 11, 12, 14},{5, 6, 10, 40},{7, 14},{5, 8, 11, 40},{5, 7, 10, 12, 13},{7, 11, 13, 14},{11, 12, 14},{7, 9, 12, 13, 15},{14, 16, 20},{15, 17, 19, 20},{16, 18, 19, 38},{17, 19, 22},{16, 17, 18, 22, 21, 20},{15, 16, 19, 21},{19, 20, 22, 23},{18, 19, 21, 23},{21, 22, 24},{23, 25, 27},{24, 26, 27},{25, 27},{24, 25, 26, 28},{27, 29, 33, 35, 36},{28, 30, 32},{29, 31, 32},{30, 32},{29, 30, 33, 34},{28, 32, 34, 35},{6, 32, 33, 40},{28, 33, 34, 36, 40, 41},{28, 35, 37, 41},{36, 38, 39, 41},{37, 39},{37, 38, 40, 41},{8, 10, 34, 35, 39, 41},{35, 36, 37, 39, 40} };
    const std::vector<std::vector<int>> ClassicContVect{ {0, 1, 2, 3},{4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 34},{15, 16, 17, 18, 19, 20, 21, 22, 23},{24, 25, 26, 27},{28, 29, 30, 31, 32, 33},{35, 36, 37, 38, 39, 40, 41} };
    const std::vector<int> ClassicContBonus = { 2, 7, 5, 2, 3, 5 };
    const std::array<int, 4> ClassicCardArr = { 14,28,42,44 };
    const std::vector<std::string> ClassicTerrNames = { "East Aus.", "West Aus.", "New Guinea", "Indonesia", "Siam", "China", "India", "Mongolia", "Afghanistan", "Japan", "Ural", "Siberia", "Irkutsk", "Yakutsk", "Kamchatka", "Alaska", "N.W. Territory", "Greenland", "Quebec", "Ontario", "Alberta", "West U.S", "East U.S", "C. America", "Venezuela", "Peru", "Argentina", "Brazil", "N. Africa", "C. Africa", "S. Africa", "Madagascar", "E. Africa", "Egypt", "Middle East", "S. Europe", "W. Europe", "Great Britain", "Iceland", "Scandinavia", "Ukraine", "N. Europe" };



class RiskGame;
class RiskObserver;

class RiskState : public State {
 public:
  
  //internal methods//


  //open spiel things//   
  explicit RiskState(std::shared_ptr<const Game> game);
  RiskState(const RiskState&) = default;
  Player CurrentPlayer() const override;
  bool IsTerminal() const override;
  std::vector<double> Returns() const override;
  void ObservationTensor(Player player,
                         absl::Span<float> values) const override;
  std::unique_ptr<State> Clone() const override;
  std::vector<std::pair<Action, double>> ChanceOutcomes() const override;
  std::vector<Action> LegalChanceOutcomes() const override;
  std::vector<Action> LegalActions() const override;
 protected:
  void DoApplyAction(Action move) override;
  int num_terrs_;
  int starting_troops_;
  std::vector<int> rewards_;
  std::vector<int> assist_;
  std::vector<int> board;
  std::vector<std::vector<bool>> adj_matrix_;
  std::vector<std::vector<bool>> cont_matrix_;
  std::vector<int> cont_bonus_;
  std::array<int, 4> card_arr;
  std::array<bool, 4> abstraction_;
  std::array<int, 4> action_q_;//for no abstraction represents how many troops from 1 to that number that can be utilised, if abstraction then it represents number of bins
  std::array<int, 10> phse_constants_;
 private:
  friend class RiskObserver;
};

class RiskGame : public Game {
 public:
  explicit RiskGame(const GameParameters& params);
  int NumDistinctActions() const override { return num_distinct_actions_; }
  std::unique_ptr<State> NewInitialState() const override;
  int MaxChanceOutcomes() const override { return 1; }
  int NumPlayers() const override { return num_players_; }
  double MinUtility() const override { return rewards_[0]; };
  double MaxUtility() const override { return rewards_[num_players_-1]; };
  double UtilitySum() const override { return 0; }
  std::vector<int> ObservationTensorShape() const override;
  int MaxGameLength() const override { return max_game_length_; }
  int MaxChanceNodesInHistory() const override { return max_chance_nodes_in_history_;}
  
  std::shared_ptr<Observer> MakeObserver(
      absl::optional<IIGObservationType> iig_obs_type,
      const GameParameters& params) const override;

  // Used to implement the old observation API.
  std::shared_ptr<RiskObserver> default_observer_;
  std::shared_ptr<RiskObserver> public_observer_;
  std::shared_ptr<RiskObserver> private_observer_;

 private:
  // Number of players.
     int num_players_;
     std::vector<std::vector<int>> adj_;
     std::vector<std::vector<int>> cont_;
     std::vector<int> cont_bonus_;
     int max_turns_;
     int num_distinct_actions_;
     int max_chance_nodes_in_history_;
     int max_game_length_;
     std::vector<int> rewards_;
     std::vector<int> assist_;
     std::array<bool, 4> abstraction_;
     std::array<int, 4> action_q_;
     std::array<int, 4> card_arr_;
     std::vector<std::string> terr_names_;

};
}  // namespace risk
}  // namespace open_spiel

#endif  // OPEN_SPIEL_GAMES_RISK_H
