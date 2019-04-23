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
#include "parameters.h"
#include <cmath>

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

uint32_t total_hosts_in_system = (SSD_PER_RACK + 1) * (simulationRunProperties::k/2) * (simulationRunProperties::k/2) * simulationRunProperties::k;

Ipv4Address* BaseTopology::hostTranslation = new Ipv4Address[total_hosts_in_system+1];

uint32_t* BaseTopology::hostaddresslogicaltophysical = new uint32_t[total_hosts_in_system +1 ];

uint32_t BaseTopology::num_of_retried_packets = 0;

uint32_t BaseTopology::num_of_retired_packets_specific_node = 0;

//uint32_t** BaseTopology::chunk_assignment_to_applications = new uint32_t*[simulationRunProperties::total_applications + 1]; //application and which chunks

uint32_t** BaseTopology::chunk_assignment_to_applications = new uint32_t*[simulationRunProperties::total_applications + 1 + DEFAULT_NUMBER_OF_DUMMY_APPLICATIONS]; //application and which chunks

uint32_t** BaseTopology::application_assignment_to_node = new uint32_t*[total_hosts_in_system];

//double** BaseTopology::chunk_assignment_probability_to_applications = new double*[simulationRunProperties::total_applications + 1];

double** BaseTopology::chunk_assignment_probability_to_applications = new double*[simulationRunProperties::total_applications + 1 +DEFAULT_NUMBER_OF_DUMMY_APPLICATIONS];

uint32_t *BaseTopology::virtual_to_absolute_mapper = new uint32_t[simulationRunProperties::total_chunk];

Ptr<RandomVariableStream> BaseTopology::application_selector = CreateObject<UniformRandomVariable>();

double BaseTopology::popularity_change_simulation_interval = simulationRunProperties::popularity_change_start_ms;

Ptr<RandomVariableStream> BaseTopology::Popularity_Change_Random_Variable = CreateObject<UniformRandomVariable>();

Ptr<RandomVariableStream> BaseTopology::chunk_copy_selection = CreateObject<UniformRandomVariable>();

//ApplicationMetrics *BaseTopology::application_metrics = new ApplicationMetrics[simulationRunProperties::total_applications];

ApplicationMetrics *BaseTopology::application_metrics = new ApplicationMetrics[simulationRunProperties::total_applications + DEFAULT_NUMBER_OF_DUMMY_APPLICATIONS];

uint32_t BaseTopology::phrase_change_counter = 0;

uint32_t *BaseTopology::phrase_change_interval;

double *BaseTopology::phrase_change_intensity_value;

double BaseTopology::intensity_change_simulation_interval = simulationRunProperties::intensity_change_start_ms;

uint32_t BaseTopology::total_phrase_changed = 0;

double BaseTopology::intensity_change_scale = 1.0;

Pod* BaseTopology::p;

Pod* BaseTopology::q;

int BaseTopology::counter_=0;

int BaseTopology::Incrcounter_=0;

bool BaseTopology::createflag=false;

chunkCopy* BaseTopology::chnkCopy;

nodedata* BaseTopology::nodeU;

nodedata* BaseTopology::nodeOldUtilization;

Result * BaseTopology::res;

double BaseTopology::sum_delay_ms = 0.0;

double BaseTopology::sum_storage_delay = 0.0;

uint64_t BaseTopology::total_packet_count = 0;

uint64_t BaseTopology::total_packet_count_inc = 0;

uint32_t *BaseTopology::host_assignment_round_robin_counter = new uint32_t[(simulationRunProperties::k*simulationRunProperties::k*simulationRunProperties::k)/4];

double *BaseTopology::host_running_avg_bandwidth = new double[total_hosts_in_system + 1];

uint32_t *BaseTopology::host_running_avg_counter = new uint32_t[total_hosts_in_system + 1];

//double *BaseTopology::application_probability = new double[simulationRunProperties::total_applications];

double *BaseTopology::application_probability = new double[simulationRunProperties::total_applications + DEFAULT_NUMBER_OF_DUMMY_APPLICATIONS];

uint32_t BaseTopology::current_number_of_flows = 0;

uint32_t *BaseTopology::total_packets_to_hosts_bits = new uint32_t[total_hosts_in_system];

//uint32_t *BaseTopology::total_packets_to_hosts_bits_old = new uint32_t[total_hosts_in_system];

uint32_t BaseTopology::total_events = 0;

uint32_t BaseTopology::total_events_learnt = 0;

uint32_t *BaseTopology::total_packets_sent_to_chunk = new uint32_t[simulationRunProperties::total_chunk];


//uint32_t *BaseTopology::total_packets_to_chunk = new uint32_t[simulationRunProperties::total_chunk];

uint32_t *BaseTopology::total_packets_to_chunk_destination = new uint32_t[simulationRunProperties::total_chunk];

double BaseTopology::last_point_of_entry = 0.0;

double BaseTopology::total_sum_of_entry = 0.0;

double BaseTopology::sum_of_number_time_packets = 0.0;

uint32_t *BaseTopology::chunk_version_tracker = new uint32_t[simulationRunProperties::total_chunk + 1];

uint32_t *BaseTopology::chunk_reference_version_tracker = new uint32_t[simulationRunProperties::total_chunk + 1];

uint32_t** BaseTopology::chunk_version_node_tracker = new uint32_t* [simulationRunProperties::total_chunk];

bool** BaseTopology::chunk_copy_node_tracker = new bool* [simulationRunProperties::total_chunk];

uint32_t BaseTopology::consistency_flow = 0;

uint32_t BaseTopology::total_consistency_packets = 0;

uint32_t BaseTopology::total_non_consistency_packets = 0;

NodeContainer BaseTopology::hosts_static;

double BaseTopology::sum_delay_ms_burst = 0.0;

uint32_t BaseTopology::total_events_learnt_burst = 0;

double BaseTopology::sum_delay_ms_no_burst = 0.0;

uint32_t BaseTopology::total_events_learnt_no_burst = 0;

uint32_t BaseTopology::sleeping_nodes = 0;

uint32_t BaseTopology::max_chunk_by_application = 0;

uint32_t BaseTopology::total_number_of_packet_for_copy_creation=0;

uint32_t** BaseTopology::transaction_rollback_packets = new uint32_t*[total_hosts_in_system];

uint32_t** BaseTopology::transaction_rollback_write_tracker = new uint32_t *[total_hosts_in_system];

uint32_t BaseTopology::rollback_packets = 0;

double BaseTopology::tail_latency=0.0;

double *BaseTopology::host_utilization_outgoing = new double[total_hosts_in_system+1];
uint32_t *BaseTopology::host_copy = new uint32_t [total_hosts_in_system+1];

bool BaseTopology::write_flow_tail=false;

int BaseTopology::copy_deleted=0;

