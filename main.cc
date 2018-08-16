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


	FILE * fp_host;

	FILE *fp_host_info;

	FILE *fp_host_by_packets;

	FILE *fp_host_info_packets;

	FILE *fp_chunks_by_packets;

	fp_host = fopen ("host_utilization.csv","w");

	fp_host_info = fopen ("host_utilization_by_host.csv","w");

	fp_host_info_packets = fopen ("host_utilization_by_throughput.csv","w");

	fp_host_by_packets = fopen ("host_utilization_by_packets.csv","w");

	fp_chunks_by_packets = fopen("chunk_utilization_by_packets.csv","w");

	uint32_t total_hosts_in_system = (SSD_PER_RACK + 1) * (simulationRunProperties::k/2) * (simulationRunProperties::k/2) * simulationRunProperties::k;

	double total_utilization = 0.0;

	//double utilization_by_packet;

	fprintf(fp_host,"Host,average_arrival_rate,utilization\n");

	fprintf(fp_host_info,"Host,utilization\n");

	fprintf(fp_host_info_packets,"Host,utilization\n");

	fprintf(fp_host_by_packets,"Host, Total Packets, utilization_by_flow_calculation, utilization_by_packets\n");

	fprintf(fp_chunks_by_packets,"Chunk, Total Packets, Total Packets Destination, Difference\n");

	uint32_t frequency;

	//uint32_t num_of_packets;

	for(uint32_t i=0;i<total_hosts_in_system;i++)
	{
		double utilization;

		frequency = BaseTopology::host_running_avg_counter[i];

		if(BaseTopology::host_running_avg_counter[i] == 0)
		{
			frequency =1;//BaseTopology::host_running_avg_counter[i] = 1;
		}
		utilization = BaseTopology::host_running_avg_bandwidth[i]/(double)frequency;

		utilization = utilization / (double)Count;

		utilization = utilization * 100.0;

		total_utilization += utilization;

		//NS_LOG_UNCOND(" total_utilization before calculation"<<total_utilization);

		if(i % (SSD_PER_RACK+1) == 0 && i>0)
		{
			//NS_LOG_UNCOND(" total_utilization "<<total_utilization);
			if(total_utilization > 0.0) fprintf(fp_host_info,"%d,%lf\n", (i/(SSD_PER_RACK+1)), total_utilization/(double)(SSD_PER_RACK));

			total_utilization = 0.0;
		}

		if(BaseTopology::host_running_avg_counter[i]>0) fprintf(fp_host,"%d,%lf,%lf\n",i,BaseTopology::host_running_avg_bandwidth[i]/(double)BaseTopology::host_running_avg_counter[i],utilization);
		//NS_LOG_UNCOND("Host No "<<i<<" Running Avg of utilization "<<BaseTopology::host_running_avg_bandwidth[i]/(double)BaseTopology::host_running_avg_counter);

	}

	total_utilization = 0.0;

	for(uint32_t i=0;i<total_hosts_in_system;i++)
	{

		//NS_LOG_UNCOND("Host No "<<i<<" Running Avg of utilization "<<BaseTopology::host_running_avg_bandwidth[i]/(double)BaseTopology::host_running_avg_counter);


		double total_capacity = (double) Count * 1000000 * simulationRunProperties::simStop;
		double utilization;

		utilization = (BaseTopology::total_packets_to_hosts_bits[i] * simulationRunProperties::packetSize * 8)/total_capacity;

		utilization = utilization * 100.0;

		total_utilization += utilization;

		//NS_LOG_UNCOND(" total_utilization before calculation"<<total_utilization);

		if(i % (SSD_PER_RACK+1) == 0 && i>0)
		{
			//NS_LOG_UNCOND(" total_utilization "<<total_utilization);
			if(total_utilization > 0.0) fprintf(fp_host_info_packets,"%d,%lf\n", (i/(SSD_PER_RACK+1)), total_utilization/(double)(SSD_PER_RACK));

			total_utilization = 0.0;
		}
	}

	//NS_LOG_UNCOND("#of sampling "<< BaseTopology::host_running_avg_counter);

	total_utilization = 0.0;

	for(uint32_t i=0;i<total_hosts_in_system;i++)
	{
		double total_capacity = (double) Count * 1000000 * simulationRunProperties::simStop;


		double utilization = (BaseTopology::total_packets_to_hosts_bits[i] * simulationRunProperties::packetSize * 8)/total_capacity;

		utilization = utilization * 100.0;

		total_utilization += utilization;

		//NS_LOG_UNCOND(" total_utilization before calculation"<<total_utilization);

		if(i % (SSD_PER_RACK+1) == 0 && i>0)
		{
			//NS_LOG_UNCOND(" total_utilization "<<total_utilization);
			//if(total_utilization > 0.0) fprintf(fp_host_info,"%d,%lf,%d\n", (i/(SSD_PER_RACK+1)), total_utilization/(double)(SSD_PER_RACK), (BaseTopology::total_packets_to_hosts_bits[i]/(simulationRunProperties::packetSize*8)));

			//total_utilization = 0.0;
		}

		if(BaseTopology::host_running_avg_counter[i]>0) fprintf(fp_host_by_packets,"%d, %d, %lf, %lf\n",i,BaseTopology::total_packets_to_hosts_bits[i],BaseTopology::host_running_avg_bandwidth[i]/(double)BaseTopology::host_running_avg_counter[i],utilization);
		//NS_LOG_UNCOND("Host No "<<i<<" Running Avg of utilization "<<BaseTopology::host_running_avg_bandwidth[i]/(double)BaseTopology::host_running_avg_counter);
	}

	for(uint32_t chunk_id=0;chunk_id<simulationRunProperties::total_chunk;chunk_id++)
	{
		fprintf(fp_chunks_by_packets,"%d,%d,%d,%d\n",chunk_id, BaseTopology::total_packets_to_chunk[chunk_id], BaseTopology::total_packets_to_chunk_destination[chunk_id], BaseTopology::total_packets_to_chunk[chunk_id]- BaseTopology::total_packets_to_chunk_destination[chunk_id]);
	}


	fclose(fp_host);

	fclose(fp_host_info);

	fclose(fp_host_by_packets);

	fclose(fp_host_info_packets);

	fclose(fp_chunks_by_packets);

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

	double average_delay = BaseTopology::sum_delay_ms/ (double)BaseTopology::total_events_learnt;

	NS_LOG_UNCOND("Avg Delay in ms "<<average_delay<<" in second "<<average_delay/(double)1000000);

	NS_LOG_UNCOND("Avg number of packets in the system "<< (BaseTopology::total_packet_count_inc/BaseTopology::total_events));

	NS_LOG_UNCOND("Total Packet Count to the Destination "<<Ipv4GlobalRouting::total_number_of_packets_to_destination);

	NS_LOG_UNCOND("Total Packet Count to the Destination after system warmed up "<<BaseTopology::total_events_learnt);

	NS_LOG_UNCOND("Currently Number of packets in the system "<<BaseTopology::total_packet_count);

	NS_LOG_UNCOND("The avg no of packets "<<BaseTopology::sum_of_number_time_packets/BaseTopology::total_sum_of_entry);





	return 0;
}


