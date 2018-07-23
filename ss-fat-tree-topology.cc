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
	SetUpInitialChunkPosition();

	SetUpNodeUtilizationStatistics();

	SetUpInitialApplicationPosition();

	SetUpApplicationAssignment();

	SetUpIntensityPhraseChangeVariables();

	SetUpRealTracesVariables();

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
	for(uint32_t index= 0 ; index <chunk_number; index++)
	{
		uint32_t logical_node_id = index%total_hosts;
		uint32_t host_id = hosts.Get(logical_node_id)->GetId();
		Ipv4Address corresponding_ip = GetIpv4Address(host_id);
		chunk_info c_info = chunk_info(index, corresponding_ip, host_id,logical_node_id, true);
		BaseTopology::chunkTracker.push_back(c_info);

		BaseTopology::virtual_to_absolute_mapper[index] = index;


		/*****************The struct set up ***********************/

//		uint32_t pod = (uint32_t) floor((double) logical_node_id/ (double) Ipv4GlobalRouting::FatTree_k);
//
//		uint32_t node = logical_node_id % Ipv4GlobalRouting::FatTree_k;
//
//		uint32_t chunk_index = BaseTopology::p[pod].nodes[node].total_chunks;
//
//		BaseTopology::p[pod].nodes[node].data[chunk_index].chunk_number = index;
//
//		BaseTopology::p[pod].nodes[node].total_chunks++;

//
		//NS_LOG_UNCOND(host_id<<" "<<corresponding_ip);
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
		if((int)strlen(str) > 1)
		{
			pch = strtok (str," ,;");
			uint32_t count = 0;
			uint32_t value = 0;

			uint32_t logical_host_number;
			uint32_t physical_host_number;

			uint32_t pod;
			uint32_t node;
			//printf("%s\n", pch);

			while (pch != NULL)
			{
				sscanf(pch,"%d",&value);
				if(count == 0)
				{
					logical_host_number = 2 * value + 1;
					physical_host_number = hosts.Get(logical_host_number)->GetId();
					pod = (uint32_t) floor((double) logical_host_number/ (double) Ipv4GlobalRouting::FatTree_k * 2);
					node = ((logical_host_number - 1)/2) % Ipv4GlobalRouting::FatTree_k;

					printf("-------The chunk node id %d-------------------\n", logical_host_number);
					//printf("The node id %d\n", value);
				}
				else
				{
					value = value -1;
					BaseTopology::chunkTracker.at(value).logical_node_id = logical_host_number;
					BaseTopology::chunkTracker.at(value).node_id = physical_host_number;

					BaseTopology::p[pod].nodes[node].data[count-1].chunk_number = value;
					//
					BaseTopology::p[pod].nodes[node].total_chunks++;

					//printf ("%d\n",value);

				}
				count++;
				//NS_LOG_UNCOND("The count is "<<count);
				pch = strtok (NULL, " ,.-");

			}
		}
		//printf("The line length %d\n", (int)strlen(str));

	}
	fclose(fp);

	ns3::BaseTopology::Popularity_Change_Random_Variable->SetAttribute("Min", DoubleValue(0));
	ns3::BaseTopology::Popularity_Change_Random_Variable->SetAttribute("Max", DoubleValue(simulationRunProperties::total_chunk-1));

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
}

