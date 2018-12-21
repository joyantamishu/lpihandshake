/*
 * ss-base-topology.h
 *
 *  Created on: Jul 24, 2016
 *      Author: sanjeev
 */

#ifndef SS_BASE_TOPOLOGY_H_
#define SS_BASE_TOPOLOGY_H_

#include <string.h>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/config-store-module.h"
#include "ns3/energy-module.h"
#include "ns3/flow-monitor-module.h"
#include "ss-base-topology-simulation-variables.h"
#include "ss-energy-source.h"
#include "ss-node.h"
#include "ss-udp-echo-client.h"

#define ARBITRARY_FORLOOP_NUMBER			50			// sanjeev May 9
#define ARBITRARY_FORLOOP_NUMBER_MR			-1			// sanjeev May 9,

// Energy source/model values
#define DefaultEnergyMeterUpdateIntervalSec		0.01			// sanjeev. mar 23. May 1st = moved here
#define CLIENTSTOPTHRESHOLD						.98

#define NUMBER_OF_ACTIVE_FLOWS						(m_flowCount - m_totalStoppedFlowCounter)

namespace ns3 {



/******Chunk Specific Change *******************/

class MultipleCopyOfChunkInfo{
public:
	uint32_t chunk_id;
	uint32_t location;
	uint32_t version;
	MultipleCopyOfChunkInfo(uint32_t chunk_id, uint32_t location, uint32_t version)
	{
		this->chunk_id = chunk_id;
		this->location = location;
		this->version = version;
	}
};



struct MarkovModel {
	bool m_IsT0_State; 	// T0 or T1 state....
	Ptr<RandomVariableStream> m_t0t1_stateInterval; // exponentially distributed mean = 50ms or 100ms
	Ptr<RandomVariableStream> m_eta_0, m_eta_1;	// probability= uniform random distribution, 0.4 & 1.6
	double tempETA0, tempETA1;// some temp variables to avoid initiating repeatedly
	double tempNextStateChangeAt;
	int metric_total_state_changes; // no of times states changed.
	double metric_time_inT0state;		// time spent in T0 state (milliSec)
	double metric_time_inT1state;		// time spent in T1 state (milliSec)
	int metric_flows_inT0state;	// no of flows injected in T0 state
	int metric_flows_inT1state;	// no of flows injected in T0 state
	double metric_totalFlowDuration_inT0time;// flow duration in T0 state (milliSec)
	double metric_totalFlowDuration_inT1time;// flow duration in T1 state (milliSec)
};

// Apr 23 sanjeev, clean re-arrange of old code
struct SawToothModel {
	bool m_IsPhase0_or_Phase1_State;// a little diff.naming..Apr 25 extended sawtooth-sanjeev
	int m_thisStoppedFlowCounter;
	int m_total_flows_to_stop;
	int m_total_flows_to_inject;
	Ptr<RandomVariableStream> m_OldFlowStopCounter;	// probability= uniform random distribution, 0.0 & 1.0
	Ptr<RandomVariableStream> m_newFlowCountToInject;
};

// Apr 23 sanjeev, clean re-arrange of old code
struct TickModel {
	Ptr<RandomVariableStream> m_TickCounter;	// exponential
};

// this is total energy & time across ALL devices (not individual device or node etc)
struct TopologyDataCollected {
	double m_totalConsumedEnergy_nJ;
	double m_totalConsumedEnergyByNodeFabricONLY_nJ;	// sanjeev Jun 13
	double m_totalSleepStateTimeEnergy_nJ;
	double m_totalIdleTimeEnergy_nJ;
	double m_totalTransmitTimeEnergy_nJ;
	double m_totalSleepTimeNanoSec;
	double m_totalIdleTimeNanoSec;
	double m_totalTransmitTimeNanoSec;
	double m_totalTimeNanoSec;
	double m_AvgThroughPut;
	double m_AvgOfferedThroughPut;
	double m_AvgConsumedThroughPut;
	int m_packetCount;
	int m_countSleepState;
	int m_deviceCount;
	int m_deviceSetQdepth;				// sanjeev, apr 17, from device settings
	int m_deviceMaxQdepth;
	int m_deviceMaxQdepth_atNode;				// sanjeev, apr27
	int m_DeviceTotalObservedQueueDepth;
	int m_FlowCount;
	int m_lostPacketCount;
	int m_FlowsWithLostPacket;
	Time m_AvgLatency;
	Time m_AvgDelay;
	Time m_AvgJitter;
};

struct FlowDataCollected {
	int flowid;				// flow id
	int packetid;			// last packet traced...
	int srcNodeid;			// where the flow started from, srcNodeid
	int dstNodeid;			// where the flow ended? dstNodeid
	Ipv4Address srcNodeIpv4;
	Ipv4Address dstNodeIpv4;
	// double microsec (also captures nanosec), sanjeev may 9
	double packet1_srcTime;	// time 1st packet left src node,
	double packet1_dstTime;	// time 1st packet reached dst node
	double packetL_srcTime;	// time last packet left src node
	double packetL_dstTime;	// time last packet reached dst node
	double flowDuration;		// Apr 24
	double flowBandwidth;		// Apr 24
};

struct HWLDataCollected {
	int flowid;				// flow id
	int totalSrcRequests;	// total requests
	int totalSrcRequestSuccess;	// total success
	int totalSrcRequestFailed;	// total failed
	int totalSrcRequestNotCorrect;// HostList gave me hostid, but other constraints failed
	int totalDstRequests;	// total requests
	int totalDstRequestSuccess;	// total success
	int totalDstRequestFailed;	// total failed
	int totalDstRequestNotCorrect;// HostList gave me hostid, but other constraints failed
	int totalNonHWL_dstRequests;// total dst TO without HWL, by Sanjeev:random dst host
	int totalNonHWL_dstRequestFailed;// total dst failed without HWL, by Sanjeev:random dst host
	int totalNonHWL_srcRequests;// total src TO without HWL, by Sanjeev:random src host
	int totalNonHWL_srcRequestFailed;// total dst failed without HWL, by Sanjeev:random src host
	int totalSrcHostEqual;
};

typedef struct datachunk{
	uint32_t chunk_number;
	double intensity_sum; //in
	double intensity_sum_out;
	uint32_t chunk_count;
	int processed;
	int highCopyCount;
	int emerCopyCount;
	datachunk()
	{
		chunk_number = simulationRunProperties::total_chunk + 1;
		intensity_sum = 0.0;
		intensity_sum_out = 0.0;
		highCopyCount=0;
		emerCopyCount=0;
		processed=0;
		chunk_count=0;
	}
}Dchunk;

typedef struct Fat_Node {
	uint32_t node_number;
	double utilization; //in
	double utilization_out;
	double max_capacity_left;
	Dchunk *data;
	uint32_t total_chunks;
}Fat_tree_Node;

typedef struct Fat_pod{
	uint32_t pod_number; //not necessary since you can use the array index of the following array
	double Pod_utilization;
	double Pod_utilization_out;
	Fat_tree_Node *nodes;
}Pod;



typedef struct noden{
int nodenumber;
double U;
int counter;
}nodedata;


typedef struct chunk{
  uint32_t count; //this is also used to index the array called exists
  uint32_t *exists;
  int highCopyCount;
  int emerCopyCount;
  double last_created_timestamp_for_chunk;
  double last_deleted_timestamp_for_chunk;
  uint32_t readCount;
  uint32_t writeCount;
  uint32_t uniqueWrite;
  uint32_t cumulative_write_sum;
  double readUtilization;
  double writeUtilization;
  double first_time_entered;
  double last_observed_utilization;
  double runningAvg;
  double max_instant_utilization;
  uint32_t freq;
  chunk() // @suppress("Class members should be properly initialized")
	{
		count = 0;
		highCopyCount=0;
		emerCopyCount=0;
		last_created_timestamp_for_chunk=0;
		last_deleted_timestamp_for_chunk=0;
		readCount=0;
		writeCount=0;
		readUtilization=0.0;
		writeUtilization=0.0;
		first_time_entered=0.0;
		uniqueWrite=0;
		cumulative_write_sum=0;
		last_observed_utilization=0.0;
		runningAvg=0.0;
		freq=0;
		max_instant_utilization=0.0;
	}
}chunkCopy;


//Madhurima added on July 20th
typedef struct result{
	uint32_t src;
	uint32_t dest;
	uint32_t chunk_number;
}Result;

//Madhurima added on July 20th
class chunk_info{
public:
	uint32_t chunk_no;
	Ipv4Address address;
	uint32_t node_id;
	bool is_latest;
	uint32_t logical_node_id;
	uint32_t version_number;
	uint32_t number_of_copy;
	chunk_info()
	{
		chunk_no = 0;
		address = "0.0.0.0";
		node_id = -1;
		is_latest = false;
		logical_node_id = -1;
		version_number = 0;
		number_of_copy = 0;
	}

