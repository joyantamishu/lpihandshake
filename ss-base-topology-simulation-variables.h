/*
 * ss-base-topology-simulation-variables.h
 *
 *  Created on: Oct 8, 2016
 *      Author: sanjeev
 */

#ifndef _SS_BASE_TOPOLOGY_SIMULATION_VARIABLES_H_
#define _SS_BASE_TOPOLOGY_SIMULATION_VARIABLES_H_

#define NANOSEC_TO_SECS  						1000000000.0
#define MEGASEC_TO_SECS  						1000000.0
#define NANOSEC_TO_MILLISEC						1000000.0

//Madhurima Changed on May 4*****Link speed should be in 1000 scale, not in scale of 1024
#define MEGABYTES_TO_BYTES  					1000*1000//1024*1024
#define MEGABITS_TO_BITS  						1000*1000//1024*1024
//*******************************************************************************************
#define WAKEUP_BEFORE_SIM_END_NANOSECONDS		1

#define MAXFLOWDATA_TO_COLLECT					64000

#define MegaSec_to_Secs 						MEGASEC_TO_SECS
#define NanoSec_to_Secs 						NANOSEC_TO_SECS

#define useRandomIntervals						(simulationRunProperties::enableMarkovModel || simulationRunProperties::enableSawToothModel || simulationRunProperties::enableTickModel)

#define COMPILE_CUSTOM_ROUTING_CODE				true

#define LOCAL_COPY								false

#define CHUNKSIZE								false






namespace ns3 {

enum PacketPriority {
	LOW, HIGH, UNKNOWN
};

/*
 * brief App Types
 */
enum appType {
	/** clean later */
	UNUSED,
};
/*
 *   I want these simulation properties to be available all over the program... Hence trying something static..
 */
class simulationRunProperties {
public:
	static char* arg0;

	static int k;
	// simplified. sanjeev 2/25
	static int packetSize;
	static uint32_t chunkSize;
	static int deviceQDepth;
	static int initialFlowCount;
	static double simStop;

	// capacity/bandwidth of topology
	static char* coreDeviceDataRate;
	static char* aggrDeviceDataRate;
	static char* edgeDeviceDataRate;
	static char* deviceDataRate1;
	static char* newDevceRate; //Nov 28

	// energy management parameters
	static bool enableDefaultns3EnergyModel; // takes precedent...
	static bool enableLinkEnergyManagement;
	static bool enableDeviceTOSManagement;
	static bool enableDeviceTRSManagement;
	static bool enableNodeFabricManagement;	// sanjeev- May 26, Only controls sleep state & runway interval (but node consumes energy always unless DISABLED!!

	// packet routing & request-response
	static bool echoPackets;
	static bool enableCustomRouting;

	// use sawTooth model with these parameters for flow generation
	static bool enableSawToothModel;
	static int flowMin;
	static int flowMax;

	// takes precedent... Use Markov model with these parameters
	static bool enableMarkovModel;
	static int markovStateInterval;
	static double markovETA0;
	static double markovETA1;

	// Tick model with these parameters, Apr 23
	static bool enableTickModel;
	static double tickStateInterval;

	// use VMC model for src/dst client/server selection
	static bool enableVMC;
	//use to enable the controller, even if the GA enables controller will remain off untill turned on explicitly
    static bool enableController; //Madhurima added this to invoke the controller and monitor separately May 7
    static int controllerFrequency;
	// parameters for flow duration & bandwidth variance
	static int product;
	static double alpha;

	// parameters for VMC
	static int vm_per_node;
	static int flows_per_VM;

	/*********************Added BY Joyanta ******************/
	static double bandwidth_thresehold_percentage;
	//Added By Joyanta June 2
	static int hypercube_radix;
	/********************************************************/

	/////////Added By Joyanta On 1st April //////////////////
	static bool enable_global_algortihm;

	//added by joyanta on 17th july

	static bool round_robin;
	static double probParam;
	static int polID;

	/******Chunk Specific Change *******************/
	static uint32_t total_chunk;
	static double ChunkzipfGeneratorAlpha;

	static uint32_t total_applications;

	static uint32_t total_sharing_chunks;

	static double popularity_change_start_ms;

	static double popularity_change_interval_ms;

	static double fraction_chunk_changes_popularity;

	static uint32_t phrase_change_number;

	static uint32_t rw_phrase_change_number;

	static double intensity_change_start_ms;

	static bool enableOptimizer;

	static bool uniformBursts;

	static double RWratio;

	static double initial_RWratio;

	static int enableCopy;

	/**********************************************/

private:
	//nobody should construct/ use this class
	simulationRunProperties(void);
	~simulationRunProperties(void);
};

} // namespace

#endif /* _SS_BASE_TOPOLOGY_SIMULATION_VARIABLES_H_ */
