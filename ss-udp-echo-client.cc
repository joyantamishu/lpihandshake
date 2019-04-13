/*
 * ss-udp-echo-client.cc7
 *
 *  Created on: Sep 30, 2016
 *      Author: sanjeev
 */

#include "ss-udp-echo-client.h"
#include "ss-udp-echo-server.h"
#include "ss-log-file-flags.h"
#include <cmath>
#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/ipv4-global-routing.h"

#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ss-tos-point-to-point-netdevice.h"
#include "ss-base-topology-simulation-variables.h"
#include "ss-base-topology.h"
#include "ns3/ipv4-global-routing.h"
/*
 flow bandwidth -  80-240MBps / avg 160...
 e.g.100MBps with packetsize of 1500, 8 bits/bytegives me --> packets/sec, interpacketTime....XYZ
 exponentialDstr- Mean=XYZ, UpperBound=<don't care>
 */

namespace ns3 {
int ssUdpEchoClient::flows_dropped = 0;

#define HOST_NODE_START_POINT				20
#define NUMBER_OF_HOST_VARIANCE_SUPPORTED 	16

#define USE_GAMMA_DISTRIBUTION				false

// SS -> MR:: Add variance values here...dirty but it will do the trick for now.
double m_hostSpecificVariance_Values[] = {
		5.609,5.609, 5.609, 5.609,
		5.609, 5.609, 5.609, 5.609,
		5.609, 5.609, 5.609, 5.609,
		5.609, 5.609, 5.609, 5.609 };

NS_LOG_COMPONENT_DEFINE("ssUdpEchoClient");

NS_OBJECT_ENSURE_REGISTERED(ssUdpEchoClient);

ssUdpEchoClientHelper::ssUdpEchoClientHelper(Address address, uint16_t port) {
	NS_LOG_FUNCTION(this);
	m_factory.SetTypeId(ssUdpEchoClient::GetTypeId());
	SetAttribute("RemoteAddress", AddressValue(address));
	SetAttribute("RemotePort", UintegerValue(port));

}

ssUdpEchoClientHelper::ssUdpEchoClientHelper(Ipv4Address ip, uint16_t port) {
	NS_LOG_FUNCTION(this);
	m_factory.SetTypeId(ssUdpEchoClient::GetTypeId());
	SetAttribute("RemoteAddress", AddressValue(Address(ip)));
	SetAttribute("RemotePort", UintegerValue(port));

}

ssUdpEchoClientHelper::~ssUdpEchoClientHelper() {
	NS_LOG_FUNCTION(this);
}

void ssUdpEchoClientHelper::SetAttribute(std::string name,
		const AttributeValue &value) {
	m_factory.Set(name, value);
}

ApplicationContainer ssUdpEchoClientHelper::Install(Ptr<Node> node, bool single_destination_flow, uint32_t source_node, uint32_t app_id, uint32_t dest_node, uint32_t no_of_packets, bool read_flow, bool non_consistent_read_flow, uint32_t read_app_id) const {
	return ApplicationContainer(InstallPriv(node,single_destination_flow, source_node, app_id, dest_node, no_of_packets, read_flow, non_consistent_read_flow, read_app_id));
}

ApplicationContainer ssUdpEchoClientHelper::Install(NodeContainer c) const {
	Ptr<Application> m_server; //!< The last created server application
	ApplicationContainer apps;
	for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i) {
		Ptr<Node> node = *i;
		m_server = m_factory.Create<ssUdpEchoClient>();
		node->AddApplication(m_server);
		apps.Add(m_server);
	}
	return apps;
}

Ptr<Application> ssUdpEchoClientHelper::InstallPriv(Ptr<Node> node, bool single_destination_flow, uint32_t source_node, uint32_t app_id, uint32_t dest_node, uint32_t no_of_packets, bool read_flow, bool non_consistent_read_flow, uint32_t read_app_id) const {
	Ptr<ssUdpEchoClient> app = m_factory.Create<ssUdpEchoClient>();

	NS_LOG_UNCOND("The source node is "<<source_node);

	app->node_index = source_node;

	if(single_destination_flow || read_flow)
	{
		app->single_destination = dest_node;

		app->total_packets_to_send = no_of_packets;
	}

	//NS_LOG_UNCOND("The random_application_id is "<<app_id);

	//NS_LOG_UNCOND("ns3::BaseTopology::application_assignment[source_node][0] "<<ns3::BaseTopology::application_assignment[source_node][0]);

	app->application_index = app_id;

	app->consistency_flow = single_destination_flow;

	if(single_destination_flow && non_consistent_read_flow) app->consistency_flow = !non_consistent_read_flow;

	app->read_flow = read_flow;

	app->read_flow_application_index = read_app_id;

	node->AddApplication(app);

	return app;
}
/***************************************************/

TypeId ssUdpEchoClient::GetTypeId(void) {
	static TypeId tid =
			TypeId("ns3::ssUdpEchoClient").SetParent<Application>().SetGroupName(
					"Applications").AddConstructor<ssUdpEchoClient>().AddAttribute(
					"Protocol", "The type of protocol to use.",
					TypeIdValue(UdpSocketFactory::GetTypeId()),
					MakeTypeIdAccessor(&ssUdpEchoClient::m_tid),
					MakeTypeIdChecker()).AddAttribute("RemoteAddress",
					"The destination Address of the outbound packets",
					AddressValue(),
					MakeAddressAccessor(&ssUdpEchoClient::m_peerAddress),
					MakeAddressChecker()).AddAttribute("RemoteHost",
					"The destination host of the outbound packets",
					UintegerValue(-99),
					MakeUintegerAccessor(&ssUdpEchoClient::m_dstHost),
					MakeUintegerChecker<uint16_t>()).AddAttribute("RemotePort",
					"The destination port of the outbound packets",
					UintegerValue(0),
					MakeUintegerAccessor(&ssUdpEchoClient::m_peerPort),
					MakeUintegerChecker<uint16_t>()).AddAttribute("PacketSize",
					"The total number of bytes per packet. ",
					UintegerValue(512),
					MakeUintegerAccessor(&ssUdpEchoClient::m_packetSize),
					MakeUintegerChecker<uint32_t>()).AddAttribute(
					"CurrentFlowNumber",
					"The current flow number (used by packetTag)",
					UintegerValue(0),
					MakeUintegerAccessor(&ssUdpEchoClient::m_currentFlowCount),
					MakeUintegerChecker<uint32_t>()).AddAttribute(
					"RequiredFlowBW",
					"Required Flow Bandwidth, set from external object",
					IntegerValue(80),
					MakeUintegerAccessor(&ssUdpEchoClient::m_flowRequiredBW),
					MakeUintegerChecker<uint32_t>()).AddAttribute(
					"RequiredReadFlowBW","Required Read Flow Bandwidth, set from external object",
					IntegerValue(80),MakeUintegerAccessor(&ssUdpEchoClient::m_readBandwidth),
					MakeUintegerChecker<uint32_t>()).AddAttribute(
							"FinishTime",
							"Finish Time of a Flow",
							DoubleValue(2),
							MakeDoubleAccessor(&ssUdpEchoClient::m_finish_time),
							MakeDoubleChecker<double_t>()).AddTraceSource("Tx",
					"A new packet is created and is sent",
					MakeTraceSourceAccessor(&ssUdpEchoClient::m_txTrace),
					"ns3::Packet::TracedCallback");
	return tid;
}

ssUdpEchoClient::ssUdpEchoClient() {
	NS_LOG_FUNCTION(this);
//	NS_ASSERT_MSG(
//			sizeof(m_hostSpecificVariance_Values)/sizeof(double) == NUMBER_OF_HOST_VARIANCE_SUPPORTED,
//			"m_hostSpecificVariance_Values initialization incorrect " << sizeof(m_hostSpecificVariance_Values)/sizeof(double));
	//m_socket = 0;       //!< Associated socket
	m_peerPort = 0; //!< Remote peer port
	t_p = 0;
	m_packetSize = 0;   //!< Limit total number of bytes sent in one packet
	m_flowRequiredBW = 80;
	m_priority = LOW;
	m_currentFlowCount = 0;
	t_sentPacketCount = 0;
	m_lastPacket = false;
	m_connected = false;
	m_dstHost = -1;
	m_flowStartTimeNanoSec = 0;
	consistency_flow = false;

	single_destination = -1;
	node_index = -1;
	application_index = -1;

	total_packets_to_send = 0;



	// initialization only, will be overridden at StartApplication()
	m_randomVariableInterPacketInterval = NULL;
	m_startNewFlow_CallbackSS.Nullify();
	total_hosts = (SSD_PER_RACK + 1) * (simulationRunProperties::k/2) * (simulationRunProperties::k/2) * simulationRunProperties::k;
	distinct_items = 0;



	//if(BaseTopology::chunk_copy_node_tracker[8][0]) NS_LOG_UNCOND("^^^^^Inside udpclient ^^^^^^^^^^^^^");
	for (std::vector<chunk_info>::iterator it = BaseTopology::chunkTracker.begin() ; it != BaseTopology::chunkTracker.end(); ++it)
	{
		uint32_t chunk_location = it->logical_node_id;

		//comment this section if you want to foward 100% traffic to the new copy
		if(it->number_of_copy >= 1)
		{
			int count = -1;
			int rand_val = BaseTopology::chunk_copy_selection->GetInteger() % (it->number_of_copy+1);

			//NS_LOG_UNCOND( " rand_val "<<rand_val);

			for(uint32_t index=0;index<total_hosts ; index++)
			{
				if(BaseTopology::chunk_copy_node_tracker[it->chunk_no][index]) count++;

				if(count == rand_val)
				{
					chunk_location = index;

					break;
				}
			}
		}
		//comment this section if you want to foward 100% traffic to the new copy

		local_chunkTracker.push_back(local_chunk_info(it->chunk_no, chunk_location, it->version_number, it->node_id));
	}


	//uint32_t total_racks = total_hosts/(SSD_PER_RACK +1 );

	sync_socket_tracker = new int[total_hosts];

	for(uint32_t i=0;i<total_hosts;i++)
	{
		sync_socket_tracker[i] = -1;
	}
	//m_socket = new Ptr<Socket>[total_hosts];


	sync_sockets = new Ptr<Socket>[BaseTopology::max_chunk_by_application];

	total_sync_sockets = 0;

	read_flow = true;

	no_packet_flow = false;

}

ssUdpEchoClient::~ssUdpEchoClient() {
	NS_LOG_FUNCTION(this);
// never called during active run, only called at end of simulation to destroy objects & close program
	NS_LOG_LOGIC(
			this << " " << m_srcIpv4Address << " <--> " << m_dstIpv4Address << " packets sent [" << t_sentPacketCount << "] in flow ["<< m_currentFlowCount << "]");
}

/*
 * Change this time interval if you want a different distribution of flow rate?
 */
void ssUdpEchoClient::SetRemote(Address ip, uint16_t port) {
	NS_LOG_FUNCTION(this << ip << port);
	m_peerAddress = ip;
	m_peerPort = port;
}

