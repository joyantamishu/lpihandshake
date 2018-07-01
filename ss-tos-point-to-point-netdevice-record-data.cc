/*
 * ssTOSPointToPointNetDevice-record-data.cc
 *
 *  Created on: Jul 25, 2016
 *      Author: sanjeev
 */
#include "ns3/ipv4.h"
#include "ns3/log.h"
#include "ns3/queue.h"
#include <fstream>

#include "ss-base-topology-simulation-variables.h"
#include "ss-device-energy-model.h"
#include "ss-energy-source.h"
#include "ss-node.h"
#include "ss-base-topology.h"
#include "ss-log-file-flags.h"
#include "ss-tos-point-to-point-netdevice.h"
/*
 * we collect device transition statistics...
 */

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("ssTOSPointToPointNetDeviceR");

/******************************************************/
// Give this work done details to the model...
DeviceData* ssTOSPointToPointNetDevice::getDeviceData() {
	//
	NS_LOG_FUNCTION(this);
	NS_ASSERT_MSG(m_deviceData != NULL, "m_deviceData is NULL");
	m_deviceData->m_totalIdleTimeNanoSec = m_totalIdleTimeNanoSec;
	m_deviceData->m_totalTransmitTimeNanoSec = m_totalTransmitTimeNanoSec;
	m_deviceData->m_totalSleepTimeNanoSec = m_totalSleepTimeNanoSec;
	m_deviceData->m_totalDeviceTimeNanoSec = m_totalIdleTimeNanoSec
			+ m_totalTransmitTimeNanoSec + m_totalSleepTimeNanoSec;
	m_deviceData->m_sendPacketCount = m_sendPacketcount;
	m_deviceData->m_rcvPacketCount = m_rcvPacketcount;
	m_deviceData->m_packetCount = m_sendPacketcount + m_rcvPacketcount;
	m_deviceData->m_countSleepState = m_countSleepState;
	m_deviceData->m_DeviceState = m_txMachineStateSS;
	m_deviceData->m_isDeviceUp = IsDeviceUp();
	// Jun 24...max Q lenght is computed in realtime, not at observed interval ...Apr 17,May 1, sanjeev
	m_deviceData->m_DeviceObservedQueueDepth = 0;
	return m_deviceData;
}

/******************************************************/
LinkData* ssTOSPointToPointNetDevice::getLinkData() {
	//
	NS_LOG_FUNCTION(this);

	if (!simulationRunProperties::enableLinkEnergyManagement) {
// !ENABLE_LINK_ENERGY_MANAGEMENT
		m_linkData->m_totalTransmitTimeNanoSec = 0;
		m_linkData->m_totalSleepTimeNanoSec = 0;
		m_linkData->m_totalIdleTimeNanoSec = 0;
		m_linkData->m_totalTransmitTimeNanoSec = 0;
		m_linkData->m_packetCount = 0;
		m_linkData->m_countSleepState = 0;
		m_linkData->IsLinkUp = false;
		return m_linkData;
	}

	NS_ASSERT_MSG(m_channelSS != NULL, "m_channelSS is NULL");

	LinkData* l0 = (LinkData *) m_channelSS->getLinkData(0);
	LinkData* l1 = (LinkData *) m_channelSS->getLinkData(1);
	m_linkData->m_totalTransmitTimeNanoSec = l0->m_totalTransmitTimeNanoSec
			+ l1->m_totalTransmitTimeNanoSec;
	m_linkData->m_totalSleepTimeNanoSec = l0->m_totalSleepTimeNanoSec
			+ l1->m_totalSleepTimeNanoSec;
	m_linkData->m_totalIdleTimeNanoSec = l0->m_totalIdleTimeNanoSec
			+ l1->m_totalIdleTimeNanoSec;
	m_linkData->m_totalTransmitTimeNanoSec = l0->m_totalTransmitTimeNanoSec
			+ l1->m_totalTransmitTimeNanoSec;
	m_linkData->m_packetCount = l0->m_packetCount + l1->m_packetCount;
	m_linkData->m_countSleepState = l0->m_countSleepState
			+ l1->m_countSleepState;

	return m_linkData;
}

