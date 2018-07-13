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
#include "parameters.h"
/*
 flow bandwidth -  80-240MBps / avg 160...
 e.g.100MBps with packetsize of 1500, 8 bits/bytegives me --> packets/sec, interpacketTime....XYZ
 exponentialDstr- Mean=XYZ, UpperBound=<don't care>
 */

namespace ns3 {
int ssUdpEchoClient::flows_dropped = 0;

#define HOST_NODE_START_POINT				20
#define NUMBER_OF_HOST_VARIANCE_SUPPORTED 	16

#define USE_GAMMA_DISTRIBUTION				true

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

ApplicationContainer ssUdpEchoClientHelper::Install(Ptr<Node> node, bool single_destination_flow, uint32_t source_node, uint32_t app_id) const {
	return ApplicationContainer(InstallPriv(node,single_destination_flow, source_node, app_id));
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

Ptr<Application> ssUdpEchoClientHelper::InstallPriv(Ptr<Node> node, bool single_destination_flow, uint32_t source_node, uint32_t app_id) const {
	Ptr<ssUdpEchoClient> app = m_factory.Create<ssUdpEchoClient>();

	NS_LOG_UNCOND("The source node is "<<source_node);

	app->node_index = source_node;



	NS_LOG_UNCOND("The random_application_id is "<<app_id);

	//NS_LOG_UNCOND("ns3::BaseTopology::application_assignment[source_node][0] "<<ns3::BaseTopology::application_assignment[source_node][0]);

	app->application_index = app_id;


	app->consistency_flow = single_destination_flow;

	app->count_for_index = 0;

	app->next_counter = 0;



	std::FILE *fp;

	char str[1000];
	double timestamp;
	double response_time;
	char io_type;
	int LUN;
	uint64_t offset;
	uint32_t size;

	uint32_t local_counter = 0;

	char assigned_sub_trace_file[15];

	sprintf(assigned_sub_trace_file,"application_%d.csv",app_id);

	fp = fopen(assigned_sub_trace_file, "r");

	if (fp == NULL){
			printf("Could not open file %s",assigned_sub_trace_file);
			return NULL;
	}


	while (fgets(str, 1000, fp) != NULL)
	{
		sscanf(str,"%lf,%lf,%c,%d,%lu,%d",&timestamp,&response_time, &io_type,&LUN,&offset,&size);

		if((int)strlen(str) > 1)
		{
			app->currentflowinfo[local_counter].next_schedule_in_ms = timestamp;
			app->currentflowinfo[local_counter].chunk_id = (offset-BaseTopology::min_offset)/CHUNK_SIZE;
			app->currentflowinfo[local_counter].size = size;
			local_counter++;

			if(local_counter>=3000) break;

		}
	}

	fclose(fp);

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
	fixed_dest = false;
	single_destination = -1;
	node_index = -1;
	application_index = -1;

	next_counter = 0;

	//count_for_index = 0;

	for (std::vector<chunk_info>::iterator it = BaseTopology::chunkTracker.begin() ; it != BaseTopology::chunkTracker.end(); ++it)
	{
		uint32_t chunk_location = it->logical_node_id;

		local_chunkTracker.push_back(local_chunk_info(it->chunk_no, chunk_location, it->version_number, it->node_id));
	}

	// initialization only, will be overridden at StartApplication()
	m_randomVariableInterPacketInterval = NULL;
	m_startNewFlow_CallbackSS.Nullify();
	total_hosts = (simulationRunProperties::k * simulationRunProperties::k * simulationRunProperties::k)/4;
	m_socket = new Ptr<Socket>[total_hosts];
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
	for(uint32_t i=0; i<total_hosts;i++)
	{
		m_socket[i]->SetRecvCallback(
				MakeCallback(&ssUdpEchoClient::callback_HandleRead, this));
		m_socket[i]->SetAllowBroadcast(true);
	}
}

void ssUdpEchoClient::FlowOperations()
{
//	NS_LOG_FUNCTION(this);
//		/*
//		 * you can overrule m_interval here, for varied inter-packet delay
//		 */
//		char str[1000];
//		double timestamp;
//		double response_time;
//		char io_type;
//		int LUN;
//		uint64_t offset;
//		uint32_t size;
//
//		uint32_t local_counter = 0;
//
//
//		if(this->application_index==15)
//		{
//			NS_LOG_UNCOND("%%%%%%%%%%%^$$$^^^^^$$$$$$$$$The count value "<<count_for_index<<" "<<this->next_counter);
//		}
//		if(count_for_index % 100 == 0)
//		{
//			if(this->application_index==15) NS_LOG_UNCOND("The count value "<<count_for_index);
//			while (fgets(str, 1000, fp) != NULL)
//			{
//				sscanf(str,"%lf,%lf,%c,%d,%lu,%d",&timestamp,&response_time, &io_type,&LUN,&offset,&size);
//
//				if((int)strlen(str) > 1)
//				{
//					if(local_counter >=count_for_index || count_for_index == 0 )
//					{
//						//CurrentFlowInfo cfi = CurrentFlowInfo(timestamp, offset-BaseTopology::min_offset, size);
//						currentflowinfo[local_counter%100].next_schedule_in_ms = timestamp;
//
//						if(this->application_index==15) NS_LOG_UNCOND(" timestamp "<<timestamp);
//						currentflowinfo[local_counter%100].chunk_id = offset-BaseTopology::min_offset;
//						currentflowinfo[local_counter%100].size = size;
//						local_counter++;
//						next_counter++;
//					}
//
//					if((next_counter - count_for_index) >=100)
//					{
//						NS_LOG_UNCOND("Here I am "<<(next_counter - count_for_index)<<" App_id "<<this->application_index);
//						break;
//					}
//
//					local_counter++;
//
//
//				}
//			}
//		}
}

void ssUdpEchoClient::ScheduleTransmit(Time dt) {


	double interval = currentflowinfo[count_for_index].next_schedule_in_ms;

//	if(this->application_index == 15)
//	{
//		NS_LOG_UNCOND("^^^^^^^^^^^^$#$#%%%%%%%%%%%%%%The count_for_index is "<<count_for_index<<" The interval "<<interval);
//	}

	m_packetInterval = Time::FromDouble(interval, Time::S);

	m_sendEvent = Simulator::Schedule(dt, &ssUdpEchoClient::Send, this);
}

void ssUdpEchoClient::StartApplication() {
	NS_LOG_FUNCTION(this);
	BaseTopology::total_appication++;

	uint32_t version;

	destination_chunks = new uint32_t[ns3::BaseTopology::chunk_assignment_to_applications[application_index][0]];

	for(uint32_t chunk_no=1;chunk_no<=ns3::BaseTopology::chunk_assignment_to_applications[application_index][0];chunk_no++)
	{
		uint32_t virtual_chunk_number = ns3::BaseTopology::chunk_assignment_to_applications[application_index][chunk_no];
		destination_chunks[chunk_no-1] = ns3::BaseTopology::virtual_to_absolute_mapper[virtual_chunk_number];
	}


	for(uint32_t i=1 ; i<= ns3::BaseTopology::chunk_assignment_to_applications[application_index][0];i++)
	{
		uint32_t chunk_value = destination_chunks[i-1];

		uint32_t chunk_location = getChunkLocation(chunk_value, &version);

		double bandwidth_distribution = ns3::BaseTopology::chunk_assignment_probability_to_applications[application_index][i] * (double)m_flowRequiredBW;

		Ipv4GlobalRouting::host_utilization[chunk_location] += bandwidth_distribution;

		//////Update the struct value///////////////

		uint32_t pod = (uint32_t) floor((double) chunk_location/ (double) Ipv4GlobalRouting::FatTree_k);

		uint32_t node = chunk_location % Ipv4GlobalRouting::FatTree_k;

		BaseTopology::p[pod].nodes[node].utilization += bandwidth_distribution;
		for(uint32_t chunk_index = 0 ;chunk_index < BaseTopology::p[pod].nodes[node].total_chunks;chunk_index++)
		{
			if(BaseTopology::p[pod].nodes[node].data[chunk_index].chunk_number == chunk_value)
			{

				BaseTopology::p[pod].nodes[node].data[chunk_index].intensity_sum += bandwidth_distribution;
			}
		}



	}

	int incrDcr=1;
	uint32_t src=999,dest=999,chunk_no=999;
	BaseTopology:: calculateNewLocation(incrDcr,&src,&dest,&chunk_no);


	if(src != 999)
	{
		NS_LOG_UNCOND("++++++++");

		BaseTopology::chunkTracker.at(chunk_no).number_of_copy++;
		//NS_LOG_UNCOND("prev BaseTopology::chunkTracker.at(chunk_no).logical_node_id "<<BaseTopology::chunkTracker.at(chunk_no).logical_node_id);

		BaseTopology::chunkTracker.at(chunk_no).logical_node_id = dest;



		//NS_LOG_UNCOND("The src is "<src<<" The dest id "<<dest<<" The chunk no is "<<chunk_no);
		NS_LOG_UNCOND("src "<<src<<" dest "<<dest<<" chunk_no "<<chunk_no);
		//BaseTopology::chunkTracker.at(chunk_no).number_of_copy++;
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

	for(uint32_t i=0; i<total_hosts;i++)
	{
		TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
		m_socket[i] = Socket::CreateSocket(GetNode(), tid);

		if (Ipv4Address::IsMatchingType(m_peerAddress) == true) {
			m_socket[i]->Bind();
			m_socket[i]->Connect(InetSocketAddress(BaseTopology::hostTranslation[i], m_peerPort));
		}
//		else if (Ipv6Address::IsMatchingType(m_peerAddress) == true) {
//			m_socket[i]->Bind6();
//			m_socket[i]->Connect(
//					Inet6SocketAddress(BaseTopology::hostTranslation[i],
//							m_peerPort));
//		}
	}

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

	//fclose(fp);

// as discussed on Mar 15. Sanjeev (overcome bug)
	if (!m_lastPacket) {

		for(uint32_t i=1 ; i<= ns3::BaseTopology::chunk_assignment_to_applications[application_index][0];i++)
		{
			uint32_t chunk_value = destination_chunks[i-1];

			uint32_t chunk_location = getChunkLocation(chunk_value, &version);

			//NS_LOG_UNCOND("The chunk Location is "<<chunk_location<<" "<<chunk_value);

			double bandwidth_distribution = ns3::BaseTopology::chunk_assignment_probability_to_applications[application_index][i] * (double)m_flowRequiredBW;

			Ipv4GlobalRouting::host_utilization[chunk_location] -= bandwidth_distribution;


			uint32_t pod = (uint32_t) floor((double) chunk_location/ (double) Ipv4GlobalRouting::FatTree_k);

			uint32_t node = chunk_location % Ipv4GlobalRouting::FatTree_k;

			BaseTopology::p[pod].nodes[node].utilization -= bandwidth_distribution;
			for(uint32_t chunk_index = 0 ;chunk_index < BaseTopology::p[pod].nodes[node].total_chunks;chunk_index++)
			{
				if(BaseTopology::p[pod].nodes[node].data[chunk_index].chunk_number == chunk_value)
				{
					BaseTopology::p[pod].nodes[node].data[chunk_index].intensity_sum -= bandwidth_distribution;
				}
			}


		}
		int incrDcr=0;
		uint32_t src=999,dest=999,chunk_no=999;
		BaseTopology:: calculateNewLocation(incrDcr,&src,&dest,&chunk_no);

		if(src != 999)
		{
			NS_LOG_UNCOND("$$$$$$$$$$$$$$$");
			if(BaseTopology::chunkTracker.at(chunk_no).number_of_copy > 0)
			{
					BaseTopology::chunkTracker.at(chunk_no).number_of_copy --;
					BaseTopology::chunkTracker.at(chunk_no).logical_node_id = dest;
			}

			else
			{
				NS_LOG_UNCOND("----------Something is wrong with deletion of copy-------------");
			}

			NS_LOG_UNCOND("src "<<src<<" dest "<<dest<<" chunk_no "<<chunk_no);

		}


		//calling the optimizer


		//Simulator::Cancel(m_sendEvent);
		SendLastPacket();
	}
// appl was already stopped. !!
	NS_ASSERT_MSG(m_socket, "Appl stopped, socket closed");

	for(uint32_t i=0; i<total_hosts;i++)
	{
		m_socket[i]->Close();
		m_socket[i]->SetRecvCallback(MakeNullCallback<void, Ptr<Socket> >());
		m_socket[i] = 0;
	}
	m_socket = NULL;

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
	delete[] destination_chunks;
}



//
// not very elegant - but I will let it run now...
//
void ssUdpEchoClient::SendLastPacket(void) {
	NS_LOG_FUNCTION(this);

	NS_LOG_LOGIC(
			this << " Sending LAST packet at [" << Simulator::Now () << "] packetCount [" << t_sentPacketCount << "] flowCount [" << m_currentFlowCount << "]");
	for(uint32_t i=0; i<total_hosts;i++)
	{
		m_lastPacket = true;
		t_p = createPacket();
		m_txTrace(t_p);
		//NS_LOG_UNCOND("This is the end");
		int actual = m_socket[i]->Send(t_p);
	// We exit this loop when actual < toSend as the send side
	// buffer is full. The "DataTxComplete" callback will pop when
	// some buffer space has freed ip.
		if ((unsigned) actual != m_packetSize) {
			NS_ABORT_MSG(
					"Node [" << m_node->GetId() << "] error sending last packet");
		}
	}

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
}
//
// added by sanjeev see *.cc file
// sanjeev... add packet tag with packet properties required by JB/MR...
//


void ssUdpEchoClient::updateOptimizationVariablesLeavingFlow()
{
	uint32_t chunk_location;
	double byte_to_mbit;

	for(uint32_t i=count_for_index-1;i>=count_for_index-ENTRIES_PER_FLOW;i--)
	{

		byte_to_mbit = 8.0 *(currentflowinfo[i].size/MegabyteToByte);

		chunk_location = currentflowinfo[i].chunk_id % total_hosts;

		uint32_t pod = (uint32_t) floor((double) chunk_location/ (double) Ipv4GlobalRouting::FatTree_k);

		uint32_t node = chunk_location % Ipv4GlobalRouting::FatTree_k;

		BaseTopology::p[pod].nodes[node].utilization -= byte_to_mbit;

		if(i==0) break;

	}
}


void ssUdpEchoClient::updateOptimizationVariablesIncomingFlow()
{
	uint32_t chunk_location;
	double byte_to_mbit;

	for(uint32_t i=count_for_index;i<count_for_index+ENTRIES_PER_FLOW;i++)
	{
		byte_to_mbit = 8.0 *(currentflowinfo[i].size/MegabyteToByte);

		chunk_location = currentflowinfo[i].chunk_id % total_hosts;

		uint32_t pod = (uint32_t) floor((double) chunk_location/ (double) Ipv4GlobalRouting::FatTree_k);

		uint32_t node = chunk_location % Ipv4GlobalRouting::FatTree_k;

		BaseTopology::p[pod].nodes[node].utilization += byte_to_mbit;


	}

}

void ssUdpEchoClient::Send(void) {
	NS_LOG_FUNCTION(this);
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
	//FlowOperations();

	uint32_t chunk_value, dest_value;

	chunk_value = single_destination;

	uint32_t version;

	uint32_t num_of_packets_to_send = 1;

	bool is_write = false;

	uint32_t total_number_of_hosts = (simulationRunProperties::k * simulationRunProperties::k * simulationRunProperties::k)/4;


	uint32_t trace_flow_id;
	//very bad structured code, fix it later

	if(!is_write || num_of_packets_to_send == 1)
	{
		uint32_t chunk_id = this->currentflowinfo[count_for_index].chunk_id ;

		dest_value = chunk_id % total_number_of_hosts;

		uint32_t number_of_packets = (uint32_t)(this->currentflowinfo[count_for_index].size / m_packetSize);

		trace_flow_id = count_for_index/ENTRIES_PER_FLOW;

		if(trace_flow_id % ENTRIES_PER_FLOW) //start of new flow
		{
			if(trace_flow_id!=0) //this is not the first flow for sure
			{
				updateOptimizationVariablesLeavingFlow();
			}
			updateOptimizationVariablesIncomingFlow();

		}

		for(uint32_t packets=0;packets<number_of_packets;packets++)
		{
			version = 1;
			t_p = createPacket(chunk_value, dest_value, is_write, version);
			// call to the trace sinks before the packet is actually sent,
			// so that tags added to the packet can be sent as well
			m_txTrace(t_p);

			t_p->trace_flow_id = trace_flow_id;



			int actual = m_socket[t_p->sub_flow_dest]->Send(t_p);


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

	}

	else
	{
		//NS_LOG_UNCOND("THe size of BaseTopology::chunkCopyLocations "<<BaseTopology::chunkCopyLocations.size());
		for(std::vector<MultipleCopyOfChunkInfo>::iterator itr = BaseTopology::chunkCopyLocations.begin(); itr != BaseTopology::chunkCopyLocations.end(); ++itr)
		{
			if(itr->chunk_id == chunk_value)
			{
				t_p = createPacket(chunk_value, itr->location, is_write, itr->version);

				m_txTrace(t_p);

				NS_LOG_UNCOND("The subflow dest id "<<t_p->sub_flow_dest <<" Chunk id is "<<t_p->sub_flow_id);

				int actual = m_socket[t_p->sub_flow_dest]->Send(t_p);

				if ((unsigned) actual != m_packetSize) {
					NS_ABORT_MSG(
							"ssUdpEchoClient Node [" << m_node->GetId() << "] error sending packet");
				}
				NS_LOG_LOGIC(
						this << " At time " << Simulator::Now ().GetSeconds () << "s client " << m_srcIpv4Address << " sent flowid " << m_currentFlowCount << " packet# " << t_sentPacketCount << " packetUid [" << t_p->GetUid() << "]");

				t_sentPacketCount++;
			}
		}
		//NS_LOG_UNCOND("THe size of BaseTopology::chunkCopyLocations1 "<<BaseTopology::chunkCopyLocations.size());
	}

	count_for_index++;

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

uint32_t ssUdpEchoClient::getChunkLocation(uint32_t chunk, uint32_t *version)
{
	for (std::vector<local_chunk_info>::iterator it = local_chunkTracker.begin() ; it != local_chunkTracker.end(); ++it)
	{
		if(it->chunk_id == chunk)
		{
			*version = it->version;
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
