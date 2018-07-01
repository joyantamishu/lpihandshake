/*
 * ss-base-topology.cc
 *
 *  Created on: Jul 12, 2016
 *      Author: sanjeev
 */

#include "ss-base-topology.h"
#include "parameters.h"
#include "ss-device-energy-model.h"
#include "ss-energy-source.h"
#include "ss-node.h"
#include "ss-point-to-point-helper.h"
#include "flow-duration-random-variable-stream.h"
#include "ns3/log.h"
#include "ss-tos-point-to-point-netdevice.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("BaseTopology");
/*
 *
 */
int BaseTopology::total_appication = 0;
FlowDataCollected *BaseTopology::m_flowData = NULL;
std::ofstream BaseTopology::fpDeviceEnergy;

std::vector<chunk_info> BaseTopology::chunkTracker;

std::vector<MultipleCopyOfChunkInfo> BaseTopology::chunkCopyLocations;

Ipv4Address* BaseTopology::hostTranslation = new Ipv4Address[simulationRunProperties::k * simulationRunProperties::k * simulationRunProperties::k];

uint32_t BaseTopology::num_of_retried_packets = 0;

uint32_t BaseTopology::num_of_retired_packets_specific_node = 0;

uint32_t** BaseTopology::chunk_assignment_to_applications = new uint32_t*[simulationRunProperties::total_applications + 1]; //application and which chunks

uint32_t** BaseTopology::application_assignment_to_node = new uint32_t*[uint32_t(simulationRunProperties::k * simulationRunProperties::k * simulationRunProperties::k)/ 4];

double** BaseTopology::chunk_assignment_probability_to_applications = new double*[simulationRunProperties::total_applications + 1];

uint32_t *BaseTopology::virtual_to_absolute_mapper = new uint32_t[simulationRunProperties::total_chunk];

Ptr<RandomVariableStream> BaseTopology::application_selector = CreateObject<UniformRandomVariable>();

double BaseTopology::popularity_change_simulation_interval = simulationRunProperties::popularity_change_start_ms;

Ptr<RandomVariableStream> BaseTopology::Popularity_Change_Random_Variable = CreateObject<UniformRandomVariable>();

ApplicationMetrics *BaseTopology::application_metrics = new ApplicationMetrics[simulationRunProperties::total_applications];

uint32_t BaseTopology::phrase_change_counter = 0;

uint32_t *BaseTopology::phrase_change_interval;

double *BaseTopology::phrase_change_intensity_value;

double BaseTopology::intensity_change_simulation_interval = simulationRunProperties::intensity_change_start_ms;

uint32_t BaseTopology::total_phrase_changed = 0;

double BaseTopology::intensity_change_scale = 1.0;

Pod* BaseTopology::p;

BaseTopology::~BaseTopology() {
	NS_LOG_FUNCTION(this);
	DoDispose();
}

void BaseTopology::DoDispose(void) {
	NS_LOG_FUNCTION(this);
	DisplayStatistics();
}

/*
 *
 */
void BaseTopology::DisplayCommandLine(void) {
	NS_LOG_FUNCTION(this);
	//
	NS_LOG_UNCOND(
			"\n--- Energy Management Logic:: Link [" << (simulationRunProperties::enableLinkEnergyManagement ? "true":"false") << "], Node Fabric Energy [" << (simulationRunProperties::enableNodeFabricManagement? "true":"false") << "], Device TOS_ENABLED [" << (simulationRunProperties::enableDeviceTOSManagement? "true":"false") << "], TRS_ENABLED [" << (simulationRunProperties::enableDeviceTRSManagement? "true":"false") << "]");
	//
	NS_LOG_UNCOND(
			"UDP App [k=" << simulationRunProperties::k << "] SawTooth Model [" << (simulationRunProperties::enableSawToothModel ? "true":"false") << "] Markov Model [" << (simulationRunProperties::enableMarkovModel ? "true":"false") << "] Tick Model [" << (simulationRunProperties::enableTickModel ? "true":"false") << "] Custom Routing [" << (simulationRunProperties::enableCustomRouting ? "true":"false") << "] GA [" << (simulationRunProperties::enable_global_algortihm ? "true":"false") << "] VMC [" << (simulationRunProperties::enableVMC ? "true":"false") << "]");
}
/*
 *
 */

