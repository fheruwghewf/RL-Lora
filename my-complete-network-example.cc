
/*
 * This script simulates a complex scenario with multiple gateways and end
 * devices. The metric of interest for this script is the throughput of the
 * network.
 */
#include "ns3/end-device-lora-phy.h"

#include "ns3/gateway-lora-phy.h"
#include "ns3/class-a-end-device-lorawan-mac.h"
#include "ns3/gateway-lorawan-mac.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/lora-helper.h"
#include "ns3/node-container.h"
#include "ns3/mobility-helper.h"
#include "ns3/position-allocator.h"
#include "ns3/double.h"
#include "ns3/random-variable-stream.h"
#include "ns3/periodic-sender-helper.h"
#include "ns3/command-line.h"
#include "ns3/network-server-helper.h"
#include "ns3/correlated-shadowing-propagation-loss-model.h"
#include "ns3/building-penetration-loss.h"
#include "ns3/building-allocator.h"
#include "ns3/buildings-helper.h"
#include "ns3/forwarder-helper.h"
#include <algorithm>
#include <ctime>

//****tests
#include "ns3/idxnodes.h"
#include "ns3/basic-energy-source-helper.h"
#include "ns3/lora-radio-energy-model-helper.h"
#include "ns3/device-energy-model.h"
#include "ns3/lrmodel.h"
#include <algorithm>
#include <iterator>
#include <fstream>
#include <curses.h>

 #include "ns3/core-module.h"
 #include "ns3/network-module.h"
 #include "ns3/mobility-module.h"
 #include "ns3/config-store.h"
 #include <ns3/buildings-helper.h>
 #include <ns3/hybrid-buildings-propagation-loss-model.h>
 #include <ns3/constant-position-mobility-model.h>

 #include "ns3/my-adr-component.h"
#include <ns3/three-gpp-propagation-loss-model.h>

#include "ns3/constant-position-mobility-model.h" // 静态模型
#include "ns3/random-walk-2d-mobility-model.h"    // 随机移动
#include "ns3/waypoint-mobility-model.h"         // 路径点移动

// #include <Python.h> // //
// #include <stdexcept> ////

//****tests


using namespace ns3;
using namespace lorawan;

NS_OBJECT_ENSURE_REGISTERED(myAdrComponent);
NS_LOG_COMPONENT_DEFINE ("myComplexLorawanNetworkExample");

// class PythonInterpreter {
// public:
//     static void Initialize() {
//         if (!Py_IsInitialized()) {
//             Py_Initialize();
//             PyEval_InitThreads();  // 启用多线程支持
//             // 设置Python路径（全局生效）
//             PyRun_SimpleString("import sys");
//             PyRun_SimpleString("sys.path.append('/home/zhou/tarballs/ns-3-allinone/ns-3.37/contrib/lorawan/model/')");
//         }
//     }
//        static void Finalize() {
//         if (Py_IsInitialized()) {
//             Py_Finalize();
//         }
//     }
// };

//****************tests
std::string adrType = "ns3::myAdrComponent";

//****************tests

// Network settings

//tests taking nDevices to idxnodes
int nGateways = 1;
double radius = 6000;

double simulationTime = 14400;// 仿真时间 ,6h，36个周期（至少包含 10个应用周期—）

// Channel model
//bool realisticChannelModel = false;
bool realisticChannelModel = true;

int appPeriodSeconds = 660; //应用周期,需符合 LoRaWAN 占空比限制（如EU868频段通常≤1%）,单节点占空比 = (包空中时间) / appPeriod ≤ 1%
//改成600

// Output control
bool print = true;





//***test

