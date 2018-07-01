/*
 * ssNode.cpp
 *
 *  Created on: Jul 25, 2016
 *      Author: sanjeev
 */

#include "ss-node.h"
#include "parameters.h"

#include <string.h>
#include "ns3/log.h"
#include "ss-base-topology.h"
#include "ss-base-topology-simulation-variables.h"
#include "ss-energy-source.h"
#include "ss-log-file-flags.h"
#include "ss-tos-point-to-point-netdevice.h"


#define DefaultNodeRunwayIntervalMicroSec				500.0 		//Sanjeev, May26 Checking node state., a.k.a runwayTime
#define DefaultNodeFabricWakeUpTimeMicroSec				200.0
// Sanjeev:: Jun 13
#define DefaultInitialVoltage_Fabric					1.10
#define DefaultActiveCurrentA_Fabric					18.18		//4.54545  Madhurima Changed the parameter as on April 21
#define	DefaultLeakageCurrentA_Fabric					3.64		//0.4*DefaultActiveCurrentA_Fabric

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("ssNode");

NS_OBJECT_ENSURE_REGISTERED(ssNode);

TypeId ssNode::GetTypeId(void) {
	static TypeId tid = TypeId("ns3::ssNode").SetParent<Node>().SetGroupName(
			"Network").AddConstructor<ssNode>();
	return tid;
}

ssNode::ssNode() {
	NS_LOG_FUNCTION(this);
	ptr_source = 0;
	nodeName = 0;
	m_NumberOfApps_In = m_NumberOfApps_Out = 0;
	m_currentUtilizedBW_In = m_maxAvailableBW_In = m_currentAvailableBW_In = 0;
	m_currentUtilizedBW_Out = m_maxAvailableBW_Out = m_currentAvailableBW_Out =
			0;
	m_deviceSleepStates = NULL;
	m_NumberOfNetDevices = 0;				// sanjeev May26
	m_nodeRunwayTime = Time::FromDouble(DefaultNodeRunwayIntervalMicroSec,
			Time::US);	//
	// if not enabled, return 0;
	if (simulationRunProperties::enableNodeFabricManagement)
		m_nodeFabricWakeUpTime = Time::FromDouble(
		DefaultNodeFabricWakeUpTimeMicroSec, Time::US);	//
	else
		m_nodeFabricWakeUpTime = Seconds(0.);
	//
	m_NodeDataCollected.m_countSleepState = 0;
	m_NodeDataCollected.m_totalDeviceTimeNanoSec = 0;
	m_NodeDataCollected.m_totalReadyTimeNanoSec = 0;
	m_NodeDataCollected.m_totalSleepTimeNanoSec = 0;
	m_NodeDataCollected.m_totalFabricWakeUpTimeAdded_displayOnly = Seconds(0.);
	m_lastUpdateTimeNanoSec = 0;
	m_nodeState = INIT;
	setNodeType(NODE);
}

ssNode::~ssNode() {
	NS_LOG_FUNCTION(this);
}
//
void ssNode::DoDispose(void) {
	NS_LOG_FUNCTION(this);
	ComputeLatestStateDurationCalculations();
	//sanjeev Jun 13
	WriteStatisticsToFile();
	// device 0 is 127.0.0.1 ..localhost..
	SS_ENEGRY_LOG(
			"ssNode: [" << GetId() << "] NodeName [" << nodeName << "] #devices [" << m_NumberOfNetDevices-1 << "] maxAvailableBW [" << m_maxAvailableBW_In << "Mbps] maxFlowPerNode [" << m_maxFlowPerNode << "] totalFabricWakeUpTimeAdded [" << m_NodeDataCollected.m_totalFabricWakeUpTimeAdded_displayOnly << "]");
	SS_ENEGRY_LOG(
			"ssNode: [" << GetId() << "] totalNodeTime(s) [" << m_NodeDataCollected.m_totalDeviceTimeNanoSec/NANOSEC_TO_SECS << "] totalReadyTime(ns) ["<< m_NodeDataCollected.m_totalReadyTimeNanoSec << "] totalSleepTime(ns) ["<< m_NodeDataCollected.m_totalSleepTimeNanoSec << "] countSleepState ["<< m_NodeDataCollected.m_countSleepState << "]");

	Node::DoDispose();
}
/*
 * rename function name sanjeev Jun 13
 */
