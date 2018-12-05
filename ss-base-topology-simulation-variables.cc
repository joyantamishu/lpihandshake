/*

 * ss-base-topology-simulation-variables.cc
 *
 *  Created on: Jul 12, 2016
 *      Author: sanjeev
 */
#include <string.h>
#include "ns3/log.h"
#include "ns3/core-module.h"
#include "ns3/ipv4-global-routing.h"
#include "ss-base-topology-simulation-variables.h"
#include "parameters.h"

using namespace ns3;

/*
 *   I want these simulation properties to be available all over the program... Hence trying something static..
 */
char* simulationRunProperties::arg0 = NULL;
int simulationRunProperties::k = Default_K;
int simulationRunProperties::packetSize = DefaultPacketSize;
int simulationRunProperties::chunkSize=DefaultChunkSize;
// simplified. sanjeev 2/25
int simulationRunProperties::deviceQDepth = MAX_QUEUE_DEPTH;
int simulationRunProperties::initialFlowCount = DefaultInitialFlowCount;

double simulationRunProperties::simStop = DefaultSimulatorStopTimeSec;
char* simulationRunProperties::coreDeviceDataRate = (char*) malloc(40);	// copy data later
char* simulationRunProperties::aggrDeviceDataRate = (char*) malloc(40);	// copy data later
char* simulationRunProperties::edgeDeviceDataRate = (char*) malloc(40);	// copy data later
char* simulationRunProperties::deviceDataRate1 = (char*) malloc(40);// copy data later
char* simulationRunProperties::newDevceRate = (char*) malloc(40);// copy data later

bool simulationRunProperties::enableDefaultns3EnergyModel =
USE_DEFAULT_NS3_ENERGY_MODEL; // takes precedent...
bool simulationRunProperties::enableLinkEnergyManagement =
ENABLE_LINK_ENERGY_MANAGEMENT;
bool simulationRunProperties::enableDeviceTOSManagement =
ENABLE_DEVICE_TOS_MANAGEMENT;
bool simulationRunProperties::enableDeviceTRSManagement =
ENABLE_DEVICE_TRS_MANAGEMENT;
bool simulationRunProperties::enableNodeFabricManagement =
ENABLE_NODE_FABRIC_MANAGEMENT;
//
bool simulationRunProperties::enableCustomRouting =
ENABLE_TESTING_CUSTOM_ROUTING;
bool simulationRunProperties::echoPackets = ECHO_PACKETS_BACK;

bool simulationRunProperties::enableSawToothModel = DefaultUseSawToothModel;
int simulationRunProperties::flowMin = DefaultFlowMin;
int simulationRunProperties::flowMax = DefaultFlowMax;

bool simulationRunProperties::enableMarkovModel = DefaultUseMarkovModel;
int simulationRunProperties::markovStateInterval = DefaultMarkovStateInterval;
double simulationRunProperties::markovETA0 = DefaultMarkovETA0Value;
double simulationRunProperties::markovETA1 = DefaultMarkovETA1Value;

// Tick model with these parameters, Apr 23
bool simulationRunProperties::enableTickModel = DefaultUseTickBasedModel;
double simulationRunProperties::tickStateInterval = DefaultTimeDurationMilliSec;

bool simulationRunProperties::enableVMC = ENABLE_VMController;

// parameters for flow duration & bandwidth variance
int simulationRunProperties::product = DefaultProduct;
double simulationRunProperties::alpha = DefaultParetoAlpha;

// parameters for VMC
int simulationRunProperties::vm_per_node = DEFAULT_VM_PER_NODE;
int simulationRunProperties::flows_per_VM = DEFAULT_FLOWS_PER_VM;

/*********************Added By Joyanta **************************/
double simulationRunProperties::bandwidth_thresehold_percentage =
BANDWIDTH_THRESHOLD_PERCENTAGE_VALUE;

//Added By Joyanta June 2
int simulationRunProperties::hypercube_radix = DEFAULT_HYPERCUBE_RADIX;
/****************************************************************/
bool simulationRunProperties::enableController =
DEFAULT_ENABLE_GLOBAL_CONTROLLER; //Madhurima added this parameter on May 7
int simulationRunProperties::controllerFrequency =
DEFAULT_MONITOR_TO_CONTROLLER_FREQUENCY;

////////////////Added By Joyanta on 1st April ///////////////
bool simulationRunProperties::enable_global_algortihm = ENABLE_GLOBAL_ALGORITHM;
////////////////////////////////////////////////////////////
//
/******************************************************/