//// Function to print each node with their attributes
void PrintNodes (int lastNode)
{

	for (int i=0;i<lastNode;i++)
		{
			std::cout<<"Node address "<<trialdevices[i].nwkaddr<< " " << trialdevices[i].success <<" "<< trialdevices[i].fail <<" " << trialdevices[i].pwr_total <<" " << trialdevices[i].distance << std::endl;
			std::cout<<"Node distance "<<trialdevices[i].distance << std::endl;
			std::cout<<"Under threshold "<<trialdevices[i].fail_for_under_threshold << " Interference " << int(trialdevices[i].fail_for_interference) <<" Transmitting "<< trialdevices[i].fail_for_transmitting  <<" Demodulators " << trialdevices[i].fail_for_no_demodulator << std::endl;
		}

	std::cout<<"############################################################################################################################################################################"<<std::endl;
	std::cout<<"Result: Under threshold "<< global_under_threshold << " Interference " << global_interference <<" Transmitting "<< global_transmitting  <<" Demodulators " << global_no_demodulator << std::endl;

}

 
//// At the end of the simulation, stores the Q matrix in the hard disk as array.txt for future use
void SaveQ (double **Q)
{

		std::ofstream myfile;
		myfile.open ("/home/zhou/tarballs/ns-3-allinone/ns-3.37/contrib/NS3-LoraRL/rladr-lorans3/automation-interface/array.txt");
      std::cout<<"创建并打开array.txt"<<std::endl;
	      for(int i = 0; i < (1344);i++)
		  {
			  for (int j=0; j<96;j++)
			  {
          if (Q[i][j] != 0) {
            std::cout << "Non-zero value at Q[" << i << "][" << j << "] = " << Q[i][j] << std::endl;
        }

				  myfile << Q[i][j];
				  myfile << "|";
			  }
			  myfile << "\n";
		  }
      std::cout<<"写入array.txt"<<std::endl;
		  myfile.close();

}

//// Loads array.txt file from disk to continue feeding the Q matrix
void LoadQ(double **Q)
{

	    std::ifstream file("/home/zhou/tarballs/ns-3-allinone/ns-3.37/contrib/NS3-LoraRL/rladr-lorans3/automation-interface/array.txt");
	    if(file.is_open())
	    {
        std::cout<<"打开array.txt"<<std::endl;
	    	std::string line;
	    	int j=0;

	    	while (std::getline(file, line))
	    	    {
	    	        std::stringstream ss(line);


	    	        for (int i=0;i<96;i++)
	    	        {
	    	        	std::string value;
	    	        	std::getline(ss,value,'|');
	    	        	Q[j][i] = std::stod(value);
	    	        	//std::cout<<"\""<<vaule<<"\"";
	    	        }

	    	        j++;
	    	    }
	    }
	    else //if the file is not present
	    {

	    		 for(int i = 0; i < ((1344));i++)
	    		    {
	    		      for (int j=0; j<96;j++)
	    		      {
	    		    	  Q[i][j] = 0;
	    			  }
	    		  }

	    }


	    file.close();
}