int BaseTopology::copy_created=0;

int BaseTopology::copy_moved=0;

uint32_t BaseTopology::totalWriteCount=0;

uint32_t BaseTopology::pkt_sent_during_phase1=0;

uint32_t BaseTopology::pkt_sent_during_phase2=0;

uint32_t BaseTopology::pkt_sent_during_phase3=0;

uint32_t BaseTopology::pkt_rcv_during_phase1=0;

uint32_t BaseTopology::pkt_rcv_during_phase2=0;

uint32_t BaseTopology::pkt_rcv_during_phase3=0;

uint32_t** BaseTopology::distance_matrix = new uint32_t* [total_hosts_in_system];

uint32_t** BaseTopology::distance_node = new uint32_t* [total_hosts_in_system];

std::map<std::string, TimeStampTracker> BaseTopology::concurrency_tracker;


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



double BaseTopology::getMinUtilizedServerInRack(uint32_t rack_id)
{
	uint32_t rack_host_id = ((SSD_PER_RACK +1 ) * rack_id) + 1 ;

	uint32_t min_host_utilization = Ipv4GlobalRouting::host_utilization[rack_host_id];

	//uint32_t expected_host = rack_host_id;

	for(uint32_t i= 0; i<SSD_PER_RACK;i++)
	{
		if(Ipv4GlobalRouting::host_utilization[rack_host_id + i] < min_host_utilization)
		{
			//expected_host = rack_host_id + i;
			min_host_utilization = Ipv4GlobalRouting::host_utilization[rack_host_id + i];
		}
	}
	//NS_LOG_UNCOND("host_id "<<rack_id<<" expected_host "<<expected_host);

	return min_host_utilization;
}




