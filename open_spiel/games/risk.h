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
namespace open_spiel {
namespace risk {

//constants
    const std::vector<std::vector<int>> SimpleAdjVect{ {0,2,11} ,{0,2,18},{ 3,16} ,{4,14,16},{5,7,14} ,{4,6,14,15},{5,7},{4,6,8},{7,9} ,{8,10} ,{11,12} ,{0,9,12},{10,11,13},{12,14,17,16},{3,4,5,13,15,16},{5,14},{2,3,13,14,17},{13,16,18},{17,1} };
    const std::vector<std::vector<int>> SimpleContinentVect{ {0,1} ,{2,3,4,16},{5,14,15},{6,7,8,9} , {10,11,12},{13,17,18} };
    const std::array<int, 4> SimpleCardArr = { 6,12,19,21 };
    const std::array<int, 6> SimpleContinentArr = { 2, 4, 3, 4, 3, 3 };
    const std::array<int, 6> SimpleContinentBonusArr = { 1,5,3,3,2,2 };
    const std::array<std::string, 19> SimpleTerrNames = { "ALPHA","BRAVO","CHARLIE","DELTA","ECHO","FOXTROT","GOLF","HOTEL","INDIA","JULIETT","KILO","LIMA","MIKE","NOVEMBER","OSCAR","PAPA","QUEBEC","ROMEO","SIERRA" };
    const std::vector<std::vector<int>> ClassicAdjVect{ {1, 2},{0, 2, 3},{0, 1, 3},{1, 2, 4},{3, 5, 6},{6, 7, 8, 10, 11},{4, 5, 8, 34},{5, 9, 11, 12, 14},{5, 6, 10, 40},{7, 14},{5, 8, 11, 40},{5, 7, 10, 12, 13},{7, 11, 13, 14},{11, 12, 14},{7, 9, 12, 13, 15},{14, 16, 20},{15, 17, 19, 20},{16, 18, 19, 38},{17, 19, 22},{16, 17, 18, 22, 21, 20},{15, 16, 19, 21},{19, 20, 22, 23},{18, 19, 21, 23},{21, 22, 24},{23, 25, 27},{24, 26, 27},{25, 27},{24, 25, 26, 28},{27, 29, 33, 35, 36},{28, 30, 32},{29, 31, 32},{30, 32},{29, 30, 33, 34},{28, 32, 34, 35},{6, 32, 33, 40},{28, 33, 34, 36, 40, 41},{28, 35, 37, 41},{36, 38, 39, 41},{37, 39},{37, 38, 40, 41},{8, 10, 34, 35, 39, 41},{35, 36, 37, 39, 40} };
    const std::vector<std::vector<int>> ClassicContinentVect{ {0, 1, 2, 3},{4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 34},{15, 16, 17, 18, 19, 20, 21, 22, 23},{24, 25, 26, 27},{28, 29, 30, 31, 32, 33},{35, 36, 37, 38, 39, 40, 41} };
    const std::array<int, 6> ClassicContinentArr = { 4, 12, 9, 4, 6, 7 };
    const std::array<int, 6> ClassicContinentBonusArr = { 2, 7, 5, 2, 3, 5 };
    const std::array<int, 4> ClassicCardArr = { 14,28,42,44 };
    const std::array<std::string, 42> ClassicTerrNames = { "East Aus.", "West Aus.", "New Guinea", "Indonesia", "Siam", "China", "India", "Mongolia", "Afghanistan", "Japan", "Ural", "Siberia", "Irkutsk", "Yakutsk", "Kamchatka", "Alaska", "N.W. Territory", "Greenland", "Quebec", "Ontario", "Alberta", "West U.S", "East U.S", "C. America", "Venezuela", "Peru", "Argentina", "Brazil", "N. Africa", "C. Africa", "S. Africa", "Madagascar", "E. Africa", "Egypt", "Middle East", "S. Europe", "W. Europe", "Great Britain", "Iceland", "Scandinavia", "Ukraine", "N. Europe" };
    const int kNumTerrs = ClassicNumTerrs;
    const int kNumContinents = 6;
    const int kNumPlayers = 6;
    std::array<bool, kNumTerrs> AdjMatrixer(std::vector<int> v);
    std::array<bool, kNumContinents> ContinentMatrixer(std::vector<int> v);
    const kAdjMatrix = AdjMatrixer(ClassicAdjVect);
    const kContinentMatrix = ContinentMatrixer(ClassicContinentVect);
    const std::array<int, kNumPlayers> kAstArr = { 0,0,0,1,2,3 };
    const std::array<int, kNumPlayers> kRewards = { -1,-1,-1,0,1,2 };
    const bool kDepAbs = false;
    const bool kAtkAbs = false;
    const bool kRedistAbs = false;
    const bool kFortAbs = false;
    const int kDepMax = 31;
    const int kAtkMax = 1000;
    const int kRedistMax = 1000;
    const int kFortMax = 1000;
    const std::array<int, 10> kPhseConstants = { 0,kNumTerrs + 1,kNumTerrs + 1 + kDepMax,2 * kNumTerrs + 2 + kDepMax,3 * kNumTerrs + 2 + kDepMax,3 * kNumTerrs + 2 + kDepMax + kAtkMax,3 * kNumTerrs + 3 + kDepMax + kAtkMax + kRedistMax,4 * kNumTerrs + 4 + kDepMax + kAtkMax + kRedistMax,5 * kNumTerrs + 4 + kDepMax + kAtkMax + kRedistMax,5 * kNumTerrs + 4 + kDepMax + kAtkMax + kRedistMax + kFortMax };
    const int kMaxGameLength = 10000;
    const int kMaxChanceNodesInHistory = 1000;

enum ActionType { kPass = 0, kBet = 1 };

class RiskGame;
class RiskObserver;

class RiskState : public State {
 public:
  
