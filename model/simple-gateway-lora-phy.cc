/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 University of Padova
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
 * Author: Davide Magrin <magrinda@dei.unipd.it>
 */

 #include "simple-gateway-lora-phy.h"
 #include "ns3/lora-tag.h"
 #include "ns3/simulator.h"
 #include "ns3/log.h"
 
 //****tests
 //extern "C++"{
 #include "ns3/idxnodes.h"
 #include "ns3/lorawan-mac-header.h"
 #include "ns3/lora-frame-header.h"
 #include "ns3/lrmodel.h"
 #include "ns3/adr-component.h"
 
 //}
 //****tests
 
 
 namespace ns3 {
 namespace lorawan {
 
 NS_LOG_COMPONENT_DEFINE ("SimpleGatewayLoraPhy");
 
 NS_OBJECT_ENSURE_REGISTERED (SimpleGatewayLoraPhy);
 
 /***********************************************************************
  *                 Implementation of Gateway methods                   *
  ***********************************************************************/
 
 TypeId
 SimpleGatewayLoraPhy::GetTypeId (void)
 {
   static TypeId tid = TypeId ("ns3::SimpleGatewayLoraPhy")
     .SetParent<GatewayLoraPhy> ()
     .SetGroupName ("lorawan")
     .AddConstructor<SimpleGatewayLoraPhy>();
 
   return tid;
 }
 
 SimpleGatewayLoraPhy::SimpleGatewayLoraPhy ()
 {
   NS_LOG_FUNCTION_NOARGS ();
 }
// SimpleGatewayLoraPhy::SimpleGatewayLoraPhy (bool enableCRDSA)
//   : m_enableCRDSA (enableCRDSA)
// {
// } 

 SimpleGatewayLoraPhy::~SimpleGatewayLoraPhy ()
 {
   NS_LOG_FUNCTION_NOARGS ();
 }
 
 void SimpleGatewayLoraPhy::TxFinished(Ptr<const Packet> packet)
 {
  
 }

 // 设置方法实现
void SimpleGatewayLoraPhy::SetEnableCRDSA(bool enable) {
    m_enableCRDSA = enable;
    NS_LOG_FUNCTION(this << enable);
}

// 获取方法实现
bool SimpleGatewayLoraPhy::GetEnableCRDSA() const {
    return m_enableCRDSA;
}
 
 void
 SimpleGatewayLoraPhy::Send (Ptr<Packet> packet, LoraTxParameters txParams,
                             double frequencyMHz, double txPowerDbm)
 {
   NS_LOG_FUNCTION (this << packet << frequencyMHz << txPowerDbm);
 
   // Get the time a packet with these parameters will take to be transmitted
   Time duration = GetOnAirTime (packet, txParams);
 
   NS_LOG_DEBUG ("Duration of packet: " << duration << ", SF" <<
                 unsigned(txParams.sf));
 
   // Interrupt all receive operations
   std::list<Ptr<SimpleGatewayLoraPhy::ReceptionPath> >::iterator it;
   for (it = m_receptionPaths.begin (); it != m_receptionPaths.end (); ++it)
     {
 
       Ptr<SimpleGatewayLoraPhy::ReceptionPath> currentPath = *it;
 
       if (!currentPath->IsAvailable ())     // Reception path is occupied
         {
           // Call the callback for reception interrupted by transmission
           // Fire the trace source
           if (m_device)
             {
               m_noReceptionBecauseTransmitting (currentPath->GetEvent ()->GetPacket (),
                                                 m_device->GetNode ()->GetId ());
 
             }
           else
             {
               m_noReceptionBecauseTransmitting (currentPath->GetEvent ()->GetPacket (), 0);
             }
 
           // Cancel the scheduled EndReceive call
           Simulator::Cancel (currentPath->GetEndReceive ());
 
           // Free it
           // This also resets all parameters like packet and endReceive call
           currentPath->Free ();
         }
     }
 
   // Send the packet in the channel
   m_channel->Send (this, packet, txPowerDbm, txParams, duration, frequencyMHz);
 
   Simulator::Schedule (duration, &SimpleGatewayLoraPhy::TxFinished, this, packet);
 
   m_isTransmitting = true;
 
   // Fire the trace source
   if (m_device)
     {
       m_startSending (packet, m_device->GetNode ()->GetId ());

     }
   else
     {
       m_startSending (packet, 0);

     }


}



 
 void
 SimpleGatewayLoraPhy::StartReceive (Ptr<Packet> packet, double rxPowerDbm,
                                     uint8_t sf, Time duration, double frequencyMHz)
 {
   NS_LOG_FUNCTION (this << packet << rxPowerDbm << duration << frequencyMHz);
  //  std::cerr<<"start receive"<<std::endl;
   // Fire the trace source
   m_phyRxBeginTrace (packet);
   
   if (m_isTransmitting)
     {
       // If we get to this point, there are no demodulators(解调器) we can use
       NS_LOG_INFO ("Dropping packet reception of packet with sf = "
                    << unsigned(sf) <<
                    " because we are in TX mode");
      //  std::cerr<<"Dropping packet reception of packet with sf = "
      //               << unsigned(sf) <<
      //               " because we are in TX mode"<<std::endl;
 
       m_phyRxEndTrace (packet);
 
       // Fire the trace source
       if (m_device)
         {
           m_noReceptionBecauseTransmitting (packet, m_device->GetNode ()->GetId ());
         }
       else
         {
           m_noReceptionBecauseTransmitting (packet, 0);
         }
 
 
       //*** tests
       // Checking if this is the place where packages fall
         Ptr<Packet> myPacket = packet->Copy ();
         LorawanMacHeader macHdr;
         myPacket->RemoveHeader (macHdr);
          LoraFrameHeader frameHdr;
          myPacket->RemoveHeader (frameHdr);
 
        LoraDeviceAddress edAddr = frameHdr.GetAddress ();
 
        trialdevices[FindNode(edAddr.GetNwkAddr())].fail+=1 ;
        trialdevices[FindNode(edAddr.GetNwkAddr())].previous_attempt = false;
        trialdevices[FindNode(edAddr.GetNwkAddr())].fail_for_transmitting +=1;
        //std::cout<<"No reception because it is transmitting\n";
        global_transmitting+=1;

        //**********tests
       return;
     }

 
   // Add the event to the LoraInterferenceHelper
   Ptr<LoraInterferenceHelper::Event> event;
   event = m_interference.Add (duration, rxPowerDbm, sf, packet, frequencyMHz);
 
   // Cycle over the receive paths to check availability to receive the packet
   std::list<Ptr<SimpleGatewayLoraPhy::ReceptionPath> >::iterator it;
 
   for (it = m_receptionPaths.begin (); it != m_receptionPaths.end (); ++it)
     {
       Ptr<SimpleGatewayLoraPhy::ReceptionPath> currentPath = *it;
 
       NS_LOG_DEBUG ("Current ReceptionPath is centered(集中) on frequency = " <<
                     currentPath->GetFrequency ());
       //std::cerr<<"Current ReceptionPath is centered(集中) on frequency = " <<
                     //currentPath->GetFrequency ()<<std::endl;
 
       // If the receive path is available and listening on the channel of
       // interest, we have a candidate
       if (currentPath->GetFrequency () == frequencyMHz
           && currentPath->IsAvailable ())
         {
           // See whether the reception power is above or below the sensitivity
           // for that spreading factor
           double sensitivity = SimpleGatewayLoraPhy::sensitivity[unsigned(sf) - 7];
 
           if (rxPowerDbm < sensitivity)       // Packet arrived below sensitivity接收端（如网关）能够正确解码信号的最低功率阈值（单位：dBm）
             {
 
               NS_LOG_INFO ("Dropping packet reception of packet with sf = "
                            << unsigned(sf) <<
                            " because under the sensitivity of "
                            << sensitivity << " dBm");
              //  std::cerr<<"rxPowerDbm= "<<rxPowerDbm<<" : Dropping packet reception of packet with sf = "
              //               << unsigned(sf) <<
              //               " because under the sensitivity of "
              //               << sensitivity << " dBm"<<std::endl;
 
               if (m_device)
                 {
                   m_underSensitivity (packet, m_device->GetNode ()->GetId ());
                 }
               else
                 {
                   m_underSensitivity (packet, 0);
                 }
 
 
               //*** tests
               // Checking if this is the place where packages fall
                 Ptr<Packet> myPacket = packet->Copy ();
                 LorawanMacHeader macHdr;
                 myPacket->RemoveHeader (macHdr);
                  LoraFrameHeader frameHdr;
                  myPacket->RemoveHeader (frameHdr);
 
                LoraDeviceAddress edAddr = frameHdr.GetAddress ();
 
                trialdevices[FindNode(edAddr.GetNwkAddr())].fail+=1 ;
                trialdevices[FindNode(edAddr.GetNwkAddr())].previous_attempt = false;
                //std::cout<<"Failed because is under the sensitivity threshold\n";
                trialdevices[FindNode(edAddr.GetNwkAddr())].fail_for_under_threshold+=1;
                global_under_threshold+=1;
 
                trialdevices[FindNode(edAddr.GetNwkAddr())].snr_ii = 0;
 
                //**********tests
 
               // Since the packet is below sensitivity, it makes no sense to
               // search for another ReceivePath
               return;
             }
           else        // We have sufficient sensitivity to start receiving,高于灵敏度，可接受
             {
               NS_LOG_INFO ("Scheduling reception of a packet, " <<
                            "occupying one demodulator");
                // std::cerr<<"Scheduling reception of a packet, " <<
                //             "occupying one demodulator"<<std::endl;
 

               // Block this resource
               currentPath->LockOnEvent (event);
               m_occupiedReceptionPaths++;

               // Schedule the end of the reception of the packet
               EventId endReceiveEventId = Simulator::Schedule (duration,
                                                                &LoraPhy::EndReceive,
                                                                this, packet,
                                                                event);
 
               currentPath->SetEndReceive (endReceiveEventId);
 
               // Make sure we don't go on searching for other ReceivePaths
               return;
             }
 
         }
 
     }
   // If we get to this point, there are no demodulators we can use
   NS_LOG_INFO ("Dropping packet reception of packet with sf = "
                << unsigned(sf) <<
                " because no suitable demodulator was found");
    // std::cerr<<"Dropping packet reception of packet with sf = "
    //             << unsigned(sf) <<
    //             " because no suitable demodulator was found"<<std::endl;
 
   //*** tests
   // Checking if this is the place where packages fall
     Ptr<Packet> myPacket = packet->Copy ();
     LorawanMacHeader macHdr;
     myPacket->RemoveHeader (macHdr);
      LoraFrameHeader frameHdr;
      myPacket->RemoveHeader (frameHdr);
 
    LoraDeviceAddress edAddr = frameHdr.GetAddress ();
 
    trialdevices[FindNode(edAddr.GetNwkAddr())].fail+=1 ;
    trialdevices[FindNode(edAddr.GetNwkAddr())].previous_attempt = false;
    //std::cout<<"Failed because there is no demodulator available\n";
    trialdevices[FindNode(edAddr.GetNwkAddr())].fail_for_no_demodulator +=1;
    global_no_demodulator+=1;
 
 
   //**********tests
 
 
   // Fire the trace source
   if (m_device)
     {
       m_noMoreDemodulators (packet, m_device->GetNode ()->GetId ());

     }
   else
     {
       m_noMoreDemodulators (packet, 0);
     }
 
 
 
 }
 
 void
 SimpleGatewayLoraPhy::EndReceive (Ptr<Packet> packet,
                                   Ptr<LoraInterferenceHelper::Event> event)
 {
   NS_LOG_FUNCTION (this << packet << *event);
  // std::cerr<<"end receive"<<std::endl;
 
   // Call the trace source
   m_phyRxEndTrace (packet);
 
 
   //*********************tests
   //std::cout<<"Testing interference on device "<< m_device->GetNode ()->GetId ()<<std::endl;
   //*********************tests
 
   // Call the LoraInterferenceHelper to determine whether there was
   // destructive interference. If the packet is correctly received, this
   // method returns a 0.
  
   uint8_t packetDestroyed = 0;
   packetDestroyed = m_interference.IsDestroyedByInterference (event);
   // Check whether the packet was destroyed
   if (packetDestroyed != uint8_t (0))
     {
       NS_LOG_DEBUG ("packetDestroyed by " << unsigned(packetDestroyed));
      // std::cerr<<"packetDestroyed by " << unsigned(packetDestroyed)<<std::endl;
       // Update the packet's LoraTag
       LoraTag tag;
       packet->RemovePacketTag (tag);
       tag.SetDestroyedBy (packetDestroyed);
       packet->AddPacketTag (tag);
 
       // Fire the trace source
       if (m_device)
         {
           m_interferedPacket (packet, m_device->GetNode ()->GetId ());
         }
       else
         {
           m_interferedPacket (packet, 0);
         }
 
     //*** tests
     // Checking if this is the place where packages fall
       Ptr<Packet> myPacket = packet->Copy ();
       LorawanMacHeader macHdr;
       myPacket->RemoveHeader (macHdr);
        LoraFrameHeader frameHdr;
        myPacket->RemoveHeader (frameHdr);
 
      LoraDeviceAddress edAddr = frameHdr.GetAddress ();
 
      trialdevices[FindNode(edAddr.GetNwkAddr())].fail+=1 ;
      trialdevices[FindNode(edAddr.GetNwkAddr())].previous_attempt = false;
      global_interference+=1;
 
      //std::cout<<"Failed because of interference\n";
      trialdevices[FindNode(edAddr.GetNwkAddr())].fail_for_interference +=1;
      //if (rlagent == 1)
      //{
      //	 rlprocess(FindNode(edAddr.GetNwkAddr()),1);
      //}
      ///************************************
      ///********************here we start the RL part
      ///************************************
      ///************************************
      ///************************************
      ///************************************
      ///************************************
 
 
      //**********tests
 
     }
   else       // Reception was correct
     {
 
       NS_LOG_INFO ("Packet with SF " <<
                    unsigned(event->GetSpreadingFactor ()) <<
                    " received correctly");
        // std::cerr<<"Packet with SF " <<
        //             unsigned(event->GetSpreadingFactor ()) <<
        //             " received correctly"<<std::endl;
       // Fire the trace source
       if (m_device)
         {
          // std::cerr<<"m_device"<<std::endl;
           m_successfullyReceivedPacket (packet, m_device->GetNode ()->GetId ());
          //std::cerr<<"GetEnableCRDSA()= "<<GetEnableCRDSA()<<std::endl;
          if (!GetEnableCRDSA())
          {
           // **** tests
          //  std::cerr<<"---Aloha--- global receive---"<<std::endl;
           Ptr<Packet> myPacket = packet->Copy ();
           LorawanMacHeader macHdr;
           myPacket->RemoveHeader (macHdr);
           LoraFrameHeader frameHdr;
           myPacket->RemoveHeader (frameHdr);
 
           LoraDeviceAddress edAddr = frameHdr.GetAddress ();
         
 
           //Apply a correction due to previous packets not being received
           if (trialdevices[FindNode(edAddr.GetNwkAddr())].previous_attempt == false)
           {

           }
 
 
            ///std::cout<< "receiving this at the end..."<< str.str() << std::endl;
          //  std::cout<<"###########DEVICE"<<  trialdevices[m_device->GetNode ()->GetId ()].id  <<"DELIVERING############"<<"\n"; // This is always the gateway
             //trialdevices[FindNode(edAddr.GetNwkAddr())].fail-=1 ;

             trialdevices[FindNode(edAddr.GetNwkAddr())].success+=1;
             trialdevices[FindNode(edAddr.GetNwkAddr())].previous_attempt = true;
              global_received+=1;
            std::cout<<"global_received= "<<global_received<<std::endl;
            
          
             if ( event->GetFrequency() == 868.1)
             {
                   release_freq(0,FindNode(edAddr.GetNwkAddr()));
                    //std::cout<<" releasing 868.1 "<<std::endl;
             }
             
             else
             {
                 if (event->GetFrequency() == 868.3)
                 {
                     release_freq(1,FindNode(edAddr.GetNwkAddr()));
                            //std::cout<<" releasing 868.3 "<<std::endl;
                 }
                 else
                 {
                     release_freq(2,FindNode(edAddr.GetNwkAddr()));
                            //std::cout<<" releasing 868.5 "<<std::endl;
                 }
             }

             }
            // ****  tests
 
         }
       else
         {
           m_successfullyReceivedPacket (packet, 0);
         }
         
      // std::cerr<<"!m_rxOkCallback.IsNull ()= "<<!m_rxOkCallback.IsNull ()<<std::endl;
       // Forward the packet to the upper layer
       if (!m_rxOkCallback.IsNull ())
         {

          
           // Make a copy of the packet
           // Ptr<Packet> packetCopy = packet->Copy ();
 
           // Set the receive power and frequency of this packet in the LoraTag: this
           // information can be useful for upper layers trying to control link
           // quality.

                               
          //compute the snr
          double snr = event->GetRxPowerdBm () + 174 - 10 * log10 (125000) - 6;
          double rxPower = event->GetRxPowerdBm ();
          double frequency = event->GetFrequency ();
          // std::cerr<<"snr= "<<snr<<" ,rxPower= "<<rxPower<<" ,frequency= "<<frequency<<std::endl;

           LoraTag tag;
           packet->RemovePacketTag (tag);
           tag.SetReceivePower (rxPower);
           tag.SetFrequency (frequency);
           tag.SetSnr(snr);
           packet->AddPacketTag (tag);

           //std::cerr<<"finish add tag"<<std::endl;
//lorawan-mac中m_phy->SetReceiveOkCallback(MakeCallback(&LorawanMac::Receive, this));
           m_rxOkCallback (packet);//->gateway-lorawan-mac.cc->Receive(packet);

         }
 
     }
 
   // Search for the demodulator that was locked on this event to free it.
 
   std::list< Ptr< SimpleGatewayLoraPhy::ReceptionPath > >::iterator it;
 
   for (it = m_receptionPaths.begin (); it != m_receptionPaths.end (); ++it)
     {
       Ptr<SimpleGatewayLoraPhy::ReceptionPath> currentPath = *it;
 
       if (currentPath->GetEvent () == event)
         {
           currentPath->Free ();
           m_occupiedReceptionPaths--;
           return;
         }
     }
 }
 
 }
 }
 