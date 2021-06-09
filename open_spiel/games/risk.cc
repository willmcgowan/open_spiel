
#include "open_spiel/games/risk.h"

#include <ctime>
#include <algorithm>
#include <array>
#include <string>
#include <utility>
#include <iostream>
#include <unordered_map>
#include <assert.h>
#include <cmath>
#include "open_spiel/abseil-cpp/absl/hash/hash.h"
#include "open_spiel/abseil-cpp/absl/strings/str_cat.h"
#include "open_spiel/fog/fog_constants.h"
#include "open_spiel/game_parameters.h"
#include "open_spiel/observer.h"
#include "open_spiel/policy.h"
#include "open_spiel/spiel.h"
#include "open_spiel/spiel_utils.h"


namespace open_spiel
{

	namespace risk
	{
		namespace
		{

			// Default parameters.
			constexpr int kDefaultPlayers = 2;
			constexpr int kDefaultMap = 0;
			constexpr int kDefaultMaxTurns = 90;
			constexpr bool kDefaultDepAbs = false;
			constexpr bool kDefaultAtkAbs = false;
			constexpr bool kDefaultRedistAbs = false;
			constexpr bool kDefaultFortAbs = false;
			constexpr int kDefaultDepQ = 41;
			constexpr int kDefaultAtkQ = 1000;
			constexpr int kDefaultRedistQ = 1000;
			constexpr int kDefaultFortQ = 1000;
			//constexpr int kDefaultSeed = 1;

			// Facts about the game
			const GameType kGameType{
				/*short_name=*/"risk",
				/*long_name=*/"Risk",
				GameType::Dynamics::kSequential,
				GameType::ChanceMode::kSampledStochastic,
				GameType::Information::kImperfectInformation,
				GameType::Utility::kZeroSum,
				GameType::RewardModel::kTerminal,
				/*max_num_players=*/6,
				/*min_num_players=*/2,
				/*provides_information_state_string=*/false,
				/*provides_information_state_tensor=*/true,
				/*provides_observation_string=*/false,
				/*provides_observation_tensor=*/true,
				/*parameter_specification=*/
				{{"players", GameParameter(kDefaultPlayers)},{"rng_seed",GameParameter(kDefaultSeed)}, {"map", GameParameter(kDefaultMap)}, {"max_turns", GameParameter(kDefaultMaxTurns)}, {"dep_abs", GameParameter(kDefaultDepAbs)}, {"atk_abs", GameParameter(kDefaultAtkAbs)}, {"redist_abs", GameParameter(kDefaultRedistAbs)}, {"fort_abs", GameParameter(kDefaultFortAbs)}, {"dep_q", GameParameter(kDefaultDepQ)}, {"atk_q", GameParameter(kDefaultAtkQ)}, {"redist_q", GameParameter(kDefaultRedistQ)}, {"fort_q", GameParameter(kDefaultFortQ)}},
				/*default_loadable=*/true,
				/*provides_factored_observation_string=*/false,
			};

			std::shared_ptr<const Game> Factory(const GameParameters &params)
			{
				return std::shared_ptr<const Game>(new RiskGame(params));
			}

			REGISTER_SPIEL_GAME(kGameType, Factory);
		} // namespace

		//helper funcs for constructing game object//
		std::vector<std::vector<bool>> AdjMatrixer(int map)
		{
			std::vector<std::vector<int>> dict;
			if (map == 0) {
				dict = SimpleAdjVect;
			}
			else {
				dict = ClassicAdjVect;
			}
			std::vector<std::vector<bool>> res(dict.size(), std::vector<bool>(dict.size(), 0));
			for (int i = 0; i < dict.size(); ++i)
			{
				for (int j = 0; j < dict[i].size(); ++j)
				{
					res[i][dict[i][j]] = 1;
					res[dict[i][j]][i] = 1;
				}
			}
			return res;
		}
		std::vector<std::vector<bool>> ContMatrixer(int map)
		{
			std::vector<std::vector<int>> dict;
			int terrs ;
			if (map == 0) {
				dict = SimpleContVect;
				terrs = 19;
			}
			else {
				dict = ClassicContVect;
				terrs = 42;
			}
			std::vector<std::vector<bool>> res(dict.size(), std::vector<bool>(terrs, 0));
			for (int i = 0; i < dict.size(); ++i)
			{
				for (int j = 0; j < dict[i].size(); ++j)
				{
					res[i][dict[i][j]] = 1;
				}
			}
			return res;
		}
		std::vector<int> ContBonuser(int map) {
			if (map == 0) {
				return SimpleContBonus;
			}
			else {
				return ClassicContBonus;
			}
		}
		std::vector<double> Rewarder(int num_players) {
			return RewardsArr[num_players - 2];
		}
		std::vector<int> Assister(int num_players) {
			return AssistsArr[num_players - 2];
		}
		std::array<int,4> CardArrer(int map){
			if(map==0){
				return SimpleCardArr;
			}
			else{
				return ClassicCardArr;
			}
		}

		std::vector<std::string> TerrNamer(int map) {
			if (map == 0) {
				return SimpleTerrNames;
			}
			else {
				return ClassicTerrNames;
			}
		}

		class RiskObserver : public Observer
		{
		public:
			RiskObserver(IIGObservationType iig_obs_type)
				: Observer(/*has_string=*/false, /*has_tensor=*/true),
				  iig_obs_type_(iig_obs_type) {}