void ssNode::ComputeLatestStateDurationCalculations(void) {
	NS_LOG_FUNCTION(this);
	uint64_t t_now = Simulator::Now().ToDouble(Time::NS);
	// add final timing as per it's last state so far..final timings...
	uint64_t timeDiff = t_now - m_lastUpdateTimeNanoSec;
	// sanjeev Jun 13
	m_lastUpdateTimeNanoSec = t_now;

	if (m_nodeState == SLEEP) {
		m_NodeDataCollected.m_totalSleepTimeNanoSec += timeDiff;
	} else if (m_nodeState == READY) {
		m_NodeDataCollected.m_totalReadyTimeNanoSec += timeDiff;
	} else {
		NS_ABORT_MSG(
				"Node [" << GetId() << "] was in unknown state[" << m_nodeState << "]");
	}
	m_NodeDataCollected.m_totalDeviceTimeNanoSec =
			m_NodeDataCollected.m_totalReadyTimeNanoSec
					+ m_NodeDataCollected.m_totalSleepTimeNanoSec;
	//end..
}
//
Ipv4Address ssNode::GetNodeIpv4Address(void) {
	NS_ASSERT_MSG((nodeType == ssNode::HOST),
			"This should be called only for hosts nodes");
	Ptr<Ipv4> ipv4 = GetObject<Ipv4>();
	NS_ASSERT_MSG(ipv4!=NULL, "Failed to get Ipv4");
	return ipv4->GetAddress(1, 0).GetLocal();
}

// get sendChannel dataRate, or rcvChannel dataRate
void ssNode::InitializeNode(void) {
	NS_LOG_FUNCTION(this);
	if (nodeType == ssNode::HOST) {
		// simplified,. we are implementing this only for hosts, so get device 1
		// remember device 0 is localhost 127.0.0.1
		Ptr<ssTOSPointToPointNetDevice> pdevice = DynamicCast<
				ssTOSPointToPointNetDevice>(GetDevice(1));
		NS_ASSERT_MSG(pdevice!=NULL,
				"Failed to get ssTOSPointToPointNetDevice");
		m_maxAvailableBW_In = pdevice->getChannelDataRate();

		NS_LOG_UNCOND("m_maxAvailableBW_In "<<m_maxAvailableBW_In);
		//**************************************Added this line by madhurima on April 21*******************************
#if COMPILE_CUSTOM_ROUTING_CODE
		m_maxAvailableBW_In = m_maxAvailableBW_In
				* Ipv4GlobalRouting::threshold_percentage_for_dropping;
#endif
		m_currentAvailableBW_Out = m_maxAvailableBW_Out =
				m_currentAvailableBW_In = m_maxAvailableBW_In;
		m_currentUtilizedBW_In = m_currentUtilizedBW_Out = 0;
		if (simulationRunProperties::enableVMC)
			m_maxFlowPerNode = simulationRunProperties::vm_per_node
					* simulationRunProperties::flows_per_VM;
		else
// sanjeev VMC logic, Apr 3rd, if VMC is OFF, set apps allowed is very very high..
			m_maxFlowPerNode = MegaSec_to_Secs;
	}
// May 26..sanjeev..initialize...everything is ON initially
	m_NumberOfNetDevices = GetNDevices();
	m_deviceSleepStates = (bool *) malloc(sizeof(bool) * m_NumberOfNetDevices);
	for (uint32_t i = 0; i < m_NumberOfNetDevices; i++)
		m_deviceSleepStates[i] = false;
	m_nodeState = READY;
//
	NS_LOG_LOGIC(
			"Node: " << GetId() << " #devices " << m_NumberOfNetDevices << " maxAvailableBW " << m_maxAvailableBW_In << " maxFlowPerNode " << m_maxFlowPerNode << " m_nodeRunwayTime " << m_nodeRunwayTime << " IsFabricAsleep " << IsFabricAsleep());
}
//
bool ssNode::IsFabricAsleep(void) {
	NS_LOG_FUNCTION(this);
// sanjeev Jun 16..host never falls asleep
	if (nodeType == ssNode::HOST)
		return false;

	bool m_isFabricAsleep = true;
// ignore device 0, it is local loop 127.0.0.1
	for (uint32_t i = 1; i < m_NumberOfNetDevices; i++) {
		m_isFabricAsleep &= m_deviceSleepStates[i];
		// no need to check if fabric is asleep (every device state should be aSleep=true
		if (!m_isFabricAsleep)
			break;
	}
	NS_LOG_LOGIC(
			"Node: " << GetId() << " #m_isFabricAsleep " << m_isFabricAsleep);
// All devices are asleep, return asleep
	return m_isFabricAsleep;
}
/*
 *  Jun 23th...New discussion...
 */
