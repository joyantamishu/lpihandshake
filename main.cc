/*
 * main.cc
 *
 *  Created on: Jul 12, 2016
 *      Author: sanjeev
 */

#include <string.h>
#include <iostream>
#include <ctime>

#include "parameters.h"
#include "ss-fat-tree-topology.h"
#include "ns3/log.h"

#include "ns3/wimax-module.h"
#include "ns3/csma-module.h"
#include "ns3/uan-module.h"
#include "ns3/wifi-module.h"

using namespace ns3;

// There is an execution time bug (coredump), so you need the next few lines as-is (donot modify) - sanjeev
BaseStationNetDevice b;
SubscriberStationNetDevice s;
CsmaNetDevice c;
UanNetDevice u;

//
void setStaticVariablesFromCommandLine(int argc1, char* argv1[]);

/******************************************************/

int main(int argc, char *argv[]) {
	time_t now = time(0);

	// convert now to string form
	char* date = ctime(&now);
	date[strlen(date) - 1] = '\0';
	char startTime[50];
	std::sprintf(startTime, "%s", date);
	NS_LOG_UNCOND(
			"\nStart Program:: My E Topology Simulation Program #25 June 2017 " << date << "\n");
	//
	// Allow the user to override any of the defaults at
	// run-time, via command-line arguments
	//
	BaseTopology* t1;

	setStaticVariablesFromCommandLine(argc, argv);
	// simplified, sanjeev 2/25
	t1 = new FatTreeTopology();
	t1->InstallSampleApplications();
	t1->InitializeNetAnim();
	t1->AddEnergyModel();
	/* runs the simulation.*/
	t1->DoRun();
	delete t1;		// delete/destroying objects prints several statistics

	NS_LOG_UNCOND(
			"\nStart Program:: My E Topology Simulation Program #25 June 2017 " << startTime);
	now = time(0);
	date = ctime(&now);date[strlen(date) - 1] = '\0';
	NS_LOG_UNCOND(
			"End  Program :: My E Topology Simulation Program #25 June 2017 " << date << "\n");





	int flow_count = 0;
	int thresehold_flow = 0;
	double sum_avg_delay;
	for (std::map<uint32_t, flow_info>::const_iterator it = Ipv4GlobalRouting::flow_map.begin(); it != Ipv4GlobalRouting::flow_map.end(); ++it) {


		if(it->second.total_packet_to_destination > 0)
		{
			flow_count++;
			//NS_LOG_UNCOND("it->second.delaysum "<<it->second.delaysum);

			sum_avg_delay+= (it->second.delaysum / (double)it->second.total_packet_to_destination);

			//NS_LOG_UNCOND("sum_avg_delay "<<sum_avg_delay<<" it->second.total_packet_to_destination "<<it->second.total_packet_to_destination);
		}

		else
		{
			NS_LOG_UNCOND("The Flow id not having any packets "<<it->first);
		}

		if (it->second.has_faced_threshold)
			thresehold_flow++;
	}
	NS_LOG_UNCOND(
			"\nStatistics: JB: The Total number of flow that has minimum of 1 packets to the destination " << flow_count << " The number of dropped flow " << thresehold_flow);

//	NS_LOG_UNCOND(
//				"\nStatistics: JB: The avg delay of flows " << sum_avg_delay/(double)flow_count<<" Flow Count "<<flow_count<<" sum_avg_delay "<<sum_avg_delay);

	NS_LOG_UNCOND(
					"\nThe number of inconsistent packets "<<BaseTopology::num_of_retried_packets);

	NS_LOG_UNCOND(
						"\nThe number of versions "<<BaseTopology::chunkTracker.at(0).version_number);
	NS_LOG_UNCOND(
							"\nnum_of_retired_packets_specific_node "<<BaseTopology::num_of_retired_packets_specific_node);


	NS_LOG_UNCOND("Total number of Flows Generated "<<BaseTopology::total_appication);

	double average_delay = BaseTopology::sum_delay_ms/ (double)Ipv4GlobalRouting::total_number_of_packets_to_destination;

	NS_LOG_UNCOND("Avg Delay in ms "<<average_delay<<" in second "<<average_delay/(double)1000000);

	NS_LOG_UNCOND("Avg number of packets in the system "<< (BaseTopology::total_packet_count_inc/Ipv4GlobalRouting::total_number_of_packets_to_destination));

	NS_LOG_UNCOND("Total Packet Count to the Destination "<<Ipv4GlobalRouting::total_number_of_packets_to_destination);

	NS_LOG_UNCOND("Currently Number of packets in the system "<<BaseTopology::total_packet_count);



	return 0;
}


