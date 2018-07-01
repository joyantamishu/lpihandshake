/*
 * ss-tos-point-to-point-netdevice.cc
 *
 *  Created on: Jul 25, 2016
 *      Author: sanjeev
 */

#include <fstream>

#include "ns3/core-module.h"
#include "ns3/ipv4.h"
#include "ns3/network-module.h"
#include "ns3/queue.h"

#include "ss-base-topology-simulation-variables.h"
#include "ss-device-energy-model.h"
#include "ss-energy-source.h"
#include "ss-node.h"
#include "ss-log-file-flags.h"
#include "ss-tos-point-to-point-netdevice.h"
#include "ss-base-topology.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("ssTOSPointToPointNetDevice");

NS_OBJECT_ENSURE_REGISTERED(ssTOSPointToPointNetDevice);

TypeId ssTOSPointToPointNetDevice::GetTypeId(void) {
	static TypeId tid =
			TypeId("ns3::ssTOSPointToPointNetDevice").SetParent<
					PointToPointNetDevice>().SetGroupName("PointToPoint").AddConstructor<
					ssTOSPointToPointNetDevice>();
	return tid;
}

/*
 "/NodeList/[i]/DeviceList/[i]/$ns3::PointToPointNetDevice"
 Attributes = DataRate: The default data rate for point to point links
 InterframeGap: The time to wait between packet (frame) transmissions

 "/NodeList/[i]/DeviceList/[i]/$ns3::PointToPointNetDevice/TxQueue"
 MaxPackets: The maximum number of packets accepted by this queue.
 */
/******************************************************/
ssTOSPointToPointNetDevice::ssTOSPointToPointNetDevice() :
		PointToPointNetDevice() {
	NS_LOG_FUNCTION(this);
	//NS_ASSERT(simulationRunProperties::enableDeviceTOSManagement);...sanjeev Apr 24,disable
	NS_ASSERT_MSG(
			DefaultDeviceReadyToTransmitTimeMicroSec_TOS < DefaultDeviceRunwayIntervalMicroSec_TOS,
			"DefaultDeviceReadyToTransmitTimeMicroSec_TOS should be less than DefaultDeviceStateUpdateIntervalMicroSec_TOS");

	m_rcvPacketcount = 0;
	m_sendPacketcount = 0;
	m_countSleepState = 0;
	m_totalSleepTimeNanoSec = 0;
	m_totalIdleTimeNanoSec = 0;
	m_totalTransmitTimeNanoSec = 0;

	if (simulationRunProperties::enableDeviceTOSManagement) {
		// ENABLE_DEVICE_ENERGY_MANAGEMENT
		m_enableDeviceEnergyManagement = true;
		// right syntax, corrected from old old fault, Jun 18
		m_device_Ready_To_Transmit_TimeDelay = Time::FromDouble(
		DefaultDeviceReadyToTransmitTimeMicroSec_TOS, Time::US);
		// this is the 100 ns time to come out of the sleep
		m_device_Sleep_To_Transmit_TimeDelay = Time::FromDouble(
		DefaultDeviceSleepToTransmitTimeMicroSec_TOS, Time::US);
		/* NOT REQUIRED Jun 18
		 m_device_Sleep_To_Transmit_TimeDelay =
		 m_device_Sleep_To_Transmit_TimeDelay / 10;
		 */
		// May 3rd, as per MR..
		m_queuedPacketTxnDelayTimeNanoSec = Seconds(0);
	} else {
		m_enableDeviceEnergyManagement = false;
		m_device_Ready_To_Transmit_TimeDelay = Seconds(0);
		m_device_Sleep_To_Transmit_TimeDelay = Seconds(0);
		m_queuedPacketTxnDelayTimeNanoSec = Seconds(0);
	}
	m_tInterframeGap = Seconds(0);
	m_deviceStateUpdateInterval = Time::FromDouble(
	DefaultDeviceRunwayIntervalMicroSec_TOS, Time::US);
	// sanjeev Jun 18. initialize
	m_additionalTimeFromNodeFabric = Seconds(0.); // sanjeev Jun 18..additional time from node fabric, if ANY?
	m_additionalTimeFromDeviceState = Seconds(0.); // sanjeev Jun 18..additional time from device state transition if any...

	m_nodeId = -1;
	m_txMachineStateSS = INIT;
	m_channelSS = 0;

	m_linkData = new LinkData;
	m_linkData->m_totalTransmitTimeNanoSec = 0;
	m_linkData->m_totalSleepTimeNanoSec = 0;
	m_linkData->m_totalIdleTimeNanoSec = 0;
	m_linkData->m_totalTransmitTimeNanoSec = 0;
	m_linkData->m_packetCount = 0;
	m_linkData->m_countSleepState = 0;
	m_linkData->IsLinkUp = false;

	m_deviceStatisticsBuffer = new DeviceStatisticsBuffers;
	m_deviceData = new DeviceData;
	m_deviceData->m_totalSleepTimeNanoSec = 0;
	m_deviceData->m_totalIdleTimeNanoSec = 0;
	m_deviceData->m_totalTransmitTimeNanoSec = 0;
	m_deviceData->m_totalDeviceTimeNanoSec = 0;
	m_deviceData->m_lastQDepthTimeNanoSec = 0;				// Jun 24
	m_deviceData->t_additionalTimeFromNodeFabricNanoSec = 0;
	m_deviceData->t_additionalTimeFromDeviceStateNanoSec = 0;
	m_deviceData->m_sendPacketCount = 0;
	m_deviceData->m_rcvPacketCount = 0;
	m_deviceData->m_packetCount = 0;
	m_deviceData->m_countSleepState = 0;
	m_deviceData->m_DeviceState = 0;
	m_deviceData->m_DeviceTotalObservedQueueDepthIntegralns = 0;// Apr 17,May 1, sanjeev, Jun 24
	m_deviceData->m_DeviceMaxQueueDepth = 0;			// Apr 17,May 1 sanjeev
	m_deviceData->m_DeviceObservedQueueDepth = 0;		// Apr 17,May 1 sanjeev
	m_deviceData->m_isDeviceUp = false;

	m_lastUpdateTimeNanoSec = 0;
	m_deviceQMetrics.m_totalProcessingTime_ns = 0;
	m_deviceQMetrics.m_totalWaitingTime_ns = 0;
	m_deviceQMetrics.m_firstPacketArriveTime_ns = 0;
	m_deviceQMetrics.m_lastPacketDepartTime_ns = 0;

	setCaptureDeviceStateBufferSize(DefaultDataCollectionBufferSize);
}
/******************************************************/
ssTOSPointToPointNetDevice::~ssTOSPointToPointNetDevice() {
	NS_LOG_FUNCTION(this);
}