Time  ssNode::GetNodeFabricStateAdditionalTimeForThisPacket(const uint32_t &flowId, const uint32_t &packetId) {
	NS_LOG_FUNCTION(this);
	if (packetId != 0) {
		return GetNodeFabricStateAdditionalTime();
	}
	return Seconds(0.);
}

//
Time ssNode::GetNodeFabricStateAdditionalTime(void) {
	NS_LOG_FUNCTION(this);
// sanjeev Jun 16..host never falls asleep and takes NO additional time
	if (nodeType == ssNode::HOST)
		return Seconds(0.);
// check if ALL devices said they are sleeping !!
	if (IsFabricAsleep()) {
		m_NodeDataCollected.m_totalFabricWakeUpTimeAdded_displayOnly +=
				m_nodeFabricWakeUpTime;
		return (m_nodeFabricWakeUpTime);
	} else
		return Seconds(0.);
}
//
//	<May 26> Node Fabric monitoring
//
void ssNode::MonitorNodeRunway(void) {
	NS_LOG_FUNCTION(this);		//
	NS_ASSERT_MSG(m_nodeStateEvent.IsExpired(),
			" ssNode::MonitorNodeRunway Event error");
	uint64_t t_now = Simulator::Now().ToDouble(Time::NS);
	// add time so far to ready_state_time
	// it was in ready state so far....runway time expired, now to go to sleep...
	uint64_t timeDiff = t_now - m_lastUpdateTimeNanoSec;
	m_NodeDataCollected.m_totalReadyTimeNanoSec += timeDiff;
	m_lastUpdateTimeNanoSec = t_now;
	// runway has expired so go to sleep_state
	m_NodeDataCollected.m_countSleepState++;
	m_nodeState = SLEEP;
//
}
/*
 * moved here, sanjeev Jun 13
 TOS or TRS true -only then Fabric can be false or true
 if TOS AND TRS = false, fabric=false (ie. defaultns3 =true , fabric=true)
 if fabric = false. then no active+no passive state , everything is active state.
 no additional delay if fabric=true, then compute active Currrent(energy/time) and
 passive current(energy/time) separately and compute totalFabricEnergy. FabricAdditionalTime
 */
