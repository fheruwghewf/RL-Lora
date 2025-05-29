/*
 * Copyright (c) 2017 University of Padova
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Davide Magrin <magrinda@dei.unipd.it>
 */

#include "gateway-lorawan-mac.h"

#include "lora-frame-header.h"
#include "lora-net-device.h"
#include "lorawan-mac-header.h"
#include "ns3/idxnodes.h"
#include "ns3/lora-tag.h"
#include "ns3/lora-packet-tracker.h"
#include "lora-device-address.h"

#include "ns3/log.h"
#include <tuple> // 必需头文件

namespace ns3
{
namespace lorawan
{

NS_LOG_COMPONENT_DEFINE("GatewayLorawanMac");

NS_OBJECT_ENSURE_REGISTERED(GatewayLorawanMac);

TypeId
GatewayLorawanMac::GetTypeId()
{
    static TypeId tid = TypeId("ns3::GatewayLorawanMac")
                            .SetParent<LorawanMac>()
                            .AddConstructor<GatewayLorawanMac>()
                            .SetGroupName("lorawan");
    return tid;
}

GatewayLorawanMac::GatewayLorawanMac()
{
    NS_LOG_FUNCTION(this);
}

GatewayLorawanMac::~GatewayLorawanMac()
{
    NS_LOG_FUNCTION(this);
}

void
GatewayLorawanMac::Send(Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(this << packet);

    // Get data rate to send this packet with
    LoraTag tag;
    packet->RemovePacketTag(tag);
    uint8_t dataRate = tag.GetDataRate();
    double frequency = tag.GetFrequency();
    NS_LOG_DEBUG("DR: " << unsigned(dataRate));
    NS_LOG_DEBUG("SF: " << unsigned(GetSfFromDataRate(dataRate)));
    NS_LOG_DEBUG("BW: " << GetBandwidthFromDataRate(dataRate));
    NS_LOG_DEBUG("Freq: " << frequency << " MHz");
    packet->AddPacketTag(tag);

    // Make sure we can transmit this packet
    if (m_channelHelper->GetWaitingTime(CreateObject<LogicalLoraChannel>(frequency)) > Time(0))
    {
        // We cannot send now!
        NS_LOG_WARN("Trying to send a packet but Duty Cycle won't allow it. Aborting.");
        return;
    }

    LoraTxParameters params;
    params.sf = GetSfFromDataRate(dataRate);
    params.headerDisabled = false;
    params.codingRate = 1;
    params.bandwidthHz = GetBandwidthFromDataRate(dataRate);
    params.nPreamble = 8;
    params.crcEnabled = true;
    params.lowDataRateOptimizationEnabled = LoraPhy::GetTSym(params) > MilliSeconds(16);

    // Get the duration
    Time duration = LoraPhy::GetOnAirTime(packet, params);

    NS_LOG_DEBUG("Duration: " << duration.GetSeconds());

    // Find the channel with the desired frequency
    double sendingPower =
        m_channelHelper->GetTxPowerForChannel(CreateObject<LogicalLoraChannel>(frequency));

    // Add the event to the channelHelper to keep track of duty cycle
    m_channelHelper->AddEvent(duration, CreateObject<LogicalLoraChannel>(frequency));

    // Send the packet to the PHY layer to send it on the channel
    m_phy->Send(packet, params, frequency, sendingPower);

    m_sentNewPacket(packet);
}

bool
GatewayLorawanMac::IsTransmitting()
{
    return m_phy->IsTransmitting();
}

// void
// GatewayLorawanMac::Receive(Ptr<const Packet> packet)
// {
//     NS_LOG_FUNCTION(this << packet);

//     // Make a copy of the packet to work on
//     Ptr<Packet> packetCopy = packet->Copy();

//     // Only forward the packet if it's uplink
//     LorawanMacHeader macHdr;
//     packetCopy->PeekHeader(macHdr);

//     if (macHdr.IsUplink())
//     {
//         DynamicCast<LoraNetDevice>(m_device)->Receive(packetCopy);

//         NS_LOG_DEBUG("Received packet: " << packet);

//         m_receivedPacket(packet);
        
//     }
//     else
//     {
//         NS_LOG_DEBUG("Not forwarding downlink message to NetDevice");
//     }
// }

// 设置方法实现
void GatewayLorawanMac::SetEnableCRDSA(bool enable) {
    m_enableCRDSA = enable;
    NS_LOG_FUNCTION(this << enable);
}

// 获取方法实现
bool GatewayLorawanMac::GetEnableCRDSA() const {
    return m_enableCRDSA;
}


void
GatewayLorawanMac::Receive(Ptr<const Packet> packet)
{
    //std::cerr<<"GatewayLorawanMac:Receive"<<std::endl;
    NS_LOG_FUNCTION(this << packet);
    // 1. 拷贝数据包并解析 MAC 头
    Ptr<Packet> packetCopy = packet->Copy();
    LorawanMacHeader macHdr;
    packetCopy->PeekHeader(macHdr);

    // 2. 仅处理上行数据
    if (!macHdr.IsUplink()) {
        NS_LOG_DEBUG("Ignoring downlink message");
        return;
    }

    // 3. 检查是否为 CRDSA 副本（通过 LoraTag）
    LoraTag crdsaTag;
    bool isCRDSAPacket = packetCopy->PeekPacketTag(crdsaTag); // 移除Tag以获取副本信息
    // std::cerr<<"isCRDSAPacket= "<<isCRDSAPacket<<" GetEnableCRDSA()= "<<GetEnableCRDSA()<<std::endl;
    if (GetEnableCRDSA()) {
        // --- CRDSA 逻辑 ---
        // std::cerr<< " --- --- CRDSA receive --- ---"<<std::endl;
        uint8_t replicaId = crdsaTag.GetReplicaId();
        uint32_t slotIndex = crdsaTag.GetSlotIndex();
        double rxPower = crdsaTag.GetReceivePower();

        // std::cerr<<"Received CRDSA replica "<< (int)replicaId<< " in slot " << slotIndex<<" rxPower= "<<rxPower<<std::endl;
        // 4. 按时隙分组存储数据包（用于后续SIC）
        m_receivedPackets[slotIndex].push_back(packetCopy);

        // 5. 尝试冲突解决（SIC）
        TrySIC();
    } else {
        // --- 原始ALOHA逻辑 ---
        // std::cerr<<"--- --- ALOHA --- ---"<<std::endl;
        DynamicCast<LoraNetDevice>(m_device)->Receive(packetCopy);

        m_receivedPacket(packet);
    }
}

void
GatewayLorawanMac::TrySIC()
{
      // 待删除的包
    //std::cerr<< "--- --- TrySIC --- ---"<<std::endl;
     //std::set<std::tuple<LoraDeviceAddress, uint32_t, uint64_t>> m_successfullyDecodedPackets;
    // 1. 遍历所有时隙
    for (auto& slot : m_receivedPackets)
     {
        
        auto& packetsInSlot = slot.second;
         // 2. 按 RSSI 降序排序当前时隙内的数据包
        std::sort(packetsInSlot.begin(), packetsInSlot.end(),
            [this](const Ptr<Packet>& a, const Ptr<Packet>& b) {
                LoraTag tagA, tagB;
                a->PeekPacketTag(tagA);
                b->PeekPacketTag(tagB);
                return tagA.GetReceivePower()> tagB.GetReceivePower(); // 按 RSSI 降序
            });
            
        
        // 2. 尝试解码每个包
        for (auto pktIter = packetsInSlot.begin(); pktIter != packetsInSlot.end(); ) {
           
            Ptr<Packet> pkt = *pktIter;
            //Ptr<Packet> pktNext = *(pktIter+1);
            //LoraTag tagNext;
            //pktNext->PeekPacketTag(tagNext);
           
            
            LoraTag tag;
            if (!pkt->PeekPacketTag(tag)) continue; // 确保Tag存在

            Ptr<Packet> pktCopy = pkt->Copy(); // 创建包的副本

                        // --- 新增检查：跳过已统计的包 ---
            LorawanMacHeader macHdr;
            LoraFrameHeader frameHdr;
            Ptr<Packet> pktCopyForCheck = pkt->Copy();
            pktCopy->RemoveHeader(macHdr);
            pktCopy->RemoveHeader(frameHdr);
            auto pktKey = std::make_tuple(frameHdr.GetAddress(), tag.GetCurrentSlot(), tag.GetPacketId());
            


            // 3. 检查是否可解码（基于SNR或其他条件）
            if (CanDecode(pkt)) {
                
                // std::cerr<<"Can Decode"<<std::endl;
                // DynamicCast<LoraNetDevice>(m_device)->Receive(pkt);
                // m_receivedPacket(pkt);
                    if (m_successfullyDecodedPackets.find(pktKey) == m_successfullyDecodedPackets.end()) {
                        m_successfullyDecodedPackets.insert(pktKey);
                    
                        // 更新设备统计
                        trialdevices[FindNode(frameHdr.GetAddress().GetNwkAddr())].success+=1;
                        trialdevices[FindNode(frameHdr.GetAddress().GetNwkAddr())].previous_attempt = true;
                        global_received++; // 全局计数器+1
                            
                        std::cerr<<"global_received= "<<global_received<<std::endl;
                    }
                    

                // 5. 从其他时隙中消除该包的副本信号
                //std::cerr<<" --- --- cancel interference --- ---"<<std::endl;
                CancelInterference(pkt);
                
                // // 6. 从当前时隙移除已解码的包
                pktIter = packetsInSlot.erase(pktIter);
                
                } 
            else {++pktIter; } // 继续下一个包     
        }
    }

    //std::cerr<<"--- --- finish TrySIC --- ---"<<std::endl;
                               // 安全删除所有已解码的包
                for (auto& pkt_to_remove : packetsToRemove) {
                     for (auto& slot : m_receivedPackets) {
                        auto& packetsInSlot = slot.second;
                        // 使用 erase-remove idiom 来移除特定的包
                        packetsInSlot.erase(std::remove(packetsInSlot.begin(), packetsInSlot.end(), pkt_to_remove), packetsInSlot.end());
                    }
                }

}
//判断接收后能否解码
bool
GatewayLorawanMac::CanDecode(Ptr<Packet> packet)
{
    LoraTag tag;
    packet->PeekPacketTag(tag);
    uint8_t sf = tag.GetSpreadingFactor();
    // --- 判断接收后能否解码。实现中应根据SNR、RSSI等物理层指标判断
    double snr = GetSnrFromPhy(packet); // 假设从PHY层获取SNR
    return (snr > m_decodingThreshold[sf-7]); // 阈值需根据实验设置,
}


double
GatewayLorawanMac::GetSnrFromPhy(Ptr<Packet> packet)
{
    // --- 通过物理层获取SNR ---
    // 方法2：通过 PacketTag 存储 SNR（推荐）
    LoraTag tag;
    //Ptr<Packet> packetCopy = packet->Copy();
    if (packet->PeekPacketTag(tag)) {
        return tag.GetSnr(); // 从 Tag 中读取 SNR
        //std::cerr<<"GetSnrFromPhy:snr= "<<tag.GetSnr()<<std::endl;
    }
    else {
        std::cerr<<"Error: Failed to get SNR from packet tag"<<std::endl;
    
    return -999.0; // 默认值（表示无效）
}
}

void
GatewayLorawanMac::CancelInterference(Ptr<Packet> decodedPkt)
{
    //std::cerr<<"CancelInterference()"<<std::endl;
    // --- 消除其余时隙中的干扰信号 ---
    // 1. 获取解码包的副本标记
    
    LoraTag decodedTag;
    decodedPkt->PeekPacketTag(decodedTag);

    // 2. 遍历所有时隙，消除同源副本
    for (auto& slot : m_receivedPackets) {

        // 跳过当前时隙
        if (slot.first == decodedTag.GetSlotIndex()) {
            // std::cerr<<"slot.first ="<<slot.first<<" ,decodedTag.GetSlotIndex()= "<<decodedTag.GetSlotIndex()<<std::endl;
            continue;
        }

        auto& packetsInSlot = slot.second;
        for (auto pktIter = packetsInSlot.begin(); pktIter != packetsInSlot.end(); ) {
            Ptr<Packet> pkt = *pktIter;

             if (pkt == decodedPkt) {
                // packetsToRemove.push_back(pkt);
                // ++pktIter;  
                pktIter = packetsInSlot.erase(pktIter);  // 更新迭代器
                //continue;  
                }

             if (pkt != decodedPkt)
              {   // 跳过已解码的包
                //std::cerr<<"pkt != decodedPkt"<<std::endl;
                    LoraTag tag;
                    pkt->PeekPacketTag(tag);

                    // 3. 如果是同一数据包的其他副本，则消除干扰
                    if (IsSameOriginalPacket(decodedTag, tag)) 
                    {
                        SubtractSignal(pkt, decodedPkt); // 信号功率相减
                        // 检查信号是否已完全抵消
                        LoraTag updatedTag;
                        pkt->PeekPacketTag(updatedTag);
                        if(updatedTag.GetReceivePower()<-150.0){
                        pktIter = packetsInSlot.erase(pktIter);
                        }

                    }
                }  
        } 
    }
}



bool
GatewayLorawanMac::IsSameOriginalPacket(const LoraTag& tag1, const LoraTag& tag2)
{
     //std::cerr<<"IsSameOriginalPacket()"<<std::endl;
    // --- 判断两个包是否为同一原始数据包 ---
    // 从 Tag 中提取设备地址和帧计数器（需扩展 LorawanTag）

    // std::cerr<<"tag1.GetDevAddr()= "<<tag1.GetDevAddr()<<" ,tag2.GetDevAddr()="<<tag2.GetDevAddr()<<std::endl;
    // std::cerr<<"tag1.GetFCnt()= "<<tag1.GetFCnt()<<" ,tag2.GetFCnt()="<<tag2.GetFCnt()<<std::endl;
    // std::cerr<<"tag1.GetCurrentSlot()= "<<tag1.GetCurrentSlot()<<" ,tag2.GetCurrentSlot()="<<tag2.GetCurrentSlot()<<std::endl;
    // std::cerr<<"tag1.GetPacketId()= "<<tag1.GetPacketId()<<" ,tag2.GetPacketId()="<<tag2.GetPacketId()<<std::endl;

    return (tag1.GetDevAddr() == tag2.GetDevAddr()) && 
            (tag1.GetFCnt() == tag2.GetFCnt());
    //return (tag1.GetDevAddr() == tag2.GetDevAddr()) && 
        //    (tag1.GetFCnt() == tag2.GetFCnt())&&(tag1.GetCurrentSlot() == tag2.GetCurrentSlot())&&(tag1.GetPacketId() == tag2.GetPacketId())&&(tag1.GetPacketId() == tag2.GetPacketId());
}

void
GatewayLorawanMac::SubtractSignal(Ptr<Packet> interferedPkt, Ptr<Packet> decodedPkt)
{
    // --- 信号功率相减 ---
    // 1. 获取两包的接收功率
   // std::cerr<<"SubtractSignal()"<<std::endl;

    //std::cerr << "Interfered packet tags: ";
    // interferedPkt->PrintPacketTags(std::cerr);
    // //std::cerr << "\nDecoded packet tags: ";
    // decodedPkt->PrintPacketTags(std::cerr);
  

    LoraTag interferedTag, decodedTag;
    interferedPkt->PeekPacketTag(interferedTag);
    decodedPkt->PeekPacketTag(decodedTag);

    double interferedPower = interferedTag.GetReceivePower(); // 干扰包功率（dBm）
    double decodedPower = decodedTag.GetReceivePower();       // 已解码包功率（dBm）
    //std::cerr<<"interferedPower= "<<interferedPower<<" decodedPower= "<<decodedPower<<std::endl;
    // 2. 计算抵消后的功率（线性空间计算）
    double interferedLinear = pow(10, interferedPower / 10); // dBm -> mW
    double decodedLinear = pow(10, decodedPower / 10);
    double newPowerLinear = interferedLinear - decodedLinear;  //interferedLinear叠加信号，相减是从从混合信号中减去已解码信号成分，剩下的信号可能被解码
    //double newPowerDbm = 10 * log10(newPowerLinear);         // mW -> dBm
    double newPowerDbm;
    if (newPowerLinear <= 0) {
        newPowerDbm = -999.0; // 表示信号已完全消除
    } else {
        newPowerDbm = 10 * log10(newPowerLinear);
    }
    //std::cerr<<"interferedLinear= "<<interferedLinear<<" ,decodedLinear= "<<decodedLinear<<" ,newPowerLinear= "<<newPowerLinear<<" ,newPowerDbm= "<<newPowerDbm<<std::endl;
    // 3. 更新干扰包的功率标签
    interferedTag.SetReceivePower(newPowerDbm);
    interferedPkt->ReplacePacketTag(interferedTag);

    // NS_LOG_DEBUG("SIC: Subtracted signal from " << decodedPower << " dBm, "
    //             << "new power: " << newPowerDbm << " dBm");
    // std::cerr<<"SIC: Subtracted signal from " << decodedPower << " dBm, "
    //             << "new power: " << newPowerDbm << " dBm"<<std::endl;
}





void
GatewayLorawanMac::FailedReception(Ptr<const Packet> packet)
{
    NS_LOG_FUNCTION(this << packet);
}

void
GatewayLorawanMac::TxFinished(Ptr<const Packet> packet)
{
    NS_LOG_FUNCTION_NOARGS();
}

Time
GatewayLorawanMac::GetWaitingTime(double frequency)
{
    NS_LOG_FUNCTION_NOARGS();

    return m_channelHelper->GetWaitingTime(CreateObject<LogicalLoraChannel>(frequency));
}
} // namespace lorawan
} // namespace ns3
