/*
 * ssMagicPacket.cc
 *
 *  Created on: Sep 10, 2016
 *      Author: sanjeev
 */

#include "ns3/ipv4.h"
#include "ns3/log.h"

#include "ns3/ppp-header.h"
#include "ss-base-topology-simulation-variables.h"
#include "ss-trs-netdevice.h"

/*
 * we do device transition statistics...
 */

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("ssTRSNetDeviceM");

/*****************************************************/
void ssTRSNetDevice::lpiTransmitSpecialPacket(const char* pMagicPacket,
		Time processingTime) {
	NS_LOG_FUNCTION(this << m_address << " packetTime" << processingTime);
	m_magicPacket = Create<Packet>((uint8_t*) pMagicPacket,
	STRING_LENGTH1);
	AddHeader(m_magicPacket, Ipv4_Protocol);
	if (simulationRunProperties::enableLinkEnergyManagement)
		//ENABLE_LINK_ENERGY_MANAGEMENT
		m_channelSS->TransmitStart(m_magicPacket, this, processingTime);
	else
		m_channel->TransmitStart(m_magicPacket, this, processingTime);

}

/*****************************************************/
void ssTRSNetDevice::SendAsleepMagicPacket(void) {
	NS_LOG_FUNCTION(this << m_address);
	lpiTransmitSpecialPacket(iamAsleep, m_SleepMagicPacketProcessingTime);
}

/*****************************************************/
void ssTRSNetDevice::SendWakeUpMagicPacket(void) {
	NS_LOG_FUNCTION(this << m_address);
	lpiTransmitSpecialPacket(wakeUpMessage, m_WakeUpMagicPacketProcessingTime);
}

/*****************************************************/
void ssTRSNetDevice::SendIamReadyMagicPacket(void) {
	NS_LOG_FUNCTION(this << m_address);
	// sanjeev Jun 18
	// if this device got a wake up message, then wake up the node fabric first
	// and then reply back "I'm awake"...so, reply_magic_packet may experience a long delay
	// add that here...
	Time t_replybackIamReadyMagicPacketProcessingTime =
			m_IamUpMagicPacketProcessingTime + m_additionalTimeFromNodeFabric;
	lpiTransmitSpecialPacket(iamUpMessage,
			t_replybackIamReadyMagicPacketProcessingTime);
	// for logging only..
	m_additionalTimeToWakeUpNodeFabricNanoSec += m_additionalTimeFromNodeFabric;
	// reset the attribute. sanjeev Jun 18
	m_additionalTimeFromNodeFabric = Seconds(0.);
}

/*****************************************************/
bool ssTRSNetDevice::IsWakeUpMagicPacket(Ptr<Packet> packet) {
	NS_LOG_FUNCTION(this << m_address);
	PppHeader t_dummyHeaderppp;
	packet->RemoveHeader(t_dummyHeaderppp);
	packet->CopyData(copyBuffer, sizeof(wakeUpMessage));
	NS_LOG_LOGIC("MagicPacket [" << copyBuffer << "]");
	if (strcmp((char*) copyBuffer, wakeUpMessage) == 0) {
		m_wakeUpPacketCount++;
		return true;
	} else
		return false;
}

/*****************************************************/
bool ssTRSNetDevice::IsIamReadyMagicPacket(Ptr<Packet> packet) {
	NS_LOG_FUNCTION(this << m_address);
	PppHeader t_dummyHeaderppp;
	packet->RemoveHeader(t_dummyHeaderppp);
	packet->CopyData(copyBuffer, sizeof(iamUpMessage));
	NS_LOG_LOGIC("MagicPacket [" << copyBuffer << "]");
	if (strcmp((char*) copyBuffer, iamUpMessage) == 0) {
		m_IamAwakePacketCount++;
		return true;
	} else
		return false;
}

/*****************************************************/
bool ssTRSNetDevice::IsAsleepMagicPacket(Ptr<Packet> packet) {
	NS_LOG_FUNCTION(this << m_address);
	PppHeader t_dummyHeaderppp;
	packet->RemoveHeader(t_dummyHeaderppp);
	packet->CopyData(copyBuffer, sizeof(iamAsleep));
	NS_LOG_LOGIC("MagicPacket [" << copyBuffer << "]");
	if (strcmp((char*) copyBuffer, iamAsleep) == 0)
		return true;
	else
		return false;
}

} // namespace
