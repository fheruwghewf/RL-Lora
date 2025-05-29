/*
 * Copyright (c) 2017 University of Padova
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Davide Magrin <magrinda@dei.unipd.it>
 */

#ifndef GATEWAY_LORAWAN_MAC_H
#define GATEWAY_LORAWAN_MAC_H

#include "lora-tag.h"
#include "lorawan-mac.h"
#include "ns3/net-device.h"
#include "lora-device-address.h"
// #include "lora-packet-tag.h"
#include <tuple> // 必需头文件

namespace ns3
{
namespace lorawan
{

/**
 * \ingroup lorawan
 *
 * Class representing the MAC layer of a LoRaWAN gateway.
 */
class GatewayLorawanMac : public LorawanMac
{
  public:
    /**
     *  Register this type.
     *  \return The object TypeId.
     */
    static TypeId GetTypeId();

    GatewayLorawanMac();           //!< Default constructor
    ~GatewayLorawanMac() override; //!< Destructor

    // Implementation of the LorawanMac interface
    void Send(Ptr<Packet> packet) override;

    /**
     * Check whether the underlying PHY layer of the gateway is currently transmitting.
     *
     * \return True if it is transmitting, false otherwise.
     */
    bool IsTransmitting();

    // Implementation of the LorawanMac interface
    void Receive(Ptr<const Packet> packet) override;

    // Implementation of the LorawanMac interface
    void FailedReception(Ptr<const Packet> packet) override;

    // Implementation of the LorawanMac interface
    void TxFinished(Ptr<const Packet> packet) override;

    /**
     * Return the next time at which we will be able to transmit on the specified frequency.
     *
     * \param frequency The frequency value [MHz].
     * \return The next transmission time.
     */
    Time GetWaitingTime(double frequency);

    void TrySIC();
    bool CanDecode(Ptr<Packet> packet);
    void CancelInterference(Ptr<Packet> decodedPkt);
    double GetSnrFromPhy(Ptr<Packet> packet);
    bool IsSameOriginalPacket(const LoraTag& tag1, const LoraTag& tag2);
    void SubtractSignal(Ptr<Packet> interferedPkt, Ptr<Packet> decodedPkt);
        // 新增CRDSA控制方法
    void SetEnableCRDSA(bool enable);
    bool GetEnableCRDSA() const;

  private:
      // CRDSA 相关状态
      std::map<uint32_t, std::vector<Ptr<Packet>>> m_receivedPackets; // 按时隙分组的包
      //double m_decodingThreshold= -20; // 解码SNR阈值（如10.0 dB）
      bool m_enableCRDSA = false ;  // 是否启用 CRDSA

      float m_decodingThreshold [6] = {-6,-9,-12,-15,-17.5,-20};

      //std::map<LoraDeviceAddress, uint32_t> m_successfullyDecodedPackets;
      //std::set<std::pair<LoraDeviceAddress, uint32_t>> m_successfullyDecodedPackets;

         std::set<std::tuple<LoraDeviceAddress, uint32_t, uint64_t>> m_successfullyDecodedPackets; //  设备地址,FCnt,原始包ID
        std::vector<Ptr<Packet>> packetsToRemove; // 待删除的包

  protected:
};

} // namespace lorawan

} // namespace ns3
#endif /* GATEWAY_LORAWAN_MAC_H */
