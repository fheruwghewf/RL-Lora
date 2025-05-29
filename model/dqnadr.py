import numpy as np
from numpy import random as rnd
import random
from matplotlib import pyplot as plt
import pickle
import torch

import torch.nn as nn
import torch.autograd as autograd
import torch.nn.functional as F
from collections import deque
from time import time
from os import system
import os

MY_PATH="/home/ubuntu/ns-allinone-3.37/ns-3.37/contrib/RL-Lora/NS3-LoraRL/rladr-lorans3/automation-interface"
memory_path = os.path.join(MY_PATH, "memory-store")  # 正确拼接路径
label_path = os.path.join(MY_PATH, "label-store")  # 正确拼接路径
model_path = os.path.join(MY_PATH, "model-store")  # 正确拼接路径
device = torch.device('cpu')

class DQN(nn.Module):
    def __init__(self, envstate_dim, action_dim):
        super(DQN, self).__init__()
        self.to(device)  # 初始化时指定设备
        self.input_dim = envstate_dim
        self.output_dim = action_dim
        self.huber_loss = F.smooth_l1_loss
        torch.manual_seed(2)
        #self.apply(init_weights)

        self.ff = nn.Sequential(
            nn.Linear(self.input_dim, 1344),
            nn.ReLU(),
            nn.Linear(1344, 2688),
            nn.ReLU(),
            nn.Linear(2688, 2688),
            nn.ReLU(),
            nn.Linear(2688, 1344),
            nn.ReLU(),
            nn.Linear(1344, self.output_dim),
        )

        #self.optimizer = torch.optim.SGD(self.parameters(),lr=0.09, momentum=0)
        self.optimizer = torch.optim.Adam(self.parameters(),lr=0.0001)
        
  


    def forward(self, state):
        qvals = self.ff(state)
        return qvals

class ReplayMemory:
    memory = None
    counter = 0

    def __init__(self, size):
        self.memory = deque(maxlen=size)

    def append(self, element):
        self.memory.append(element)
        self.counter += 1
        #print("Deque ",self.memory, " Element ",element)

    def sample(self, n):
        return random.sample(self.memory, n)


def decisionmaker(statein):
    print("This is the decisionmaker process ")
    if os.path.exists(model_path):
        agent = torch.load(model_path,map_location=torch.device("cpu"), weights_only=False)
        print("open the decisionmaker model ")
    else:
        agent=DQN(1344,96)
        torch.save(agent, model_path)
        print("Is it the first round? Model not found!")

    if os.path.exists(label_path):
        label = torch.load(label_path,map_location=torch.device("cpu"), weights_only=False)
        print("open the decisionmaker label ")

    else:
        label = DQN(1344,96)
        print("Is it the first round? Label not found!")
        torch.save(label, label_path)


    state = np.zeros(1344)
    state[statein] = 1
    qvals = agent.forward(torch.FloatTensor(state)) # forward run through the policy network   输出每个动作的所有预期回报
    action = np.argmax(qvals.detach().numpy()) # need to detach from auto
    print("finish the decisionmaker ")

    return action, 0


def remember(state1,state2,act1,reward):
    print("This is the remember process ",state1, state2, act1, reward)

    if os.path.exists(memory_path):
        with open(memory_path, 'rb') as memory_file:
            memory = pickle.load(memory_file)
            print("open remember-memory ")
    else:
        memory = ReplayMemory(2000)
        print("remember-memory ")
    
    mem_state1 = np.zeros(1344)
    mem_state1[state1] = 1
    
    mem_state2 = np.zeros(1344)
    mem_state2[state2] = 1

    #Add to memory and store it on disk

    memory.append({'state':mem_state1,'action':act1,'reward':reward,'next_state':mem_state2})
    with open(memory_path, 'w+b') as memory_file:
        pickle.dump(memory, memory_file)
    print("remember-memory added")
    

def train(state1,state2,act1,reward):
    print("This is the training process ",state1, state2, act1, reward)
    
    if os.path.exists(memory_path):
        with open(memory_path, 'rb') as memory_file:
            memory = pickle.load(memory_file)
            print("open memory-store")
    else:
        print("Error, no memory to work with")
        return 1
  ######load label#######  
    if os.path.exists(model_path):
        print("yes?")
        agent = torch.load(model_path, map_location=torch.device("cpu"),weights_only=False)
        print("open model-store")
    else:
        print("no agent-store")
        return 1
    
    if os.path.exists(label_path):
        label = torch.load(label_path,map_location=torch.device("cpu"), weights_only=False)
        print("open label-store")
    else:
        print("no label-model")
        return 1

    batch = memory.sample(min(10, len(memory.memory))) # here, batch size equals 10
    states = torch.FloatTensor(np.array(list(map(lambda x: x['state'], batch)))).to(device)
    actions = torch.LongTensor(np.array(list(map(lambda x: x['action'], batch)))).to(device)
    rewards = torch.FloatTensor(np.array(list(map(lambda x: x['reward'], batch)))).to(device)
    next_states = torch.FloatTensor(np.array(list(map(lambda x: x['next_state'], batch)))).to(device)
    print("data ok")

#DQN网络输出的是对于给定状态state，所有可能动作的Q值估计（即每个动作的未来预期回报）。
#所以agent.forward(states)返回的是一个形状为[batch_size, action_dim]的张量，包含该状态下所有动作的Q值。
#actions.unsqueeze(1)将动作索引从[batch_size]变为[batch_size,1]
#.gather(1, actions.unsqueeze(1))沿着维度1收集实际执行的动作对应的Q值
#结果curr_Q的形状是[batch_size,1]，表示每个状态-动作对的实际Q值
    #:print(states,agent.input_dim)
    curr_Q = agent.forward(states).gather(1,actions.unsqueeze(1)) # calculate the current Q(s,a) estimates
    next_Q = label.forward(next_states) # calculate Q'(s,a) (EV)
    max_next_Q = torch.max(next_Q,1)[0] # equivalent of taking a greedy action
    expected_Q = rewards + 0.8 * max_next_Q # Calculate total Q(s,a)
    loss = agent.huber_loss(curr_Q, expected_Q.unsqueeze(1)) # unsqueeze is really
    agent.optimizer.zero_grad()
    loss.backward()
    agent.optimizer.step()    
    print("finish training")
   
    torch.save(agent, "/home/ubuntu/ns-allinone-3.37/ns-3.37/contrib/RL-Lora/NS3-LoraRL/rladr-lorans3/automation-interface/model-store")
    #torch.save(agent.state_dict(), os.path.join(MY_PATH, "model-weights.pt"))
    print("model-store saved")
    torch.save(label, "/home/ubuntu/ns-allinone-3.37/ns-3.37/contrib/RL-Lora/NS3-LoraRL/rladr-lorans3/automation-interface/label-store")
    #torch.save(label.state_dict(), os.path.join(MY_PATH, "label-weights.pt"))
    print("label-store saved")
