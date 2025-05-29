/*
 * Copyright (c) 2017 University of Padova
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Davide Magrin <magrinda@dei.unipd.it>
 */

#ifndef LORA_TAG_H
#define LORA_TAG_H

#include "ns3/tag.h"

namespace ns3
{
namespace lorawan
{

/**
 * \ingroup lorawan
 *
 * Tag used to save various data about a packet, like its Spreading Factor and data about
 * interference.
 */
class LoraTag : public Tag
{
  public:
    /**
     *  Register this type.
     *  \return The object TypeId.
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;

    /**
     * Create a LoraTag with a given spreading factor and collision.
     *
     * \param sf The Spreading Factor.
     * \param destroyedBy The spreading factor this tag's packet was destroyed by.
     */
    LoraTag(uint8_t sf = 0, uint8_t destroyedBy = 0);

    ~LoraTag() override; //!< Destructor

    void Serialize(TagBuffer i) const override;
    void Deserialize(TagBuffer i) override;
    uint32_t GetSerializedSize() const override;
    void Print(std::ostream& os) const override;

    void SetReplicaId(uint8_t id);   //设置数据包副本的 ID
    uint8_t GetReplicaId() const;
    void SetSlotIndex(uint32_t index);  //设置数据包所属的时隙编号（如 0、1、2表示第几个时隙）
    uint32_t GetSlotIndex() const;
    void SetDevAddr(uint32_t addr);   //设置设备地址
    uint32_t GetDevAddr() const;
    void SetFCnt(uint16_t fcnt);   //设置帧计数器
    uint16_t GetFCnt() const;

    // 新增 CRDSA 副本标记方法
    void SetIsReplica(bool isReplica);
    bool GetIsReplica() const;


    /**
     * Read which Spreading Factor this packet was transmitted with.
     *
     * \return This tag's packet's spreading factor.
     */
    uint8_t GetSpreadingFactor() const;

    /**
     * Read which Spreading Factor this packet was destroyed by.
     *
     * \return The spreading factor this packet was destroyed by.
     */
    uint8_t GetDestroyedBy() const;

    /**
     * Read the power this packet arrived with.
     *
     * \return This tag's packet received power.
     */
    double GetReceivePower() const;

    /**
     * Set which Spreading Factor this packet was transmitted with.
     *
     * \param sf The Spreading Factor.
     */
    void SetSpreadingFactor(uint8_t sf);

    /**
     * Set which Spreading Factor this packet was destroyed by.
     *
     * \param sf The Spreading Factor.
     */
    void SetDestroyedBy(uint8_t sf);

    /**
     * Set the power this packet was received with.
     *
     * \param receivePower The power, in dBm.
     */
    void SetReceivePower(double receivePower);

    /**
     * Set the frequency of the packet.
     *
     * This value works in two ways:
     * - It is used by the gateway to signal to the network server the frequency of the uplink
     * packet
     * - It is used by the network server to signal to the gateway the frequency of a downlink
     * packet.
     *
     * \param frequency The frequency value [MHz].
     */
    void SetFrequency(double frequency);

    /**
     * Get the frequency of the packet.
     *
     * This value works in two ways:
     * - It is used by the gateway to signal to the network server the frequency of the uplink
     * packet
     * - It is used by the network server to signal to the gateway the frequency of a downlink
     * packet.
     *
     * \return The frequency value [MHz].
     */
    double GetFrequency() const;

    /**
     * Get the data rate for this packet.
     *
     * \return The data rate that needs to be employed for this packet.
     */
    uint8_t GetDataRate() const;

    /**
     * Set the data rate for this packet.
     *
     * \param dataRate The data rate.
     */
    void SetDataRate(uint8_t dataRate);

    void SetSnr(double snr);
    double GetSnr(void) const;

    void SetCurrentSlot(uint32_t currentSlot);
    uint32_t GetCurrentSlot(void) const ;// 获取当前时隙编号

    void SetPacketId(uint64_t packetId) ;
    uint64_t GetPacketId() const; 

  private:
    uint8_t m_sf;          //!< The Spreading Factor used by the packet.
    uint8_t m_destroyedBy; //!< The Spreading Factor that destroyed the packet.
    double m_receivePower; //!< The reception power of this packet.
    uint8_t m_dataRate;    //!< The data rate that needs to be used to send this packet.
    double m_frequency;    //!< The frequency of this packet

    uint8_t m_replicaId;   // 副本ID（ 1, ...）
    uint32_t m_slotIndex;  // 目标时隙编号
    uint32_t m_devAddr; // 设备地址
    uint16_t m_fcnt;    // 帧计数器
    double m_snr;         // 信噪比 (dB)

    bool m_isReplica; // 新增成员变量，标记是否为 CRDSA 副本
    //uint64_t GetOriginalPacketId() const;  // 新增成员变量，全局唯一ID（由发送端生成）
    uint32_t m_currentSlot; // 当前时隙编号
    uint64_t m_packetId;
};
} // namespace lorawan
} // namespace ns3
#endif