/******************************************************/
void ssTOSPointToPointNetDevice::setCaptureDeviceStateBufferSize(
		int bufferSize) {
	NS_LOG_FUNCTION(this);
	m_BufferSize = bufferSize;
	m_currentBufferPoint = 0;
	m_deviceStateBuffer = (int *) malloc(sizeof(int) * m_BufferSize);
	m_deviceDurationBuffer = (uint64_t *) malloc(
			sizeof(uint64_t) * m_BufferSize);
	NS_ASSERT_MSG(m_deviceStateBuffer != NULL,
			"m_deviceStateBuffer is NULL (malloc failed)");
	NS_ASSERT_MSG(m_deviceDurationBuffer != NULL,
			"m_deviceDurationBuffer is NULL (malloc failed)");

	for (int k = 0; k < m_BufferSize;
			m_deviceDurationBuffer[k] = 0, m_deviceStateBuffer[k++] = 0)
		; // initialize buffer....
// initialize the link statistics buffer this is the buffer pointer u return to higher levels...
	NS_ASSERT(m_deviceStatisticsBuffer != NULL);
	m_deviceStatisticsBuffer->m_currentBufferPointer = m_currentBufferPoint;
	m_deviceStatisticsBuffer->m_maxBufferSize = m_BufferSize;
	m_deviceStatisticsBuffer->m_durationBuffer = m_deviceDurationBuffer;
	m_deviceStatisticsBuffer->m_stateBuffer = m_deviceStateBuffer;
	m_deviceStatisticsBuffer->m_deviceStateUpdateIntervalNanoSec =
			m_deviceStateUpdateInterval.GetNanoSeconds();
}

/******************************************************/
void ssTOSPointToPointNetDevice::recordDeviceState(int newState,
		uint64_t duration) {
	NS_LOG_FUNCTION(this);

// this is rotating buffer, so reset the pointer to start index, if exceeds the buffer limit..
	if (m_currentBufferPoint > m_BufferSize) {
		m_currentBufferPoint = 0;
	}
// remember this state transition happened now, so record the index for current state in previous time slot, for energy calculation (later)
	t_durationArrayIndex = (
			m_currentBufferPoint == 0 ? 0 : m_currentBufferPoint - 1);
	m_deviceStateBuffer[m_currentBufferPoint] = newState;
	m_deviceDurationBuffer[t_durationArrayIndex] = duration;
	m_currentBufferPoint++;
}

/******************************************************/
DeviceStatisticsBuffers* ssTOSPointToPointNetDevice::getDeviceStatisticsBuffer() {
	NS_LOG_FUNCTION(this);
	return m_deviceStatisticsBuffer;
}

/******************************************************/
bool ssTOSPointToPointNetDevice::IsDeviceUp(void) {
	NS_LOG_FUNCTION(this);
	if ((m_txMachineStateSS == READY) || (m_txMachineStateSS == BUSY))
		return true;
	return false;
}

/******************************************************/
void ssTOSPointToPointNetDevice::SetDeviceEnergyModel(
		Ptr<ssDeviceEnergyModel> p) {
	NS_LOG_FUNCTION(this);
	m_pEnergyModel = p;
}

