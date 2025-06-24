/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

// Authors:
// Zhan Zhang
#include "ns3/applications-module.h"
#include "ns3/config-store-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/olsr-helper.h"
#include "ns3/point-to-point-module.h"
#include "ns3/propagation-module.h"
#include "ns3/trace-helper.h"
#include "ns3/wifi-module.h"
#include "ns3/stats-module.h"
#include <string>
#include <vector>
#include <sys/stat.h>
#include <fstream>  // 添加文件流头文件
#include <iomanip>  // 添加iomanip头文件
#include <sys/types.h>
#include <unistd.h>
#include <sstream>

#include "AdhocUdpApplication.h"

using namespace ns3;

// Log部件键名
NS_LOG_COMPONENT_DEFINE("wifi-adhoc-UAV-experiment");

// --------- 实验具体数据 -------------
std::string phyMode("ErpOfdmRate12Mbps");  
std::string propagationLossModel("ns3::FriisPropagationLossModel");
std::string propagationDelayModel("ns3::ConstantSpeedPropagationDelayModel");
std::string mobilitySpeed("ns3::UniformRandomVariable[Min=10|Max=50]");
std::string mobilityModel("ns3::GaussMarkovMobilityModel");
double areaLength = 500;   // 活动区域长度 (m)
double areaWidth = 500;    // 活动区域宽度 (m)
double areaHeight = 100;   // 活动区域高度 (m) - 无人机飞行高度限制
// 节点个数
uint32_t numNodes = 5;
// 仿真时间
uint32_t simuTime = 60;
double periodicInterval = 0.1;  
double CompletionTime = 0;

// ------------ End -----------------

// ---------- 实验数据记录标签 ----------
// 实验内容
std::string experiment;
// 实验参数
std::string strategy;
// 实验节点数
std::string input;
// 实验序号
std::string runId;
// ------------- End -----------------

// 检查是否所有节点都完成了密钥收集
bool CheckAllNodesCompleted(const NodeContainer& nodes) {
	for (uint32_t i = 0; i < nodes.GetN(); i++) {
		Ptr<AppReceiver> receiver = DynamicCast<AppReceiver>(nodes.Get(i)->GetApplication(1));
		if (!receiver->IsCompleted()) {
			return false;
		}
	}
	return true;
}

void CheckCompletionAndStop(const NodeContainer& nodes) {
	if (CheckAllNodesCompleted(nodes)) {
		NS_LOG_INFO("所有节点都已收齐密钥贡献，提前结束仿真");
		CompletionTime = Simulator::Now().GetSeconds()-1;	
		Simulator::Stop();
	} else {
		// 获取当前模拟时间
		double currentTime = Simulator::Now().GetSeconds();
		if (currentTime < 2.0) {
			Simulator::Schedule(Seconds(0.01), &CheckCompletionAndStop, nodes);
		} else {
			Simulator::Schedule(Seconds(0.1), &CheckCompletionAndStop, nodes);
		}
	}
}



