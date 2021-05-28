"""NFSP agents trained on simplified risk."""

from absl import app
from absl import flags
from absl import logging
import tensorflow.compat.v1 as tf
import numpy as np
import igraph as ig
import cairocffi
import random

from open_spiel.python import policy
from open_spiel.python import rl_environment
from open_spiel.python.algorithms import exploitability
from open_spiel.python.algorithms import nfsp

simple_adj = [[0,2,11] ,[0,2,18],[3,16],[4,14,16],[5,7,14] ,[4,6,14,15],[5,7],[4,6,8],[7,9] ,[8,10] ,[11,12] ,[0,9,12],[10,11,13],[12,14,17,16],[3,4,5,13,15,16],[5,14],[2,3,13,14,17],[13,16,18],[17,1]]
vertex_lis = []
for i in range(len(simple_adj)):
  for j in simple_adj[i]:
    if (i,j) not in vertex_lis and (j,i) not in vertex_lis:
      vertex_lis.append((i,j))

FLAGS = flags.FLAGS

flags.DEFINE_integer("human_player_id",1,"Which player is the human")
flags.DEFINE_string("game_name", "risk",
                    "Name of the game.")
flags.DEFINE_integer("num_players", 2,
                     "Number of players.")
flags.DEFINE_integer("num_train_episodes", int(1e5),
                     "Number of training episodes.")
flags.DEFINE_integer("eval_every", 1000,
                     "Episode frequency at which the agents are evaluated.")
flags.DEFINE_list("hidden_layers_sizes", [
    174,174,174
], "Number of hidden units in the avg-net and Q-net.")
flags.DEFINE_integer("replay_buffer_capacity", int(1e3),
                     "Size of the replay buffer.")
flags.DEFINE_integer("reservoir_buffer_capacity", int(1e4),
                     "Size of the reservoir buffer.")
flags.DEFINE_integer("min_buffer_size_to_learn", 1000,
                     "Number of samples in buffer before learning begins.")
flags.DEFINE_float("anticipatory_param", 0.1,
                   "Prob of using the rl best response as episode policy.")
flags.DEFINE_integer("batch_size", 128,
                     "Number of transitions to sample at each learning step.")
flags.DEFINE_integer("learn_every", 64,
                     "Number of steps between learning updates.")
flags.DEFINE_float("rl_learning_rate", 0.0001,
                   "Learning rate for inner rl agent.")
flags.DEFINE_float("sl_learning_rate", 0.0001,
                   "Learning rate for avg-policy sl network.")
flags.DEFINE_string("optimizer_str", "sgd",
                    "Optimizer, choose from 'adam', 'sgd'.")
flags.DEFINE_string("loss_str", "mse",
                    "Loss function, choose from 'mse', 'huber'.")
flags.DEFINE_integer("update_target_network_every", 2000,
                     "Number of steps between DQN target network updates.")
flags.DEFINE_float("discount_factor", 1.0,
                   "Discount factor for future rewards.")
flags.DEFINE_integer("epsilon_decay_duration", int(1e5),
                     "Number of game steps over which epsilon is decayed.")
flags.DEFINE_float("epsilon_start", 0.06,
                   "Starting exploration parameter.")
flags.DEFINE_float("epsilon_end", 0.001,
                   "Final exploration parameter.")
flags.DEFINE_bool("use_checkpoints", True, "Save/load neural network weights.")
flags.DEFINE_string("checkpoint_dir", "/tmp/risk_nfsp",
                    "Directory to save/load the agent.")


class NFSPPolicies(policy.Policy):
  """Joint policy to be evaluated."""

  def __init__(self, env, nfsp_policies, mode):
    game = env.game
    player_ids = list(range(FLAGS.num_players))
    super(NFSPPolicies, self).__init__(game, player_ids)
    self._policies = nfsp_policies
    self._mode = mode
    self._obs = {
        "info_state": [None] * FLAGS.num_players,
        "legal_actions": [None] * FLAGS.num_players
    }

  def action_probabilities(self, state, player_id=None):
    cur_player = state.current_player()
    legal_actions = state.legal_actions(cur_player)

    self._obs["current_player"] = cur_player
    self._obs["info_state"][cur_player] = (
        state.observation_state_tensor(cur_player))
    self._obs["legal_actions"][cur_player] = legal_actions

    info_state = rl_environment.TimeStep(
        observations=self._obs, rewards=None, discounts=None, step_type=None)

    with self._policies[cur_player].temp_mode_as(self._mode):
      p = self._policies[cur_player].step(info_state, is_evaluation=True).probs
    prob_dict = {action: p[action] for action in legal_actions}
    return prob_dict

