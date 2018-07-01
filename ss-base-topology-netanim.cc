/*
 * ss-base-topology-netanim.cc
 *
 *  Created on: Jul 12, 2016
 *      Author: sanjeev
 */

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ss-base-topology.h"
#include "ss-energy-source.h"
#include "ss-log-file-flags.h"
#include "ss-node.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("BaseTopologyN");

/*****************************************************/
void BaseTopology::UpdateNodeCounters() {
	NS_LOG_FUNCTION(this << " .... " << Simulator::Now());
	if (simulationRunProperties::enableDefaultns3EnergyModel)
		UpdateNodeCountersns3();
	else
		UpdateNodeCountersCustom();
}

/*****************************************************/
void BaseTopology::UpdateNodeCountersCustom() {
	NS_LOG_FUNCTION(this);
	// reset all earlier cumulative counters...
	NS_ASSERT(m_totalTreeEnergy != NULL);
	m_totalTreeEnergy->m_totalConsumedEnergy_nJ = 0;
	m_totalTreeEnergy->m_totalConsumedEnergyByNodeFabricONLY_nJ = 0; // sanjeev Jun 13
	m_totalTreeEnergy->m_totalIdleTimeEnergy_nJ = 0;
	m_totalTreeEnergy->m_totalTransmitTimeEnergy_nJ = 0;
	m_totalTreeEnergy->m_totalSleepStateTimeEnergy_nJ = 0;
	m_totalTreeEnergy->m_totalTransmitTimeNanoSec = 0;
	m_totalTreeEnergy->m_totalSleepTimeNanoSec = 0;
	m_totalTreeEnergy->m_totalIdleTimeNanoSec = 0;
	m_totalTreeEnergy->m_totalTimeNanoSec = 0;
	// m_totalTreeEnergy->m_deviceCount = already calculated - does not change with time;
	// sanjeev fixed int to uint32_t
	for (uint32_t nodeId = 0; nodeId < allNodes.GetN(); nodeId++) {

		t_node = DynamicCast<ssNode>(allNodes.Get(nodeId));
		t_eSource = DynamicCast<ssEnergySource>(t_node->ptr_source);
		uint32_t nD = t_node->GetNDevices() - 1;
		// sanjeev Jun 13
		double t_fabricEnergy = t_node->GetNodeFabricEnergy(); // sanjeev Jun 13
		m_totalTreeEnergy->m_totalConsumedEnergyByNodeFabricONLY_nJ +=
				t_fabricEnergy; // sanjeev Jun 13
		// add node fabric energy to this...
		m_totalTreeEnergy->m_totalConsumedEnergy_nJ += t_fabricEnergy;

		if (t_eSource == NULL) {
			// energy model is not implemented...
			if (totalEnergyCounterId != 0)
				m_anim->UpdateNodeCounter(totalEnergyCounterId, nodeId, 0);
			if (activeEnergyCounterId != 0)
				m_anim->UpdateNodeCounter(activeEnergyCounterId, nodeId, 0);
			if (sleepTimeEnergyCounterId != 0)
				m_anim->UpdateNodeCounter(sleepTimeEnergyCounterId, nodeId, 0);
			if (leakageEnergyCounterId != 0)
				m_anim->UpdateNodeCounter(leakageEnergyCounterId, nodeId, 0);
			if (activeTimeCounterId != 0)
				m_anim->UpdateNodeCounter(activeTimeCounterId, nodeId, 0);
			if (sleepTimeCounterId != 0)
				m_anim->UpdateNodeCounter(sleepTimeCounterId, nodeId, 0);
			if (idleTimeCounterId != 0)
				m_anim->UpdateNodeCounter(idleTimeCounterId, nodeId, 0);
			if (sleepStateCounterId != 0)
				m_anim->UpdateNodeCounter(sleepStateCounterId, nodeId, 0);
			if (packetCounterId != 0)
				m_anim->UpdateNodeCounter(packetCounterId, nodeId, 0);
			continue;
		}
		t_ptrMED = t_eSource->CalculateEnergyConsumed();

		if (totalEnergyCounterId != 0)
			m_anim->UpdateNodeCounter(totalEnergyCounterId, nodeId,
					t_ptrMED->m_totalConsumedEnergy_nJ / NanoSec_to_Secs);
		if (activeEnergyCounterId != 0)
			m_anim->UpdateNodeCounter(activeEnergyCounterId, nodeId,
					t_ptrMED->m_totalTransmitTimeEnergy_nJ / NanoSec_to_Secs);
		if (sleepTimeEnergyCounterId != 0)
			m_anim->UpdateNodeCounter(sleepTimeEnergyCounterId, nodeId,
					t_ptrMED->m_totalSleepStateTimeEnergy_nJ / NanoSec_to_Secs);
		if (leakageEnergyCounterId != 0)
			m_anim->UpdateNodeCounter(leakageEnergyCounterId, nodeId,
					t_ptrMED->m_totalIdleTimeEnergy_nJ / NanoSec_to_Secs);
		if (activeTimeCounterId != 0)
			m_anim->UpdateNodeCounter(activeTimeCounterId, nodeId,
					t_ptrMED->m_totalTransmitTimeNanoSec
							/ (NanoSec_to_Secs * nD));
		if (sleepTimeCounterId != 0)
			m_anim->UpdateNodeCounter(sleepTimeCounterId, nodeId,
					t_ptrMED->m_totalSleepTimeNanoSec / (NanoSec_to_Secs * nD));
		if (idleTimeCounterId != 0)
			m_anim->UpdateNodeCounter(idleTimeCounterId, nodeId,
					t_ptrMED->m_totalIdleTimeNanoSec / (NanoSec_to_Secs * nD));
		if (sleepStateCounterId != 0)
			m_anim->UpdateNodeCounter(sleepStateCounterId, nodeId,
					(double) t_ptrMED->m_countSleepState);
		if (packetCounterId != 0)
			m_anim->UpdateNodeCounter(packetCounterId, nodeId,
					(double) t_ptrMED->m_packetCount);

		// store totals for entire tree ....for later use...(cumulative totals for all nodes)..
		m_totalTreeEnergy->m_totalConsumedEnergy_nJ +=
				t_ptrMED->m_totalConsumedEnergy_nJ;
		m_totalTreeEnergy->m_totalIdleTimeEnergy_nJ +=
				t_ptrMED->m_totalIdleTimeEnergy_nJ;
		m_totalTreeEnergy->m_totalTransmitTimeEnergy_nJ +=
				t_ptrMED->m_totalTransmitTimeEnergy_nJ;
		m_totalTreeEnergy->m_totalSleepStateTimeEnergy_nJ +=
				t_ptrMED->m_totalSleepStateTimeEnergy_nJ;
		m_totalTreeEnergy->m_totalTransmitTimeNanoSec +=
				t_ptrMED->m_totalTransmitTimeNanoSec;
		m_totalTreeEnergy->m_totalSleepTimeNanoSec +=
				t_ptrMED->m_totalSleepTimeNanoSec;
		m_totalTreeEnergy->m_totalIdleTimeNanoSec +=
				t_ptrMED->m_totalIdleTimeNanoSec;
		m_totalTreeEnergy->m_totalTimeNanoSec +=
				t_ptrMED->m_totalDeviceTimeNanoSec;
		if (m_totalTreeEnergy->m_deviceMaxQdepth
				< t_ptrMED->m_DeviceMaxQueueDepth) {
			m_totalTreeEnergy->m_deviceMaxQdepth =
					t_ptrMED->m_DeviceMaxQueueDepth;
			m_totalTreeEnergy->m_deviceMaxQdepth_atNode = nodeId;
		}
		m_totalTreeEnergy->m_DeviceTotalObservedQueueDepth +=
				t_ptrMED->m_DeviceTotalObservedQueueDepth;
	}

	// sanjeev, mar 23, write debug file netanim counters,
	// cannot depend on netanim to run & show these numbers in time scale
	if (fpNetAnimCounters != NULL) {
		fpNetAnimCounters << Simulator::Now().ToDouble(Time::MS) << ","
				<< m_flowCount << ","
				<< m_totalTreeEnergy->m_totalConsumedEnergy_nJ / NanoSec_to_Secs
				<< ","
				<< m_totalTreeEnergy->m_totalIdleTimeEnergy_nJ / NanoSec_to_Secs
				<< ","
				<< m_totalTreeEnergy->m_totalTransmitTimeEnergy_nJ
						/ NanoSec_to_Secs << ","
				<< m_totalTreeEnergy->m_totalSleepStateTimeEnergy_nJ
						/ NanoSec_to_Secs << ","
				<< m_totalTreeEnergy->m_totalTransmitTimeNanoSec
						/ NanoSec_to_Secs << ","
				<< m_totalTreeEnergy->m_totalSleepTimeNanoSec / NanoSec_to_Secs
				<< ","
				<< m_totalTreeEnergy->m_totalIdleTimeNanoSec / NanoSec_to_Secs
				<< ","
				<< m_totalTreeEnergy->m_totalTimeNanoSec / NanoSec_to_Secs
				<< "\n";
	}

	// if simulation has stopped, then STOP...
	if (!Simulator::IsFinished())
		Simulator::Schedule(m_EnergyMeterUpdateIntervalSec, //sanjeev?bug?2/25,protected v public method?
				&BaseTopology::UpdateNodeCounters, this);
}