void SetupLinkQuality (YansWifiPhyHelper &wifiPhy, std::string quality)
{
  /* 硬件常量 */
  wifiPhy.Set ("RxNoiseFigure", DoubleValue (7.0));   // 无人机常见 NF≈7 dB
  wifiPhy.Set ("RxGain",        DoubleValue (2.0));   // +2 dBi 小型天线

  /* ========== HIGH ==========  (LoS 0-150 m) */
  if (quality == "high")
    {
      wifiPhy.Set ("TxPowerStart", DoubleValue (20.0));   // 20 dBm
      wifiPhy.Set ("TxPowerEnd",   DoubleValue (20.0));
      wifiPhy.Set ("CcaMode1Threshold", DoubleValue (-82.0));   // RxSens-84 +2 dB

      YansWifiChannelHelper ch;
      ch.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
      ch.AddPropagationLoss ("ns3::FriisPropagationLossModel",
                             "Frequency", DoubleValue (2.4e9));
      ch.AddPropagationLoss ("ns3::RandomPropagationLossModel",
                             "Variable", StringValue ("ns3::NormalRandomVariable[Mean=0|Variance=4]"));   // σ=2 dB
      ch.AddPropagationLoss ("ns3::NakagamiPropagationLossModel",
                             "m0", DoubleValue (2.5));   // 轻微快衰落
      ch.AddPropagationLoss ("ns3::RangePropagationLossModel",
                             "MaxRange", DoubleValue (1200.0));   // 1.2 km
      wifiPhy.SetChannel (ch.Create ());
    }

  /* ========== MEDIUM ==========  (轻遮挡 150-600 m) */
  else if (quality == "medium")
    {
      wifiPhy.Set ("TxPowerStart", DoubleValue (18.0));
      wifiPhy.Set ("TxPowerEnd",   DoubleValue (18.0));
      wifiPhy.Set ("CcaMode1Threshold", DoubleValue (-81.0));

      YansWifiChannelHelper ch;
      ch.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
      ch.AddPropagationLoss ("ns3::LogDistancePropagationLossModel",
                             "Exponent",          DoubleValue (2.5),
                             "ReferenceDistance", DoubleValue (1.0),
                             "ReferenceLoss",     DoubleValue (40.05));
      ch.AddPropagationLoss ("ns3::RandomPropagationLossModel",
                             "Variable", StringValue ("ns3::NormalRandomVariable[Mean=0|Variance=25]")); // σ=5 dB
      ch.AddPropagationLoss ("ns3::NakagamiPropagationLossModel",
                             "m0", DoubleValue (1.5),
                             "m1", DoubleValue (1.2),
                             "m2", DoubleValue (1.0),
                             "Distance1", DoubleValue (150.0),
                             "Distance2", DoubleValue (400.0));
      ch.AddPropagationLoss ("ns3::RangePropagationLossModel",
                             "MaxRange", DoubleValue (700.0));
      wifiPhy.SetChannel (ch.Create ());
    }

  /* ========== LOW ==========  (远距 / 频繁遮挡) */
  else if (quality == "low")
    {
      wifiPhy.Set ("TxPowerStart", DoubleValue (14.0));
      wifiPhy.Set ("TxPowerEnd",   DoubleValue (14.0));
      wifiPhy.Set ("CcaMode1Threshold", DoubleValue (-78.0));

      YansWifiChannelHelper ch;
      ch.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
      ch.AddPropagationLoss ("ns3::LogDistancePropagationLossModel",
                             "Exponent",          DoubleValue (3.2),
                             "ReferenceDistance", DoubleValue (1.0),
                             "ReferenceLoss",     DoubleValue (46.7));
      ch.AddPropagationLoss ("ns3::RandomPropagationLossModel",
                             "Variable", StringValue ("ns3::NormalRandomVariable[Mean=0|Variance=49]")); // σ=7 dB
      ch.AddPropagationLoss ("ns3::NakagamiPropagationLossModel",
                             "m0", DoubleValue (1.0),
                             "m1", DoubleValue (0.8),
                             "m2", DoubleValue (0.7),
                             "Distance1", DoubleValue (200.0),
                             "Distance2", DoubleValue (500.0));
      ch.AddPropagationLoss ("ns3::RangePropagationLossModel",
                             "MaxRange", DoubleValue (600.0));
      wifiPhy.SetChannel (ch.Create ());
    }

  /* ========== VERY POOR ==========  (NLoS / 密集遮挡) */
  else   /* very_poor */
    {
      wifiPhy.Set ("TxPowerStart", DoubleValue (10.0));
      wifiPhy.Set ("TxPowerEnd",   DoubleValue (10.0));
      wifiPhy.Set ("CcaMode1Threshold", DoubleValue (-75.0));

      YansWifiChannelHelper ch;
      ch.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
	  // 设置传播损失
      ch.AddPropagationLoss ("ns3::LogDistancePropagationLossModel",
                             "Exponent",          DoubleValue (3.9),
                             "ReferenceDistance", DoubleValue (1.0),
                             "ReferenceLoss",     DoubleValue (46.7));
      // 设置随机传播损失
      ch.AddPropagationLoss ("ns3::RandomPropagationLossModel",
                             "Variable", StringValue ("ns3::NormalRandomVariable[Mean=0|Variance=81]")); // σ=9 dB
      ch.AddPropagationLoss ("ns3::NakagamiPropagationLossModel",
                             "m0", DoubleValue (1.0),
                             "m1", DoubleValue (0.7),
                             "m2", DoubleValue (0.6),
                             "Distance1", DoubleValue (150.0),
                             "Distance2", DoubleValue (400.0));
      ch.AddPropagationLoss ("ns3::RangePropagationLossModel",
                             "MaxRange", DoubleValue (400.0));
      wifiPhy.SetChannel (ch.Create ());
    }
}


