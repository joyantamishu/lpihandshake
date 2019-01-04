/*
 * ss-fat-tree-topology4.cc
 *
 *  Created on: Jul 12, 2016
 *      Author: sanjeev
 */

#include "ss-node.h"
#include "ss-point-to-point-helper.h"
#include "ss-fat-tree-topology.h"
#include "ns3/ipv4-global-routing.h"
#include "ss-base-topology.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("FatTreeTopology");

/*
 *
 */
FatTreeTopology::~FatTreeTopology() {
	NS_LOG_FUNCTION(this);
	NS_LOG_UNCOND("*********FatTreeTopology*********");
}

/*************************************************************/
FatTreeTopology::FatTreeTopology(void) :
	BaseTopology() {
	NS_LOG_FUNCTION(this);
	BuildInitialTopology();
	SetUpInitialOpmizationVariables();


	SetUpNodeUtilizationStatistics();

	SetUpInitialApplicationPosition();

	SetUpInitialChunkPosition();
	SetUpApplicationAssignment();

	SetUpIntensityPhraseChangeVariables();

	//SetUpInitialOpmizationVariables();
}

void FatTreeTopology::SetUpInitialBandwidthMetric(Ipv4Address ip1,
		Ipv4Address ip2, uint32_t total_bandwidth, uint32_t node_id1,
		uint32_t node_id2) {
#if COMPILE_CUSTOM_ROUTING_CODE
	MetricListClass single_metric_entry = MetricListClass();
	single_metric_entry.SetFirstIp(ip1);
	single_metric_entry.SetSecondIP(ip2);
	single_metric_entry.SetTotalBandwidth(total_bandwidth);
	single_metric_entry.SetAvailableBandwidth(total_bandwidth);
	single_metric_entry.SetFirstNodeId(node_id1);
	single_metric_entry.SetSecondNodeId(node_id2);

	Ipv4GlobalRouting::MetricList.push_back(single_metric_entry);

	MetricListClass single_metric_entry1 = MetricListClass();

	single_metric_entry1.SetFirstIp(ip2);
	single_metric_entry1.SetSecondIP(ip1);
	single_metric_entry1.SetTotalBandwidth(total_bandwidth);
	single_metric_entry1.SetAvailableBandwidth(total_bandwidth);
	single_metric_entry1.SetFirstNodeId(node_id2);
	single_metric_entry1.SetSecondNodeId(node_id1);

	Ipv4GlobalRouting::MetricList.push_back(single_metric_entry1);
#endif
}