def visualise(info_state):
  g = ig.Graph()
  g.add_vertices(19)
  for i in vertex_lis:
    g.add_edges([(i[0],i[1])])
  colour_dict = {0:'red',0.5:'black',1:'blue'}
  g.vs["terr_no"]=[i for i in range(19)]
  troops=[0 for i in range(19)]
  ownership=[0.5 for i in range(19)]
  for player in range(2):
    for terr in range(19):
      if player == 0 and info_state[21+terr]>0:
        ownership[terr]=0
        troops[terr]=info_state[21+terr]
      if player == 1 and info_state[40+terr]>0:
        ownership[terr]=1
        troops[terr]=info_state[40+terr]
  g.vs["player"]=ownership
  g.vs["troops"]=troops
  g.vs["label"]=["______"+str(g.vs["terr_no"][i])+","+str(g.vs["troops"][i]) for i in range(19)]
  layout = g.layout_kamada_kawai()
  return(ig.plot(g,layout=layout,vertex_color = [colour_dict[player] for player in g.vs["player"]]))
  
def main(unused_argv):
  logging.info("Loading %s", FLAGS.game_name)
  game = FLAGS.game_name
  num_players = FLAGS.num_players

  env_configs = {"players": num_players,"map":0,"rng_seed":-1,"max_turns":90,"dep_abs":False,"atk_abs":True,"redist_abs":True,"fort_abs":True,"dep_q":31,"atk_q":2,"redist_q":2,"fort_q":2}
  env = rl_environment.Environment(game, **env_configs)
  info_state_size = env.observation_spec()["info_state"][0]
  num_actions = env.action_spec()["num_actions"]

  hidden_layers_sizes = [int(l) for l in FLAGS.hidden_layers_sizes]
  kwargs = {
      "replay_buffer_capacity": FLAGS.replay_buffer_capacity,
      "reservoir_buffer_capacity": FLAGS.reservoir_buffer_capacity,
      "min_buffer_size_to_learn": FLAGS.min_buffer_size_to_learn,
      "anticipatory_param": FLAGS.anticipatory_param,
      "batch_size": FLAGS.batch_size,
      "learn_every": FLAGS.learn_every,
      "rl_learning_rate": FLAGS.rl_learning_rate,
      "sl_learning_rate": FLAGS.sl_learning_rate,
      "optimizer_str": FLAGS.optimizer_str,
      "loss_str": FLAGS.loss_str,
      "update_target_network_every": FLAGS.update_target_network_every,
      "discount_factor": FLAGS.discount_factor,
      "epsilon_decay_duration": FLAGS.epsilon_decay_duration,
      "epsilon_start": FLAGS.epsilon_start,
      "epsilon_end": FLAGS.epsilon_end,
  }

  with tf.Session() as sess:
    # pylint: disable=g-complex-comprehension
    agents = [
        nfsp.NFSP(sess, idx, info_state_size, num_actions, hidden_layers_sizes,
                  **kwargs) for idx in range(num_players)
    ]
    joint_avg_policy = NFSPPolicies(env, agents, nfsp.MODE.average_policy)
    sess.run(tf.global_variables_initializer())
    if FLAGS.use_checkpoints:
      for agent in agents:
        if agent.has_checkpoint(FLAGS.checkpoint_dir):
          agent.restore(FLAGS.checkpoint_dir)

    time_step = env.reset()
    while not time_step.last():
      player_id = time_step.observations["current_player"]
      if FLAGS.human_player_id==player_id:
        if time_step.observations['info_state'][player_id][61]<40:
          time_step = env.step([random.choice(time_step.observations['legal_actions'][player_id])])
        else:
          print(time_step.observations['info_state'][player_id])
          print(time_step.observations['legal_actions'][player_id])
          visualise(time_step.observations['info_state'][player_id])
          human_action = input('Human action:')
          time_step = env.step([int(human_action)])
      else:
        agent_output = agents[player_id].step(time_step)
        action_list = [agent_output.action]
        print(action_list)
        time_step = env.step(action_list)
if __name__ == "__main__":
  app.run(main)