	chunk_info(uint32_t chunk_no, Ipv4Address address, uint32_t node_id, uint32_t logical_node_id, bool is_latest=false)
	{
		this->chunk_no = chunk_no;
		this->address = address;
		this->node_id = node_id;
		this->is_latest = is_latest;
		this->logical_node_id = logical_node_id;
		this->version_number = 0;
		this->number_of_copy = 0;
	}
};



class ApplicationMetrics
{
public:
	double avg_delay;
	uint32_t total_flows;

	ApplicationMetrics()
	{
		this->avg_delay = DoubleValue(0).Get();
		this->total_flows = 0;
	}
};

class HostBandwidthRequirement
{
public:
	double *hosts;

	HostBandwidthRequirement()
	{
		uint32_t total_hosts = (simulationRunProperties::k * simulationRunProperties::k * simulationRunProperties::k)/4;
		hosts = new double[total_hosts];
		for(uint32_t i=0;i<total_hosts;i++)
		{
			hosts[i] = 0.0;
		}
	}
};



/*
 *
 */
class BaseTopology {
public:

	/**
	 * \brief Constructor. (k = number of hosts per pod)
	 */
	BaseTopology(void);

	/**
	 * \brief Destructor.
	 */
	virtual ~BaseTopology(void);
	void InitializeNetAnim(void); // call this as 1st step, input =  netAnim output file