/******Chunk Specific Change *******************/
void FatTreeTopology::SetUpInitialChunkPosition()
{

	uint32_t chunk_number = simulationRunProperties::total_chunk;

	uint32_t total_hosts = hosts.GetN();

	uint32_t total_hosts_in_pod = total_hosts / simulationRunProperties::k;

	//uint32_t physical_host_number;

	for(uint32_t host_index = 0; host_index < total_hosts ; host_index++)
	{
		BaseTopology::transaction_rollback_packets[host_index] = new uint32_t[total_hosts];
		BaseTopology::transaction_rollback_write_tracker[host_index] = new uint32_t[total_hosts];

		for(uint32_t index=0;index<total_hosts;index++)
		{
			BaseTopology::transaction_rollback_packets[host_index][index] = 0;
			BaseTopology::transaction_rollback_write_tracker[host_index][index] = 0;
		}
	}

	for(uint32_t index= 0 ; index <chunk_number; index++)
	{
		uint32_t logical_node_id = index%total_hosts;
		uint32_t host_id = hosts.Get(logical_node_id)->GetId();
		Ipv4Address corresponding_ip = GetIpv4Address(host_id);
		chunk_info c_info = chunk_info(index, corresponding_ip, host_id,logical_node_id, true);
		BaseTopology::chunkTracker.push_back(c_info);

		BaseTopology::virtual_to_absolute_mapper[index] = index;

		//BaseTopology::total_packets_to_chunk[index] = 0;

		BaseTopology::total_packets_sent_to_chunk[index] = 0;

		BaseTopology::total_packets_to_chunk_destination[index] = 0;

		BaseTopology::chunk_version_tracker[index] = 0;

		BaseTopology::chunk_reference_version_tracker[index] = 0;

		//for(uint32_t host_index=0; host_index<total_hosts/(SSD_PER_RACK+1); host_index++)
		//{
		BaseTopology::chunk_version_node_tracker[index] = new uint32_t[total_hosts];

		BaseTopology::chunk_copy_node_tracker[index] = new bool [total_hosts];

		for (uint32_t host_index = 0; host_index < total_hosts; host_index++)
		{
			BaseTopology::chunk_version_node_tracker[index][host_index] = 0;
			BaseTopology::chunk_copy_node_tracker[index][host_index] = false;
		}
	}

	FILE *fp;
	char str[MAXCHAR];
	const char* filename = "data.txt";

	fp = fopen(filename, "r");
	if (fp == NULL){
		printf("Could not open file %s",filename);
		return;
	}
	char * pch;
	while (fgets(str, MAXCHAR, fp) != NULL)
	{
		printf("%s\n", str);
		if((int)strlen(str) > 1)
		{
			pch = strtok (str," ,;");
			uint32_t count = 0;
			uint32_t value = 0;

			uint32_t logical_host_number;
			//uint32_t physical_host_number;

			uint32_t pod;
			uint32_t node;

			uint32_t round_robin_counter = 0;

			//bool flag=false;
			while (pch != NULL)
			{
				sscanf(pch,"%d",&value);
				if(count == 0)
				{
					//logical_host_number = (SSD_PER_RACK + 1) * value + 1;
					logical_host_number = value;
					//physical_host_number = hosts.Get(logical_host_number)->GetId();
					pod = (uint32_t) floor((double) logical_host_number/ (double) total_hosts_in_pod);
					node = ((logical_host_number - 1)/(SSD_PER_RACK + 1)) % Ipv4GlobalRouting::FatTree_k;
					//printf("-------The chunk node id %d-------------------\n", logical_host_number);

					//NS_LOG_UNCOND("The pod is "<<pod<<" value is "<<value);

					//NS_LOG_UNCOND("The node is "<<node);

				}
				else
				{
					//flag=false;
					//NS_LOG_UNCOND("Value is "<<value);
					value = value -1;
					NS_LOG_UNCOND("chunk number########## "<<value<<" logical_node_id "<<logical_host_number+round_robin_counter);
					//round_robin_counter = (count -1) % (SSD_PER_RACK);
					round_robin_counter = 0;

					//NS_LOG_UNCOND(" logical_host_number "<<logical_host_number+round_robin_counter);


					BaseTopology::chunkTracker.at(value).logical_node_id = logical_host_number+round_robin_counter;
					BaseTopology::chunkTracker.at(value).node_id = hosts.Get(logical_host_number+round_robin_counter)->GetId();

					//NS_LOG_UNCOND("BaseTopology::chunkTracker.at(value).node_id" << BaseTopology::chunkTracker.at(value).node_id);

					BaseTopology::chunkTracker.at(value).number_of_copy ++;

					BaseTopology::chunk_copy_node_tracker[value][logical_host_number+round_robin_counter] = true;

					//BaseTopology::p[pod].nodes[node].data[count-1].chunk_number = value; //bug fix on Oct 8
				    for(uint32_t t=0;t<BaseTopology::p[pod].nodes[node].total_chunks;t++)
				    {
				      if(value==BaseTopology::p[pod].nodes[node].data[t].chunk_number)
				      {
				    	// flag=true;
				    	 BaseTopology::p[pod].nodes[node].data[t].chunk_count++;
				         break;
				      }
				    }
				     //  if(!flag)
					   BaseTopology::p[pod].nodes[node].data[BaseTopology::p[pod].nodes[node].total_chunks].chunk_count = 1;

				       BaseTopology::p[pod].nodes[node].data[BaseTopology::p[pod].nodes[node].total_chunks].chunk_number = value;
					   //
				       BaseTopology::p[pod].nodes[node].total_chunks++;

					//uint32_t host_number =  (logical_host_number - 1)/(SSD_PER_RACK + 1);

					//BaseTopology::chunk_copy_node_tracker[value][host_number] = true;



					//printf ("%d\n",value);

				}
				count++;
				//NS_LOG_UNCOND("The count is "<<count);
				pch = strtok (NULL, " ,;\n");

			}
		}
		//printf("The line length %d\n", (int)strlen(str));

	}
	fclose(fp);

	//reduce the copy number by 1


	for(uint32_t chunk_index =0;chunk_index<chunk_number;chunk_index++)
	{
		NS_LOG_UNCOND("Chunk No "<<chunk_index<< " Position "<<BaseTopology::chunkTracker.at(chunk_index).logical_node_id);
	}


	for(uint32_t chunk_index =0;chunk_index<chunk_number;chunk_index++)
	{
		BaseTopology::chunkTracker.at(chunk_index).number_of_copy --;
	}

	for(uint32_t chunk_index =0;chunk_index<chunk_number;chunk_index++)
	{
		//NS_LOG_UNCOND("Chunk id "<<chunk_index<<" Total Copies "<<BaseTopology::chunkTracker.at(chunk_index).number_of_copy);

		if(BaseTopology::chunkTracker.at(chunk_index).number_of_copy >=1)
		{
			for(uint32_t host_id=0;host_id<total_hosts;host_id++)
			{
				if(BaseTopology::chunk_copy_node_tracker[chunk_index][host_id])
				{
					NS_LOG_UNCOND("Node "<<host_id);
				}
			}
		}
	}

	//exit(0);


	ns3::BaseTopology::Popularity_Change_Random_Variable->SetAttribute("Min", DoubleValue(0));
	ns3::BaseTopology::Popularity_Change_Random_Variable->SetAttribute("Max", DoubleValue(simulationRunProperties::total_chunk-1));


	ns3::BaseTopology::chunk_copy_selection->SetAttribute("Min", DoubleValue(0));
	ns3::BaseTopology::chunk_copy_selection->SetAttribute("Max", DoubleValue(65536));

	//if(BaseTopology::chunk_copy_node_tracker[8][0]) NS_LOG_UNCOND("^^^^^Inside fat tree topolgy ^^^^^^^^^^^^^");

	///Customized Multiple Copy, should remove later

//	BaseTopology::chunkTracker.at(0).number_of_copy = 2;
//
//	BaseTopology::chunkCopyLocations.push_back(MultipleCopyOfChunkInfo(0, 0 , 0));
//	BaseTopology::chunkCopyLocations.push_back(MultipleCopyOfChunkInfo(0, 8, 0));
//	BaseTopology::chunkCopyLocations.push_back(MultipleCopyOfChunkInfo(0, 13, 0));



	//exit(0);
}

