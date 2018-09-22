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

uint32_t** BaseTopology::chunk_assignment_to_applications = new uint32_t*[simulationRunProperties::total_applications + 1]; //application and which chunks



uint32_t** BaseTopology::application_assignment_to_node = new uint32_t*[total_hosts_in_system];

double** BaseTopology::chunk_assignment_probability_to_applications = new double*[simulationRunProperties::total_applications + 1];

uint32_t *BaseTopology::virtual_to_absolute_mapper = new uint32_t[simulationRunProperties::total_chunk];

Ptr<RandomVariableStream> BaseTopology::application_selector = CreateObject<UniformRandomVariable>();

double BaseTopology::popularity_change_simulation_interval = simulationRunProperties::popularity_change_start_ms;

Ptr<RandomVariableStream> BaseTopology::Popularity_Change_Random_Variable = CreateObject<UniformRandomVariable>();

Ptr<RandomVariableStream> BaseTopology::chunk_copy_selection = CreateObject<UniformRandomVariable>();

ApplicationMetrics *BaseTopology::application_metrics = new ApplicationMetrics[simulationRunProperties::total_applications];

uint32_t BaseTopology::phrase_change_counter = 0;

uint32_t *BaseTopology::phrase_change_interval;

double *BaseTopology::phrase_change_intensity_value;

double BaseTopology::intensity_change_simulation_interval = simulationRunProperties::intensity_change_start_ms;

uint32_t BaseTopology::total_phrase_changed = 0;

double BaseTopology::intensity_change_scale = 1.0;

Pod* BaseTopology::p;

int BaseTopology::counter_=0;

int BaseTopology::Incrcounter_=0;

bool BaseTopology::createflag=false;

chunkCopy* BaseTopology::chnkCopy;

nodedata* BaseTopology::nodeU;

nodedata* BaseTopology::nodeOldUtilization;

Result * BaseTopology::res;

double BaseTopology::sum_delay_ms = 0.0;

uint64_t BaseTopology::total_packet_count = 0;

uint64_t BaseTopology::total_packet_count_inc = 0;

uint32_t *BaseTopology::host_assignment_round_robin_counter = new uint32_t[(simulationRunProperties::k*simulationRunProperties::k*simulationRunProperties::k)/4];

double *BaseTopology::host_running_avg_bandwidth = new double[total_hosts_in_system + 1];

uint32_t *BaseTopology::host_running_avg_counter = new uint32_t[total_hosts_in_system + 1];

double *BaseTopology::application_probability = new double[simulationRunProperties::total_applications];

uint32_t BaseTopology::current_number_of_flows = 0;

uint32_t *BaseTopology::total_packets_to_hosts_bits = new uint32_t[total_hosts_in_system];

uint32_t BaseTopology::total_events = 0;

uint32_t BaseTopology::total_events_learnt = 0;

uint32_t *BaseTopology::total_packets_to_chunk = new uint32_t[simulationRunProperties::total_chunk];

uint32_t *BaseTopology::total_packets_to_chunk_destination = new uint32_t[simulationRunProperties::total_chunk];

double BaseTopology::last_point_of_entry = 0.0;

double BaseTopology::total_sum_of_entry = 0.0;

double BaseTopology::sum_of_number_time_packets = 0.0;