	// installs a sample udpApplications on random hosts
	virtual void InstallSampleApplications(void);

	virtual void DoRun(void); // runs the simulation....writes output to netAnim file
	virtual void ReadConfiguration(void); // reads the config file and set properties
	virtual void AddEnergyModel(void);

	virtual void UpdateNodeCounters(void);
	// new initial flows as per MR...for flow generation. sanjeev 3/18
	virtual void InjectInitialFlowsMR(const int &requiredInitialFlowCount);

	// call backs..	for flow generation. sanjeev 3/18
	virtual void InjectNewFlowCallBack(const uint32_t &flowid,
			const uint16_t &src, const uint16_t &dst, const uint16_t &flowBW);
	// for flow generation adjustment. sanjeev 4/17
	virtual bool markFlowStoppedMetric(const uint32_t &flowId,
			const uint16_t &srcNodeId, const uint16_t &dstNodeId,
			const uint16_t &flowBW);

	static void InjectANewRandomFlowCopyCreation(uint32_t src, uint32_t dest, uint32_t number_of_packets, bool read_flow = true, uint32_t required_bandwidth = 100, bool non_consistent_read_flow = false, double finish_time= 40.0, uint32_t app_id = -1);//JB change Dec 12

	static FlowDataCollected *m_flowData;
	static std::ofstream fpDeviceEnergy;

	/******Chunk Specific Change *******************/
	static std::vector<chunk_info> chunkTracker;
	static Ipv4Address* hostTranslation;
	static uint32_t* hostaddresslogicaltophysical;
	static std::vector<MultipleCopyOfChunkInfo> chunkCopyLocations;
	static uint32_t getChunkLocation(uint32_t chunk_number);
	static Ipv4Address getChunkLocationIP(uint32_t chunk_number);
	static uint32_t getChunkLocationHostId(uint32_t chunk_number);
	int *frequency_values;
	static uint32_t num_of_retried_packets;
	static uint32_t num_of_retired_packets_specific_node;

	static uint32_t total_number_of_packet_for_copy_creation;

	static double getMinUtilizedServerInRack(uint32_t rack_id);

	static uint32_t **application_assignment_to_node;
	static uint32_t **chunk_assignment_to_applications;
	static double **chunk_assignment_probability_to_applications;

	static uint32_t *virtual_to_absolute_mapper;

	Ptr<RandomVariableStream> application_assigner;
	static Ptr<RandomVariableStream> application_selector;

	static Pod *p;
	static Pod *q;

	static uint32_t pkt_sent_during_phase1;

	static uint32_t pkt_sent_during_phase2;

	static uint32_t pkt_sent_during_phase3;

	static double tail_latency;

	static bool write_flow_tail;

	static uint32_t sleeping_nodes;