void FatTreeTopology::SetUpInitialApplicationPosition()
{
	uint32_t total_applications = simulationRunProperties::total_applications;
//
	ns3::BaseTopology::application_selector->SetAttribute("Min", DoubleValue(0));
	ns3::BaseTopology::application_selector->SetAttribute("Max", DoubleValue(simulationRunProperties::total_applications-1));





	uint32_t total_hosts = hosts.GetN();

	for(uint32_t i = 0;i<total_hosts;i++)
	{
		ns3::BaseTopology::application_assignment_to_node[i] = new uint32_t[total_applications+1];
		ns3::BaseTopology::application_assignment_to_node[i][0] = 0;

	}
	FILE *fp;
	char str[MAXCHAR];
	const char* filename = "app.txt";

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
//			/printf("The string is %s\n", str);
			pch = strtok (str," ,;");

			while (pch != NULL)
			{
				sscanf(pch,"%d",&value);

				if(count == 0)
				{
					host = 2 * value;

					printf("The application node id %d\n", host);

					//printf("The node id %d\n", value);
				}
				else
				{
					value = value - 1;
					ns3::BaseTopology::application_assignment_to_node[host][0]++;
					ns3::BaseTopology::application_assignment_to_node[host][count] = value;
					//printf ("**%d\n",value);
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

	BaseTopology::phrase_change_intensity_value[1] = 3.5;

	BaseTopology::phrase_change_intensity_value[2] = 2.5;

	BaseTopology::phrase_change_intensity_value[3] = 1.0;




	BaseTopology::phrase_change_interval[0] = 80; //in ms

	BaseTopology::phrase_change_interval[1] = 130;

	BaseTopology::phrase_change_interval[2] = 500;

	BaseTopology::phrase_change_interval[3] = 100;





	//BaseTopology::phrase_change_intersity_value[0] =
}

void FatTreeTopology::SetUpNodeUtilizationStatistics()
{
	int total_hosts = hosts.GetN();

	Ipv4GlobalRouting::host_utilization = new double[total_hosts];

	for(int i=0;i<total_hosts;i++)
	{
		Ipv4GlobalRouting::host_utilization[i] = 0.0;
	}
}

void FatTreeTopology::SetUpInitialOpmizationVariables()
{
	if(BaseTopology::createflag==false)
	{
		BaseTopology::p= new Pod[Ipv4GlobalRouting::FatTree_k];
		uint32_t number_of_hosts = (uint32_t)(Ipv4GlobalRouting::FatTree_k * Ipv4GlobalRouting::FatTree_k * Ipv4GlobalRouting::FatTree_k)/ 4;
		uint32_t nodes_in_pod = number_of_hosts / Ipv4GlobalRouting::FatTree_k;

		for(uint32_t i=0;i<(uint32_t)Ipv4GlobalRouting::FatTree_k;i++)
		{

			BaseTopology::p[i].nodes = new Fat_tree_Node[nodes_in_pod];
			BaseTopology::p[i].pod_number = (int) i;
		}
		for(uint32_t i = 0; i<number_of_hosts; i++)
		{
			uint32_t pod = (uint32_t) floor((double) i/ (double) Ipv4GlobalRouting::FatTree_k);
			BaseTopology::p[pod].nodes[i%Ipv4GlobalRouting::FatTree_k].node_number = i;
			BaseTopology::p[pod].nodes[i%Ipv4GlobalRouting::FatTree_k].utilization=0.0;
			BaseTopology::p[pod].nodes[i%Ipv4GlobalRouting::FatTree_k].data = new Dchunk[simulationRunProperties::total_chunk];
			BaseTopology::p[pod].nodes[i%Ipv4GlobalRouting::FatTree_k].total_chunks = 0;
		}
		BaseTopology::createflag=true;
	}
}




void FatTreeTopology::SetUpRealTracesVariables()
{

	FILE *fp;

	double interarrival_time_scale = 1.0;

	char str[MAXCHAR];
	uint32_t count = 0;
	uint64_t max_offset, min_offset;

	double timestamp;
	double response_time;
	char io_type;
	int LUN;
	uint64_t offset;
	uint32_t size;
	double trace_starting_time;

	int total_entries = 0;

	double total_time = 0.0;

	max_offset = 0;

	min_offset = std::numeric_limits<uint64_t>::max();

	double trace_ending_time;

	uint64_t total_bandwidth_in_bytes = 0;





	fp = fopen("trace2.csv", "r");
	if (fp == NULL){
		printf("Could not open file %s","trace2.csv");
		return;
	}

	trace_starting_time = 0;

	trace_ending_time = 0;

	//double next_interval = 0.0;

	int bandwidth_during_interval = 0;

	count = 0;

	while (fgets(str, MAXCHAR, fp) != NULL)
	{
		total_entries++;
		sscanf(str,"%lf,%lf,%c,%d,%lu,%d",&timestamp,&response_time, &io_type,&LUN,&offset,&size);
		if((int)strlen(str) > 1)
		{
			if(count==0)
			{
				trace_starting_time = timestamp;
			}

			//cout<<total_bandwidth_in_bytes<<" "<<size<<" "<<count<<endl;

			total_bandwidth_in_bytes += size;

			bandwidth_during_interval +=size;

			if(offset >= max_offset) max_offset = offset;

			if(offset <= min_offset) min_offset = offset;

			trace_ending_time = timestamp;

		}

		count++;
	}

	total_time += (trace_ending_time -trace_starting_time);


	fclose(fp);

	BaseTopology::min_offset = min_offset;

	BaseTopology::chunk_size = (max_offset - min_offset)/ simulationRunProperties::total_chunk;


	uint32_t trace_entries_per_application = total_entries/simulationRunProperties::total_applications;



	NS_LOG_UNCOND("Total Seconds expired "<<total_time );

	NS_LOG_UNCOND("Total Data exchange in bytes "<<total_bandwidth_in_bytes);

	NS_LOG_UNCOND("Average bw usages "<<total_bandwidth_in_bytes/(total_time*1000000)<<" MBps");

	NS_LOG_UNCOND("Average bw usages "<<total_bandwidth_in_bytes * 8/(total_time*1000000)<<" Mbps");

	int total_hosts = hosts.GetN();

	//NS_LOG_UNCOND("The total hosts "<<total_hosts);

	total_hosts = total_hosts / 2;

	double calculated_utilization = ((total_bandwidth_in_bytes * 8)/(total_time*1000000)) * simulationRunProperties::initialFlowCount ;

	double desired_utilization = simulationRunProperties::utilization_value * DRIVE_CAPACITY * total_hosts;

	NS_LOG_UNCOND("Desired utilization "<<desired_utilization<<" calculated_utilization "<<calculated_utilization);

	interarrival_time_scale = desired_utilization / calculated_utilization;

	NS_LOG_UNCOND("interarrival_time_scale "<<interarrival_time_scale);

	NS_LOG_UNCOND("Processing traces for applications............");

	trace_starting_time = 0.0;

	trace_ending_time = 0.0;

	uint32_t app_id = 0;

	count = 0;

	FILE *fp_subtraces;

	//double next_interval = 0.0;
	uint32_t total_hosts_machines = (simulationRunProperties::k * simulationRunProperties::k * simulationRunProperties::k)/4;

	char splitted_filename[15];

	sprintf(splitted_filename, "%s_%d.csv", "application",app_id);

	fp = fopen("trace2.csv", "r");
		if (fp == NULL){
			printf("Could not open file %s","trace2.csv");
			return;
	}
	fp_subtraces = fopen(splitted_filename,"w");

	double relative_time_of_traces;

	while (fgets(str, MAXCHAR, fp) != NULL)
	{
		total_entries++;
		sscanf(str,"%lf,%lf,%c,%d,%lu,%d",&timestamp,&response_time, &io_type,&LUN,&offset,&size);
		if((int)strlen(str) > 1)
		{
			if(trace_starting_time == 0.0)
			{
				trace_starting_time = timestamp;
				if(count == 0)
				{
					relative_time_of_traces = trace_starting_time;
				}
			}


			trace_ending_time = timestamp;

			fprintf(fp_subtraces,"%lf,%lf,%c,%d,%lu,%d\n",(timestamp-relative_time_of_traces) / interarrival_time_scale,response_time, io_type,LUN,offset,size );

			relative_time_of_traces = timestamp;
			count++;

			if(count % trace_entries_per_application == 0) //End of Application Entry
			{

				for(uint32_t index=0;index<total_hosts_machines;index++)
				{
					trace_starting_time = 0.0;
				}
				app_id++;
				trace_starting_time = 0.0;

				relative_time_of_traces = timestamp;

				fclose(fp_subtraces);

				sprintf(splitted_filename, "%s_%d.csv", "application",app_id);

				fp_subtraces = fopen(splitted_filename,"w");
			}
		}


	}

	fclose(fp);

	fclose(fp_subtraces);
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
	hosts_per_pod = (2 *k * k) / 4;	// number of hosts switch in a pod
	n_edge_routers = k / 2;	// number of bridge in a pod
	n_aggregate_routers = (k / 2);	// number of core switch in a group
	hosts_per_edge = hosts_per_pod / n_edge_routers; // number of hosts per edge router

	n_core_routers = hosts_per_pod/2;	// no of core switches
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
	p2p.SetQueue("ns3::DropTailQueue");

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
	percentage_of_overall_bandwidth = (double)count * Ipv4GlobalRouting::threshold_percentage_for_dropping;
	percentage_count = (uint32_t) percentage_of_overall_bandwidth ;
	p2p.SetDeviceAttribute("DataRate",
			StringValue(simulationRunProperties::edgeDeviceDataRate));
	for (a = 0, b1 = 0; a < n_pods; a++) {
		for (b = 0; b < n_edge_routers; b++, b1++) {
			x = (n_edge_routers * a) + b;
			for (c = 0; c < hosts_per_edge; c++) {
				//NS_LOG_UNCOND(percentage_count);
				y = (hosts_per_edge * b1) + c;
				ndc3 = p2p.Install(edgeRouters.Get(x), hosts.Get(y));
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
