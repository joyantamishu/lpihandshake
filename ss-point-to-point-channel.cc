/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007, 2008 University of Washington
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
 */

#include "ns3/trace-source-accessor.h"
#include "ns3/packet.h"
#include "ns3/log.h"
#include <stdio.h>
#include "ns3/point-to-point-net-device.h"
#include "ss-base-topology.h"
#include "ss-point-to-point-channel.h"
#include "ss-base-topology-simulation-variables.h"
#include "ss-log-file-flags.h"

/*
 * we collect link transition statistics...
 */
// p2p->SetChannelAttribute("Delay", StringValue("2ms"));
#define DefaultBufferSize 						20
#define DefaultLinkStateUpdateIntervalMicroSec	50  				// frequency of checking link state., a.k.a runwayTime
//************************************************************Madhurima Changed on May 4*****************************************************************
#define DefaultLinkWakeUpTimeMicroSec			 0.1				// Syntax Corrected, Jun 18   time penalty for waking up a sleeping link (to start transmitting)
//*******************************************************************************************************************************************************

//
namespace ns3 {

NS_LOG_COMPONENT_DEFINE("ssPointToPointChannel");

NS_OBJECT_ENSURE_REGISTERED(ssPointToPointChannel);

TypeId ssPointToPointChannel::GetTypeId(void) {
	static TypeId tid = TypeId("ns3::ssPointToPointChannel").SetParent<
			PointToPointChannel>().SetGroupName("PointToPoint").AddConstructor<
			ssPointToPointChannel>();
	return tid;
}

//
// By default, you get a channel that 
// has an "infitely" fast transmission speed and zero delay.
ssPointToPointChannel::ssPointToPointChannel() :
		PointToPointChannel() {
	NS_LOG_FUNCTION(this);
	NS_ASSERT(simulationRunProperties::enableLinkEnergyManagement);
	t_now = 0;
	t_duration = 0;
	t_remainingDuration = 0;
	t_transmitDuration = 0;
	t_durationArrayIndex = 0;
	m_DoDisposed = false;
}
/*
 *
 */

ssPointToPointChannel::~ssPointToPointChannel() {
	NS_LOG_FUNCTION(this);
}

void ssPointToPointChannel::WriteStatisticsToFile(void) {
	NS_LOG_FUNCTION(this);

	// already written by respective links
	// nothing specific to this class to write
}

void ssPointToPointChannel::DoDispose(
		const Ptr<PointToPointNetDevice> &device) {
	NS_LOG_FUNCTION(this);
	WriteStatisticsToFile();
	if (!m_DoDisposed) {
		// show display to 'send' side link only, so we dont display same twice.
		// sanjeev mar 18
		if (device == m_link[0].m_src)
			m_link[0].DoDispose();
		if (device == m_link[1].m_src)
			m_link[1].DoDispose();
	}
	m_DoDisposed = true;
}

void ssPointToPointChannel::Attach(Ptr<PointToPointNetDevice> device) {
	NS_LOG_FUNCTION(this << device);
	NS_ASSERT_MSG(m_nDevices < N_DEVICES, "Only two devices permitted");
	NS_ASSERT(device != 0);

	m_link[m_nDevices].m_lastUpdateTimeNanoSec =
			Simulator::Now().GetNanoSeconds();
	m_link[m_nDevices].m_srcNodeId = device->GetNode()->GetId();
	m_link[m_nDevices].recordLinkState(INITIALIZING, 0);

	m_link[m_nDevices++].m_src = device;
//
// If we have both devices connected to the channel, then finish introducing
// the two halves and set the links to IDLE.
//
	if (m_nDevices == N_DEVICES) {
		m_link[0].m_dst = m_link[1].m_src;
		m_link[1].m_dst = m_link[0].m_src;
		m_link[0].m_state = IDLE;
		m_link[1].m_state = IDLE;
		m_link[0].m_dstNodeId = m_link[1].m_srcNodeId;
		m_link[1].m_dstNodeId = m_link[0].m_srcNodeId;
	}
}

void ssPointToPointChannel::DoInitialize() {
	NS_LOG_FUNCTION(this);
	NS_ASSERT_MSG(m_nDevices == N_DEVICES, "Devices not attached");
	m_link[0].startLink();
	m_link[1].startLink();
}
// this is the modified method based on link wake up time (sleep->txn)
bool ssPointToPointChannel::TransmitStart(Ptr<Packet> p,
		Ptr<PointToPointNetDevice> src, Time txTime) {
	NS_LOG_FUNCTION(this << p << src);

	NS_ASSERT(m_link[0].m_state != INITIALIZING);
	NS_ASSERT(m_link[1].m_state != INITIALIZING);

	uint32_t wire = src == m_link[0].m_src ? 0 : 1;

	// cancel monitoring this link till end of this transmission...
	if (m_link[wire].m_linkStateUpdateEvent.IsRunning()) // JUN 25 CHECK
		m_link[wire].m_linkStateUpdateEvent.Cancel();
	// needed to reset link monitoring time again, since we set to diff value at Update-->Ready-->Sleep
	//Sanjeev Jun 18, Syntax Corrected
	m_link[wire].m_linkStateUpdateInterval = Time::FromDouble(
	DefaultLinkStateUpdateIntervalMicroSec, Time::US);

	t_now = Simulator::Now().GetNanoSeconds();
	t_additionalTime = Seconds(0.0);
	// this duration when transmit started (from previous link sample time)
	t_duration = t_now - m_link[wire].m_lastUpdateTimeNanoSec;

	switch (m_link[wire].m_state) {
	// if link was earlier idle, put it to sleep...
	case IDLE:
		m_link[wire].linkData.m_totalIdleTimeNanoSec += t_duration;
		//m_link[wire].linkData.m_TransmitEndTime = Simulator::Now() + txTime
		//		+ m_delay;
		m_link[wire].recordLinkState(IDLE, t_duration);
		NS_LOG_LOGIC(
				"m_link [" << wire << "] moving IDLE --> TXN, duration=" << t_duration);
		break;
	case SLEEPING:
		m_link[wire].linkData.m_totalSleepTimeNanoSec += t_duration;
		//m_link[wire].linkData.m_TransmitEndTime = Simulator::Now() + txTime
		//		+ m_delay + m_link[wire].m_linkWakeUpTimeDelay;
		t_additionalTime = m_link[wire].m_linkWakeUpTimeDelay;
		m_link[wire].recordLinkState(SLEEPING, t_duration);
		NS_LOG_LOGIC(
				"m_link [" << wire << "] moving SLEEP --> TXN, duration=" << t_duration);
		break;
	case TRANSMITTING:
		// we will calculate the transmit time in multiple parts below...
		// this packet will be transmitted after previous packet, so time is longer...
		//m_link[wire].linkData.m_TransmitEndTime += txTime + m_delay;
		NS_LOG_LOGIC(
				"m_link [" << wire << "] moving TXN --> TXN, duration=" << t_duration);
		break;
	default:
		NS_LOG_LOGIC(
				"m_link [" << wire << "] ERROR:: moving DeFauLT --> TXN, duration=" << 9999);
		// m_link[wire].linkData.m_TransmitEndTime = Seconds(0.0);
		break;
	} // switch

	// total time to do this packet transmission
	t_transmitDuration = txTime.GetNanoSeconds() + m_delay.GetNanoSeconds()
			+ t_additionalTime.GetNanoSeconds();

	m_link[wire].recordLinkState(TRANSMITTING, t_transmitDuration);
	m_link[wire].linkData.m_totalTransmitTimeNanoSec += t_transmitDuration;

	m_link[wire].m_state = TRANSMITTING;
	m_link[wire].linkData.m_packetCount++;
	m_link[wire].linkData.m_linkState = m_link[wire].m_state;

// Call the tx anim callback on the net device
	m_txrxPointToPoint(p, src, m_link[wire].m_dst, txTime,
			txTime + m_delay + t_additionalTime);

	// Let the link monitor after this transmission complete
	m_link[wire].m_linkStateUpdateEvent = Simulator::Schedule(
			txTime + m_delay + t_additionalTime,
			&ssPointToPointChannel::Link::updateLinkState, &m_link[wire]);
	NS_LOG_LOGIC(
			"ssPointToPointChannel::TransmitStart Node [" << m_link[wire].m_dstNodeId << "] will receive at [" << txTime + m_delay + t_additionalTime << "]");
	Simulator::ScheduleWithContext(m_link[wire].m_dstNodeId,
			txTime + m_delay + t_additionalTime,
			&PointToPointNetDevice::Receive, m_link[wire].m_dst, p);

	return true;
}

uint32_t ssPointToPointChannel::GetNDevices(void) const {
	int N = PointToPointChannel::GetNDevices();
	NS_LOG_FUNCTION(this << N);
	return N;
}

Ptr<PointToPointNetDevice> ssPointToPointChannel::GetPointToPointDevice(
		uint32_t i) const {
	NS_ASSERT(i < 2);
	return m_link[i].m_src;
}

Ptr<PointToPointNetDevice> ssPointToPointChannel::GetSource(uint32_t i) const {
	return m_link[i].m_src;
}

Ptr<PointToPointNetDevice> ssPointToPointChannel::GetDestination(
		uint32_t i) const {
	return m_link[i].m_dst;
}

LinkData* ssPointToPointChannel::getLinkData(int link) {
	NS_ASSERT_MSG(link < N_DEVICES,
			"ssPointToPointChannel::getLinkData -- Only two devices permitted");
	return &m_link[link].linkData;
}

LinkStatisticsBuffer* ssPointToPointChannel::getLinkStatisticsBuffer(int link) {
	NS_LOG_FUNCTION(this);
	NS_ASSERT_MSG(link < N_DEVICES,
			"PointToPointChannel::getLinkStatisticsBuffer -- Only two devices permitted");
	m_link[link].m_linkStatisticsBuffer->m_currentBufferPointer =
			m_link[link].m_currentBufferPointer;
	m_link[link].m_linkStatisticsBuffer->m_dstNodeId = m_link[link].m_dstNodeId;
	m_link[link].m_linkStatisticsBuffer->m_srcNodeId = m_link[link].m_srcNodeId;

	return m_link[link].m_linkStatisticsBuffer;
}

bool ssPointToPointChannel::isChannelUp(void) {
	NS_LOG_FUNCTION(this);
	if ((m_link[0].m_state == SLEEPING) || (m_link[1].m_state == SLEEPING)
			|| (m_link[0].m_state == INITIALIZING)
			|| (m_link[1].m_state == INITIALIZING))
		return false;
	return true;
}

void ssPointToPointChannel::setLinkWakeupTime(Time wakeUpTime) {
	NS_LOG_FUNCTION(this);
	m_link[0].m_linkWakeUpTimeDelay = wakeUpTime;
	m_link[1].m_linkWakeUpTimeDelay = wakeUpTime;
}

ssPointToPointChannel::Link::Link() {
	NS_LOG_FUNCTION(this);
	NS_ASSERT_MSG(
			DefaultLinkWakeUpTimeMicroSec < DefaultLinkStateUpdateIntervalMicroSec,
			"DefaultLinkWakeUpTimeMicroSec should be less than DefaultLinkStateUpdateIntervalMicroSec");

	m_state = INITIALIZING;

	if (simulationRunProperties::enableDefaultns3EnergyModel) {
		// USE_DEFAULT_NS3_ENERGY_MODEL
		m_enableLinkEnergyManagement = false;
		m_linkWakeUpTimeDelay = (Seconds(0.0));	// no sleep state wake up delay, if energy_management is NOT enabled.
	} else {
		m_enableLinkEnergyManagement =
				simulationRunProperties::enableLinkEnergyManagement;
		m_linkWakeUpTimeDelay = Time::FromDouble(DefaultLinkWakeUpTimeMicroSec,
				Time::US);
		/* NOT NEEDED, Sanjeev Jun 18, Syntax Corrected
		 m_linkWakeUpTimeDelay=m_linkWakeUpTimeDelay/10;
		 */
	}
	//Sanjeev Jun 18, Syntax Corrected
	m_linkStateUpdateInterval = Time::FromDouble(
	DefaultLinkStateUpdateIntervalMicroSec, Time::US);
	m_src = 0;
	m_dst = 0;
	m_lastUpdateTimeNanoSec = 0;
	m_srcNodeId = -1;
	m_dstNodeId = -1;
	m_BufferSize = 0;
	m_linkStateBuffer = 0;
	m_linkDurationBuffer = 0;
	m_currentBufferPointer = 0;
	t1_Now = 0;
	t1_Duration = 0;
	t1_RemainingDuration = 0;
	t1_TransmitDuration = 0;

	linkData.m_totalSleepTimeNanoSec = 0;
	linkData.m_totalIdleTimeNanoSec = 0;
	linkData.m_totalTransmitTimeNanoSec = 0;
	linkData.m_packetCount = 0;
	linkData.m_countSleepState = 0;

	// we collect the list of all state transitions...
	m_linkStatisticsBuffer = new LinkStatisticsBuffer;
	setCaptureLinkStateBufferSize(DefaultBufferSize);
}

ssPointToPointChannel::Link::~Link() {
	NS_LOG_FUNCTION(this);
}

void ssPointToPointChannel::Link::WriteStatisticsToFile(void) {
	NS_LOG_FUNCTION(this);
	return; // for now skip
	if (WRITE_DEVICE_ENERGY_TO_FILE) {
		double linkTime = (linkData.m_totalTransmitTimeNanoSec
				+ linkData.m_totalIdleTimeNanoSec
				+ linkData.m_totalSleepTimeNanoSec) / NanoSec_to_Secs;
		BaseTopology::fpDeviceEnergy << "ssP2PChannel,srcNode," << m_srcNodeId
				<< ",dstNode," << m_dstNodeId << ",totalLinkTime[s],"
				<< linkTime << ",totalTransmitTime[ns],"
				<< linkData.m_totalTransmitTimeNanoSec << ",totallIdleTime[ns],"
				<< linkData.m_totalIdleTimeNanoSec << ",totalSleepTime [ns],"
				<< linkData.m_totalSleepTimeNanoSec << ",Enabled?,"
				<< m_enableLinkEnergyManagement << ",#packets,"
				<< linkData.m_packetCount << ",sleepStateCount,"
				<< linkData.m_countSleepState << "\n";
	}
}

void ssPointToPointChannel::Link::DoDispose() {
	NS_LOG_FUNCTION(this);
	WriteStatisticsToFile();
	char* debugString = (char*) malloc(
			(2 * sizeof(uint64_t) * m_BufferSize) + 200);
	char debug1[10];
	sprintf(debugString, "srcNode[%d]-->dstNode[%d] (Rotating Buffers) State=",
			m_srcNodeId, m_dstNodeId);
	if (!m_enableLinkEnergyManagement)
		strcat(debugString, ":: m_enableLinkEnergyManagement NOT enabled\n");
	else {
		for (int k = 0; k < m_BufferSize; k++) {
			sprintf(debug1, "%d,", m_linkStateBuffer[k]);
			strcat(debugString, debug1);
		}
		strcat(debugString, "\n(Rotating Buffers)  Durations=");
		for (int k = 0; k < m_BufferSize; k++) {
			sprintf(debug1, "%ld,", m_linkDurationBuffer[k]);
			strcat(debugString, debug1);
		}
	}
	free(m_linkDurationBuffer);
	free(m_linkStateBuffer);
	free(m_linkStatisticsBuffer);
	double linkTime =
			(linkData.m_totalTransmitTimeNanoSec
					+ linkData.m_totalIdleTimeNanoSec
					+ linkData.m_totalSleepTimeNanoSec) / NanoSec_to_Secs;
	SS_ENEGRY_LOG(
			"ssP2PChannel sourceNode [" << m_srcNodeId << "], dstNode [" << m_dstNodeId << "] enabled? [" << m_enableLinkEnergyManagement << "], #packets [" << linkData.m_packetCount << "] sleepStateCount [" << linkData.m_countSleepState << "]");
	SS_ENEGRY_LOG(
			"ssP2PChannel totalLinkTime [" << linkTime << "] totalTransmitTime [" << linkData.m_totalTransmitTimeNanoSec << "] totallIdleTime [" << linkData.m_totalIdleTimeNanoSec << "] totalSleepTime [" << linkData.m_totalSleepTimeNanoSec << "]");
	NS_LOG_LOGIC("ssPointToPointChannel::Link [" << debugString << "]");
}

void ssPointToPointChannel::Link::startLink() {
	linkData.m_linkState = m_state;
// record initial state in the buffer
	recordLinkState(IDLE, 0);
// start monitoring the link states...
// now the link is ready, start monitoring for idle time sleep time etc.
	m_linkStateUpdateEvent = Simulator::Schedule(m_linkStateUpdateInterval,
			&ssPointToPointChannel::Link::updateLinkState, this);
}

void ssPointToPointChannel::Link::recordLinkState(int newState,
		uint64_t duration) {
	NS_LOG_FUNCTION(this);
	NS_LOG_LOGIC(
			"bufPointer " << m_currentBufferPointer << " newState " << newState << " duration " << duration);

// if LinkEnergyManagement is switched off, then everything is the cost of 'tranmission' energy..
	if (!m_enableLinkEnergyManagement)
		newState = TRANSMITTING;

// this is rotating buffer, so reset the pointer to start index, if exceeds the buffer limit..
	if (m_currentBufferPointer > m_BufferSize) {
		m_currentBufferPointer = 0;
	}
// remember this state transition happened now, so record the index for current state in previous time slot, for energy calculation (later)
	m_linkStateBuffer[m_currentBufferPointer] = newState;
	m_linkDurationBuffer[m_currentBufferPointer] = duration;
	m_currentBufferPointer++;
}

void ssPointToPointChannel::Link::setCaptureLinkStateBufferSize(
		int bufferSize) {
	NS_LOG_FUNCTION(this);
	m_BufferSize = bufferSize;
// we collect the list of all state transitions...
	if (m_linkStateBuffer != NULL)
		free(m_linkStateBuffer);
	if (m_linkDurationBuffer != NULL)
		free(m_linkDurationBuffer);
	m_linkStateBuffer = (int *) malloc(sizeof(int) * m_BufferSize);
	m_linkDurationBuffer = (uint64_t *) malloc(sizeof(uint64_t) * m_BufferSize);
	NS_ASSERT_MSG(m_linkStateBuffer != NULL,
			"m_linkStateBuffer is NULL (malloc failed)");
	NS_ASSERT_MSG(m_linkDurationBuffer != NULL,
			"m_linkDurationBuffer is NULL (malloc failed)");

	m_currentBufferPointer = 0;
	for (int k = 0; k < m_BufferSize;
			m_linkStateBuffer[k] = 0, m_linkDurationBuffer[k++] = 0)
		;		// initialize buffer....
				// initialize the link statistics buffer this is the buffer pointer u return to higher levels...
	NS_ASSERT(m_linkStatisticsBuffer != NULL);
	m_linkStatisticsBuffer->m_currentBufferPointer = m_currentBufferPointer;
//m_linkStatisticsBuffer->m_linkStateUpdateIntervalNanoSec =
//		m_linkStateUpdateInterval.GetNanoSeconds();
	m_linkStatisticsBuffer->m_maxBufferSize = m_BufferSize;
	m_linkStatisticsBuffer->m_durationBuffer = m_linkDurationBuffer;
	m_linkStatisticsBuffer->m_stateBuffer = m_linkStateBuffer;
}

void ssPointToPointChannel::Link::updateLinkState() {
	NS_LOG_FUNCTION(this);
// if simulation has stopped, then STOP...
	if (Simulator::IsFinished())
		return;

	t1_Now = Simulator::Now().GetNanoSeconds();
//
// for this link - check the duration and put them into diff state...
//
	t1_Duration = t1_Now - m_lastUpdateTimeNanoSec;

	switch (m_state) {
// if link was earlier idle, put it to sleep...
	case IDLE:
		m_state = SLEEPING;
		linkData.m_totalIdleTimeNanoSec += t1_Duration;
		linkData.m_countSleepState++;
		recordLinkState(IDLE, t1_Duration);
		NS_LOG_LOGIC("moving IDLE --> SLEEP [" << t1_Duration << "]");

		// REDUCE_SLEEP_EVENTS (syntax corrected, Jun 18)
		// how long to sleep? sleep long long long time, till simulation ends if no packet arrives...
		m_linkStateUpdateInterval = Seconds(simulationRunProperties::simStop)
				- Simulator::Now()
				- Time::FromDouble(WAKEUP_BEFORE_SIM_END_NANOSECONDS, Time::NS);
		break;
	case SLEEPING:
		m_state = SLEEPING;
		linkData.m_totalSleepTimeNanoSec += t1_Duration;
		recordLinkState(SLEEPING, t1_Duration);
		NS_LOG_LOGIC("STILL IN --> SLEEP [" << t1_Duration << "]");
		break;
	case TRANSMITTING:
		// you have come here after transmit of all packets, so time to put to idle..
		// no need to record anything till next sample duration., you have already recorded txn_time.
		m_state = IDLE;
		NS_LOG_LOGIC("moving TXN --> IDLE [" << t1_Duration << "]");
		break;
	default:
		m_state = IDLE;
		NS_LOG_LOGIC("ERROR:: move DeFauLT --> IDLE [" << 999999 << "]");
		break;
	} // switch

	linkData.m_linkState = m_state;
	m_lastUpdateTimeNanoSec = t1_Now;

// Let the link keep on updating itself for model power consumption at constant intervals
	m_linkStateUpdateEvent = Simulator::Schedule(m_linkStateUpdateInterval,
			&ssPointToPointChannel::Link::updateLinkState, this);
}

}
// namespace ns3
