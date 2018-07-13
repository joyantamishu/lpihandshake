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

uint32_t* BaseTopology::hostaddresslogicaltophysical = new uint32_t[((simulationRunProperties::k * simulationRunProperties::k * simulationRunProperties::k)/4) +1 ];

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

bool BaseTopology::createflag=false;

HostBandwidthRequirement** BaseTopology::application_statistics = new HostBandwidthRequirement*[simulationRunProperties::total_applications+1];

uint64_t BaseTopology::min_offset = 0;



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

	application_count = 0;
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

void BaseTopology::calculateNewLocation(int incrDcr,uint32_t *src, uint32_t *dest, uint32_t *chunk)
{
	 float cuttoffnode_high=500.0;   //this has to be made dynamic
	// float cuttoffnode_low=100.0;   //this has to be made dynamic
	 uint32_t number_of_hosts = (uint32_t)(Ipv4GlobalRouting::FatTree_k * Ipv4GlobalRouting::FatTree_k * Ipv4GlobalRouting::FatTree_k)/ 4;
	 uint32_t nodes_in_pod = number_of_hosts / Ipv4GlobalRouting::FatTree_k;
	 float cuttoffpod=nodes_in_pod*cuttoffnode_high; //this has to be made dynamic
	 float max_node_u=0.0;
	 uint32_t max_node_no=0;
	 uint32_t max_pod=0;
	 float min_node_u=10000.0;
	 uint32_t min_node_no=0;
	 uint32_t min_pod=0;
	 //find the node with max utilization and minimum utilized node
	 for(uint32_t i = 0; i<number_of_hosts; i++)
	 {
		 uint32_t pod = (uint32_t) floor((double) i/ (double) Ipv4GlobalRouting::FatTree_k);
		 if(incrDcr && BaseTopology::p[pod].nodes[i%Ipv4GlobalRouting::FatTree_k].utilization>max_node_u)
		 {
			 max_node_u=BaseTopology::p[pod].nodes[i%Ipv4GlobalRouting::FatTree_k].utilization;
			 max_node_no=i%Ipv4GlobalRouting::FatTree_k;
			 max_pod=pod;
		 }
		 if(!incrDcr &&  BaseTopology::p[pod].nodes[i%Ipv4GlobalRouting::FatTree_k].utilization>0 && BaseTopology::p[pod].nodes[i%Ipv4GlobalRouting::FatTree_k].utilization<min_node_u && BaseTopology::p[pod].nodes[i%Ipv4GlobalRouting::FatTree_k].total_chunks)
		 {
				 min_node_u=BaseTopology::p[pod].nodes[i%Ipv4GlobalRouting::FatTree_k].utilization;
				 min_node_no=i%Ipv4GlobalRouting::FatTree_k;
				 min_pod=pod;
		  }
	 }
//	NS_LOG_UNCOND("The maximally used node is------------------------------------------------------------------------------------------- :"<<max_node_u<<" ::: "<<max_node_no<<" ::: "<<max_pod);
	//NS_LOG_UNCOND("The minimally used node is------------------------------------------------------------------------------------------- :"<<min_node_u<<" ::: "<<min_node_no<<" ::: "<<min_pod);

	 //if((incrDcr && std::abs(cuttoffnode_high-max_node_u)<20) || (!incrDcr && std::abs(cuttoffnode_low-min_node_u)<20))
	// {
/*/Do we need to do this everytime? or when the absolute difference shirnks only then
	 if(std::abs(cuttoffnode_high-max_node_u)<std::abs(cuttoffnode_low-min_node_u))
	 { incrDcr=1; NS_LOG_UNCOND("increase selected");}
	 else
	 { incrDcr=0; NS_LOG_UNCOND("decrease selected");}
*/
	 //find out the chunk maximum used within the highly used node
	 //Here is the logic for increase
	 //This part actually assign the data to the node
    if(incrDcr){
     uint32_t max_chunk_no=0;
   	 float max_chunk_u=0.0;

   	 for(uint32_t i = 0; i<BaseTopology::p[max_pod].nodes[max_node_no].total_chunks; i++)
   	 	 {
   	 		 if(max_chunk_u<BaseTopology::p[max_pod].nodes[max_node_no].data[i].intensity_sum) //also check whether the chunk is valid in the node -sinceit could be deleted
   	 		 {
   	 			 max_chunk_no=BaseTopology::p[max_pod].nodes[max_node_no].data[i].chunk_number;
   	 			 max_chunk_u = BaseTopology::p[max_pod].nodes[max_node_no].data[i].intensity_sum;
   	 		 }
   	 	 }
  // 	NS_LOG_UNCOND("Found chunk utilization--------------------------------------------------------------------------------------$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ :"<<max_chunk_no<<" "<<max_chunk_u);


   	 uint32_t target_pod=0;
   	 uint32_t target_node=0;
	 float diff_from_pod_cuttoff=0.0;
	 float diff_from_node_cutoff=0.0;
	 float max_diff_from_pod_cuttoff=0.0;
     float max_diff_from_node_cutoff=0.0;
	 //Now split the load and assign to the nodes
     bool exists=false;
	 for (int i=0;i<Ipv4GlobalRouting::FatTree_k;i++)
		{
		 if(BaseTopology::p[i].Pod_utilization+max_chunk_u>=cuttoffpod) //placement can lead to exceed the pod level threshold
			 continue;
		 else
		 {
			 diff_from_pod_cuttoff=cuttoffpod-(BaseTopology::p[i].Pod_utilization+max_chunk_u);
			 for(uint32_t j=0;j<nodes_in_pod;j++)
			 {
				 for(uint32_t q=0;q<BaseTopology::p[i].nodes[j].total_chunks;q++)
				 {
					 if(BaseTopology::p[i].nodes[j].data[q].chunk_number==max_chunk_no)
				  //already the chunk exists in the node so continue - Also the maximum node will not come as we are checking for validlity
					 { exists=true; break;}
				 }
				 if(exists)
				 {
					 exists=false;
					 continue;
				 }
				 else if(BaseTopology::p[i].nodes[j].utilization+max_chunk_u>=cuttoffnode_high)  //placement can lead to exceed the node level threshold
						 continue;
				 else
				 {
					 diff_from_node_cutoff=cuttoffnode_high-(BaseTopology::p[i].nodes[j].utilization+max_chunk_u);
					 //register benefit -need to cross check
					 if((max_diff_from_pod_cuttoff<=diff_from_pod_cuttoff) && (max_diff_from_node_cutoff<=diff_from_node_cutoff))
					 {   max_diff_from_pod_cuttoff=diff_from_pod_cuttoff;
						 max_diff_from_node_cutoff=diff_from_node_cutoff;
						 target_pod=i;
						 target_node=j;
					 }

				 }
			 }
		 }
		}

	 *src=(target_pod*Ipv4GlobalRouting::FatTree_k)+target_node;
	 *chunk=max_chunk_no;
	 *dest=(target_pod*Ipv4GlobalRouting::FatTree_k)+target_node;
  //  Ipv4GlobalRouting::p[target_pod].nodes[target_node].data[max_chunk_no].valid=1; //make it valid where the new copy will be created
    }
    else
    {

	//make sure that atleast one copy of the chunk exists in the sytem - needs to be done
		 uint32_t min_chunk_no=0;
		 float min_chunk_u=9999.0;
		 for(uint32_t i = 0; i<BaseTopology::p[min_pod].nodes[min_node_no].total_chunks; i++)
		 {
           //  NS_LOG_UNCOND("value:  "<<BaseTopology::p[min_pod].nodes[min_node_no].data[i].intensity_sum<<"------------"<<BaseTopology::p[min_pod].nodes[min_node_no].data[i].chunk_number);
			 if(min_chunk_u>BaseTopology::p[min_pod].nodes[min_node_no].data[i].intensity_sum &&BaseTopology::p[min_pod].nodes[min_node_no].data[i].intensity_sum>0) //also check whether the chunk is valid in the node -sinceit could be deleted
			 {
				 min_chunk_no=BaseTopology::p[min_pod].nodes[min_node_no].data[i].chunk_number;
				 min_chunk_u = BaseTopology::p[min_pod].nodes[min_node_no].data[i].intensity_sum;
			 }
		 }
	 //NS_LOG_UNCOND("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@Found chunk utilization---------------------------------------------------- :"<<min_chunk_no<<" "<<min_chunk_u);
    	 uint32_t target_pod=999;
		 uint32_t target_node=999;
		 float diff_from_pod_cuttoff=0.0;
		 float diff_from_node_cutoff=0.0;
		 float min_diff_from_pod_cuttoff=100000.0;
		 float min_diff_from_node_cutoff=1000000.0;
		 //Now split the load and assign to the nodes
		 //bool exists=false;
		 for (int i=0;i<Ipv4GlobalRouting::FatTree_k;i++)
		 {
			 if(BaseTopology::p[i].Pod_utilization+min_chunk_u>=cuttoffpod) //placement can lead to exceed the pod level threshold
				 continue;
			 else
			 {
				 diff_from_pod_cuttoff=cuttoffpod-(BaseTopology::p[i].Pod_utilization+min_chunk_u);
				 for(uint32_t j=0;j<nodes_in_pod;j++)
				 {
					 for(uint32_t q=0;q<BaseTopology::p[i].nodes[j].total_chunks;q++)
					 {
						 if(BaseTopology::p[i].nodes[j].data[q].chunk_number==min_chunk_no)
					  //already the chunk exists in the node so continue - Also the maximum node will not come as we are checking for validlity
						 {

							if(BaseTopology::p[i].nodes[j].utilization+min_chunk_u<=cuttoffnode_high)  //placement can lead to exceed the node level threshold
								 {
									 diff_from_node_cutoff=cuttoffnode_high-(BaseTopology::p[i].nodes[j].utilization+min_chunk_u);
									 //register benefit -need to cross check
									 if((min_diff_from_pod_cuttoff>diff_from_pod_cuttoff) && (min_diff_from_node_cutoff>diff_from_node_cutoff))
									 {   min_diff_from_pod_cuttoff=diff_from_pod_cuttoff;
										 min_diff_from_node_cutoff=diff_from_node_cutoff;
										 target_pod=i;
										 target_node=j;
									 }

								 }
							break;
						 }
					 }
				 }
			 }
		 }
         if(target_pod!=999 && target_node!=999){
		 *src=(min_pod*Ipv4GlobalRouting::FatTree_k)+min_node_no;
		 *chunk=min_chunk_no;
		 *dest=(target_pod*Ipv4GlobalRouting::FatTree_k)+target_node;}
		// BaseTopology::p[min_pod].nodes[min_node_no].data[min_chunk_no].valid=0; //make it invalid from for at chunk where it will be removed from the node
    }
  //}

}

//-----------------------------------------------------------------------------------




} // namespace