void BaseTopology::DisplayStatistics(void) {
	NS_LOG_FUNCTION(this);
	// Sanjeev Apr21
	DisplayCommandLine();
//
	if (useRandomIntervals) {
		// flow bandwidth selector...
		double BWmean = DynamicCast<TriangularRandomVariable>(
				m_randomBWVariable)->GetMean();
		double Flowmean = -999.99;
		// sanjeev, debug info mar 20
		Flowmean = DynamicCast<FlowDurationRandomVariable>(
				m_flowStopDurationTimer)->GetMean();
		NS_LOG_UNCOND(
				"FlowDuration::type " << m_flowStopDurationTimer->GetInstanceTypeId() << " mean " << Flowmean << ", Bandwidth::type " << m_randomBWVariable->GetInstanceTypeId() << " mean " << BWmean);

	} else {
		// show output to user for clarity...
		NS_LOG_UNCOND(
				"FlowDuration::type " << m_flowStopDurationTimer->GetInstanceTypeId() << " constant " << TESTFLOWSTOPTIME_MILLISEC << "ms, Bandwidth::type " << m_randomBWVariable->GetInstanceTypeId() << " constant " << TESTFLOWBANDWIDTH);
	}
	// small change, if default ns3 is selected, display ns3Runtime
	double ns3RunTime, totalTime;
	if (simulationRunProperties::enableDefaultns3EnergyModel) {
		totalTime = ns3RunTime = simulationRunProperties::simStop;
	} else {
		totalTime = (m_totalTreeEnergy->m_totalTransmitTimeNanoSec
				+ m_totalTreeEnergy->m_totalSleepTimeNanoSec
				+ m_totalTreeEnergy->m_totalIdleTimeNanoSec) / NanoSec_to_Secs;
		ns3RunTime = totalTime / m_totalTreeEnergy->m_deviceCount;
	}

	double perCent = 0;
	if (m_totalTreeEnergy->m_FlowCount > 0)
		perCent = (double) ssUdpEchoClient::flows_dropped * 100
				/ (double) m_totalTreeEnergy->m_FlowCount;
	const char* nodeNameMaxQ = "";
	if (m_totalTreeEnergy->m_deviceMaxQdepth_atNode != -1)
		nodeNameMaxQ =
				DynamicCast<ssNode>(
						allNodes.Get(
								m_totalTreeEnergy->m_deviceMaxQdepth_atNode))->nodeName;
	NS_LOG_UNCOND(
			"m_deviceCount [" << m_totalTreeEnergy->m_deviceCount << "] device_SetQ_depth [" << m_totalTreeEnergy->m_deviceSetQdepth << "] m_deviceMaxQdepth [" << m_totalTreeEnergy->m_deviceMaxQdepth << "] at Node [" << m_totalTreeEnergy->m_deviceMaxQdepth_atNode << "] [" << nodeNameMaxQ << "]");
	// sanjeev Jun 13
	NS_LOG_UNCOND(
			"m_totalConsumedEnergyJ [" << m_totalTreeEnergy->m_totalConsumedEnergy_nJ/ NanoSec_to_Secs << "] Total Power(W) " << m_totalTreeEnergy->m_totalConsumedEnergy_nJ / (simulationRunProperties::simStop * NanoSec_to_Secs) << " totalEnergyConsumedByNodeFabric[J] " << m_totalTreeEnergy->m_totalConsumedEnergyByNodeFabricONLY_nJ/NanoSec_to_Secs);
	NS_LOG_UNCOND(
			"m_totalTransmitTimeEnergyJ [" << m_totalTreeEnergy->m_totalTransmitTimeEnergy_nJ/ NanoSec_to_Secs << "]");
	NS_LOG_UNCOND(
			"m_totalIdleTimeEnergyJ [" << m_totalTreeEnergy->m_totalIdleTimeEnergy_nJ/NanoSec_to_Secs << "]");
	NS_LOG_UNCOND(
			"m_totalSleepStateTimeEnergyJ [" << m_totalTreeEnergy->m_totalSleepStateTimeEnergy_nJ/ NanoSec_to_Secs << "]");
	NS_LOG_UNCOND(
			"m_totalTime(sec) [" << totalTime << "] idle time(sec)[" << m_totalTreeEnergy->m_totalIdleTimeNanoSec/NanoSec_to_Secs << "] sleep time(sec) [" << m_totalTreeEnergy->m_totalSleepTimeNanoSec/NanoSec_to_Secs << "] txn time(sec)[" << m_totalTreeEnergy->m_totalTransmitTimeNanoSec/NanoSec_to_Secs << "]");
	NS_LOG_UNCOND(
			"m_AvgDelay [" << m_totalTreeEnergy->m_AvgDelay << "] m_AvgThroughPut [" << m_totalTreeEnergy->m_AvgThroughPut << "Mbps] FlowsExecuted [" << m_totalTreeEnergy->m_FlowCount << "]");
	NS_LOG_UNCOND(
			"m_OfferedThroughput [" << m_totalTreeEnergy->m_AvgOfferedThroughPut << "Mbps] m_ConsumedThroughput [" << m_totalTreeEnergy->m_AvgConsumedThroughPut << "Mbps] ns3:RunTime [" << ns3RunTime << "s] GivenSimTime [" << simulationRunProperties::simStop << "s]\n");
	NS_LOG_UNCOND(
			"m_lostPacketCount_inQ [" << m_totalTreeEnergy->m_lostPacketCount << "] m_FlowsWithLostPacket_inQ [" << m_totalTreeEnergy->m_FlowsWithLostPacket << "] Dropped_Flows [" << ssUdpEchoClient::flows_dropped << "] about [" << perCent << "%] of flows\n");

	if (simulationRunProperties::enableVMC) {
		double hwlsrcEfficiency = (
				m_hwlDataMetric.totalSrcRequests > 0 ?
						m_hwlDataMetric.totalSrcRequestSuccess * 100
								/ m_hwlDataMetric.totalSrcRequests :
						0);
		double hwldstEfficiency = (
				m_hwlDataMetric.totalDstRequests > 0 ?
						m_hwlDataMetric.totalDstRequestSuccess * 100
								/ m_hwlDataMetric.totalDstRequests :
						0);
		NS_LOG_UNCOND(
				"TotalBW sent [" << m_totalBW << "Mbps] Avg BW/flow [" << double(m_totalBW)/double(m_totalTreeEnergy->m_FlowCount) << "] Avg BW/flow/host(link) [" << double(m_totalBW)/double(m_totalTreeEnergy->m_FlowCount)/double(hosts.GetN()) << "]");
		NS_LOG_UNCOND(
				"HostWeightList Source: Requests [" << m_hwlDataMetric.totalSrcRequests << "] Success [" << m_hwlDataMetric.totalSrcRequestSuccess << "] Efficiency [" << hwlsrcEfficiency << "%] HWL_Src_Failed [" << m_hwlDataMetric.totalSrcRequestFailed << "]");NS_LOG_UNCOND(
				"HostWeightList Destination: Requests [" << m_hwlDataMetric.totalDstRequests << "] Success [" << m_hwlDataMetric.totalDstRequestSuccess << "] Efficiency [" << hwldstEfficiency << "%] HWL_Dst_Failed [" << m_hwlDataMetric.totalDstRequestFailed << "]" << " Equal=" << m_hwlDataMetric.totalSrcHostEqual);
	}
	//  Metrics for NonHWL:: Dst & SRc:: Requests + Failure, Apr 23 , sanjeev
	NS_LOG_UNCOND(
			"Non-HostWeightList Source: Requests [" << m_hwlDataMetric.totalNonHWL_srcRequests << "] Failed [" << m_hwlDataMetric.totalNonHWL_srcRequestFailed << "]");
	NS_LOG_UNCOND(
			"Non-HostWeightList Destination: Requests [" << m_hwlDataMetric.totalNonHWL_dstRequests << "] Failed [" << m_hwlDataMetric.totalNonHWL_dstRequestFailed << "]");

	if (simulationRunProperties::enableMarkovModel) {
		NS_LOG_UNCOND(
				"Markov model parameters: Time [" << simulationRunProperties::markovStateInterval << "ms] eta0 [" << simulationRunProperties::markovETA0 << "] eta1 [" << simulationRunProperties::markovETA1 << "]");
		NS_LOG_UNCOND(
				"Markov metrics: T0 time [" << m_MarkovModelVariable.metric_time_inT0state << "ms] T0 flows [" << m_MarkovModelVariable.metric_flows_inT0state << "] T0 total flow duration [" << m_MarkovModelVariable.metric_totalFlowDuration_inT0time << "ms] T0 avg. flow duration [" << m_MarkovModelVariable.metric_totalFlowDuration_inT0time/m_MarkovModelVariable.metric_flows_inT0state << "ms]");
// Sanjeev Apr21
		NS_LOG_UNCOND(
				"Markov metrics: T1 time [" << m_MarkovModelVariable.metric_time_inT1state << "ms] T1 flows [" << m_MarkovModelVariable.metric_flows_inT1state << "] T1 total flow duration [" << m_MarkovModelVariable.metric_totalFlowDuration_inT1time << "ms] T1 avg. flow duration [" << m_MarkovModelVariable.metric_totalFlowDuration_inT1time/m_MarkovModelVariable.metric_flows_inT1state << "ms]");NS_LOG_UNCOND(
				"Markov metrics: State Changes [" << m_MarkovModelVariable.metric_total_state_changes << "] Total flows [" << m_MarkovModelVariable.metric_flows_inT0state + m_MarkovModelVariable.metric_flows_inT1state << "] Total flow duration [" << m_MarkovModelVariable.metric_totalFlowDuration_inT0time + m_MarkovModelVariable.metric_totalFlowDuration_inT1time << "ms]");
	}
//write to file...
	WriteFinalTestOutputToFile();
}

