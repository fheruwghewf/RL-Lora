#include <cstdint>
#include "stdio.h"
#include <iostream>
#include "ns3/idxnodes.h"
#include <ctime>
#include <time.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <fstream>
#include <curses.h>
#include <Python.h>
#include <sstream>  // 添加头文件
#include "ns3/log.h"
#include <vector>

namespace ns3 { namespace lorawan {

NS_LOG_COMPONENT_DEFINE("LrModel");

double** Q = new double*[1344];



float ToA [6] = {66.8,123.4,226.3,411.6,823.3,1646.6};	

bool training = true;

float ALPHA = 0.7; // learning rate
float GAMMA = 0.8; // discount factor
float EPS = 0.9; // exploration factor

//bool m_enableCRDSA = true;
int freq_on_air[3] = {0,0,0} ;
int st_freq = 2 ;
int nd_freq = 1 ;
int rd_freq = 0;



int freq_states[] = {4,5,8,13,10,14};


//compute next_action


// select random action
int rdaction ( )
{
    const boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
    const boost::posix_time::time_duration td = now.time_of_day();
    const long hours        = td.hours();
    const long minutes      = td.minutes();
    const long seconds      = td.seconds();
    const long milliseconds = td.total_milliseconds() - ((hours * 3600 + minutes * 60 + seconds) * 1000);
    srand( int(milliseconds ));

    //srand(system("/usr/bin/od -vAn -N4 -t u4 < /dev/urandom"));    
    return (rand() % 96);

}

//Save Q value into file
void SaveQ (double **Q)
{

		std::ofstream myfile;
		myfile.open ("/home/ubuntu/zxq/NS3-LoraRL/rladr-lorans3/automation-interface/array.txt");

	      for(int i = 0; i < (1344);i++)
		  {
			  for (int j=0; j<96;j++)
			  {

				  myfile << Q[i][j];
				  myfile << "|";
			  }
			  myfile << "\n";
		  }

		  myfile.close();
}

//Debug printing reward
void get_reward(int nodeid)
{
	//std::cout<<"this was the decision "<<trialdevices[FindNode(nodeid)].action_ii  <<" in the state "<< trialdevices[FindNode(nodeid)].snr_ii <<"with reward"<< trialdevices[FindNode(nodeid)].reward_total_ii << "by the node "<< nodeid<<std::endl;
}

//Decode Frequency from action index
// double fr_from_action(int action)
// {
// 	int fr_base = int(action/48);
// 	double FCP[3] = {868.1,868.3,868.5};

// 	return FCP[fr_base];
// }
 
double fr_from_action(int action) {
    const double FCP[3] = {868.1, 868.3, 868.5};
    return FCP[(action / 32) % 3];  // 将95个动作均匀分配到3个频率
}

//Decode SF from action index
// int sf_from_action(int action)
// {

// 	int sftx = action;
// 	int sf = int(sftx/8);  // 95/8=11
// 	return sf+7;   // // 返回18（但LoRa SF通常只到12）,当action=95时返回SF18，超出LoRa标准范围（SF7-SF12）

// }

int sf_from_action(int action) {
    return 7 + (action % 6);  // SF7-SF12
}

//Decode TxPwr from action index
int tx_from_action(int action)
{

	return (2*((action%8)+1));


}

//Decode CRDSA from action index
bool crdsa_from_action(int action)
{
	return bool(action % 2);
}

//find the max within the least used frequency interval
//  0-47  being freq 0 the least used
// 48-95  being freq 1  the least used
// 48-143 being freq 2 the least used
//
// quota is defined but not in use yet
// (define whether penalties will be applied for physical prohibited choices)
//从给定状态的Q表中，选择一个Q值最大的动作，返回动作索引
int find_max(int state,int border,int idx)
{
	double max = -1000;
	int maxid = 0;

	for(int i = 0;i < 96; i++)
    {
		 if( max < Q[state][i])
          {
	
			 max = Q[state][i];
			 maxid = i;
          }

    }


	/*if (Q[state][maxid] == 0)
	{
		maxid = rdaction();
	}*/

    std::cout<<"Acao escolhida "<<maxid<<" valor "<<Q[state][maxid]<<std::endl; 
    return maxid;
}

//Decode Action index from TxPwr and SF
// int get_action (int sf, int tx)
// {
// 	int action = 0;

// 	action = action+((8*sf)+tx);

// 	return action;

// }

// int get_action (int sf, int tx,bool m_enableCRDSA)
// {
// 	int action = 0;
// 	int crdsa_bit = m_enableCRDSA ? 1 : 0;
    
//     // 编码公式: action = (sf_idx << 4) | (tx_power_idx << 1) | crdsa_bit
// 	action = action+((sf * 8 * 2) + (tx * 2) + crdsa_bit);

// 	return action;
// }

// 统一编码规则（示例）：
// Bit 0: CRDSA
// Bit 1-3: TX power (0-7)
// Bit 4-6: SF index (0-5 for SF7-SF12)
int get_action(int sf, int tx, bool crdsa) {
    return ((sf-7) << 4) | (tx << 1) | crdsa;
}



//Greedy selection
int chose_action(int nodeid)
{
	std::cerr<<"======chose_action====== "<< nodeid <<std::endl;
	int chosen;
	srand( (unsigned)time( NULL ) );
	//srand(system("/usr/bin/od -vAn -N4 -t u4 < /dev/urandom"));	
	float p = (float) rand()/RAND_MAX;
	std::cerr<<"p= "<<p<<" ,eps= "<<EPS<<std::endl;
	if (p < EPS)
	{
		//chosen = int(p*1000) % 48;
		chosen = rdaction();

		std::cerr << "Random chosen value: " << chosen << std::endl;  // 检查是否超出预期范围
		//std::cout<<"Random Choice "<< trialdevices[nodeid].mdp_state_ii<< " "<<chosen<<std::endl;
		trialdevices[nodeid].random_selection = true;
	}
	else
	{
		//change state condition
		//find the max in the range of the least used frequency
		
		if ( rlagent == 1 )
		{
			std::cout<<"Learned State "<< trialdevices[nodeid].mdp_state_ii<<std::endl;
			chosen=find_max(trialdevices[nodeid].mdp_state_ii,rd_freq, nodeid);
	  		trialdevices[nodeid].random_selection = false;
		}
		else
		{
			std::cerr<<"=======decisionmaker======="<<std::endl;
			Py_SetPythonHome(L"/home/ubuntu/anaconda3/envs/dl");
			// PyThreadState *statepy = Py_NewInterpreter();
			// Py_Initialize();// 初始化Python解释器
			std::cerr<<"python opened!!!!!!!!!!"<<std::endl;
			PyRun_SimpleString("import sys\n"
				"sys.path.append('/home/ubuntu/zxq/ns-allinone-3.37/ns-3.37/contrib/lorawan/model/')");
			PyObject *pName;
        	PyObject *module;
		    pName = PyUnicode_DecodeFSDefault("dqnadr");  // 加载Python模块
        	module = PyImport_Import(pName);
        	Py_DECREF(pName);
//通过 Python/C API 调用 DQN 模型（dqnadr.decisionmaker）选择动作
		    PyObject *func = PyObject_GetAttrString(module,"decisionmaker"); // 获取函数		
			PyObject *arg_state = Py_BuildValue("(i)",trialdevices[nodeid].mdp_state_ii);// 构建参数
			PyObject *selected_action = PyObject_CallObject(func, arg_state);   // 调用函数
			//PyObject *selected_action = PyEval_CallObject(func,NULL);
			//PyEval_CallObject(func,NULL);
			
			int *resulting_action =new int();
			int *resulting_output =new int();
			PyArg_ParseTuple(selected_action, "ii", resulting_action, resulting_output); // 解析返回值


			chosen = *resulting_action; // 获取选择的动作
	  		trialdevices[nodeid].random_selection = false;
			std::cerr<<"Chosen action "<< chosen << " SF "<< sf_from_action(chosen) << " Tx  " << tx_from_action(chosen)  <<std::endl;
			// Py_Finalize();		 //解释器释
			// std::cerr<<Py_FinalizeEx()<<std::endl;
			// Py_EndInterpreter(statepy);
			std::cerr<<"python closed!!!!!!!!!!"<<std::endl;	
		}
	}

        if (((EPS - 0.001) / global_sent) > 0)
        {
	    std::cout<<"New EPS "<<EPS <<" ,global_sent "<<global_sent <<std::endl;
            EPS = EPS - ((EPS - 0.001) / global_sent);
        }
        else
        {
            EPS = 0;
        }
	
	//std::cout<<"Action taken is "<< chosen << " SF "<< sf_from_action(chosen) << " Tx  " << tx_from_action(chosen)  <<std::endl;
	
	return chosen;
}


//Run the RL process 
void rlprocess(int nodeid, int rectype, uint8_t received_sf, double received_tx)
{
	//std::cerr << "Entering RL update function" << std::endl;
	NS_LOG_FUNCTION_NOARGS();
	switch (rectype)
	{
	case 0://received with success


		bool mismatch;

		mismatch = false; 


		//check if must penalize previous round
		if (      ((sf_from_action(trialdevices[nodeid].action_ii) != (int(unsigned(received_sf))))) or ((tx_from_action(trialdevices[nodeid].action_ii)) != (16-received_tx))  )     
		{ 
			mismatch = true; 
		}		

		//check if the battery used is the maximum so far

		if  (trialdevices[nodeid].pwr_ii > trialdevices[nodeid].max_pwr)  trialdevices[nodeid].max_pwr = trialdevices[nodeid].pwr_ii;
		
		//assign data rate and battery rate to local variables// 将数据速率和电池速率分配给本地变量
		float prate; 
		prate = trialdevices[nodeid].pwr_ii/trialdevices[nodeid].max_pwr;
		float drate; 
		drate = (66.8/(ToA[trialdevices[nodeid].state_ii[0]-7]));
		
		std::cerr<<"prate "<<prate<<std::endl; 
		std::cerr<<"drate "<<drate<<std::endl;
		//if last command reflects back, then give a reward, otherwise penalize//如果最后一条命令得到回复，则给予奖励，否则进行惩罚
		if ( mismatch )
		{ 
			trialdevices[nodeid].reward_total_ii = -(drate*prate); 
			//trialdevices[nodeid].reward_total_ii = -1; 
		}
		else
		{
			trialdevices[nodeid].reward_total_ii = (drate)-(drate*prate);
			//trialdevices[nodeid].reward_total_ii = 0;
		}

		std::cerr<<"pkg_on_air "<<trialdevices[nodeid].pkg_on_air<<", training= "<<training<<",EPS= "<<EPS<<std::endl; 
		
		//if (trialdevices[nodeid].pkg_on_air > 1 and EPS != 0)
		//if (trialdevices[nodeid].pkg_on_air > 1 )
		if (trialdevices[nodeid].pkg_on_air > 1 and training and EPS != 0)
		{
			int state1 = int(trialdevices[nodeid].mdp_state_i);
			int state2 = int(trialdevices[nodeid].mdp_state_ii);
			int act1 = int(trialdevices[nodeid].action_i);
			int act2 = int(trialdevices[nodeid].action_ii);
			std::cerr<<"state1: "<< state1 <<" ,state2 :"<< state2<<std::endl;
			std::cerr<<"act1: "<< act1 <<", act2 :"<< act2<<std::endl;

			//std::cout<<"To verify "<<state1<<" "<<state2<<" "<<act1<<" "<<act2<<" "<<trialdevices[nodeid].reward_total_ii <<std::endl;	
			//std::cout<<"Supposedly running RL training "<<rlagent <<std::endl; 
			//根据数据包接收结果（成功/失败）更新节点的 Q 表（Q-learning）或更新DQN经验回放缓冲区
			if ( rlagent == 1)
			{
				std::cerr<<"Supposedly running RL training "<<std::endl; 
				Q[state1][act1] = Q[state1][act1] + (ALPHA*(trialdevices[nodeid].reward_total_ii + ((GAMMA*Q[state2][act2]) - Q[state1][act1])));

				// 使用cerr确保立即输出（不受NS-3日志控制）
				std::cerr << "[DEBUG] Updating Q[" << state1 << "][" << act1 << "] : " << Q[state1][act1] << std::endl;
			}
			else
			{
				//If using DQN, record every interaction into the memory
				//Every 10 rounds, run the optimizer
				std::cerr<<"Supposedly running DQN training "<<std::endl; 
				Py_SetPythonHome(L"/home/ubuntu/anaconda3/envs/dl");
				// PyThreadState *statepy = Py_NewInterpreter();
				// Py_Initialize();  //// 初始化Python解释器
				std::cerr<<"python opened!!!!!!!!!!"<<std::endl;
				PyRun_SimpleString("import sys\n"
					"sys.path.append('/home/ubuntu/zxq/ns-allinone-3.37/ns-3.37/contrib/lorawan/model/')");
				PyObject *pName;
        		PyObject *module;
				
		        pName = PyUnicode_DecodeFSDefault("dqnadr");
        		module = PyImport_Import(pName);
				if (!module) { //检查模块加载
					PyErr_Print();
					std::cerr << "Error: Failed to load dqnadr.py" << std::endl;
					return;
				}
        		Py_DECREF(pName);
		        PyObject *func = PyObject_GetAttrString(module,"remember");		
				PyObject *arg_bellman = Py_BuildValue("(iiif)",state1,state2,act1,trialdevices[nodeid].reward_total_ii);	
				PyObject_CallObject(func,arg_bellman);
				
				std::cerr<<"(trialdevices[nodeid].success + trialdevices[nodeid].fail)= "<<(trialdevices[nodeid].success + trialdevices[nodeid].fail) <<std::endl;
				if ( ((trialdevices[nodeid].success + trialdevices[nodeid].fail) % 3) == 0)
				{	
		        	PyObject *func = PyObject_GetAttrString(module,"train");		
					PyObject *arg_bellman = Py_BuildValue("(iiif)",state1,state2,act1,trialdevices[nodeid].reward_total_ii);	
					PyObject_CallObject(func,arg_bellman);
				}
				// std::cerr<<Py_FinalizeEx()<<std::endl;
				// Py_EndInterpreter(statepy);
				std::cerr<<"python closed!!!!!!!!!!"<<std::endl;
			}

		}
		else
		{
			std::cerr<<"Not running RL and DQN training "<<std::endl; 
		}
		break;
	case 1://received with interference or under threshold

		break;


	}
}

//Update used carrier frequency - Decrease
void release_freq(int freq, int nodeid)
{

	if (trialdevices[nodeid].pkg_on_air > 1 )
	{
		freq_on_air[freq] = freq_on_air[freq] - 1;
	}


}
//Update used carrier frequency - Increase
void alloc_freq(int freq)
{
	freq_on_air[freq] = freq_on_air[freq] + 1;
}

//Sort the carrier frequencies used
int sort_freq()
{
	 int min = freq_on_air[0];
	 int max = freq_on_air[2];
	 //int mid = freq_on_air[1];
	 int index_min = 0; //start min with 868.1
	 int index_mid = 1; //start min with 868.3
	 int index_max = 2; //start min with 868.5


	 for (int i=0;i<3;i++)
	 {

		 	 if (freq_on_air[i] < min)
			 {
				 min = freq_on_air[i];
				 index_min = i;
			 }
			 else
			 {
				if (freq_on_air[i] > max)
		 	 	 	 {
		 	     	 	 max = freq_on_air[i];
		 	     	 	 index_max = i;
		 	 	 	 }
			 }


			 index_mid = 3 - index_max - index_min;
			 //mid = freq_on_air[index_mid];

	 }


	 st_freq = index_max;
	 nd_freq = index_mid;
	 rd_freq = index_min;
	

 	 return index_min;
}


} }




