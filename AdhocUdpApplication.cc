/*
 * AdhocUdpApplication.cc
 *
 *  Created on: 2025年5月23日
 *      Author: Zhang Zhan
 */
#include "AdhocUdpApplication.h"

#include <ostream>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cmath>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/stats-module.h"
#include "ns3/node.h"
#include "ns3/ptr.h"

using namespace ns3;


NS_LOG_COMPONENT_DEFINE("wifi-adhoc-app");


//------------------------------------------------------
//-- 发包应用实现
//------------------------------------------------------

// 获取类型ID
TypeId AppSender::GetTypeId(void) {
	static TypeId tid =
    // 设置父类为Application
			TypeId("AppSender").SetParent<Application>().AddConstructor<
					AppSender>().AddAttribute("PacketSize",	"The size of packets transmitted.", UintegerValue(96),
					MakeUintegerAccessor(&AppSender::m_pktSize),
					MakeUintegerChecker<uint32_t>(1)).AddAttribute("Destination", "Target host address.",
					Ipv4AddressValue("255.255.255.255"),
					MakeIpv4AddressAccessor(&AppSender::m_destAddr),
					MakeIpv4AddressChecker()).AddAttribute("Port","Destination app port.", UintegerValue(666),
					MakeUintegerAccessor(&AppSender::m_destPort),
					MakeUintegerChecker<uint32_t>()).AddAttribute("Interval",
					"Delay between transmissions.", UintegerValue(1),
					MakeUintegerAccessor(&AppSender::m_interval),
					MakeUintegerChecker<uint32_t>());
	return tid;
}

// 代码含义：
AppSender::AppSender() {
    m_sendCounter = 0;
    m_nodeId = 0;
    m_networkSize = 0;
    m_neighborList = new std::vector<Ipv4Address>();
    m_periodicInterval = 0.1; 
}

// 析构函数
AppSender::~AppSender() {}

void AppSender::SetNodeId(uint32_t id) {
    m_nodeId = id;
}

void AppSender::SetNetworkSize(uint32_t size) {
    m_networkSize = size;
    // 初始化KeyMatrix
    m_keyMatrix.InitializeMatrix(size, m_nodeId);
}

// 设置发包计数器
void AppSender::SetSendCounter(Ptr<CounterCalculator<> > calc) {
	m_sendCounter = 0;
}   


void AppSender::DoDispose(void) {
	m_Socket = 0;
	Application::DoDispose();
}

void AppSender::PeriodicBroadcast() {
    // 检查是否已经收齐所有密钥贡献
    if (m_keyMatrix.IsFull1()) {
        NS_LOG_INFO("所有节点已收齐所有密钥贡献，KeyMatrix全为1");
        return;
    }

    // 初始化转发字符串，初始为未收到的贡献为0，已收到的贡献为1
    std::string forwardingContributions = std::string(m_networkSize, '0');
    
    // 遍历所有节点，如果已经收到某节点的密钥贡献，则该位置为1，通过AppReceiver的m_keyMatrix判断
    for (uint32_t i = 0; i < m_networkSize; i++) {
        // 获取AppReceiver的m_keyMatrix
        Ptr<AppReceiver> receiver = DynamicCast<AppReceiver>(GetNode()->GetApplication(1));
        if (receiver->GetKeyMatrix().HasKeyContribution(m_nodeId, i)) {
            forwardingContributions[i] = '1';
        }
    }
    
    // 构建数据包，内容为节点ID+转发字符串+本地的KeyMatrix
    std::ostringstream packetContent;
    packetContent << m_nodeId << " " << forwardingContributions << " " << m_keyMatrix.MatrixToString();
    std::string content = packetContent.str();
    
    SendPacket(m_destAddr, content);
    
    // 周期后广播
    m_periodicEvent = Simulator::Schedule(Seconds(m_periodicInterval), &AppSender::PeriodicBroadcast, this);
}

// 启动应用
void AppSender::StartApplication() {
	
    // 创建UDP类型socket，并绑定
	TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
	m_Socket = Socket::CreateSocket(GetNode(), tid);       // 用于发送数据
    m_Socket->Bind();

	// 设置目的地址和端口
	InetSocketAddress dataRemote = InetSocketAddress(m_destAddr, m_destPort);
    // InetSocketAddress neighborRemote = InetSocketAddress(m_destAddr, m_neighborPort);
	
    // 设置广播
	m_Socket->SetAllowBroadcast(true);
	
    // 初始化链接
	m_Socket->Connect(dataRemote);

    // 初始化转发字符串,初始化为全0
    std::string forwardingContributions = std::string(m_networkSize, '0');
    // 第ID位为1
    forwardingContributions[m_nodeId] = '1';    
    std::ostringstream packetContent;
    packetContent << m_nodeId << " " << forwardingContributions << " " << m_keyMatrix.MatrixToString();
    std::string content = packetContent.str();

    // 发送数据包
    m_sendEvent = Simulator::Schedule(Seconds(0.005), &AppSender::SendPacket, this, m_destAddr, content);
    NS_LOG_INFO("节点" << m_nodeId << "开始发送首次数据包");
    
    // 1.1秒后开始周期性广播
    m_periodicEvent = Simulator::Schedule(Seconds(1.1), &AppSender::PeriodicBroadcast, this);
}