void ssUdpEchoClient::SetRemote(Ipv4Address ip, uint16_t port) {
	NS_LOG_FUNCTION(this << ip << port);
	m_peerAddress = Address(ip);
	m_peerPort = port;
}

void ssUdpEchoClient::SetRemote(Ipv6Address ip, uint16_t port) {
	NS_LOG_FUNCTION(this << ip << port);
	m_peerAddress = Address(ip);
	m_peerPort = port;
}

void ssUdpEchoClient::DoDispose(void) {
	NS_LOG_FUNCTION(this);
	Application::DoDispose();
}

// set all the call backs for this socket..
void ssUdpEchoClient::RegisterCallBackFunctions(void) {
	NS_LOG_FUNCTION(this);
	for(uint32_t i=0; i<distinct_items;i++)
	{
		m_socket[i]->SetRecvCallback(
				MakeCallback(&ssUdpEchoClient::callback_HandleRead, this));
		m_socket[i]->SetAllowBroadcast(true);
	}
}

void ssUdpEchoClient::ScheduleTransmit(Time dt) {
	NS_LOG_FUNCTION(this);
	/*
	 * you can overrule m_interval here, for varied inter-packet delay
	 */
	if (WRITE_PACKET_TIMING_TO_FILE)
		fp2 << dt.ToDouble(Time::MS) << "\n";
// setNextInterPacketInterval
	if (!consistency_flow)
	{
		m_packetInterval = Time::FromDouble(m_randomVariableInterPacketInterval->GetValue(), Time::S);

	}
	else
	{
		m_packetInterval = Time::FromDouble(CONSISTENCY_FLOW_START_DUARTION_CONSTANT, Time::S);
	}
	m_sendEvent = Simulator::Schedule(dt, &ssUdpEchoClient::Send, this);
}

uint32_t ssUdpEchoClient::FindChunkAssignedHost(uint32_t chunk_id, uint32_t rack_id)
{
	uint32_t host_starting = rack_id * (SSD_PER_RACK + 1) + 1;
	uint32_t i;
	for(i= 0;i<SSD_PER_RACK;i++)
	{
		if(BaseTopology::chunk_copy_node_tracker[chunk_id][host_starting+i])
		{
			return host_starting+i;
		}
	}
	NS_ASSERT_MSG(i<SSD_PER_RACK, " Unable to Track the Chunk ");

	return -1;
	//BaseTopology::chunkCopyLocations.push_back(MultipleCopyOfChunkInfo(chunk_id, location, version));
}

uint32_t ssUdpEchoClient::getHostInfoMadeBypolicy(uint32_t host_id)
{
	uint32_t rack_host_id = ((SSD_PER_RACK +1 ) * host_id) + 1 ;

	uint32_t min_host_utilization = Ipv4GlobalRouting::host_utilization[rack_host_id];

	uint32_t expected_host = rack_host_id;

	for(uint32_t i= 0; i<SSD_PER_RACK;i++)
	{
		if(Ipv4GlobalRouting::host_utilization[rack_host_id + i] < min_host_utilization)
		{
			expected_host = rack_host_id + i;
			min_host_utilization = Ipv4GlobalRouting::host_utilization[rack_host_id + i];
		}
	}

	NS_LOG_UNCOND("host_id "<<host_id<<" expected_host "<<expected_host);

	return expected_host;
}




//new function by Joyanta given on Oct 8 to fix sync packet

void ssUdpEchoClient::CreateandRemoveIndependentReadFlows(uint32_t distinct_hosts, double finish_time, int create)
{

	uint32_t version;


	int socket_index;

	uint32_t total_hosts_in_pod = (SSD_PER_RACK + 1) * (simulationRunProperties::k/2) * (simulationRunProperties::k/2);

	uint32_t destination  = GetNode()->GetId() - 20;

	for(uint32_t i=0;i<distinct_items;i++)
	{
		uint32_t ssd_host = socket_mapping.at(i).location;
		double sum_of_probability = 0.0;


		uint32_t pod = (uint32_t) floor((double) ssd_host/ (double) total_hosts_in_pod);

		uint32_t node = ((ssd_host -1)/(SSD_PER_RACK + 1)) % Ipv4GlobalRouting::FatTree_k;


		for(uint32_t chunk_no=1;chunk_no<=ns3::BaseTopology::chunk_assignment_to_applications[application_index][0];chunk_no++)
		{
			uint32_t chunk_location = getChunkLocation(destination_chunks[chunk_no - 1], &version, &socket_index);

			if(chunk_location == ssd_host) sum_of_probability += ns3::BaseTopology::chunk_assignment_probability_to_applications[application_index][chunk_no];
		}


		double bandwidth_distribution = sum_of_probability * (double)m_readBandwidth * (double)create;

		BaseTopology::p[pod].nodes[node].utilization_out += bandwidth_distribution;




	//	NS_LOG_UNCOND("bandwidth_distribution "<<bandwidth_distribution * (double) create);


		//Ipv4GlobalRouting::host_utilization[ssd_host] += bandwidth_distribution;

		BaseTopology::host_utilization_outgoing[ssd_host] += bandwidth_distribution;



		uint32_t count_chunk = 0;

		for(uint32_t chunk_no=1;chunk_no<=ns3::BaseTopology::chunk_assignment_to_applications[application_index][0];chunk_no++)
		{
			uint32_t chunk_location = getChunkLocation(destination_chunks[chunk_no - 1], &version, &socket_index);

			if(chunk_location == ssd_host)
			{
				count_chunk ++;
				double chunk_bandwidth_distribution = (ns3::BaseTopology::chunk_assignment_probability_to_applications[application_index][chunk_no]/ sum_of_probability) * bandwidth_distribution;

				//NS_LOG_UNCOND("chunk_assignment_probability_to_applications"<<ns3::BaseTopology::chunk_assignment_probability_to_applications[application_index][chunk_no]);

				//NS_LOG_UNCOND("sum_of_probability"<<sum_of_probability);

				//NS_LOG_UNCOND("chunk_bandwidth_distribution "<<chunk_bandwidth_distribution);

//				/NS_LOG_UNCOND("bandwidth_distribution "<<chunk_bandwidth_distribution);

				for(uint32_t chunk_index = 0 ;chunk_index < BaseTopology::p[pod].nodes[node].total_chunks;chunk_index++)
				{

					if(BaseTopology::p[pod].nodes[node].data[chunk_index].chunk_number == destination_chunks[chunk_no - 1])
					{
						//double current_simulation_time = Simulator::Now().ToDouble(Time::US);
						//if(create && floor(chunk_bandwidth_distribution) > 0)
						//{
							//uint32_t readcount=(chunk_bandwidth_distribution*1000*1000)/(simulationRunProperties::packetSize*8);
							uint32_t chunk=BaseTopology::p[pod].nodes[node].data[chunk_index].chunk_number;
							//NS_LOG_UNCOND("read count "<<readcount<<" chunk number "<<chunk);
							//BaseTopology::chnkCopy[chunk].readCount+=readcount;
						//}
					//	NS_LOG_UNCOND("chunk_number "<<chunk<<" create or delete "<<create<<"chunk_bandwidth_distribution-before---"<<chunk_bandwidth_distribution<<"BaseTopology::p[pod].nodes[node].data[chunk_index].intensity_sum_out "<<BaseTopology::p[pod].nodes[node].data[chunk_index].intensity_sum_out);
						BaseTopology::p[pod].nodes[node].data[chunk_index].intensity_sum_out += chunk_bandwidth_distribution;
					//	NS_LOG_UNCOND("chunk_number "<<chunk<<" after BaseTopology::p[pod].nodes[node].data[chunk_index].intensity_sum_out "<<BaseTopology::p[pod].nodes[node].data[chunk_index].intensity_sum_out);
						BaseTopology::p[pod].nodes[node].data[chunk_index].processed=0;

//////////////////////////////////////////////////////////////chunk usage calculation/////////////////////////////////////////////////
//						if(BaseTopology::chnkCopy[chunk].first_time_entered==0)
//							{
//								BaseTopology::chnkCopy[chunk].first_time_entered=current_simulation_time;
//								BaseTopology::chnkCopy[chunk].last_observed_utilization=BaseTopology::p[pod].nodes[node].data[chunk_index].intensity_sum_out;
//							}

						//double utilization=0.0,timelapse=0.0;
//						if(BaseTopology::p[pod].nodes[node].data[chunk_index].intensity_sum_out>BaseTopology::chnkCopy[chunk].last_observed_utilization)
//						{
						   //utilization=(BaseTopology::p[pod].nodes[node].data[chunk_index].intensity_sum_out-BaseTopology::chnkCopy[chunk].last_observed_utilization);
						  // timelapse=current_simulation_time-BaseTopology::chnkCopy[chunk].first_time_entered;
						  // utilization=float(utilization/timelapse);
						   BaseTopology::chnkCopy[chunk].runningAvg+=BaseTopology::p[pod].nodes[node].data[chunk_index].intensity_sum_out;
						// BaseTopology::chnkCopy[chunk].first_time_entered=current_simulation_time;
						 //BaseTopology::chnkCopy[chunk].last_observed_utilization=BaseTopology::p[pod].nodes[node].data[chunk_index].intensity_sum_out;
						   BaseTopology::chnkCopy[chunk].freq++;
						   if(BaseTopology::chnkCopy[chunk].max_instant_utilization<BaseTopology::p[pod].nodes[node].data[chunk_index].intensity_sum_out)
							   BaseTopology::chnkCopy[chunk].max_instant_utilization=BaseTopology::p[pod].nodes[node].data[chunk_index].intensity_sum_out;
//						}
/////////////////////////////////////////////////////////////chunk usage calculation/////////////////////////////////////////////////
						break;
					}

				}
			}
		}
		if (floor(bandwidth_distribution) > 0 && create == 1 && count_chunk>=1) BaseTopology::InjectANewRandomFlowCopyCreation(ssd_host, destination, 0, true, bandwidth_distribution, true, finish_time, application_index);
	}

   //calculating aggregate metric at the pod level
    uint32_t number_of_hosts = (uint32_t)(Ipv4GlobalRouting::FatTree_k * Ipv4GlobalRouting::FatTree_k * Ipv4GlobalRouting::FatTree_k)/ 4;
    uint32_t nodes_in_pod = number_of_hosts / Ipv4GlobalRouting::FatTree_k;
    for (int i=0;i<Ipv4GlobalRouting::FatTree_k;i++)
    {
        BaseTopology::p[i].Pod_utilization_out=0;
         for(uint32_t j=0;j<nodes_in_pod;j++)
          {
             BaseTopology::p[i].Pod_utilization_out=BaseTopology::p[i].Pod_utilization_out+BaseTopology::p[i].nodes[j].utilization_out;
          }
    }

}