Ipv4Address FatTreeTopology::GetIpv4Address(uint32_t node_id)
{
	for (std::list<MetricListClass>::iterator it = Ipv4GlobalRouting::MetricList.begin(); it != Ipv4GlobalRouting::MetricList.end(); it++)
	{
		if (it->GetfirstNodeId() == node_id)
		{
			return it->GetFirstIp();
		}
	}

	return "0.0.0.0";
}

void FatTreeTopology::SetUpApplicationAssignment()
{


	uint32_t max_total_chunk_per_application = 100;

	NS_LOG_UNCOND("Start SetUpApplicationAssignment");

	//for(uint32_t i = 0;i<simulationRunProperties::total_applications;i++)
	for(uint32_t i = 0;i<simulationRunProperties::total_applications+DEFAULT_NUMBER_OF_DUMMY_APPLICATIONS;i++)
	{
		ns3::BaseTopology::chunk_assignment_to_applications[i] = new uint32_t[max_total_chunk_per_application + 1];
		ns3::BaseTopology::chunk_assignment_to_applications[i][0] = 0;
		ns3::BaseTopology::chunk_assignment_probability_to_applications[i] = new double [max_total_chunk_per_application + 1];
		for(uint32_t index=0;index<=max_total_chunk_per_application;index++)
		{
			ns3::BaseTopology::chunk_assignment_probability_to_applications[i][index] = 0.0;
		}

	}

	NS_LOG_UNCOND("Start SetUpApplicationAssignment1");

	FILE *fp;
	char str[MAXCHAR];
	const char* filename = "appfreq.txt";
	
	const char* dummy_filename = "appfreq_dummy.txt";

	fp = fopen(filename, "r");

	if (fp == NULL){
			printf("Could not open file %s",filename);
			return;
	}

	double sum_probabilty = 0;


	while (fgets(str, MAXCHAR, fp) != NULL)
	{
		uint32_t app_id;

		double sum,probability;
		if((int)strlen(str) > 1)
		{
			//printf("%s\n",str);
			sscanf(str,"%d,%lf,%lf",&app_id,&sum, &probability);

			NS_LOG_UNCOND("app_id "<<app_id<<" sum "<<sum<<" probability "<<probability);

			BaseTopology::application_probability[app_id-1] = probability;

			sum_probabilty += probability;
		}
	}

	NS_LOG_UNCOND("&&&&&&&&&&&&&7 The probability sum is "<<sum_probabilty);

	fclose(fp);

	///Dummy Application Set up
	fp = fopen(dummy_filename, "r");

	uint32_t app_start = simulationRunProperties::total_applications;
	if (fp == NULL){
			printf("Could not open file %s",filename);
			return;
	}

	while (fgets(str, MAXCHAR, fp) != NULL)
	{
		uint32_t app_id;

		double sum,probability;

		if((int)strlen(str) > 1)
		{
			//printf("%s\n",str);
			sscanf(str,"%d,%lf,%lf",&app_id,&sum, &probability);

			//NS_LOG_UNCOND("Dummy app_id "<<app_id<<" sum "<<sum<<" probability "<<probability);

			BaseTopology::application_probability[app_id-1+app_start] = probability;
		}
	}

	//exit(0);
	fclose(fp);



	filename = "final.txt";

	fp = fopen(filename, "r");
	if (fp == NULL){
		printf("Could not open file %s",filename);
		return;
	}

	while (fgets(str, MAXCHAR, fp) != NULL)
	{
		uint32_t app, chunk;

		double prob;

		if((int)strlen(str) > 1)
		{
		   sscanf(str,"%d,%d,%lf",&app,&chunk, &prob);

		   chunk = chunk -1;
		   app = app-1;

		   ns3::BaseTopology::chunk_assignment_to_applications[app][0]++;

		   ns3::BaseTopology::chunk_assignment_to_applications[app][ns3::BaseTopology::chunk_assignment_to_applications[app][0]] = chunk;
		   ns3::BaseTopology::chunk_assignment_probability_to_applications[app][ns3::BaseTopology::chunk_assignment_to_applications[app][0]] = prob;
		}

	}


//Dummy final.txt


	filename = "final_dummy.txt";

	fp = fopen(filename, "r");
	if (fp == NULL){
		printf("Could not open file %s",filename);
		return;
	}

	while (fgets(str, MAXCHAR, fp) != NULL)
	{
		uint32_t app, chunk;

		double prob;

		if((int)strlen(str) > 1)
		{
		   sscanf(str,"%d,%d,%lf",&app,&chunk, &prob);

		   //NS_LOG_UNCOND("app, chunk"<<app<<" "<<chunk);

		   chunk = chunk -1;
		   app = app-1 + simulationRunProperties::total_applications;

		   ns3::BaseTopology::chunk_assignment_to_applications[app][0]++;

		   ns3::BaseTopology::chunk_assignment_to_applications[app][ns3::BaseTopology::chunk_assignment_to_applications[app][0]] = chunk;
		   ns3::BaseTopology::chunk_assignment_probability_to_applications[app][ns3::BaseTopology::chunk_assignment_to_applications[app][0]] = prob;

		}

	}
	fclose(fp);

	/*End processing dummy Application*/

	NS_LOG_UNCOND("************Hello There Normalization*****************");
	//normalization
	for(uint32_t i=0;i<simulationRunProperties::total_applications;i++)
	{
		double sum = 0.0;

		for(uint32_t j=1;j<=ns3::BaseTopology::chunk_assignment_to_applications[i][0];j++)
		{
			sum += ns3::BaseTopology::chunk_assignment_probability_to_applications[i][j];
		}

		for(uint32_t j=1;j<=ns3::BaseTopology::chunk_assignment_to_applications[i][0];j++)
		{
			ns3::BaseTopology::chunk_assignment_probability_to_applications[i][j] = ns3::BaseTopology::chunk_assignment_probability_to_applications[i][j]/sum;
		}


	}

	for(uint32_t i=0;i<simulationRunProperties::total_applications;i++)
	{
		if(ns3::BaseTopology::chunk_assignment_to_applications[i][0] >= BaseTopology::max_chunk_by_application)
		{
			BaseTopology::max_chunk_by_application = ns3::BaseTopology::chunk_assignment_to_applications[i][0];
		}

	}

	NS_LOG_UNCOND("End SetUpApplicationAssignment");

	NS_LOG_UNCOND("BaseTopology::max_chunk_by_application "<<BaseTopology::max_chunk_by_application);


}