void AppSender::StopApplication() {
	Simulator::Cancel(m_sendEvent);
    Simulator::Cancel(m_periodicEvent);
}


void AppSender::SendPacket(Ipv4Address neighborAddress, std::string packetContent) {
    Simulator::Schedule(MilliSeconds(5), &AppSender::DoSendPacket, this, neighborAddress, packetContent);
}

void AppSender::DoSendPacket(Ipv4Address neighborAddress, std::string packetContent) {
    std::ostringstream msg;
    msg << packetContent;

    int numContributions = 0;
    // 找到转发字符串
    std::string forwardingContributions = packetContent.substr(packetContent.find(" ") + 1, packetContent.find(" ", packetContent.find(" ") + 1) - packetContent.find(" ") - 1);
    // 如果转发字符串为全1，则设置numContributions=1
    if (forwardingContributions == std::string(m_networkSize, '1')) {
        numContributions = 1;
    } else {
        numContributions = std::count(forwardingContributions.begin(), forwardingContributions.end(), '1');
    }
    // 计算额外字节数
    int additionalBytes = 8*(160 + 64 * (std::ceil(log2(numContributions)) + 1));
    
    // 添加填充字节
    std::string padding(additionalBytes, '0');
    msg << padding;

    std::string content = msg.str();
    Ptr<Packet> packet = Create<Packet>((uint8_t*) content.c_str(), content.size());
    InetSocketAddress remote = InetSocketAddress(neighborAddress, m_destPort);
    m_Socket->Connect(remote);
    m_Socket->Send(packet);
    m_sendCounter++;
}



//------------------------------------------------------
//-- 收包应用实现
//------------------------------------------------------
TypeId AppReceiver::GetTypeId(void) {
	static TypeId tid =
			TypeId("AppReceiver").SetParent<Application>().AddConstructor<
					AppReceiver>()
					.AddAttribute("Port", "Listening port.", UintegerValue(666),
							MakeUintegerAccessor(&AppReceiver::m_port),
							MakeUintegerChecker<uint32_t>())
					.AddAttribute("Destination", "Target host address.",
							Ipv4AddressValue("255.255.255.255"),
							MakeIpv4AddressAccessor(&AppReceiver::m_destAddr),
							MakeIpv4AddressChecker());
	return tid;
}

AppReceiver::AppReceiver() {
    m_packetBuffer = new std::map<std::string, int>;
    m_keyAgreementDelay = 0;
    m_receivedCounter = 0;   
    m_isCompleted = false;
    m_nodeId = 0;
    m_networkSize = 0;
    m_neighborList = new std::vector<Ipv4Address>();
    // KeyMatrix在SetNetworkSize时初始化
}

AppReceiver::~AppReceiver() {
    delete m_packetBuffer;
}

// 设置节点数量
void AppReceiver::SetNumNodes(uint32_t num) {
	m_numNodes = num;
}

// 设置节点ID
void AppReceiver::SetNodeId(uint32_t id) {
	m_nodeId = id;
}

// 设置网络大小
void AppReceiver::SetNetworkSize(uint32_t size) {
    m_networkSize = size;
    // 初始化KeyMatrix
    m_keyMatrix.InitializeMatrix(size, m_nodeId);
}

// 设置收包计数器
void AppReceiver::SetReceiveCounter(Ptr<CounterCalculator<> > calc){
    m_receivedCounter = 0;
}


// 获取收包计数器
uint32_t AppReceiver::GetReceivedPackets() const {
    return m_receivedCounter;
}

// 获取密钥协商完成时间
double AppReceiver::GetKeyAgreementDelay() const {
    return m_keyAgreementDelay;
}

// 判断是否完成
bool AppReceiver::IsCompleted() const {
    return m_isCompleted;
}


// 用于释放资源
void AppReceiver::DoDispose(void) {
	m_socket = 0;
    // m_neighborSocket = 0;    
	// chain up
	Application::DoDispose();
}

