/*
 * ss-udp-echo-client.cc7
 *
 *  Created on: Sep 30, 2016
 *      Author: sanjeev
 */

#include "ss-udp-echo-client.h"
#include "ss-udp-echo-server.h"
#include "ss-log-file-flags.h"

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

ApplicationContainer ssUdpEchoClientHelper::Install(Ptr<Node> node, bool single_destination_flow, uint32_t source_node, uint32_t app_id, uint32_t dest_node, uint32_t no_of_packets) const {
	return ApplicationContainer(InstallPriv(node,single_destination_flow, source_node, app_id, dest_node, no_of_packets));
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

Ptr<Application> ssUdpEchoClientHelper::InstallPriv(Ptr<Node> node, bool single_destination_flow, uint32_t source_node, uint32_t app_id, uint32_t dest_node, uint32_t no_of_packets) const {
	Ptr<ssUdpEchoClient> app = m_factory.Create<ssUdpEchoClient>();

	NS_LOG_UNCOND("The source node is "<<source_node);

	app->node_index = source_node;

	if(single_destination_flow)
	{
		app->single_destination = dest_node;

		app->total_packets_to_send = no_of_packets;
	}

	NS_LOG_UNCOND("The random_application_id is "<<app_id);

	//NS_LOG_UNCOND("ns3::BaseTopology::application_assignment[source_node][0] "<<ns3::BaseTopology::application_assignment[source_node][0]);

	app->application_index = app_id;


	app->consistency_flow = single_destination_flow;
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
					MakeUintegerChecker<uint32_t>()).AddTraceSource("Tx",
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

				//if(chunk_location == 0) NS_LOG_UNCOND("^^^^^^^^^Hello Shukhla^^^^^^^^^^^^^^^^^^");
			}
		}

		//if((BaseTopology::chunk_copy_selection->GetInteger()))

		//if(it->logical_node_id %2 == 0) NS_LOG_UNCOND("Even logical node id");

		local_chunkTracker.push_back(local_chunk_info(it->chunk_no, chunk_location, it->version_number, it->node_id));
	}


	uint32_t total_racks = total_hosts/(SSD_PER_RACK +1 );

	sync_socket_tracker = new int[total_racks];

	for(uint32_t i=0;i<total_racks;i++)
	{
		sync_socket_tracker[i] = -1;
	}
	//m_socket = new Ptr<Socket>[total_hosts];


	sync_sockets = new Ptr<Socket>[total_racks];

	total_sync_sockets = 0;



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

//void ssUdpEchoClient::deleteCopyinformation(uint32_t chunk_id, uint32_t chunk_location, uint32_t location_from, uint32_t version)
//{
////	/for(std::vector<MultipleCopyOfChunkInfo>::iterator itr = BaseTopology::chunkCopyLocations.begin(); itr != BaseTopology::chunkCopyLocations.end(); ++itr)
//}

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