			void WriteTensor(const State &observed_state, int player,
							 Allocator *allocator) const override
			{
				const RiskState &state =
					open_spiel::down_cast<const RiskState &>(observed_state);
				SPIEL_CHECK_GE(player, 0);
				SPIEL_CHECK_LT(player, state.num_players_);
				const int num_players = state.num_players_;
				const int num_terrs = state.num_terrs_;

				//have added in asserts to check
				if (iig_obs_type_.private_info == PrivateInfoType::kSinglePlayer)
				{
					//the player in question's hand//
					auto out = allocator->Get("private_cards", {num_terrs + 2});
					auto hand = state.GetHand(player);
					for (int i = 0; i < num_terrs + 2; ++i)
					{
						assert(std::isfinite((float) hand[i]));
						out.at(i) = hand[i];
					}
				}

				if (iig_obs_type_.public_info && !iig_obs_type_.perfect_recall)
				{
					auto out = allocator->Get("board", {num_players * num_terrs + 2 * num_players + 14 + 5 * num_terrs + num_players * (num_players - 1)});
					for (int i = 0; i < num_players * num_terrs + num_players + 14 + 5 * num_terrs; ++i)
					{
						assert(std::isfinite((float)state.board[i]));
						out.at(i) = state.board[i];
					}
					for (int i = 0; i < num_players; ++i)
					{
						assert(std::isfinite((float) state.GetHandSum(i)));
						out.at(i + num_players * num_terrs + num_players + 14 + 5 * num_terrs) = state.GetHandSum(i);
					}
					for (int i = 0; i < num_players * (num_players - 1); ++i)
					{
						assert(std::isfinite((float) state.board[2 * num_players * num_terrs + 14 + 5 * num_terrs + 3 * num_players]));
						out.at(i + num_players * num_terrs + 2 * num_players + 14 + 5 * num_terrs) = state.board[2 * num_players * num_terrs + 14 + 5 * num_terrs + 3 * num_players];
					}
				}
			}

			std::string StringFrom(const State& observed_state,int player) const override {
				const RiskState& state =open_spiel::down_cast<const RiskState&>(observed_state);
				SPIEL_CHECK_GE(player, 0);
				SPIEL_CHECK_LT(player, state.num_players_);
				std::string result;
				result = "";
				return result;
			}

		private:
			IIGObservationType iig_obs_type_;
		};
		RiskState::RiskState(std::shared_ptr<const RiskGame> game) : State(game),risk_parent_game_(std::static_pointer_cast<const RiskGame>(game)),num_terrs_((int)game->adj_matrix_.size())
		{
			board = std::vector<int>(2 * num_players_ * num_terrs_ + 14 + 2 * num_players_ + num_players_ * num_players_ + 5 * num_terrs_, 0);
			phse_constants_ = {0, num_terrs_ + 1, num_terrs_ + 1 +risk_parent_game_->action_q_[0], 2 * num_terrs_ + 2 + risk_parent_game_->action_q_[0], 3 * num_terrs_ + 2 + risk_parent_game_->action_q_[0], 3 * num_terrs_ + 2 + risk_parent_game_->action_q_[0] + risk_parent_game_->action_q_[1], 3 * num_terrs_ + 3 + risk_parent_game_->action_q_[0] + risk_parent_game_->action_q_[1] + risk_parent_game_->action_q_[2], 4 * num_terrs_ + 4 + risk_parent_game_->action_q_[0] + risk_parent_game_->action_q_[1] + risk_parent_game_->action_q_[2], 5 * num_terrs_ + 4 + risk_parent_game_->action_q_[0] +risk_parent_game_->action_q_[1] + risk_parent_game_->action_q_[2]};
			SetIncome(1);
			SetPlayer(0);
			SetPhse(0);
		}
		void RiskState::ResetPlayer()
		{
			for (int i = 0; i < num_players_; ++i)
			{
				board[num_players_ * num_terrs_ + i] = 0;
			}
		}

		void RiskState::SetPlayer(int player)
		{
			ResetPlayer();
			board[num_players_ * num_terrs_ + player] = 1;
		}

		void RiskState::SetNextPlayer()
		{
			int cur_player = GetPlayer();
			int new_player = (cur_player + 1) % num_players_;
			board[num_players_ * num_terrs_ + cur_player] = 0;
			board[num_players_ * num_terrs_ + new_player] = 1;
			if (new_player == 0)
			{
				IncrementTurns(1);
			}
		}

		int RiskState::GetPlayer() const
		{
			for (int i = 0; i < num_players_; ++i)
			{
				if (board[num_players_ * num_terrs_ + i] == 1)
				{
					return i;
				}
			}
			return num_players_;
		}

		int RiskState::GetTurns() const
		{
			return board[num_players_ * num_terrs_ + num_players_];
		}
		void RiskState::IncrementTurns(int increment)
		{
			board[num_players_ * num_terrs_ + num_players_] += 1;
		}

		void RiskState::ResetPhse()
		{
			for (int i = 0; i < 9; ++i)
			{
				board[num_players_ * num_terrs_ + num_players_ + 1 + i] = 0;
			}
		}

		void RiskState::SetPhse(int phse)
		{
			ResetPhse();
			board[num_players_ * num_terrs_ + num_players_ + 1 + phse] = 1;
		}

		int RiskState::GetPhse() const
		{
			for (int i = 0; i < 9; ++i)
			{
				if (board[num_players_ * num_terrs_ + num_players_ + 1 + i] == 1)
				{
					return i;
				}
			}
			return 9;
		}

		void RiskState::SetIncome(int income)
		{
			board[num_players_ * num_terrs_ + num_players_ + 2 + 9] = income;
		}

		int RiskState::GetIncome() const
		{
			return board[num_players_ * num_terrs_ + num_players_ + 2 + 9];
		}

		void RiskState::IncrementIncome(int increment)
		{
			board[num_players_ * num_terrs_ + num_players_ + 2 + 9] += increment;
		}

		void RiskState::SetChance(int chance)
		{
			board[num_players_ * num_terrs_ + num_players_ + 1 + 9] = chance;
		}

		int RiskState::GetChance() const
		{
			return board[num_players_ * num_terrs_ + num_players_ + 1 + 9];
		}

		void RiskState::SetTerr(int coord, int player, int troops)
		{
			board[player * num_terrs_ + coord] = troops;
		}

		void RiskState::IncrementTerr(int coord, int player, int increment)
		{
			board[player * num_terrs_ + coord] += increment;
		}