void ssUdpEchoClient::StartApplication() {
    NS_LOG_FUNCTION(this);

    //NS_LOG_UNCOND("Here I am ");
    uint32_t total_hosts_in_system = (SSD_PER_RACK + 1) * (simulationRunProperties::k/2) * (simulationRunProperties::k/2) * simulationRunProperties::k;
    float alpha=.75;

    uint32_t total_hosts_in_pod = (SSD_PER_RACK + 1) * (simulationRunProperties::k/2) * (simulationRunProperties::k/2);

    uint32_t version;


    int socket_index;

//    NS_LOG_UNCOND("The bandwidth original "<<m_flowRequiredBW);

  //  NS_LOG_UNCOND("The read bandwidth "<<m_readBandwidth);

    //NS_LOG_UNCOND("Finish Time "<<m_finish_time);

    if(this->consistency_flow)
    {
     //   NS_LOG_UNCOND("This is a consistency flow");

        this->distinct_items = 1;

        m_socket = new Ptr<Socket>[this->distinct_items];

        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        m_socket[0] = Socket::CreateSocket(GetNode(), tid);

        if (Ipv4Address::IsMatchingType(m_peerAddress) == true) {
            m_socket[0]->Bind();
            m_socket[0]->Connect(InetSocketAddress(BaseTopology::hostTranslation[single_destination], m_peerPort));//JB change Dec 12
        }

        m_socket[0]->SetRecvCallback(MakeCallback(&ssUdpEchoClient::callback_HandleRead, this));
        m_socket[0]->SetAllowBroadcast(true);

        ScheduleTransmit(Time::FromDouble(CONSISTENCY_FLOW_START_DUARTION_CONSTANT, Time::S));

        return;
    }
    else
    {
    	if(read_flow)
    	{
    	//	NS_LOG_UNCOND("This is a read flow");
    		/*****Set Up independent read flow chunk access probability ***********/


    		uint32_t ssd_host = GetNode()->GetId() - 20;

    		uint32_t count_host_chunk = 0;

    		double sum_of_probability = 0.0;

    		for(uint32_t chunk_no=1;chunk_no<=ns3::BaseTopology::chunk_assignment_to_applications[read_flow_application_index][0];chunk_no++)
			{
				uint32_t chunk_location = getChunkLocation(BaseTopology::chunk_assignment_to_applications[read_flow_application_index][chunk_no], &version, &socket_index);



				if(chunk_location == ssd_host)
				{
					count_host_chunk++;
					//NS_LOG_UNCOND(" Chunk "<<BaseTopology::chunk_assignment_to_applications[read_flow_application_index][chunk_no]<<" location "<<ssd_host<<" Application "<<read_flow_application_index<<" real location "<<BaseTopology::chunkTracker.at(BaseTopology::chunk_assignment_to_applications[read_flow_application_index][chunk_no]).logical_node_id);
					sum_of_probability += ns3::BaseTopology::chunk_assignment_probability_to_applications[read_flow_application_index][chunk_no];
				}
				else
				{
					if(BaseTopology::chunk_copy_node_tracker[BaseTopology::chunk_assignment_to_applications[read_flow_application_index][chunk_no]][ssd_host])
					{
						count_host_chunk++;
											//NS_LOG_UNCOND(" Chunk "<<BaseTopology::chunk_assignment_to_applications[read_flow_application_index][chunk_no]<<" location "<<ssd_host<<" Application "<<read_flow_application_index<<" real location "<<BaseTopology::chunkTracker.at(BaseTopology::chunk_assignment_to_applications[read_flow_application_index][chunk_no]).logical_node_id);
						sum_of_probability += ns3::BaseTopology::chunk_assignment_probability_to_applications[read_flow_application_index][chunk_no];
					}
				}
			}

    		if(count_host_chunk == 0)
    		{
    			NS_LOG_UNCOND("read_flow_application_index "<<read_flow_application_index);

    			NS_LOG_UNCOND(" ssd_host "<<ssd_host);
    			exit(0);
    		}

    		chunk_assignment_probability_for_read_flow = new double[count_host_chunk];

    		selected_chunk_for_read_flow = new uint32_t[count_host_chunk];

    		//NS_LOG_UNCOND(" sum_of_probability "<<sum_of_probability);
    		count_host_chunk = 0;
    		for(uint32_t chunk_no=1;chunk_no<=ns3::BaseTopology::chunk_assignment_to_applications[read_flow_application_index][0];chunk_no++)
    		{
    			uint32_t chunk_location = getChunkLocation(BaseTopology::chunk_assignment_to_applications[read_flow_application_index][chunk_no], &version, &socket_index);
    			if(chunk_location == ssd_host)
    			{
    				selected_chunk_for_read_flow[count_host_chunk] = BaseTopology::chunk_assignment_to_applications[read_flow_application_index][chunk_no];

    				chunk_assignment_probability_for_read_flow[count_host_chunk] = ns3::BaseTopology::chunk_assignment_probability_to_applications[read_flow_application_index][chunk_no]/sum_of_probability;

    				count_host_chunk ++;
    			}

    			else
    			{
    				if(BaseTopology::chunk_copy_node_tracker[BaseTopology::chunk_assignment_to_applications[read_flow_application_index][chunk_no]][ssd_host])
    				{
    					selected_chunk_for_read_flow[count_host_chunk] = BaseTopology::chunk_assignment_to_applications[read_flow_application_index][chunk_no];

						chunk_assignment_probability_for_read_flow[count_host_chunk] = ns3::BaseTopology::chunk_assignment_probability_to_applications[read_flow_application_index][chunk_no]/sum_of_probability;

						count_host_chunk ++;

    				}
    			}

    		}

    		selected_total_chunk = count_host_chunk;
    		/***********************************************************************/

    		//uint32_t read_src = GetNode()->GetId() - 20;




    		this->distinct_items = 1;
			m_socket = new Ptr<Socket>[1];
			TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
			m_socket[0] = Socket::CreateSocket(GetNode(), tid);

			if (Ipv4Address::IsMatchingType(m_peerAddress) == true) {
				m_socket[0]->Bind();
				m_socket[0]->Connect(InetSocketAddress(BaseTopology::hostTranslation[single_destination], m_peerPort));
			}

			m_socket[0]->SetRecvCallback(MakeCallback(&ssUdpEchoClient::callback_HandleRead, this));
			m_socket[0]->SetAllowBroadcast(true);
			setFlowVariables();
			RegisterCallBackFunctions();
			ScheduleTransmit(Time::FromDouble(m_randomVariableInterPacketInterval->GetValue(), Time::S));

			return;
    	}

    }


	//NS_LOG_UNCOND("this in neither consistency nor a read flow ");
    BaseTopology::total_appication++;

    BaseTopology::current_number_of_flows++;

    ChangeIntensity();

    destination_chunks = new uint32_t[ns3::BaseTopology::chunk_assignment_to_applications[application_index][0]];

    for(uint32_t chunk_no=1;chunk_no<=ns3::BaseTopology::chunk_assignment_to_applications[application_index][0];chunk_no++)
    {
        uint32_t virtual_chunk_number = ns3::BaseTopology::chunk_assignment_to_applications[application_index][chunk_no];
        destination_chunks[chunk_no-1] = ns3::BaseTopology::virtual_to_absolute_mapper[virtual_chunk_number];
    }
   // NS_LOG_UNCOND("here I am 2");

    for(uint32_t i=1 ; i<= ns3::BaseTopology::chunk_assignment_to_applications[application_index][0];i++)
    {
        uint32_t chunk_value = destination_chunks[i-1];

        uint32_t chunk_location = getChunkLocation(chunk_value, &version, &socket_index);

       // NS_LOG_UNCOND(" chunk_value "<<chunk_value<<" chunk_location "<<chunk_location);

        double bandwidth_distribution = ns3::BaseTopology::chunk_assignment_probability_to_applications[application_index][i] * (double)m_flowRequiredBW;

        Ipv4GlobalRouting::host_utilization[chunk_location] += bandwidth_distribution; //incase of write flow we have to add this

		//NS_LOG_UNCOND("bandwidth_distribution ---from within the start application-------------------------------"<<bandwidth_distribution);

        Ipv4GlobalRouting::host_utilization_smoothed[chunk_location]=(alpha*Ipv4GlobalRouting::host_utilization[chunk_location])+((1-alpha)*Ipv4GlobalRouting::host_utilization_smoothed[chunk_location]);

        //the following two lines are for statistics

        BaseTopology::host_running_avg_bandwidth[chunk_location] += Ipv4GlobalRouting::host_utilization[chunk_location];

        BaseTopology::host_running_avg_counter[chunk_location]++;

       	//set some flag for the host
        if ( Ipv4GlobalRouting::host_utilization_smoothed[chunk_location]>Count*.8) //parameter involved
        	Ipv4GlobalRouting::host_congestion_flag[chunk_location]= 1;



        uint32_t pod = (uint32_t) floor((double) chunk_location/ (double) total_hosts_in_pod);

        uint32_t node = ((chunk_location - 1)/(SSD_PER_RACK + 1)) % Ipv4GlobalRouting::FatTree_k;

       // uint32_t number_of_hosts = (uint32_t)(Ipv4GlobalRouting::FatTree_k * Ipv4GlobalRouting::FatTree_k * Ipv4GlobalRouting::FatTree_k)/ 4;
       // uint32_t nodes_in_pod = number_of_hosts / Ipv4GlobalRouting::FatTree_k;

        BaseTopology::p[pod].nodes[node].utilization += bandwidth_distribution;



        for(uint32_t chunk_index = 0 ;chunk_index < BaseTopology::p[pod].nodes[node].total_chunks;chunk_index++)
        {

            if(BaseTopology::p[pod].nodes[node].data[chunk_index].chunk_number == chunk_value)
            {
                BaseTopology::p[pod].nodes[node].data[chunk_index].intensity_sum += bandwidth_distribution;
                BaseTopology::p[pod].nodes[node].data[chunk_index].processed=0;
                break;
            }
        }
    }

  //  NS_LOG_UNCOND("here I am 22");

    for(uint32_t i=1 ; i<= ns3::BaseTopology::chunk_assignment_to_applications[application_index][0];i++)
    {

        uint32_t j;
        for (j=1; j<i; j++)
        {
            if (getChunkLocation(destination_chunks[i-1], &version, &socket_index) == getChunkLocation(destination_chunks[j-1], &version, &socket_index)) break;
        }
        if (i == j)
        {
            socket_mapping.push_back(MappingSocket(distinct_items, getChunkLocation(destination_chunks[i-1], &version, &socket_index)));
            distinct_items++;
        }

    }

    m_socket = new Ptr<Socket>[distinct_items+1];

    CreateandRemoveIndependentReadFlows(distinct_items, m_finish_time);

    for(uint32_t i=0;i<distinct_items;i++)
    {
        TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        m_socket[i] = Socket::CreateSocket(GetNode(), tid);

        if (Ipv4Address::IsMatchingType(m_peerAddress) == true) {
            m_socket[i]->Bind();
            m_socket[i]->Connect(InetSocketAddress(BaseTopology::hostTranslation[socket_mapping.at(i).location], m_peerPort));
        }
    }

    for(uint32_t i=1 ; i<= ns3::BaseTopology::chunk_assignment_to_applications[application_index][0];i++)
    {
        uint32_t chunk_location = local_chunkTracker.at(destination_chunks[i-1]).node_id;

        for (std::vector<MappingSocket>::iterator it = socket_mapping.begin() ; it != socket_mapping.end(); ++it)
        {
            if(it->location == chunk_location) local_chunkTracker.at(destination_chunks[i-1]).socket_index = it->socket_id;
        }
    }

   //NS_LOG_UNCOND("here I am with App id "<<this->application_index);




    uint32_t number_of_hosts = (uint32_t)(Ipv4GlobalRouting::FatTree_k * Ipv4GlobalRouting::FatTree_k * Ipv4GlobalRouting::FatTree_k)/ 4;
    uint32_t nodes_in_pod = number_of_hosts / Ipv4GlobalRouting::FatTree_k;
    for (int i=0;i<Ipv4GlobalRouting::FatTree_k;i++)
    {
        BaseTopology::p[i].Pod_utilization=0;
         for(uint32_t j=0;j<nodes_in_pod;j++)
          {
             BaseTopology::p[i].Pod_utilization=BaseTopology::p[i].Pod_utilization+BaseTopology::p[i].nodes[j].utilization;
          }
    }

    //dump the data into a csv
    BaseTopology::sleeping_nodes=0;

    //checking for the nodes that can go to sleep mode
	FILE *fp_sleep_tracking;

	fp_sleep_tracking = fopen ("server_level_sleep.csv","a");

//    for (uint32_t t=0;t<total_hosts_in_system;t++)
//    {
//    	if(t%(SSD_PER_RACK+1)!=0)
//    	{
//    	if(Ipv4GlobalRouting::host_utilization[t]==0)
//    		BaseTopology::sleeping_nodes++;
//    	}
//    }
   // NS_LOG_UNCOND("sleep count"<<BaseTopology::sleeping_nodes);
    //fprintf(fp_sleep_tracking,"%d,\n",BaseTopology::sleeping_nodes);
    fclose(fp_sleep_tracking);

	FILE *fp_host_utilization;

	double time_now=Simulator::Now().ToDouble(Time::MS);
	fp_host_utilization = fopen ("host_utilization_madhurima.csv","a");
	fprintf(fp_host_utilization,"%f,",time_now);
	for (uint32_t t=0;t<total_hosts_in_system;t++)
	{
		//fprintf(fp_host_utilization,"%f,,",Ipv4GlobalRouting::host_utilization[t]);
		//fprintf(fp_host_utilization,"%f,,",Ipv4GlobalRouting::host_utilization_smoothed[t]);
		fprintf(fp_host_utilization,"%f,",BaseTopology::host_utilization_outgoing[t]);


	}
	fprintf(fp_host_utilization,"\n");
	fclose(fp_host_utilization);

    /********Uncomment it when function ReturnSomething is ready */
if( simulationRunProperties::enableOptimizer)
{
	BaseTopology::Incrcounter_++;
    if(BaseTopology::Incrcounter_%5==0)
    {
        //BaseTopology::Incrcounter_=0;
        int incrDcr=1;

        BaseTopology::calculateNewLocation(incrDcr);

        int i=0;

        //dump the data into a csv

        FILE *fp_copy;

        fp_copy = fopen ("copy_create_delete.csv","a");


        while(BaseTopology::res[i].src != 99999)// && BaseTopology::res!=NULL)
        {
            printf("++++++++++++++++++++++++++++\n");
            NS_LOG_UNCOND(BaseTopology::res[i].chunk_number);




            uint32_t num_of_packets_to_send = BaseTopology::chunk_version_tracker[BaseTopology::res[i].chunk_number] - BaseTopology::chunk_version_node_tracker[BaseTopology::res[i].chunk_number][BaseTopology::res[i].dest];
            BaseTopology::chunkTracker.at(BaseTopology::res[i].chunk_number).number_of_copy++;
            BaseTopology::copy_created++;
            char p='c';
             // NS_LOG_UNCOND("BaseTopology::chunk_version_tracker[BaseTopology::res[i].chunk_number])"<<BaseTopology::chunk_version_tracker[BaseTopology::res[i].chunk_number]);
           // NS_LOG_UNCOND("BaseTopology::chunk_version_node_tracker[BaseTopology::res[i].chunk_number][BaseTopology::res[i].dest]"<<BaseTopology::chunk_version_node_tracker[BaseTopology::res[i].chunk_number][BaseTopology::res[i].dest]);
            //NS_LOG_UNCOND("num_of_packets_to_send"<<num_of_packets_to_send);
            //commenting off this following line will stop the copy creation
            //if(num_of_packets_to_send > 0) BaseTopology::InjectANewRandomFlowCopyCreation (BaseTopology::res[i].src, BaseTopology::res[i].dest, num_of_packets_to_send);

//this part of the code places the chunk on the any server in a rack in a round robin fashion
           // BaseTopology::host_assignment_round_robin_counter[BaseTopology::res[i].dest]++;

//            BaseTopology::chunkTracker.at(BaseTopology::res[i].chunk_number).logical_node_id =(SSD_PER_RACK + 1) * BaseTopology::res[i].dest + 1 + (BaseTopology::host_assignment_round_robin_counter[BaseTopology::res[i].dest]%SSD_PER_RACK);


//this part of the code places the chunk on the least utilized server in a rack

//            uint32_t start = (BaseTopology::res[i].dest * ( SSD_PER_RACK + 1)) + 1;
//            double minimim_utilization=99999;
//            uint32_t min_node;
//            for (uint32_t t=start;t<(start+SSD_PER_RACK);t++)
//			{
//            	if(BaseTopology::host_utilization_outgoing[t]<minimim_utilization && !BaseTopology::chunk_copy_node_tracker[BaseTopology::res[i].chunk_number][t]) //make sure no duplicate assignment while assigning
//            		{
//            			//minimim_utilization=Ipv4GlobalRouting::host_utilization[t];
//            		    minimim_utilization=BaseTopology::host_utilization_outgoing[t];
//            			min_node=t;
//            		}
//
//			}
//            NS_LOG_UNCOND("minimum node with least utilization inside the rack is :"<<min_node);
//            BaseTopology::chunkTracker.at(BaseTopology::res[i].chunk_number).logical_node_id =min_node;
//
//            if(num_of_packets_to_send > 0)
//            {
//                uint32_t source = BaseTopology::res[i].src * (SSD_PER_RACK + 1);
//                uint32_t destination = BaseTopology::chunkTracker.at(BaseTopology::res[i].chunk_number).logical_node_id;
//              //  NS_LOG_UNCOND("num_of_packets_to_send"<<num_of_packets_to_send);
//#if CHUNKSIZE
//                if(num_of_packets_to_send>(simulationRunProperties::chunkSize/simulationRunProperties::packetSize))
//                	num_of_packets_to_send=ceil(simulationRunProperties::chunkSize/simulationRunProperties::packetSize);
//#else
//				num_of_packets_to_send=log(num_of_packets_to_send);
//#endif
//
//				BaseTopology::InjectANewRandomFlowCopyCreation (source, destination, num_of_packets_to_send);
//				BaseTopology::total_number_of_packet_for_copy_creation+=num_of_packets_to_send;


				uint32_t start = (BaseTopology::res[i].dest * ( SSD_PER_RACK + 1)) + 1;
				double minimim_utilization=99999;
				uint32_t min_node;
				for (uint32_t t=start;t<(start+SSD_PER_RACK);t++)
				{
					if(BaseTopology::host_utilization_outgoing[t]<minimim_utilization && !BaseTopology::chunk_copy_node_tracker[BaseTopology::res[i].chunk_number][t] && BaseTopology::host_copy[t]==0) //make sure no duplicate assignment while assigning
						{
							//minimim_utilization=Ipv4GlobalRouting::host_utilization[t];
							minimim_utilization=BaseTopology::host_utilization_outgoing[t];
							min_node=t;
						}

				}
				BaseTopology::host_copy[min_node]=1;
				NS_LOG_UNCOND("minimum node with least utilization inside the rack is :"<<min_node);
				BaseTopology::chunkTracker.at(BaseTopology::res[i].chunk_number).logical_node_id =min_node;
				fprintf(fp_copy,"%c,%f,%d,%d,%d,%d,%d\n",p,Simulator::Now().ToDouble(Time::MS),BaseTopology::res[i].chunk_number,BaseTopology::res[i].src,BaseTopology::res[i].dest,num_of_packets_to_send,min_node);

				uint32_t destination = min_node;
				min_node=9999999;
				start = (BaseTopology::res[i].src * ( SSD_PER_RACK + 1)) + 1;

				for (uint32_t t=start;t<(start+SSD_PER_RACK);t++)
				{
					if(BaseTopology::chunk_copy_node_tracker[BaseTopology::res[i].chunk_number][t]) //make sure no duplicate assignment while assigning
					{		min_node=t;
							break;
					}
				}

				uint32_t source = min_node;




				if(num_of_packets_to_send > 0)
				{

				//  NS_LOG_UNCOND("num_of_packets_to_send"<<num_of_packets_to_send);
				if(CHUNKSIZE==true)
				{
					if(num_of_packets_to_send>ceil(float(simulationRunProperties::chunkSize)/simulationRunProperties::packetSize))
						{
							num_of_packets_to_send=ceil(simulationRunProperties::chunkSize/simulationRunProperties::packetSize);
							NS_LOG_UNCOND("num_of_packets_to_send"<<num_of_packets_to_send);
						}
				}
				else
					num_of_packets_to_send=log(num_of_packets_to_send);

				NS_LOG_UNCOND("Destination "<<BaseTopology::res[i].dest<<" Server "<<destination<<" Source "<<BaseTopology::res[i].src<<"Server "<< source <<" num_of_packets_to_send "<<num_of_packets_to_send);

				BaseTopology::InjectANewRandomFlowCopyCreation (source, destination, num_of_packets_to_send);
				BaseTopology::total_number_of_packet_for_copy_creation+=num_of_packets_to_send;


                BaseTopology::chunk_version_node_tracker[BaseTopology::res[i].chunk_number][destination] = BaseTopology::chunk_version_tracker[BaseTopology::res[i].chunk_number];
                NS_LOG_UNCOND("write count of chunk number "<<BaseTopology::res[i].chunk_number<<" is "<<BaseTopology::chnkCopy[BaseTopology::res[i].chunk_number].writeCount<<" read count "<<BaseTopology::chnkCopy[BaseTopology::res[i].chunk_number].readCount);
               // NS_LOG_UNCOND("BaseTopology::chunk_version_node_tracker[BaseTopology::res[i].chunk_number][destination]"<< BaseTopology::chunk_version_node_tracker[BaseTopology::res[i].chunk_number][destination]);
               // NS_LOG_UNCOND("BaseTopology::chunk_version_tracker[BaseTopology::res[i].chunk_number"<<BaseTopology::chunk_version_tracker[BaseTopology::res[i].chunk_number]);

            }



            BaseTopology::chunk_copy_node_tracker[BaseTopology::res[i].chunk_number][BaseTopology::chunkTracker.at(BaseTopology::res[i].chunk_number).logical_node_id] = true;

            //BaseTopology::chunkTracker.at(BaseTopology::res[i].chunk_number).logical_node_id = getHostInfoMadeBypolicy(BaseTopology::res[i].dest);

            NS_LOG_UNCOND(" BaseTopology::chunkTracker.at(BaseTopology::res[i].chunk_number).logical_node_id "<<BaseTopology::chunkTracker.at(BaseTopology::res[i].chunk_number).logical_node_id);

            NS_LOG_UNCOND("The chunk address is "<<BaseTopology::hostaddresslogicaltophysical[BaseTopology::chunkTracker.at(BaseTopology::res[i].chunk_number).logical_node_id]<<" BaseTopology::res[i].dest "<<BaseTopology::res[i].dest);


            uint32_t pod = (uint32_t) floor((double) BaseTopology::res[i].dest/ (double) Ipv4GlobalRouting::FatTree_k);

            uint32_t node = BaseTopology::res[i].dest % Ipv4GlobalRouting::FatTree_k;

            NS_LOG_UNCOND("BaseTopology::res[i].dest "<<BaseTopology::res[i].dest<<" Pod "<<pod<<" Ipv4GlobalRouting::FatTree_k "<<Ipv4GlobalRouting::FatTree_k);

            bool entry_already_exists = false;
            uint32_t location;

            for(uint32_t chunk_index = 0 ;chunk_index < BaseTopology::p[pod].nodes[node].total_chunks;chunk_index++)
            {
                if(BaseTopology::p[pod].nodes[node].data[chunk_index].chunk_number == BaseTopology::res[i].chunk_number)
                {
					entry_already_exists = true;
					location=chunk_index;
					break;
                }

            }

            if(!entry_already_exists)
            {
                BaseTopology::p[pod].nodes[node].data[BaseTopology::p[pod].nodes[node].total_chunks].chunk_number = BaseTopology::res[i].chunk_number;
                BaseTopology::p[pod].nodes[node].data[BaseTopology::p[pod].nodes[node].total_chunks].intensity_sum = 0.0;
                BaseTopology::p[pod].nodes[node].data[BaseTopology::p[pod].nodes[node].total_chunks].chunk_count = 1;
                BaseTopology::p[pod].nodes[node].total_chunks++;
            }
            else
            {
            	NS_LOG_UNCOND("herevxcvksdlflsdfjsd1");
            	BaseTopology::p[pod].nodes[node].data[location].chunk_count++;
            	NS_LOG_UNCOND("herevxcvksdlflsdfjsd12");
				BaseTopology::p[pod].nodes[node].data[BaseTopology::p[pod].nodes[node].total_chunks].chunk_count = 1;
				NS_LOG_UNCOND("herevxcvksdlflsdfjsd123");
				BaseTopology::p[pod].nodes[node].data[BaseTopology::p[pod].nodes[node].total_chunks].processed=0;
				NS_LOG_UNCOND("herevxcvksdlflsdfjsd1234");
				BaseTopology::p[pod].nodes[node].data[BaseTopology::p[pod].nodes[node].total_chunks].chunk_number = BaseTopology::res[i].chunk_number;
				NS_LOG_UNCOND("herevxcvksdlflsdfjsd12345");
				BaseTopology::p[pod].nodes[node].total_chunks++;
				NS_LOG_UNCOND("herevxcvksdlflsdfjsd123456");

            }

            NS_LOG_UNCOND("src "<<BaseTopology::res[i].src<<" dest "<<BaseTopology::res[i].dest<<" chunk_no "<<BaseTopology::res[i].chunk_number);

            printf("%d %d %d\n", BaseTopology::res[i].src,BaseTopology::res[i].dest,BaseTopology::res[i].chunk_number);
            i++;
        }
        fclose(fp_copy);
    }
    //calling the optimizer
}

    /*
     * Set inter_flow interval & inter-packet interval to some random distribution...
     */
    setFlowVariables();
    NS_LOG_LOGIC(
            this << " node " << GetNode()->GetId() << " " << m_srcIpv4Address << " <--> " << m_dstIpv4Address << " t_currentFlowCount::"<< m_currentFlowCount);
    NS_LOG_LOGIC(
            this << " requiredBW " << m_flowRequiredBW << " flowPriority " << m_priority << " interpacketInterval::type " << m_randomVariableInterPacketInterval->GetTypeId());


    RegisterCallBackFunctions();
// 1st packet itself is staggered between diffflows...
    ScheduleTransmit(
            Time::FromDouble(m_randomVariableInterPacketInterval->GetValue(),
                    Time::S));

}