double ssNode::GetNodeFabricEnergy(void) {
	NS_LOG_FUNCTION(this);

	if (COMPLETELY_DISABLE_NODE_FABRIC_ENERGY_MODEL)
		return 0;

// sanjeev Jun 16..host never consumes energy
	if (nodeType == ssNode::HOST)
		return 0;

	double nodeFabricVoltage = DefaultInitialVoltage_Fabric;
	double nodeFabricLeakageCurrent = DefaultLeakageCurrentA_Fabric;
	double nodeFabricActiveCurrent = DefaultActiveCurrentA_Fabric;

	//
	if (!simulationRunProperties::enableNodeFabricManagement)
		nodeFabricLeakageCurrent = nodeFabricActiveCurrent;
	//
	//sanjeev Jun 13
	ComputeLatestStateDurationCalculations();

	double tFabric_ReadyStateEnergy =
			m_NodeDataCollected.m_totalReadyTimeNanoSec * nodeFabricVoltage
					* nodeFabricActiveCurrent;
	double tFabric_SleepStateEnergy =
			m_NodeDataCollected.m_totalSleepTimeNanoSec * nodeFabricVoltage
					* nodeFabricLeakageCurrent;
	// return '0' if you want to test without this energy..
	return tFabric_ReadyStateEnergy + tFabric_SleepStateEnergy;
}
//
void ssNode::SetDeviceState(const uint32_t &deviceIndex,
		const bool &isSleepState) {

	// sanjeev Jun 16..host node ? nothing to do here..
	if (nodeType == ssNode::HOST)
		return;

	NS_LOG_FUNCTION(this << " isSleepState "<< isSleepState);
	NS_ASSERT_MSG(deviceIndex < m_NumberOfNetDevices,
			"SetDeviceState wrong indexA, node [" << GetId() << "] index [" << deviceIndex << "/" << m_NumberOfNetDevices << "] state [" << isSleepState << "]");
	NS_ASSERT_MSG(deviceIndex >= 0,
			"SetDeviceState wrong indexB, node [" << GetId() << "] index [" << deviceIndex << "/" << m_NumberOfNetDevices << "] state [" << isSleepState << "]");
	m_deviceSleepStates[deviceIndex] = isSleepState;
	bool isFabricSleeping = IsFabricAsleep();

	if (m_nodeStateEvent.IsRunning())
		m_nodeStateEvent.Cancel();

// now decide...?
	switch (m_nodeState) {
	case (READY): {
		if (isFabricSleeping) {
			NS_LOG_LOGIC(
					"Node [" << GetId() << "] was previously READY, am I ready to go to SLEEP? Start RunwayMonitor");
			// start runway monitor
			m_nodeStateEvent = Simulator::Schedule(m_nodeRunwayTime,
					&ssNode::MonitorNodeRunway, this);
		}
		break;
	}
	case (SLEEP): {
		if (!isFabricSleeping) {
			NS_LOG_LOGIC(
					"Node [" << GetId() << "] was previously SLEEPING, and I'm waking up..");
			// it was in sleeping state so far....add time to sleep state & then wake up...
			uint64_t t_now = Simulator::Now().ToDouble(Time::NS);
			uint64_t timeDiff = t_now - m_lastUpdateTimeNanoSec;
			m_NodeDataCollected.m_totalSleepTimeNanoSec += timeDiff;
			m_lastUpdateTimeNanoSec = t_now;
			m_nodeState = READY;
		}
		break;
	}
	default: {
		NS_ABORT_MSG(
				"Node [" << GetId() << "] was in unknown state[" << m_nodeState << "]");
	}
	} // end of switch
}

