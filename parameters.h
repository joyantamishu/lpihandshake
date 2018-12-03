/*
1 * ss-fat-tree-parameters.h
 *
 *  Created on: Jul 27, 2016
 *      Author: sanjeev
 */

#ifndef _PARAMETERS_H_
#define _PARAMETERS_H_

/*
 * ALL THE BELOW PARAMETERS CAN BE SET FROM COMMAND LINE- See Main.cc and ns3 documentation for details
 * e.g: --run my-e-topology --k=8 --packetCount=50 ....etc....etc...(see main.cc for parameter names).
 */
// k-fat-tree and applications
#define Default_K							4
#define DefaultPacketSize					1400
#define DefaultProduct					    3650//2000
#define DefaultParetoAlpha					3.5			// cannot be less than 2.0

// VM parameters
#define DEFAULT_VM_PER_NODE					2
#define DEFAULT_FLOWS_PER_VM				2

// flow parameters & flow duration
#define TESTFLOWSTOPTIME_MILLISEC				7.5
#define MINFLOWSTOPTIME_MILLISEC				15//7.5//37.5
#define MAXFLOWSTOPTIME_MILLISEC				375.0
#define DEFAULT_INITIAL_PARETO_ALPHA_DONOT_CHANGE	2.5		// cannot be less than 2.0

// if random variables are not used, these are the constant variables.
#define TESTFLOWBANDWIDTH					20
#define bw_lower_limit						0.5
#define bw_upper_limit						1.5

#define	DefaultSimulatorStopTimeSec			1.0
#define DefaultInitialFlowCount				5				// sanjeev Apr 25

// Saw tooth model parameter
#define DefaultUseSawToothModel				false
#define DefaultFlowMax						9				// see random value generation
#define DefaultFlowMin						3

// Tick model parameter
#define DefaultUseTickBasedModel			false
#define DefaultTimeDurationMilliSec			.6//4.5				// Apr 23, New Discussion

// Markov Model parameters
#define DefaultUseMarkovModel				false
#define DefaultMarkovStateInterval			100		// millisec
#define DefaultMarkovETA0Value				1 //0.5 //Madhurima Changed on May 4***** Simple constant Model in default
#define DefaultMarkovETA1Value				0 //0.1 //Madhurima Changed on May 4***** Simple constant Model in default

// network device parameters
#define CoreDeviceLinkDataRate 						"1000Mbps" //Madhurima Changed on May 4*****Link speed should be in 1000 scale, not in scale of 1024
#define AggrDeviceLinkDataRate 						"1000Mbps" //Madhurima Changed on May 4*****Link speed should be in 1000 scale, not in scale of 1024
#define EdgeDeviceLinkDataRate 						"100Mbps" //Madhurima Changed on May 4*****Link speed should be in 1000 scale, not in scale of 1024
#define Count										100
#define MAX_QUEUE_DEPTH 							0			// port queue depth threshold before dropping packets (vary this to test diff results in packet drop), set non-zero value
#define myLinkRate 						"1000Mbps"

// should we enable energy management -monitor idle time, sleep state etc? true/false?
#define USE_DEFAULT_NS3_ENERGY_MODEL			false			// takes precedence...Apr 24

#define ENABLE_LINK_ENERGY_MANAGEMENT			false
#define ENABLE_DEVICE_TOS_MANAGEMENT			false   // Madhurima Changed the default mode as false as on April 21
#define ENABLE_DEVICE_TRS_MANAGEMENT			false	// Madhurima Changed the default mode as false as on April 21
#define	ENABLE_NODE_FABRIC_MANAGEMENT			false	// Sanjeev. May 26

#define ENABLE_TESTING_CUSTOM_ROUTING			false
#define ECHO_PACKETS_BACK						false
#define ENABLE_VMController						false
//////////////Added By Joyanta on 1st April
#define ENABLE_GLOBAL_ALGORITHM					false

#define DEFAULT_HYPERCUBE_RADIX							1

/**********Added By Joyanta ***************/

#define BANDWIDTH_THRESHOLD_PERCENTAGE_VALUE	1.0 //NOT FUNCTIONAL NOW, IGNORE IT

#define ROUND_ROBIN								false

/******************************************/
#define DEFAULT_ENABLE_GLOBAL_CONTROLLER				false //Madhurima added this to invoke the controller and monitor separately
#define DEFAULT_MONITOR_TO_CONTROLLER_FREQUENCY         50

/******************************************/
// set this to TRUE if you want to disable COMPLETELY node fabric energy, if FALSE, Node "always" consumes some energy
#define COMPLETELY_DISABLE_NODE_FABRIC_ENERGY_MODEL			false

#define PROBABILITY_PARAMETER 										0.4

#define POLICY_NUMBER									3

/******Chunk Specific Change *******************/
#define DEFAULT_CHUNK_NUMBER						1500
#define DEFAULT_CHUNK_GENERATOR_CONSTANT				1
#define READ_WRITE_RATIO						    0.6
#define DUMMY_READ_WRITE_RATIO						1.0

#define DEFAULT_NUMBER_OF_APPLICATIONS				47

#define DEFAULT_NUMBER_OF_DUMMY_APPLICATIONS		10

#define DEFAULT_SHARED_CHUNKS						4

#define DEFAULT_POPULARITY_CHANGE_START_TIME_MS		80.0
#define DEFAULT_POPULARITY_CHANGE_INTERVAL_TIME_MS	200.0
#define DEFAULT_FRACTION_CHUNK_CHANGES_POPULARITY	0.05 //turn it to zero to make the popularity change feature deactivated

#define DEFAULT_INTENSITY_CHANGE_START_TIME_MS		80.0

#define DEFAULT_NUMBER_OF_INTENSITY_PHRASE_CHANGE	4

#define DEFAULT_LOCAL_ACCESS_LATENCY				0 //100 microsecond

#define SSD_PER_RACK								8

#define CONSISTENCY_FLOW_DUARTION_CONSTANT			50

#define CONSISTENCY_FLOW_START_DUARTION_CONSTANT	0.0001
/**********************************************/
#endif /* _PARAMETERS_H_ */
