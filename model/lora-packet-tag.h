#ifndef LORA_PACKET_TAG_H
#define LORA_PACKET_TAG_H

#include "ns3/tag.h"
#include "ns3/nstime.h"

namespace ns3 {
namespace lorawan
{

class LoraPacketTag : public Tag
{
public:
    static TypeId GetTypeId (void);
    TypeId GetInstanceTypeId (void) const override;

    // 构造函数
    LoraPacketTag();
    ~LoraPacketTag() override;

    // ----------- 字段操作方法 -----------
    // SNR (dB)
    void SetSnr(double snr);
    double GetSnr(void) const;

    // 接收功率 (dBm)
    void SetRxPower(double power);
    double GetRxPower(void) const;

    // 设备地址 (32-bit DevAddr)
    void SetDevAddr(uint32_t addr);
    uint32_t GetDevAddr(void) const;

    // 帧计数器 (16-bit FCnt)
    void SetFCnt(uint16_t fcnt);
    uint16_t GetFCnt(void) const;

    // 副本ID (CRDSA)
    void SetReplicaId(uint8_t id);
    uint8_t GetReplicaId(void) const;

    // 时隙索引 (CRDSA)
    void SetSlotIndex(uint32_t index);
    uint32_t GetSlotIndex(void) const;

    // ----------- NS-3 Tag 必须实现的虚函数 -----------
    void Serialize(TagBuffer i) const override;
    void Deserialize(TagBuffer i) override;
    uint32_t GetSerializedSize(void) const override;
    void Print(std::ostream &os) const override;

private:
    double m_snr;         // 信噪比 (dB)
    double m_rxPower;      // 接收功率 (dBm)
    uint32_t m_devAddr;    // 设备地址 (DevAddr)
    uint16_t m_fcnt;       // 帧计数器 (FCnt)
    uint8_t m_replicaId;   // 副本ID (0,1,2...)
    uint32_t m_slotIndex;  // 时隙索引
};
} // namespace lorawan
} // namespace ns3

#endif // LORA_PACKET_TAG_H