void FatTreeTopology::SetUpInitialApplicationPosition()
{
	//uint32_t total_applications = simulationRunProperties::total_applications;
	uint32_t total_applications = simulationRunProperties::total_applications + DEFAULT_NUMBER_OF_DUMMY_APPLICATIONS;

//	application_assigner = CreateObject<UniformRandomVariable>();
//	application_assigner->SetAttribute("Min", DoubleValue(0));
//	application_assigner->SetAttribute("Max", DoubleValue(hosts.GetN() - 1));
//
	ns3::BaseTopology::application_selector->SetAttribute("Min", DoubleValue(0));
	ns3::BaseTopology::application_selector->SetAttribute("Max", DoubleValue(1));

//
//	printf("The total number of hosts %d\n", total_hosts);
//
//
	for(int i = 0;i<total_hosts;i++)
	{
		ns3::BaseTopology::application_assignment_to_node[i] = new uint32_t[total_applications+1];
		ns3::BaseTopology::application_assignment_to_node[i][0] = 0;
		BaseTopology::total_packets_to_hosts_bits[i] = 0;
	//	BaseTopology::total_packets_to_hosts_bits_old[i] = 0;

	}

//	for(uint32_t index= 0 ; index <total_applications; index++)
//	{
//		int host = (int)application_assigner->GetInteger();
//		int array_index = ns3::BaseTopology::application_assignment_to_node[host][0] + 1;
//		ns3::BaseTopology::application_assignment_to_node[host][0]++;
//		ns3::BaseTopology::application_assignment_to_node[host][array_index] = index;
////
//		NS_LOG_UNCOND(" host "<<" "<<host);
//	}
//
//	for(uint32_t index= 0 ; index <total_hosts; index++)
//	{
//		printf("host %d has %d applications\n",index, ns3::BaseTopology::application_assignment_to_node[index][0]);
//	}
	FILE *fp;
	char str[MAXCHAR];
	const char* filename = "app.txt";

	const char* dummy_filename = "app_dummy.txt";

	fp = fopen(filename, "r");
	if (fp == NULL){
		printf("Could not open file %s",filename);
		return;
	}
	char * pch;
	while (fgets(str, MAXCHAR, fp) != NULL)
	{
		if((int)strlen(str) > 1)
		{
			uint32_t value = 0;
			uint32_t count = 0;

			uint32_t host;
			printf("The string is %s\n", str);
			pch = strtok (str," ,;");

			while (pch != NULL)
			{
				sscanf(pch,"%d",&value);

				if(count == 0)
				{
					host = (SSD_PER_RACK+1) * value;

					printf("The application node id %d\n", host);
				}
				else
				{
					value = value - 1;
					ns3::BaseTopology::application_assignment_to_node[host][0]++;
					ns3::BaseTopology::application_assignment_to_node[host][count] = value;
					printf ("**%d\n",value);
				}
				count++;
				pch = strtok (NULL, " ,.-\n");

			}
		}

		else
		{
			printf("End of line \n");
		}

	}

	fclose(fp);

fp = fopen(dummy_filename, "r");
	if (fp == NULL){
		printf("Could not open file %s",filename);
		return;
	}

	uint32_t app_start = DEFAULT_NUMBER_OF_APPLICATIONS;

	while (fgets(str, MAXCHAR, fp) != NULL)
	{
		if((int)strlen(str) > 1)
		{
			uint32_t value = 0;
			uint32_t count = 0;

			uint32_t starting_point = 0;

			uint32_t host;
			printf("The string is %s\n", str);
			pch = strtok (str," ,;");

			while (pch != NULL)
			{
				sscanf(pch,"%d",&value);

				if(count == 0)
				{
					host = (SSD_PER_RACK+1) * value;

					printf("The application node id %d\n", host);

					starting_point = ns3::BaseTopology::application_assignment_to_node[host][0];
				}
				else
				{
					value = value - 1;
					value = value + app_start;
					ns3::BaseTopology::application_assignment_to_node[host][0]++;
					ns3::BaseTopology::application_assignment_to_node[host][starting_point+count] = value;
					printf ("**%d\n",value);
				}
				count++;
				pch = strtok (NULL, " ,.-\n");

			}
		}

		else
		{
			printf("End of line \n");
		}

	}


	fclose(fp);

}