/*************************************************************/
BaseTopology::BaseTopology(void) {
	NS_LOG_FUNCTION(this);
	m_anim = 0;
	sleepTimeEnergyCounterId = 0;
	totalEnergyCounterId = 0;
	leakageEnergyCounterId = 0;
	activeTimeCounterId = 0;
	sleepTimeCounterId = 0;
	idleTimeCounterId = 0;
	packetCounterId = 0;
	sleepStateCounterId = 0;
	activeEnergyCounterId = 0;
	deviceCountCounterId = 0;
	m_EnergyMeterUpdateIntervalSec = Seconds(
	DefaultEnergyMeterUpdateIntervalSec);
	m_simStopTime = Seconds(simulationRunProperties::simStop);
	m_internalClientStopTime = Seconds(
			simulationRunProperties::simStop * CLIENTSTOPTHRESHOLD);
	m_totalTreeEnergy = new TopologyDataCollected;
	m_totalTreeEnergy->m_totalConsumedEnergy_nJ = 0;
	// sanjeev Jun 13
	m_totalTreeEnergy->m_totalConsumedEnergyByNodeFabricONLY_nJ = 0;
	m_totalTreeEnergy->m_totalIdleTimeEnergy_nJ = 0;
	m_totalTreeEnergy->m_totalTransmitTimeEnergy_nJ = 0;
	m_totalTreeEnergy->m_totalSleepStateTimeEnergy_nJ = 0;
	m_totalTreeEnergy->m_totalTransmitTimeNanoSec = 0;
	m_totalTreeEnergy->m_totalSleepTimeNanoSec = 0;
	m_totalTreeEnergy->m_totalIdleTimeNanoSec = 0;
	m_totalTreeEnergy->m_deviceCount = 0;
	m_totalTreeEnergy->m_deviceMaxQdepth = 0; // sanjeev Apr 4- added for display..
	m_totalTreeEnergy->m_deviceMaxQdepth_atNode = -1;
	m_totalTreeEnergy->m_DeviceTotalObservedQueueDepth = 0;		// May 14
	m_totalTreeEnergy->m_AvgLatency = Seconds(0);
	m_totalTreeEnergy->m_AvgOfferedThroughPut = 0;
	m_totalTreeEnergy->m_AvgConsumedThroughPut = 0;
	m_totalTreeEnergy->m_FlowCount = 0;
	m_totalTreeEnergy->m_lostPacketCount = 0;
	m_totalTreeEnergy->m_FlowsWithLostPacket = 0;
//
	m_MarkovModelVariable.tempETA0 = 0;
	m_MarkovModelVariable.tempETA1 = 0;
	m_MarkovModelVariable.m_IsT0_State = true;
	m_MarkovModelVariable.metric_total_state_changes = 0;
	m_MarkovModelVariable.metric_flows_inT0state =
			m_MarkovModelVariable.metric_flows_inT1state = 0;
	m_MarkovModelVariable.metric_time_inT0state =
			m_MarkovModelVariable.metric_time_inT1state = 0;
	m_MarkovModelVariable.metric_totalFlowDuration_inT0time =
			m_MarkovModelVariable.metric_totalFlowDuration_inT1time = 0;
	m_MarkovModelVariable.tempNextStateChangeAt = 0;
// my short-cut to generate IPv4Address
	ipv4Address.sn1 = 0; // un-used
	ipv4Address.sn2 = 10;
	ipv4Address.sn3 = 0;
	ipv4Address.sn4 = 0;
// metrics
	m_flowCount = m_totalBW = m_totalStoppedFlowCounter = 0;
	m_SawToothModel.m_IsPhase0_or_Phase1_State = false; // Apr 25 extended sawtooth-sanjeev
	m_SawToothModel.m_total_flows_to_inject = 0;
	m_SawToothModel.m_total_flows_to_stop = 0;
	m_SawToothModel.m_thisStoppedFlowCounter = 0;
	BaseTopology::m_flowData = (FlowDataCollected *) malloc(
			sizeof(FlowDataCollected) *
			MAXFLOWDATA_TO_COLLECT);
	m_hwlDataMetric.flowid = m_hwlDataMetric.totalSrcHostEqual = 0;
	m_hwlDataMetric.totalDstRequestSuccess = m_hwlDataMetric.totalDstRequests =
			m_hwlDataMetric.totalDstRequestFailed =
					m_hwlDataMetric.totalDstRequestNotCorrect = 0;
	m_hwlDataMetric.totalSrcRequestSuccess = m_hwlDataMetric.totalSrcRequests =
			m_hwlDataMetric.totalSrcRequestFailed =
					m_hwlDataMetric.totalDstRequestNotCorrect = 0;
	m_hwlDataMetric.totalNonHWL_dstRequestFailed =
			m_hwlDataMetric.totalNonHWL_srcRequestFailed = 0;
	m_hwlDataMetric.totalNonHWL_dstRequests =
			m_hwlDataMetric.totalNonHWL_srcRequests = 0;
	t_a = t_b = t_x = t_i = t_reqBW = 0;
}

/**************************************************************/
void BaseTopology::DoRun(void) {
	NS_LOG_FUNCTION(this);
	Ipv4GlobalRoutingHelper::PopulateRoutingTables();

	EnableAllStatisticsAllNodes();

	Simulator::Stop(m_simStopTime);
	Simulator::Run();

	CollectAllStatisticsAllNodes();

	Simulator::Destroy();
}

} // namespace