/*****************************************************/
void BaseTopology::UpdateNodeCountersns3() {
	NS_LOG_FUNCTION(this);

	NS_ASSERT(m_totalTreeEnergy != NULL);
	double energyConsumedS = 0;
	m_totalTreeEnergy->m_totalConsumedEnergy_nJ = 0;
	// sanjeev Jun 13
	m_totalTreeEnergy->m_totalConsumedEnergyByNodeFabricONLY_nJ = 0;
	Ptr<SimpleDeviceEnergyModel> pModel;
	Ptr<BasicEnergySource> pSource;
	t_x = allNodes.GetN();

	for (int t_a = 0; t_a < t_x; t_a++) {
		t_node = DynamicCast<ssNode>(allNodes.Get(t_a));
		pSource = t_node->GetObject<BasicEnergySource>();
		// energy model not implemented
		if (pSource != NULL)
			energyConsumedS = pSource->GetInitialEnergy()
					- pSource->GetRemainingEnergy();
		// convert to nanoJoules, because later we are dividing again.
		energyConsumedS *= NANOSEC_TO_SECS;
		NS_LOG_LOGIC(
				Simulator::Now() << " UpdateNodeCountersns3:: Node [" << t_a << "] energyConsumedS [" << energyConsumedS << "]");
		m_totalTreeEnergy->m_totalConsumedEnergy_nJ += energyConsumedS;
		// sanjeev Jun 13
		double t_fabricEnergy = t_node->GetNodeFabricEnergy(); // sanjeev Jun 13
		m_totalTreeEnergy->m_totalConsumedEnergyByNodeFabricONLY_nJ +=
				t_fabricEnergy; // sanjeev Jun 13
		// add node fabric energy to this...sanjeev Jun 13
		m_totalTreeEnergy->m_totalConsumedEnergy_nJ += t_fabricEnergy;
		//
		if (totalEnergyCounterId != 0)
			m_anim->UpdateNodeCounter(totalEnergyCounterId, t_a,
					energyConsumedS);
	}

	// if simulation has stopped, then STOP...
	if (!Simulator::IsFinished())
		Simulator::Schedule(m_EnergyMeterUpdateIntervalSec,
				&BaseTopology::UpdateNodeCounters, this);
}