/******************************************************/
void ssTOSPointToPointNetDevice::DoInitialize() {
	NS_LOG_FUNCTION(this);
	NS_ASSERT_MSG(
			m_deviceStateUpdateInterval
					< DynamicCast<ssNode>(m_node)->m_nodeRunwayTime,
			"m_deviceStateUpdateInterval should be less than m_nodeRunwayTime");
	PointToPointNetDevice::DoInitialize();
	Ptr<Queue> queue = GetQueue();
	NS_ASSERT_MSG(queue != NULL, "P2Pnetdevice Queue is NULL");
	if (simulationRunProperties::deviceQDepth > 0)
		queue->SetMaxPackets(simulationRunProperties::deviceQDepth); // this helps trigger drop packets, if non-zero, gets good test-results !!

	if (simulationRunProperties::enableLinkEnergyManagement) {
		// ENABLE_LINK_ENERGY_MANAGEMENT
		m_channelSS->DoInitialize();
	}

	Ptr<Ipv4> ipv4 = m_node->GetObject<Ipv4>(); // Get Ipv4 instance of the node
	int32_t ifIndex = ipv4->GetInterfaceForDevice(this);
	if (ifIndex == -1)
		m_address = Ipv4Address("0.0.0.0");
	m_address = ipv4->GetAddress(ifIndex, 0).GetLocal(); // Get Ipv4InterfaceAddress of xth interface.;

	m_txMachineStateSS = READY;
	// start monitoring the device states...
	// now the device is ready, start monitoring for idle time sleep time etc.
	m_deviceStateUpdateEvent = Simulator::Schedule(m_deviceStateUpdateInterval,
			&ssTOSPointToPointNetDevice::UpdateDeviceState, this);
}