		void RiskState::SetSucc(int val)
		{
			board[num_players_ * num_terrs_ + num_players_ + 4 + 9 + 5 * num_terrs_] = val;
		}

		int RiskState::GetSucc() const
		{
			return board[num_players_ * num_terrs_ + num_players_ + 4 + 9 + 5 * num_terrs_];
		}

		void RiskState::ResetActions()
		{
			for (int i = num_players_ * num_terrs_ + num_players_ + 3 + 9; i < num_players_ * num_terrs_ + num_players_ + 4 + 9 + 5 * num_terrs_; ++i)
			{
				board[i] = 0;
			}
		}

		void RiskState::SetCoord(int coord)
		{
			switch (GetPhse())
			{
			case 0:
				//dep_to//
				board[num_players_ * num_terrs_ + num_players_ + 3 + 9 + coord] = 1;
				SetPhse(1);
				break;
			case 2:
				//atk_from//
				board[num_players_ * num_terrs_ + num_players_ + 3 + 9 + num_terrs_ + coord] = 1;
				SetPhse(3);
				break;
			case 3:
				//atk_to/
				board[num_players_ * num_terrs_ + num_players_ + 3 + 9 + 2 * num_terrs_ + coord] = 1;
				SetPhse(4);
				break;
			case 6:
				//fort_from//
				board[num_players_ * num_terrs_ + num_players_ + 4 + 9 + 3 * num_terrs_ + coord] = 1;
				SetPhse(7);
				break;
			case 7:
				//fort_to//
				board[num_players_ * num_terrs_ + num_players_ + 4 + 9 + 4 * num_terrs_ + coord] = 1;
				SetPhse(8);
				break;
			}
		}

