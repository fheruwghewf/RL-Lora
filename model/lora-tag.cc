/*
 * Copyright (c) 2017 University of Padova
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Davide Magrin <magrinda@dei.unipd.it>
 */

#include "lora-tag.h"

#include "ns3/tag.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"

namespace ns3
{
namespace lorawan
{

NS_OBJECT_ENSURE_REGISTERED(LoraTag);

TypeId
LoraTag::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::LoraTag").SetParent<Tag>()
        .SetGroupName("lorawan")
        .AddConstructor<LoraTag>()
        .AddAttribute("Snr", "Signal-to-noise ratio (dB)",
                      DoubleValue(0.0),
                      MakeDoubleAccessor(&LoraTag::GetSnr, &LoraTag::SetSnr),
                      MakeDoubleChecker<double>());
    return tid;
}

TypeId
LoraTag::GetInstanceTypeId() const
{
    return GetTypeId();
}

LoraTag::LoraTag(uint8_t sf, uint8_t destroyedBy)
    : m_sf(sf),
      m_destroyedBy(destroyedBy),
      m_receivePower(0),
      m_dataRate(0),
      m_frequency(0),

      m_replicaId(0),
      m_slotIndex(0),
      m_devAddr(0),
      m_fcnt(0),
      m_snr(0.0)
{
}

LoraTag::~LoraTag()
{
}

// --- 副本ID操作 ---
void
LoraTag::SetReplicaId (uint8_t id)
{
    m_replicaId = id;
}

uint8_t
LoraTag::GetReplicaId (void) const
{
    return m_replicaId;
}

// --- 时隙索引操作 ---
void
LoraTag::SetSlotIndex (uint32_t index)
{
    m_slotIndex = index;
}

uint32_t
LoraTag::GetSlotIndex (void) const
{
    return m_slotIndex;
}

 // --- 设备地址操作 ---
void 
LoraTag::SetDevAddr(uint32_t addr)
{
    m_devAddr = addr;
}
uint32_t
LoraTag::GetDevAddr (void) const
{
    return m_devAddr;
}

// --- 帧计数器操作 ---
void 
LoraTag::SetFCnt(uint16_t fcnt)
{
    m_fcnt = fcnt;
}
uint16_t
LoraTag::GetFCnt (void) const
{
    return m_fcnt;
}

void LoraTag::SetIsReplica(bool isReplica)
{
    m_isReplica = isReplica;
}

bool LoraTag::GetIsReplica() const
{
    return m_isReplica;
}

uint32_t
LoraTag::GetSerializedSize() const
{
    // Each datum about a spreading factor is 1 byte + receivePower (the size of a double) +
    // frequency (the size of a double)
    //return 3 + 2 * sizeof(double);

    return sizeof(uint8_t)   // m_sf
    + sizeof(uint8_t)   // m_destroyedBy
    + sizeof(double)    // m_receivePower
    + sizeof(uint8_t)   // m_dataRate
    + sizeof(double)    // m_frequency
    + sizeof(uint8_t)   // m_replicaId
    + sizeof(uint32_t)  // m_slotIndex
    + sizeof(uint32_t)  // m_devAddr
    + sizeof(uint16_t) // m_fcnt
    + sizeof(double)  // m_snr
    +sizeof(bool)    // m_isReplica
    +sizeof(uint32_t)
    +sizeof(uint64_t);

}

void
LoraTag::Serialize(TagBuffer i) const
{
    i.WriteU8(m_sf);
    i.WriteU8(m_destroyedBy);
    i.WriteDouble(m_receivePower);
    i.WriteU8(m_dataRate);
    i.WriteDouble(m_frequency);

    i.WriteU8 (m_replicaId);
    i.WriteU32 (m_slotIndex);
    i.WriteU32 (m_devAddr);
    i.WriteU16 (m_fcnt);
    i.WriteDouble (m_snr);

    i.WriteU8(m_isReplica ? 1 : 0); // 将 bool 转换为 uint8_t 存储
    i.WriteU32 (m_currentSlot); // 序列化当前时隙
    i.WriteU64 (m_packetId);
}

void
LoraTag::Deserialize(TagBuffer i)
{
    m_sf = i.ReadU8();
    m_destroyedBy = i.ReadU8();
    m_receivePower = i.ReadDouble();
    m_dataRate = i.ReadU8();
    m_frequency = i.ReadDouble();

    m_replicaId = i.ReadU8 ();
    m_slotIndex = i.ReadU32 ();
    m_devAddr = i.ReadU32 ();
    m_fcnt = i.ReadU16 ();
    m_snr = i.ReadDouble ();

    m_isReplica = (i.ReadU8() == 1); // 读取 uint8_t 并转换为 bool
    m_currentSlot = i.ReadU32 ();    // 初始化当前时隙
    m_packetId = i.ReadU64 ();
}

void
LoraTag::Print(std::ostream& os) const
{
    os << m_sf << " " << m_destroyedBy << " " << m_receivePower << " " << m_dataRate;
    os << "ReplicaId=" << (int)m_replicaId << ", SlotIndex=" << m_slotIndex;
}

uint8_t
LoraTag::GetSpreadingFactor() const
{
    return m_sf;
}

uint8_t
LoraTag::GetDestroyedBy() const
{
    return m_destroyedBy;
}

double
LoraTag::GetReceivePower() const
{
    return m_receivePower;
}

void
LoraTag::SetDestroyedBy(uint8_t sf)
{
    m_destroyedBy = sf;
}

void
LoraTag::SetSpreadingFactor(uint8_t sf)
{
    m_sf = sf;
}

void
LoraTag::SetReceivePower(double receivePower)
{
    m_receivePower = receivePower;
}

void
LoraTag::SetFrequency(double frequency)
{
    m_frequency = frequency;
}

double
LoraTag::GetFrequency() const
{
    return m_frequency;
}

uint8_t
LoraTag::GetDataRate() const
{
    return m_dataRate;
}

void
LoraTag::SetDataRate(uint8_t dataRate)
{
    m_dataRate = dataRate;
}

void 
LoraTag::SetSnr(double snr) { m_snr = snr; }
double LoraTag::GetSnr() const { return m_snr; }

void 
LoraTag::SetCurrentSlot(uint32_t currentSlot) { m_currentSlot = currentSlot; }
uint32_t LoraTag::GetCurrentSlot() const { return m_currentSlot; }

void 
LoraTag::SetPacketId(uint64_t packetId) { m_packetId = packetId; }
uint64_t LoraTag::GetPacketId() const{ return m_packetId; }  // 设置全局唯一ID（由发送端生成）

} // namespace lorawan
} // namespace ns3