//Added by Joyanta on 17th july

bool simulationRunProperties::round_robin = ROUND_ROBIN;

double simulationRunProperties::probParam =PROBABILITY_PARAMETER;

int simulationRunProperties::polID=POLICY_NUMBER;

/******Chunk Specific Change *******************/
uint32_t simulationRunProperties::total_chunk = DEFAULT_CHUNK_NUMBER;

double simulationRunProperties::ChunkzipfGeneratorAlpha = DEFAULT_CHUNK_GENERATOR_CONSTANT;

uint32_t simulationRunProperties::total_applications = DEFAULT_NUMBER_OF_APPLICATIONS;

uint32_t simulationRunProperties::total_sharing_chunks = DEFAULT_SHARED_CHUNKS;

double simulationRunProperties::popularity_change_start_ms = DEFAULT_POPULARITY_CHANGE_START_TIME_MS;

double simulationRunProperties::popularity_change_interval_ms = DEFAULT_POPULARITY_CHANGE_INTERVAL_TIME_MS;

double simulationRunProperties::fraction_chunk_changes_popularity = DEFAULT_FRACTION_CHUNK_CHANGES_POPULARITY;

uint32_t simulationRunProperties::phrase_change_number = DEFAULT_NUMBER_OF_INTENSITY_PHRASE_CHANGE;

double simulationRunProperties::intensity_change_start_ms = DEFAULT_INTENSITY_CHANGE_START_TIME_MS;

//int Ipv4GlobalRouting::totalchunk=simulationRunProperties::total_chunk;

/***********************************************/


