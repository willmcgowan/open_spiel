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
  
  //internal methods
 void ResetPlayer();//resets all the player entries in board to 0
 void SetPlayer(int player);//sets the player entry to 1
 void SetNextPlayer();//sets the next player to 1 and sets all others to 0
 int GetPlayer() const;//gets the current player
 int GetTurns() const;// gets the current no of elapsed turns
 void IncrementTurns(int increment);//increments the no of elapsed turns
 void ResetPhse();//resets the current phse entry in board
 void SetPhse(int phse);//sets the current phse entry in board
 int GetPhse() const;//gets the current phse entry in board
 void SetIncome(int income);//sets the income entry in board
 int GetIncome() const;//gets the current income 
 void IncrementIncome(int increment);//increments the current income
 void SetChance(int chance);//sets the chance entry to chance
 int GetChance() const;//gets the current chance entry
 void SetTerr(int coord, int player, int troops);//sets a terr for a player to troops
 void IncrementTerr(int coord, int player, int increment);//increments a terr for a player
 void SetSucc(int val);//sets the successful attack entry to val
 int GetSucc() const;//gets the successful attack entyr
 void ResetActions();//resets all the action entries 
 void SetCoord(int coord);//sets the coord in the correct place given the phase
 int GetCoord(int phse) const;//gets the coord for a given phse
 void SetAtkNum(int num);//sets the current atk num
 int GetAtkNum() const;//gets the current atk num
 void SetDeploy();//sets the board to deploy mode
 void SetAttack();//sets the board to attack mode
 void SetRedistribute();//sets the board to redist mode
 void SetFortify();//sets the board to fortify mode
 void SetCard(int player, int card_coord, int amount);//sets a card in a player's hand to amount
 bool GetCard(int player, int card_coord) const;//gets a card in a player's hand
 void ResetHand(int player);//resets a player's hand
 std::vector<bool> GetHand(int player) const;//gets a player's hand
 std::vector<bool> GetHands() const;//gets hands
 std::vector<bool> GetCards() const;//gets all the cards held in hands
 int GetCardSig(int card_coord) const;//gets the prime signature of a card
 int GetHandSig(int player) const;//gets the product of the card signatures of hand of a player
 int GetHandSum(int player) const;//gets the total num of cards in a player's hand
 bool GetCashable(int player) const;//gets if a player's hand is cashable
 std::array<int, 3> GetSatCards(std::array<int, 3> component_arr) const;//gets the cards to cash from a hand
 int GetOwner(int coord) const;//gets the owner of a territory
 std::vector<int> GetTroopArr(int player) const;//gets the troop array of a player
 std::vector<bool> GetTroopMask(int player) const;//gets the troop mask of a player
 int GetEliminated(int player) const;//gets the position a player was eliminated 0 for uneliminated 1 for 1st to be eliminated etc.
 int GetMaxElim() const;//gets the total num of elims
 void EndTurn();//ends the turn of the current player and mutates board accordingly
 void Eliminate(int victim, int victor);//handles the player elimination process
 void Deal();//handles the card dealing process
 void Cash();//handles the card cashing process
 void Income();//handles the income distrib process
 void Deploy(int amount);//handles the act of deploying
 void Attack();//handles the attack process
 void Redistribute(int amount);//handles the redistrib process
 void Fortify(int amount);//handles the fortify process
 void DepthFirstSearch(int player, int vertex, std::vector<bool>* out) const;//used to determine legal fortifies given a chosen territory to fortify from//
 std::vector<int> GetAbstraction(int num, int abs) const;//pretty hacky way to reduce number of actions
 int RetAbstraction(int action, int abs) const;//ditto


  //open spiel things//   
  explicit RiskState(std::shared_ptr<const RiskGame> game);
  RiskState(const RiskState&) = default;
  Player CurrentPlayer() const override;
  bool IsTerminal() const override;
  std::vector<double> Returns() const override;
  std::vector<double> Rewards() const override;
  bool IsChanceNode() const override;
  bool IsPlayerNode() const override;
  void ObservationTensor(Player player,
                         absl::Span<float> values) const override;
  std::unique_ptr<State> Clone() const override;
  std::vector<std::pair<Action, double>> ChanceOutcomes() const override;
  std::vector<Action> LegalChanceOutcomes() const override;
  std::vector<Action> LegalActions() const override;
  std::string ToString() const override {
      return "0";
  }
  std::string ActionToString(Player player, Action action_id) const override {
      return "0";
  }
 

protected:
  void DoApplyAction(Action move) override;
  int num_terrs_;
  int starting_troops_;
  int max_turns_;
  std::vector<double> rewards_;
  std::vector<int> assist_;
  std::vector<int> board;
  std::vector<std::vector<bool>> adj_matrix_;
  std::vector<std::vector<bool>> cont_matrix_;
  std::vector<int> cont_bonus_;
  std::array<int, 4> card_arr_;
  std::array<bool, 4> abstraction_;
  std::array<int, 4> action_q_;//for no abstraction represents how many troops from 1 to that number that can be utilised, if abstraction then it represents number of bins
  std::array<int, 10> phse_constants_;
  std::vector<std::string> terr_names_;
 private:
  friend class RiskObserver;
};

class RiskGame :public Game{
 public:
  explicit RiskGame(const GameParameters & params);
  int NumDistinctActions() const override { return num_distinct_actions_; }
  std::unique_ptr<State> NewInitialState() const override;
  int MaxChanceOutcomes() const override { return 1; }
  int NumPlayers() const override { return num_players_; }
  double MinUtility() const override { return rewards_[0]; };
  double MaxUtility() const override { return rewards_[num_players_ - 1]; };
  double UtilitySum() const override { return 0; }
  std::vector<int> ObservationTensorShape() const override;
  int MaxGameLength() const override { return max_game_length_; }
  int MaxChanceNodesInHistory() const override { return max_chance_nodes_in_history_; }
  int MaxTurns() const{return max_turns_;}
  int NumTerrs() const{return num_terrs_;}
  std::vector<std::vector<int>> Adj() const{return adj_;}
  std::vector<std::vector<int>> Cont() const{return cont_;}
  std::vector<int> ContBonus() const{return cont_bonus_;}
  std::vector<double> Rewards() const{return rewards_;}
  std::vector<int> Assist() const{return assist_;}
  std::array<bool, 4> Abstraction() const{return abstraction_;}
  std::array<int, 4> ActionQ() const{return action_q_;}
  std::array<int, 4> CardArr() const{return card_arr_;}
  std::vector<std::string> TerrNames() const{return terr_names_;}
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
        int map_type_;
        bool dep_abs_;
        bool atk_abs_;
        bool redist_abs_;
        bool fort_abs_;
        int dep_q_;
        int atk_q_;
        int redist_q_;
        int fort_q_;
        std::vector<std::vector<int>> adj_;
        std::vector<std::vector<int>> cont_;
        std::vector<int> cont_bonus_;
        int max_turns_;
        int num_terrs_;
        int num_distinct_actions_;
        int max_chance_nodes_in_history_;
        int max_game_length_;
        std::vector<double> rewards_;
        std::vector<int> assist_;
        std::array<bool, 4> abstraction_;
        std::array<int, 4> action_q_;
        std::array<int, 4> card_arr_;
        std::vector<std::string> terr_names_;
};
}  // namespace risk
}  // namespace open_spiel

#endif  // OPEN_SPIEL_GAMES_RISK_H