int
main (int argc, char *argv[])
{
  // // 初始化Python解释器（整个程序生命周期一次）
  // PythonInterpreter::Initialize();

  CommandLine cmd;
  bool enableCRDSA = false;
  cmd.AddValue("enableCRDSA", "Enable CRDSA (true/false)", enableCRDSA);
  cmd.AddValue ("nDevices", "Number of end devices to include in the simulation", nDevices);
  cmd.AddValue ("radius", "The radius of the area to simulate", radius);
  cmd.AddValue ("simulationTime", "The time for which to simulate", simulationTime);
  cmd.AddValue ("appPeriod",
                "The period in seconds to be used by periodically transmitting applications",
                appPeriodSeconds);
  cmd.AddValue ("print", "Whether or not to print various informations", print);
  //** tests
  cmd.AddValue ("pkgbasesize", "Package size provided as argument", pkgbasesize);
  cmd.AddValue ("rlagent", "Activate reinforcement learning agent", rlagent);//// 是否启用RL代理
  cmd.AddValue ("settled", "Position of single node experiment", settled);
  cmd.AddValue ("training", "Whether it is a training/learning round", training);// // 是否为训练模
  cmd.AddValue ("alpha", "RL Alpha value", ALPHA);  // 学习率
  cmd.AddValue ("gamma", "RL Gamma value", GAMMA);  // 折扣因子
  cmd.AddValue ("epsilon", "RL epsilon value", EPS);  //// 探索率

 std::string movementMode = "static"; // 默认静态
 cmd.AddValue("movementMode", "Node movement pattern (static/dispersed/clustered)", movementMode);

  //** tests
  cmd.Parse (argc, argv);



  std::cerr << "===Start=== " << std::endl;

  /***********
   *  Setup  *
   ***********/

  //**** test
   if (rlagent == 1)
  {
	  for(int i = 0; i < ((1344));i++)
	  	  {Q[i] = new double[96];}

	  //Otherwise load Q matrix stored from previous simulations
	  LoadQ(Q);

  }


  //******test

  // Create the time value from the period
  Time appPeriod = Seconds (appPeriodSeconds);

  // Mobility
  // MobilityHelper mobility;
  // mobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator", "rho", DoubleValue (radius),
  //                                "X", DoubleValue (0.0), "Y", DoubleValue (0.0));
  // mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  /************************
   *  Create the channel  *
   ************************/

  // Create the lora channel object
  Ptr<LogDistancePropagationLossModel> loss = CreateObject<LogDistancePropagationLossModel> ();
  loss->SetPathLossExponent (3.76);
  loss->SetReference (1, 7.7);

  if (realisticChannelModel)
    {
      // (阴影衰落）Create the correlated shadowing component
      Ptr<CorrelatedShadowingPropagationLossModel> shadowing =
          CreateObject<CorrelatedShadowingPropagationLossModel> ();
      // Aggregate shadowing to the logdistance loss
      loss->SetNext (shadowing);

      // （建筑物穿透损失）Add the effect to the channel propagation loss
      Ptr<BuildingPenetrationLoss> buildingLoss = CreateObject<BuildingPenetrationLoss> ();
      shadowing->SetNext (buildingLoss);

    }

  Ptr<PropagationDelayModel> delay = CreateObject<ConstantSpeedPropagationDelayModel> ();

  Ptr<LoraChannel> channel = CreateObject<LoraChannel> (loss, delay);

  /************************
   *  Create the helpers  *
   ************************/

  // Create the LoraPhyHelper
  LoraPhyHelper phyHelper = LoraPhyHelper ();
  phyHelper.SetChannel (channel);

  // Create the LorawanMacHelper
  LorawanMacHelper macHelper = LorawanMacHelper ();

  // Create the LoraHelper
  LoraHelper helper = LoraHelper ();
  helper.EnablePacketTracking (); // Output filename
  // helper.EnableSimulationTimePrinting ();

  //Create the NetworkServerHelper
  NetworkServerHelper nsHelper = NetworkServerHelper ();

  //Create the ForwarderHelper
  ForwarderHelper forHelper = ForwarderHelper ();

  /************************
   *  Create End Devices  *
   ************************/

  // Create a set of nodes
  NodeContainer endDevices;
  endDevices.Create (nDevices);

  // Assign a mobility model to each node
  // mobility.Install (endDevices);//删掉的

  // Make it so that nodes are at a certain height > 0
  
  //***tests start
 
//   std::fstream nds;
//    nds.open("/home/zhou/tarballs/ns-3-allinone/ns-3.37/contrib/NS3-LoraRL/movingGateway/CPLEX/nodes.csv",std::ios::in);
//    //nds.open("/home/zhou/tarballs/ns-3-allinone/ns-3.37/contrib/NS3-LoraRL/movingGateway/CPLEX/nodes.csv",std::ios::in);
// ////"/home/zhou/tarballs/ns-3-allinone/ns-3.37/contrib/NS3-LoraRL/rladr-lorans3/automation-interface/nodes.csv"
//   std::cerr<<"ndevice= "<<nDevices<<std::endl;

//    if (nds.is_open())
//    {
//       std::cerr << "可以打开nodes.csv文件" << std::endl;
//       std::string tp;
//       std::vector<std::string> row;
//       std::string word;
//       int loaded_nodes = 0;  //new

//       while(getline(nds, tp)&& loaded_nodes < nDevices) //// 仅读取前 nDevices 行
//       {
//         std::cout <<"行内容："<< tp << std::endl;
// 	      std::stringstream s(tp);
//                 while (getline(s, word, ','))
//                 {
//                 row.push_back(word);
//                 }
//                 loaded_nodes++;

//       }
//       nds.close();  //new
//       std::cerr << row.size()<<std::endl;
//       //Confirming that the CSV has been imported in a multiple of 3 (which follows the expected format)
//       if( ((row.size())%3) != 0 || ( (int(row.size()/3)) != nDevices ) )
//         {
// 		std::cerr << "There is a problem with the nodes CSV input. Please, re-validate it!\n";
// 		std::exit(-1);
//         }

//       //Print out the number of nodes
//       std::cerr<<"The number of nodes is "<< (row.size()+1)/3  <<std::endl;


//       //Prepare to create nodes
//       NodeContainer::Iterator j = endDevices.Begin ();
//       Ptr<Node> node = *j;
//       Ptr<MobilityModel> mobility = (*j)->GetObject<MobilityModel> ();
//       Vector position = mobility->GetPosition ();

//       //Reading the coordinates of each node (every 3 elements)
//       for (std::size_t i = 0; i < row.size(); i=i+3)
//               {
        	
//         	mobility = (*j)->GetObject<MobilityModel> ();
//         	position = mobility->GetPosition ();
        	
// 		      position.x=std::stod(row[i]);
//         	position.y=std::stod(row[i+1]);
//         	position.z=std::stod(row[i+2]);

//         	mobility->SetPosition (position);
//         	trialdevices[node->GetId()].distance=sqrt((pow(position.x,2)+(pow(position.y,2))));
//         	std::cout<<"distance "<<trialdevices[node->GetId()].distance<<" node->GetId() "<< node->GetId() <<std::endl;
//         	double avgDistance = (avgDistance + trialdevices[node->GetId()].distance);
 		
// 		if (i < (row.size()-3) )
// 		{
// 			j++;
// 			node = *j;
// 		}

// 	      }
//       nds.close();
//    }


//***add mobility nodes

// 替换原有的静态加载代码
// MobilityHelper mobility;

MobilityHelper mobility;


if (movementMode == "dispersed") {
    // 离散模式
    mobility.SetPositionAllocator(
        "ns3::UniformDiscPositionAllocator",
        "X", StringValue("0.0"),
        "Y", StringValue("0.0"),
        "rho", StringValue("6000.0"));  // 半径300米

        mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    
// 使用RandomWalk2dMobilityModel
       mobility.SetMobilityModel(
        "ns3::RandomWalk2dMobilityModel",
        "Bounds", RectangleValue(Rectangle(-6000, 6000, -6000, 6000)),  // 匹配6000米半径
        "Mode", StringValue("Time"),
        // "Distance", DoubleValue(50),  // 每次移动距离（米）
        "Time", StringValue("1s"),       // 每1秒改变方向
        "Speed", StringValue("ns3::UniformRandomVariable[Min=0.5|Max=1.5]")  // 移动速度（m/s）
       );

       

       // 统一安装移动模型
mobility.Install(endDevices);
}
else if (movementMode == "clustered") {
    // 聚集模式 - 仅设置模型类型
    // mobility.SetMobilityModel("ns3::WaypointMobilityModel",
    //                         "InitialPositionIsWaypoint", BooleanValue(true));
       // 使用固定位置模型
    // mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    const uint32_t numClusters = std::max(15, (int)nDevices/100);
    const double clusterRadius = 6000.0;
    const double clusterSpread = 50.0;

    // 生成簇中心
    std::vector<Vector> clusterCenters(numClusters);
    Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
    for (uint32_t c = 0; c < numClusters; ++c) {
        double angle = rand->GetValue(0, 2 * M_PI);
        double distance = rand->GetValue(0, clusterRadius);
        clusterCenters[c] = Vector(distance * cos(angle), distance * sin(angle), 0);
    }

    // 配置随机游走模型
    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
        "Mode", StringValue("Time"),
        "Time", StringValue("1s"),       // 每1秒改变方向
        "Speed", StringValue("ns3::UniformRandomVariable[Min=0.5|Max=1.5]"),
        "Bounds", RectangleValue(
            Rectangle(-clusterRadius, clusterRadius, -clusterRadius, clusterRadius)));

    // 安装模型并设置初始位置
    mobility.Install(endDevices);
    for (uint32_t i = 0; i < endDevices.GetN(); ++i) {
        uint32_t clusterId = i % numClusters;
        Vector center = clusterCenters[clusterId];
        double angle = rand->GetValue(0, 2 * M_PI);
        double r = rand->GetValue(0, clusterSpread);
        Ptr<MobilityModel> mob = endDevices.Get(i)->GetObject<MobilityModel>();
        mob->SetPosition(Vector(
            center.x + r*cos(angle),
            center.y + r*sin(angle),
            0));
    }
}