uint32_t *BaseTopology::chunk_version_tracker = new uint32_t[simulationRunProperties::total_chunk + 1];

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
/*Result **/void BaseTopology::calculateNewLocation(int incrDcr)
{

	float cuttoffnode_low=(int(Count)*int(SSD_PER_RACK))*0.2;//80;
	float cuttoffnode_high=(int(Count)*int(SSD_PER_RACK))*0.4;//320;
	float cuttoffnode_emer=(int(Count)*int(SSD_PER_RACK))*0.8;// 640;
	uint32_t number_of_hosts = (uint32_t)(Ipv4GlobalRouting::FatTree_k * Ipv4GlobalRouting::FatTree_k * Ipv4GlobalRouting::FatTree_k)/ 4;
	uint32_t nodes_in_pod = number_of_hosts / Ipv4GlobalRouting::FatTree_k;
	float cuttoffpod=nodes_in_pod*cuttoffnode_high; //this has to be made dynamic
	float max_node_u=0.0;
	uint32_t max_node_no=99999;
	uint32_t max_pod=999999;
	uint32_t min_node_no=0;
	uint32_t min_pod=0;
	uint32_t r=0;
	uint32_t res_index=0;
	float delta=.1;


	NS_LOG_UNCOND("enetering");

	if(BaseTopology::createflag!=true)
	{
	for (int i=0;i<Ipv4GlobalRouting::FatTree_k;i++)
	{
		for(uint32_t j=0;j<nodes_in_pod;j++)
		{
			for(uint32_t q=0;q<BaseTopology::p[i].nodes[j].total_chunks;q++)
			{
				uint32_t  chunk_num=BaseTopology::p[i].nodes[j].data[q].chunk_number;
				BaseTopology::chnkCopy[chunk_num].exists[BaseTopology::chnkCopy[chunk_num].count]=j+i*Ipv4GlobalRouting::FatTree_k;
				BaseTopology::chnkCopy[chunk_num].count=1;


			}
		}
	}


		for (int i=0;i<Ipv4GlobalRouting::FatTree_k;i++)
			{
				for(uint32_t j=0;j<nodes_in_pod;j++)
				{
					nodeU[r].U=BaseTopology::p[i].nodes[j].utilization;
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
    float alpha =0.75;
	for (int i=0;i<Ipv4GlobalRouting::FatTree_k;i++)
		{
		NS_LOG_UNCOND("--------------------------------"<<BaseTopology::p[i].Pod_utilization);
			for(uint32_t j=0;j<nodes_in_pod;j++)
			{
				int nod=j+i*Ipv4GlobalRouting::FatTree_k;
				for ( k=0;k<number_of_hosts;k++)
				{
					if (nod==nodeU[k].nodenumber)
						break;
				}
				nodeU[k].U=(alpha*BaseTopology::p[i].nodes[j].utilization)+(nodeU[k].U*(1-alpha));
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
   	// uint32_t action=0;
   	 uint32_t max_chunk_index;
   	 float beyond_cutoff;
   	 float target_utilization=0.0;
   	 float estimated_utlization; //this is the utilization we exopect on each of the chunk after copy creation
	//float diff_from_pod_cuttoff=0.0;
	float diff_from_node_cutoff=0.0;
	//float max_diff_from_pod_cuttoff=0.0;
	float max_diff_from_node_cutoff=0.0;
  // 	 uint32_t numberOfNodeProcessed=0; //number of node already visited from the list
   	 for(int i =number_of_hosts-1 ; i>=0; i--)
 	 {
       	target_pod=999;
       	target_node=999;
     	max_node_u=0.0;
     	max_node_no=9999;
     	max_chunk_no=9999;
        max_chunk_u=0;
        max_chunk_index=99999;
        uint32_t nodeN;

        printf("PPPPPPPPPP\n");
        if(nodeU[i].U<cuttoffnode_high)
        	break;
        else
		{
			nodeN=nodeU[i].nodenumber;
			NS_LOG_UNCOND("nodeU[i].nodenumber  "<<nodeU[i].nodenumber<<" nodeOldUtilization[nodeN]  "<<nodeOldUtilization[nodeN].nodenumber<<" Current Utilization "<<nodeU[i].U<<" old Utilization "<<nodeOldUtilization[nodeN].U<<"  chnage since the last time"<<(nodeU[i].U-nodeOldUtilization[nodeN].U)/nodeOldUtilization[nodeN].U);
			if(nodeOldUtilization[nodeN].U!=0 && (nodeU[i].U-nodeOldUtilization[nodeN].U)/nodeOldUtilization[nodeN].U<delta)
			{
				//nodeOldUtilization[nodeN].U=nodeU[i].U; // this node is being selected but not qualified so updating the utilizaton with the current and smoothed value
				nodeOldUtilization[nodeN].counter++; //how many times one is selected for serive and either server or rejected
				if(nodeOldUtilization[nodeN].counter%20==0)
					nodeOldUtilization[nodeN].U=0.0;
				continue;
			}
			else
				nodeOldUtilization[nodeN].counter++;
		}

     //   else
      	//    numberOfNodeProcessed++;
        uint32_t pod = (uint32_t) floor((double) nodeN/ (double) Ipv4GlobalRouting::FatTree_k);

        max_node_no=nodeN%Ipv4GlobalRouting::FatTree_k;
		max_node_u=BaseTopology::p[pod].nodes[max_node_no].utilization;//nodeU[i].U;
		max_pod=pod;
		//if the highest utilized node is below high cut off then retur


		NS_LOG_UNCOND("The maximally used node is------------------&&&&&&&&&&------------------------------------------------------------------------- :"<<max_node_u<<" ::: max_node_no "<<max_node_no<<" ::: max_pod "<<max_pod<<BaseTopology::p[max_pod].nodes[max_node_no].total_chunks);
		beyond_cutoff=max_node_u-(cuttoffnode_high+(cuttoffnode_high*.7));
		target_utilization=0.0;
		//uint32_t targettedChunksCount=1;//BaseTopology::p[max_pod].nodes[max_node_no].total_chunks*.01; //these are the desired number of chunks that we want to replicate
		//printf("targettedChunksCount %d\n",targettedChunksCount);
        //action=0;

//we are checking if a node is beyond the high range and below the emergency range
	    if(cuttoffnode_high<max_node_u /* && cuttoffnode_emer>max_node_u*/ && beyond_cutoff>0.0 )
	    {

	       while(target_utilization<beyond_cutoff)
	       {
	    	   max_chunk_no=9999;
	    	   max_chunk_u=0;
	    	   max_chunk_index=99999;

	    	   NS_LOG_UNCOND("here we are and the pod number , node number a :"<<max_pod<<"::::"<<max_node_no);
	    	   for(uint32_t i = 0; i<BaseTopology::p[max_pod].nodes[max_node_no].total_chunks; i++)
		      	 	 {

		      	 		 if( BaseTopology::p[max_pod].nodes[max_node_no].data[i].highCopyCount<Ipv4GlobalRouting::FatTree_k
		      	 		     && BaseTopology::chnkCopy[BaseTopology::p[max_pod].nodes[max_node_no].data[i].chunk_number].highCopyCount<Ipv4GlobalRouting::FatTree_k
		      	 		     && BaseTopology::p[max_pod].nodes[max_node_no].data[i].processed!=1
							 && max_chunk_u<BaseTopology::p[max_pod].nodes[max_node_no].data[i].intensity_sum
							 && BaseTopology::p[max_pod].nodes[max_node_no].data[i].intensity_sum>max_node_u*0.03//0.5*beyond_cutoff  //not sure about this condition
		   					 /*&& BaseTopology::chnkCopy[BaseTopology::p[max_pod].nodes[max_node_no].data[i].chunk_number].count<number_of_hosts*/) //also check whether the chunk is valid in the node -sinceit could be deleted
		      	 		 {
//		      	 		     if(BaseTopology::p[max_pod].nodes[max_node_no].data[i].chunk_number==247)
//		      	 		    	 NS_LOG_UNCOND("How come we are here for node 247"<<BaseTopology::chnkCopy[BaseTopology::p[max_pod].nodes[max_node_no].data[i].chunk_number].highCopyCount);
		      	 			 max_chunk_no=BaseTopology::p[max_pod].nodes[max_node_no].data[i].chunk_number;
		      	 			 max_chunk_u = BaseTopology::p[max_pod].nodes[max_node_no].data[i].intensity_sum;
		      	 			 max_chunk_index=i;

		      	 		 }
		      	 	 }




		   NS_LOG_UNCOND("Found chunk utilization--------------------------------------------------------------------------------------max_chunk_no :"<<max_chunk_no<<" ::: max_pod "<<max_pod<<" ::::max_chunk_u "<<max_chunk_u<<" :::::totalchunk on the node"<<BaseTopology::p[max_pod].nodes[max_node_no].total_chunks);		  //last_max_chunk=max_chunk_no;
		   //if
		  // uint32_t action = 0;
		   int flag=0;
		   if(max_chunk_no!=9999)// && action<targettedChunksCount)
		   {
			   estimated_utlization=((max_chunk_u*BaseTopology::chnkCopy[max_chunk_no].count)/(BaseTopology::chnkCopy[max_chunk_no].count+1));
			   BaseTopology::p[max_pod].nodes[max_node_no].data[max_chunk_index].processed=1;
			 //  diff_from_pod_cuttoff=0.0;
			   diff_from_node_cutoff=0.0;
			   //max_diff_from_pod_cuttoff=0.0;
			   max_diff_from_node_cutoff=0.0;
			  	 //Now split the load and assign to the nodes
			  // printf("here%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n");
			   //We are deciding which pod is best here
			  	 for (int i=Ipv4GlobalRouting::FatTree_k-1;i>=0;i--)
			  		{
			  		 //NS_LOG_UNCOND(BaseTopology::p[i].Pod_utilization);
			  		 if(BaseTopology::p[i].Pod_utilization+estimated_utlization<cuttoffpod) //placement can lead to exceed the pod level threshold
			  		 {

			  	 //selecting the destination node for the chunk
			  		 for(uint32_t j=0;j<nodes_in_pod;j++)
				     {
					 flag=0;
					 for(uint32_t q=0;q<BaseTopology::p[i].nodes[j].total_chunks;q++)
					 {
						 if(BaseTopology::p[i].nodes[j].data[q].chunk_number==max_chunk_no)
							 //already the chunk exists in the node so continue - Also the maximum node will not come as we are checking for validlity
							 {
								 flag=1;
								 break;
							 }
					 }

					 if(BaseTopology::p[i].nodes[j].utilization+estimated_utlization<cuttoffnode_high && !flag /*&& BaseTopology::p[i].nodes[j].utilization>0*/)  //placement can lead to exceed the node level threshold
					 {
						 diff_from_node_cutoff=cuttoffnode_high-(BaseTopology::p[i].nodes[j].utilization+max_chunk_u);
						 //register benefit
						 if(max_diff_from_node_cutoff<=diff_from_node_cutoff)
						 {
							 max_diff_from_node_cutoff=diff_from_node_cutoff;
							 target_node=j;
							 target_pod=i;
							 //update the values that you have with the U of the current chunk
						 }

					 }//end of if
				   }//end of node level for

			  }
		   }
			  }//end of if max chunk found
		   else
			   break; //no max chunk on the node satisfy all the requirements
			  	 //we have placed the item
			  	if(target_pod!=999 && target_node!=999)
			  	{

			  		//printf("gotcha ------------------------------------((((())))))))))))))))))))))))))))))))))---------%d-----------%d\n",target_node,target_pod);
			  		BaseTopology::res[res_index].src=(max_pod*Ipv4GlobalRouting::FatTree_k)+max_node_no;
			  		BaseTopology::res[res_index].dest=(target_pod*Ipv4GlobalRouting::FatTree_k)+target_node;
			  		BaseTopology::res[res_index].chunk_number=max_chunk_no;
					res_index++;
					//action++;
					target_utilization=target_utilization+estimated_utlization;
					BaseTopology::p[target_pod].nodes[target_node].utilization=BaseTopology::p[target_pod].nodes[target_node].utilization+estimated_utlization;
					BaseTopology::p[target_pod].Pod_utilization=BaseTopology::p[target_pod].Pod_utilization+estimated_utlization;
					uint32_t location=BaseTopology::chnkCopy[max_chunk_no].count;
					BaseTopology::chnkCopy[max_chunk_no].count=BaseTopology::chnkCopy[max_chunk_no].count+1;
					BaseTopology::chnkCopy[max_chunk_no].exists[location]=((target_pod*Ipv4GlobalRouting::FatTree_k)+target_node);
					BaseTopology::p[max_pod].nodes[max_node_no].data[max_chunk_index].highCopyCount=BaseTopology::p[max_pod].nodes[max_node_no].data[max_chunk_index].highCopyCount+1;
//					if(max_chunk_no==247)
//					{
//						NS_LOG_UNCOND("Treating chunk 247 before"<<BaseTopology::chnkCopy[max_chunk_no].highCopyCount);
//					}
					BaseTopology::chnkCopy[max_chunk_no].highCopyCount=BaseTopology::chnkCopy[max_chunk_no].highCopyCount+1;

					//nodeN is the node for which we did increase
					nodeOldUtilization[nodeN].U=max_node_u;

//					if(max_chunk_no==247)
//					{
//						NS_LOG_UNCOND("Treating chunk 247 after"<<BaseTopology::chnkCopy[max_chunk_no].highCopyCount);
//					}
			  	    target_pod=999;
			  	    target_node=999;
			  	    //diff_from_pod_cuttoff=0;
			  	    diff_from_node_cutoff=0;
			  	   // max_diff_from_pod_cuttoff=0;
			  	    max_diff_from_node_cutoff=0;
			  	    max_chunk_no=9999;
			  	    max_chunk_u=0;
			  	    max_chunk_index=99999;

			  	}

	       }//end of while
	       ;
	      }//end of if for difference from cutoff
	else if(cuttoffnode_emer<max_node_u)   //to be implemented
		NS_LOG_UNCOND("!!!!!!!!!!!!!!!!!!!!!!!!!the node is under emergency range so we might have to create more than one copy of the most used chunk on them in order to relief"<<max_node_no);
        //for now we are not creating copies just looking for an odd situation
	else
		break; //no break before we exhaust x% of the busy item in the node
 	 }//end of for

   	//printf("res_index%d\n",res_index);
   	BaseTopology::res[res_index].src=99999;
   	BaseTopology::res[res_index].dest=99999;
   	BaseTopology:: res[res_index].chunk_number=99999;


    }//end of if incrdcr

//Decrease starts here-----------------------------------------------------------------------------------------------------------------
    else
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
    // for (uint32_t i = 0; i < number_of_hosts ; i++)
    // {
    	 //NS_LOG_UNCOND("here-------------------"<<nodeU[i].U<<"-----------------"<<nodeU[i].nodenumber);
   // }

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
		if(nodeU[i].U > cuttoffnode_low)
			break;
		uint32_t pod = (uint32_t) floor((double) nodeN/ (double) Ipv4GlobalRouting::FatTree_k);



	 if(BaseTopology::p[pod].nodes[nodeN%Ipv4GlobalRouting::FatTree_k].utilization>0
			 && BaseTopology::p[pod].nodes[nodeN%Ipv4GlobalRouting::FatTree_k].utilization<cuttoffnode_low
			 && BaseTopology::p[pod].nodes[nodeN%Ipv4GlobalRouting::FatTree_k].total_chunks>0)
	 {
			 //min_node_u=BaseTopology::p[pod].nodes[nodeN%Ipv4GlobalRouting::FatTree_k].utilization;
			 min_node_no=nodeN%Ipv4GlobalRouting::FatTree_k;
			 min_pod=pod;

        NS_LOG_UNCOND("The minimally used node is-----------------------------------------------------------------------------------------: "<<min_node_no<<" ::: "<<min_pod);
        processed_unit=0;

    	while(number_of_chunk_to_process>processed_unit)
    	{
    		min_chunk_u=99999;
			min_chunk_index=1000000;
			min_chunk_no=1000000;

		 for(uint32_t j = 0; j<BaseTopology::p[min_pod].nodes[min_node_no].total_chunks; j++)
		 {

			 uint32_t l=BaseTopology::p[min_pod].nodes[min_node_no].data[j].chunk_number;
			 if( BaseTopology::chnkCopy[l].count>1
					 && BaseTopology::p[min_pod].nodes[min_node_no].data[j].processed!=1
					 && BaseTopology::p[min_pod].nodes[min_node_no].data[j].intensity_sum>0) //also check whether the chunk is valid in the node -sinceit could be deleted
			 {
				 if(min_chunk_no==1000000) //this is only for the first time per node
				 {
			        min_chunk_no=BaseTopology::p[min_pod].nodes[min_node_no].data[j].chunk_number;
			        min_chunk_u = BaseTopology::p[min_pod].nodes[min_node_no].data[j].intensity_sum;
			        min_chunk_index=j;
				 }
				 else if(min_chunk_no!=1000000 && min_chunk_u<BaseTopology::p[min_pod].nodes[min_node_no].data[j].intensity_sum)
				 {
					min_chunk_no=BaseTopology::p[min_pod].nodes[min_node_no].data[j].chunk_number;
					min_chunk_u = BaseTopology::p[min_pod].nodes[min_node_no].data[j].intensity_sum;
					min_chunk_index=j;
				 }

			 }
		 }
		// NS_LOG_UNCOND("We camw out clean-----");
		 if(min_chunk_no!=1000000)
		 {

			 BaseTopology::p[min_pod].nodes[min_node_no].data[min_chunk_index].processed=1;
			 NS_LOG_UNCOND("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@Found chunk utilization---------------------------------------------------- :"<<min_chunk_no<<"::::: "<<min_chunk_u);
			 //based on the mean  utilization of the minimum chunk calculate the utilization of the rest

			 //find the location of the minimum chunk
			 utilizationOnEach=((BaseTopology::chnkCopy[min_chunk_no].count + 1)*min_chunk_u)/BaseTopology::chnkCopy[min_chunk_no].count;

			 for (uint32_t n = 0; n<BaseTopology::chnkCopy[min_chunk_no].count; n++)
			 {  uint32_t node_=BaseTopology::chnkCopy[min_chunk_no].exists[n];
				uint32_t pod_node=node_/Ipv4GlobalRouting::FatTree_k;
				node_=node_%Ipv4GlobalRouting::FatTree_k;

				if(node_==min_node_no
						&& pod_node==min_pod)
					//	&& BaseTopology::p[pod_node].Pod_utilization+min_chunk_u<=cuttoffpod
					//	&& BaseTopology::p[pod_node].nodes[node_].utilization+min_chunk_u<=cuttoffnode_high)
				{
					continue;
				}
				else
				{
					if( BaseTopology::p[pod_node].nodes[node_].utilization+utilizationOnEach<cuttoffnode_high)
					{
						if(target_node==999) //for the first time we will do this
						//current_min_u=BaseTopology::p[pod_node].nodes[node_].data[min_chunk_no].intensity_sum;
						{
							target_node=node_;
							target_pod=pod_node;
							target_u=BaseTopology::p[pod_node].nodes[node_].utilization;
						}
						else if(target_node!=999
								&& target_u<BaseTopology::p[pod_node].nodes[node_].utilization)
						{
							target_node=node_;
							target_pod=pod_node;
							target_u=BaseTopology::p[pod_node].nodes[node_].utilization;
						}
					}
					else
					{
						NS_LOG_UNCOND("any one of the node from the whole lot can not sustain the load"<<BaseTopology::p[pod_node].nodes[node_].utilization<<"-----"<<utilizationOnEach);
						target_pod=999;
						target_node=999;
						break;
					}
				}
			 }


		 }
		 else
		 {
			 NS_LOG_UNCOND("for this node we are not able to find a chunk that can statify all the conditions so going to some other node");
			 break;
		 }


		 if(target_pod!=999 && target_node!=999)
		{

			NS_LOG_UNCOND("target_pod to delete from  "<<target_pod<<" target_node delete from"<<target_node);
			NS_LOG_UNCOND("source pod to move to "<<min_pod<<" source node to move to"<<min_node_no);
			BaseTopology::res[res_index].src=(target_pod*Ipv4GlobalRouting::FatTree_k)+target_node;
			BaseTopology::res[res_index].dest=(min_pod*Ipv4GlobalRouting::FatTree_k)+min_node_no;
			BaseTopology::res[res_index].chunk_number=min_chunk_no;
			res_index++;
			processed_unit++;
			//BaseTopology::p[min_pod].nodes[min_node_no].utilization=BaseTopology::p[target_pod].nodes[target_node].utilization+utilizationOnEach;
			//BaseTopology::p[target_pod].Pod_utilization=BaseTopology::p[target_pod].Pod_utilization+min_chunk_u;

			 for (uint32_t n = 0; n<BaseTopology::chnkCopy[min_chunk_no].count; n++)
			 {

	           uint32_t node_=BaseTopology::chnkCopy[min_chunk_no].exists[n];
	           uint32_t pod_node=node_/Ipv4GlobalRouting::FatTree_k;
			   node_=node_%Ipv4GlobalRouting::FatTree_k;
			  // if(min_chunk_no==8)
			  // NS_LOG_UNCOND("node"<<node_<<"############### pod"<<pod_node);
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
				BaseTopology::p[pod_node].nodes[node_].utilization=BaseTopology::p[pod_node].nodes[node_].utilization+utilizationOnEach;
			 }


			target_pod=999;
			target_node=999;
			min_chunk_u=99999;
			min_chunk_index=1000000;
			min_chunk_no=1000000;


		}
		 else
		 {
		 NS_LOG_UNCOND("***********************************target_node*********************************************************************"<<target_node);
		 }

		 }//end of while

        }//end of if


      }//end of for

    BaseTopology::res[res_index].src=99999;
    BaseTopology::res[res_index].dest=99999;
    BaseTopology::res[res_index].chunk_number=99999;

  }//end of else


 return;// BaseTopology::res;
}
//Madhurima added on July 20th
//-----------------------------------------------------------------------------------




} // namespace