/******************************************************/
void ssTOSPointToPointNetDevice::WriteStateDurationEnergyGranular(
		std::ostream *fpState, std::ostream *fpEnergy, std::ostream *fpDuration,
		Ptr<BasicEnergySource> pE) {
	NS_LOG_FUNCTION(this);
// get voltages from the source..
	NS_ASSERT_MSG(fpState != NULL, "Failed to get correct FILE* fpState");
	NS_ASSERT_MSG(fpDuration != NULL, "Failed to get correct FILE* fpDuration");
	NS_ASSERT_MSG(fpEnergy != NULL, "Failed to get correct FILE* fpEnergy");

	// NO EnergyModel or NO EnergySource set? then nothing to write.!!
	if ((m_pEnergyModel == NULL) || (pE == NULL)) {
		*fpState << "--NOT ENABLED--\n";
		*fpDuration << "---NOT ENABLED--\n";
		*fpEnergy << "--NOT ENABLED--\n";
		return;
	}

	int deviceId = GetIfIndex();
	double supplyVoltage = pE->GetSupplyVoltage();
	double currentA = m_pEnergyModel->SimpleDeviceEnergyModel::GetCurrentA();
	double lowPowerStateVoltage = supplyVoltage;
	double currentL = currentA;
// enable metrics if energy management is ON
	if (m_enableDeviceEnergyManagement) {
		lowPowerStateVoltage =
				DynamicCast<ssEnergySource>(pE)->GetLowPowerStateVoltage();
		currentL = m_pEnergyModel->GetCurrentL();
	}

	int i;
	double energyConsumed = 0;
	int* dState;
	uint64_t* dDuration;
	int dmaxBufferSize;
	int dcurrentPointer;
// now decide the starting point....
	int startPointer;
	int index;

// read the buffers from the device
	Ptr<ssPointToPointChannel> m_channel1 = DynamicCast<ssPointToPointChannel>(
			m_channel);
	dState = m_deviceStateBuffer;
	dDuration = m_deviceDurationBuffer;

	dmaxBufferSize = m_BufferSize;
	dcurrentPointer = m_currentBufferPoint;
// now decide the starting point....
	startPointer = 0;
	index = 0;
// reached end of circular buffer OR nothing is written in next slot (i.e circular buffer not circular!!written)
// be careful about out of bounds, seg voilation errors
	if (dcurrentPointer > dmaxBufferSize)
		startPointer = 0;
	else if (dDuration[dcurrentPointer] == 0)
		startPointer = 0;
	else
		startPointer = dcurrentPointer;

// start writing state metrics for this link.
	*fpState << "srcNode[" << m_nodeId << "]device[" << deviceId << "]address["
			<< m_address << "]::" << "(cp=" << dcurrentPointer << ")";
	for (i = startPointer; i < dmaxBufferSize + startPointer; i++) {
		index = (i >= dmaxBufferSize ? i % dmaxBufferSize : i);
		*fpState << "," << dState[index];
	} // end of writing state metrics for this device..

// start writing energy metrics for this device..
	*fpEnergy << "srcNode[" << m_nodeId << "]device[" << deviceId << "]address["
			<< m_address << "]::" << "(cp=" << dcurrentPointer << ")";

	for (i = startPointer; i < dmaxBufferSize + startPointer; i++) {
		index = (i >= dmaxBufferSize ? i % dmaxBufferSize : i);
		switch (dState[index]) {
		// compute energy depending on link state...
		case INIT:
			energyConsumed = dDuration[index] * currentL * supplyVoltage;
			break; //
		case SLEEP:
			energyConsumed = dDuration[index] * currentL * lowPowerStateVoltage;
			break;
		case READY:
			energyConsumed = dDuration[index] * currentL * supplyVoltage;
			break;
		case BUSY:
			energyConsumed = dDuration[index] * currentA * supplyVoltage;
			break;
		default:
			energyConsumed = -999999.99; //error condition
			break;
		} // switch
		*fpEnergy << "," << (energyConsumed / NanoSec_to_Secs);
	} // end of writing energy metrics for this link...

// start writing state metrics for this link.
	*fpDuration << "srcNode[" << m_nodeId << "]device[" << deviceId
			<< "]address[" << m_address << "]::" << "(cp=" << dcurrentPointer
			<< ")";

	for (i = startPointer; i < dmaxBufferSize + startPointer; i++) {
		index = (i >= dmaxBufferSize ? i % dmaxBufferSize : i);
		*fpDuration << "," << dDuration[index];
	} // end of writing state metrics for this link..

	*fpState << "\n";
	*fpEnergy << "\n";
	*fpDuration << "\n";

	// now write the granular details for the P2PChannel Links

	if (!simulationRunProperties::enableLinkEnergyManagement) {
		// ! ENABLE_LINK_ENERGY_MANAGEMENT
		*fpState << "--LinkState--NOT ENABLED--\n";
		*fpEnergy << "--LinkEnergy--NOT ENABLED--\n";
		*fpDuration << "--LinkDuration--NOT ENABLED--\n";
		return;
	}

	deviceId = GetIfIndex();
	supplyVoltage = pE->GetSupplyVoltage();
	currentA = m_pEnergyModel->GetCurrentA();
	lowPowerStateVoltage = supplyVoltage;
	currentL = currentA;

// enable metrics if energy management is ON
	if (m_enableDeviceEnergyManagement) {
		lowPowerStateVoltage =
				DynamicCast<ssEnergySource>(pE)->GetLowPowerStateVoltage();
		currentL = m_pEnergyModel->GetCurrentL();
	}

	int N_DEVICES = 2;
	energyConsumed = 0;
	LinkStatisticsBuffer* lsBuffer;
	int* linkState;
	uint64_t* linkDuration;
	int nlink_maxBufferSize;
	int nlink_currentPointer;
	int nodeId = m_node->GetId();

	for (int NoOfLinks = 0; NoOfLinks < N_DEVICES; NoOfLinks++) {
		// read the buffers from the link/channel
		lsBuffer = m_channelSS->getLinkStatisticsBuffer(NoOfLinks);
		linkState = lsBuffer->m_stateBuffer;
		linkDuration = lsBuffer->m_durationBuffer;
		nlink_maxBufferSize = lsBuffer->m_maxBufferSize;
		nlink_currentPointer = lsBuffer->m_currentBufferPointer;
		// now decide the starting point....
		startPointer = 0;
		index = 0;
		// reached end of circular buffer OR nothing is written in next slot (i.e circular buffer not circular!!written)
		// be careful about out of bounds, seg voilation errors
		if (nlink_currentPointer > nlink_maxBufferSize)
			startPointer = 0;
		else if (linkDuration[nlink_currentPointer] == 0)
			startPointer = 0;
		else
			startPointer = nlink_currentPointer;

		// start writing state metrics for this link.
		*fpState << "srcNode[" << nodeId << "]device[" << deviceId << "]link["
				<< NoOfLinks << "]address[" << m_address << "]::"
				<< lsBuffer->m_srcNodeId << "-->" << lsBuffer->m_dstNodeId
				<< "(cp=" << nlink_currentPointer << ")";
		for (i = startPointer; i < nlink_maxBufferSize + startPointer; i++) {
			index = (i >= nlink_maxBufferSize ? i % nlink_maxBufferSize : i);
			*fpState << "," << linkState[index];
		} // end of writing state metrics for this link..

		// start writing energy metrics for this link..
		*fpEnergy << "srcNode[" << nodeId << "]device[" << deviceId << "]link["
				<< NoOfLinks << "]address[" << m_address << "]::"
				<< lsBuffer->m_srcNodeId << "-->" << lsBuffer->m_dstNodeId
				<< "(cp=" << nlink_currentPointer << ")";

		for (i = startPointer; i < nlink_maxBufferSize + startPointer; i++) {
			index = (i >= nlink_maxBufferSize ? i % nlink_maxBufferSize : i);
			switch (linkState[index]) {
			// compute energy depending on link state...
			case ssPointToPointChannel::INITIALIZING:
				energyConsumed = 0;
				break;
			case ssPointToPointChannel::SLEEPING:
				energyConsumed = linkDuration[index] * currentL
						* lowPowerStateVoltage;
				break;
			case ssPointToPointChannel::IDLE:
				energyConsumed = linkDuration[index] * currentL * supplyVoltage;
				break;
			case ssPointToPointChannel::TRANSMITTING:
				energyConsumed = linkDuration[index] * currentA * supplyVoltage;
				break;
			case ssPointToPointChannel::PROPAGATING:
				energyConsumed = linkDuration[index] * currentA * supplyVoltage;
				break;
			default:
				energyConsumed = -999999.99; //error condition
				break;
			} // switch
			*fpEnergy << "," << (energyConsumed / NanoSec_to_Secs);
		} // end of writing energy metrics for this link...

		// start writing state metrics for this link.
		*fpDuration << "srcNode[" << nodeId << "]device[" << deviceId
				<< "]link[" << NoOfLinks << "]address[" << m_address << "]::"
				<< lsBuffer->m_srcNodeId << "-->" << lsBuffer->m_dstNodeId
				<< "(cp=" << nlink_currentPointer << ")";

		for (i = startPointer; i < nlink_maxBufferSize + startPointer; i++) {
			index = (i >= nlink_maxBufferSize ? i % nlink_maxBufferSize : i);
			*fpDuration << "," << linkDuration[index];
		} // end of writing state metrics for this link..

		*fpState << "\n";
		*fpEnergy << "\n";
		*fpDuration << "\n";

	} // end for N links
}
//
void ssTOSPointToPointNetDevice::WriteStatisticsToFile(void) {
	NS_LOG_FUNCTION(this);
	if (WRITE_DEVICE_ENERGY_TO_FILE) {
		Ptr<Queue> queue = GetQueue();
		// sanjeev, Jun 24
		double t_total_response_time_ns = m_totalTransmitTimeNanoSec;
		double t_total_requests = m_sendPacketcount;
		double t_total_system_time_ns = m_totalTransmitTimeNanoSec
				+ m_totalIdleTimeNanoSec + m_totalSleepTimeNanoSec;
		double deviceTime = (t_total_system_time_ns) / NANOSEC_TO_SECS;
		double avg_processing_time_ns = 0;
		// aka avg queue time, incl service time + wait time, *avg* processing time per request.
		if (t_total_response_time_ns != 0.0 && t_total_requests != 0.0)
			avg_processing_time_ns = t_total_response_time_ns
					/ t_total_requests;
		double avg_arrival_rate_ns = 0;
		// at what rate is the requests coming in...
		if (t_total_system_time_ns != 0 && t_total_requests != 0)
			avg_arrival_rate_ns = t_total_requests / t_total_system_time_ns;
		double deviceUtilzation = 100 * m_totalTransmitTimeNanoSec
				/ (t_total_system_time_ns);
		// additional log..sanjeev...Jun 24
		BaseTopology::fpDeviceEnergy << "TOSNetDevice,Node," << m_nodeId
				<< ",Device," << GetIfIndex() << ",totalDeviceTime[s],"
				<< deviceTime << ",totalTransmitTime[ns],"
				<< m_totalTransmitTimeNanoSec << ",totalIdleTime[ns],"
				<< m_totalIdleTimeNanoSec << ",totalSleepTime[ns],"
				<< m_totalSleepTimeNanoSec
				// Jun 24 Max Q length observed in real time
				<< ",FabricAdditionalTime, "
				<< m_deviceData->t_additionalTimeFromNodeFabricNanoSec
				<< ",x,x,Type," << DynamicCast<ssNode>(GetNode())->nodeName
				<< ",RcvPacket," << m_rcvPacketcount << ",SendPacket,"
				<< m_sendPacketcount << ",SleepCount," << m_countSleepState
				<< "\n";
		BaseTopology::fpDeviceEnergy << "TOSNetDevice,Node," << m_nodeId
				<< ",DeviceQ," << GetIfIndex() << ",DeviceIpV4Address,"
				<< m_address << ",DroppedPackets,"
				<< queue->GetTotalDroppedPackets() << ",AvgArrivalRate[ns],"
				<< avg_arrival_rate_ns << ",AvgProcessingTime[ns],"
				<< avg_processing_time_ns << ",deviceUtilzation%,"
				<< deviceUtilzation << ",MaxQDepth,"
				<< m_deviceData->m_DeviceMaxQueueDepth << ",Type,"
				<< DynamicCast<ssNode>(GetNode())->nodeName
				<< ",DeviceStateAdditionalTime,"
				<< m_deviceData->t_additionalTimeFromDeviceStateNanoSec
				<< ",debug,"
				<< m_deviceData->m_DeviceTotalObservedQueueDepthIntegralns
				<< "\n";
		// Jun 26..device queue metrics.
		double avgResponseTimeCalc = 0;
		double avgThoughputCalc = 0;
		double avgWaitingTime_ns = 0;
		double avgResponseTime_ns = 0;
		double avgProcessingTime_ns = 0;
		double avgThoughput_as_packets = 0;
		double avgUtilizationCalc = 100
				* m_deviceQMetrics.m_totalProcessingTime_ns
				/ (simulationRunProperties::simStop * NANOSEC_TO_SECS);
		double avgQueueDepthCalc = double(
				m_deviceData->m_DeviceTotalObservedQueueDepthIntegralns)
				/ t_total_system_time_ns;
		if (t_total_requests != 0) {
			avgThoughput_as_packets = (NANOSEC_TO_SECS * t_total_requests)
					/ (m_deviceQMetrics.m_lastPacketDepartTime_ns
							- m_deviceQMetrics.m_firstPacketArriveTime_ns);
			avgThoughputCalc = avgThoughput_as_packets * 8
					* simulationRunProperties::packetSize;
			avgResponseTimeCalc = avgQueueDepthCalc / avgThoughput_as_packets;
			avgProcessingTime_ns = m_deviceQMetrics.m_totalProcessingTime_ns
					/ (t_total_requests);
			avgWaitingTime_ns = m_deviceQMetrics.m_totalWaitingTime_ns
					/ (t_total_requests);
			avgResponseTime_ns = avgProcessingTime_ns + avgWaitingTime_ns;
		}
		BaseTopology::fpDeviceEnergy << "TOSNetDevice,Node," << m_nodeId
				<< ",DeviceQ," << GetIfIndex() << ",AvgProcessingTime,"
				<< avgProcessingTime_ns << "ns,AvgWaitingTime,"
				<< avgWaitingTime_ns << "ns,AvgResponseTime,"
				<< avgResponseTime_ns << "ns,AvgResponseTimeCalc,"
				<< avgResponseTimeCalc << ",AvgThoughputCalc,"
				<< avgThoughputCalc / 1000 << "Mbps,AvgQueueDepthCalc,"
				<< avgThoughputCalc * avgResponseTime_ns
				<< ",AvgQueueDepthCalc1," << avgQueueDepthCalc
				<< ",AvgUtilizationCalc," << avgUtilizationCalc << "\n";

	}
}

}   //namespace