//
void ssNode::setNodeType(int n) {
	NS_LOG_FUNCTION(this);

	if (nodeName == NULL)
		nodeName = (char *) malloc(7);
	switch (n) {
	case CORE: {
		std::strcpy(nodeName, "CORE");
		nodeType = CORE;
		rColor = 0;
		gColor = 0;
		bColor = 255;
		break;
	}
	case AGGR: {
		std::strcpy(nodeName, "AGGR");
		nodeType = AGGR;
		rColor = 0;
		gColor = 255;
		bColor = 0;
		break;
	}
	case EDGE: {
		std::strcpy(nodeName, "EDGE");
		nodeType = EDGE;
		rColor = 255;
		gColor = 0;
		bColor = 0;
		break;
	}
	case HOST: {
		std::strcpy(nodeName, "HOST");
		nodeType = HOST;
		rColor = 0;
		gColor = 0;
		bColor = 0;
		break;
	}
	case SERVER: {
		std::strcpy(nodeName, "SRV");
		nodeType = EDGE;
		rColor = 255;
		gColor = 255;
		bColor = 0;
		break;
	}
	case CLIENT: {
		std::strcpy(nodeName, "Client");
		nodeType = EDGE;
		rColor = 255;
		gColor = 0;
		bColor = 255;
		break;
	}
	default: {
		std::strcpy(nodeName, "Node");
		nodeType = NODE;
		rColor = 255;
		gColor = 255;
		bColor = 255;
		break;
	}
	}
}
//
void ssNode::SetEnergySource(Ptr<BasicEnergySource> pE) {
	NS_LOG_FUNCTION(this);
	ptr_source = pE;
	AggregateObject(pE);
}
/*
 * sanjeev Jun 13
 */
void ssNode::WriteStatisticsToFile(void) {
	NS_LOG_FUNCTION(this);

	if (WRITE_DEVICE_ENERGY_TO_FILE) {
		double nodeFabricVoltage = DefaultInitialVoltage_Fabric;
		double nodeFabricLeakageCurrent = DefaultLeakageCurrentA_Fabric;
		double nodeFabricActiveCurrent = DefaultActiveCurrentA_Fabric;
		//
		if (!simulationRunProperties::enableNodeFabricManagement)
			nodeFabricLeakageCurrent = nodeFabricActiveCurrent;
		//
		double tFabric_ReadyStateEnergy =
				m_NodeDataCollected.m_totalReadyTimeNanoSec * nodeFabricVoltage
						* nodeFabricActiveCurrent;
		double tFabric_SleepStateEnergy =
				m_NodeDataCollected.m_totalSleepTimeNanoSec * nodeFabricVoltage
						* nodeFabricLeakageCurrent;
		double totalEnergy = (tFabric_ReadyStateEnergy
				+ tFabric_SleepStateEnergy) / NANOSEC_TO_SECS;

		// added extra log...Jun 18.sanjeev
		BaseTopology::fpDeviceEnergy << "NodeFabric,Node," << GetId()
				<< ",Device,x,totalNodeTime[s],"
				<< m_NodeDataCollected.m_totalDeviceTimeNanoSec
						/ NANOSEC_TO_SECS << ",totalReadyTime[ns],"
				<< m_NodeDataCollected.m_totalReadyTimeNanoSec
				<< ",totalSleepTime[ns],"
				<< m_NodeDataCollected.m_totalSleepTimeNanoSec
				<< ",totalNodeEnergy[J]," << totalEnergy
				<< ",totalReadyEnergy[nJ]," << tFabric_ReadyStateEnergy
				<< ",totalSleepEnergy[nJ]," << tFabric_SleepStateEnergy
				<< ",Type," << nodeName
				<< ",AdditionalTime," << m_NodeDataCollected.m_totalFabricWakeUpTimeAdded_displayOnly
				<< ",TotalSleepCount," << m_NodeDataCollected.m_countSleepState
				<< "\n";

	}
}
//
void ssNode::WriteStateDurationEnergyGranular(std::ostream* fpState,
		std::ostream* fpEnergy, std::ostream* fpDuration) {
	NS_LOG_FUNCTION(this);
	Ptr<ssTOSPointToPointNetDevice> pdevice;
	int nD = GetNDevices();
	for (int i = 1; i < nD; i++) {
		pdevice = DynamicCast<ssTOSPointToPointNetDevice>(GetDevice(i));
		pdevice->WriteStateDurationEnergyGranular(fpState, fpEnergy, fpDuration,
				ptr_source);
	}

}

}
/* namespace ns3 */