	static Result *res;
	static void calculateNewLocation(int incrDcr); //this function will set the variable Tuple t

	static bool createflag;

	static int counter_;
	static int Incrcounter_;

	static chunkCopy *chnkCopy;

	static int copy_created;

	static int copy_deleted;

	static nodedata *nodeOldUtilization;;

	static nodedata *nodeU;

	static double popularity_change_simulation_interval;

	static Ptr<RandomVariableStream> Popularity_Change_Random_Variable;

	static Ptr<RandomVariableStream> chunk_copy_selection;

	static ApplicationMetrics *application_metrics;

	static uint32_t phrase_change_counter;

	static uint32_t *phrase_change_interval;	  // what would be the next phrase change interval

	static double *phrase_change_intensity_value; // what would be the intensity at some specific point of time

	static double intensity_change_simulation_interval;

	static uint32_t total_phrase_changed;

	static double intensity_change_scale;

	/**********************************************/

	static int total_appication;

	static double sum_delay_ms;

	static uint64_t total_packet_count;

	static uint64_t total_packet_count_inc;

	static uint32_t *host_assignment_round_robin_counter;

	static double *host_running_avg_bandwidth;

	static uint32_t *host_running_avg_counter;

	static double *application_probability;

	static uint32_t current_number_of_flows;

	static uint32_t *total_packets_to_hosts_bits;

	//static uint32_t *total_packets_to_hosts_bits_old;

	static uint32_t total_events;

	static uint32_t total_events_learnt;

	static uint32_t *chunk_version_tracker;

	static uint32_t *chunk_reference_version_tracker;

	static uint32_t **chunk_version_node_tracker;

	static bool **chunk_copy_node_tracker;

	static uint32_t *total_packets_sent_to_chunk;

	////remove this variable after all done

	//static uint32_t *total_packets_to_chunk;

	static uint32_t *total_packets_to_chunk_destination;

	static double last_point_of_entry;

	static double total_sum_of_entry;

	static double sum_of_number_time_packets;

	static uint64_t sum_of_times;

	static NodeContainer hosts_static;

	static uint32_t consistency_flow;

	static uint32_t total_consistency_packets;
	static uint32_t total_non_consistency_packets;

	static double sum_delay_ms_burst;
	static uint32_t total_events_learnt_burst;

	static double sum_delay_ms_no_burst;
	static uint32_t total_events_learnt_no_burst;

	static uint32_t max_chunk_by_application;

	static uint32_t **transaction_rollback_packets;

	static uint32_t **transaction_rollback_write_tracker;

	static uint32_t rollback_packets;

	static double *host_utilization_outgoing;

	static uint32_t totalWriteCount;

	static uint64_t min_offset;

	static HostBandwidthRequirement **application_statistics;


	static std::list<uint32_t> finished_application_list;

	static uint32_t *latest_flow;

protected:
	virtual void DoDispose(void);
	virtual void DisplayCommandLine(void);

	virtual void DisplayStatistics(void);
	//
	virtual double GetNodeFabricEnergy_NotUsed(Ptr<ssNode>);			// Sanjeev Jun 13
	//
	virtual void UpdateNodeCountersns3(void); // to be called by scheduler, hence public.
	virtual void UpdateNodeCountersCustom(void); // to be called by scheduler, hence public.

	virtual void InjectNewFlow_MarkovModel(void);
	virtual void InjectNewFlow_SawToothModel(void);
	virtual void InjectNewFlow_TickBasedModel(void);
	//virtual void InjectANewRandomFlow(void);
	virtual void InjectANewRandomFlow(bool dummy_application=false);
	// for flow generation adjustment. sanjeev 5/9
	virtual bool markFlowStartedMetric(const uint32_t &flowId,
			const uint16_t &srcNodeId, const uint16_t &dstNodeId,
			const uint16_t &flowBW);
	// for generating our IPv4 addresses...(my shortcut)...
	struct IPV4ADDRESS {
		int sn1, sn2, sn3, sn4;
	};
	/*
	 * * Called from Constructor, k = number of pods !!!
	 * ***** Core function, builds initial fat tree *****
	 */
	virtual void BuildInitialTopology(void) = 0;
	virtual void SetNodeAnimPosition(void) = 0;
	virtual int getRandomClientNode(void);
	virtual int getRandomServerNode(const int &conflictClientNodeId);
	virtual int getRandomClientNode(const int &reqBW);
	virtual int getRandomServerNode(const int &conflictClientNodeId,
			const int &reqBW);