  //internal methods//
  void ResetPlayer();//sets the current player entries in the board array to all zero;
  void SetPlayer(Player player);//sets the player entry in the board array to one;
  void SetNextPlayer();//sets the present player entry to 0 and sets the next(consecutive player) entry to one;
  int GetPlayer() const;//gets the current player entry =1;
  int GetTurns() const;//gets the current turn entry;
  void IncrementTurns(int increment); //increments the current turn entry by the increment;
  void ResetPhse(); //sets all the phse entries in the board to zero;
  void SetPhse(int phse);//sets the phse entry to 1;
  int GetPhse() const;//gets the phse entry containing 1;
  void SetIncome(int income);//sets the income entry to income;
  int GetIncome() const;//gets the income entry;
  void IncrementIncome();//increments the income entry;
  void SetChance(int chance);//sets the chance entry to chance;
  int GetChance() const;//gets the chance entry;
  void SetTerr(int coord, int player, int troops);//sets the players troop amount in that terr to troops;
  void IncrementTerr(int coord, int player, int troops);//increments the players troop amount in that terr by troops;
  void SetSucc(int val);//sets the successful attack entry to val;
  int GetSucc() const;//gets the successful attack entry;
  void ResetActions();//resets all the action entries in the board to 0;
  void SetCoord(int coord);//given the current phse it sets the coord in the correct action entry;
  int GetCoord(int phse) const;//returns the action for a given phse choice;
  void SetAtkNum(int num);//sets the atknum entry to num;
  int GetAtkNum() const;//gets the atknum entry;
  void SetDeploy();//sets the board into deploy mode (ie at the start of a seq of deploy actions;
  void SetAttack();//sets the board into atk mode (ie at the start of a seq of atk actions;
  void SetRedistribute();//sets the board into redist mode (ie at the start of a seq of redist actions;
  void SetFortify();//sets the board into fort mode (ie at the start of a seq of fort actions;
  void SetCard(int player, int card_coord, int amount);//sets the players card entry to amount;
  bool GetCard(int player, int card_coord) const;//gets the players card entry;
  void ResetHand(int player);//sets all player's cards to 0;
  std::array<bool, kNumTerrs + 2> GetHand(int player) const;//Gets all the player's cards;
  std::array<bool, kNumTerrs + 2> GetCards() const;//Gets all the cards in all the players's hands;
  int GetCardSig(int card_coord) const;//Gets a cards prime signature(for cashing);
  int GetHandSig(int player) const;//Gets the product of all the cardsigs in player's hand;
  int GetHandSum(int player) const;//gets the amount of cards in player's hand;
  bool GetCashable(int player) const;//is player's hand cashable;
  std::array<int, 3> GetSatCards(std::array<int, 3> component_arr) const;//returns the cards to be cashed;
  int GetOwner(int coord) const;//returns the owner of a territory;
  std::array<int, kNumTerrs> GetTroopArr(int player) const;//returns player's troop entries;
  std::array<bool, kNumTerrs> GetTroopMask(int player) const;//returns all player's troop entries;
  int GetEliminated(int player) const;//if player is not eliminated returns 0, else returns when they were eliminated ie first second third etc;
  int GetMaxElim() const;//returns the highest elim value ie how many players have been eliminated;
  void EndTurn();//ends player's turn;
  void Eliminate(int victim, int victor);//eliminates a player giving their cards to the eliminator + some other logic;
  void Deal();//deals cards at end of turn to player if succ_atk;
  void Cash();//cashes cards for player;
  void Income();//handles giving players their income;
  void Deploy(int amount);//deploys amount to selected terr;
  void Attack();//handles the attack process which is random;
  void Redistribute(int amount);//redistributes the amount from where they attacked to to where they attacked from if they so chose;
  void Fortify(int amount);//fortifies the amount from where they fort_from and to fort_to, then ends their turn;
  void DepthFirstSearch(int player, int vertex, std::array<bool, kNumTerrs>* out) const;//is a depth first search to find if a fort to is legal
  std::vector<int> GetAbstraction(int num, int abs) const;//pretty hacky way to abstract actions;
  int RetAbstraction(int action, int abs) const;//given an action returns what it represents;


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

