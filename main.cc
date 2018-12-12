/*
 * main.cc
 *
 *  Created on: Jul 12, 2016
 *      Author: sanjeev
 */

#include <string.h>
#include <iostream>
#include <fstream>
#include <ctime>
#include <cstdio>
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

	if(remove("host_utilization_madhurima.csv")!=0)
	{
		NS_LOG_UNCOND("error while deleting the file");
	}
//	if(remove("flow_duration_madhurima.csv")!=0)
//	{
//		NS_LOG_UNCOND("error while deleting the file");
//	}
	if(remove("server_level_sleep.csv")!=0)
	{
		NS_LOG_UNCOND("error while deleting the file");
	}
	if(remove("copy_create_delete.csv")!=0)
	{
		NS_LOG_UNCOND("error while deleting the file");
	}

	FILE *fp_packet;
	fp_packet= fopen("all_packets_send.csv","w");
	fprintf(fp_packet,"flowId, requiredBW, sub_flow_dest , sub_flow_dest_physical, application_id, is_First, packet_id, dstNodeId,srcNodeId, is_write, creation_time\n");
	fclose(fp_packet);

	FILE *fp_packet_s;
	fp_packet_s= fopen("all_packets_rcv.csv","w");
	fprintf(fp_packet_s,"flowId, requiredBW, sub_flow_dest , sub_flow_dest_physical, application_id, is_First, packet_id, dstNodeId,srcNodeId, is_write, creation_time, reached_at,e2eDelay\n");
	fclose(fp_packet_s);

	FILE *fp_chunk_utilization;
	fp_chunk_utilization = fopen("chunk_readwrite_utilization.csv","w");
	fprintf(fp_chunk_utilization,"Chunk, RW, no of copies, read_count, read Utilization ,unique write count at send side, cumulative_write, write count ,write Utilization\n");
	fclose(fp_chunk_utilization);

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



//	for(uint32_t chunk=0;chunk<simulationRunProperties::total_chunk;chunk++)
//	{
//		NS_LOG_UNCOND("Chunk id "<<chunk<<" version no "<<BaseTopology::chunk_version_tracker[chunk]);
//	}



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


	FILE *fp_host;

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

	fprintf(fp_chunks_by_packets,"Chunk, Total Packets, Total Packets Destination, Difference, Total Copies\n");


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

//	for(uint32_t chunk_id=0;chunk_id<simulationRunProperties::total_chunk;chunk_id++)
//	{
//		if(BaseTopology::chunkTracker.at(chunk_id).number_of_copy >= 1)
//		{
//			fprintf(fp_chunks_by_packets,"%d,%d,%d,%d,%d\n",chunk_id, BaseTopology::total_packets_to_chunk[chunk_id], BaseTopology::total_packets_to_chunk_destination[chunk_id], BaseTopology::total_packets_to_chunk[chunk_id]- BaseTopology::total_packets_to_chunk_destination[chunk_id], BaseTopology::chunkTracker.at(chunk_id).number_of_copy);
//		}
//	}


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

	NS_LOG_UNCOND("BaseTopology::total_consistency_packets "<<BaseTopology::total_consistency_packets);

	NS_LOG_UNCOND("BaseTopology::total_non_consistency_packets "<<BaseTopology::total_non_consistency_packets);

	double average_delay_burst = BaseTopology::sum_delay_ms_burst/ (double)BaseTopology::total_events_learnt_burst;

	NS_LOG_UNCOND("BaseTopology::sum_delay_ms"<<BaseTopology::sum_delay_ms);

    NS_LOG_UNCOND("BaseTopology::sum_delay_ms_burst"<<BaseTopology::sum_delay_ms_burst);

    NS_LOG_UNCOND("BaseTopology::sum_delay_ms_no_burst"<<BaseTopology::sum_delay_ms_no_burst);

	NS_LOG_UNCOND("BaseTopology::total_events_learnt_burst "<<BaseTopology::total_events_learnt_burst);

	NS_LOG_UNCOND("BaseTopology::total_events_learnt_no_burst "<<BaseTopology::total_events_learnt_no_burst);

	NS_LOG_UNCOND("BaseTopology::total_events_learnt "<<BaseTopology::total_events_learnt);

	double average_delay_no_burst = BaseTopology::sum_delay_ms_no_burst/ (double)BaseTopology::total_events_learnt_no_burst;

	NS_LOG_UNCOND("Avg Delay in ms When there is burst "<<average_delay_burst<<" in second "<<average_delay_burst/(double)1000000);

	NS_LOG_UNCOND("Avg Delay in ms When there is no burst "<<average_delay_no_burst<<" in second "<<average_delay_no_burst/(double)1000000);

	NS_LOG_UNCOND("These are the number of packets sent during any copy creation "<<BaseTopology::total_number_of_packet_for_copy_creation);

	NS_LOG_UNCOND("tail latency "<<BaseTopology::tail_latency);
	if(BaseTopology::write_flow_tail)
		NS_LOG_UNCOND("tail latency faced in write traffic");
	else
		NS_LOG_UNCOND("tail latency faced in read traffic");

	NS_LOG_UNCOND("Total Roll-back Packets "<<BaseTopology::rollback_packets);

	NS_LOG_UNCOND("Number of copy created "<<BaseTopology::copy_created);

	NS_LOG_UNCOND("Number of copy deleted "<<BaseTopology::copy_deleted);

	FILE *fp_stat;
	std::ifstream ifile("Statistics.csv");
	if(ifile)
	{
		fp_stat = fopen ("Statistics.csv","a");
	}
	else
	{	fp_stat = fopen ("Statistics.csv","w");
		fprintf(fp_stat,"enableOptimizer, tickInterval, simStop, RWratio,uniformBursts, Total number of Flows Generated, Total Packets Destination,Currently No of pkts in system, Avg Delay(ms), Avg Delay(s), Average_delay_no_burst(sec),Avg Delay in bursts(sec),Tail Latency(microsec),Total Rollback Pkts,#Copy Created, #Copy Deleted, Consistency Pkts, Non-Consistency Pkts,Copy Creation Pkts \n");
	}
	fprintf(fp_stat,"%d,%f, %f, %f,%d,%d, %d, %lu, %f,%f,%f,%f,%f,%d,%d,%d,%d,%d,%d\n",simulationRunProperties::enableOptimizer, simulationRunProperties::tickStateInterval, simulationRunProperties::simStop,simulationRunProperties::RWratio, simulationRunProperties::uniformBursts,BaseTopology::total_appication,Ipv4GlobalRouting::total_number_of_packets_to_destination,BaseTopology::total_packet_count,average_delay,average_delay/(double)1000000,(average_delay_no_burst/(double)1000000),(average_delay_burst/(double)1000000),BaseTopology::tail_latency,BaseTopology::rollback_packets,BaseTopology::copy_created,BaseTopology::copy_deleted, BaseTopology::total_consistency_packets,BaseTopology::total_non_consistency_packets,BaseTopology::total_number_of_packet_for_copy_creation);
	fclose(fp_stat);

	return 0;
}