		int RiskState::GetCoord(int phse) const
		{
			int constant = 0;
			switch (phse)
			{
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
			for (int i = 0; i < num_terrs_; ++i)
			{
				if (board[constant + i] == 1)
				{
					return i;
				}
			}
			return num_terrs_;
		}

		void RiskState::SetAtkNum(int num)
		{
			board[num_players_ * num_terrs_ + num_players_ + 3 + 9 + 3 * num_terrs_] = num;
			SetChance(1);
		}

		int RiskState::GetAtkNum() const
		{
			return board[num_players_ * num_terrs_ + num_players_ + 3 + 9 + 3 * num_terrs_];
		}

		void RiskState::SetDeploy()
		{
			ResetActions();
			SetPhse(0);
		}

		void RiskState::SetAttack()
		{
			ResetActions();
			SetPhse(2);
		}

		void RiskState::SetRedistribute()
		{
			SetPhse(5);
		}

		void RiskState::SetFortify()
		{
			ResetActions();
			SetPhse(6);
		}

		void RiskState::SetCard(int player, int card_coord, int amount)
		{
			board[num_players_ * num_terrs_ + num_players_ + 5 + 9 + 5 * num_terrs_ + (num_terrs_ + 2) * player + card_coord] = amount;
		}

		bool RiskState::GetCard(int player, int card_coord) const
		{
			return board[num_players_ * num_terrs_ + num_players_ + 5 + 9 + 5 * num_terrs_ + (num_terrs_ + 2) * player + card_coord];
		}

		void RiskState::ResetHand(int player)
		{
			for (int i = 0; i < num_terrs_ + 2; ++i)
			{
				SetCard(player, i, 0);
			}
		}

		std::vector<bool> RiskState::GetHand(int player) const
		{
			std::vector<bool> res(num_terrs_ + 2, 0);
			for (int i = 0; i < num_terrs_ + 2; ++i)
			{
				res[i] = GetCard(player, i);
			}
			return res;
		}
		//potentially redundant func//
		std::vector<bool> RiskState::GetHands() const
		{
			std::vector<bool> res((num_terrs_ + 2) * num_players_, 0);
			for (int j = 0; j < num_players_; ++j)
			{
				for (int i = 0; i < (num_terrs_ + 2) * num_players_; ++i)
				{
					res[j * (num_players_ + 2) + i] = GetCard(j, i);
				}
			}
		}

		std::vector<bool> RiskState::GetCards() const
		{
			std::vector<bool> res(num_terrs_ + 2, 0);
			for (int i = 0; i < num_players_; ++i)
			{
				auto hand = GetHand(i);
				for (int j = 0; j < num_terrs_ + 2; ++j)
				{
					if (hand[j])
					{
						res[j] = 1;
					}
				}
			}
			return res;
		}

		int RiskState::GetCardSig(int card_coord) const
		{
			int arr[4] = {2, 3, 5, 7};
			for (int i = 0; i < 4; ++i)
			{
				if (risk_parent_game_->card_arr_[i] > card_coord)
				{
					return arr[i];
				}
			}
			return 0;
		}

		int RiskState::GetHandSig(int player) const
		{
			auto hand = GetHand(player);
			int res = 1;
			for (int i = 0; i < (int)hand.size(); ++i)
			{
				if (hand[i])
				{
					res *= GetCardSig(i);
				}
			}
			return res;
		}
		int RiskState::GetHandSum(int player) const
		{
			auto hand = GetHand(player);
			int sum = 0;
			for (int i = 0; i < (int)hand.size(); ++i)
			{
				if (hand[i])
				{
					sum += 1;
				}
			}
			return sum;
		}

		bool RiskState::GetCashable(int player) const
		{
			int sig = GetHandSig(player);
			std::array<int, 10> key_arr = {30, 42, 70, 105, 98, 147, 245, 125, 27, 8};
			for (int i = 0; i < 10; ++i)
			{
				if (sig % key_arr[i] == 0)
				{
					return true;
				}
			}
			return false;
		}

		std::array<int, 3> RiskState::GetSatCards(std::array<int, 3> component_arr) const
		{
			std::array<int, 3> arr = {0};
			int player = GetPlayer();
			auto hand = GetHand(player);
			for (int i = 0; i < 3; ++i)
			{
				if (component_arr[i] == 0)
				{
					for (int j = 0; j <risk_parent_game_->card_arr_[0]; ++j)
					{
						if (GetCard(player, j))
						{
							arr[i] = j;
							hand[j] = 0;
							break;
						}
					}
				}
				else
				{
					for (int j = risk_parent_game_->card_arr_[component_arr[i] - 1]; j < risk_parent_game_->card_arr_[component_arr[i]]; ++j)
					{
						if (GetCard(player, j))
						{
							arr[i] = j;
							hand[j] = 0;
							break;
						}
					}
				}
			}
			return arr;
		}

		int RiskState::GetOwner(int coord) const
		{
			for (int i = 0; i < num_players_; ++i)
			{
				if (board[i * num_terrs_ + coord] != 0)
				{
					return i;
				}
			}
			return num_players_;
		}

		std::vector<int> RiskState::GetTroopArr(int player) const
		{
			std::vector<int> res(num_terrs_, 0);
			for (int i = 0; i < num_terrs_; ++i)
			{
				if (board[num_terrs_ * player + i] > 0)
				{
					res[i] = board[num_terrs_ * player + i];
				}
			}
			return res;
		}

		std::vector<bool> RiskState::GetTroopMask(int player) const
		{
			std::vector<bool> res(num_terrs_, 0);
			for (int i = 0; i < num_terrs_; ++i)
			{
				if (board[num_terrs_ * player + i] > 0)
				{
					res[i] = 1;
				}
			}
			return res;
		}

		int RiskState::GetEliminated(int player) const
		{
			for (int i = 0; i < num_players_ - 1; ++i)
			{
				if (board[2 * num_players_ * num_terrs_ + 5 + 9 + 5 * num_terrs_ + 3 * num_players_ + player * (num_players_ - 1) + i] == 1)
				{
					return i + 1;
				}
			}
			return 0;
		}

		int RiskState::GetMaxElim() const
		{
			int max = 0;
			for (int i = 0; i < num_players_; ++i)
			{
				int elim = GetEliminated(i);
				if (elim > max)
				{
					max = elim;
				}
			}
			return max;
		}

		void RiskState::EndTurn()
		{
			if (GetSucc() == 1)
			{
				SetChance(1);
			}
			else
			{
				SetNextPlayer();
				while (GetEliminated(GetPlayer()) != 0)
				{
					SetNextPlayer();
				}
				SetDeploy();
				Income();
				int sum = GetHandSum(GetPlayer());
				if (sum > 4)
				{
					Cash();
				}
			}
		}

		void RiskState::Eliminate(int victim, int victor)
		{
			auto victim_hand = GetHand(victim);
			for (int i = 0; i < num_terrs_ + 2; ++i)
			{
				board[num_players_ * num_terrs_ + num_players_ + 5 + 9 + 5 * num_terrs_ + (num_terrs_ + 2) * victor + i] = victim_hand[i];
				ResetHand(victim);
			}
			int max = GetMaxElim();
			board[2 * num_players_ * num_terrs_ + 5 + 9 + 5 * num_terrs_ + 3 * num_players_ + victim * (num_players_ - 1) + max] = 1;
		}

		void RiskState::Deal(long seed)
		{
			int player = GetPlayer();
			assert(GetChance() == 1);
			std::vector<int> internal_deck = {};
			std::vector<bool> cards = GetCards();
			for (int i = 0; i < (int)cards.size(); ++i)
			{
				if (!cards[i])
				{
					internal_deck.push_back(i);
				}
			}
			auto sig = GetHandSig(player);
			//this may be belt and braces bruh//
			std::uniform_int_distribution<> distrib(0, internal_deck.size() - 1);
			std::mt19937 gen(seed);
			int choice = distrib(gen);
			int card = internal_deck[choice];
			SetCard(player, card, 1);
			SetChance(0);
			SetSucc(0);
			EndTurn();
		}

		void RiskState::Cash()
		{
			int player = GetPlayer();
			assert(GetCashable(player));
			int key = GetHandSig(player);
			std::array<int, 10> key_arr = {30, 42, 70, 105, 98, 147, 245, 125, 27, 8};
			std::array<std::array<int, 3>, 10> component_arr = {{{0, 1, 2}, {0, 1, 3}, {0, 2, 3}, {1, 2, 3}, {0, 3, 3}, {1, 3, 3}, {2, 3, 3}, {2, 2, 2}, {1, 1, 1}, {0, 0, 0}}};
			std::array<int, 10> troop_arr = {10, 10, 10, 10, 10, 10, 10, 8, 6, 4};
			int index = 0;
			for (int i = 0; i < 10; ++i)
			{
				if (key % key_arr[i] == 0)
				{
					index = i;
					break;
				}
			}
			int income = troop_arr[index];
			bool bonus = false;
			auto sat_cards = GetSatCards(component_arr[index]);
			for (int i = 0; i < 3; ++i)
			{
				if (sat_cards[i] < num_terrs_ && bonus == false)
				{
					if (GetOwner(sat_cards[i] == player))
					{
						bonus = true;
						IncrementTerr(sat_cards[i], player, 2);
					}
				}
				SetCard(player, sat_cards[i], 0);
			}
			IncrementIncome(income);
		}

		void RiskState::Income()
		{
			int turns = GetTurns();
			int income = 0;
			assert(GetIncome() == 0);
			if (turns < risk_parent_game_->starting_troops_)
			{
				income = 1;
			}
			else
			{
				std::vector<bool> troop_mask = GetTroopMask(GetPlayer()); //could make a new func to not do looping but hey ho
				int terrs = 0;
				for (int i = 0; i < num_terrs_; ++i)
				{
					if (troop_mask[i])
					{
						terrs += 1;
					}
				}
				int base = std::max((int)std::floor(terrs / 3), 3);
				int continent_bonus = 0;
				int ast_bonus = 0;
				int num_continents = risk_parent_game_->cont_matrix_.size();
				for (int i = 0; i < num_continents; ++i)
				{
					bool sat = true;
					for (int j = 0; j < num_terrs_; ++j)
					{
						if (!troop_mask[j] && risk_parent_game_->cont_matrix_[i][j])
						{
							sat = false;
							break;
						}
					}
					if (sat)
					{
						continent_bonus += risk_parent_game_->cont_bonus_[i];
					}
				}
				if (turns == risk_parent_game_->starting_troops_)
				{
					ast_bonus = risk_parent_game_->assist_[GetPlayer()];
				}
				income = base + continent_bonus + ast_bonus;
			}
			SetIncome(income);
		}

		void RiskState::Deploy(int amount)
		{
			int coord = GetCoord(0);
			int player = GetPlayer();
			IncrementTerr(coord, player, amount);
			IncrementIncome(-amount);
			if (GetIncome() == 0)
			{
				if (GetTurns() < risk_parent_game_->starting_troops_)
				{
					EndTurn();
				}
				else
				{
					SetAttack();
				}
			}
			else
			{
				SetDeploy();
			}
		}

		void RiskState::Attack(long seed)
		{
			double prob_arr[6][2] = {{0.2925668724279835, 0.628343621399177}, {0.3402777777777778, 1.0}, {0.44830246913580246, 0.7723765432098766}, {0.4212962962962963, 1.0}, {0.7453703703703703, 1.0}, {0.5833333333333334, 1.0}};
			std::unordered_map<int, int> dict = { {10,0},{4,1},{8,2},{3,3},{2,5},{6,4}};
			int n_atk = GetAtkNum();
			int coord_from = GetCoord(2);
			int coord_to = GetCoord(3);
			int atk_player = GetPlayer();
			int def_player = GetOwner(coord_to);
			int n_def = board[num_terrs_ * def_player + coord_to];
			int start_amount = n_atk;
			std::mt19937 gen(seed);
			while (n_atk > 0 && n_def > 0)
			{
				//this may also be belt and braces//
				std::uniform_real_distribution<> distrib(0, 1);
				double randnum = distrib(gen);
				int atk_dice = std::min(3, n_atk);
				int def_dice = std::min(2, n_def);
				int def_losses = 0;
				int hash = def_dice*(atk_dice+def_dice);
				if (randnum <= prob_arr[dict[hash]][0]) {
					def_losses = 0;
				}
				else if (randnum <= prob_arr[dict[hash]][1]) {
					def_losses = 1;
				}
				else {
					def_losses = 2;
				}
				n_def -= def_losses;
				n_atk -= std::min(atk_dice, def_dice) - def_losses;
			}
			SetChance(0);
			assert(n_def == 0 || n_atk == 0);
			if (n_def == 0)
			{
				SetTerr(coord_to, GetPlayer(), n_atk);
				SetTerr(coord_to, def_player, 0);
				IncrementTerr(coord_from, atk_player, -start_amount);
				SetSucc(1);
				SetRedistribute();
			}
			if (n_atk == 0)
			{
				SetTerr(coord_to, def_player, n_def);
				IncrementTerr(coord_from, atk_player, -start_amount);
				SetAttack();
			}
			bool def_dead = true;
			for (int i = 0; i < num_terrs_; ++i)
			{
				if (board[num_terrs_ * def_player + i] != 0)
				{
					def_dead = false;
					break;
				}
			}
			if (def_dead)
			{
				Eliminate(def_player, GetPlayer());
			}
		}

		void RiskState::Redistribute(int amount)
		{
			int coord_from = GetCoord(3);
			int coord_to = GetCoord(2);
			int player = GetPlayer();
			IncrementTerr(coord_from, player, -amount);
			IncrementTerr(coord_to, player, amount);
			int sum = GetHandSum(player);
			if (sum > 5)
			{
				SetDeploy();
			}
			else
			{
				SetAttack();
			}
			while (sum > 5)
			{
				Cash();
				sum -= 3;
			}
		}

		void RiskState::Fortify(int amount)
		{
			int player = GetPlayer();
			int coord_from = GetCoord(6);
			int coord_to = GetCoord(7);
			IncrementTerr(coord_to, player, amount);
			IncrementTerr(coord_from, player, -amount);
			EndTurn();
		}

		//populate above with all necessary asserts/

		//High Level funcs involving interacting with GameState

		//this is needed for fortify algorithm//
		void RiskState::DepthFirstSearch(int player, int vertex, std::vector<bool> *out) const
		{
			(*out)[vertex] = 1;
			for (int i = 0; i < num_terrs_; ++i)
			{
				if (risk_parent_game_->adj_matrix_[vertex][i] && board[player * num_terrs_ + i] != 0 && !(*out)[i])
				{
					DepthFirstSearch(player, i, out);
				}
			}
		}

		//current abstraction operators//
		std::vector<int> RiskState::GetAbstraction(int num, int abs) const
		{
			std::vector<int> res = {};
			for (int i = 0; i < abs; ++i)
			{
				if ((int)std::floor(num * (i + 1) / (abs)) > 0)
				{
					res.push_back(i);
				}
			}
			return res;
		}

		int RiskState::RetAbstraction(int action, int abs) const
		{
			int phse = GetPhse();
			int num = 0;
			switch (phse)
			{
			case 1:
				num = GetIncome();
				break;
			case 4:
				num = GetTroopArr(GetPlayer())[GetCoord(2)] - 1;
				break;
			case 5:
				num = GetTroopArr(GetPlayer())[GetCoord(3)] - 1;
				break;
			case 8:
				num = GetTroopArr(GetPlayer())[GetCoord(6)] - 1;
				break;
			}
			return (int)std::floor((action + 1) * num / abs);
		}
		// OPENSPIEL METHODS BELOW //

		//obviously the most costly func//
		std::vector<Action> RiskState::LegalActions() const
		{
			std::vector<Action> res = {};
			int player = GetPlayer();
			if (GetChance() == 1)
			{
				res.push_back(0);
			}
			//returns legal fort_q's
			else
			{
				switch (GetPhse())
				{
				case 0:
				{ //returns legal dep_to's
					if (GetCashable(player))
					{
						res.push_back(0);
					}
					bool map_occupied = true;
					for (int i = 0; i < num_terrs_; ++i)
						if (GetOwner(i) == num_players_)
						{
							map_occupied = false;
							res.push_back(i + 1);
						}
					if (map_occupied)
					{
						for (int i = 0; i < num_terrs_; ++i)
							if (board[player * num_terrs_ + i] != 0)
							{
								res.push_back(i + 1);
							}
					}
					break;
				}
				case 1:
				{
					if (!risk_parent_game_->abstraction_[0])
					{
						for (int i = 0; i < GetIncome(); ++i)
						{
							res.push_back(i + phse_constants_[1]);
						}
					}
					else
					{
						std::vector<int> abs = GetAbstraction(GetIncome(), risk_parent_game_->action_q_[0]);
						for (size_t i = 0; i < abs.size(); ++i)
						{
							res.push_back(abs[i] + phse_constants_[1]);
						}
					}
					break;
				}
					//returns legal dep_q's
				case 2:
				{ //returns legal atk_from's
					std::vector<bool> attackable_mask(num_terrs_, 0);
					std::vector<int> troop_arr = GetTroopArr(player);
					std::vector<bool> anti_mask(num_terrs_, 0);
					for (int i = 0; i < num_terrs_; ++i)
					{
						if (troop_arr[i] > 1)
						{
							attackable_mask[i] = 1;
						}
						else if (troop_arr[i] == 0)
						{
							anti_mask[i] = 1;
						}
					}
					res.push_back(phse_constants_[2]);
					for (int i = 0; i < num_terrs_; ++i)
					{
						for (int j = 0; j < num_terrs_; ++j)
						{
							if (risk_parent_game_->adj_matrix_[i][j]&& attackable_mask[i] && anti_mask[j])
							{
								res.push_back(phse_constants_[2] + 1 + i);
								break;
							}
						}
					}
					break;
				}
				case 3:
				{
					int from_coord = GetCoord(2);
					std::vector<int> troop_arr = GetTroopArr(player);
					for (int i = 0; i < num_terrs_; ++i)
					{
						if (risk_parent_game_->adj_matrix_[i][from_coord] && troop_arr[i] == 0)
						{
							res.push_back(i + phse_constants_[3]);
						}
					}
					break;
				}
					//returns legal atk_to's
				case 4:
				{
					int coord_from = GetCoord(2);
					int atk_num = board[player * num_terrs_ + coord_from];
					if (!risk_parent_game_->abstraction_[1])
					{
						for (int i = 0; i < atk_num - 1; ++i)
						{
							res.push_back(i + phse_constants_[4]);
						}
					}
					else
					{
						std::vector<int> abs = GetAbstraction(atk_num - 1,risk_parent_game_->action_q_[1]);
						for (size_t i = 0; i < abs.size(); ++i)
						{
							res.push_back(abs[i] + phse_constants_[4]);
						}
					}
					break;
				}
					//returns legal atk_q's
				case 5:
				{
					int coord_from = GetCoord(3);
					int redist_num = board[player * num_terrs_ + coord_from];
					res.push_back(0 + phse_constants_[5]);
					if (!risk_parent_game_->abstraction_[2])
					{
						for (int i = 0; i < redist_num - 1; ++i)
						{
							res.push_back(i + 1 + phse_constants_[5]);
						}
					}
					else
					{
						std::vector<int> abs = GetAbstraction(redist_num - 1, risk_parent_game_->action_q_[2]);
						for (size_t i = 0; i < abs.size(); ++i)
						{
							res.push_back(abs[i] + 1 + phse_constants_[5]);
						}
					}
					break;
				} //returns legal redist_q's

				case 6:
				{
					std::vector<int> troop_arr = GetTroopArr(player);
					res.push_back(phse_constants_[6]);
					for (int i = 0; i < num_terrs_; ++i)
					{
						if (troop_arr[i] > 1)
						{
							for (int j = 0; j < num_terrs_; ++j)
							{
								if (troop_arr[j] && risk_parent_game_->adj_matrix_[i][j] && i != j)
								{
									res.push_back(phse_constants_[6] + 1 + i);
									break;
								}
							}
						}
					}
					break;
				} //returns legal fort_from's
				case 7:
				{
					int from_coord = GetCoord(6);
					std::vector<bool> dfs_arr(num_terrs_, 0);
					DepthFirstSearch(player, from_coord, &dfs_arr);
					for (int i = 0; i < num_terrs_; ++i)
					{
						if (dfs_arr[i] && i != from_coord)
						{
							res.push_back(phse_constants_[7] + i);
						
						}
					}
					break;
				}
					//returns legal fort_to's needs to use depth first search
				case 8:
				{
					int from_coord = GetCoord(6);
					std::vector<int> troop_arr = GetTroopArr(player);
					if (!risk_parent_game_->abstraction_[3])
					{
						for (int i = 0; i < troop_arr[from_coord]; ++i)
						{
							res.push_back(phse_constants_[8] + i);
						}
					}
					else
					{
						std::vector<int> abs = GetAbstraction(troop_arr[from_coord] - 1,risk_parent_game_->action_q_[3]);
						for (int i = 0; i != abs.size(); ++i)
						{
							res.push_back(abs[i] + phse_constants_[8]);
						}
					}
					break;
				}
				}
			}
			assert(res.size()>0);
			return res;
		}

		void RiskState::ActionHandler(Action action_id,long seed)
		{
			int player = GetPlayer();
			if (action_id == 0 && GetChance())
			{
				switch (GetPhse())
				{
				case 4:
					Attack(seed);
					break;
				case 6:
					Deal(seed);
					break;
				case 8:
					Deal(seed);
					break;
				}
				return;
			}
			else
			{
				int t_action = action_id - phse_constants_[GetPhse()];
				switch (GetPhse())
				{
				case 0:
				{
					if (t_action == 0)
					{
						Cash();
					}
					else
					{
						SetCoord(t_action - 1);
					}
					break;
				}
				case 1:
					if (!risk_parent_game_->abstraction_[0])
					{

						Deploy(t_action + 1);
					}
					else
					{
						Deploy(RetAbstraction(t_action, risk_parent_game_->action_q_[0]));
					}
					break;
				case 2:
					if (t_action == 0)
					{
						SetFortify();
					}
					else
					{
						SetCoord(t_action - 1);
					}
					break;
				case 3:
					SetCoord(t_action);
					break;
				case 4:
					if (!risk_parent_game_->abstraction_[1])
					{
						SetAtkNum(t_action + 1);
					}
					else
					{
						SetAtkNum(RetAbstraction(t_action, risk_parent_game_->action_q_[1]));
					}
					SetChance(1);
					break;
				case 5:
					if (t_action == 0)
					{
						SetAttack();
					}
					else if (!risk_parent_game_->abstraction_[2])
					{
						Redistribute(t_action);
					}
					else
					{
						Redistribute(RetAbstraction(t_action - 1, risk_parent_game_->action_q_[2]));
					}
					break;
				case 6:
					if (t_action == 0)
					{
						EndTurn();
					}
					else
					{
						SetCoord(t_action - 1);
					}
					break;
				case 7:
					SetCoord(t_action);
					break;
				case 8:
					if (!risk_parent_game_->abstraction_[3])
					{
						Fortify(t_action + 1);
					}
					else
					{
						Fortify(RetAbstraction(t_action, risk_parent_game_->action_q_[3]));
					}
					break;
				}
			}
		}
		void RiskState::DoApplyAction(Action action_id){
			if(IsChanceNode()){
				long random_seed = risk_parent_game_->RNG();
			}
			ActionHandler(action_id,random_seed);
			random_seeds.push_back(random_seed);
		}

		std::vector<double> RiskState::Rewards() const
		{	
			std::vector<double> res = std::vector<double>(num_players_,0);
			if(IsTerminal()){
			std::vector<int> arr(num_players_, 0);
			int split = 0;
			int split_rewards = 0;
			for (int i = 0; i < num_players_; ++i)
			{
				arr[i] = GetEliminated(i);
				if (arr[i] == 0)
				{
					split_rewards += risk_parent_game_->rewards_[num_players_ - split - 1];
					split += 1;
				}
			}
			double split_reward = (double)split_rewards / split;
			for (int i = 0; i < num_players_; ++i)
			{
				if (arr[i] != 0)
				{
					res[i]=risk_parent_game_->rewards_[arr[i] - 1];
				}
				else
				{
					res[i] = split_reward;
				}
			}
			}
			return res;
		}

		std::vector<double> RiskState::Returns() const
		{
			if (!IsTerminal())
			{
				std::vector<double> res = {};
				for (int i = 0; i < num_players_; ++i)
				{
					res.push_back(0);
				}
				return res;
			}
			else
			{
				return Rewards();
			}
		}

		Player RiskState::CurrentPlayer() const
		{
			if (GetChance())
			{
				return kChancePlayerId;
			}
			else if(IsTerminal())
			{
				return kTerminalPlayerId;
			}
			else
			{
				return GetPlayer();
			}
		}

		bool RiskState::IsTerminal() const
		{
			if (GetMaxElim() == num_players_ || GetTurns() >= risk_parent_game_->max_turns_)
			{
				return true;
			}
			return false;
		}

		bool RiskState::IsChanceNode() const
		{
			return (CurrentPlayer() == kChancePlayerId);
		}

		bool RiskState::IsPlayerNode() const
		{
			return (CurrentPlayer() >= 0);
		}

		ActionsAndProbs RiskState::ChanceOutcomes() const
		{
			return ActionsAndProbs{std::make_pair(0, 1)};
		}

		std::vector<Action> RiskState::LegalChanceOutcomes() const
		{
			if (IsChanceNode())
			{
				return {0};
			}
			else
			{
				return {};
			}
		}

		void RiskState::ObservationTensor(Player player,
										  absl::Span<float> values) const
		{
			ContiguousAllocator allocator(values);
			const RiskGame &game = open_spiel::down_cast<const RiskGame &>(*game_);
			game.default_observer_->WriteTensor(*this, player, &allocator);
		}

		void RiskState::InformationStateTensor(Player player,
			absl::Span<float> values) const {
			ContiguousAllocator allocator(values);
			const RiskGame& game = open_spiel::down_cast<const RiskGame&>(*game_);
			game.default_observer_->WriteTensor(*this, player, &allocator);
		}

		std::unique_ptr<State> RiskState::Clone() const
        {
			return std::unique_ptr<RiskState>(new RiskState(*this));
		}
		std::string RiskState::ActionToString(Player player, Action action_id) const {
			return std::to_string(action_id);
		}
		std::string RiskState::ToString() const {
			std::string out;
			for (int i = 0; i < board.size(); ++i) {
				out.append(std::to_string(board[i]));
				if(i!=board.size()-1){
					out.append(",");
				}
			}
			return out;
		}
		std::string RiskState::Serialize() const{
			std::vector<Action> history = History();
			std::string str = "";
			for(int i =0;i<history.size();++i){
				str.append(std::to_string(history[i])+" "+std::to_string(random_seeds[i])+"\n");
			}
			return str;
		}
		//this implements some kind of infostate hashing protocol, think zobrist hashing etc for search based algos//
		std::string RiskState::InformationStateString(Player player)const{
			auto hash = absl::Hash<std::vector<int>>;
			std::vector<int> info_state(risk_parent_game_->InformationStateTensorShape()[0],0);
			auto hand = GetHand(player);
			for (int i = 0; i < num_terrs_ + 2; ++i){info_state[i]=hand[i];};
			for (int i = 0; i < num_players_ * num_terrs_ + num_players_ + 14 + 5 * num_terrs_; ++i){info_state[i+num_terrs_+2] = board[i];};
			for (int i = 0; i < num_players_; ++i){info_state[i + num_players_ * num_terrs_ + num_players_ + 14 + 5 * num_terrs_+num_terrs_+2] = GetHandSum(i);};
			for (int i = 0; i < num_players_ * (num_players_- 1); ++i){info_state[i + num_players_ * num_terrs_ + 2 * num_players_ + 14 + 5 * num_terrs_+num_terrs_+2] =  board[2 * num_players_ * num_terrs_ + 14 + 5 * num_terrs_ + 3 * num_players_];};
			auto hash_value = hash(info_state);
			return std::to_string(hash_value);
		}
		std::unique_ptr<State> ResampleFromInfostate(int player_id, std::function<double()> rng) const {
			//this is exceptionally crude resampling//
			auto clone = Clone();
			auto hand = clone->GetHand(player_id);
			std::vector<int> deck={};
			for(int i =0;i<num_terrs_+2;++i){
				if(hand[i]==0){
					deck.push_back(i);
				}
			}
			long seed = risk_parent_game_->RNG();
			std::mt19937 gen(seed);
			std::shuffle(deck.begin(),deck.end(),gen);
			int count = 0;
			for(int i =0 ; i<num_players_;++i){
				if(i!=player_id){
					int sum = clone->GetHandSum(i);	
					for(int j=0;j<num_terrs_+2;++j){
					clone->board[num_players_*num_terrs_+num_players_+14+5*num_terrs_+i*(num_terrs_+2)+j]=0;
					}
					while(sum>0){
						int card = deck[count];
						clone->board[num_players_*num_terrs_+num_players_+14+5*num_terrs_+i*(num_terrs_+2)+card];
						sum -=1;
						count+=1;
					}
				}

			}
			return clone;
		}

		RiskGame::RiskGame(const GameParameters& params)
			: Game(kGameType, params), num_players_(ParameterValue<int>("players")),
			map_(ParameterValue<int>("map")),max_turns_(ParameterValue<int>("max_turns")),
			abstraction_{ ParameterValue<bool>("dep_abs"),ParameterValue<bool>("atk_abs"),ParameterValue<bool>("redist_abs"),ParameterValue<bool>("fort_abs") },
			action_q_{ ParameterValue<int>("dep_q"),ParameterValue<int>("atk_q"),ParameterValue<int>("redist_q"),ParameterValue<int>("fort_q")},
			adj_matrix_{AdjMatrixer(ParameterValue<int>("map"))},cont_matrix_{ContMatrixer(ParameterValue<int>("map"))},cont_bonus_{ContBonuser(ParameterValue<int>("map"))},
			rewards_{Rewarder(ParameterValue<int>("players"))},assist_{Assister(ParameterValue<int>("players"))},card_arr_{CardArrer(ParameterValue<int>("map"))},terr_names_{TerrNamer(ParameterValue<int>("map"))},
			starting_troops_{40-5*(ParameterValue<int>("players")-2)},
			rng_(std::mt19937(ParameterValue<int>("rng_seed") == -1
				? std::time(0)
				: ParameterValue<int>("rng_seed")))
		{
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
		
		std::unique_ptr<RiskState> RiskGame::NewInitialRiskState() const {
            const auto ptr =std::dynamic_pointer_cast<const RiskGame>(shared_from_this());
			return std::make_unique<RiskState>(ptr);
		}
		
		std::unique_ptr<State> RiskGame::NewInitialState() const {
  			return NewInitialRiskState();
		}


		std::vector<int> RiskGame::ObservationTensorShape() const {
			int num_terrs_ = adj_matrix_.size();
			return {num_players_ * num_terrs_ + 2 * num_players_ + 16 + 6 * num_terrs_ + num_players_ * (num_players_ - 1)};
		}
		
		std::vector<int> RiskGame::InformationStateTensorShape() const {
			int num_terrs_ = adj_matrix_.size();
			return { num_players_ * num_terrs_ + 2 * num_players_ + 16 + 6 * num_terrs_ + num_players_ * (num_players_ - 1) };
		}


		int RiskGame::NumDistinctActions() const {
			int sum = 0;
			for (int i = 0; i < 4; ++i) {
				sum += action_q_[i];
			}
			sum += 4 + 5*(int)adj_matrix_.size();
			return sum;
		}


		std::shared_ptr<Observer> RiskGame::MakeObserver(absl::optional<IIGObservationType> iig_obs_type, const GameParameters& params) const {
            if (!params.empty())
				SpielFatalError("Observation params not supported");
			return std::make_shared<RiskObserver>(iig_obs_type.value_or(kDefaultObsType));
		}
		std::string RiskGame::GetRNGState() const {
			std::ostringstream rng_stream;
			rng_stream << rng_;
			return rng_stream.str();
		}
		void RiskGame::SetRNGState(const std::string& rng_state) const {
			if (rng_state.empty()) return;
			std::istringstream rng_stream(rng_state);
			rng_stream >> rng_;
		}

		int RiskGame::RNG() const { return rng_(); }
		std::unique_ptr<State> RiskGame::DeserializeState(const std::string& str) const{
			std::unique_ptr<RiskState> state = NewInitialRiskState();
			if(str.empty()) return state;
			std::vector<std::string> lines = absl::StrSplit(str, '\n');
			for(int i =0;i<lines.size();++i){
				std::vector<std::string> splittage = absl::StrSplit(lines[i]," ");
				Action act = std::stol(splittage[0]);
				long seed = std::stol(splittage[1]);
				state->ActionHandler(act,seed);
				state->history_.push_back({state->CurrentPlayer(),act});
				state->move_number_+=1;
			}
			return state;
		}

	} // namespace risk

}// namespace open_spiel