void ssUdpEchoClient::StartApplication() {
	NS_LOG_FUNCTION(this);

	//NS_LOG_UNCOND("Here I am ");

	if(this->consistency_flow)
	{
		//NS_LOG_UNCOND("This is a consistency flow");

		this->distinct_items = 1;

		m_socket = new Ptr<Socket>[this->distinct_items];

		TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
		m_socket[0] = Socket::CreateSocket(GetNode(), tid);

		if (Ipv4Address::IsMatchingType(m_peerAddress) == true) {
			m_socket[0]->Bind();
			m_socket[0]->Connect(InetSocketAddress(Ipv4Address::ConvertFrom(m_peerAddress), m_peerPort));
		}

		m_socket[0]->SetRecvCallback(MakeCallback(&ssUdpEchoClient::callback_HandleRead, this));
		m_socket[0]->SetAllowBroadcast(true);

		ScheduleTransmit(Time::FromDouble(CONSISTENCY_FLOW_START_DUARTION_CONSTANT, Time::S));

		return;
	}


	BaseTopology::total_appication++;

	BaseTopology::current_number_of_flows++;

	//NS_LOG_UNCOND("^^^^^^^^^^^^^Current number of flows^^^^^^^^^^^^^^^"<< BaseTopology::current_number_of_flows);

	uint32_t total_hosts_in_pod = (SSD_PER_RACK + 1) * (simulationRunProperties::k/2) * (simulationRunProperties::k/2);

	//ChangePopularity();
	ChangeIntensity();

	//NS_LOG_UNCOND("%^&%&^%&^^&^&^&*^ "<<m_flowRequiredBW);

	uint32_t version;

	int socket_index;

	destination_chunks = new uint32_t[ns3::BaseTopology::chunk_assignment_to_applications[application_index][0]];

	for(uint32_t chunk_no=1;chunk_no<=ns3::BaseTopology::chunk_assignment_to_applications[application_index][0];chunk_no++)
	{
		uint32_t virtual_chunk_number = ns3::BaseTopology::chunk_assignment_to_applications[application_index][chunk_no];

		//NS_LOG_UNCOND(" virtual_chunk_number "<<virtual_chunk_number);
		destination_chunks[chunk_no-1] = ns3::BaseTopology::virtual_to_absolute_mapper[virtual_chunk_number];
		//NS_LOG_UNCOND(" virtual_to_absolute_mapper "<< ns3::BaseTopology::virtual_to_absolute_mapper[virtual_chunk_number]);


		//NS_LOG_UNCOND(" ns3::BaseTopology::virtual_to_absolute_mapper[virtual_chunk_number] "<<ns3::BaseTopology::virtual_to_absolute_mapper[virtual_chunk_number]<<" application_index "<<application_index);
	}
	//NS_LOG_UNCOND("here I am 2");

	for(uint32_t i=1 ; i<= ns3::BaseTopology::chunk_assignment_to_applications[application_index][0];i++)
	{
		uint32_t chunk_value = destination_chunks[i-1];

		uint32_t chunk_location = getChunkLocation(chunk_value, &version, &socket_index);

		//NS_LOG_UNCOND(" chunk_value "<<chunk_value<<" chunk_location "<<chunk_location);

		double bandwidth_distribution = ns3::BaseTopology::chunk_assignment_probability_to_applications[application_index][i] * (double)m_flowRequiredBW;

		Ipv4GlobalRouting::host_utilization[chunk_location] += bandwidth_distribution;

		BaseTopology::host_running_avg_bandwidth[chunk_location] += Ipv4GlobalRouting::host_utilization[chunk_location];

		BaseTopology::host_running_avg_counter[chunk_location]++;
		//////Update the struct value///////////////

		//NS_LOG_UNCOND(" chunk_location "<<chunk_location);


		uint32_t pod = (uint32_t) floor((double) chunk_location/ (double) total_hosts_in_pod);

		uint32_t node = ((chunk_location - 1)/(SSD_PER_RACK + 1)) % Ipv4GlobalRouting::FatTree_k;

		//NS_LOG_UNCOND(" Pod value "<<pod);

		//NS_LOG_UNCOND(" Node value "<<node);

		//if((int) pod >= Ipv4GlobalRouting::FatTree_k) NS_LOG_UNCOND("Pod is greater than k");

		BaseTopology::p[pod].nodes[node].utilization += bandwidth_distribution;

		for(uint32_t chunk_index = 0 ;chunk_index < BaseTopology::p[pod].nodes[node].total_chunks;chunk_index++)
		{
			if(BaseTopology::p[pod].nodes[node].data[chunk_index].chunk_number == chunk_value)
			{
				BaseTopology::p[pod].nodes[node].data[chunk_index].intensity_sum += bandwidth_distribution;
				BaseTopology::p[pod].nodes[node].data[chunk_index].processed=0;
			}
		}
	}

	//NS_LOG_UNCOND("here I am 2");

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

	//NS_LOG_UNCOND("# of distinct dest "<<distinct_items);

	for (std::vector<MappingSocket>::iterator it = socket_mapping.begin() ; it != socket_mapping.end(); ++it)
	{
		//NS_LOG_UNCOND(it->location<<" "<<it->socket_id);
	}

	m_socket = new Ptr<Socket>[distinct_items+1];



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
			 //printf("here 88888888888888888888888888888888888------%f\n",BaseTopology::p[i].nodes[j].utilization);
			 BaseTopology::p[i].Pod_utilization=BaseTopology::p[i].Pod_utilization+BaseTopology::p[i].nodes[j].utilization;
		  }
	}


	/********Uncomment it when function ReturnSomething is ready */

	if(BaseTopology::Incrcounter_==0)
	{
		BaseTopology::Incrcounter_=0;
		int incrDcr=1;

		BaseTopology::calculateNewLocation(incrDcr);

		int i=0;

		while(BaseTopology::res[i].src != 99999)// && BaseTopology::res!=NULL)
		{
			printf("++++++++++++++++++++++++++++\n");
			NS_LOG_UNCOND(BaseTopology::res[i].chunk_number);



			BaseTopology::chunkTracker.at(BaseTopology::res[i].chunk_number).number_of_copy++;

			uint32_t num_of_packets_to_send = BaseTopology::chunk_version_tracker[BaseTopology::res[i].chunk_number] - BaseTopology::chunk_version_node_tracker[BaseTopology::res[i].chunk_number][BaseTopology::res[i].dest];
			if(num_of_packets_to_send > 0) BaseTopology::InjectANewRandomFlowCopyCreation (BaseTopology::res[i].src, BaseTopology::res[i].dest, num_of_packets_to_send);


			BaseTopology::host_assignment_round_robin_counter[BaseTopology::res[i].dest]++;

			BaseTopology::chunkTracker.at(BaseTopology::res[i].chunk_number).logical_node_id = (SSD_PER_RACK + 1) * BaseTopology::res[i].dest + 1 + (BaseTopology::host_assignment_round_robin_counter[BaseTopology::res[i].dest]%SSD_PER_RACK);


			BaseTopology::chunk_copy_node_tracker[BaseTopology::res[i].chunk_number][BaseTopology::chunkTracker.at(BaseTopology::res[i].chunk_number).logical_node_id] = true;

			//BaseTopology::chunkTracker.at(BaseTopology::res[i].chunk_number).logical_node_id = getHostInfoMadeBypolicy(BaseTopology::res[i].dest);

			NS_LOG_UNCOND(" BaseTopology::chunkTracker.at(BaseTopology::res[i].chunk_number).logical_node_id "<<BaseTopology::chunkTracker.at(BaseTopology::res[i].chunk_number).logical_node_id);

			NS_LOG_UNCOND("The chunk address is "<<BaseTopology::hostaddresslogicaltophysical[BaseTopology::chunkTracker.at(BaseTopology::res[i].chunk_number).logical_node_id]<<" BaseTopology::res[i].dest "<<BaseTopology::res[i].dest);


			uint32_t pod = (uint32_t) floor((double) BaseTopology::res[i].dest/ (double) Ipv4GlobalRouting::FatTree_k);

			uint32_t node = BaseTopology::res[i].dest % Ipv4GlobalRouting::FatTree_k;

			NS_LOG_UNCOND("BaseTopology::res[i].dest "<<BaseTopology::res[i].dest<<" Pod "<<pod<<" Ipv4GlobalRouting::FatTree_k "<<Ipv4GlobalRouting::FatTree_k);

			bool entry_already_exists = false;

			for(uint32_t chunk_index = 0 ;chunk_index < BaseTopology::p[pod].nodes[node].total_chunks;chunk_index++)
			{
				if(BaseTopology::p[pod].nodes[node].data[chunk_index].chunk_number == BaseTopology::res[i].chunk_number)
				{
					entry_already_exists = true;
				}
			}

			if(!entry_already_exists)
			{
				BaseTopology::p[pod].nodes[node].data[BaseTopology::p[pod].nodes[node].total_chunks].chunk_number = BaseTopology::res[i].chunk_number;
				BaseTopology::p[pod].nodes[node].data[BaseTopology::p[pod].nodes[node].total_chunks].intensity_sum = 0.0;

				BaseTopology::p[pod].nodes[node].total_chunks++;
			}

			NS_LOG_UNCOND("src "<<BaseTopology::res[i].src<<" dest "<<BaseTopology::res[i].dest<<" chunk_no "<<BaseTopology::res[i].chunk_number);

			printf("%d %d %d\n", BaseTopology::res[i].src,BaseTopology::res[i].dest,BaseTopology::res[i].chunk_number);
			i++;
		}

	}
	//calling the optimizer


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

	//NS_LOG_UNCOND("here I am 5");
}