void setStaticVariablesFromCommandLine(int argc, char *argv[]) {

	std::strcpy(simulationRunProperties::coreDeviceDataRate,
	CoreDeviceLinkDataRate);
	std::strcpy(simulationRunProperties::aggrDeviceDataRate,
	AggrDeviceLinkDataRate);
	std::strcpy(simulationRunProperties::edgeDeviceDataRate,
	EdgeDeviceLinkDataRate);

	std::strcpy(simulationRunProperties::newDevceRate,
			myLinkRate);
	simulationRunProperties::arg0 = argv[0];

	//
	// Allow the user to override any of the defaults at
	// run-time, via command-line arguments
	//
	CommandLine cmd;
	cmd.AddValue("k", "k number of pods in Fat Tree",
			simulationRunProperties::k);
	cmd.AddValue("packetSizeDoNotChange", "Size of each packet",
			simulationRunProperties::packetSize);
	cmd.AddValue("initialFlowCount", "Initial flow count for cont. run apps",
			simulationRunProperties::initialFlowCount);
	cmd.AddValue("flowMin",
			"Random flow range counter to stop/inject random flows(min)",
			simulationRunProperties::flowMin = DefaultFlowMin);
	cmd.AddValue("flowMax",
			"Random flow range counter to stop/inject random flows(max)",
			simulationRunProperties::flowMax = DefaultFlowMax);
	// simplified. sanjeev 2/25
	cmd.AddValue("simStop", "Simulation Stop time in seconds",
			simulationRunProperties::simStop);
	cmd.AddValue("deviceQDepth", "Max Device Queue depth (no of packets)",
			simulationRunProperties::deviceQDepth);
	cmd.AddValue("useNS3", "Use Default ns3 Energy Model (true/false)",
			simulationRunProperties::enableDefaultns3EnergyModel); // takes precedent...
	cmd.AddValue("enableLink", "Enable Link Energy Management (true/false)",
			simulationRunProperties::enableLinkEnergyManagement);
	cmd.AddValue("enableTOS", "Enable Device Energy Management (true/false)",
			simulationRunProperties::enableDeviceTOSManagement);
	cmd.AddValue("enableTRS", "Enable Device LPI model (true/false)",
			simulationRunProperties::enableDeviceTRSManagement);

	if (!COMPLETELY_DISABLE_NODE_FABRIC_ENERGY_MODEL) {
		// don't read user input if disabled, always set to FALSE..
		cmd.AddValue("enableNode",
				"Enable Node Fabric Energy model (true/false)",
				simulationRunProperties::enableNodeFabricManagement);
		// sanjeev- May 26, Only controls sleep state & runway interval (but node consumes energy always unless DISABLED!!
	} else
		NS_LOG_UNCOND(
				" COMPLETELY_DISABLE_NODE_FABRIC_ENERGY_MODEL is " << (COMPLETELY_DISABLE_NODE_FABRIC_ENERGY_MODEL?"true":"false"));

	cmd.AddValue("enableCustomRouting", "Enable Custom Routing (true/false)",
			simulationRunProperties::enableCustomRouting);
	cmd.AddValue("echoPackets",
			"Enable packets to be echoed back from server (true/false)",
			simulationRunProperties::echoPackets);
	// SawTooth
	cmd.AddValue("enableSawToothModel",
			"Use SawToothModel for injecting new flows (true/false)",
			simulationRunProperties::enableSawToothModel);
	// Markov
	cmd.AddValue("enableMarkovModel",
			"Use Markov Model for injecting new flows (true/false)",
			simulationRunProperties::enableMarkovModel);
	cmd.AddValue("markovStateInterval",
			"Markov Model T0 T1 State intervals (millisec)",
			simulationRunProperties::markovStateInterval);
	cmd.AddValue("markovETA0", "Markov Model ETA0 State value",
			simulationRunProperties::markovETA0);
	cmd.AddValue("markovETA1", "Markov Model ETA1 State value",
			simulationRunProperties::markovETA1);
	// Apr 23
	cmd.AddValue("enableTickModel",
			"Use Tick Model for injecting new flows (true/false)",
			simulationRunProperties::enableTickModel);
	cmd.AddValue("tickInterval", "Tick Model State intervals (millisec)",
			simulationRunProperties::tickStateInterval);

	cmd.AddValue("enableVMC",
			"Use VM Controller for choosing hosts (true/false)",
			simulationRunProperties::enableVMC);
	cmd.AddValue("alpha",
			"Alpha for random variance for BW & Flow Duration (double)",
			simulationRunProperties::alpha);
	cmd.AddValue("product",
			"Product for random variance for BW & Flow Duration (int)",
			simulationRunProperties::product);
	cmd.AddValue("vmPerNode", "VMs per Node (int)",
			simulationRunProperties::vm_per_node);
	cmd.AddValue("flowsPerVM", "Flows per VM (int)",
			simulationRunProperties::flows_per_VM);

	/////////////////Added By Joyanta on 1st April ///////////////////////
	cmd.AddValue("enableGA", "Enable Calling Global Algorithm",
			simulationRunProperties::enable_global_algortihm);
	//////////////////////////////////////////////////////////////////////

	/********************Added By Joyanta ********************************/
	cmd.AddValue("bwThreshold",
			"Percentage of bandwidth threshold(value between 0.6 and 1.0)",
			simulationRunProperties::bandwidth_thresehold_percentage);
	//Added By Joyanta June 2
	cmd.AddValue("hypercubeRadix", "Hypercube Radix (total nodes = Radix * 8)",
			simulationRunProperties::hypercube_radix);
	/*********************************************************************/
	cmd.AddValue("enableControl",
			"based on the parameter it determines whether to call Global controller or not)",
			simulationRunProperties::enableController); ///Madhurima Added this on 7th May

	cmd.AddValue("controllerFreq",
			"This is to define how frequently you want to call the controller, here default 50 times you call the GM once GC is called)",
			simulationRunProperties::controllerFrequency); ///Madhurima Added this on 7th May

	//added by joyanta on 17th july

	cmd.AddValue("enableRoundRobin",
				"based on the parameter it determines whether to call Customized RoundRobin or Not)",
				simulationRunProperties::round_robin);

	cmd.AddValue("probInterPodDestination",
					"This is the probability you want to define for a flow destined outside of a pod)",
					simulationRunProperties::probParam);


	cmd.AddValue("policyNumber",
					"This is the policy under which you would like to run your code)",
					simulationRunProperties::polID);

	cmd.Parse(argc, argv);
	if (simulationRunProperties::enableTickModel) { // takes precedent... Rearranged precedence, Apr26
		simulationRunProperties::enableSawToothModel = false;
		simulationRunProperties::enableMarkovModel = false;
	} else if (simulationRunProperties::enableSawToothModel) {
		simulationRunProperties::enableTickModel = false;
		simulationRunProperties::enableMarkovModel = false;
	} else if (simulationRunProperties::enableMarkovModel) {
		simulationRunProperties::enableSawToothModel = false;
		simulationRunProperties::enableTickModel = false;
	}  // no else, if nothing is set- constant model is used.

	if (simulationRunProperties::enableDeviceTRSManagement) { // Apr 26, highest precedence,
		simulationRunProperties::enableDefaultns3EnergyModel = false; //sets other cross-dependent params...
		simulationRunProperties::enableDeviceTOSManagement = true;
	} else if (simulationRunProperties::enableDeviceTOSManagement) { // sets other cross-dependent  params...
		simulationRunProperties::enableDeviceTRSManagement = false;
		simulationRunProperties::enableDefaultns3EnergyModel = false;
	} else if (simulationRunProperties::enableDefaultns3EnergyModel) { // takes precedent...
		simulationRunProperties::enableDeviceTOSManagement = false;
		simulationRunProperties::enableDeviceTRSManagement = false;
		simulationRunProperties::enableNodeFabricManagement = false;// sanjeev Jun 13..if ns3default- then switchOFF fabric
	} else {
		simulationRunProperties::enableDefaultns3EnergyModel = true; // Apr 28, if everything is false, set one of them to true
		simulationRunProperties::enableNodeFabricManagement = false;// sanjeev Jun 13..if ns3default- then switchOFF fabric
	}
	// adjust limits to take care of any overflow
	if (simulationRunProperties::flowMax
			>= simulationRunProperties::initialFlowCount)
		simulationRunProperties::flowMax =
				simulationRunProperties::initialFlowCount - 1;
	if (simulationRunProperties::flowMin < 0)
		simulationRunProperties::flowMin = 0;

	//Added By Joyanta June 2
	if (simulationRunProperties::hypercube_radix > 2)
		simulationRunProperties::hypercube_radix = 2; //just for now

#if COMPILE_CUSTOM_ROUTING_CODE
	////Added By Joyanta on 1st April /////////////////////////////////////
	Ipv4GlobalRouting::is_enable_GA =
			simulationRunProperties::enable_global_algortihm;
	Ipv4GlobalRouting::probabilityA=simulationRunProperties::probParam;

	/********************Added By Joyanta ********************************/
//	if (simulationRunProperties::bandwidth_thresehold_percentage < 0.6)
//		simulationRunProperties::bandwidth_thresehold_percentage = 0.6;
//	if (simulationRunProperties::bandwidth_thresehold_percentage > 1.0)
//		simulationRunProperties::bandwidth_thresehold_percentage = 1.0;
	/*********************************************************************/
	Ipv4GlobalRouting::is_ControllerEnabled =
			simulationRunProperties::enableController; //Madhurima added this to invoke the controller and monitor separately May 7
	Ipv4GlobalRouting::is_customized =
			simulationRunProperties::enableCustomRouting;
//	NS_LOG_UNCOND("The custom routing value is "<<simulationRunProperties::enableCustomRouting);
//	NS_LOG_UNCOND("The threshold value is "<<simulationRunProperties::bandwidth_thresehold_percentage);

	//Ipv4GlobalRouting::GA_Call_Event = 10;
	Ipv4GlobalRouting::monitor_and_controller_frequency_ratio =
			simulationRunProperties::controllerFrequency;
	Ipv4GlobalRouting::global_algorithm_calling_checkpoint = 1000; //100 is the best value for monitoring

	Ipv4GlobalRouting::FatTree_k = simulationRunProperties::k;

	Ipv4GlobalRouting::IsRoundRobin = simulationRunProperties::round_robin;


	////AmitangshuDa specific change

	Ipv4GlobalRouting::upper_utilization_threshold = 0.5;


	Ipv4GlobalRouting::N_MAX = 16;

	//Ipv4GlobalRouting::lower_utilization_threshold = 0.9; //give some times to learn the system


	//Dr kant specific change

	Ipv4GlobalRouting::has_system_learnt = false; //if you turn on this variable to true, don't forget to set the value of Ipv4GlobalRouting::lower_utilization_threshold to a reasonable one like - 0.2 or 0.1


	//Ipv4GlobalRouting::utilization_to_be_set = Ipv4GlobalRouting::lower_utilization_threshold;

	Ipv4GlobalRouting::lower_utilization_threshold = 0.9 * Ipv4GlobalRouting::upper_utilization_threshold;

	//Ipv4GlobalRouting::utilization_last_drop = 0.9;
	//Ipv4GlobalRouting::threshold_percentage = 1.0; //you can set the lower threshold value from here too, if you want.

	Ipv4GlobalRouting::policyId=simulationRunProperties::polID;

	double expected_number_of_packet = (double) Count / (double)(simulationRunProperties::packetSize * 8);

	expected_number_of_packet = expected_number_of_packet * (double)(1000000.0);

//	NS_LOG_UNCOND("The expected number of packet is "<<(int)expected_number_of_packet);

	Ipv4GlobalRouting::highest_capacity_of_packets = (int) expected_number_of_packet;

	#endif
}
