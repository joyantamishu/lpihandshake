/*
 * ssTRSNetDevice.cc
 *
 *  Created on: Jul 25, 2016
 *      Author: sanjeev
 */
#include "ns3/log.h"
#include "ns3/ipv4.h"
#include "ns3/ppp-header.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/queue.h"
#include "ss-log-file-flags.h"
#include "ss-base-topology.h"

#include <fstream>

#include "ss-base-topology-simulation-variables.h"
#include "ss-trs-netdevice.h"
#include "ss-node.h"
/*
 * we collect device transition statistics...
 */
//************************************************************Madhurima Changed on May 4*****************************************************************
#define DefaultSleepMagicPacketProcessingTimeMicroSec		0 		//2.5 sanjeev Apr 19
#define DefaultWakeUpMagicPacketProcessingTimeMicroSec		0		//2.5 sanjeev Apr 19
#define DefaultIamUpMagicPacketProcessingTimeMicroSec		0		//2.5  sanjeev Apr 19
//*******************************************************************************************************************************************************
namespace ns3 {

NS_LOG_COMPONENT_DEFINE("ssTRSNetDevice");

NS_OBJECT_ENSURE_REGISTERED(ssTRSNetDevice);

TypeId ssTRSNetDevice::GetTypeId(void) {
	static TypeId tid =
			TypeId("ns3::ssTRSNetDevice").SetParent<ssTOSPointToPointNetDevice>().SetGroupName(
					"PointToPoint").AddConstructor<ssTRSNetDevice>();
	return tid;
}

ssTRSNetDevice::ssTRSNetDevice() :
		ssTOSPointToPointNetDevice() {
	NS_LOG_FUNCTION(this);
	NS_ASSERT(simulationRunProperties::enableDeviceTRSManagement);
	m_wakeUpPacketCount = 0;
	m_warningPacketCount = 0;
	m_IamAwakePacketCount = 0;
	// sanjeev. the flag used to send only 1 request, reset after getting a reply back, set during sendmagicpacket
	m_IsentWakeUpMagicPacket = false;
	m_additionalTimeToWakeUpNodeFabricNanoSec = Seconds(0.);
	m_magicPacket = 0;
	m_isDestinationUp = false;
	//
	// magic packet values
	//
	wakeUpMessage = wakeUpMessage_STRING;
	iamUpMessage = iamUpMessage_STRING;
	iamAsleep = iamAsleep_STRING;
	// right syntax, corrected from old old fault, Jun 18
	m_SleepMagicPacketProcessingTime = Time::FromDouble(
	DefaultSleepMagicPacketProcessingTimeMicroSec, Time::US);
	m_WakeUpMagicPacketProcessingTime = Time::FromDouble(
	DefaultWakeUpMagicPacketProcessingTimeMicroSec, Time::US);
	m_IamUpMagicPacketProcessingTime = Time::FromDouble(
	DefaultIamUpMagicPacketProcessingTimeMicroSec, Time::US);
	//
	// over-ride these values from base class, Apr 19, sanjeev
	//
	m_device_Ready_To_Transmit_TimeDelay = Time::FromDouble(
	DefaultDeviceReadyToTransmitTimeMicroSec_TRS, Time::US);
	m_device_Sleep_To_Transmit_TimeDelay = Time::FromDouble(
	DefaultDeviceSleepToTransmitTimeMicroSec_TRS, Time::US);
	m_deviceStateUpdateInterval = Time::FromDouble(
	DefaultDeviceRunwayIntervalMicroSec_TRS, Time::US);
	/* NOT REQUIRED Jun 18
	 m_device_Sleep_To_Transmit_TimeDelay =
	 m_device_Sleep_To_Transmit_TimeDelay / 10;
	 */
}

ssTRSNetDevice::~ssTRSNetDevice() {
	NS_LOG_FUNCTION(this);
}

void ssTRSNetDevice::DoDispose() {
	NS_LOG_FUNCTION(this);
	WriteStatisticsToFile();
	SS_ENEGRY_LOG(
			"TRSNetDevic Node [" << m_nodeId << "] device [" << GetIfIndex() << "] DeviceIpV4Address [" << m_address << "]  m_wakeUpPacketCount [" << m_wakeUpPacketCount << "], m_IamAwakePacketCount [" << m_IamAwakePacketCount <<"] AdditionalTimeToWakeUpNodeFabric [" << m_additionalTimeToWakeUpNodeFabricNanoSec << "ns]");
	ssTOSPointToPointNetDevice::DoDispose();
}

void ssTRSNetDevice::HandleDeviceStateChange(TxMachineState newState,
		TxMachineState oldState, bool restartMonitor) {
	NS_LOG_FUNCTION(this);
	// if simulation has stopped, then STOP...
	if (Simulator::IsFinished())
		return;

	// --> ready to sleep
	if (oldState == READY && newState == SLEEP) {
		// donot repeat sleep-->sleep (see#2,Apr19,sanjeev)
		if (m_isDestinationUp != false)
			SendAsleepMagicPacket();
	}
	// --> sleep to ready
	if (oldState == SLEEP && newState == READY) {
	}
	ssTOSPointToPointNetDevice::HandleDeviceStateChange(newState, oldState,
			restartMonitor);
}

bool ssTRSNetDevice::IsDestinationUp(void) {
	NS_LOG_FUNCTION(this);
	return m_isDestinationUp;
}

void ssTRSNetDevice::Receive(Ptr<Packet> packet) {
	NS_LOG_FUNCTION(this);
	NS_LOG_LOGIC(
			this << " Ipv4Address ["<< m_address << "] Node [" << m_nodeId << "] packetGUid [" << packet->GetUid () << "] packetId ["<< packet->packet_id << "]");
	// donot write traceEndPacket message, written by base class

	// check LPI logic incoming packet and machine state....
	if (IsWakeUpMagicPacket(packet->Copy())) {
		// sanjeev Jun 18.. check MY node fabric state & get addditional time for acknowledging this magic packet
		m_additionalTimeFromNodeFabric =
				DynamicCast<ssNode>(m_node)->GetNodeFabricStateAdditionalTime();
		if (m_txMachineStateSS == SLEEP) {
			t_now = Simulator::Now().GetNanoSeconds();
			t_duration = t_now - m_lastUpdateTimeNanoSec;
			m_lastUpdateTimeNanoSec = t_now;			// 3sanjeev
			m_txMachineStateSS = READY;
			m_totalSleepTimeNanoSec += t_duration;
			recordDeviceState(SLEEP, t_duration);
			// time to restart the monitoring timer, since we are moving from sleep to ready state....
			HandleDeviceStateChange(READY, SLEEP, true);
		}
		SendIamReadyMagicPacket(); //  duplicate IsWakeUpMagicPacket..., neighbor still thinks I'm sleeping- resend AWAKE
		return;
	}

	if (IsIamReadyMagicPacket(packet->Copy())) {
		// sanjeev. reset the flag only after getting a reply back, so that next time we can resend again if needed...
		NS_ASSERT_MSG(m_IsentWakeUpMagicPacket,
				"I did not sent wakeup request, but I got a wakeup response");
		m_IsentWakeUpMagicPacket = false;
		if (!m_isDestinationUp) {
			m_isDestinationUp = true;
			SendRemainingPackets();
			return;
		}
		return; // ignore duplicate IamReadyMagicPacket...
	}

	if (IsAsleepMagicPacket(packet->Copy())) {
		if (m_txMachineStateSS == BUSY)
			NS_LOG_LOGIC(
					"**ERROR/WARNING**  Neighbor went to sleep while I am still transmitting? He does not know that (check receive logic) [" << m_txMachineStateSS <<"]");
		else {
			m_isDestinationUp = false;
			// sanjeev Apr 17 ...Transition myself to sleep
			NS_LOG_LOGIC("IF NEIGHBOR IS ASLEEP, PUT MYSELF TO SLEEP TOO");
			if (m_txMachineStateSS != SLEEP)
				UpdateDeviceState(); // could go from ready-->sleep only (see#2,Apr19,sanjeev)
		}
		return;
	}

	if (m_txMachineStateSS == SLEEP) {
		NS_LOG_LOGIC(
				"**ERROR/WARNING** ssTRSNetDevice::Receive when m_txMachineStateSS == SLEEP, packetUid = " << packet->GetUid());
		// WARNING: Do not treat this as ERROR, this could be overlap of timing cycle, this device went to sleep AFTER the neighbor sent the packet,
		// packet was received after transition from ready->sleep.
		t_now = Simulator::Now().GetNanoSeconds();
		t_duration = t_now - m_lastUpdateTimeNanoSec;
		m_lastUpdateTimeNanoSec = t_now;			// 3sanjeev
		m_txMachineStateSS = READY;
		m_totalSleepTimeNanoSec += t_duration;
		recordDeviceState(SLEEP, t_duration);
		HandleDeviceStateChange(READY, SLEEP, true);
		m_warningPacketCount++;
		// continue, do not return;
	}

// call base class: ssPointToPointNetDevice
	return ssTOSPointToPointNetDevice::Receive(packet);
}

/******************************************************
 *
 * Send is same as ssP2Pnetdevice, with little change, see below
 * Sanjeev Mar 31
 ******************************************************/
bool ssTRSNetDevice::Send(Ptr<Packet> packet, const Address &dest,
		uint16_t protocolNumber) {
	NS_LOG_FUNCTION(this);
	NS_LOG_LOGIC(
			this << " Ipv4Address ["<< m_address << "] Node [" << m_nodeId << "] packetGUid [" << packet->GetUid () << "] packetId ["<< packet->packet_id << "]");
	// Jun 24 Max Q length observed in real time

	UpdateDeviceQueueData();
	// JUn 26. queuing metrics.
	packet->timePacketArrivedLink_ns = Simulator::Now().GetNanoSeconds();
	//
	//
	if (WRITE_PACKET_TO_FILE)
		tracePacket(Simulator::Now().ToDouble(Time::US), packet->flow_id,
				packet->packet_id, packet->GetUid(), packet->is_Last,
				GetNode()->GetId(), 0, DynamicCast<ssNode>(GetNode())->nodeName,
				"TRSDevice Send", packet->required_bandwidth);

	if (simulationRunProperties::enableCustomRouting)
		if (NetDeviceSendCallBack(packet))
			return true;

	Ptr<NetDeviceQueue> txq;
	if (m_queueInterface) {
		txq = m_queueInterface->GetTxQueue(0);
	}

	NS_ASSERT_MSG(!txq || !txq->IsStopped(),
			"Send should not be called when the device is stopped");

	//
	// If IsLinkUp() is false it means there is no channel to send any packet
	// over so we just hit the drop trace on the packet and return an error.
	//
	if (IsLinkUp() == false) {
		m_macTxDropTrace(packet);
		return false;
	}

	//
	// Stick a point to point protocol header on the packet in preparation for
	// shoving it out the door.
	//
	AddHeader(packet, protocolNumber);
	m_macTxTrace(packet);
	// sanjeev Jun 18.. check the node fabric state & get addditional time for this packet
	m_additionalTimeFromNodeFabric =
			DynamicCast<ssNode>(m_node)->GetNodeFabricStateAdditionalTime();
	NS_LOG_LOGIC(
			"**DEBUG** ssTRSNetDevice::Send::m_additionalTimeFromNodeFabric " << m_additionalTimeFromNodeFabric);
	// this is LPI logic if destination is not ready,
	// If the channel is NOT ready for transition, put packet in queue & we send the WAKEUP packet
	if (!m_isDestinationUp) {
		NS_LOG_LOGIC(
				"Packet [" << packet << "] put in Queue:: CurrentQueueItemCount=" << m_queue->GetNPackets());
		// AddHeader(packet, protocolNumber);// small work around now, sanjeev Mar 1
		if (!m_queue->Enqueue(Create<QueueItem>(packet))) {
			NS_LOG_UNCOND(
					"**ERROR** ::Enqueue failed:: Ipv4Address [" << m_address << "] Node [" << m_nodeId << "] FlowId [" << packet->flow_id << "] packetId ["<< packet->packet_id << "]");
			return false;
		}
		// sanjeev. Jun 18. If I have not sent the wakeup request, send it only once, and set this flag (will be reset after getting response)
		if (!m_IsentWakeUpMagicPacket)
			SendWakeUpMagicPacket();
		m_IsentWakeUpMagicPacket = true;
		return true; // sanjeev/3/31/tell the sender that packet transmitted <we will transmit packet later>
	}

	//
	// We should enqueue and dequeue the packet to hit the tracing hooks.
	//
	if (m_queue->Enqueue(Create<QueueItem>(packet))) {
		//
		// If the channel is ready for transition we send the packet right now
		//
		// Remember: you have to only these steps if transmitting packets out of Q,
		// in LPI mode (after destination comes ON)...
		if ((m_txMachineStateSS == READY) || (m_txMachineStateSS == SLEEP)) {
			packet = m_queue->Dequeue()->GetPacket();
			m_snifferTrace(packet);
			m_promiscSnifferTrace(packet);
			return TransmitStart(packet);
		}
		return true;
	}

	// Enqueue may fail (overflow). Stop the tx queue, so that the upper layers
	// do not send packets until there is room in the queue again.
	m_macTxDropTrace(packet);
	if (txq) {
		txq->Stop();
	}
	return false;
}

void ssTRSNetDevice::SendRemainingPackets(void) {
	NS_LOG_FUNCTION(this);
	NS_LOG_LOGIC(
			this << " m_address=" << m_address << " QueueItemCount=" << m_queue->GetNPackets());
	// sanjeev Jun 18.. check NOT REQUIRED for node fabric state ..but how to verify this..
	// GetNodeFabricStateAdditionalTime() is not required, since it is set for 1st packet and
	// subsequent packets should NOT experience node fabric delay.

	Address voidAddress;
	Ptr<Packet> packet;
	// send remaining packets that were stacked in the queue..
	if (m_queue->GetNPackets() > 0) {
		//NS_ASSERT_MSG(m_txMachineStateSS == SLEEP, "");
		packet = m_queue->Dequeue()->GetPacket();
		m_snifferTrace(packet);
		m_promiscSnifferTrace(packet);
		TransmitStart(packet);
	}
}

// May 1
void ssTRSNetDevice::WriteStatisticsToFile(void) {
	NS_LOG_FUNCTION(this);
	if (WRITE_DEVICE_ENERGY_TO_FILE) {

		BaseTopology::fpDeviceEnergy << "TRSNetDevice,Node," << m_nodeId
				<< ",Device," << GetIfIndex()
				<< ",DeviceIpV4Address," << m_address
				<< ",m_wakeUpPacketCount," << m_wakeUpPacketCount
				<< ",m_IamAwakePacketCount," << m_IamAwakePacketCount
				<< ",AdditionalTimeToWakeUpNodeFabric[ns],"	<< m_additionalTimeToWakeUpNodeFabricNanoSec
				<< "\n";
	}
}

}   //namespace