/*Result **/void BaseTopology::calculateNewLocation(int incrDcr)
{
//Parameters
	float cuttoffnode_high=(int(Count)*int(SSD_PER_RACK))*0.75 ;//400;
	float cuttoffnode_real=(int(Count)*int(SSD_PER_RACK))*0.45;// 240;
	double time_window_following_another_create=1000.0; //1000
	double time_window_following_another_delete=1000.0; //1000
	double time_window_delete=3000.0; //1500
//	float delta=.075;
	float alpha =.85; //for smoothing
	float theta =.4;//.6; //for picking up only significant chunks
	float theta2 =.01;
	bool energy = true;
//Parameters

	uint32_t number_of_hosts = (uint32_t)(Ipv4GlobalRouting::FatTree_k * Ipv4GlobalRouting::FatTree_k * Ipv4GlobalRouting::FatTree_k)/ 4;
	uint32_t nodes_in_pod = number_of_hosts / Ipv4GlobalRouting::FatTree_k;
	float cuttoffpod=nodes_in_pod*cuttoffnode_real; //this has to be made dynamic
	float max_node_u=0.0;
	uint32_t max_node_no=99999;
	uint32_t max_pod=999999;
	uint32_t min_node_no=0;
	uint32_t min_pod=0;
	uint32_t r=0;
	uint32_t res_index=0;
	double time_now=Simulator::Now().ToDouble(Time::MS);
	NS_LOG_UNCOND("enetering"<<time_now);


	//copying the entire data set:


	//*BaseTopology::q=*BaseTopology::p;


	for (int i=0;i<Ipv4GlobalRouting::FatTree_k;i++)
	{
		BaseTopology::q[i].pod_number=BaseTopology::p[i].pod_number;
		BaseTopology::q[i].Pod_utilization=BaseTopology::p[i].Pod_utilization;
		BaseTopology::q[i].Pod_utilization_out=BaseTopology::p[i].Pod_utilization_out;
		for(uint32_t j=0;j<nodes_in_pod;j++)
		{
			BaseTopology::q[i].nodes[j].node_number=BaseTopology::p[i].nodes[j].node_number;
			BaseTopology::q[i].nodes[j].total_chunks=BaseTopology::p[i].nodes[j].total_chunks;
			BaseTopology::q[i].nodes[j].utilization=BaseTopology::p[i].nodes[j].utilization;
			BaseTopology::q[i].nodes[j].utilization_out=BaseTopology::p[i].nodes[j].utilization_out;
//			BaseTopology::q[i].nodes[j].max_capacity_left=BaseTopology::p[i].nodes[j].max_capacity_left;

			 for(uint32_t k = 0 ;k < BaseTopology::p[i].nodes[j].total_chunks;k++)
			{
				 BaseTopology::q[i].nodes[j].data[k].chunk_number= BaseTopology::p[i].nodes[j].data[k].chunk_number;
				 BaseTopology::q[i].nodes[j].data[k].intensity_sum= BaseTopology::p[i].nodes[j].data[k].intensity_sum;
				 BaseTopology::q[i].nodes[j].data[k].intensity_sum_out= BaseTopology::p[i].nodes[j].data[k].intensity_sum_out;
				 BaseTopology::q[i].nodes[j].data[k].chunk_count= BaseTopology::p[i].nodes[j].data[k].chunk_count;
				 BaseTopology::q[i].nodes[j].data[k].processed= BaseTopology::p[i].nodes[j].data[k].processed;
				 BaseTopology::q[i].nodes[j].data[k].highCopyCount= BaseTopology::p[i].nodes[j].data[k].highCopyCount;
				 BaseTopology::q[i].nodes[j].data[k].emerCopyCount= BaseTopology::p[i].nodes[j].data[k].emerCopyCount;

			}

		}
	}




	if(BaseTopology::createflag!=true)
	{
	for (int i=0;i<Ipv4GlobalRouting::FatTree_k;i++)
	{
		for(uint32_t j=0;j<nodes_in_pod;j++)
		{
			//NS_LOG_UNCOND("--------------------------"<<BaseTopology::q[i].nodes[j].total_chunks);
			for(uint32_t q=0;q<BaseTopology::q[i].nodes[j].total_chunks;q++)
			{
				uint32_t  chunk_num=BaseTopology::q[i].nodes[j].data[q].chunk_number;

			//	NS_LOG_UNCOND("--------------------------"<<chunk_num<<"bandwidth "<<BaseTopology::q[i].nodes[j].data[q].intensity_sum);
				//uint32_t copy=0;
				//while (copy<BaseTopology::q[i].nodes[j].data[q].chunk_count)
				//{
					BaseTopology::chnkCopy[chunk_num].exists[BaseTopology::chnkCopy[chunk_num].count]=j+i*Ipv4GlobalRouting::FatTree_k;
					BaseTopology::chnkCopy[chunk_num].count++;
					//copy++;
				//}


			}
		}
	}


		for (int i=0;i<Ipv4GlobalRouting::FatTree_k;i++)
			{
				for(uint32_t j=0;j<nodes_in_pod;j++)
				{
					//nodeU[r].U=BaseTopology::q[i].nodes[j].utilization; //mark
					nodeU[r].U=BaseTopology::q[i].nodes[j].utilization_out; //unmark
					nodeU[r].nodenumber=j+i*Ipv4GlobalRouting::FatTree_k;
					nodeOldUtilization[r].nodenumber=j+i*Ipv4GlobalRouting::FatTree_k;
					nodeOldUtilization[r].U=0.0;
					nodeOldUtilization[r].counter=0;
					r++;
				}
		}

		BaseTopology::createflag=true;
	}



//---------------------------------------Smoothing----------------------------------------------------------------------------------
    uint32_t k;

	for (int i=0;i<Ipv4GlobalRouting::FatTree_k;i++)
		{
		NS_LOG_UNCOND("------POD--------------------------"<<BaseTopology::q[i].Pod_utilization);
		NS_LOG_UNCOND("------POD Out--------------------------"<<BaseTopology::q[i].Pod_utilization_out);
			for(uint32_t j=0;j<nodes_in_pod;j++)
			{
				int nod=j+i*Ipv4GlobalRouting::FatTree_k;
				for ( k=0;k<number_of_hosts;k++)
				{
					if (nod==nodeU[k].nodenumber)
						break;
				}
				//nodeU[k].U=(alpha*BaseTopology::q[i].nodes[j].utilization)+(nodeU[k].U*(1-alpha)); //mark
				nodeU[k].U=(alpha*BaseTopology::q[i].nodes[j].utilization_out)+(nodeU[k].U*(1-alpha)); //unmark
				NS_LOG_UNCOND("------NODE--------------------------"<<"node number:----"<<nod<<" utilization:----"<<BaseTopology::q[i].nodes[j].utilization<<" utilization out:----"<<BaseTopology::q[i].nodes[j].utilization_out);
				//nodeU[k].nodenumber=j+i*Ipv4GlobalRouting::FatTree_k;

			}
	}

	//---------------------------------------Sorting the nodes as per Utilization ascending order----------------------------------------------------------------------------------
	for (uint32_t i = 0; i < number_of_hosts ; i++)
	{
		for (uint32_t j = i + 1; j < number_of_hosts; j++)
		{
			if (nodeU[i].U > nodeU[j].U)
			{
				nodedata temp =  nodeU[i];
				nodeU[i]= nodeU[j];
				nodeU[j] = temp;
			}
		}
	}
//-------------------------------------------------------------------------------------------------------------------------------------------
	//Increase starts here
	 //find out the chunk maximum used within the highly used node
    if(incrDcr){
		uint32_t max_chunk_no;
		float max_chunk_u;
		uint32_t target_pod;
		uint32_t target_node;
		uint32_t max_chunk_index;
		float beyond_cutoff;
		float target_utilization=0.0;
		float estimated_utlization; //this is the utilization we exopect on each of the chunk after copy creation
		float diff_from_node_cutoff=0.0;
		float max_diff_from_node_cutoff=0.0;
   	 for(int i =number_of_hosts-1 ; i>=0; i--)
 	 {
       	target_pod=999;
       	target_node=999;
     	max_node_u=0.0;
     	max_node_no=9999;
     	max_chunk_no=9999;
        max_chunk_u=0;
        max_chunk_index=99999;
        uint32_t nodeN=nodeU[i].nodenumber;
        NS_LOG_UNCOND("PPPPPPPPPP current "<<nodeU[i].U<<" old "<<nodeOldUtilization[nodeN].U);
        if(nodeU[i].U<cuttoffnode_high)
#if LOCAL_COPY
                { //still look for any congested server within the rack and if found then place a copy locally
        			uint32_t pod = (uint32_t) floor((double) nodeN/ (double) Ipv4GlobalRouting::FatTree_k);
        			uint32_t node =	nodeN%Ipv4GlobalRouting::FatTree_k;
                	nodeOldUtilization[nodeN].U=nodeU[i].U;
                	uint32_t ssd_number=nodeN*(SSD_PER_RACK+1)+1;
                	//success=false; //per node one local copy permitted
        			//check if for the node any server violated the threshold lately
        			for(uint32_t k=ssd_number;k<ssd_number+SSD_PER_RACK;k++)
        			{
        			//	success=false;//per server one local copy permitted
        				if(/*Ipv4GlobalRouting::host_congestion_flag[k]==1 &&*/ BaseTopology::host_utilization_outgoing[k]>Count*.6) //while placing the last flow we have seen congestion at the server
        				{
        					//NS_LOG_UNCOND("we found pod"<<pod<<" node id "<<node);
        					//for those servers in the rack i need to know the chunk number on them and see if the rack utilization will be fine for local copy and alo
        					for(uint32_t q=0;q<BaseTopology::q[pod].nodes[node].total_chunks;q++)
        					{
        							uint32_t chunk_number=BaseTopology::q[pod].nodes[node].data[q].chunk_number;
        						//	NS_LOG_UNCOND("chunk_number"<<chunk_number);
        							uint32_t logical_node_id=BaseTopology::chunkTracker.at(chunk_number).logical_node_id;
        							if(logical_node_id==k)
        							{
        								//NS_LOG_UNCOND("we found a logical node id"<<rack_id<<" host id "<<logical_node_id<<" chunk number "<<chunk_number);
        //check for the existence of the chunk on that server while doing local placement
        								//check for the maximum chunk utilization and then repeat it until the utlization falls way behind the desired
        								if((time_now-BaseTopology::chnkCopy[chunk_number].last_deleted_timestamp_for_chunk)>time_window_following_another_delete
        									&& ((time_now-BaseTopology::chnkCopy[chunk_number].last_created_timestamp_for_chunk)>time_window_following_another_create) //not sure
        									&&  BaseTopology::chnkCopy[chunk_number].highCopyCount<Ipv4GlobalRouting::FatTree_k
        									&&  BaseTopology::q[pod].nodes[node].data[q].processed!=1
        									&&  BaseTopology::q[pod].nodes[node].data[q].intensity_sum_out>BaseTopology::host_utilization_outgoing[k]*.7)//unmark
        									{
        									if(/*BaseTopology::q[pod].nodes[node].max_capacity_left>BaseTopology::q[pod].nodes[node].data[q].intensity_sum
        										&&*/ BaseTopology::q[pod].nodes[node].data[q].intensity_sum_out+BaseTopology::q[pod].nodes[node].utilization_out<cuttoffnode_real)
        									{
        										NS_LOG_UNCOND("the host that satisfy utilization????????"<<BaseTopology::host_utilization_outgoing[k]<<"chunk number "<<chunk_number<<"chunk utilization "<< BaseTopology::q[pod].nodes[node].data[q].intensity_sum_out);
        										BaseTopology::res[res_index].src=nodeN;
        										BaseTopology::res[res_index].dest=nodeN;
        										BaseTopology::res[res_index].chunk_number=chunk_number;
        										res_index++;

        										BaseTopology::chnkCopy[chunk_number].exists[BaseTopology::chnkCopy[chunk_number].count]=nodeN;
        										BaseTopology::chnkCopy[chunk_number].count++;

        										BaseTopology::q[pod].nodes[node].data[q].highCopyCount++;
        										//this is the time stamp per chunk level- i.e. when under the whole system any copy of the chunk created
        										BaseTopology::chnkCopy[chunk_number].last_created_timestamp_for_chunk=time_now;
        										BaseTopology::chnkCopy[chunk_number].highCopyCount++;

        										//BaseTopology::q[pod].nodes[node].max_capacity_left=BaseTopology::q[pod].nodes[node].max_capacity_left-BaseTopology::q[pod].nodes[node].data[q].intensity_sum;

        										BaseTopology::q[pod].nodes[node].utilization_out=BaseTopology::q[pod].nodes[node].data[q].intensity_sum_out+BaseTopology::q[pod].nodes[node].utilization_out; //unmark
        									//	success=true;
        									}
        									//else //do we want to create a remote copy??

        									//anyways we have to mark this to be true
        									BaseTopology::q[pod].nodes[node].data[q].processed=1;
        									}
        							}
        						}
        					Ipv4GlobalRouting::host_congestion_flag[k]=0;
        					//for time_now create only one copy and break;
        				//	if(success)
        					//	break;
        				}
        				else
        				{
        					Ipv4GlobalRouting::host_congestion_flag[k]=0;
        				}
        			}

                }
#else
        break;
#endif

        else
		{
        	if(nodeU[i].U<nodeOldUtilization[nodeN].U)
				continue;
//			else if(nodeOldUtilization[nodeN].U!=0.0 && (nodeU[i].U>nodeOldUtilization[nodeN].U) && (nodeU[i].U-nodeOldUtilization[nodeN].U)/nodeOldUtilization[nodeN].U<delta)
//			{
//				nodeOldUtilization[nodeN].U=nodeU[i].U; // this node is being selected but not qualified so updating the utilizaton with the current and smoothed value
//				continue;
//			}
			//else
				//nodeOldUtilization[nodeN].U=nodeU[i].U;


        uint32_t pod = (uint32_t) floor((double) nodeN/ (double) Ipv4GlobalRouting::FatTree_k);
        max_node_no=nodeN%Ipv4GlobalRouting::FatTree_k;
        max_node_u=BaseTopology::q[pod].nodes[max_node_no].utilization_out;//nodeU[i].U; //unmark
		max_pod=pod;



		NS_LOG_UNCOND("The maximally used node is------------------&&&&&&&&&&------------------------------------------------------------------------- :"<<max_node_u<<" ::: max_node_no "<<max_node_no<<" ::: max_pod "<<max_pod<<" total chunk on node "<<BaseTopology::q[max_pod].nodes[max_node_no].total_chunks);
		beyond_cutoff=max_node_u-(cuttoffnode_high-(cuttoffnode_high*.3)); //changed cuttoffnode_high to cuttoffnode_real --Madhu
		target_utilization=0.0;
		NS_LOG_UNCOND("beyond_cutoff"<<beyond_cutoff);

//we are checking if a node is beyond the high range and below the emergency range
	    if(cuttoffnode_high<max_node_u /* && cuttoffnode_emer>max_node_u*/ && beyond_cutoff>0.0 )
	    {
	       while(target_utilization<beyond_cutoff)
	       {
	    	   max_chunk_no=9999;
	    	   max_chunk_u=0;
	    	   max_chunk_index=99999;
	    	   NS_LOG_UNCOND("here we are and the pod number , node number a :"<<max_pod<<"::::"<<max_node_no);
	    	   for(uint32_t i = 0; i<BaseTopology::q[max_pod].nodes[max_node_no].total_chunks; i++)
		      	 	 {

	    		   	   	 uint32_t l=BaseTopology::q[max_pod].nodes[max_node_no].data[i].chunk_number;
	    		   	   	 int deno=(BaseTopology::chnkCopy[l].readCount+BaseTopology::chnkCopy[l].writeCount);
	    		   	     float write_ratio=float(BaseTopology::chnkCopy[l].writeCount)/deno;
	    		   	     //write_ratio=std::ceil(write_ratio * 100) / 100;
	    		   	   	 NS_LOG_UNCOND("chunk number "<<l<<" utilization "<<BaseTopology::q[max_pod].nodes[max_node_no].data[i].intensity_sum_out<<" write ratio "<<write_ratio<<" write count compared to whole system "<<float(BaseTopology::chnkCopy[l].writeCount)/(BaseTopology::totalWriteCount));
	    		   	    if(/* BaseTopology::q[max_pod].nodes[max_node_no].data[i].highCopyCount<Ipv4GlobalRouting::FatTree_k
		      	 		     &&*/ (time_now-BaseTopology::chnkCopy[l].last_deleted_timestamp_for_chunk)>time_window_following_another_delete
							 && (time_now-BaseTopology::chnkCopy[l].last_created_timestamp_for_chunk)>time_window_following_another_create //not sure
		      	 		     && BaseTopology::chnkCopy[BaseTopology::q[max_pod].nodes[max_node_no].data[i].chunk_number].highCopyCount<Ipv4GlobalRouting::FatTree_k
		      	 		     && BaseTopology::q[max_pod].nodes[max_node_no].data[i].processed!=1
							 && max_chunk_u<BaseTopology::q[max_pod].nodes[max_node_no].data[i].intensity_sum_out //unmark
							 && BaseTopology::q[max_pod].nodes[max_node_no].data[i].intensity_sum_out>int(Count)*theta)
							 //&& float(BaseTopology::chnkCopy[l].writeCount)/(BaseTopology::totalWriteCount)<0.008)
							 //&& write_ratio<0.1)
							 //&& BaseTopology::q[max_pod].nodes[max_node_no].data[i].chunk_number>10
							 //&&  BaseTopology::q[max_pod].nodes[max_node_no].data[i].chunk_number<20) //unmark
	    		   	    	//&& BaseTopology::chnkCopy[l].writeCount/BaseTopology::chnkCopy[l].readCount<.3)
		      	 		 {
	    		   	    	 NS_LOG_UNCOND("BaseTopology::chnkCopy[l].read_count"<<BaseTopology::chnkCopy[l].readCount<<"BaseTopology::chnkCopy[l].write_count"<<BaseTopology::chnkCopy[l].writeCount<<" fraction "<<float(BaseTopology::chnkCopy[l].writeCount)/(BaseTopology::totalWriteCount)<< " Total copy in the system "<<BaseTopology::totalWriteCount);
		      	 			 max_chunk_no=BaseTopology::q[max_pod].nodes[max_node_no].data[i].chunk_number;
		      	 			 max_chunk_u = BaseTopology::q[max_pod].nodes[max_node_no].data[i].intensity_sum_out;//unmark
		      	 			 max_chunk_index=i;
		      	 			 NS_LOG_UNCOND("max chunk number "<<max_chunk_no<<" utilization "<<max_chunk_u);

		      	 		 }
		      	 	 }

		   int flag=0;
		   if(max_chunk_no!=9999)
		   {
			   NS_LOG_UNCOND("Found chunk utilization--------------------------------------------------------------------------------------max_chunk_no :"<<max_chunk_no<<" ::: max_pod "<<max_pod<<" ::::max_chunk_u "<<max_chunk_u<<" :::::totalchunk on the node"<<BaseTopology::q[max_pod].nodes[max_node_no].total_chunks);		  //last_max_chunk=max_chunk_no;

			if(energy){

			   max_chunk_u=BaseTopology::q[max_pod].nodes[max_node_no].data[max_chunk_index].intensity_sum_out/BaseTopology::q[max_pod].nodes[max_node_no].data[max_chunk_index].chunk_count; //unmark
			   estimated_utlization=max_chunk_u;//((max_chunk_u*BaseTopology::chnkCopy[max_chunk_no].count)/(BaseTopology::chnkCopy[max_chunk_no].count+1));
			   BaseTopology::q[max_pod].nodes[max_node_no].data[max_chunk_index].processed=1;

			   diff_from_node_cutoff=0.0;

			   max_diff_from_node_cutoff=0.0;
			   NS_LOG_UNCOND("estimated_utlization"<<estimated_utlization<<" total copy "<<BaseTopology::chnkCopy[max_chunk_no].count);
			  	 //Now split the load and assign to the nodes
			   printf("here$$%%%%%%%%%%%%%%%%%%%%%%%%$$$$$$$$$$$$$\n");
			   //We are deciding which pod is best here
			  	 for (int i=Ipv4GlobalRouting::FatTree_k-1;i>=0;i--)
			  		{
			  		 if(BaseTopology::q[i].Pod_utilization_out+estimated_utlization<cuttoffpod)//unmark
			  		 {

			  	 //selecting the destination node for the chunk
			  		 for(uint32_t j=0;j<nodes_in_pod;j++)
				     {
					 flag=0;
					 for(uint32_t q=0;q<BaseTopology::q[i].nodes[j].total_chunks;q++)
					 {
						 if(BaseTopology::q[i].nodes[j].data[q].chunk_number==max_chunk_no)
							 //already the chunk exists in the node so continue - Also the maximum node will not come as we are checking for validlity
							 {
								 flag=1;
								 break;
							 }
					 }


					 if(BaseTopology::q[i].nodes[j].utilization_out+estimated_utlization</*(cuttoffnode_high-(cuttoffnode_high*.3))*/ cuttoffnode_real  && !flag /*&& BaseTopology::q[i].nodes[j].utilization>0*/) //unmark
					 {
						 diff_from_node_cutoff=(int(Count)*int(SSD_PER_RACK))-BaseTopology::q[i].nodes[j].utilization_out;//+estimated_utlization); //unmark
						 //diff_from_node_cutoff=/*cuttoffnode_high */cuttoffnode_real-(BaseTopology::q[i].nodes[j].utilization_out+estimated_utlization); //unmark
						 //register benefit
						 printf("here---- diff_from_node_cutoff=%f\n",diff_from_node_cutoff);
						 if(max_diff_from_node_cutoff<diff_from_node_cutoff)
						 {
							 max_diff_from_node_cutoff=diff_from_node_cutoff;
							 target_node=j;
							 target_pod=i;
							 //update the values that you have with the U of the current chunk
							 NS_LOG_UNCOND("energy false target pod target node and target chunk "<<i<<j<< max_chunk_no);
						 }

					 }//end of if
				   }//end of node level for

			  }
		   }
		   }
		 else if (!energy){

			       max_chunk_u=BaseTopology::q[max_pod].nodes[max_node_no].data[max_chunk_index].intensity_sum_out/BaseTopology::q[max_pod].nodes[max_node_no].data[max_chunk_index].chunk_count;//unmark
				   estimated_utlization=((max_chunk_u*BaseTopology::chnkCopy[max_chunk_no].count)/(BaseTopology::chnkCopy[max_chunk_no].count+1));
				   BaseTopology::q[max_pod].nodes[max_node_no].data[max_chunk_index].processed=1;

				   diff_from_node_cutoff=0.0;

				   max_diff_from_node_cutoff=99999;

				   NS_LOG_UNCOND("estimated_utlization"<<estimated_utlization<<" total copy "<<BaseTopology::chnkCopy[max_chunk_no].count);
					 //Now split the load and assign to the nodes
				   printf("here%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n");
				   //We are deciding which pod is best here
					 for (int i=0; i<Ipv4GlobalRouting::FatTree_k;i++)
						{
						 if(BaseTopology::q[i].Pod_utilization_out+estimated_utlization<cuttoffpod) //unmark
						 {
						  NS_LOG_UNCOND("so we statisfy the node cutoff and we are here with the chunk number "<<max_chunk_no<<" pod number "<<i);
					 //selecting the destination node for the chunk
						 for(uint32_t j=0;j<nodes_in_pod;j++)
						 {
						  flag=0;
						  NS_LOG_UNCOND("node_in_pod"<<j<<"max_chunk_no"<<max_chunk_no);
						 for(uint32_t q=0;q<BaseTopology::q[i].nodes[j].total_chunks;q++)
						 {
							 if(BaseTopology::q[i].nodes[j].data[q].chunk_number==max_chunk_no)
								 //already the chunk exists in the node so continue - Also the maximum node will not come as we are checking for validlity
								 {
									 flag=1;
									 break;
								 }
						 }

						 if(BaseTopology::q[i].nodes[j].utilization+estimated_utlization<cuttoffnode_real && !flag)  //placement can lead to exceed the node level threshold
						 {

							 diff_from_node_cutoff=cuttoffnode_real-(BaseTopology::q[i].nodes[j].utilization_out+estimated_utlization); //unmark
							 //register benefit - looking for a node which has utilization less than the real cutoff but still very near to reall cutoff
							 if(max_diff_from_node_cutoff>diff_from_node_cutoff)
							 {
								 max_diff_from_node_cutoff=diff_from_node_cutoff;
								 target_node=j;
								 target_pod=i;
								 NS_LOG_UNCOND("target pod target node and target chunk "<<i<<j<< max_chunk_no);
								 //update the values that you have with the U of the current chunk
							 }

						 }//end of if
					   }//end of node level for

				  }
			   }
		  }

	}//end of if max chunk found
		   else
			   break; //no max chunk on the node satisfy all the requirements
			  	 //we have placed the item
			  	if(target_pod!=999 && target_node!=999)
			  	{

			  		BaseTopology::res[res_index].src=(max_pod*Ipv4GlobalRouting::FatTree_k)+max_node_no;
			  		BaseTopology::res[res_index].dest=(target_pod*Ipv4GlobalRouting::FatTree_k)+target_node;
			  		BaseTopology::res[res_index].chunk_number=max_chunk_no;


					target_utilization=target_utilization+estimated_utlization;
					BaseTopology::q[target_pod].nodes[target_node].utilization_out=BaseTopology::q[target_pod].nodes[target_node].utilization_out+estimated_utlization; //crtitical //unmark
					BaseTopology::q[target_pod].Pod_utilization_out=BaseTopology::q[target_pod].Pod_utilization_out+estimated_utlization;//crtitical //unmark
					uint32_t location=BaseTopology::chnkCopy[max_chunk_no].count;
					if (BaseTopology::chunkTracker.at(BaseTopology::res[res_index].chunk_number).copy_vs_move==1)
					 //if it is copy then we have to increase the number of copies, for the move we always update the current copy
						BaseTopology::chnkCopy[max_chunk_no].count=BaseTopology::chnkCopy[max_chunk_no].count+1;

					BaseTopology::chnkCopy[max_chunk_no].exists[location]=((target_pod*Ipv4GlobalRouting::FatTree_k)+target_node);
					BaseTopology::q[max_pod].nodes[max_node_no].data[max_chunk_index].highCopyCount=BaseTopology::q[max_pod].nodes[max_node_no].data[max_chunk_index].highCopyCount+1;
					BaseTopology::chnkCopy[max_chunk_no].last_created_timestamp_for_chunk=time_now; //this is actually last created or moved timestamp
					BaseTopology::chnkCopy[max_chunk_no].highCopyCount=BaseTopology::chnkCopy[max_chunk_no].highCopyCount+1;

					//nodeN is the node for which we did increase
					nodeOldUtilization[nodeN].U=max_node_u; //repeated
					res_index++;

			  	    target_pod=999;
			  	    target_node=999;
			  	    diff_from_node_cutoff=0;
			  	    max_diff_from_node_cutoff=0;
			  	    max_chunk_no=9999;
			  	    max_chunk_u=0;
			  	    max_chunk_index=99999;

			  	}

	       }//end of while

	      }//end of if for difference from cutoff
        //for time_now we are not creating copies just looking for an odd situation
	else
		break; //no break before we exhaust x% of the busy item in the node
 	  } //end of else condition where node cutoff is exceeded
 	 }//end of for

   	//printf("res_index%d\n",res_index);
   	BaseTopology::res[res_index].src=99999;
   	BaseTopology::res[res_index].dest=99999;
   	BaseTopology:: res[res_index].chunk_number=99999;


    }//end of if incrdcr


    //Decrease starts here-----------------------------------------------------------------------------------------------------------------
        else //if(time_now>4000.00)
        {
    		uint32_t target_pod=999;
    		uint32_t target_node=999;
    		uint32_t loc=0;
    		uint32_t min_chunk_no=9999;
    		float min_chunk_u=9999.0;
    		uint32_t min_chunk_index=9999;
    		uint32_t number_of_chunk_to_process=1;
    		uint32_t processed_unit=0;
    		float target_u;
    		float utilizationOnEach;
    		bool flag=false;
    		float min_node_u;
    		uint32_t last_created_location;
    		uint32_t last_created_at_node;
    		uint32_t last_created_at_pod;

    //here the major for loop starts
        for(uint32_t i = 0; i<number_of_hosts; i++)
    	 {
    		target_pod=999;
    		target_node=999;
    		min_node_no=99999;
    		min_pod=9999;
    		min_chunk_no=1000000;
    		min_chunk_u=9999.0;
    		uint32_t nodeN=nodeU[i].nodenumber;
    		NS_LOG_UNCOND("HHHHHHHHHHHH\n");
    //		if(nodeU[i].U > cuttoffnode_low) //changed cuttoffnode_low to cuttoffnode_real  --Madhu
    //			break;
    //		else if(nodeU[i].U<=0)
    //			continue;
    	  uint32_t pod = (uint32_t) floor((double) nodeN/ (double) Ipv4GlobalRouting::FatTree_k);

    	 if(BaseTopology::q[pod].nodes[nodeN%Ipv4GlobalRouting::FatTree_k].utilization_out>0) //unmark
    	 {

    		 min_node_no=nodeN%Ipv4GlobalRouting::FatTree_k;
    		 min_pod=pod;
    		 min_node_u=BaseTopology::q[min_pod].nodes[min_node_no].utilization_out; //unmark

            NS_LOG_UNCOND("The minimally used node is-----------------------------------------------------------------------------------------: "<<min_node_no<<" ::: "<<min_pod<<" minimu node utilization"<<min_node_u);
            processed_unit=0;
        	NS_LOG_UNCOND("the total number of chunks on the node"<<BaseTopology::q[min_pod].nodes[min_node_no].total_chunks);
        	while(number_of_chunk_to_process>processed_unit)
        	{
        		min_chunk_u=99999;
    			min_chunk_index=1000000;
    			min_chunk_no=1000000;


    		 for(uint32_t j = 0; j<BaseTopology::q[min_pod].nodes[min_node_no].total_chunks; j++)
    		 {

    			 uint32_t l=BaseTopology::q[min_pod].nodes[min_node_no].data[j].chunk_number;

    			 if( BaseTopology::chnkCopy[l].count>1
    					 && (time_now-BaseTopology::chnkCopy[l].last_created_timestamp_for_chunk)>time_window_delete
    					 && BaseTopology::q[min_pod].nodes[min_node_no].data[j].processed!=1
    					 && BaseTopology::q[min_pod].nodes[min_node_no].data[j].intensity_sum_out>0 //unmark
    					 && BaseTopology::q[min_pod].nodes[min_node_no].data[j].intensity_sum_out<min_node_u*theta2)
    			 {
    				 if(min_chunk_no==1000000) //this is only for the first time per node
    				 {
    			        min_chunk_no=BaseTopology::q[min_pod].nodes[min_node_no].data[j].chunk_number;
    			        min_chunk_u = BaseTopology::q[min_pod].nodes[min_node_no].data[j].intensity_sum_out; //unmark
    			        min_chunk_index=j;
    				 }
    			     else if(energy==false && min_chunk_no!=1000000 && min_chunk_u<BaseTopology::q[min_pod].nodes[min_node_no].data[j].intensity_sum_out)//unmark
    				 {
    					min_chunk_no=BaseTopology::q[min_pod].nodes[min_node_no].data[j].chunk_number;
    					min_chunk_u = BaseTopology::q[min_pod].nodes[min_node_no].data[j].intensity_sum_out;//unmark
    					min_chunk_index=j;
    				 }

    			     else if(energy==true && min_chunk_no!=1000000 && min_chunk_u>BaseTopology::q[min_pod].nodes[min_node_no].data[j].intensity_sum_out)//unmark
    				 {
    					min_chunk_no=BaseTopology::q[min_pod].nodes[min_node_no].data[j].chunk_number;
    					min_chunk_u = BaseTopology::q[min_pod].nodes[min_node_no].data[j].intensity_sum_out;//unmark
    					min_chunk_index=j;
    				 }

    			 }
    		 }
    		 NS_LOG_UNCOND("We came out clean-----"<<min_chunk_no<<"utilization"<<min_chunk_u);
    		 //if minimum chunk is possible to find then we decide how where to place it to based on energy management or congestion control
    		 if(min_chunk_no!=1000000)
    		 {
    			 if(!energy)
    			 {
    				 BaseTopology::q[min_pod].nodes[min_node_no].data[min_chunk_index].processed=1;
    				 NS_LOG_UNCOND("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@Found chunk utilization---------------------------------------------------- :"<<min_chunk_no<<"::::: "<<min_chunk_u);
    				 //based on the mean  utilization of the minimum chunk calculate the utilization of the rest

    				 //find the location of the minimum chunk
    				 if(min_chunk_u>1)
    					 utilizationOnEach=((BaseTopology::chnkCopy[min_chunk_no].count )*min_chunk_u)/(BaseTopology::chnkCopy[min_chunk_no].count-1);
    				 else
    					 {
    					 	 utilizationOnEach=0.0;
    					 	  min_chunk_u=0.0;
    					 }
    				 for (uint32_t n = 0; n<BaseTopology::chnkCopy[min_chunk_no].count; n++)
    				 {  uint32_t node_=BaseTopology::chnkCopy[min_chunk_no].exists[n];
    					uint32_t pod_node=node_/Ipv4GlobalRouting::FatTree_k;
    					node_=node_%Ipv4GlobalRouting::FatTree_k;

    					if(node_==min_node_no
    							&& pod_node==min_pod)
    					{
    						continue;
    					}
    					else
    					{
    						if( BaseTopology::q[pod_node].nodes[node_].utilization_out+utilizationOnEach<cuttoffnode_high)//unmark
    						{
    							if(target_node==999) //for the first time we will do this

    							{
    								target_node=node_;
    								target_pod=pod_node;
    								target_u=BaseTopology::q[pod_node].nodes[node_].utilization_out; //unmark
    							}

    							else if(target_node!=999 && target_u<BaseTopology::q[pod_node].nodes[node_].utilization_out) //unmark
    							{
    								target_node=node_;
    								target_pod=pod_node;
    								target_u=BaseTopology::q[pod_node].nodes[node_].utilization_out; //unmark
    							}
    						}
    						else
    						{
    							NS_LOG_UNCOND("any one of the node from the whole lot can not sustain the load"<<BaseTopology::q[pod_node].nodes[node_].utilization<<"-----"<<utilizationOnEach);
    							target_pod=999;
    							target_node=999;
    							break;
    						}
    					}
    				 }
    			}
    			else if (energy)
    			{
    				flag=false;
    				BaseTopology::q[min_pod].nodes[min_node_no].data[min_chunk_index].processed=1;
    				NS_LOG_UNCOND("AAAAAAAAAAAAAAAAAAAAAAAAAAAAFound chunk utilization---------------------------------------------------- :"<<min_chunk_no<<"::::: "<<min_chunk_u);
    				//based on the mean  utilization of the minimum chunk calculate the utilization of the rest

    				//find the location of the minimum chunk
    				//utilizationOnEach=(BaseTopology::chnkCopy[min_chunk_no].count*min_chunk_u)/(BaseTopology::chnkCopy[min_chunk_no].count-1);

    				 if(min_chunk_u>1)
    					 utilizationOnEach=((BaseTopology::chnkCopy[min_chunk_no].count )*min_chunk_u)/(BaseTopology::chnkCopy[min_chunk_no].count-1);
    				 else
    				 {
    					 utilizationOnEach=0;
    					 min_chunk_u=0;
    				 }
    				last_created_location=BaseTopology::chnkCopy[min_chunk_no].exists[BaseTopology::chnkCopy[min_chunk_no].count-1];
    				NS_LOG_UNCOND("last location  "<<last_created_location);
    				last_created_at_node=last_created_location%Ipv4GlobalRouting::FatTree_k;
    				last_created_at_pod=last_created_location/Ipv4GlobalRouting::FatTree_k;
    				if (min_node_no==last_created_at_node && last_created_at_pod==min_pod)
    				{
    				  //set some flag since next part we will be doing anyways
    					flag=true;
    					NS_LOG_UNCOND("flag is set");
    				}
    				for (uint32_t n = 0; n<BaseTopology::chnkCopy[min_chunk_no].count; n++)
    				{
    					uint32_t node_real=BaseTopology::chnkCopy[min_chunk_no].exists[n];
    					uint32_t pod_node=node_real/Ipv4GlobalRouting::FatTree_k;
    					uint32_t node_=node_real%Ipv4GlobalRouting::FatTree_k;


    					if(node_==min_node_no
    							&& pod_node==min_pod && flag)
    					{
    						continue;
    					}
    					else
    					{
    						if(utilizationOnEach==0)
    						{
    							NS_LOG_UNCOND("target node "<<target_node);
    							if(target_node==999 && node_real!=last_created_location) //for the first time we will do this

    							{

    									NS_LOG_UNCOND("default first time11");
    									target_node=node_;
    									target_pod=pod_node;
    									target_u=BaseTopology::q[pod_node].nodes[node_].utilization; //unmark

    							}
    							else if(target_node!=999 && target_u>BaseTopology::q[pod_node].nodes[node_].utilization_out && node_real!=last_created_location) //unmark
    							{

    									NS_LOG_UNCOND("next time onward11");
    									target_node=node_;
    									target_pod=pod_node;
    									target_u=BaseTopology::q[pod_node].nodes[node_].utilization; //unmark

    							}
    						}
    						else if(utilizationOnEach>0 && (BaseTopology::q[pod_node].nodes[node_].utilization_out-min_chunk_u)+utilizationOnEach<cuttoffnode_real) //unmark
    						{
    							if(target_node==999 && node_!=last_created_at_node && pod_node!=last_created_at_pod) //for the first time we will do this
    							{

    									NS_LOG_UNCOND("default first time");
    									target_node=node_;
    									target_pod=pod_node;
    									target_u=BaseTopology::q[pod_node].nodes[node_].utilization_out; //unmark

    							}
    						else if(target_node!=999 && target_u>BaseTopology::q[pod_node].nodes[node_].utilization_out && node_!=last_created_at_node && pod_node!=last_created_at_pod && (BaseTopology::q[pod_node].nodes[node_].utilization_out-min_chunk_u)+utilizationOnEach<cuttoffnode_real) //mark
    							{

    									NS_LOG_UNCOND("next time onward");
    									target_node=node_;
    									target_pod=pod_node;
    									target_u=BaseTopology::q[pod_node].nodes[node_].utilization_out; //unmark

    							}
    						}
    						else
    						{
    							NS_LOG_UNCOND("any one of the node from the whole lot can not sustain the load"<<BaseTopology::q[pod_node].nodes[node_].utilization<<"-----"<<utilizationOnEach);
    							target_pod=999;
    							target_node=999;
    							break;
    						}
    					}
    				}

    			}

    		 }
    		 else
    		 {
    			 NS_LOG_UNCOND("for this node we are not able to find a chunk that can satisfy all the conditions so going to some other node");
    			 break;
    		 }


    		 if(target_pod!=999 && target_node!=999 && energy==false)
    		{
    			if(!energy){
    			NS_LOG_UNCOND("target_pod to delete from  "<<target_pod<<" target_node delete from"<<target_node);
    			NS_LOG_UNCOND("source pod to move to "<<min_pod<<" source node to move to"<<min_node_no);
    			BaseTopology::res[res_index].src=(target_pod*Ipv4GlobalRouting::FatTree_k)+target_node;
    			BaseTopology::res[res_index].dest=(min_pod*Ipv4GlobalRouting::FatTree_k)+min_node_no;
    			BaseTopology::res[res_index].chunk_number=min_chunk_no;
    			res_index++;
    			processed_unit++;
    			//this is the time stamp per chunk level- i.e. when under the whole system any copy of the chunk deleted
    			BaseTopology::chnkCopy[min_chunk_no].last_deleted_timestamp_for_chunk=time_now;

    			 for (uint32_t n = 0; n<BaseTopology::chnkCopy[min_chunk_no].count; n++)
    			 {

    	           uint32_t node_=BaseTopology::chnkCopy[min_chunk_no].exists[n];
    	           uint32_t pod_node=node_/Ipv4GlobalRouting::FatTree_k;
    			   node_=node_%Ipv4GlobalRouting::FatTree_k;

    			   if(node_== target_node && pod_node==target_pod)
    				{
    				  loc=n;
    				  break;
    				}

    			 }
    			NS_LOG_UNCOND("the next location in the array where we want to the new copy nodes is"<<loc);
    			//we have to shift the whole data structure
    			if(loc==BaseTopology::chnkCopy[min_chunk_no].count-1)
    			{
    				BaseTopology::chnkCopy[min_chunk_no].count=BaseTopology::chnkCopy[min_chunk_no].count-1;
    			}
    			else
    			{
    				for (uint32_t n = loc; n<BaseTopology::chnkCopy[min_chunk_no].count-1; n++)
    				{
    					BaseTopology::chnkCopy[min_chunk_no].exists[n]=BaseTopology::chnkCopy[min_chunk_no].exists[n+1];
    				}
    				BaseTopology::chnkCopy[min_chunk_no].count=BaseTopology::chnkCopy[min_chunk_no].count-1;
    			}


    			 for (uint32_t n = 0; n<BaseTopology::chnkCopy[min_chunk_no].count; n++)
    			 {  uint32_t node_=BaseTopology::chnkCopy[min_chunk_no].exists[n];
    				uint32_t pod_node=node_/Ipv4GlobalRouting::FatTree_k;
    				node_=node_%Ipv4GlobalRouting::FatTree_k;
    				BaseTopology::q[pod_node].nodes[node_].utilization_out=BaseTopology::q[pod_node].nodes[node_].utilization_out+utilizationOnEach; //crtitical //unmark
    			 }
    			}
    		  }
    		else if(target_pod!=999 && target_node!=999 && energy==true)
    		{
    			NS_LOG_UNCOND("energy target_pod to delete from  "<<last_created_at_node<<" target_node delete from"<<last_created_at_pod);
    			NS_LOG_UNCOND("source pod to move to "<<target_pod<<" source node to move to"<<target_pod);
    			BaseTopology::res[res_index].dest=(target_pod*Ipv4GlobalRouting::FatTree_k)+target_node;
    			BaseTopology::res[res_index].src=(last_created_at_pod*Ipv4GlobalRouting::FatTree_k)+last_created_at_node;

    			BaseTopology::res[res_index].chunk_number=min_chunk_no;
    			res_index++;
    			processed_unit++;
    			//this is the time stamp per chunk level- i.e. when under the whole system any copy of the chunk deleted
    			BaseTopology::chnkCopy[min_chunk_no].last_deleted_timestamp_for_chunk=time_now;

    			 NS_LOG_UNCOND("chunk number :"<<min_chunk_no);
    			 for (uint32_t n = 0; n<BaseTopology::chnkCopy[min_chunk_no].count; n++)
    			 {
    				 NS_LOG_UNCOND("node :"<<BaseTopology::chnkCopy[min_chunk_no].exists[n]<<" location in the array "<<n);
    		      }
  				 BaseTopology::chnkCopy[min_chunk_no].count=BaseTopology::chnkCopy[min_chunk_no].count-1;


    			 for (uint32_t n = 0; n<BaseTopology::chnkCopy[min_chunk_no].count; n++)
    			 {  uint32_t node_=BaseTopology::chnkCopy[min_chunk_no].exists[n];
    				uint32_t pod_node=node_/Ipv4GlobalRouting::FatTree_k;
    				node_=node_%Ipv4GlobalRouting::FatTree_k;
    				BaseTopology::q[pod_node].nodes[node_].utilization_out=BaseTopology::q[pod_node].nodes[node_].utilization_out+utilizationOnEach-min_chunk_u; //crtitical //unmark
    			 }
    		//	 BaseTopology::chnkCopy[min_chunk_no].highCopyCount--;

    		}

    			target_pod=999;
    			target_node=999;
    			min_chunk_u=99999;
    			min_chunk_index=1000000;
    			min_chunk_no=1000000;

    		 }//end of while

            }//end of if


          }//end of for

        BaseTopology::res[res_index].src=99999;
        BaseTopology::res[res_index].dest=99999;
        BaseTopology::res[res_index].chunk_number=99999;

      }//end of else
//        else
//        {
//            BaseTopology::res[res_index].src=99999;
//            BaseTopology::res[res_index].dest=99999;
//            BaseTopology::res[res_index].chunk_number=99999;
//        }


 return;// BaseTopology::res;
}
//Madhurima added on July 20th
//-----------------------------------------------------------------------------------




} // namespace