// // 如果是聚集模式，后添加路径点
// if (movementMode == "clustered") {
//     const uint32_t numClusters = std::max(15, (int)nDevices/200); // 簇的数量（可调整）
//     const double clusterRadius = 6000.0;  // 总分布半径
//     const double clusterSpread = 200.0;  //// 每个簇的分布半径（节点在簇中心附近100米内）
//     // 生成簇中心...  // 1. 为每个簇随机生成中心坐标（均匀分布在6000米半径范围内）
//     std::vector<Vector> clusterCenters(numClusters);
//     Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
//     for (uint32_t c = 0; c < numClusters; ++c) {
//         double angle = rand->GetValue(0, 2 * M_PI);  // 随机角度
//         double distance = rand->GetValue(0, clusterRadius);  // 随机半径
//         clusterCenters[c] = Vector(distance * cos(angle), distance * sin(angle), 0);
//     }

//     // mobility.Install(endDevices);

//     // 分配静态位置
//     for (uint32_t i = 0; i < endDevices.GetN(); ++i) {
//         Ptr<ConstantPositionMobilityModel> mob = 
//             endDevices.Get(i)->GetObject<ConstantPositionMobilityModel>();
//         NS_ASSERT_MSG(mob != nullptr, "Failed to get mobility model");
        
//         uint32_t clusterId = i % numClusters;
//         Vector center = clusterCenters[clusterId];
        