void startSimulation(const std::string& linkQuality) {
    
	NodeContainer nodes;
	nodes.Create(numNodes);

	// --------------------------------------------------------------
	// ---------- 设置物理层、数据链路层 ----------
	// --------------------------------------------------------------
	// 设置wifi标准
	WifiHelper wifi;
	// wifi.SetStandard(WIFI_PHY_STANDARD_80211b);
	wifi.SetStandard(WIFI_PHY_STANDARD_80211g);  // 使用802.11g标准

	// 设置物理层-PHY层
	YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
	// wireshark抓包需开启
	// wifiPhy.SetPcapDataLinkType(YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

	// 设置链路质量
	SetupLinkQuality(wifiPhy, linkQuality);

	// 设置数据链路层-MAC层
	NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default();
	wifi.SetRemoteStationManager("ns3::MinstrelWifiManager");		//链路恶化时会自动退速
	// 设置模式
	wifiMac.SetType("ns3::AdhocWifiMac");

	// 安装至设备
	NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, nodes);
	// -------------- End ----------------


	// ------------------------------------------------------------
	// ---------- 设置网络层、传输层协议 -------------
	// ------------------------------------------------------------
	// 安装传输层
	InternetStackHelper internet;
	internet.Install(nodes);
	// 安装网络层，分配IP
	Ipv4AddressHelper ipv4;
	ipv4.SetBase("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer ipv4Container = ipv4.Assign(devices);
	// ------------- End -----------------


	// ------------------------------------------------------------
	// ---------- 设置应用层 -------------
	// ------------------------------------------------------------
	// 设置收包计数器
	Ptr<CounterCalculator<> > totalSentPackets = CreateObject<CounterCalculator<> >();
	// 设置发包计数器
	Ptr<CounterCalculator<> > totalRecvPackets = CreateObject<CounterCalculator<> >();
	totalSentPackets->SetKey("发送方");
	totalSentPackets->SetContext("发包总数");
	totalRecvPackets->SetKey("接收方");
	totalRecvPackets->SetContext("收包总数");

	// 设置应用层信息
	for (uint32_t i = 0; i < numNodes; i++) {
		Ptr<Node> nodeToInstallApp = nodes.Get(i);
		Ptr<AppSender> sender = CreateObject<AppSender>();
		Ptr<AppReceiver> receiver = CreateObject<AppReceiver>();

		sender->SetSendCounter(totalSentPackets);
		receiver->SetReceiveCounter(totalRecvPackets);
		// 为接收方设置网络节点个数，用于判断是否完成通信
		receiver->SetNumNodes(numNodes);
		// 设置节点ID用于标识
		receiver->SetNodeId(i);
        sender->SetNodeId(i);
        
        // 初始化KeyMatrix
        receiver->SetNetworkSize(numNodes);
        sender->SetNetworkSize(numNodes);

		nodeToInstallApp->AddApplication(sender);
		nodeToInstallApp->AddApplication(receiver);

		receiver->SetStartTime(Seconds(0));
		// 注意，这里发送方必须把时间岔开，否则不能发送消息
		sender->SetStartTime(Seconds(1 + 0.00001 * i));

		receiver->SetStopTime(Seconds(simuTime));
		sender->SetStopTime(Seconds(simuTime));
	}
	// ------------- End -----------------


	// ------------------------------------------------------------
	// ---------- 设置移动模型 -------------
	// ------------------------------------------------------------
	MobilityHelper mobility;

	// RandomBoxPositionAllocator实现在一个立方体内任意选取位置
	Ptr<RandomBoxPositionAllocator> randPosLocator =
			CreateObject<RandomBoxPositionAllocator>();
	// 随机设置节点的位置
	std::ostringstream playGround;
	playGround << "ns3::UniformRandomVariable[Min=0.0|" << "Max=" << areaLength << "]";
	randPosLocator->SetAttribute("X", StringValue(playGround.str()));
	playGround.str("");
	playGround << "ns3::UniformRandomVariable[Min=0.0|" << "Max=" << areaWidth << "]";
	randPosLocator->SetAttribute("Y", StringValue(playGround.str()));
	playGround.str("");
	playGround << "ns3::UniformRandomVariable[Min=0.0|" << "Max=" << areaHeight << "]";
	randPosLocator->SetAttribute("Z", StringValue(playGround.str()));
	mobility.SetPositionAllocator(randPosLocator);

	// 设置移动模型
	mobility.SetMobilityModel(mobilityModel,									// 移动模型
			"Bounds", BoxValue(												// 边界
					Box(0, areaLength, 0, areaWidth, 0, areaHeight)),
			"Alpha", DoubleValue (0.85),										// 模型可调参数
			"TimeStep", TimeValue(Seconds(1)),								// 运动方向和速度改变间隔
			"MeanVelocity", StringValue(mobilitySpeed),						// 运动平均速度
			"MeanDirection", StringValue(										// 运动平均方向
					"ns3::UniformRandomVariable[Min=0|Max=6.283185307]"),
			"MeanPitch", StringValue(											// 平均??
					"ns3::UniformRandomVariable[Min=0.05|Max=0.05]"),
			"NormalVelocity", StringValue(										// 计算下一个随机速度值的随机变量
					"ns3::NormalRandomVariable[Mean=0.0|Variance=0.0|Bound=0.0]"),
			"NormalDirection", StringValue(									// 计算下一个随机方向的随机变量
					"ns3::NormalRandomVariable[Mean=0.0|Variance=0.2|Bound=0.4]"),
			"NormalPitch", StringValue(										// 计算下一个随机??的随机变量
					"ns3::NormalRandomVariable[Mean=0.0|Variance=0.02|Bound=0.04]")
			);

	for (uint32_t i = 0; i < numNodes; ++i) {
		mobility.Install(nodes.Get(i));
	}
	// -------------- End ----------------


	// ------------------------------------------------------------
	// ---------- 实验数据收集 -------------
	// ------------------------------------------------------------
	DataCollector dataCollector;
	dataCollector.DescribeRun(experiment, strategy, input, runId);
	dataCollector.AddMetadata("author", "z");
	dataCollector.AddDataCalculator(totalSentPackets);
	dataCollector.AddDataCalculator(totalRecvPackets);
	// -------------- End ----------------


	// ------------------------------------------------------------
	// ---------- 开始仿真 -------------
	// ------------------------------------------------------------
    
	// 设置仿真结束时间
	Simulator::Stop(Seconds(simuTime));
	Simulator::Schedule(Seconds(0.001), &CheckCompletionAndStop, nodes);

	// 仿真开始
	Simulator::Run();
 

	// 统计数据包数量
	uint32_t totalSent = 0;
	uint32_t totalReceived = 0;
	uint32_t successfulNodes = 0; // 成功接收所有数据包的节点数

	std::vector<uint32_t> sentPackets;
	std::vector<uint32_t> receivedPackets;
	sentPackets.reserve(numNodes);
	receivedPackets.reserve(numNodes);

	for (uint32_t i = 0; i < numNodes; i++) {
		// 获取接收方和发送方
		Ptr<AppReceiver> receiver = DynamicCast<AppReceiver>(nodes.Get(i)->GetApplication(1));
		Ptr<AppSender> sender = DynamicCast<AppSender>(nodes.Get(i)->GetApplication(0));
		// 获取接收方和发送方数据包数量
		uint32_t packetsReceived = receiver->GetReceivedPackets();
		NS_LOG_INFO("节点" << i << "接收到的数据包数量: " << packetsReceived);
		uint32_t packetsSent = sender->GetSentPackets();
		NS_LOG_INFO("节点" << i << "发送的数据包数量: " << packetsSent);
		
		// 保存每个节点的包数据（用于数据库记录）
		sentPackets.push_back(packetsSent);
		receivedPackets.push_back(packetsReceived);
		
		// 获取KeyMatrix
		const KeyMatrix& keyMatrix = receiver->GetKeyMatrix();
		// 检查节点i是否收到所有密钥贡献
		bool allContributionsReceived = true;
		for (uint32_t j = 0; j < numNodes; j++) {
			if (!keyMatrix.HasKeyContribution(i, j)) {
				allContributionsReceived = false;
				break;
			}	
		}
		if (allContributionsReceived) {
			successfulNodes++;
			NS_LOG_INFO("节点" << i << "成功收集所有密钥贡献");
			
		}
		totalSent += packetsSent;
		totalReceived += packetsReceived;
		
	}
		// 输出每个节点接收到的数据包信息
	NS_LOG_INFO("------------节点数据包统计------------");
	NS_LOG_INFO("+--------+-------------+-------------+----------+");
	NS_LOG_INFO("| 节点ID | 发送数据包  | 接收数据包  | 是否完成 |");
	NS_LOG_INFO("+--------+-------------+-------------+----------+");
	NS_LOG_INFO("+--------+-------------+-------------+----------+");

	for (uint32_t i = 0; i < numNodes; i++) {
		Ptr<AppReceiver> receiver = DynamicCast<AppReceiver>(nodes.Get(i)->GetApplication(1));
		Ptr<AppSender> sender = DynamicCast<AppSender>(nodes.Get(i)->GetApplication(0));
		NS_LOG_INFO("| " << std::setw(6) << i << " | " << std::setw(11) << sender->GetSentPackets() << " | " << std::setw(11) << receiver->GetReceivedPackets() << " | " << (receiver->IsCompleted() ? "是" : "否") << " |");
	}

	// 输出汇总信息
	NS_LOG_INFO("汇总统计:");
	NS_LOG_INFO("  总发送数据包: " << totalSent);
	NS_LOG_INFO("  总接收数据包: " << totalReceived);
		double avgSent = (double)totalSent / numNodes;
	double avgReceived = (double)totalReceived / numNodes;
	double overheadRatio = (totalSent > 0) ? (double)totalReceived / totalSent : 0;
	
	NS_LOG_INFO("分析指标:");
	NS_LOG_INFO("  平均每节点发送数据包: " << std::fixed << std::setprecision(2) << avgSent);
	NS_LOG_INFO("  平均每节点接收数据包: " << std::fixed << std::setprecision(2) << avgReceived);
	NS_LOG_INFO("  通信开销比(接收/发送): " << std::fixed << std::setprecision(2) << overheadRatio);
	
	// 计算成功率
	double successRate = (double)successfulNodes / numNodes * 100;
	NS_LOG_INFO("  成功接收所有数据包的节点比例: " << successfulNodes << "/" << numNodes 
				<< " (" << successRate << "%)");
	NS_LOG_INFO("----------------------------------------");
    
	// 密钥协商完成的时延
	double keyAgreementDelay = CompletionTime;
	NS_LOG_INFO("密钥协商完成时延: " << keyAgreementDelay << " 秒");
    
	// ------------------------------------------------------------
	// ---------- 输出结果到本地data.db文件 -------------
	// ------------------------------------------------------------
	Ptr<DataOutputInterface> output = 0;
	output = CreateObject<SqliteDataOutput>();
	// 将收集到的数据写入文件
	if (output != 0)
		output->Output(dataCollector);

	const char* cacheDir = "/home/z/ns-allinone-3.25/ns-3.25/scratch/REGKA-Ours/results_cache";
	struct stat st;
	if (stat(cacheDir, &st) != 0) {
		mkdir(cacheDir, 0777);
	}
	std::ostringstream cacheFileName;
	cacheFileName << cacheDir << "/" << areaLength << "*" << areaWidth << "*" << areaHeight << "_" << numNodes << "_" << linkQuality << "_" << runId << ".csv";
	std::ofstream out(cacheFileName.str(), std::ios::app);

	// 生成时间戳
	std::time_t now = std::time(nullptr);
	char buf[32];
	std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

	// 写入结果
	out << buf << "," << areaLength << "," << areaWidth << "," << areaHeight << "," << numNodes << "," << linkQuality << "," << runId << "," << keyAgreementDelay << "," << totalSent << "," << totalReceived << "," << overheadRatio << "," << successRate << std::endl;
	out.close();

	// NS_LOG_INFO("-----------------仿真结束-------------------");
	Simulator::Destroy();
}