/******************************************************/
void ssTOSPointToPointNetDevice::DoDispose() {
	NS_LOG_FUNCTION(this);
	// sanjeev, Jun 24
	// https://en.wikipedia.org/wiki/Little%27s_law
	double t_total_response_time_ns = m_totalTransmitTimeNanoSec;
	double t_total_requests = m_sendPacketcount;
	double t_total_system_time_ns = m_totalTransmitTimeNanoSec
			+ m_totalIdleTimeNanoSec + m_totalSleepTimeNanoSec;
	double deviceTime = (t_total_system_time_ns) / NANOSEC_TO_SECS;
	double avg_processing_time_ns = 0;
	// aka avg queue time, incl service time + wait time, *avg* processing time per request.
	if (t_total_response_time_ns != 0.0 && t_total_requests != 0.0)
		avg_processing_time_ns = t_total_response_time_ns / t_total_requests;
	double avg_arrival_rate_s = 0;
	// at what rate is the requests coming in...
	if (t_total_system_time_ns != 0 && t_total_requests != 0)
		avg_arrival_rate_s = t_total_requests / deviceTime; //requests per second unit
	double deviceUtilzation = 100 * m_totalTransmitTimeNanoSec
			/ (t_total_system_time_ns);
	Ptr<Queue> queue = GetQueue();
	SS_ENEGRY_LOG(
			"ssNetDevice Node [" << m_nodeId << "] device [" << GetIfIndex() << "] Type [" << DynamicCast<ssNode>(GetNode())->nodeName << "] DataRate [" << getChannelDataRate()
			<< "Mbps] DeviceStateAdditionalTime [" << m_deviceData->t_additionalTimeFromDeviceStateNanoSec
			<< "ns] AdditionalTimeFromNodeFabric [" << m_deviceData->t_additionalTimeFromNodeFabricNanoSec << "ns]");
	SS_ENEGRY_LOG(
			"ssNetDevice Node [" << m_nodeId << "] device [" << GetIfIndex() << "] DeviceIpV4Address [" << m_address << "] rcvPacket [" << m_rcvPacketcount << "] sendPacket [" << m_sendPacketcount << "] queue_TotalDroppedPackets [" << queue->GetTotalDroppedPackets() << "] countSleepState [" << m_countSleepState << "]");
	SS_ENEGRY_LOG(
			"ssNetDevice Node [" << m_nodeId << "] device [" << GetIfIndex() << "] totalDeviceTime [" << deviceTime << "s] m_totalTransmitTimeNanoSec [" << m_totalTransmitTimeNanoSec << "] m_totalIdleTimeNanoSec [" << m_totalIdleTimeNanoSec << "] m_totalSleepTimeNanoSec [" << m_totalSleepTimeNanoSec << "]");
	SS_ENEGRY_LOG(
			"ssNetDevice Node [" << m_nodeId << "] device [" << GetIfIndex()
			<< "] Device Utilzation [" << deviceUtilzation
			<< "%] MaxQueueDepth [" << m_deviceData->m_DeviceMaxQueueDepth
			<< "] QdepthIntegralns [" << m_deviceData->m_DeviceTotalObservedQueueDepthIntegralns << "]");
	SS_ENEGRY_LOG(
			"ssNetDevice Node [" << m_nodeId << "] device [" << GetIfIndex() << "] avg_processing_time [" << avg_processing_time_ns << "ns/pkt] avg_arrival_rate [" << avg_arrival_rate_s << "pkt/s]");
	/*
	 *  Jun 26..device queue metrics.
	 struct DeviceLevelQueuingMetrics {
	 int64_t m_totalProcessingTime_ns;
	 int64_t m_totalWaitingTime_ns;
	 int64_t m_firstPacketArriveTime_ns;
	 int64_t m_lastPacketDepartTime_ns;
	 timePacketArrivedLink_ns, timePacketProcessAtLink_ns,
	 timePacketLeftLink_ns; // sanjeev Jun 26
	 struct DeviceLevelQueuingMetrics.
	 (1) packet->timePacketArrivedLink_ns = customers come in - arrivaltime
	 (2) customer waits waits waits...
	 (3) packet->timePacketProcessAtLink_ns = customer gets processed - process_start_time
	 (4) packet->timePacketLeftLink_ns = customer finishes processing (I know when he will get out  meaning when he will reach next device/node)
	 (5) Computation::waitingTime::   m_totalWaitingTime_ns = timePacketProcessAtLink_ns  - timePacketArrivedLink_ns
	 (6) Computation::processingTime:: m_totalProcessingTime_ns = timePacketLeftLink_ns - timePacketProcessAtLink_ns
	 customer process time = process_finish - process_start_time
	 total service = processTime+waitTime.
	 (7) Utilization:: if my total TxnTime = x, and totalSimTime = y, ,i am busy.utilization = x/y remaining time i am nnot doing anything....
	 ** everything displayed with suffix Calc is calculated using Little's law...
	 */
	double avgResponseTimeCalc = 0;
	double avgThoughputCalc = 0;
	double avgWaitingTime_ns = 0;
	double avgResponseTime_ns = 0;
	double avgProcessingTime_ns = 0;
	double avgThoughput_as_packets = 0;
	double avgUtilizationCalc = 100 * m_deviceQMetrics.m_totalProcessingTime_ns
			/ (simulationRunProperties::simStop * NANOSEC_TO_SECS);
	double avgQueueDepthCalc = double(
			m_deviceData->m_DeviceTotalObservedQueueDepthIntegralns)
			/ t_total_system_time_ns;
	if (t_total_requests != 0) {
		avgThoughput_as_packets = (NANOSEC_TO_SECS * t_total_requests) //packets per second
				/ (m_deviceQMetrics.m_lastPacketDepartTime_ns
						- m_deviceQMetrics.m_firstPacketArriveTime_ns);
		avgThoughputCalc = avgThoughput_as_packets * 8 //bits per second
				* simulationRunProperties::packetSize;
		avgResponseTimeCalc = avgQueueDepthCalc / avgThoughput_as_packets; // time in second, calculated using little's law
		avgProcessingTime_ns = m_deviceQMetrics.m_totalProcessingTime_ns //calculated per packet processing time in nanosecond unit
				/ (t_total_requests);
		avgWaitingTime_ns = m_deviceQMetrics.m_totalWaitingTime_ns //calculated per packet waiting time in nanosecond unit
				/ (t_total_requests);
		avgResponseTime_ns = avgProcessingTime_ns + avgWaitingTime_ns; //calculated response time in nanosecond, this should be matched with avgResponseTimeCalc
	}
	//ssNetDevice Node [7] device [1] AvgUtilizationCalc [8.69326%] AvgQueueDepthCalc [15.7227] avgQueueDepthCalc1 [0.00140381] AvgResponseTimeCalc [9.2368e-08s]
	// everything displayed with suffix Calc is calculated using Little's law...
	SS_ENEGRY_LOG(
			"ssNetDevice Node [" << m_nodeId << "] device [" << GetIfIndex()
			<< "] AvgProcessingTimeCalc [" << avgProcessingTime_ns
			<< "ns] AvgWaitingTimeCalc [" << avgWaitingTime_ns
			<< "ns] AvgResponseTimeCalc [" << avgResponseTime_ns
			<< "ns] AvgThoughputCalc [" << avgThoughputCalc/1000 << "Mbps]");
	SS_ENEGRY_LOG(
			"ssNetDevice Node [" << m_nodeId << "] device [" << GetIfIndex()
			<< "] AvgUtilizationCalc [" << avgUtilizationCalc
			<< "%] AvgQueueDepthCalc [" << avgThoughput_as_packets*avgResponseTimeCalc
			<< "] avgQueueDepthCalc1 [" << avgQueueDepthCalc //Sanjeev, You are using the same equation just in the above line. So obviously you should always get the same value as AvgQueueDepthCalc, so this equation simply makes no sense
			<< "] AvgResponseTimeCalcLittle'sLaw [" << (avgResponseTimeCalc * NANOSEC_TO_SECS) << "ns]"); //According to me, this supposed to be the waiting time, not the Response Time
	SS_ENEGRY_LOG(
			"ssNetDevice Node [" << m_nodeId << "] device [" << GetIfIndex()
			<< "] Delay/Wait [" << NANOSEC_TO_SECS * avgQueueDepthCalc/avg_arrival_rate_s << "ns"
			<< "] AvgUtilizationCalc1 [" << avgUtilizationCalc
			<< "] AvgUtilizationCalc2Little'sLaw [ " << 100 * 8 * simulationRunProperties::packetSize * avg_arrival_rate_s/(getChannelDataRate() * MegaSec_to_Secs) //Sanjeev,avg_arrival_rate_s in packets per second, so we need to divide with something that has a unit of per second, that's why I have multiplied it with MegaSec_to_Secs, I did not change the main method getChannelDataRate(), since it might be used in other places
			<< "%] AvgUtilizationCalc3DoesnotMakeAnySense [" << avgResponseTimeCalc/getChannelDataRate() //This equation does not make any sense
			<< " utilization3DoesnotMakeAnySense " << 8
			* simulationRunProperties::packetSize * avg_arrival_rate_s/avgResponseTimeCalc ); //Same as the previous, does not make any sense

	ssTOSPointToPointNetDevice::WriteStatisticsToFile();

	if (m_channelSS)
		m_channelSS->DoDispose(this);
	if (!simulationRunProperties::enableDeviceTOSManagement)
		SS_ENEGRY_LOG(
				"ssNetDevice Node [" << m_nodeId << "] device [" << GetIfIndex() << "] DeviceIpV4Address [" << m_address << "] DEVICE ENERGY MODEL NOT ENABLED");
	else
		m_pEnergyModel->DoDispose();

	PointToPointNetDevice::DoDispose();
}