void ssUdpEchoClient::ForceStopApplication(void) {
	NS_LOG_FUNCTION(this);
}

void ssUdpEchoClient::StopApplication(void) {
	NS_LOG_FUNCTION(this);

	uint32_t version;

	int socket_index;

	NS_LOG_UNCOND("Here The stop app ");

	if(!consistency_flow) BaseTopology::current_number_of_flows--;

	NS_LOG_UNCOND("^^^^^^^^^^^^^Current number of flows^^^^^^^^^^^^^^^"<< BaseTopology::current_number_of_flows);

	uint32_t total_hosts_in_pod = (SSD_PER_RACK + 1) * (simulationRunProperties::k/2) * (simulationRunProperties::k/2);

// as discussed on Mar 15. Sanjeev (overcome bug)
	if (!m_lastPacket && !consistency_flow) {

		for(uint32_t i=1 ; i<= ns3::BaseTopology::chunk_assignment_to_applications[application_index][0];i++)
		{
			uint32_t chunk_value = destination_chunks[i-1];

			uint32_t chunk_location = getChunkLocation(chunk_value, &version, &socket_index);

			//NS_LOG_UNCOND("The chunk Location is "<<chunk_location<<" "<<chunk_value);

			double bandwidth_distribution = ns3::BaseTopology::chunk_assignment_probability_to_applications[application_index][i] * (double)m_flowRequiredBW;

			Ipv4GlobalRouting::host_utilization[chunk_location] -= bandwidth_distribution;

			BaseTopology::host_running_avg_bandwidth[chunk_location] += Ipv4GlobalRouting::host_utilization[chunk_location];

			BaseTopology::host_running_avg_counter[chunk_location]++;


			uint32_t pod = (uint32_t) floor((double) chunk_location/ (double) total_hosts_in_pod);

			uint32_t node = ((chunk_location - 1)/(SSD_PER_RACK + 1)) % Ipv4GlobalRouting::FatTree_k;

			BaseTopology::p[pod].nodes[node].utilization -= bandwidth_distribution;
			for(uint32_t chunk_index = 0 ;chunk_index < BaseTopology::p[pod].nodes[node].total_chunks;chunk_index++)
			{
				if(BaseTopology::p[pod].nodes[node].data[chunk_index].chunk_number == chunk_value)
				{
					BaseTopology::p[pod].nodes[node].data[chunk_index].intensity_sum -= bandwidth_distribution;
					BaseTopology::p[pod].nodes[node].data[chunk_index].processed=0;



					if(BaseTopology::p[pod].nodes[node].data[chunk_index].intensity_sum < 0.0)
					{
						BaseTopology::p[pod].nodes[node].data[chunk_index].intensity_sum = ceil(BaseTopology::p[pod].nodes[node].data[chunk_index].intensity_sum );
					}
				}
			}


		}
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

		//BaseTopology::counter_++;
		/********Uncomment it when function ReturnSomething is ready */
		if(BaseTopology::counter_==0)
		{
			int incrDcr=0;

			BaseTopology::calculateNewLocation(incrDcr);

			int i=0;

			while(BaseTopology::res[i].src != 99999 && BaseTopology::res!=NULL)
			{
				//BaseTopology::chunk_copy_node_tracker[BaseTopology::res[i].chunk_number][BaseTopology::res[i].src] = false;
				//BaseTopology::chunk_copy_node_tracker[BaseTopology::res[i].chunk_number][BaseTopology::res[i].dest] = true;
				NS_ASSERT_MSG(BaseTopology::chunkTracker.at(BaseTopology::res[i].chunk_number).number_of_copy > 0, " SomeThing Wrong in deletion of the copy ");
				if(BaseTopology::chunkTracker.at(BaseTopology::res[i].chunk_number).number_of_copy > 0)
				{
					uint32_t chunk_location = FindChunkAssignedHost(BaseTopology::res[i].chunk_number, BaseTopology::res[i].src);
						//uint32_t round_robin_counter = BaseTopology::host_assignment_round_robin_counter[BaseTopology::res[i].dest] % SSD_PER_RACK;
					BaseTopology::chunkTracker.at(BaseTopology::res[i].chunk_number).number_of_copy --;
					BaseTopology::chunkTracker.at(BaseTopology::res[i].chunk_number).logical_node_id = (SSD_PER_RACK + 1) * BaseTopology::res[i].dest + 1;

					BaseTopology::chunk_copy_node_tracker[BaseTopology::res[i].chunk_number][chunk_location] = false;
				}

				else
				{
					NS_LOG_UNCOND("----------Something is wrong with deletion of copy-------------");

				}

				i++;

			}
			BaseTopology::counter_=0;
		}



		//calling the optimizer


		//Simulator::Cancel(m_sendEvent);
		SendLastPacket();
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

	//if(this->consistency_flow) NS_LOG_UNCOND(" This is almost end 1");
//	for(uint32_t i=0; i<total_hosts;i++)
//	{
//		m_socket[i]->Close();
//		m_socket[i]->SetRecvCallback(MakeNullCallback<void, Ptr<Socket> >());
//		m_socket[i] = 0;
//	}
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
	//if(this->consistency_flow) NS_LOG_UNCOND(" This is almost end 4");
	local_chunkTracker.clear();
	//if(this->consistency_flow) NS_LOG_UNCOND(" This is almost end 5");
	if(!this->consistency_flow)
	{
		delete[] destination_chunks;

		delete[] sync_socket_tracker;
	}
	//if(this->consistency_flow) NS_LOG_UNCOND(" This is almost end 3");
	socket_mapping.clear();
	//if(this->consistency_flow) NS_LOG_UNCOND(" This is almost end 2");
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

	/**********************************************/


}
//
// added by sanjeev see *.cc file
// sanjeev... add packet tag with packet properties required by JB/MR...
//

void ssUdpEchoClient::Send(void) {

	//NS_LOG_UNCOND("BaseTopology::chunkTracker.at(0).number_of_copy "<<BaseTopology::chunkTracker.at(0).number_of_copy);
	NS_LOG_FUNCTION(this);

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

	chunk_value = single_destination;

	uint32_t version;

	int socket_index;

	uint32_t num_of_packets_to_send = 1;

	bool is_write = false;


	if(consistency_flow)
	{
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
		double inc_prob = 0.0;
		uint32_t index = 0;
		double desired_value = ClientChunkAccessGenerator->GetValue();

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

		if(ReadWriteCalculation->GetValue() > READ_WRITE_RATIO) //this is write request
		{
			num_of_packets_to_send = BaseTopology::chunkTracker.at(chunk_value).number_of_copy + 1;

			NS_LOG_UNCOND("The num_of_packets_to_send "<<num_of_packets_to_send<<" Chunk value "<<chunk_value);

			BaseTopology::total_packet_count += num_of_packets_to_send -1 ;
			is_write = true;
			BaseTopology::chunk_version_tracker[chunk_value]++;
		}
	}

	else
	{
//		single_destination =  (uint32_t) ClientChunkAccessGenerator->GetValue ();
//		single_destination = single_destination - 1;
//		chunk_value = single_destination;

		//nothing to do here
	}



	//very bad structured code, fix it later

	if((!is_write || num_of_packets_to_send == 1) && !consistency_flow)
	{
		BaseTopology::total_non_consistency_packets++;
		dest_value = getChunkLocation(chunk_value, &version, &socket_index);

		BaseTopology::total_packets_to_chunk[chunk_value]++;

		if(is_write)
		{
			uint32_t host_id = dest_value/(SSD_PER_RACK+1);

			//NS_LOG_UNCOND(" dest_value "<<dest_value<<" host_id "<<host_id);
			BaseTopology::chunk_version_node_tracker[chunk_value][host_id]++;
		}


		t_p = createPacket(chunk_value, dest_value, is_write, version);
		// call to the trace sinks before the packet is actually sent,
		// so that tags added to the packet can be sent as well
		m_txTrace(t_p);

		//NS_LOG_UNCOND("I am here 0.1 "<<t_p->sub_flow_dest);

		int actual = m_socket[socket_index]->Send(t_p);

		//NS_LOG_UNCOND("I am here 1");


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
				t_p = createPacket(0, single_destination, is_write, 0);
				m_txTrace(t_p);
				int actual = m_socket[0]->Send(t_p);


				if ((unsigned) actual != m_packetSize) {
					NS_ABORT_MSG(
							"ssUdpEchoClient Node [" << m_node->GetId() << "] error sending packet");
				}
			}

			this->total_packets_to_send = 0;
		}
		else //this is not consistency flow and number of packets to be sent is more than 1
		{

			NS_LOG_UNCOND("&&&&&&&&&&&& num_of_packets_to_send &&&&&&&&&&&&& "<<num_of_packets_to_send<<" Chunk value "<<chunk_value);
			for(uint32_t host_index=0;host_index<total_hosts;host_index++)
			{
				bool sync_traffic = true;
				uint32_t rack_id ;
				if(BaseTopology::chunk_copy_node_tracker[chunk_value][host_index])
				{
					BaseTopology::total_consistency_packets++;
					if(local_chunkTracker.at(chunk_value).node_id == host_index) //there is a socket already opened for it
					{
						sync_traffic = false;
					}
					else
					{
						rack_id = host_index/(SSD_PER_RACK + 1);

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
					t_p = createPacket(chunk_value, host_index, is_write, 0);

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

			BaseTopology::total_consistency_packets = BaseTopology::total_consistency_packets -1;
//			for(std::vector<MultipleCopyOfChunkInfo>::iterator itr = BaseTopology::chunkCopyLocations.begin(); itr != BaseTopology::chunkCopyLocations.end(); ++itr)
//			{
//				if(itr->chunk_id == chunk_value)
//				{
//					t_p = createPacket(chunk_value, itr->location, is_write, itr->version);
//
//					m_txTrace(t_p);
//
//					NS_LOG_UNCOND("The subflow dest id "<<t_p->sub_flow_dest <<" Chunk id is "<<t_p->sub_flow_id);
//
//					int actual = m_socket[t_p->sub_flow_dest]->Send(t_p);
//
//					if ((unsigned) actual != m_packetSize) {
//						NS_ABORT_MSG(
//								"ssUdpEchoClient Node [" << m_node->GetId() << "] error sending packet");
//					}
//					NS_LOG_LOGIC(
//							this << " At time " << Simulator::Now ().GetSeconds () << "s client " << m_srcIpv4Address << " sent flowid " << m_currentFlowCount << " packet# " << t_sentPacketCount << " packetUid [" << t_p->GetUid() << "]");
//
//					t_sentPacketCount++;
//				}
//			}
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


Ptr<Packet> ssUdpEchoClient::createPacket(uint32_t chunk_id, uint32_t chunk_location, bool is_write, uint32_t version) {
	return createPacket(m_currentFlowCount, t_sentPacketCount, m_flowRequiredBW,
			m_lastPacket, UNUSED, (t_sentPacketCount == 0 ? true : false),
			m_flowStartTimeNanoSec, m_priority, m_srcIpv4Address,
			m_dstIpv4Address, chunk_id, chunk_location, is_write, version);
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
		const Ipv4Address &src, const Ipv4Address &dst, uint32_t chunk_id, uint32_t chunk_location, bool is_write, uint32_t version) {
	NS_LOG_FUNCTION(this);

	t_p = Create<Packet>(m_packetSize);

	t_p->sub_flow_id = chunk_id;

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

	t_p->is_phrase_changed = false;

	if(BaseTopology::total_phrase_changed == 3)
	{
		t_p->is_phrase_changed = true;
	}
	//NS_LOG_UNCOND(t_p->srcNodeId);
	if(t_p->sub_flow_dest_physical == t_p->srcNodeId)
	{

		NS_LOG_UNCOND("The dest and src are same "<<t_p->sub_flow_dest_physical);
		exit(0);
//		if (Ipv4GlobalRouting::flow_map.count(t_p->flow_id) <= 0)
//		{
//			flow_info flow_entry  = flow_info();
//			flow_entry.set_flow_id(t_p->flow_id);
//			flow_entry.set_source(m_srcIpv4Address);
//			flow_entry.set_destination(m_dstIpv4Address);
//			flow_entry.set_bandwidth(requiredBW);
//			flow_entry.destination_node = t_p->sub_flow_dest_physical;
//			Ipv4GlobalRouting::flow_map[t_p->flow_id] = flow_entry;
//		}
//		//NS_LOG_UNCOND("t_p->flow_id "<<t_p->flow_id);
//		Ipv4GlobalRouting::flow_map.at(t_p->flow_id).delaysum += DEFAULT_LOCAL_ACCESS_LATENCY;
//		Ipv4GlobalRouting::flow_map.at(t_p->flow_id).total_packet_to_destination++;
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
