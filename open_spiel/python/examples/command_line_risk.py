"""NFSP agents trained on simplified risk."""

from absl import app
from absl import flags
from absl import logging
import tensorflow.compat.v1 as tf
import numpy as np
import igraph as ig
import cairocffi
import random
import pyspiel

from open_spiel.python import policy
from open_spiel.python import rl_environment
from open_spiel.python.algorithms import exploitability
from open_spiel.python.algorithms import nfsp

simple_adj = [ [1, 2],[0, 2, 3],[0, 1, 3],[1, 2, 4],[3, 5, 6],[6, 7, 8, 10, 11],[4, 5, 8, 34],[5, 9, 11, 12, 14],[5, 6, 10, 40],[7, 14],[5, 8, 11, 40],[5, 7, 10, 12, 13],[7, 11, 13, 14],[11, 12, 14],[7, 9, 12, 13, 15],[14, 16, 20],[15, 17, 19, 20],[16, 18, 19, 38],[17, 19, 22],[16, 17, 18, 22, 21, 20],[15, 16, 19, 21],[19, 20, 22, 23],[18, 19, 21, 23],[21, 22, 24],[23, 25, 27],[24, 26, 27],[25, 27],[24, 25, 26, 28],[27, 29, 33, 35, 36],[28, 30, 32],[29, 31, 32],[30, 32],[29, 30, 33, 34],[28, 32, 34, 35],[6, 32, 33, 40],[28, 33, 34, 36, 40, 41],[28, 35, 37, 41],[36, 38, 39, 41],[37, 39],[37, 38, 40, 41],[8, 10, 34, 35, 39, 41],[35, 36, 37, 39, 40] ]
vertex_lis = []
for i in range(len(simple_adj)):
  for j in simple_adj[i]:
    if (i,j) not in vertex_lis and (j,i) not in vertex_lis:
      vertex_lis.append((i,j))

FLAGS = flags.FLAGS
SEED = 12983641
final_policy_type = pyspiel.ISMCTSFinalPolicyType.MAX_VISIT_COUNT
"""final_policy_type = pyspiel.ISMCTSFinalPolicyType.MAX_VALUE"""
evaluator = pyspiel.RandomRolloutEvaluator(1, SEED)
bot = pyspiel.ISMCTSBot(SEED, evaluator, 0.75, 1000, -1, final_policy_type,
                              False, False)
human_id = 1
bot_id = 0

def visualise(state,player_id):
  g = ig.Graph()
  g.add_vertices(42)
  for i in vertex_lis:
    g.add_edges([(i[0],i[1])])
  colour_dict = {0:'red',0.5:'black',1:'blue'}
  g.vs["terr_no"]=[i for i in range(42)]
  troops=[0 for i in range(42)]
  ownership=[0.5 for i in range(42)]
  info_state =state.information_state_tensor(player_id)
  for player in range(2):
    for terr in range(42):
      if player == 0 and info_state[44+terr]>0:
        ownership[terr]=0
        troops[terr]=info_state[44+terr]
      if player == 1 and info_state[86+terr]>0:
        ownership[terr]=1
        troops[terr]=info_state[86+terr]
  g.vs["player"]=ownership
  g.vs["troops"]=troops
  g.vs["label"]=["______"+str(g.vs["terr_no"][i])+","+str(g.vs["troops"][i]) for i in range(42)]
  layout = g.layout_kamada_kawai()
  return(ig.plot(g,layout=layout,vertex_color = [colour_dict[player] for player in g.vs["player"]]))
  
def main(unused_argv):
  game = pyspiel.load_game("risk")
  state = game.new_initial_state()
  count = 0
  while not state.is_terminal():
    
    """if count <160:
      actions = state.legal_actions()
      action = random.choice(actions)
      state.apply_action(action)
      count+=1
      continue"""
    
    current_player = state.current_player()
    if state.is_chance_node():
      state.apply_action(0)
    
    elif current_player ==human_id:
      visualise(state,human_id)
      info_state = state.information_state_tensor(human_id)
      print(info_state[:42])
      print(info_state[-4:-2])
      legal = state.legal_actions()
      print(state.legal_actions())
      action = "1000"
      while int(action) not in legal:
        action = input("Action:")
        if action =="":
          action = "1000"
      state.apply_action(int(action))
    elif current_player == bot_id:
      action = bot.step(state)
      print("Bot action:"+str(action))
      state.apply_action(action)
  print(state.rewards())
  
  
if __name__ == "__main__":
  app.run(main)