//         double angle = rand->GetValue(0, 2 * M_PI);
//         double r = rand->GetValue(0, clusterSpread);
//         mob->SetPosition(Vector(center.x + r*cos(angle), center.y + r*sin(angle), 0));
//     }
// }
 



  //***tests
  double avgDistance = (avgDistance/nDevices);
  //*****tests


  // Create the LoraNetDevices of the end devices
  uint8_t nwkId = 54; //网络标识符（Network Identifier）标识LoRaWAN网络的逻辑分区，用于保证不同网络的设备地址不冲突
  uint32_t nwkAddr = 1864;  // 基础网络地址（Base Network Address）地址分配的起始值，后续设备地址在此基础递增。1864 表示第一个设备的地址从1864开始分配
  Ptr<LoraDeviceAddressGenerator> addrGen =
      CreateObject<LoraDeviceAddressGenerator> (nwkId, nwkAddr);

  // Create the LoraNetDevices of the end devices
  macHelper.SetAddressGenerator (addrGen);
  phyHelper.SetDeviceType (LoraPhyHelper::ED);  //PHY层设备类型设置：LoraPhyHelper::ED 表示终端设备（End Device）的物理层
  macHelper.SetDeviceType (LorawanMacHelper::ED_A); //Class A	:	纯ALOHA，下行仅在RX1/RX2窗口
  //helper.Install (phyHelper, macHelper, endDevices);
  // *************tests
  //设备安装（带返回值）,遍历endDevices中的每个节点,为每个节点创建：LoraPhy（物理层）LorawanMac（MAC层）LorawanNetDevice（网络设备抽象）,将设备绑定到节点,返回值：NetDeviceContainer包含所有创建的LoRa网络设备
  NetDeviceContainer EndNetDevices = helper.Install (phyHelper, macHelper, endDevices);
  // *************tests


  // Now end devices are connected to the channel


  // Connect trace sources
  for (NodeContainer::Iterator j = endDevices.Begin (); j != endDevices.End (); ++j)
    {
      Ptr<Node> node = *j;
      Ptr<LoraNetDevice> loraNetDevice = node->GetDevice (0)->GetObject<LoraNetDevice> ();
      Ptr<LoraPhy> phy = loraNetDevice->GetPhy ();
  
      // tests *****************************************
      // adding confirmation message - default is unconfirmed 
      Ptr<LorawanMac> loraNetMac = node->GetDevice (0)->GetObject<LoraNetDevice> ()->GetMac ();
      Ptr<ClassAEndDeviceLorawanMac> edLorawanMac = loraNetMac->GetObject<ClassAEndDeviceLorawanMac> ();
      edLorawanMac->SetEnableCRDSA(enableCRDSA);


      Ptr<LorawanMac> lorawanMac = loraNetDevice->GetMac();
      Ptr<EndDeviceLorawanMac> edMac = DynamicCast<EndDeviceLorawanMac>(lorawanMac);
      edMac->SetEnableCRDSA(enableCRDSA);

      //edLorawanMac->SetMType (LorawanMacHeader::CONFIRMED_DATA_DOWN);
      LoraDeviceAddress edAddr = edLorawanMac->GetDeviceAddress();
        
      trialdevices[node->GetId()].id=node->GetId();
      trialdevices[node->GetId()].nwkaddr = edAddr.GetNwkAddr();
      trialdevices[node->GetId()].fail = 0;
      trialdevices[node->GetId()].success =0;


    }

  // ********************************************tests

  /*********************
   *  Create Gateways  *
   *********************/

  // Create the gateway nodes (allocate them uniformely on the disc)
  NodeContainer gateways;
  gateways.Create (nGateways);

  Ptr<ListPositionAllocator> allocator = CreateObject<ListPositionAllocator> ();
  // Make it so that nodes are at a certain height > 0
  allocator->Add (Vector (0.0, 0.0, 15.0));
  mobility.SetPositionAllocator (allocator);
  mobility.Install (gateways);


  // Create a netdevice for each gateway
  phyHelper.SetDeviceType (LoraPhyHelper::GW);
  macHelper.SetDeviceType (LorawanMacHelper::GW);
  helper.Install (phyHelper, macHelper, gateways);

  // Connect trace sources