	virtual void InitializeRandomVariables(void);
	virtual void InstallUdpApplicationOnRandomClientServer(void);

	virtual void generateNextMarkovState(void);

	virtual void SetNodeAnimProperties(void);

	virtual void WriteConfiguration(void);

	virtual void AddEnergyModelns3(void);
	virtual void AddEnergyModelCustom(void);

	// monitoring and statistic tools..
	virtual void CollectAllStatisticsAllNodes(void);
	virtual void EnableAllStatisticsAllNodes(void);
	virtual void EnableFlowMonitorStatisticsAllNodes(void);
	virtual void EnablePcapAllDevices(void);
	virtual void EnableTrackRoutingTable(void);
	virtual void DisplayJBFlowStatistics(void);
	virtual void Displayns3FlowMonitorStatistics(void);
	virtual void Writens3FlowMonitorStatisticsToFile(void);
	virtual void WriteStateDurationEnergyGranularAllNodesToFile(void);
	virtual void WriteCustomFlowStatisticsToFile(void);
	virtual void WriteFinalTestOutputToFile(void);
	// apr 23, sanjeev. neat + additional logging...
	virtual void WriteFlowDurationToFile(const int &flowCount,
			const double &appDuration, const int &reqBW,
			const double &appStopTime, const char *modelState,
			const double &param1, const double &param2);
	virtual void WriteVMPlacementMetricsToFile(void);
	virtual void WriteHWLEfficiencyMetricsToFile(
			const uint16_t &currentSelectedNode);
	// Function to create address string from numbers
	virtual char * getNewIpv4Address(void);
	// sanjeev mar 18
	virtual void openApplicationDebugFiles(void);
	virtual void DebugIpv4Address(void);
	virtual void printDebugInfo1(void);

	//
	// life-span of these variables is throughout this class-life, visible in all functions
	//
	NodeContainer hosts;
	NodeContainer allNodes;

	ObjectFactory m_objFactory;
	Time m_EnergyMeterUpdateIntervalSec, m_simStopTime,
	m_internalClientStopTime;
	//FlowMonitorHelper flowmon;
	//Ptr<FlowMonitor> monitor;

	AnimationInterface* m_anim;
	// this is for my debugging only delete later & all its usage
	// sanjeev mar 18, mar 23
	std::ofstream fp1, fpVMPlacement, fpVMCEff, fpNetAnimCounters;
	// sanjeev mar 23. fixed type
	uint32_t totalEnergyCounterId, activeEnergyCounterId,
	sleepTimeEnergyCounterId, leakageEnergyCounterId,
	activeTimeCounterId, sleepTimeCounterId, idleTimeCounterId,
	packetCounterId, sleepStateCounterId, deviceCountCounterId;
	int m_flowCount, m_totalBW, m_totalStoppedFlowCounter;

	HWLDataCollected m_hwlDataMetric;
	TopologyDataCollected *m_totalTreeEnergy;

	MarkovModel m_MarkovModelVariable;
	SawToothModel m_SawToothModel;// Apr 23 sanjeev, clean re-arrange of old code
	TickModel m_TickModel;

	IPV4ADDRESS ipv4Address;
	Ptr<RandomVariableStream> m_randomClientNodeSelector,
	m_randomServerNodeSelector, m_randomBWVariable,
	m_flowStopDurationTimer, m_newStaggeredflowTime;
	//
	// some temp variables repeatedly used by some functions
	// variable life-span is valid within a function only
	//
	Ptr<ssNode> t_client, t_server, t_node;
	Ptr<ssEnergySource> t_eSource;
	ModelEnergyData* t_ptrMED;
	ApplicationContainer t_allClientApps;
	ssUdpEchoClientHelper *t_echoClient;
	Ptr<ssUdpEchoClient> t_clientApp;
	Time t_appStopTimeRandom;
	Ipv4Address t_addr;
	int t_a, t_b, t_x, t_i, t_reqBW;

	//chunk specific change

	virtual uint32_t getCustomizedRandomClientNode(uint32_t &application_id);

	virtual uint32_t getCustomizedRandomClientNodeDummy(uint32_t &application_id);
};

}  //namespace

#endif /* SS_BASE_TOPOLOGY_H_ */