// 更新邻居列表，只保留最新的N/2个邻居,N为节点数量
void AppReceiver::UpdateNeighborList(Ipv4Address neighborAddress) {
    // 如果该邻居在邻居列表中，则不添加
    if (std::find(m_neighborList->begin(), m_neighborList->end(), neighborAddress) != m_neighborList->end()) {
        // 将该邻居在邻居列表中的位置，移动到列表的末尾
        m_neighborList->erase(std::find(m_neighborList->begin(), m_neighborList->end(), neighborAddress));
        m_neighborList->push_back(neighborAddress);
    } else {
        // 否则添加邻居，只保留最新的N/2个邻居
        m_neighborList->push_back(neighborAddress);
        if (m_neighborList->size() > m_networkSize / 2) {
            m_neighborList->erase(m_neighborList->begin());
        }
    }
}

// 启动应用
void AppReceiver::StartApplication() {
	TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
	m_socket = Socket::CreateSocket(GetNode(), tid);
    
    // 获取节点IP地址
    Ipv4Address address = GetNode()->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
    // 绑定IP地址和端口
    InetSocketAddress local = InetSocketAddress(address, m_port);
	m_socket->Bind(local);
    NS_LOG_INFO("节点" << m_nodeId << "开始监听: " << address << ":" << m_port);
    m_socket->SetRecvCallback(MakeCallback(&AppReceiver::Receive, this));
}

void AppReceiver::StopApplication() {
	if (m_socket != 0) {
		m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket> >());
	}
}

void AppReceiver::Receive(Ptr<Socket> socket) {
    Ptr<Packet> packet;
    Address from;

    while ((packet = socket->RecvFrom(from))) {
        m_receivedCounter++;
        // 获取发送方地址
        Ipv4Address senderAddr = InetSocketAddress::ConvertFrom(from).GetIpv4();
        // 将发送方地址添加到邻居列表
        UpdateNeighborList(senderAddr);
               
        // 从packet中提取数据
        uint8_t *buffer = new uint8_t[packet->GetSize()];
        packet->CopyData(buffer, packet->GetSize());
        std::string msg = std::string((char*)buffer, packet->GetSize());
        delete[] buffer;

        // 从msg中提取发送节点ID - 使用string流替代std::stoi
        std::istringstream senderIdStream(msg.substr(0, msg.find(" ")));
        uint32_t senderId;
        senderIdStream >> senderId;

        std::string ReceivedKeyContributions = msg.substr(msg.find(" ") + 1, msg.find(" ", msg.find(" ") + 1) - msg.find(" ") - 1);


        std::string ReceivedKeyMatrixString = msg.substr(msg.find(" ", msg.find(" ") + 1) + 1, m_networkSize * m_networkSize);

        // 创建接收矩阵
        KeyMatrix ReceivedMatrix = m_keyMatrix.StringToMatrix(ReceivedKeyMatrixString);

        for (uint32_t i = 0; i < ReceivedKeyContributions.size(); i++) {
            // 检查本地是否拥有该密钥贡献
            if (m_keyMatrix.HasKeyContribution(m_nodeId, i)) {
                continue;
            } else {
                // 接受该密钥贡献
                m_keyMatrix.ReceiveKeyContribution(i);
                // 记录日志
                NS_LOG_INFO("节点" << m_nodeId << "未拥有密钥贡献" << i << "，接受该密钥贡献");
            }   
        }   

        m_keyMatrix.MergeMatrix(ReceivedMatrix);	


        // 如果自己已经收齐所有密钥贡献，则设置m_isCompleted为true
        if(m_keyMatrix.SelfIsFull1()) {
            // NS_LOG_INFO("节点" << m_nodeId << "已收齐所有密钥贡献");
            m_isCompleted = true;
        }

       

        // 遍历所有邻居
        for (uint32_t i = 0; i < m_neighborList->size(); i++) {     
            // 获取邻居的IP地址
            Ipv4Address neighborAddr = m_neighborList->at(i);
            // 获取邻居ID，邻居ID就是邻居的IP地址的最后一位，255.255.255.1 -> 1
            uint8_t ipBytes[4];
            neighborAddr.Serialize(ipBytes);
            uint32_t neighborId = ipBytes[3]-1;

            // 获取该邻居节点的转发字符串
            std::string forwardingContributions = m_keyMatrix.GetForwardingContributions(neighborId);

            // 如果字符串不为全0，则转发
            if (forwardingContributions != std::string(m_networkSize, '0')) {               
                // 构建数据包内容
                std::ostringstream msg;
                // 数据包内容    
                msg << m_nodeId << " " << forwardingContributions << " " << m_keyMatrix.MatrixToString();
                std::string content = msg.str();
                
                // 通过发送者应用转发
                Ptr<AppSender> sender = DynamicCast<AppSender>(GetNode()->GetApplication(0));
                sender->SendPacket(neighborAddr, content);
            }
        }
    }
}