for (uint32_t i = 0; i < gateways.GetN(); ++i) {
    Ptr<Node> node = gateways.Get(i);
        
    // 防御性编程：逐层验证
    if (node->GetNDevices() == 0) {
        NS_LOG_ERROR("网关节点 " << i << " 未安装设备");
        continue;
    }

    Ptr<NetDevice> netDevice = node->GetDevice(0);
    Ptr<LoraNetDevice> loraNetDevice = DynamicCast<LoraNetDevice>(netDevice);

    Ptr<LorawanMac> lorawanMac = loraNetDevice->GetMac();
    Ptr<GatewayLorawanMac> gwMac = DynamicCast<GatewayLorawanMac>(lorawanMac);
    if (!gwMac) {
        NS_LOG_ERROR("MAC类型转换失败（非网关MAC）");
        continue;
    }

    // 最终安全设置
    gwMac->SetEnableCRDSA(enableCRDSA);
    std::cerr << "enableCRDSA from CommandLine: " << enableCRDSA << std::endl;

    // Ptr<SimpleGatewayLoraPhy> gatewayPhy = CreateObject<SimpleGatewayLoraPhy> (enableCRDSA);
     Ptr<LoraPhy> lorawanPhy = loraNetDevice->GetPhy();
    Ptr<SimpleGatewayLoraPhy> gwPhy= DynamicCast<SimpleGatewayLoraPhy>(lorawanPhy);
    gwPhy->SetEnableCRDSA(enableCRDSA);
    }

  /**********************
   *  Handle buildings  *
   **********************/

    double xLength = 130;
    double deltaX = 32;
    double yLength = 64;
    double deltaY = 17;

  int gridWidth = 2 * radius / (xLength + deltaX);
  int gridHeight = 2 * radius / (yLength + deltaY);
  if (realisticChannelModel == false)
    {
      gridWidth = 0;
      gridHeight = 0;
    }
  Ptr<GridBuildingAllocator> gridBuildingAllocator;


  gridBuildingAllocator = CreateObject<GridBuildingAllocator> ();
  gridBuildingAllocator->SetAttribute ("GridWidth", UintegerValue (gridWidth));
  gridBuildingAllocator->SetAttribute ("LengthX", DoubleValue (xLength));
  gridBuildingAllocator->SetAttribute ("LengthY", DoubleValue (yLength));
  gridBuildingAllocator->SetAttribute ("DeltaX", DoubleValue (deltaX));
  gridBuildingAllocator->SetAttribute ("DeltaY", DoubleValue (deltaY));
  gridBuildingAllocator->SetAttribute ("Height", DoubleValue (6));
  gridBuildingAllocator->SetBuildingAttribute ("NRoomsX", UintegerValue (2));
  gridBuildingAllocator->SetBuildingAttribute ("NRoomsY", UintegerValue (4));
  gridBuildingAllocator->SetBuildingAttribute ("NFloors", UintegerValue (2));
  

  //***** tests
  //gridBuildingAllocator->SetBuildingAttribute ("ExtWallsType", UintegerValue (3));
  //***** tests
  gridBuildingAllocator->SetAttribute (
      "MinX", DoubleValue (-gridWidth * (xLength + deltaX) / 2 + deltaX / 2));
  gridBuildingAllocator->SetAttribute (
      "MinY", DoubleValue (-gridHeight * (yLength + deltaY) / 2 + deltaY / 2));
   
  BuildingContainer bContainer = gridBuildingAllocator->Create (gridWidth * gridHeight);
  //BuildingContainer bContainer = gridBuildingAllocator->Create (352);
  //BuildingContainer bContainer = gridBuildingAllocator->Create (1);
  

  BuildingsHelper::Install (endDevices);
  BuildingsHelper::Install (gateways);
  //BuildingsHelper::MakeMobilityModelConsistent ();

  // Print the buildings
  if (print)
    {
      std::ofstream myfile;
      myfile.open ("/home/zhou/tarballs/ns-3-allinone/ns-3.37/contrib/NS3-LoraRL/rladr-lorans3/automation-interface/buildings.txt");
      if (myfile.is_open())
   {
      std::cerr << "可以打开buildings.txt文件" << std::endl;
   }
      std::vector<Ptr<Building>>::const_iterator it;
      int j = 1;
      for (it = bContainer.Begin (); it != bContainer.End (); ++it, ++j)
        {
          Box boundaries = (*it)->GetBoundaries ();
          myfile << "set object " << j << " rect from " << boundaries.xMin << "," << boundaries.yMin
                 << " to " << boundaries.xMax << "," << boundaries.yMax  <<std::endl;
        }
      myfile.close ();
    }

  /**********************************************
    *  Set up the end device's spreading factor  *
   **********************************************/

  macHelper.SetSpreadingFactorsUp (endDevices, gateways, channel);

  NS_LOG_DEBUG ("Completed configuration");

  /*********************************************
   *  Install applications on the end devices  *
   *********************************************/

  Time appStopTime = Seconds (simulationTime);
  PeriodicSenderHelper appHelper = PeriodicSenderHelper ();
  appHelper.SetPeriod (Seconds (appPeriodSeconds));
  appHelper.SetPacketSize (27);

  Ptr<RandomVariableStream> rv = CreateObjectWithAttributes<UniformRandomVariable> (
      "Min", DoubleValue (0), "Max", DoubleValue (10));
  ApplicationContainer appContainer = appHelper.Install (endDevices);

  appContainer.Start (Seconds (0));
  appContainer.Stop (appStopTime);

  /**************************
   *  Create Network Server  *
   ***************************/


  Ptr<Node> networkServer = CreateObject<Node> ();

  //p2p links between server and gateways
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  p2p.SetChannelAttribute("Delay",StringValue("2ms"));
  //store network server app registration details for later
 
  P2PGwRegistration_t gwRegistration;    //存储网络服务器与每个网关之间的P2P连接信息
  for (auto gw = gateways.Begin();gw != gateways.End();++gw)  //gateways:NodeContainer类型，包含所有网关节点,gw指向当前网关节点（Ptr<Node>）
  {
      auto container = p2p.Install(networkServer,*gw);   //创建P2P连接
      //返回值：NetDeviceContainer包含两个设备：container.Get(0)：网络服务器侧的P2P设备。container.Get(1)：网关侧的P2P设备
      auto serverP2PNetDev = DynamicCast<PointToPointNetDevice>(container.Get(0));
      gwRegistration.emplace_back(serverP2PNetDev,*gw);  //将<服务器P2P设备指针，网关节点指针>对存入容器
  }


  // Create a network server for the network
  nsHelper.SetGatewaysP2P (gwRegistration);
  nsHelper.SetEndDevices (endDevices);

  //**********tests
  nsHelper.EnableAdr(true);
  //nsHelper.SetAdr (adrType);
  nsHelper.SetAdr(adrType); 
  //**********tests
  nsHelper.Install(networkServer);




  //Create a forwarder for each gateway
  forHelper.Install (gateways);


  /////////////////////////////////////
  /****************** tests adding power consumption measurement*/

  BasicEnergySourceHelper basicSourceHelper;
  LoraRadioEnergyModelHelper radioEnergyHelper;

  // configure energy source


  
  basicSourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue (10000)); // Energy in J
  basicSourceHelper.Set ("BasicEnergySupplyVoltageV", DoubleValue (3.3));

  radioEnergyHelper.Set ("StandbyCurrentA", DoubleValue (0.0014));
  radioEnergyHelper.Set ("TxCurrentA", DoubleValue (0.028));
  radioEnergyHelper.Set ("SleepCurrentA", DoubleValue (0.0000015));
  radioEnergyHelper.Set ("RxCurrentA", DoubleValue (0.0112));

  radioEnergyHelper.SetTxCurrentModel ("ns3::ConstantLoraTxCurrentModel",
                                       "TxCurrent", DoubleValue (0.028));

  // install source on EDs' nodes
  energy::EnergySourceContainer sources = basicSourceHelper.Install (endDevices);
  Names::Add ("/Names/EnergySource", sources.Get (0));

  // install device model
  energy::DeviceEnergyModelContainer deviceModels = radioEnergyHelper.Install(EndNetDevices, sources);

  ////////////////
  // Simulation //
  ////////////////

  Simulator::Stop (appStopTime + Hours (1));

  NS_LOG_INFO ("Running simulation...");
  Simulator::Run ();


  Simulator::Destroy ();

  ///////////////////////////
  // Print results to file //
  ///////////////////////////
  NS_LOG_INFO ("Computing performance metrics...");
  


// LoraPacketTracker &tracker = helper.GetPacketTracker ();

// //方法一：直接使用全局统计（若只需总数）
// std::cerr << global_power << " " << tracker.CountMacPacketsGlobally (Seconds (0), appStopTime + Hours (1)) <<std::endl;
//函数返回：发送的数据包数和至少一个网关接收的数据包数

  //new
  float indicator = float(global_received)/float(global_sent);
  float power_indicator = float(global_power/global_received);
  float throughput= (float(global_received)*60)/simulationTime;

  std::cout<<"Result "<<global_received<<" "<< global_sent <<" "<<global_power<< " "<< power_indicator  <<" "<< throughput<< " "<<avgDistance<< " " << freq_on_air[0] << " "<< freq_on_air[1] <<" "<<freq_on_air[2] <<std::endl;




  if (rlagent == 1)
  {
	if (training)
	{
		SaveQ(Q);
	}
  }

//

  ///***tests
  //PrintNodes(nDevices);
  ///***tests
  // 程序退出前清理（可选，系统会自动清理）
 // PythonInterpreter::Finalize();
  return 0;
}