void FatTreeTopology::SetUpIntensityPhraseChangeVariables()
{
	BaseTopology::phrase_change_intensity_value = new double[simulationRunProperties::phrase_change_number];
	BaseTopology::phrase_change_interval = new uint32_t[simulationRunProperties::phrase_change_number];

	BaseTopology::phrase_change_intensity_value[0] = 1.0;

	BaseTopology::phrase_change_intensity_value[1] = 1.0;

	BaseTopology::phrase_change_intensity_value[2] = 1.5;

	BaseTopology::phrase_change_intensity_value[3] = 1.0;




	BaseTopology::phrase_change_interval[0] = 1000; //in ms

	BaseTopology::phrase_change_interval[1] = 1000;

	BaseTopology::phrase_change_interval[2] = 2000;

	BaseTopology::phrase_change_interval[3] = 100;


	//exit(0);




	//BaseTopology::phrase_change_intersity_value[0] =
}

void FatTreeTopology::SetUpNodeUtilizationStatistics()
{
	int total_hosts = hosts.GetN();

	Ipv4GlobalRouting::host_utilization = new double[total_hosts];
	Ipv4GlobalRouting::host_utilization_smoothed= new double[total_hosts];
	Ipv4GlobalRouting::host_congestion_flag=new uint32_t[total_hosts];

	for(int i=0;i<total_hosts;i++)
	{
		Ipv4GlobalRouting::host_utilization[i] = 0.0;
		BaseTopology::host_running_avg_bandwidth[i] = 0.0;
		BaseTopology::host_running_avg_counter[i] = 0;
		Ipv4GlobalRouting::host_utilization_smoothed[i]=0.0;
		Ipv4GlobalRouting::host_congestion_flag[i]=0;
		BaseTopology::host_utilization_outgoing[i] = 0.0;
		BaseTopology::host_copy[i] = 0;
	}
}


void FatTreeTopology::SetUpInitialOpmizationVariables()
{

		BaseTopology::p= new Pod[Ipv4GlobalRouting::FatTree_k];
		BaseTopology::q= new Pod[Ipv4GlobalRouting::FatTree_k];
		uint32_t number_of_hosts = (uint32_t)(Ipv4GlobalRouting::FatTree_k * Ipv4GlobalRouting::FatTree_k * Ipv4GlobalRouting::FatTree_k)/ 4;
		uint32_t nodes_in_pod = number_of_hosts / Ipv4GlobalRouting::FatTree_k;
		BaseTopology::chnkCopy=new chunkCopy[simulationRunProperties::total_chunk];

		for(uint32_t i=0;i<(uint32_t)Ipv4GlobalRouting::FatTree_k;i++)
		{

			BaseTopology::p[i].nodes = new Fat_tree_Node[nodes_in_pod];
			BaseTopology::p[i].pod_number = (int) i;
			BaseTopology::p[i].Pod_utilization=0.0;
			BaseTopology::p[i].Pod_utilization_out=0.0;

			BaseTopology::q[i].nodes = new Fat_tree_Node[nodes_in_pod];
			BaseTopology::q[i].pod_number = (int) i;
			BaseTopology::q[i].Pod_utilization=0.0;
			BaseTopology::q[i].Pod_utilization_out=0.0;
		}
		for(uint32_t i = 0; i<number_of_hosts; i++)
		{
			uint32_t pod = (uint32_t) floor((double) i/ (double) Ipv4GlobalRouting::FatTree_k);
			BaseTopology::p[pod].nodes[i%Ipv4GlobalRouting::FatTree_k].node_number = i;
			BaseTopology::p[pod].nodes[i%Ipv4GlobalRouting::FatTree_k].utilization=0.0;
			BaseTopology::p[pod].nodes[i%Ipv4GlobalRouting::FatTree_k].utilization_out=0.0;
			BaseTopology::p[pod].nodes[i%Ipv4GlobalRouting::FatTree_k].data = new Dchunk[simulationRunProperties::total_chunk];
			BaseTopology::p[pod].nodes[i%Ipv4GlobalRouting::FatTree_k].total_chunks = 0;
			BaseTopology::p[pod].nodes[i%Ipv4GlobalRouting::FatTree_k].max_capacity_left = 0.0;

			BaseTopology::q[pod].nodes[i%Ipv4GlobalRouting::FatTree_k].node_number = i;
			BaseTopology::q[pod].nodes[i%Ipv4GlobalRouting::FatTree_k].utilization=0.0;
			BaseTopology::q[pod].nodes[i%Ipv4GlobalRouting::FatTree_k].utilization_out=0.0;
			BaseTopology::q[pod].nodes[i%Ipv4GlobalRouting::FatTree_k].data = new Dchunk[simulationRunProperties::total_chunk];
			BaseTopology::q[pod].nodes[i%Ipv4GlobalRouting::FatTree_k].total_chunks = 0;
			BaseTopology::q[pod].nodes[i%Ipv4GlobalRouting::FatTree_k].max_capacity_left = 0.0;


		}
		for(uint32_t i = 0; i<simulationRunProperties::total_chunk; i++)
		{
			//Madhurima added on July 20th
			BaseTopology::chnkCopy[i].count=0;  //this line added
			//Madhurima added on July 20th
		   BaseTopology::chnkCopy[i].exists=new uint32_t[number_of_hosts];
		   for(uint32_t j = 0; j<number_of_hosts; j++)
			   BaseTopology::chnkCopy[i].exists[j]=0;
		}
		BaseTopology::nodeU=new nodedata[number_of_hosts];

		BaseTopology::nodeOldUtilization=new nodedata[number_of_hosts];

		BaseTopology::res = new Result[simulationRunProperties::total_chunk+1];

		for(uint32_t i=0; i<number_of_hosts;i++)
		{
			BaseTopology::host_assignment_round_robin_counter[i] = 0;
		}



//	for(uint32_t i = 0; i<number_of_hosts; i++)
//	{
//
//		//uint32_t pod = (uint32_t) floor((double) i/ (double) Ipv4GlobalRouting::FatTree_k);
//
//		//for(uint32_t host_count = 0; host_count < )
//	}
}

