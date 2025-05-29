#include "lora-packet-tag.h"
#include "ns3/log.h"
#include "ns3/tag.h"
#include "ns3/double.h"

namespace ns3 {
namespace lorawan
{

NS_LOG_COMPONENT_DEFINE("LoraPacketTag");

NS_OBJECT_ENSURE_REGISTERED(LoraPacketTag);

TypeId
LoraPacketTag::GetTypeId (void)
{
    static TypeId tid = TypeId("ns3::LoraPacketTag")
        .SetParent<Tag>()
        .SetGroupName("lorawan")
        .AddConstructor<LoraPacketTag>()
        .AddAttribute("Snr", "Signal-to-noise ratio (dB)",
                      DoubleValue(0.0),
                      MakeDoubleAccessor(&LoraPacketTag::GetSnr, &LoraPacketTag::SetSnr),
                      MakeDoubleChecker<double>())
        .AddAttribute("RxPower", "Received power (dBm)",
                      DoubleValue(0.0),
                      MakeDoubleAccessor(&LoraPacketTag::GetRxPower, &LoraPacketTag::SetRxPower),
                      MakeDoubleChecker<double>());
    return tid;
}

TypeId
LoraPacketTag::GetInstanceTypeId (void) const
{
    return GetTypeId();
}

LoraPacketTag::LoraPacketTag()
    : m_snr(0.0),
      m_rxPower(0.0),
      m_devAddr(0),
      m_fcnt(0),
      m_replicaId(0),
      m_slotIndex(0)
{
}
LoraPacketTag::~LoraPacketTag()
{
}

// ----------- 字段操作方法实现 -----------
void LoraPacketTag::SetSnr(double snr) { m_snr = snr; }
double LoraPacketTag::GetSnr() const { return m_snr; }

void LoraPacketTag::SetRxPower(double power) { m_rxPower = power; }
double LoraPacketTag::GetRxPower() const { return m_rxPower; }

void LoraPacketTag::SetDevAddr(uint32_t addr) { m_devAddr = addr; }
uint32_t LoraPacketTag::GetDevAddr() const { return m_devAddr; }

void LoraPacketTag::SetFCnt(uint16_t fcnt) { m_fcnt = fcnt; }
uint16_t LoraPacketTag::GetFCnt() const { return m_fcnt; }

void LoraPacketTag::SetReplicaId(uint8_t id) { m_replicaId = id; }
uint8_t LoraPacketTag::GetReplicaId() const { return m_replicaId; }

void LoraPacketTag::SetSlotIndex(uint32_t index) { m_slotIndex = index; }
uint32_t LoraPacketTag::GetSlotIndex() const { return m_slotIndex; }

// ----------- 序列化/反序列化 -----------
void
LoraPacketTag::Serialize(TagBuffer i) const
{
    // 写入顺序需与读取顺序一致
    i.WriteDouble(m_snr);
    i.WriteDouble(m_rxPower);
    i.WriteU32(m_devAddr);
    i.WriteU16(m_fcnt);
    i.WriteU8(m_replicaId);
    i.WriteU32(m_slotIndex);
}

void
LoraPacketTag::Deserialize(TagBuffer i)
{
    m_snr = i.ReadDouble();
    m_rxPower = i.ReadDouble();
    m_devAddr = i.ReadU32();
    m_fcnt = i.ReadU16();
    m_replicaId = i.ReadU8();
    m_slotIndex = i.ReadU32();
}

uint32_t
LoraPacketTag::GetSerializedSize() const
{
    // 计算总字节数: 2*double(8) + U32(4) + U16(2) + U8(1) + U32(4) = 8*2 + 4 + 2 + 1 + 4 = 27 bytes
    return sizeof(double)*2 + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint8_t) + sizeof(uint32_t);
}

void
LoraPacketTag::Print(std::ostream &os) const
{
    os << "LoraPacketTag("
       << "SNR=" << m_snr << " dB, "
       << "RxPower=" << m_rxPower << " dBm, "
       << "DevAddr=0x" << std::hex << m_devAddr << std::dec << ", "
       << "FCnt=" << m_fcnt << ", "
       << "ReplicaId=" << (int)m_replicaId << ", "
       << "SlotIndex=" << m_slotIndex << ")";
}
} // namespace lorawan
} // namespace ns3