void ssUdpEchoClient::ForceStopApplication(void) {
	NS_LOG_FUNCTION(this);
}

void ssUdpEchoClient::StopApplication(void) {
	NS_LOG_FUNCTION(this);

	uint32_t version;

	float alpha=.75;
	int socket_index;
	uint32_t number_of_hosts = (uint32_t)(Ipv4GlobalRouting::FatTree_k * Ipv4GlobalRouting::FatTree_k * Ipv4GlobalRouting::FatTree_k)/ 4;
    //uint32_t total_hosts_in_system = (SSD_PER_RACK + 1) * (simulationRunProperties::k/2) * (simulationRunProperties::k/2) * simulationRunProperties::k;


	NS_LOG_UNCOND("Here The stop app ");

	if(!consistency_flow && !read_flow) BaseTopology::current_number_of_flows--;

	NS_LOG_UNCOND("^^^^^^^^^^^^^Current number of flows^^^^^^^^^^^^^^^"<< BaseTopology::current_number_of_flows);

	uint32_t total_hosts_in_pod = (SSD_PER_RACK + 1) * (simulationRunProperties::k/2) * (simulationRunProperties::k/2);

// as discussed on Mar 15. Sanjeev (overcome bug)
	if (!m_lastPacket && !consistency_flow && !read_flow) {
		NS_LOG_UNCOND("Application index "<<application_index);

		for(uint32_t i=1 ; i<= ns3::BaseTopology::chunk_assignment_to_applications[application_index][0];i++)
		{
			//NS_LOG_UNCOND("Here We go");

			uint32_t chunk_value = destination_chunks[i-1];

			uint32_t chunk_location = getChunkLocation(chunk_value, &version, &socket_index);

			//NS_LOG_UNCOND("The chunk Location is "<<chunk_location<<" "<<chunk_value);

			double bandwidth_distribution = ns3::BaseTopology::chunk_assignment_probability_to_applications[application_index][i] * (double)m_flowRequiredBW;

			Ipv4GlobalRouting::host_utilization[chunk_location] -= bandwidth_distribution;

			Ipv4GlobalRouting::host_utilization_smoothed[chunk_location]=(alpha*Ipv4GlobalRouting::host_utilization[chunk_location])+((1-alpha)*Ipv4GlobalRouting::host_utilization_smoothed[chunk_location]);


			BaseTopology::host_running_avg_bandwidth[chunk_location] += Ipv4GlobalRouting::host_utilization[chunk_location];

			BaseTopology::host_running_avg_counter[chunk_location]++;


			uint32_t pod = (uint32_t) floor((double) chunk_location/ (double) total_hosts_in_pod);

			uint32_t node = ((chunk_location - 1)/(SSD_PER_RACK + 1)) % Ipv4GlobalRouting::FatTree_k;

			BaseTopology::p[pod].nodes[node].utilization -= bandwidth_distribution;



			for(uint32_t chunk_index = 0 ;chunk_index < BaseTopology::p[pod].nodes[node].total_chunks;chunk_index++)
			{
				if(BaseTopology::p[pod].nodes[node].data[chunk_index].chunk_number == chunk_value)
				{
					if(BaseTopology::p[pod].nodes[node].data[chunk_index].intensity_sum>bandwidth_distribution)
						{BaseTopology::p[pod].nodes[node].data[chunk_index].intensity_sum -= bandwidth_distribution;
						if (BaseTopology::p[pod].nodes[node].data[chunk_index].intensity_sum<1)
							BaseTopology::p[pod].nodes[node].data[chunk_index].intensity_sum=0.0;
						}
					else
						BaseTopology::p[pod].nodes[node].data[chunk_index].intensity_sum=0.0;

					BaseTopology::p[pod].nodes[node].data[chunk_index].processed=0;



					if(BaseTopology::p[pod].nodes[node].data[chunk_index].intensity_sum < 0.0)
					{
						BaseTopology::p[pod].nodes[node].data[chunk_index].intensity_sum = ceil(BaseTopology::p[pod].nodes[node].data[chunk_index].intensity_sum );
					}
					break;
				}
			}


		}
		uint32_t nodes_in_pod = number_of_hosts / Ipv4GlobalRouting::FatTree_k;
		for (int i=0;i<Ipv4GlobalRouting::FatTree_k;i++)
		{
			BaseTopology::p[i].Pod_utilization=0;
			for(uint32_t j=0;j<nodes_in_pod;j++)
			{
				BaseTopology::p[i].Pod_utilization=BaseTopology::p[i].Pod_utilization+BaseTopology::p[i].nodes[j].utilization;
			}
		}
		uint32_t total_hosts_in_system = (SSD_PER_RACK + 1) * (simulationRunProperties::k/2) * (simulationRunProperties::k/2) * simulationRunProperties::k;

		FILE *fp_host_utilization;
		double time_now=Simulator::Now().ToDouble(Time::MS);
		fp_host_utilization = fopen ("host_utilization_madhurima.csv","a");
		fprintf(fp_host_utilization,"%f,",time_now);
		for (uint32_t t=0;t<total_hosts_in_system;t++)
		{
			//fprintf(fp_host_utilization,"%f,,",Ipv4GlobalRouting::host_utilization[t]);
			fprintf(fp_host_utilization,"%f,",BaseTopology::host_utilization_outgoing[t]);
			//fprintf(fp_host_utilization,"%f,,",Ipv4GlobalRouting::host_utilization_smoothed[t]);

		}
		fprintf(fp_host_utilization,"\n");
		fclose(fp_host_utilization);

if(simulationRunProperties::enableOptimizer)
{
		//BaseTopology::counter_++;
		/********Uncomment it when function ReturnSomething is ready */
		if(BaseTopology::counter_==0 )
		{

			int incrDcr=0;

			BaseTopology::calculateNewLocation(incrDcr);

			int i=0;

		    FILE *fp_copy;

		    fp_copy = fopen ("copy_create_delete.csv","a");
			while(BaseTopology::res[i].src != 99999 && BaseTopology::res!=NULL)
			{
				printf("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n");
				 char p='d';
				 BaseTopology::copy_deleted++;
				 fprintf(fp_copy,"%c,%f,%d,%d,%d,0,0\n",p,Simulator::Now().ToDouble(Time::MS),BaseTopology::res[i].chunk_number,BaseTopology::res[i].src,BaseTopology::res[i].dest);
				//BaseTopology::chunk_copy_node_tracker[BaseTopology::res[i].chunk_number][BaseTopology::res[i].src] = false;
				//BaseTopology::chunk_copy_node_tracker[BaseTopology::res[i].chunk_number][BaseTopology::res[i].dest] = true;
				NS_LOG_UNCOND("src "<<BaseTopology::res[i].src<<" dest "<<BaseTopology::res[i].dest<<" chunk_no "<<BaseTopology::res[i].chunk_number);
				NS_ASSERT_MSG(BaseTopology::chunkTracker.at(BaseTopology::res[i].chunk_number).number_of_copy > 0, " SomeThing Wrong in deletion of the copy ");
				if(BaseTopology::chunkTracker.at(BaseTopology::res[i].chunk_number).number_of_copy > 0)
				{
					uint32_t chunk_location = FindChunkAssignedHost(BaseTopology::res[i].chunk_number, BaseTopology::res[i].src);
						//uint32_t round_robin_counter = BaseTopology::host_assignment_round_robin_counter[BaseTopology::res[i].dest] % SSD_PER_RACK;
					BaseTopology::chunkTracker.at(BaseTopology::res[i].chunk_number).number_of_copy --;
					BaseTopology::chunkTracker.at(BaseTopology::res[i].chunk_number).logical_node_id = (SSD_PER_RACK + 1) * BaseTopology::res[i].dest + 1;

					BaseTopology::chunk_copy_node_tracker[BaseTopology::res[i].chunk_number][chunk_location] = false;

					uint32_t node_name=BaseTopology::res[i].src%Ipv4GlobalRouting::FatTree_k;
					uint32_t pod_name=BaseTopology::res[i].src/Ipv4GlobalRouting::FatTree_k;
					uint32_t deletion_location;
					uint32_t first_location=9999999;
					for(uint32_t j = 0; j<BaseTopology::p[pod_name].nodes[node_name].total_chunks; j++)
					{
						 if(BaseTopology::p[pod_name].nodes[node_name].data[j].chunk_number==BaseTopology::res[i].chunk_number)
						 {
						   deletion_location=j;
						   if(first_location==9999999)//get hold of the first index of the item in the list in the list
							   first_location=deletion_location;
						  // break;
						 }
					}
					if(first_location!=deletion_location)
					{
						BaseTopology::p[pod_name].nodes[node_name].data[first_location].chunk_count--;
					}
					if(deletion_location==BaseTopology::p[pod_name].nodes[node_name].total_chunks-1)
					{
						BaseTopology::p[pod_name].nodes[node_name].total_chunks--;
					}
					else
					{
						 for(uint32_t j = deletion_location; j<BaseTopology::p[pod_name].nodes[node_name].total_chunks-1; j++)
						 {
							 BaseTopology::p[pod_name].nodes[node_name].data[j]=BaseTopology::p[pod_name].nodes[node_name].data[j+1]; //copying structure
						 }
						 BaseTopology::p[pod_name].nodes[node_name].total_chunks--;
					}
					}

				else
				{
					NS_LOG_UNCOND("----------Something is wrong with deletion of copy-------------");

				}

				i++;

			}
			BaseTopology::counter_=0;
			fclose(fp_copy);
		}

}

		//calling the optimizer


		//Simulator::Cancel(m_sendEvent);
		SendLastPacket();

		CreateandRemoveIndependentReadFlows(distinct_items, m_finish_time, -1);
	}
// appl was already stopped. !!
	NS_ASSERT_MSG(m_socket, "Appl stopped, socket closed");


	//if(this->consistency_flow) NS_LOG_UNCOND(" This is almost end");

	for(uint32_t i=0;i<distinct_items;i++)
	{
		m_socket[i]->Close();
		m_socket[i]->SetRecvCallback(MakeNullCallback<void, Ptr<Socket> >());
		m_socket[i] = 0;

	}
	for(uint32_t i=0;i<total_sync_sockets;i++)
	{
		sync_sockets[i]->Close();
		sync_sockets[i]->SetRecvCallback(MakeNullCallback<void, Ptr<Socket> >());
		sync_sockets[i] = 0;
	}

	m_socket = NULL;

	sync_sockets = NULL;

	if (WRITE_PACKET_TIMING_TO_FILE)
		fp2.close();

// let external trigger RESTART a new flow,
	SS_APPLIC_LOG(
			"Old flow [" << m_currentFlowCount << "] at node ["
			<< GetNode()->GetId() << "] ["<< m_srcIpv4Address
			<< "] connected to node [" << m_dstHost << "] ["<< m_dstIpv4Address
			<< "] stopped @ [" << Simulator::Now().ToDouble(Time::MS) << "ms]"
			<< " #pkts " << t_sentPacketCount);
// call backs, if any...
	if (!m_startNewFlow_CallbackSS.IsNull()) {
		m_startNewFlow_CallbackSS(m_currentFlowCount, GetNode()->GetId(),
				m_dstHost, m_flowRequiredBW);
		m_startNewFlow_CallbackSS.Nullify();
	}

	local_chunkTracker.clear();

	if(!this->consistency_flow && !read_flow)
	{
		delete[] destination_chunks;

		delete[] sync_socket_tracker;

	}

	if(read_flow)
	{
		if(!this->consistency_flow)  //JB change Dec 12
		{
			delete[] chunk_assignment_probability_for_read_flow;
			delete[] selected_chunk_for_read_flow;
		}
	}

	socket_mapping.clear();

}