int main(int argc, char *argv[]) {
	CommandLine cmd;
	cmd.AddValue("numNodes", "节点个数", numNodes);
	cmd.AddValue("areaLength", "活动区域长度 (m)", areaLength);
	cmd.AddValue("areaWidth", "活动区域宽度 (m)", areaWidth);
	cmd.AddValue("areaHeight", "活动区域高度 (m)", areaHeight);
	cmd.AddValue("run", "运行标识", runId);
	std::string linkQuality = "medium";  // 默认中等链路质量
	cmd.AddValue("linkQuality", "Link quality (high/medium/low/very_poor)", linkQuality);
	cmd.Parse(argc, argv);
	
	// 显式设置日志级别
	LogComponentEnable("wifi-adhoc-UAV-experiment", LOG_LEVEL_INFO);
	LogComponentEnable("wifi-adhoc-app", LOG_LEVEL_INFO);

	const char* logDir = "/home/z/ns-allinone-3.25/ns-3.25/scratch/REGKA-Ours/Log";
	struct stat st;
	if (stat(logDir, &st) != 0) {
		mkdir(logDir, 0777);
		NS_LOG_INFO("创建日志目录: " << logDir);
	}
	
	// 生成日志文件名，使用ostringstream替代to_string
	std::ostringstream logFileNameStream;
	logFileNameStream << logDir << "/simulation_nodes" << numNodes 
		<< "_area" << (int)areaLength << "*" << (int)areaWidth << "*" << (int)areaHeight
		<< "_linkQuality" << linkQuality
		<< "_run" << runId << ".txt";
	std::string logFileName = logFileNameStream.str();
	
	// 重定向日志输出到文件
	std::ofstream logFile(logFileName.c_str());
	if (logFile.is_open()) {
		chmod(logFileName.c_str(), 0666);
		std::streambuf* originalBuffer = std::clog.rdbuf();
		std::clog.rdbuf(logFile.rdbuf());
		std::clog.setf(std::ios::unitbuf);
		
		// 添加明确的日志开始标记
		NS_LOG_INFO("=====================================");
		NS_LOG_INFO("实验开始: 节点数=" << numNodes << ", 活动区域长度=" << areaLength << ", 活动区域宽度=" << areaWidth << ", 活动区域高度=" << areaHeight << ", 运行标识=" << runId);
		NS_LOG_INFO("=====================================");
		
		// 标注实验节点数目
		input = numNodes;
		// 标注实验信息
		std::ostringstream ss;
		ss << "numNodes:" << numNodes << ";areaLength:" << areaLength << ";areaWidth:" << areaWidth << ";areaHeight:" << areaHeight;
		experiment = ss.str();
		ss.str("");
		strategy = "单轮通信";
		
		// 运行仿真
		startSimulation(linkQuality);
		
		// 确保所有日志都写入文件
		std::clog.flush();
		
		NS_LOG_INFO("=====================================");
		NS_LOG_INFO("实验结束");
		NS_LOG_INFO("=====================================");
		
		std::clog.rdbuf(originalBuffer);
		logFile.close();
		
		std::cout << "日志已保存到: " << logFileName << std::endl;
	} else {
		NS_LOG_ERROR("无法创建日志文件: " << logFileName);
	}
    // 本次模拟完整运行，输出成功到终端
    std::cout << areaLength << "*" << areaWidth << "*" << areaHeight << "_" << numNodes << "_" << linkQuality << "_" << runId << " 成功" << std::endl;
    return 0;
}