 private:
  friend class RiskObserver;
  int num_terrs_;
  int num_continents;
  int starting_troops;
  std::vector<int> rewards;
  std::vector<int> assist;
  std::array<int, 2 * kNumPlayers * kNumTerrs + 14 + 2 * kNumPlayers + kNumPlayers * kNumPlayers + 5 * kNumTerrs> board;
  std::array<std::array<bool,num_terrs>,num_terrs> adj_matrix;
  std::array<std::array<bool, num_terrs>, num_terrs> cont_matrix;
  std::array<int, num_continents> cont_bonus;
  std::array<int, 4> card_arr;
  std::array<bool, 4> abstraction;
  std::array<int, 4> action_q;//for no abstraction represents how many troops from 1 to that number that can be utilised, if abstraction then it represents number of bins
  std::array<int, 10> phse_constants;
};

class RiskGame : public Game {
 public:
  explicit KuhnGame(const GameParameters& params);
  int NumDistinctActions() const override { return num_distinct_actions_; }
  std::unique_ptr<State> NewInitialState() const override;
  int MaxChanceOutcomes() const override { return 1; }
  int NumPlayers() const override { return num_players_; }
  double MinUtility() const override { return std::min(rewards_); };
  double MaxUtility() const override { return std::max(rewards_); };
  double UtilitySum() const override { return 0; }
  std::vector<int> ObservationTensorShape() const override;
  int MaxGameLength() const override { return max_game_length_; }
  int MaxChanceNodesInHistory() const override { return max_chance_nodes_in_history_;}
  int MapType() const { return map_type_; }
  int NumTerrs() const { return num_terrs_; }
  std::vector<int> Rewards() const { return rewards_; }
  std::vector<int> Assist() const { return assist_; }
  std::array<bool, 4> Abstraction()const { return abstraction_; }
  std::array<int, 4> ActionQ() const { return action_q_; }
  
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
     int num_terrs_;
     int max_turns_;
     int num_distinct_actions_;
     int max_chance_nodes_in_history_;
     int max_game_length_;
     std::vector<int> rewards_;
     std::vector<int> assist_;
     std::array<bool, 4> abstraction_;
     std::array<int, 4> action_q_;
        

};
}  // namespace risk
}  // namespace open_spiel

#endif  // OPEN_SPIEL_GAMES_RISK_H