//
// not very elegant - but I will let it run now...
//
void ssUdpEchoClient::SendLastPacket(void) {
//	NS_LOG_FUNCTION(this);
//
//	NS_LOG_LOGIC(
//			this << " Sending LAST packet at [" << Simulator::Now () << "] packetCount [" << t_sentPacketCount << "] flowCount [" << m_currentFlowCount << "]");
//	for(uint32_t i=0; i<total_hosts;i++)
//	{
//		//BaseTopology::total_packet_count++;
//		m_lastPacket = true;
//		t_p = createPacket();
//		m_txTrace(t_p);
//		//NS_LOG_UNCOND("This is the end");
//		int actual = m_socket[i]->Send(t_p);
//	// We exit this loop when actual < toSend as the send side
//	// buffer is full. The "DataTxComplete" callback will pop when
//	// some buffer space has freed ip.
//		if ((unsigned) actual != m_packetSize) {
//			NS_ABORT_MSG(
//					"Node [" << m_node->GetId() << "] error sending last packet");
//		}
//	}

// end of send packet...wait for blocking call to be released (txComplete) before sending next packet...
}
//
//
//
void ssUdpEchoClient::callback_HandleRead(Ptr<Socket> socket) {
	NS_LOG_FUNCTION(this << socket);
	Ptr<Packet> packet;
	Address from;
	while ((packet = socket->RecvFrom(from))) {
		if (InetSocketAddress::IsMatchingType(from)) {
			NS_LOG_LOGIC(
					this << " At time " << Simulator::Now ().GetSeconds () << "s client received " << packet->GetSize () << " bytes from " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " << InetSocketAddress::ConvertFrom (from).GetPort ());
		} else if (Inet6SocketAddress::IsMatchingType(from)) {
			NS_LOG_LOGIC(
					this << " At time " << Simulator::Now ().GetSeconds () << "s client received " << packet->GetSize () << " bytes from " << Inet6SocketAddress::ConvertFrom (from).GetIpv6 () << " port " << Inet6SocketAddress::ConvertFrom (from).GetPort ());
		}
		// sanjeev simplified..3/28
		if (packet->is_Last)
			NS_LOG_LOGIC(
					"Last packet of #flowId " << packet->flow_id << ", packetId" << packet->packet_id);
	}
}

void ssUdpEchoClient::setFlowVariables(void) {
	NS_LOG_FUNCTION(this);
	if (WRITE_PACKET_TIMING_TO_FILE) {
		char* aFileName = (char*) malloc(
				std::strlen(PACKETINTERVAL_DEBUG_FILENAME2)
						+ std::strlen(simulationRunProperties::arg0) + 10);
		std::sprintf(aFileName, "%s_%d_%s", simulationRunProperties::arg0,
				m_currentFlowCount, PACKETINTERVAL_DEBUG_FILENAME2);
		fp2.open(aFileName);
	}
// set some application+flow level parameters...
	t_sentPacketCount = 0;
	m_lastPacket = false;
	m_priority = (m_currentFlowCount % 3 ? HIGH : LOW);
//...
	double t_packet_per_second = m_flowRequiredBW * MEGABITS_TO_BITS
			/ (m_packetSize * 8);

	if (floor(t_packet_per_second) <= 0.0)
	{
		no_packet_flow = true;
		t_packet_per_second = 1.0;

	}

	double t_interpacketInterval = 1.0 / t_packet_per_second;
	NS_LOG_FUNCTION(
			"reqBW " << m_flowRequiredBW << " useRandom "<< useRandomIntervals << " pkt/sec " << t_packet_per_second << " inter-packet interval " << t_interpacketInterval);


// set random packet interval OR constant packet interval
	if (useRandomIntervals) {
		// May 14.
		// https://www.nsnam.org/docs/release/3.26/doxygen/classns3_1_1_gamma_random_variable.html#a65f2515eaf15d4540509c9620c844c57
		// SS -> MR - verify setting this alpha & beta values (alpha=mean, beta=variance)
		if (USE_GAMMA_DISTRIBUTION && (simulationRunProperties::k == 4)) {
			double variance = GetHostSpecificVariance();

			double mean =t_interpacketInterval;

			double beta=((variance*variance)*(mean*mean))/mean;

			double alpha=mean/beta;

			m_randomVariableInterPacketInterval = CreateObject<
					GammaRandomVariable>();
			m_randomVariableInterPacketInterval->SetAttribute("Alpha",
					DoubleValue(alpha));
			m_randomVariableInterPacketInterval->SetAttribute("Beta",
					DoubleValue(beta));
		} else {
			m_randomVariableInterPacketInterval = CreateObject<
					ExponentialRandomVariable>();
			m_randomVariableInterPacketInterval->SetAttribute("Mean",
					DoubleValue(t_interpacketInterval));
		}
	} else {
		m_randomVariableInterPacketInterval = CreateObject<
				ConstantRandomVariable>();
		m_randomVariableInterPacketInterval->SetAttribute("Constant",
				DoubleValue(t_interpacketInterval));
	}
	if (m_currentFlowCount == 0)
		SS_APPLIC_LOG("For every flow, interpacketInterval::type " << m_randomVariableInterPacketInterval->GetInstanceTypeId());

	m_packetInterval = Seconds(m_randomVariableInterPacketInterval->GetValue()); //	setNextFlowInterval

	Ptr<ssTOSPointToPointNetDevice> d = DynamicCast<ssTOSPointToPointNetDevice>(
			GetNode()->GetDevice(1));
	m_srcIpv4Address = d->getIpv4Address();
	m_dstIpv4Address = Ipv4Address::ConvertFrom(m_peerAddress);

	/*******Chunk Specific Changes ****************/

	//uint32_t application_id = ns3::BaseTopology::application_assignment_to_node[node_index][application_index];

	//uint32_t number_of_chunks_assigned = ns3::BaseTopology::chunk_assignment_to_applications[application_id][0];

	//ClientChunkAccessGenerator = CreateObject<ZipfRandomVariable>();
	//ClientChunkAccessGenerator->SetAttribute ("N", IntegerValue (number_of_chunks_assigned));
	//ClientChunkAccessGenerator->SetAttribute ("Alpha", DoubleValue (simulationRunProperties::ChunkzipfGeneratorAlpha));

	ReadWriteCalculation = CreateObject<UniformRandomVariable> ();
	ReadWriteCalculation->SetAttribute ("Min", DoubleValue (0.0));
	ReadWriteCalculation->SetAttribute ("Max", DoubleValue (1.0));

	ClientChunkAccessGenerator = CreateObject<UniformRandomVariable> ();
	ClientChunkAccessGenerator->SetAttribute ("Min", DoubleValue (0.0));
	ClientChunkAccessGenerator->SetAttribute ("Max", DoubleValue (1.0));

	ReadFlowClientChunkAccessGenerator = CreateObject<UniformRandomVariable> ();
	ReadFlowClientChunkAccessGenerator->SetAttribute ("Min", DoubleValue (0.0));
	ReadFlowClientChunkAccessGenerator->SetAttribute ("Max", DoubleValue (1.0));

	/**********************************************/


}
//
// added by sanjeev see *.cc file
// sanjeev... add packet tag with packet properties required by JB/MR...
//

void ssUdpEchoClient::Send(void) {

	//NS_LOG_UNCOND("BaseTopology::chunkTracker.at(0).number_of_copy "<<BaseTopology::chunkTracker.at(0).number_of_copy);
	NS_LOG_FUNCTION(this);


	if(no_packet_flow)
	{
		ScheduleTransmit(m_packetInterval);
		return;
	}


	//if(consistency_flow) NS_LOG_UNCOND("Entered Here");
	//NS_LOG_UNCOND("Entered Here");
	if (m_socket == NULL)
			return;
		else
			NS_ASSERT(m_sendEvent.IsExpired());
#if COMPILE_CUSTOM_ROUTING_CODE
// flow needs to continue or stop?drop?
	if (Ipv4GlobalRouting::FindReachThreshold(m_currentFlowCount)) {
		// sanjeev, as discussed on Mar 15
		ssUdpEchoClient::flows_dropped++;
		m_lastPacket = true;
		SS_APPLIC_LOG(
				"Dropflow [" << m_currentFlowCount << "] at node ["
				<< GetNode()->GetId() << "] ["<< m_srcIpv4Address
				<< "] connected to node [" << m_dstHost << "] ["<< m_dstIpv4Address
				<< "] stopped @ [" << Simulator::Now().ToDouble(Time::MS) << "ms]"
				<< " #pkts " << t_sentPacketCount);
	}
#endif
	/* bug sanjeev 7 Feb, rectified	 - old code deleted*/

	//NS_LOG_UNCOND("I am here "<<application_index);
	uint32_t chunk_value, dest_value;

	double desired_value;

	chunk_value = single_destination;

	uint32_t version;

	int socket_index;

	uint32_t num_of_packets_to_send = 1;

	bool is_write = !read_flow;


	if(consistency_flow)
	{
		//exit(0);
		if(this->total_packets_to_send > 0) BaseTopology::total_packet_count += this->total_packets_to_send;
	}
	else
	{
		BaseTopology::total_packet_count ++;
	}

	if(Ipv4GlobalRouting::has_system_learnt)
	{
		if(BaseTopology::last_point_of_entry <= 0.0)
		{
			BaseTopology::last_point_of_entry = Simulator::Now().ToDouble(Time::MS);
		}
		else
		{
			double current_time =  Simulator::Now().ToDouble(Time::MS);
			double time_diff = current_time - BaseTopology::last_point_of_entry;

			BaseTopology::last_point_of_entry = current_time;

			BaseTopology::sum_of_number_time_packets += (BaseTopology::total_packet_count -1) * time_diff;

			//NS_LOG_UNCOND(" BaseTopology::total_packet_count -1) * time_diff "<<(BaseTopology::total_packet_count -1) * time_diff<<" Total packets "<<BaseTopology::total_packet_count);

			BaseTopology::total_sum_of_entry += time_diff;
		}

		BaseTopology::total_packet_count_inc += BaseTopology::total_packet_count;

		BaseTopology::total_events++;
	}

	if(!consistency_flow)
	{
		if(is_write) //this is write request
		{
			double inc_prob = 0.0;
			uint32_t index = 0;


			desired_value = ClientChunkAccessGenerator->GetValue();

			//NS_LOG_UNCOND("Desired value "<<desired_value);
			for(;index<ns3::BaseTopology::chunk_assignment_to_applications[this->application_index][0];index++)
			{
				inc_prob += ns3::BaseTopology::chunk_assignment_probability_to_applications[this->application_index][index+1];

				if(inc_prob >= desired_value)
				{
					break;
				}
			}

			//NS_LOG_UNCOND("The index is "<<index+1<<" The value is "<<inc_prob);
			//chunk_value = BaseTopology::chunk_assignment_to_applications[this->application_index][index+1];
			chunk_value = destination_chunks[index];


			BaseTopology::chnkCopy[chunk_value].uniqueWrite++;

			num_of_packets_to_send = BaseTopology::chunkTracker.at(chunk_value).number_of_copy + 1;

			//NS_LOG_UNCOND("The num_of_packets_to_send "<<num_of_packets_to_send<<" Chunk value "<<chunk_value);

			BaseTopology::total_packet_count += num_of_packets_to_send -1 ;
			//is_write = read_flow;
			BaseTopology::chunk_version_tracker[chunk_value]++;
		}
	}

	//very bad structured code, fix it later

	bool sync_packet = is_write;

	if((!is_write || num_of_packets_to_send == 1) && !consistency_flow)
	{
		BaseTopology::total_non_consistency_packets++;

		if(is_write) dest_value = getChunkLocation(chunk_value, &version, &socket_index);

		else //this is read flow request
		{

			dest_value = single_destination;

			version = 0;

			socket_index = 0;

			desired_value = ReadFlowClientChunkAccessGenerator->GetValue();

			double inc_prob = 0.0;

			uint32_t chunk_index;

			for(chunk_index = 0;chunk_index<selected_total_chunk;chunk_index++)
			{
				inc_prob += chunk_assignment_probability_for_read_flow[chunk_index];

				if(inc_prob >= desired_value)
				{
					break;
				}
			}


			chunk_value = selected_chunk_for_read_flow[chunk_index];

		}



		//BaseTopology::total_packets_to_chunk[chunk_value]++;
		
		t_p = createPacket(chunk_value, dest_value, is_write, version, sync_packet);

		//if(read_flow && chunk_value == 0) NS_LOG_UNCOND("T_p info "<<t_p->sub_flow_id<<" t_p->packet_id "<<t_p->packet_id);
		//t_p = createPacket(chunk_value, dest_value, is_write, version);
		// call to the trace sinks before the packet is actually sent,
		// so that tags added to the packet can be sent as well
		m_txTrace(t_p);

		int actual = m_socket[socket_index]->Send(t_p);




		// We exit this loop when actual < toSend as the send side
		// buffer is full. The "DataTxComplete" callback will pop when
		// some buffer space has freed ip.
		if ((unsigned) actual != m_packetSize) {
			NS_ABORT_MSG(
					"ssUdpEchoClient Node [" << m_node->GetId() << "] error sending packet");
		}
		NS_LOG_LOGIC(
				this << " At time " << Simulator::Now ().GetSeconds () << "s client " << m_srcIpv4Address << " sent flowid " << m_currentFlowCount << " packet# " << t_sentPacketCount << " packetUid [" << t_p->GetUid() << "]");

		t_sentPacketCount++;

	}

	else
	{
		if(consistency_flow)
		{
			//BaseTopology::total_packet_count += this->total_packets_to_send -1;
			//NS_LOG_UNCOND(" ^^^^^^^^^^^^^^^^^^This is Consistency traffic^^^^^^^^^^^^^^^^");
			for(uint32_t cpacket=0;cpacket<this->total_packets_to_send;cpacket++)
			{

				BaseTopology::total_consistency_packets++;
				//NS_LOG_UNCOND("Inside For loop");
				//t_p = createPacket(0, single_destination, is_write, 0);
				t_p = createPacket(0, single_destination, is_write, 0, false, true);
				m_txTrace(t_p);
				int actual = m_socket[0]->Send(t_p);


				if ((unsigned) actual != m_packetSize) {
					NS_ABORT_MSG(
							"ssUdpEchoClient Node [" << m_node->GetId() << "] error sending packet");
				}
			}

			this->total_packets_to_send = 0;
		}
		else //this is not consistency flow and number of packets to be sent is more than 1, and this is a write operation
		{

			//NS_LOG_UNCOND("&&&&&&&&&&&& num_of_packets_to_send &&&&&&&&&&&&& "<<num_of_packets_to_send<<" Chunk value "<<chunk_value);

			//uint32_t total_sync_packets = 0;
			uint32_t current_version = BaseTopology::chunkTracker.at(chunk_value).version_number;

			BaseTopology::chunkTracker.at(chunk_value).version_number++;

			for(uint32_t host_index=0;host_index<total_hosts;host_index++)
			{
				bool sync_traffic = true;
				uint32_t rack_id ;
				if(BaseTopology::chunk_copy_node_tracker[chunk_value][host_index])
				{
					//total_sync_packets++;
					BaseTopology::total_consistency_packets++;
					if(local_chunkTracker.at(chunk_value).node_id == host_index) //there is a socket already opened for it
					{
						sync_traffic = false;
					}
					else
					{
						rack_id = host_index;

						if(sync_socket_tracker[(int)rack_id] == -1)
						{
							sync_socket_tracker[(int)rack_id] = total_sync_sockets;

							TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
							sync_sockets[total_sync_sockets] = Socket::CreateSocket(GetNode(), tid);

							if (Ipv4Address::IsMatchingType(m_peerAddress) == true) {
								sync_sockets[total_sync_sockets]->Bind();
								sync_sockets[total_sync_sockets]->Connect(InetSocketAddress(BaseTopology::hostTranslation[host_index], m_peerPort));
							}

							total_sync_sockets ++;

						}


					}
					//t_p = createPacket(chunk_value, host_index, is_write, 0);

					t_p = createPacket(chunk_value, host_index, is_write, current_version, sync_packet);

					m_txTrace(t_p);

					int actual;

					if(sync_traffic)
					{
						actual = sync_sockets[sync_socket_tracker[(int)rack_id]]->Send(t_p);

					}
					else
					{
						actual = m_socket[local_chunkTracker.at(chunk_value).socket_index]->Send(t_p);
					}

					if ((unsigned) actual != m_packetSize)
					{
						NS_ABORT_MSG("ssUdpEchoClient Node [" << m_node->GetId() << "] error sending packet");
					}
					NS_LOG_LOGIC(
							this << " At time " << Simulator::Now ().GetSeconds () << "s client " << m_srcIpv4Address << " sent flowid " << m_currentFlowCount << " packet# " << t_sentPacketCount << " packetUid [" << t_p->GetUid() << "]");

					t_sentPacketCount++;

				}
			}

		}
		//NS_LOG_UNCOND("THe size of BaseTopology::chunkCopyLocations1 "<<BaseTopology::chunkCopyLocations.size());
	}



	// did we send the last packet?, no need for further packet creation/sending
	if (m_lastPacket) {
		// trigger new flow....
		if (!m_startNewFlow_CallbackSS.IsNull()) {
			m_startNewFlow_CallbackSS(m_currentFlowCount, GetNode()->GetId(),
					m_dstHost, m_flowRequiredBW);
			m_startNewFlow_CallbackSS.Nullify();
		}
	} else {
		ScheduleTransmit(m_packetInterval);
	}
	//NS_LOG_UNCOND("I am here end"<<application_index);
}


/*Ptr<Packet> ssUdpEchoClient::createPacket(uint32_t chunk_id, uint32_t chunk_location, bool is_write, uint32_t version) {
	return createPacket(m_currentFlowCount, t_sentPacketCount, m_flowRequiredBW,
			m_lastPacket, UNUSED, (t_sentPacketCount == 0 ? true : false),
			m_flowStartTimeNanoSec, m_priority, m_srcIpv4Address,
			m_dstIpv4Address, chunk_id, chunk_location, is_write, version);
}*/

Ptr<Packet> ssUdpEchoClient::createPacket(uint32_t chunk_id, uint32_t chunk_location, bool is_write, uint32_t version, bool sync_packet, bool copy_creation) {
	return createPacket(m_currentFlowCount, t_sentPacketCount, m_flowRequiredBW,
			m_lastPacket, UNUSED, (t_sentPacketCount == 0 ? true : false),
			m_flowStartTimeNanoSec, m_priority, m_srcIpv4Address,
			m_dstIpv4Address, chunk_id, chunk_location, is_write, version, sync_packet, copy_creation);
}

Ptr<Packet> ssUdpEchoClient::createPacket(void) {
	return createPacket(m_currentFlowCount, t_sentPacketCount, m_flowRequiredBW,
			m_lastPacket, UNUSED, (t_sentPacketCount == 0 ? true : false),
			m_flowStartTimeNanoSec, m_priority, m_srcIpv4Address,
			m_dstIpv4Address, 0, 0, false, 0);
}

Ptr<Packet> ssUdpEchoClient::createPacket(const uint32_t &flowId,
		const uint32_t &packetId, const uint32_t &requiredBW, const bool &last,
		const appType &p2, const bool &isFirstPacket,
		const uint64_t &FlowStartTimeNanoSec, const PacketPriority &priority,
		const Ipv4Address &src, const Ipv4Address &dst, uint32_t chunk_id, uint32_t chunk_location, bool is_write, uint32_t version, bool sync_packet, bool copy_creation) {
		//const Ipv4Address &src, const Ipv4Address &dst, uint32_t chunk_id, uint32_t chunk_location, bool is_write, uint32_t version) {
	NS_LOG_FUNCTION(this);

	t_p = Create<Packet>(m_packetSize);

	t_p->sub_flow_id = chunk_id;

	BaseTopology::total_packets_sent_to_chunk[chunk_id]++;

	FILE *fp_packet;
	fp_packet= fopen("all_packets_send.csv","a");
	//value = getChunkLocation(value, &version);

	/**********Added By Joyanta *************/
	t_p->AddFlowIdWithPacket(flowId);
	t_p->AddBandwidthWithPacket(requiredBW);
	t_p->AddLastWithPacket(last);
	t_p->AddIcmpDestUnreach(false);

	t_p->sub_flow_dest = chunk_location;

	t_p->sub_flow_dest_physical = BaseTopology::hostaddresslogicaltophysical[chunk_location];

	t_p->application_id = application_index;
	/***************************************/
	t_p->is_First = isFirstPacket;
	t_p->packet_id = packetId;
	t_p->dstNodeId = m_dstHost;
	t_p->srcNodeId = GetNode()->GetId();
	t_p->is_write = is_write;
	t_p->creation_time = Simulator::Now().ToDouble(Time::US);
	t_p->version = version;
	t_p->sync_packet = sync_packet;

	t_p->copy_creation_packet = copy_creation;
	t_p->is_phrase_changed = false;


//	printf("%d, %d, %d , %d, %d, %d, %d, %d,%d, %d, %f\n", flowId, requiredBW, chunk_location , t_p->sub_flow_dest_physical, application_index, isFirstPacket, packetId, m_dstHost,GetNode()->GetId(), is_write, t_p->creation_time);
	//fprintf(fp_packet,"%d, %d, %d , %d, %d, %d, %d, %d,%d, %d, %f\n", flowId, requiredBW, chunk_location , t_p->sub_flow_dest_physical, application_index, isFirstPacket, packetId, m_dstHost,GetNode()->GetId(), is_write, t_p->creation_time);

	if(t_p->sync_packet)
	{
		BaseTopology::transaction_rollback_write_tracker[t_p->sub_flow_dest][t_p->srcNodeId -20]++;
	}

	if(BaseTopology::total_phrase_changed == 3)
	{
		t_p->is_phrase_changed = true;
		BaseTopology::pkt_sent_during_phase2++;
	}
	else if(BaseTopology::total_phrase_changed >= simulationRunProperties::phrase_change_number)
	{
		BaseTopology::pkt_sent_during_phase3++;
	}
	else{
		BaseTopology::pkt_sent_during_phase1++;
	}
	//NS_LOG_UNCOND(t_p->srcNodeId);
	if(t_p->sub_flow_dest_physical == t_p->srcNodeId)
	{

		NS_LOG_UNCOND("The dest and src are same "<<t_p->sub_flow_dest_physical);
		exit(0);
	}
	//NS_LOG_UNCOND("The t_p->sub_flow_dest_physical "<<t_p->sub_flow_dest_physical<<" t_p->srcNodeId "<<t_p->srcNodeId);

// sanjeev debug...3/15..trace packetinfo at client !!! // Apr 24
	if (isFirstPacket || last)
		ssUdpEchoServer::writeFlowDataToFile(
				Simulator::Now().ToDouble(Time::US), t_p->flow_id,
				ssNode::CLIENT, t_p->is_First, t_p->is_Last, t_p->packet_id,
				GetNode()->GetId(), src, m_stopTime.ToDouble(Time::MS),
				m_flowRequiredBW);
// debug all packet, SS,JB: Mar 26
	if (WRITE_PACKET_TO_FILE)
		ssTOSPointToPointNetDevice::tracePacket(
				Simulator::Now().ToDouble(Time::US), m_currentFlowCount,
				t_sentPacketCount, t_p->GetUid(), m_lastPacket,
				GetNode()->GetId(), 0, DynamicCast<ssNode>(GetNode())->nodeName,
				"SRC", t_p->required_bandwidth);
	fclose(fp_packet);
	return t_p;
}



void ssUdpEchoClient::RegisterStartNewFlow_CallbackSS(
		startNewFlow_CallbackSS cbSS) {
	NS_LOG_FUNCTION(this);
	m_startNewFlow_CallbackSS = cbSS;
}
/*
 * May 14
 */
double ssUdpEchoClient::GetHostSpecificVariance(void) {
	NS_LOG_FUNCTION(this);
	int hostId = GetNode()->GetId() - HOST_NODE_START_POINT;

	NS_LOG_UNCOND("The host id is "<<hostId);
	NS_ASSERT_MSG(hostId >= 0 && hostId < NUMBER_OF_HOST_VARIANCE_SUPPORTED,
			"GetHostSpecificVariance HostId failed " << hostId);
	// SS ->MR check this condition, beta/variance should be greater than 1? see wehpage
	NS_ASSERT_MSG(m_hostSpecificVariance_Values[hostId] > 0,
				"m_hostSpecificVariance_Values[" << hostId <<"] initialization failed [" << m_hostSpecificVariance_Values[hostId] << "]");
	return m_hostSpecificVariance_Values[hostId];
}

uint32_t ssUdpEchoClient::getChunkLocation(uint32_t chunk, uint32_t *version, int * socket_index)
{
	for (std::vector<local_chunk_info>::iterator it = local_chunkTracker.begin() ; it != local_chunkTracker.end(); ++it)
	{
		if(it->chunk_id == chunk)
		{
			*version = it->version;
			*socket_index = it->socket_index;
			return it->node_id;
		}
	}
	//NS_LOG_UNCOND("^^^^^^^^^^^^^^The chunk number is^^^^^^^^^^^^^^"<<chunk);
	return -1;
}

void ssUdpEchoClient::ChangePopularity()
{
	if(Simulator::Now().ToDouble(Time::MS) > ns3::BaseTopology::popularity_change_simulation_interval)
	{
		uint32_t total_chunks_to_shunffle = (uint32_t)simulationRunProperties::total_chunk * simulationRunProperties::fraction_chunk_changes_popularity;
		//change the priority
		for(int i=0;i<=20;i++)
		{
			NS_LOG_UNCOND("******************");
		}

		for(uint32_t i=0;i<total_chunks_to_shunffle;i++)
		{
			//NS_LOG_UNCOND(ns3::BaseTopology::Popularity_Change_Random_Variable->GetInteger());
			uint32_t source_index = ns3::BaseTopology::Popularity_Change_Random_Variable->GetInteger();

			uint32_t dest_index =  ns3::BaseTopology::Popularity_Change_Random_Variable->GetInteger();

			uint32_t temp = BaseTopology::virtual_to_absolute_mapper[source_index];

			BaseTopology::virtual_to_absolute_mapper[source_index] = BaseTopology::virtual_to_absolute_mapper[dest_index];

			BaseTopology::virtual_to_absolute_mapper[dest_index] = temp;

			NS_LOG_UNCOND(source_index<<" --> "<<dest_index);

		}

		ns3::BaseTopology::popularity_change_simulation_interval = Simulator::Now().ToDouble(Time::MS) + simulationRunProperties::popularity_change_interval_ms;
	}
}

void ssUdpEchoClient::ChangeIntensity()
{
	if(BaseTopology::total_phrase_changed >= simulationRunProperties::phrase_change_number)
	{
		BaseTopology::intensity_change_scale = 1;
		return;
	}
	if(Simulator::Now().ToDouble(Time::MS) > ns3::BaseTopology::intensity_change_simulation_interval)
	{
		for(int i=0;i<=20;i++)
		{
			NS_LOG_UNCOND("--------------------------");
		}

		//NS_LOG_UNCOND("BaseTopology::total_phrase_changed "<<BaseTopology::total_phrase_changed);

		BaseTopology::intensity_change_scale = BaseTopology::phrase_change_intensity_value[BaseTopology::total_phrase_changed];
		ns3::BaseTopology::intensity_change_simulation_interval += BaseTopology::phrase_change_interval[BaseTopology::total_phrase_changed];
		BaseTopology::total_phrase_changed++;


	}
}


}  // namespace