/***************************************************/


/*************************************************************/
void FatTreeTopology::BuildInitialTopology(void) {
	NS_LOG_FUNCTION(this);
	int k = simulationRunProperties::k;
	NS_LOG_UNCOND(
			"UDP App [k=" << simulationRunProperties::k
			<< "] SawTooth Model [" << (simulationRunProperties::enableSawToothModel ? "true":"false")
			<< "] Markov Model [" << (simulationRunProperties::enableMarkovModel ? "true":"false")
			<< "] Custom Routing [" << (simulationRunProperties::enableCustomRouting ? "true":"false")
			<< "] VMC [" << (simulationRunProperties::enableVMC ? "true":"false") << "]");
	NS_ASSERT_MSG(k >= 3,
			"k should be greater than or equal to 3 to build fat-tree");
	NS_ASSERT_MSG(k <= 12,
			"Fat-tree not tested for k > 12, but it may still work!!!");
	/*
	 * The below value configures the default behavior of global routing.
	 * By default, it is disabled.  To respond to interface events, set to true
	 *  THIS HAS TO BE THE 1st LINE BEFORE SETTING THE NETWORK, NO DOCUMENTATION !
	 */
	Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting",
			BooleanValue(true)); // enable multi-path route
	Config::SetDefault("ns3::Ipv4GlobalRouting::RespondToInterfaceEvents",
			BooleanValue(true));
	//Config::SetDefault("ns3::DropTailQueue::MaxPackets", UintegerValue(10));
	/*
	 FROM WIKIPAGES:
	 https://en.wikipedia.org/wiki/Data_center_network_architectures
	 The network elements in fat tree topology also follows hierarchical organization of network switches in access,
	 aggregate, and core layers. However, the number of network switches is much larger than the three-tier DCN.
	 The architecture is composed of k pods, where each pod contains, (k/2)^2 servers, k/2 access layer switches, and
	 k/2 aggregate layer switches in the topology. The core layers contain (k/2)^2 core switches where each of the
	 core switches is connected to one aggregate layer switch in each of the pods
	 */
	int x, y, a, b, b1, c;
	//=========== Define parameters based on value of k ===========//
	int n_pods = k;
// these are PER POD nodes
		// number of hosts switch in a pod
	n_edge_routers = k / 2;	// number of bridge in a pod

	hosts_per_edge = (SSD_PER_RACK + 1) * (k/2); // number of hosts per edge router

	hosts_per_pod = hosts_per_edge * n_edge_routers;


	NS_LOG_UNCOND("hosts_per_edge "<<hosts_per_edge);
	NS_LOG_UNCOND("hosts_per_pod "<<hosts_per_pod);

	n_aggregate_routers = (k / 2);	// number of core switch in a group


	n_core_routers = (k/2) * (k/2);	// no of core switches
// these are TOTAL nodes....
	total_hosts = hosts_per_pod * k;	// number of hosts in the entire network
	total_aggregate_routers = k * n_aggregate_routers;
	total_edge_routers = k * n_edge_routers;
	total_core_routers = n_core_routers;