/******************************************************/
bool ssTOSPointToPointNetDevice::Attach(Ptr<PointToPointChannel> ch) {
	NS_LOG_FUNCTION(this);
	m_txMachineStateSS = READY;
	m_nodeId = m_node->GetId();
	if (simulationRunProperties::enableLinkEnergyManagement) {
		// ENABLE_LINK_ENERGY_MANAGEMENT
		m_channelSS = DynamicCast<ssPointToPointChannel>(ch);
	}

	t_duration = Simulator::Now().GetNanoSeconds();
	recordDeviceState(INIT, t_duration); // test........time device was in init-state
	return PointToPointNetDevice::Attach(ch);
}

/******************************************************/
bool ssTOSPointToPointNetDevice::IsDestinationUp_Unused() {
	NS_LOG_FUNCTION(this << m_address);
	/*...sanjeev --*/
	Ptr<ssTOSPointToPointNetDevice> pND =
			DynamicCast<ssTOSPointToPointNetDevice>(
					m_channelSS->GetDestination(0));
	if (pND == NULL)
		return false;
	if (pND == this)
		pND = DynamicCast<ssTOSPointToPointNetDevice>(
				m_channelSS->GetDestination(1));
	if (pND == NULL)
		return false;

	return pND->IsDeviceUp();
}