/*************************************************************/
void BaseTopology::SetNodeAnimProperties() {
	NS_LOG_FUNCTION(this);

	if (m_anim == NULL)
		return;

	double nodeSize = 0.15;

	/*** add counters for each node ***/
	totalEnergyCounterId = m_anim->AddNodeCounter("Total Energy (J)",
			AnimationInterface::DOUBLE_COUNTER);
	activeEnergyCounterId = m_anim->AddNodeCounter("Active Energy (J)",
			AnimationInterface::DOUBLE_COUNTER);
	sleepTimeEnergyCounterId = m_anim->AddNodeCounter(
			"Low Power State Energy (J)", AnimationInterface::DOUBLE_COUNTER);
	leakageEnergyCounterId = m_anim->AddNodeCounter("Leakage Energy (J)",
			AnimationInterface::DOUBLE_COUNTER);
	activeTimeCounterId = m_anim->AddNodeCounter(
			"Active Time (sec) [avg over all NetDevices]",
			AnimationInterface::DOUBLE_COUNTER);
	sleepTimeCounterId = m_anim->AddNodeCounter(
			"Sleep Time (sec) [avg over all NetDevices]",
			AnimationInterface::DOUBLE_COUNTER);
	idleTimeCounterId = m_anim->AddNodeCounter(
			"Idle Time (sec) [avg over all NetDevices]",
			AnimationInterface::DOUBLE_COUNTER);
	sleepStateCounterId = m_anim->AddNodeCounter(
			"Count of Sleep State Transition [total node]",
			AnimationInterface::UINT32_COUNTER);
	packetCounterId = m_anim->AddNodeCounter("Packets Processed (total node)",
			AnimationInterface::UINT32_COUNTER);
	deviceCountCounterId = m_anim->AddNodeCounter("Net devices on this node",
			AnimationInterface::UINT32_COUNTER);
	// now start labeling for Animation FAT tree from topmost core routers...
	// sanjeev. making types match with example.!(maybe I will on counter stats & metrics
	std::ostringstream node0Oss;
	for (uint32_t i = 0; i < allNodes.GetN(); i++) {
		// connect the topmost level core_routers with next level aggregators_routers...
		t_node = DynamicCast<ssNode>(allNodes.Get(i));
		//now set names & sizes...
		m_anim->UpdateNodeSize(i, nodeSize, nodeSize);
		m_anim->UpdateNodeColor(t_node->GetId(), t_node->rColor, t_node->gColor,
				t_node->bColor);
		m_anim->UpdateNodeDescription(i, t_node->nodeName);
		if (deviceCountCounterId != 0)
			m_anim->UpdateNodeCounter(deviceCountCounterId, i,
					(t_node->GetNDevices() - 1));
		// set it here once only..
		m_totalTreeEnergy->m_deviceCount += (t_node->GetNDevices() - 1);
	}
}

/**************************************************************/
void BaseTopology::InitializeNetAnim() {
	NS_LOG_FUNCTION(this);

	SetNodeAnimPosition();
	const char *ext1 = NS3_NETANIM_SIMULATION_FILENAME1;
	char* animFileName = (char*) malloc(
			std::strlen(simulationRunProperties::arg0) + std::strlen(ext1) + 3);
	std::sprintf(animFileName, "%s%s", simulationRunProperties::arg0, ext1);
	std::string animFile = animFileName;
	m_anim = new AnimationInterface(animFile);
	NS_ASSERT_MSG(m_anim != NULL, "Failed to create AnimationInterface");
	SetNodeAnimProperties();

	NS_LOG_UNCOND(
			"Statistics: ns3::anim Simulation output is at \t[" << animFile << "]");
}

} // namespace