// Note: node creation order is important because we use allNodes NodeCollector later, top down construction...
	Ptr<ssNode> pNode;
	uint32_t count = 10000;
	//uint32_t count = 1000 ;
	double percentage_of_overall_bandwidth = (double)count * Ipv4GlobalRouting::threshold_percentage_for_dropping;
	uint32_t percentage_count = (uint32_t) percentage_of_overall_bandwidth ;
	for (a = 0; a < n_core_routers; a++) {
		pNode = CreateObject<ssNode>();
		pNode->setNodeType(ssNode::CORE);
		coreRouters.Add(pNode);
	}
	for (a = 0; a < total_aggregate_routers; a++) {
		pNode = CreateObject<ssNode>();
		pNode->setNodeType(ssNode::AGGR);
		aggrRouters.Add(pNode);
	}
	for (a = 0; a < total_edge_routers; a++) {
		pNode = CreateObject<ssNode>();
		pNode->setNodeType(ssNode::EDGE);
		edgeRouters.Add(pNode);
	}
	for (a = 0; a < total_hosts; a++) {
		pNode = CreateObject<ssNode>();
		pNode->setNodeType(ssNode::HOST);
		hosts.Add(pNode);
		BaseTopology::hosts_static.Add(pNode);
	}
	// NOW FOR LATER USE, GROUP ALL UNDER ONE COLLECTION.
	allNodes.Add(coreRouters);
	allNodes.Add(aggrRouters);
	allNodes.Add(edgeRouters);
	allNodes.Add(hosts);
	Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting",
			BooleanValue(true)); // enable multi-path route
// install i n/w stack
	InternetStackHelper iStack;
	iStack.Install(allNodes);

	ssPointToPointHelper p2p;
	ssPointToPointHelper p2pSSD;
	//p2p.SetQueue("ns3::DropTailQueue");
	//p2pSSD.SetQueue("ns3::DropTailQueue");

//	p2p.SetQueue ("ns3::DropTailQueue",
//	                                 "Mode", StringValue ("QUEUE_MODE_PACKETS"),
//	                                 "MaxPackets", UintegerValue (100));
//
//	p2pSSD.SetQueue ("ns3::DropTailQueue",
//	                                 "Mode", StringValue ("QUEUE_MODE_PACKETS"),
//	                                 "MaxPackets", UintegerValue (100));

	char *add;
	const char *ipv4Mask = "255.255.255.0";
	Ipv4AddressHelper addressHelper;
	Ipv4InterfaceContainer subnetIf1, subnetIf2, subnetIf3;
	NetDeviceContainer ndc1, ndc2, ndc3;

	//uint32_t node1, node2;

	// now create the FAT TREE topology with these layers and for "k" pods.
	// connect core routers to aggr routers..
	//
	p2p.SetDeviceAttribute("DataRate",
			StringValue(simulationRunProperties::coreDeviceDataRate));

	// bug sanjeev 2/20/17
	x = n_aggregate_routers; //total_aggregate_routers / n_pods; // aggr_routers_per_pod
	c = total_core_routers / x;
	for (b = 0; b < total_core_routers; b++) {
		for (a = 0; a < n_pods; a++) {
			//NS_LOG_UNCOND(percentage_count);
			y = (b / c + (x * a));
			y = y % total_aggregate_routers;
			add = getNewIpv4Address();
			NS_LOG_LOGIC(
					x << " Connect core [" << b << "] to aggr [" << y << "] ipv4 [" << add << "] linkDataRate [" << simulationRunProperties::coreDeviceDataRate << "]");
			ndc1 = p2p.Install(coreRouters.Get(b), aggrRouters.Get(y));
			addressHelper.SetBase(add, ipv4Mask);
			subnetIf1 = addressHelper.Assign(ndc1);
			SetUpInitialBandwidthMetric(subnetIf1.GetAddress(0),
					subnetIf1.GetAddress(1), percentage_count, coreRouters.Get(b)->GetId(),
					aggrRouters.Get(y)->GetId());
		}   // end pods
	}		// next level of core routers
	//count = count / 2;
	percentage_of_overall_bandwidth = (double)count * Ipv4GlobalRouting::threshold_percentage_for_dropping;
	percentage_count = (uint32_t) percentage_of_overall_bandwidth ;
	//
	// connect aggr routers to edge routers
	//
	p2p.SetDeviceAttribute("DataRate",
			StringValue(simulationRunProperties::aggrDeviceDataRate));
	for (a = 0; a < n_pods; a++) {
		for (b = 0; b < n_aggregate_routers; b++) {
			x = (n_aggregate_routers * a) + b;
			for (c = 0; c < n_edge_routers; c++) {
				//NS_LOG_UNCOND(percentage_count);
				y = (n_edge_routers * a) + c;
				y = (x < total_edge_routers ? y : y / total_edge_routers);
				ndc2 = p2p.Install(aggrRouters.Get(x), edgeRouters.Get(y));
				add = getNewIpv4Address();
				NS_LOG_LOGIC(
						"Connect aggr [" << x << "] to edge [" << y << "] ipv4 [" << add << "] linkDataRate [" << simulationRunProperties::aggrDeviceDataRate << "]");
				addressHelper.SetBase(add, ipv4Mask);
				subnetIf2 = addressHelper.Assign(ndc2);
				SetUpInitialBandwidthMetric(subnetIf2.GetAddress(0),
						subnetIf2.GetAddress(1), percentage_count,
						aggrRouters.Get(x)->GetId(),
						edgeRouters.Get(y)->GetId());
			}
		}
	}

	//
	// connect edge routers to hosts in this pod
	//
	//count = count / 2;
	p2p.SetDeviceAttribute("DataRate",StringValue(simulationRunProperties::newDevceRate));
	percentage_of_overall_bandwidth = (double)count * Ipv4GlobalRouting::threshold_percentage_for_dropping;
	percentage_count = (uint32_t) percentage_of_overall_bandwidth ;
