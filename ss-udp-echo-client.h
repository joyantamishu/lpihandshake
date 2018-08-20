/*
 * ss-udp-echo-client.h
 *
 *  Created on: Sep 30, 2016
 *      Author: sanjeev
 */

#ifndef SCRATCH_LPIHANDSHAKE_SS_UDP_ECHO_CLIENT_H_
#define SCRATCH_LPIHANDSHAKE_SS_UDP_ECHO_CLIENT_H_

#include "ns3/core-module.h"
#include "ns3/application-container.h"
#include "ns3/node-container.h"

#include "ns3/ipv4-flow-probe.h"
#include "ns3/ipv4-flow-classifier.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/flow-monitor.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/config.h"

#include "ss-base-topology-simulation-variables.h"

#include "parameters.h"

namespace ns3 {

class local_chunk_info
{
public:
	uint32_t chunk_id;

	uint32_t node_id;

	uint32_t version;

	uint32_t physical_node_number;

	int socket_index;

	local_chunk_info(uint32_t chunk_id, uint32_t node_id, uint32_t version_number, uint32_t physical_node_number)
	{
		this->chunk_id = chunk_id;
		this->node_id = node_id;
		this->version = version_number;

		this->physical_node_number = physical_node_number;

		this->socket_index = -1;
	}
};

class MappingSocket{
public:
	uint32_t socket_id;
	uint32_t location;
	MappingSocket(uint32_t socket_id, uint32_t location)
	{
		this->socket_id = socket_id;
		this->location = location;
	}

};


// copied from UdpEchoClientHelper
class ssUdpEchoClientHelper {
public:
	ssUdpEchoClientHelper(Address ip, uint16_t port);
	ssUdpEchoClientHelper(Ipv4Address ip, uint16_t port);
	virtual ~ssUdpEchoClientHelper();
	void SetAttribute(std::string name, const AttributeValue &value);
	ApplicationContainer Install(Ptr<Node> node, bool single_destination_flow, uint32_t source_node, uint32_t app_id, uint32_t dest_node = -1, uint32_t no_of_packets = 0) const;
	ApplicationContainer Install(NodeContainer c) const;

protected:
	Ptr<Application> InstallPriv(Ptr<Node> node, bool single_destination_flow, uint32_t source_node, uint32_t app_id, uint32_t dest_node = -1, uint32_t no_of_packets = 0) const;
	ObjectFactory m_factory; //!< Object factory.
};

// copied from UdpEchoClient
class ssUdpEchoClient: public Application {
public:
	static TypeId GetTypeId(void);
	ssUdpEchoClient();
	virtual ~ssUdpEchoClient(void);

	virtual void StartApplication(void);
	virtual void StopApplication(void);
	//param device a pointer to the topology class which will inject a new flow
	// for VMC. sanjeev 3/18
	typedef Callback<void, const uint32_t &, const uint16_t &, const uint16_t &,
			const uint16_t &> startNewFlow_CallbackSS;

	// this should come after the declaration of typedef...
	virtual void RegisterStartNewFlow_CallbackSS(startNewFlow_CallbackSS cbSS);
	static int flows_dropped;	// sanjeev, small change, moved variable here.

	/******** Chunk Specific Change ***************/
	uint32_t getChunkLocation(uint32_t chunk, uint32_t *version, int *socket_index);
	bool consistency_flow;
	uint32_t single_destination;
	//uint32_t *possible_dest;

	uint32_t node_index;
	uint32_t application_index;
	uint32_t *destination_chunks;
	std::vector<MappingSocket> socket_mapping;

	uint32_t total_packets_to_send;
	uint32_t distinct_items;
	virtual void ChangePopularity();
	virtual void ChangeIntensity();
	virtual void insertCopyInformation(uint32_t chunk_id, uint32_t chunk_location_to, uint32_t chunk_location_from, uint32_t version);
	virtual void deleteCopyinformation(uint32_t chunk_id, uint32_t chunk_location_to, uint32_t chunk_location_from, uint32_t version);
	/*********************************************/

protected:

	/**
	 * \param cb callback to invoke whenever a flow stops (i.e. app stops) is to be sent
	 *  and must start a new flow
	 */
	virtual void DoDispose(void);
	virtual void RegisterCallBackFunctions(void);

	void SetRemote(Ipv4Address ip, uint16_t port);
	void SetRemote(Ipv6Address ip, uint16_t port);
	void SetRemote(Address ip, uint16_t port);

	void Send(void);
	void SendLastPacket(void);

	virtual void ForceStopApplication(void);
	virtual void ScheduleTransmit(Time dt);
	// simplified, sanjeev 2/25
	virtual Ptr<Packet> createPacket(void);
	virtual Ptr<Packet> createPacket(uint32_t chunk_id, uint32_t chunk_location, bool is_write, uint32_t version);
	virtual Ptr<Packet> createPacket(const uint32_t &flowId,
			const uint32_t &packetId, const uint32_t &required,
			const bool &last1, const appType &dummy2, const bool &isFirstPacket,
			const uint64_t &FlowStartTimeNanoSec,
			const PacketPriority &priority, const Ipv4Address &src,
			const Ipv4Address &dst, uint32_t chunk_id, uint32_t chunk_location, bool is_write, uint32_t version);
	virtual void setFlowVariables(void);
	virtual double GetHostSpecificVariance(void);		// may 14

	// call back fuctions...
	virtual void callback_HandleRead(Ptr<Socket> socket);

	// all flow control & connection variables
	// simplified. sanjeev 2/25
	Ptr<Socket> *m_socket;       //!< Associated socket
	Address m_peerAddress; //!< Remote peer address
	uint16_t m_peerPort; //!< Remote peer port
	uint16_t m_dstHost; //!< Remote destinationhost
	bool m_connected;    //!< True if connected
	TypeId m_tid;          //!< The type of protocol to use.
	// simplified. sanjeev 2/25
	Ptr<RandomVariableStream> m_randomVariableInterPacketInterval;
	EventId m_sendEvent;
	// for VMC. sanjeev 3/18
	startNewFlow_CallbackSS m_startNewFlow_CallbackSS;

	// all packet info transmission variables
	Ptr<Packet> t_p;
	bool m_lastPacket;
	uint32_t m_packetSize;   //!< Limit total number of bytes sent in one packet
	uint32_t t_sentPacketCount; //!< Maximum number of packets the application will send
	uint32_t m_currentFlowCount;  	// current count of the flow execution.
	Time m_packetInterval; 			//!< inter-packet time
	// simplyfied sanjeev 2/25
	uint64_t m_flowStartTimeNanoSec; // flow age since 1st packet...
	uint32_t m_flowRequiredBW;
	PacketPriority m_priority;
	Ipv4Address m_dstIpv4Address;		// destination IP address
	Ipv4Address m_srcIpv4Address;		// source IP address

	// Callbacks for tracing the packet Tx events
	TracedCallback<Ptr<const Packet> > m_txTrace;
	std::ofstream fp2; // this is for my debugging only delete later & all its usage

	/*******Chunk Specific Changes ****************/
	//Ptr<RandomVariableStream> ClientChunkAccessGenerator;
	Ptr<UniformRandomVariable> ClientChunkAccessGenerator;
	Ptr<UniformRandomVariable> ReadWriteCalculation;
	uint32_t total_hosts;
	std::vector<local_chunk_info> local_chunkTracker;
	/**********************************************/

};

}

#endif /* SCRATCH_LPIHANDSHAKE_SS_UDP_ECHO_CLIENT_H_ */
