/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 University of Padova
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Matteo Perin <matteo.perin.2@studenti.unipd.it
 */

#include "ns3/my-adr-component.h"


//************ tests
#include "ns3/idxnodes.h"
#include "ns3/lrmodel.h"
//*************  tests



namespace ns3 {
namespace lorawan {

////////////////////////////////////////
// LinkAdrRequest commands management //
////////////////////////////////////////

NS_LOG_COMPONENT_DEFINE ("myAdrComponent");

NS_OBJECT_ENSURE_REGISTERED (myAdrComponent);

TypeId myAdrComponent::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::myAdrComponent")
    .SetGroupName ("lorawan")
    .AddConstructor<myAdrComponent> ()
    .SetParent<NetworkControllerComponent> ()
    .AddAttribute ("MultipleGwCombiningMethod",
                   "Whether to average the received power of gateways or to use the maximum",
                   EnumValue (myAdrComponent::MAXIMUM), //AVERAGE AVERAGE
		   MakeEnumAccessor<myAdrComponent::CombiningMethod> (&myAdrComponent::tpAveraging),
                   MakeEnumChecker (myAdrComponent::AVERAGE,
                                    "avg",
                                    myAdrComponent::MAXIMUM,
                                    "max",
                                    myAdrComponent::MINIMUM,
                                    "min"))
    .AddAttribute ("MultiplePacketsCombiningMethod",
                   "Whether to average SNRs from multiple packets or to use the maximum",
                   EnumValue (myAdrComponent::MAXIMUM), //AVERAGE
                   MakeEnumAccessor<myAdrComponent::CombiningMethod> (&myAdrComponent::historyAveraging),
                   MakeEnumChecker (myAdrComponent::AVERAGE,
                                    "avg",
                                    myAdrComponent::MAXIMUM,
                                    "max",
                                    myAdrComponent::MINIMUM,
                                    "min"))
    .AddAttribute ("HistoryRange",
                   "Number of packets to use for averaging",
                   IntegerValue (21),
                   MakeIntegerAccessor (&myAdrComponent::historyRange),
                   MakeIntegerChecker<int> (0, 100))
    .AddAttribute ("ChangeTransmissionPower",
                   "Whether to toggle the transmission power or not",
                   BooleanValue (true),
                   MakeBooleanAccessor (&myAdrComponent::m_toggleTxPower),
                   MakeBooleanChecker ())
  ;
  return tid;
}



//****** tests

int computeSteps(int currsteps)
{
	int steps;

	if (currsteps <= 0)
	{
		steps = 0;
	}
	else
	{
		if (currsteps >= 13)
		{
			steps = 13;
		}
		else
		{
			steps = currsteps ;
		}
	}
    return steps;
}

//****** tests



myAdrComponent::myAdrComponent ()
{
  NS_LOG_FUNCTION_NOARGS();
}

myAdrComponent::~myAdrComponent ()
{
}

void myAdrComponent::OnReceivedPacket (Ptr<const Packet> packet,
                                     Ptr<EndDeviceStatus> status,
                                     Ptr<NetworkStatus> networkStatus)
{
  NS_LOG_FUNCTION (this->GetTypeId () << packet << networkStatus);

  // We will only act just before reply, when all Gateways will have received
  // the packet, since we need their respective received power.
}

//自适应数据速率（ADR）算法的实现:根据终端设备（End Device）的通信状态动态调整其数据传输参数（扩频因子SF和发射功率 TP）,同时集成了强化学习（RL）优化逻辑
void
myAdrComponent::BeforeSendingReply (Ptr<EndDeviceStatus> status,
                                  Ptr<NetworkStatus> networkStatus)
{


  NS_LOG_UNCOND("==== BeforeSendingReply CALLED ====");//new
  NS_LOG_UNCOND("rlagent=" << rlagent  << ", training=" << training);  //new
  NS_LOG_FUNCTION (this << status << networkStatus);



  Ptr<Packet> myPacket = status->GetLastPacketReceivedFromDevice ()->Copy ();
  LorawanMacHeader mHdr;
  LoraFrameHeader fHdr;
  //fHdr.SetAsUplink ();
  myPacket->RemoveHeader (mHdr);
  myPacket->RemoveHeader (fHdr);

  LoraTag tag;
  myPacket->PeekPacketTag(tag);

  //***** tests
  fHdr.SetAdr(true);
  int nodeid = status->GetMac()->GetDevice()->GetNode()->GetId();
  //bool nodeReqstatus = fHdr.GetAdrAckReq();

  if (rlagent == 1 or rlagent == 4)
  {
  //*obtaining the SNR of the last received package
    // 获取当前 SNR 和所需 SNR 的差值
   auto it = status->GetReceivedPacketList ().rbegin ();
   double curr_SNR = RxPowerToSNR (GetReceivedPower (it->second.gwList));  // 当前实际SNR
   uint8_t currspreadingFactor = status->GetFirstReceiveWindowSpreadingFactor ();
   double reqcurr_SNR = treshold[SfToDr (currspreadingFactor)]; // 当前SF所需的最低SNR，当前使用的扩频因子（SF）要求的最低SNR阈值（不同SF的阈值不同，SF越高需求越低）。
   double margincurr_SNR = curr_SNR - reqcurr_SNR; // SNR余量，实际SNR超出需求SNR的量，反映通信的“安全裕度”。
   //converting into real steps to be used
   int currsteps = floor (margincurr_SNR / 3);

   std::cerr<<"curr_SNR:"<<curr_SNR<<std::endl;
   std::cerr << "[DEBUG] Current SF: " << static_cast<int>(currspreadingFactor) << std::endl;
   std::cerr<<"reqcurr_SNR:"<<reqcurr_SNR<<std::endl;


    //Get the SF used by the device// 获取设备当前的 SF 和发射功率
    uint8_t received_spreadingFactor = unsigned(status->GetFirstReceiveWindowSpreadingFactor ());

    //Get the device transmission power (dBm)
    double received_transmissionPower = status->GetMac ()->GetTransmissionPower ();

   std::cerr<<"trialdevices[nodeid].success"<< trialdevices[nodeid].success<<std::endl;

      //增加关于crdsa的惩罚项
  Ptr<ClassAEndDeviceLorawanMac> gwMac = status->GetMac ();
  double crdsa_collision_reduction = gwMac->GetEnableCRDSA() ? 1.0 / (1.0 + tag.GetReplicaId() * 0.6) : 1.0; // 经验值：每增加1个副本降低40%碰撞概率

  double effective_success_rate = (float(trialdevices[nodeid].success)/(float(trialdevices[nodeid].success) + float(trialdevices[nodeid].fail) * crdsa_collision_reduction));
  double crdsa_energy_penalty = gwMac->GetEnableCRDSA() ? 1.0 + 0.3 * tag.GetReplicaId() : 1.0; // 每副本增加30%能耗
  double normalized_energy = ((float(trialdevices[nodeid].pwr_ii) * crdsa_energy_penalty * 100) / float(global_power));
  int crdsa_flag = gwMac->GetEnableCRDSA() ? 1 : 0;
  double success_component = effective_success_rate * 100;
  double energy_component = 1.0 / normalized_energy;

// 权重系数（可调参数）
    double new_alpha = 0.7;  // 成功率权重
    double new_beta = 0.3;   // 能耗权重


   //If this is the first packet being received// 第一次成功接收包时的初始化
   if (trialdevices[nodeid].success == 1 and EPS != 0 )
   {


    trialdevices[nodeid].instant_reward_ii = 
    (new_alpha * success_component) + 
    (new_beta * energy_component * (gwMac->GetEnableCRDSA() ? 0.9 : 1.0)); // CRDSA能耗惩罚
    
	  //  trialdevices[nodeid].instant_reward_ii = double(((float(trialdevices[nodeid].success)/(float(trialdevices[nodeid].success)+float(trialdevices[nodeid].fail)))*100)/(((float(trialdevices[nodeid].pwr_ii))*100)/float(global_power))); // 计算初始奖励
	   trialdevices[nodeid].snr_ii=computeSteps(currsteps);
	   trialdevices[nodeid].action_ii = rdaction(); // 随机选择动作

     std::cerr << "trialdevices[nodeid].action_ii: " << trialdevices[nodeid].action_ii << std::endl;
	  //  trialdevices[nodeid].mdp_state_ii = ((48*trialdevices[nodeid].snr_ii)+(8*((trialdevices[nodeid].state_ii[0])-7))+ (((16-received_transmissionPower)-2)/2));
	   //std::cout<<"Action selected is envio 1 "<< trialdevices[nodeid].action_ii << std::endl;
     trialdevices[nodeid].mdp_state_ii = 
    (96 * trialdevices[nodeid].snr_ii) +       // 原SNR部分放大2倍
    (16 * (trialdevices[nodeid].state_ii[0] - 7)) + 
    ((32 - received_transmissionPower) / 2) +  // 功率部分调整
    crdsa_flag;             
   }

   // 第二次成功接收包时的处理
   if (trialdevices[nodeid].success == 2 and EPS != 0 )
   {
	   trialdevices[nodeid].instant_reward_i = trialdevices[nodeid].instant_reward_ii;
         trialdevices[nodeid].instant_reward_ii = 
    (new_alpha * success_component) + 
    (new_beta * energy_component * (gwMac->GetEnableCRDSA() ? 0.9 : 1.0)); // CRDSA能耗惩罚
	  //  trialdevices[nodeid].instant_reward_ii = double(((float(trialdevices[nodeid].success)/(float(trialdevices[nodeid].success)+float(trialdevices[nodeid].fail)))*100)/(((float(trialdevices[nodeid].pwr_ii))*100)/float(global_power)));  // 更新奖励
	   trialdevices[nodeid].snr_i=trialdevices[nodeid].snr_ii;
	   trialdevices[nodeid].snr_ii=computeSteps(currsteps);
	   //update Q for the first time with this dev
	   //Q[trialdevices[nodeid].mdp_state_ii][trialdevices[nodeid].action_ii]= trialdevices[nodeid].instant_reward_ii - trialdevices[nodeid].instant_reward_i;
	   trialdevices[nodeid].mdp_state_i = trialdevices[nodeid].mdp_state_ii;
	  //  trialdevices[nodeid].mdp_state_ii = ((48*trialdevices[nodeid].snr_ii)+(8*((trialdevices[nodeid].state_ii[0])-7))+ (((16-received_transmissionPower)-2)/2)); //更新状态
    trialdevices[nodeid].mdp_state_ii = 
    (96 * trialdevices[nodeid].snr_ii) +       // 原SNR部分放大2倍
    (16 * (trialdevices[nodeid].state_ii[0] - 7)) + 
    ((32 - received_transmissionPower) / 2) +  // 功率部分调整
    crdsa_flag; 

      
	   trialdevices[nodeid].action_i = trialdevices[nodeid].action_ii;
	   trialdevices[nodeid].action_ii = rdaction(); // 随机选择动作
	   //std::cout<<"Action selected is envio 2 "<< trialdevices[nodeid].action_ii << std::endl;
   }

   //常规强化学习处理
   if (trialdevices[nodeid].success > 2 or EPS == 0)
   {
    std::cerr << "RL or DQN" << std::endl;
	   trialdevices[nodeid].snr_i=trialdevices[nodeid].snr_ii;
	   trialdevices[nodeid].snr_ii=computeSteps(currsteps);
	   //update Q
	   // 执行强化学习更新
	   if (rlagent == 1 or rlagent == 4 )
	   { 
	   	rlprocess(nodeid,0,received_spreadingFactor,received_transmissionPower);
	   }
      // 更新状态和动作
	   trialdevices[nodeid].mdp_state_i = trialdevices[nodeid].mdp_state_ii;
    trialdevices[nodeid].mdp_state_ii = 
    (96 * trialdevices[nodeid].snr_ii) +       // 原SNR部分放大2倍
    (16 * (trialdevices[nodeid].state_ii[0] - 7)) + 
    ((32 - received_transmissionPower) / 2) +  // 功率部分调整
    crdsa_flag;  
	  //  trialdevices[nodeid].mdp_state_ii = ((48*trialdevices[nodeid].snr_ii)+(8*((trialdevices[nodeid].state_ii[0])-7))+(((16-received_transmissionPower)-2)/2));//更新状态
	   trialdevices[nodeid].action_i = trialdevices[nodeid].action_ii;
	   //chose the best action for the next sending
     std::cerr<<"chose_action"<<std::endl;
	   trialdevices[nodeid].action_ii = chose_action(nodeid); // 根据策略选择动作

   }

   if (trialdevices[nodeid].success == 0)
   {
	   std::cout<<"Panic. Something went wrong"<<std::endl;
   }

  
  }//Finish the entire RL processing loop


  //传统 ADR 算法处理
  //Execute the ADR algorithm only if the request bit is set
 // if (fHdr.GetAdr ()) changing this to be compatible with the rl implementation
 if ((fHdr.GetAdr ()) and (rlagent == 0)) //***** tests
 {
    std::cerr << "ADR" << std::endl;

	  if (int(status->GetReceivedPacketList ().size ()) < historyRange)
        {
          // NS_LOG_ERROR ("Not enough packets received by this device (" << status->GetReceivedPacketList ().size () << ") for the algorithm to work (need " << historyRange << ")");

          std::cerr << "Not enough packets received by this device (" << status->GetReceivedPacketList ().size () << ") for the algorithm to work (need " << historyRange << ")" << std::endl;
          //****** tests
           //std::cout << "Not enough packets for the node " << nodeid << " "<< status->GetReceivedPacketList ().size ()<< std::endl;
           std::cerr << "Node " << nodeid << " received " << status->GetReceivedPacketList().size() 
          << " packets at time " << Simulator::Now().GetSeconds() << std::endl;
          //****** tests

        }
      else
        {
          NS_LOG_DEBUG ("New ADR request");
          std::cerr<<"New ADR request"<<std::endl;
// 获取当前参数
          //Get the SF used by the device
          uint8_t spreadingFactor = status->GetFirstReceiveWindowSpreadingFactor ();

          //Get the device transmission power (dBm)
          uint8_t transmissionPower = status->GetMac ()->GetTransmissionPower ();

        std::cerr<<"SF:"<<static_cast<unsigned>(spreadingFactor)<<" ,transmissionPower:"<<static_cast<unsigned>(transmissionPower)<<std::endl;

// 执行 ADR 算法          
	  //New parameters for the end-device
          uint8_t newDataRate;
          uint8_t newTxPower;

          //ADR Algorithm
          AdrImplementation (&newDataRate,
                             &newTxPower,
                             status);


          //**** tests

// 处理功率调整
          // Change the power back to the default if we don't want to change it
          std::cerr<<"m_toggleTxPower: "<<m_toggleTxPower<<" ,newDataRate="<<static_cast<unsigned>(newDataRate)<<" ,newTxPower="<<static_cast<unsigned>(newTxPower)<<std::endl;
          if (!m_toggleTxPower)
            {
              newTxPower = transmissionPower;
            }
// 如果需要更改参数，发送 LinkAdrReq
          if (newDataRate != SfToDr (spreadingFactor) || newTxPower != transmissionPower)
            {
              //Create a list with mandatory channel indexes
              int channels[] = {0, 1, 2};
              std::list<int> enabledChannels (channels,
                                              channels + sizeof(channels) /
                                              sizeof(int));

              //Repetitions Setting
              const int rep = 1;

              NS_LOG_DEBUG ("Sending LinkAdrReq with DR = " << (unsigned)newDataRate << " and TP = " << (unsigned)newTxPower << " dBm");
              std::cerr<<"newSF:"<<(unsigned)newDataRate<<" ,newTP:"<<(unsigned)newTxPower<<std::endl;

              status->m_reply.frameHeader.AddLinkAdrReq (newDataRate,
                                                         GetTxPowerIndex (newTxPower),
                                                         enabledChannels,
                                                         rep);
              status->m_reply.frameHeader.SetAsDownlink ();
              status->m_reply.macHeader.SetMType (LorawanMacHeader::UNCONFIRMED_DATA_DOWN);

              status->m_reply.needsReply = true;
            }
          else
            {
              NS_LOG_DEBUG ("Skipped request");
              std::cerr<<"Skipped request"<<std::endl;
            }
        }
    }
  else
    {
      // Do nothing
    }
}

void myAdrComponent::OnFailedReply (Ptr<EndDeviceStatus> status,
                                  Ptr<NetworkStatus> networkStatus)
{
  NS_LOG_FUNCTION (this->GetTypeId () << networkStatus);
}

void myAdrComponent::AdrImplementation (uint8_t *newDataRate,
                                      uint8_t *newTxPower,
                                      Ptr<EndDeviceStatus> status)
{

	   //***** tests
	 std::cerr<<"******* Real ADR AdrImplementation ******"<<std::endl;
     	  //***** tests

   //Compute the maximum or median SNR, based on the boolean value historyAveraging
  double m_SNR = 0;
  switch (historyAveraging)
    {
    case myAdrComponent::AVERAGE:
      m_SNR = GetAverageSNR (status->GetReceivedPacketList (),
                             historyRange);
      break;
    case myAdrComponent::MAXIMUM:
      m_SNR = GetMaxSNR (status->GetReceivedPacketList (),
                         historyRange);
      break;
    case myAdrComponent::MINIMUM:
      m_SNR = GetMinSNR (status->GetReceivedPacketList (),
                         historyRange);
    }

  NS_LOG_DEBUG ("m_SNR = " << m_SNR);
  std::cerr<<"m_SNR:"<<m_SNR<<std::endl;
  //** tests
  //std::cout<<"The SNR starts with this " << m_SNR <<std::endl;
  //** tests

  //Get the SF used by the device
  uint8_t spreadingFactor = status->GetFirstReceiveWindowSpreadingFactor ();

  NS_LOG_DEBUG ("SF = " << (unsigned)spreadingFactor);
  std::cerr<<"SF:"<<static_cast<unsigned>(spreadingFactor)<<std::endl;

  //Get the device data rate and use it to get the SNR demodulation treshold
  double req_SNR = treshold[SfToDr (spreadingFactor)];
  NS_LOG_DEBUG ("Required SNR = " << req_SNR);
  std::cerr<<"req_SNR:"<<req_SNR<<std::endl;

  //Get the device transmission power (dBm)
  double transmissionPower = status->GetMac ()->GetTransmissionPower ();

  NS_LOG_DEBUG ("Transmission Power = " << transmissionPower);
  std::cerr<<"transmissionPower:"<<transmissionPower<<std::endl;

  //Compute the SNR margin taking into consideration the SNR of
  //previously received packets
  double margin_SNR = m_SNR - req_SNR;

  NS_LOG_DEBUG ("Margin = " << margin_SNR);
  std::cerr<<"Margin:"<<margin_SNR<<std::endl;

  //Number of steps to decrement the SF (thereby increasing the Data Rate)
  //and the TP.
  int steps = std::floor (margin_SNR / 3);

  NS_LOG_DEBUG ("steps = " << steps);
  std::cerr<<"steps:"<<steps<<std::endl;

  //If the number of steps is positive (margin_SNR is positive, so its
  //decimal value is high) increment the data rate, if there are some
  //leftover steps after reaching the maximum possible data rate
  //(corresponding to the minimum SF) decrement the transmission power as
  //well for the number of steps left.
  //If, on the other hand, the number of steps is negative (margin_SNR is
  //negative, so its decimal value is low) increase the transmission power
  //(note that the SF is not incremented as this particular algorithm
  //expects the node itself to raise its SF whenever necessary).
  while (steps > 0 && spreadingFactor > min_spreadingFactor)
    {

      //***** tests
       //std::cout<<"Steps "<< steps <<std::endl;
       // std::cout<<"spreadingFactor "<< (unsigned)spreadingFactor <<std::endl;
       // std::cout<<"spreadingFactor "<< status->GetFirstReceiveWindowSpreadingFactor() <<std::endl;
        //***** tests

	  spreadingFactor--;
      steps--;
      NS_LOG_DEBUG ("Decreased SF by 1");
      std::cerr<<"Decreased SF by 1"<<std::endl;

      //***** tests
        //std::cout<<"Steps "<< steps <<std::endl;
        //std::cout<<"spreadingFactor "<< (unsigned)spreadingFactor <<std::endl;
        //std::cout<<"spreadingFactor "<< status->GetFirstReceiveWindowSpreadingFactor() <<std::endl;
        //***** tests
    }
  while (steps > 0 && transmissionPower > min_transmissionPower)
    {
	  	  	  //***** tests
	          // std::cout<<"Steps "<< steps <<std::endl;
	          // std::cout<<"transmissionPower "<< transmissionPower <<std::endl;
	          //***** tests

	  transmissionPower -= 2;
      steps--;
      NS_LOG_DEBUG ("Decreased Ptx by 2");
      std::cerr<<"Decreased Ptx by 2"<<std::endl;
      //***** tests
      //   std::cout<<"Steps "<< steps <<std::endl;
      //   std::cout<<"transmissionPower "<< transmissionPower <<std::endl;
        //***** tests
    }
  while (steps < 0 && transmissionPower < max_transmissionPower)
    {
        //***** tests
      //  std::cout<<"Steps "<< steps <<std::endl;
     //    std::cout<<"transmissionPower "<< transmissionPower <<std::endl;
        //***** tests

	  transmissionPower += 2;
      steps++;
      NS_LOG_DEBUG ("Increased Ptx by 2");
      std::cerr<<"Increased Ptx by 2"<<std::endl;
        //***** tests
      //***** tests
       //  std::cout<<"Steps "<< steps <<std::endl;
      //   std::cout<<"transmissionPower "<< transmissionPower <<std::endl;
        //***** tests
     }


  	  	  //***** tests
           //std::cout<<"Steps "<< steps <<std::endl;
           //std::cout<<"transmissionPower "<< transmissionPower << "new data rate "<<  SfToDr (spreadingFactor) <<std::endl;
          //***** tests


  *newDataRate = SfToDr (spreadingFactor);
  *newTxPower = transmissionPower;



}

uint8_t myAdrComponent::SfToDr (uint8_t sf)
{
  switch (sf)
    {
    case 12:
      return 0;
      break;
    case 11:
      return 1;
      break;
    case 10:
      return 2;
      break;
    case 9:
      return 3;
      break;
    case 8:
      return 4;
      break;
    default:
      return 5;
      break;
    }
}

double myAdrComponent::RxPowerToSNR (double transmissionPower)  
{
  //The following conversion ignores interfering packets
  return transmissionPower + 174 - 10 * log10 (B) - NF;
}

//Get the maximum received power (it considers the values in dB!)
double myAdrComponent::GetMinTxFromGateways (EndDeviceStatus::GatewayList gwList)
{
  EndDeviceStatus::GatewayList::iterator it = gwList.begin ();
  double min = it->second.rxPower;

  for (; it != gwList.end (); it++)
    {
      if (it->second.rxPower < min)
        {
          min = it->second.rxPower;
        }
    }

  return min;
}

//Get the maximum received power (it considers the values in dB!)
double myAdrComponent::GetMaxTxFromGateways (EndDeviceStatus::GatewayList gwList)
{
  EndDeviceStatus::GatewayList::iterator it = gwList.begin ();
  double max = it->second.rxPower;

  for (; it != gwList.end (); it++)
    {
      if (it->second.rxPower > max)
        {
          max = it->second.rxPower;
        }
    }

  return max;
}

//Get the maximum received power
double myAdrComponent::GetAverageTxFromGateways (EndDeviceStatus::GatewayList gwList)
{
  double sum = 0;

  for (EndDeviceStatus::GatewayList::iterator it = gwList.begin (); it != gwList.end (); it++)
    {
      NS_LOG_DEBUG ("Gateway at " << it->first << " has TP " << it->second.rxPower);
      sum += it->second.rxPower;
    }

  double average = sum / gwList.size ();

  NS_LOG_DEBUG ("TP (average) = " << average);

  return average;
}

double
myAdrComponent::GetReceivedPower (EndDeviceStatus::GatewayList gwList)
{
  switch (tpAveraging)
    {
    case myAdrComponent::AVERAGE:
      return GetAverageTxFromGateways (gwList);
    case myAdrComponent::MAXIMUM:
      return GetMaxTxFromGateways (gwList);
    case myAdrComponent::MINIMUM:
      return GetMinTxFromGateways (gwList);
    default:
      return -1;
    }
}

// TODO Make this more elegant
double myAdrComponent::GetMinSNR (EndDeviceStatus::ReceivedPacketList packetList,
                                int historyRange)
{
  double m_SNR;

  //Take elements from the list starting at the end
  auto it = packetList.rbegin ();
  double min = RxPowerToSNR (GetReceivedPower (it->second.gwList));

  for (int i = 0; i < historyRange; i++, it++)
    {
      m_SNR = RxPowerToSNR (GetReceivedPower (it->second.gwList));

      NS_LOG_DEBUG ("Received power: " << GetReceivedPower (it->second.gwList));
      NS_LOG_DEBUG ("m_SNR = " << m_SNR);

      if (m_SNR < min)
        {
          min = m_SNR;
        }
    }

  NS_LOG_DEBUG ("SNR (min) = " << min);

  return min;
}

double myAdrComponent::GetMaxSNR (EndDeviceStatus::ReceivedPacketList packetList,
                                int historyRange)
{
  double m_SNR;

  //Take elements from the list starting at the end
  auto it = packetList.rbegin ();
  double max = RxPowerToSNR (GetReceivedPower (it->second.gwList));

  for (int i = 0; i < historyRange; i++, it++)
    {
      m_SNR = RxPowerToSNR (GetReceivedPower (it->second.gwList));

      NS_LOG_DEBUG ("Received power: " << GetReceivedPower (it->second.gwList));
      NS_LOG_DEBUG ("m_SNR = " << m_SNR);

      if (m_SNR > max)
        {
          max = m_SNR;
        }
    }

  NS_LOG_DEBUG ("SNR (max) = " << max);

  return max;
}

double myAdrComponent::GetAverageSNR (EndDeviceStatus::ReceivedPacketList packetList,
                                    int historyRange)
{
  double sum = 0;
  double m_SNR;

  //Take elements from the list starting at the end
  auto it = packetList.rbegin ();
  for (int i = 0; i < historyRange; i++, it++)
    {
      m_SNR = RxPowerToSNR (GetReceivedPower (it->second.gwList));

      NS_LOG_DEBUG ("Received power: " << GetReceivedPower (it->second.gwList));
      NS_LOG_DEBUG ("m_SNR = " << m_SNR);

      sum += m_SNR;
    }

  double average = sum / historyRange;

  NS_LOG_DEBUG ("SNR (average) = " << average);

  return average;
}

int myAdrComponent::GetTxPowerIndex (int txPower)
{
  if (txPower >= 16)
    {
      return 0;
    }
  else if (txPower >= 14)
    {
      return 1;
    }
  else if (txPower >= 12)
    {
      return 2;
    }
  else if (txPower >= 10)
    {
      return 3;
    }
  else if (txPower >= 8)
    {
      return 4;
    }
  else if (txPower >= 6)
    {
      return 5;
    }
  else if (txPower >= 4)
    {
      return 6;
    }
  else
    {
      return 7;
    }
}

//// *******************tests
int myAdrComponent::GetIndexTxPower (int index)
{
  if (index == 0)
    {
      return 16;
    }
  else if (index == 1)
    {
      return 14;
    }
  else if (index ==  2)
    {
      return 12;
    }
  else if (index ==  3)
    {
      return 10;
    }
  else if (index ==  4)
    {
      return 8;
    }
  else if (index ==  5)
    {
      return 6;
    }
  else if (index ==  6)
    {
      return 4;
    }
  else
    {
      return 2;
    }
}


int myAdrComponent::ADRLR (int SF, int step)
{

	//retv = system("cd /home/zuolo/src/my-schc/ && /usr/bin/python3 process-rules.py");
	//int size = m_basePktSize + (retv/256);

	std::string sf = std::to_string(int(SF));
	std::string st = std::to_string(int(step));

	//std::string cmd = "cd /home/zhou/tarballs/ns-3-allinone/ns-3.37/contrib/NS3-LoraRL/rladr-lorans3/lorawan/adr-rl && /usr/bin/python3 lora-game.py " + sf + " " + st;
  std::string cmd = "cd /home/ubuntu/ns-allinone-3.37/ns-3.37/contrib/RL-Lora/NS3-LoraRL/adr-rl/lora-game.py " + sf + " " + st;
  
	return system(cmd.c_str());
	//std::cout << "saida "<< size<<std::endl;
	//int size = (retv/256);

	//std::cout<<" Saida " << retv << std::endl;



}






//// *******************tests

}
}