//	p2p.SetDeviceAttribute("DataRate",
//			StringValue(simulationRunProperties::edgeDeviceDataRate));
	p2pSSD.SetDeviceAttribute("DataRate", StringValue(simulationRunProperties::edgeDeviceDataRate));

	for (a = 0, b1 = 0; a < n_pods; a++) {
		for (b = 0; b < n_edge_routers; b++, b1++) {
			x = (n_edge_routers * a) + b;
			for (c = 0; c < hosts_per_edge; c++) {
				//NS_LOG_UNCOND(percentage_count);
				y = (hosts_per_edge * b1) + c;

				NS_LOG_UNCOND("The value of Y "<<y<<" id "<<hosts.Get(y)->GetId());

				if(y% (SSD_PER_RACK+1) == 0) ndc3 = p2p.Install(edgeRouters.Get(x), hosts.Get(y));
				else ndc3 = p2pSSD.Install(edgeRouters.Get(x), hosts.Get(y));

				add = getNewIpv4Address();
				NS_LOG_LOGIC(
						"Connect edge [" << x << "] to host [" << y << "] ipv4 [" << add << "] linkDataRate [" << simulationRunProperties::edgeDeviceDataRate << "]");
				addressHelper.SetBase(add, ipv4Mask);
				subnetIf3 = addressHelper.Assign(ndc3);
				SetUpInitialBandwidthMetric(subnetIf3.GetAddress(0),
						subnetIf3.GetAddress(1), percentage_count,
						edgeRouters.Get(x)->GetId(), hosts.Get(y)->GetId());

				BaseTopology::hostTranslation[y] = subnetIf3.GetAddress(1);
			}
		}
	}
#if COMPILE_CUSTOM_ROUTING_CODE
	///////Added By Joyanta on 17th March to fix the k value of fat tree Ipv4GlobalRouting::FatTree_k
#endif
	///////////////////////////////////////
	// always populate node BW data...
	for (a = 0; a < total_hosts; a++) {
		DynamicCast<ssNode>(hosts.Get(a))->InitializeNode();
		//NS_LOG_UNCOND("The host id "<<hosts.Get(a)->GetId());
	}
	for (a = 0; a < total_aggregate_routers; a++) {
		DynamicCast<ssNode>(aggrRouters.Get(a))->InitializeNode();
	}
	for (a = 0; a < total_edge_routers; a++) {
		DynamicCast<ssNode>(edgeRouters.Get(a))->InitializeNode();
	}
	for (a = 0; a < total_core_routers; a++) {
		DynamicCast<ssNode>(coreRouters.Get(a))->InitializeNode();
	}

	for (uint32_t a1= 0; a1 < allNodes.GetN(); a1++) {
			DynamicCast<ssNode>(allNodes.Get(a1))->InitializeNode();
	}

	Ipv4GlobalRouting::system_constant[0] = 0.0;
	Ipv4GlobalRouting::system_constant[1] = 1.0;
	Ipv4GlobalRouting::system_constant[2] = 0.62;
	Ipv4GlobalRouting::system_constant[3] = 0.544;
	Ipv4GlobalRouting::system_constant[4] = 0.52;
	Ipv4GlobalRouting::system_constant[5] = 0.51;
	Ipv4GlobalRouting::system_constant[6] = 0.5;

	for(int i=7;i<30;i++)
	{
		Ipv4GlobalRouting::system_constant[i] = 0.5;
	}

	ns3::BaseTopology::total_appication = 0;

	for(uint32_t i=0;i<(uint32_t)total_hosts;i++)
	{
		BaseTopology::hostaddresslogicaltophysical[i] = hosts.Get(i)->GetId();
	}

	NS_LOG_UNCOND("total_hosts "<<total_hosts);
//
	//exit(0);

}

/*****************************************************/
void FatTreeTopology::SetNodeAnimPosition() {
	NS_LOG_FUNCTION(this);
	int i;
//AnimationInterface::userBoundary
// now start labeling for Animation FAT tree from topmost core routers...
	for (i = 0; i < total_core_routers; i++) {
		// connect the topmost level core_routers with next level aggregators_routers...
		AnimationInterface::SetConstantPosition(coreRouters.Get(i),
				2 * (i + 1.0), 0.0);
	}
	double j = (total_hosts / 4 > 0 ? total_hosts / 4 : 0.5);
	for (i = 0; i < total_aggregate_routers; i++) {
		// connect the topmost level core_routers with next level aggregators_routers...
		AnimationInterface::SetConstantPosition(aggrRouters.Get(i),
				i + (i / 2) + 2.0, j);
	}
	for (i = 0; i < total_edge_routers; i++) {
		// connect the topmost level core_routers with next level aggregators_routers...
		AnimationInterface::SetConstantPosition(edgeRouters.Get(i),
				i + (i / 2) + 2.0, total_hosts / 2);
	}
	for (i = 0; i < total_hosts; i++) {
		// connect the topmost level core_routers with next level aggregators_routers...
		AnimationInterface::SetConstantPosition(hosts.Get(i), i + 1.0,
				total_hosts);
	}
}

/*****************************************************/

} // namespace