/******************************************************/
Ipv4Address ssTOSPointToPointNetDevice::getIpv4Address() const {
	return m_address;
}

/******************************************************/
int ssTOSPointToPointNetDevice::getNodeId() const {
	return m_nodeId;
}

// get sendChannel dataRate, or rcvChannel dataRate
int ssTOSPointToPointNetDevice::getChannelDataRate(void) {
	NS_LOG_FUNCTION(this); // these are public for fast access, without growing stack
	return m_bps.GetBitRate() / MegaSec_to_Secs;
}

/******************************************************/
void ssTOSPointToPointNetDevice::HandleDeviceStateChange(
		TxMachineState newState, TxMachineState oldState, bool restartMonitor) {
	NS_LOG_FUNCTION(this);
	/*Sanjeev May 26*/
	DynamicCast<ssNode>(m_node)->SetDeviceState(GetIfIndex(),
			newState == SLEEP);
	if (restartMonitor) {
		if (m_deviceStateUpdateEvent.IsRunning()) // CHECK-JUN25
			m_deviceStateUpdateEvent.Cancel();
		// we need to set this again since we change it at UpdateDevice()-->Ready-->Sleep
		// old syntax corrected, sanjee Jun 18
		m_deviceStateUpdateInterval = Time::FromDouble(
		DefaultDeviceRunwayIntervalMicroSec_TOS, Time::US);
		if (simulationRunProperties::enableDeviceTRSManagement)
			m_deviceStateUpdateInterval = Time::FromDouble(
			DefaultDeviceRunwayIntervalMicroSec_TRS, Time::US);

		m_deviceStateUpdateEvent = Simulator::Schedule(
				m_deviceStateUpdateInterval,
				&ssTOSPointToPointNetDevice::UpdateDeviceState, this);
	}
}
/*
 * This should be called after the p2p.install has been called...and after device is installed/connected/attached..
 */
void ssTOSPointToPointNetDevice::SetDeviceReceiveErrorModel() {
	NS_LOG_FUNCTION(this);
	std::string errorModelType = "ns3::RateErrorModel";
	ObjectFactory factory;
	factory.SetTypeId(errorModelType);
	Ptr<ErrorModel> em = factory.Create<ErrorModel>();
	em->SetAttribute("ErrorRate", DoubleValue(0.00001));
	SetAttribute("ReceiveErrorModel", PointerValue(em));
}

/******************************************************/
void ssTOSPointToPointNetDevice::Receive(Ptr<Packet> packet) {
	NS_LOG_FUNCTION(this);
	NS_LOG_LOGIC(
			this << " Ipv4Address ["<< m_address << "] Node [" << m_nodeId << "] packetGUid [" << packet->GetUid () << "] packetId ["<< packet->packet_id << "]");
	// debug end packet, SS,JB: Mar 26
	if (WRITE_PACKET_TO_FILE)
		tracePacket(Simulator::Now().ToDouble(Time::US), packet->flow_id,
				packet->packet_id, packet->GetUid(), packet->is_Last, m_nodeId,
				m_queue->GetNPackets(),
				DynamicCast<ssNode>(GetNode())->nodeName, "TOS Device Rcv",
				packet->required_bandwidth);
	m_rcvPacketcount++;
	NS_LOG_LOGIC(
			this << " Ipv4Address ["<< m_address << "] Node [" << m_nodeId << "], m_rcvPacketcount [" << m_rcvPacketcount <<"] thisPacketUid [" << packet->GetUid () << "] packet " << packet);

	if (true)
		if (NetDeviceReceiveCallBack(packet))
			return;
	// donot call base routing if my custom router did the custom routing
	//... Of course, you have to handle the statistics counters correctly...
	return PointToPointNetDevice::Receive(packet);
}

/******************************************************/
bool ssTOSPointToPointNetDevice::Send(Ptr<Packet> packet, const Address &dest,
		uint16_t protocolNumber) {
	NS_LOG_FUNCTION(this);
	NS_LOG_LOGIC(
			this << " Ipv4Address ["<< m_address << "] Node [" << m_nodeId << "] packetGUid [" << packet->GetUid () << "] packetId ["<< packet->packet_id << "]");
	// Jun 24
	UpdateDeviceQueueData();
	// JUn 26. queuing metrics.
	packet->timePacketArrivedLink_ns = Simulator::Now().GetNanoSeconds();
	//
	if (WRITE_PACKET_TO_FILE)
		tracePacket(Simulator::Now().ToDouble(Time::US), packet->flow_id,
				packet->packet_id, packet->GetUid(), packet->is_Last, m_nodeId,
				m_queue->GetNPackets(),
				DynamicCast<ssNode>(GetNode())->nodeName, "TOS Device Send",
				packet->required_bandwidth);

	if (true)
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
			"**DEBUG** ssTOSPointToPointNetDevice::Send::m_additionalTimeFromNodeFabric " << m_additionalTimeFromNodeFabric);

	//
	// We should enqueue and dequeue the packet to hit the tracing hooks.
	//
	if (m_queue->Enqueue(Create<QueueItem>(packet))) {
		//
		// If the channel is ready for transition we send the packet right now
		//
		//
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

/******************************************************/
// over ride the method here, cannot call base class method since I am re-writing this function
bool ssTOSPointToPointNetDevice::TransmitStart(Ptr<Packet> p) {
	NS_LOG_FUNCTION(this);
	NS_LOG_LOGIC(
			this << " Ipv4Address ["<< m_address << "] Node [" << m_nodeId << "], thisPacketUid [" << p->GetUid () << "] packet " << p << " m_txMachineStateSS=" << m_txMachineStateSS);
	if (WRITE_PACKET_TO_FILE)
		tracePacket(Simulator::Now().ToDouble(Time::US), p->flow_id,
				p->packet_id, p->GetUid(), p->is_Last, m_nodeId,
				m_queue->GetNPackets(),
				DynamicCast<ssNode>(GetNode())->nodeName, "TOS Txn Start",
				p->required_bandwidth);
	//
	// This function is called to start the process of transmitting a packet.
	// We need to tell the channel that we've started wiggling the wire and
	// schedule an event that will be executed when the transmission is complete.
	//
	NS_ASSERT_MSG(
			(m_txMachineStateSS == READY) || (m_txMachineStateSS == SLEEP),
			"Must be READY or SLEEP to transmit " << m_txMachineStateSS);
	m_currentPkt = p;
	m_phyTxBeginTrace (m_currentPkt);
	m_sendPacketcount++;
	if (m_deviceStateUpdateEvent.IsRunning()) // CHECK-JUN25
		m_deviceStateUpdateEvent.Cancel();
	// restart monitoring only after txn is complete... (see next function)
	// we need to set this again since we change it at UpdateDevice()-->Ready-->Sleep
	// sanjeev: Jun 18..correct old syntax...
	m_deviceStateUpdateInterval = Time::FromDouble(
	DefaultDeviceRunwayIntervalMicroSec_TOS, Time::US);
	if (simulationRunProperties::enableDeviceTRSManagement)
		m_deviceStateUpdateInterval = Time::FromDouble(
		DefaultDeviceRunwayIntervalMicroSec_TRS, Time::US);

	// for this link - check the duration and put them into diff state...
	t_now = Simulator::Now().GetNanoSeconds();
	t_duration = t_now - m_lastUpdateTimeNanoSec;
	m_lastUpdateTimeNanoSec = t_now;
	m_additionalTimeFromDeviceState = Seconds(0.0);

	// start monitoring the device states...
	switch (m_txMachineStateSS) {
	//
	case READY:
		if (simulationRunProperties::enableDeviceTOSManagement
				|| simulationRunProperties::enableDeviceTRSManagement)
			m_additionalTimeFromDeviceState =
					m_device_Ready_To_Transmit_TimeDelay;
		m_totalIdleTimeNanoSec += t_duration;
		recordDeviceState(READY, t_duration);
		// additional logging , Mar 8
		NS_LOG_LOGIC(
				this->GetInstanceTypeId().GetName() << " Node [" << m_nodeId << "], Device moving READY --> BUSY [" << t_duration << "] additionalTime [" << m_additionalTimeFromDeviceState << "] Run-way [" << m_deviceStateUpdateInterval << "]");
		break;
	case BUSY:
		NS_LOG_LOGIC(
				"Node [" << m_nodeId << "], Device ?ERROR? moving BUSY --> BUSY [" << t_duration <<"]");
		// no need to record time, it is alreaady done in rcv/send logic, after busy time --> put it back to ready state
		break;
	case SLEEP:
		m_additionalTimeFromDeviceState = m_device_Sleep_To_Transmit_TimeDelay;
		m_totalSleepTimeNanoSec += t_duration;
		recordDeviceState(SLEEP, t_duration);
		// additional logging , Mar 8
		NS_LOG_LOGIC(
				this->GetInstanceTypeId().GetName() << " Node [" << m_nodeId << "], Device moving SLEEP --> BUSY [" << t_duration << "] additionalTime [" << m_additionalTimeFromDeviceState << "] Run-way [" << m_deviceStateUpdateInterval << "]");
		break;
	default:
		NS_LOG_LOGIC(
				"Node [" << m_nodeId << "], Device *ERROR* default --> BUSY [" << t_duration<< "]");
		break;
	}
	// as per MR, May 3rd
	m_tInterframeGap = m_queuedPacketTxnDelayTimeNanoSec
			* m_queue->GetNPackets();
	// for logging only..
	m_deviceData->t_additionalTimeFromNodeFabricNanoSec +=
			m_additionalTimeFromNodeFabric.ToDouble(Time::NS);
	m_deviceData->t_additionalTimeFromDeviceStateNanoSec +=
			m_additionalTimeFromDeviceState.ToDouble(Time::NS);
	// sanjeev, jun 18. critical see p2p original code , line 240
	Time txTime = m_bps.CalculateBytesTxTime(p->GetSize());
	Time txCompleteTime = txTime + m_additionalTimeFromDeviceState
			+ m_additionalTimeFromNodeFabric;
	// Jun 26... ready to process
	p->timePacketProcessAtLink_ns = Simulator::Now().GetNanoSeconds();
	p->timePacketLeftLink_ns = Simulator::Now().GetNanoSeconds()
			+ txCompleteTime.GetNanoSeconds();
	UpdatePacketMetricsData(p);
	//
	HandleDeviceStateChange(BUSY, m_txMachineStateSS, false);// sanjeev May 26

	m_txMachineStateSS = BUSY;
	m_lastUpdateTimeNanoSec = txCompleteTime.GetNanoSeconds();
	m_totalTransmitTimeNanoSec += m_lastUpdateTimeNanoSec;
	recordDeviceState(BUSY, m_lastUpdateTimeNanoSec);
	// sanjeev, jun 18. critical see p2p original code , line 240
	NS_LOG_LOGIC(
			"Schedule TransmitCompleteEvent in " << txCompleteTime+ m_tInterframeGap << ", this includes m_additionalTimeFromDeviceState [" << m_additionalTimeFromDeviceState << "] + m_additionalTimeFromNodeFabric [" << m_additionalTimeFromNodeFabric << "]");
	Simulator::Schedule(txCompleteTime + m_tInterframeGap,
			&ssTOSPointToPointNetDevice::TransmitComplete, this);

	bool result = true;
	if (simulationRunProperties::enableLinkEnergyManagement) {
		// ENABLE_LINK_ENERGY_MANAGEMENT
		// sanjeev, jun 18. critical see p2p original code , line 240
		// packet will reach other end only after this delay...
		result = m_channelSS->TransmitStart(p, this, txCompleteTime);
	} else {
		result = m_channel->TransmitStart(p, this, txCompleteTime);
	}

	if (result == false) {
		m_phyTxDropTrace(p);
	}

	return result;
}

/******************************************************/
// over ride the method here, cannot call base class method since I am re-writing this function
void ssTOSPointToPointNetDevice::TransmitComplete(void) {
	NS_LOG_FUNCTION(this);

	NS_LOG_LOGIC(
			this << " Ipv4Address ["<< m_address << "] Node [" << m_nodeId << "] m_txMachineStateSS=" << m_txMachineStateSS);
	// Jun 24
	UpdateDeviceQueueData();
	//
	// This function is called to when we're all done transmitting a packet.
	// We try and pull another packet off of the transmit queue.  If the queue
	// is empty, we are done, otherwise we need to start transmitting the
	// next packet.
	//
	NS_ASSERT_MSG(m_txMachineStateSS == BUSY, "Must be BUSY if transmitting");
	m_txMachineStateSS = READY;
	m_lastUpdateTimeNanoSec = Simulator::Now().GetNanoSeconds();
	HandleDeviceStateChange(READY, BUSY, true);		// sanjeev1

	NS_ASSERT_MSG(m_currentPkt != 0,
			"ssTOSPointToPointNetDevice::TransmitComplete(): m_currentPkt zero");

	m_phyTxEndTrace (m_currentPkt);
	m_currentPkt = 0;

	Ptr<NetDeviceQueue> txq;
	if (m_queueInterface) {
		txq = m_queueInterface->GetTxQueue(0);
	}

	Ptr<QueueItem> item = m_queue->Dequeue();
	if (item == 0) {
		NS_LOG_LOGIC("No pending packets in device queue after tx complete");
		if (txq) {
			txq->Wake();
		}
		return;
	}
	// sanjeev Jun 18. always reset these variables after each packet transmission,
	// if required let other methods set it for next packet txn condition
	m_additionalTimeFromNodeFabric = Seconds(0.);
	m_additionalTimeFromDeviceState = Seconds(0.);

	//
	// Got another packet off of the queue, so start the transmit process again.
	// If the queue was stopped, start it again. Note that we cannot wake the upper
	// layers because otherwise a packet is sent to the device while the machine
	// state is busy, thus causing the assert in TransmitStart to fail.
	//
	if (txq && txq->IsStopped()) {
		txq->Start();
	}
	Ptr<Packet> p = item->GetPacket();
	m_snifferTrace(p);
	m_promiscSnifferTrace(p);
	TransmitStart(p);

}

/******************************************************/
void ssTOSPointToPointNetDevice::UpdateDeviceState() {
	NS_LOG_FUNCTION(this << m_address);
	if (m_deviceStateUpdateEvent.IsRunning()) // CHECK-JUN25
		m_deviceStateUpdateEvent.Cancel();

	t_now = Simulator::Now().GetNanoSeconds();
	//
	// for this link - check the duration and put them into diff state...
	//
	t_duration = t_now - m_lastUpdateTimeNanoSec;

	// start monitoring the device states...
	switch (m_txMachineStateSS) {
	//
	case READY:
		m_txMachineStateSS = SLEEP;
		NS_LOG_LOGIC(
				"Node [" << m_nodeId << "], Device moving READY --> SLEEP [" << t_duration << "]");
		m_totalIdleTimeNanoSec += t_duration;
		recordDeviceState(READY, t_duration);
		HandleDeviceStateChange(SLEEP, READY, false);
		// REDUCE_SLEEP_EVENTS (syntax corrected, Jun 18)
		// how long to sleep? sleep long long long time, till simulation ends if no packet arrives...
		m_deviceStateUpdateInterval = Seconds(simulationRunProperties::simStop)
				- Simulator::Now()
				- Time::FromDouble(WAKEUP_BEFORE_SIM_END_NANOSECONDS, Time::NS);
		m_countSleepState++;
		break;
	case BUSY:
		NS_LOG_LOGIC(
				"Node [" << m_nodeId << "], Device moving BUSY --> BUSY [" << t_duration << "]");
		// no need to record time, it is already done in rcv/send logic, after busy time --> transmitComplete will put it to ready state,
		// m_txMachineStateSS = READY;
		break;
	case SLEEP:
		NS_LOG_LOGIC(
				"Node [" << m_nodeId << "], Device moving SLEEP --> SLEEP [" << t_duration << "]");
		m_totalSleepTimeNanoSec += t_duration;
		recordDeviceState(SLEEP, t_duration);
		break;
	default:
		NS_LOG_LOGIC(
				"Node [" << m_nodeId << "], Device *ERROR* default --> READY [" << t_duration << "]");
		break;
	}
	m_lastUpdateTimeNanoSec = t_now;
	// sanjeev. my debug statement... all ok, nothing to worry.
	NS_LOG_LOGIC(
			"ssTOSPointToPointNetDevice::updateDeviceState " << Simulator::Now() << " s " << m_totalSleepTimeNanoSec << " i " << m_totalIdleTimeNanoSec << " tx " << m_totalTransmitTimeNanoSec << " T " << m_totalSleepTimeNanoSec + m_totalIdleTimeNanoSec + m_totalTransmitTimeNanoSec << " ID " << m_nodeId);
	// if simulation has stopped, then STOP...
	if (!Simulator::IsFinished())
		m_deviceStateUpdateEvent = Simulator::Schedule(
				m_deviceStateUpdateInterval,
				&ssTOSPointToPointNetDevice::UpdateDeviceState, this);
}

/*
 * Jun 24
 */
void ssTOSPointToPointNetDevice::UpdateDeviceQueueData(void) {
	NS_LOG_FUNCTION(this << m_address);
	// Jun 24 Max Q length observed in real time
	int qNow = m_queue->GetNPackets();
	if (m_deviceData->m_DeviceMaxQueueDepth < qNow)
		m_deviceData->m_DeviceMaxQueueDepth = qNow;
	// for AvgQDepthCalc. integral of time..computed only during send time
	// i.e when a customer(packet) arrives, capture queue data
	uint64_t timeDiff = Simulator::Now().GetNanoSeconds()
			- m_deviceData->m_lastQDepthTimeNanoSec;
	m_deviceData->m_lastQDepthTimeNanoSec = Simulator::Now().GetNanoSeconds();
	m_deviceData->m_DeviceTotalObservedQueueDepthIntegralns +=
			(qNow * timeDiff);
	// for device through put calc, capture 1st packet and last packet...
	if (m_deviceQMetrics.m_firstPacketArriveTime_ns == 0)
		m_deviceQMetrics.m_firstPacketArriveTime_ns =
				Simulator::Now().GetNanoSeconds();
	m_deviceQMetrics.m_lastPacketDepartTime_ns =
			Simulator::Now().GetNanoSeconds();
}

/*
 * Jun 24
 */
void ssTOSPointToPointNetDevice::UpdatePacketMetricsData(
		const Ptr<const Packet> p) {
	NS_LOG_FUNCTION(this << m_address);
	m_deviceQMetrics.m_totalProcessingTime_ns += (p->timePacketLeftLink_ns
			- p->timePacketProcessAtLink_ns);
	m_deviceQMetrics.m_totalWaitingTime_ns += (p->timePacketProcessAtLink_ns
			- p->timePacketArrivedLink_ns);
	NS_LOG_LOGIC(
			this << " Ipv4Address ["<< m_address << "] Node [" << m_nodeId << "], packetId [" << p->packet_id << "] Arrived [" << p->timePacketArrivedLink_ns << "] processed [" << p->timePacketProcessAtLink_ns << "] LeftAt [" << p->timePacketLeftLink_ns << "]");

}

}   //namespace
