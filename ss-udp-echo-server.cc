/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 */

#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/address-utils.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"
#include "ss-base-topology.h"
#include "ss-udp-echo-server.h"
#include "ss-log-file-flags.h"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE("ssUdpEchoServer");
NS_OBJECT_ENSURE_REGISTERED(ssUdpEchoServer);

ssUdpEchoServerHelper::ssUdpEchoServerHelper(uint16_t port) {
	m_factory.SetTypeId(ssUdpEchoServer::GetTypeId());
	SetAttribute("Port", UintegerValue(port));
}

void ssUdpEchoServerHelper::SetAttribute(std::string name,
		const AttributeValue &value) {
	m_factory.Set(name, value);
}

ApplicationContainer ssUdpEchoServerHelper::Install(Ptr<Node> node) const {
	return ApplicationContainer(InstallPriv(node));
}

ApplicationContainer ssUdpEchoServerHelper::Install(NodeContainer c) const {
	Ptr<ssUdpEchoServer> m_server; //!< The last created server application
	ApplicationContainer apps;
	for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i) {
		Ptr<Node> node = *i;
		m_server = m_factory.Create<ssUdpEchoServer>();
		node->AddApplication(m_server);
		apps.Add(m_server);
	}
	return apps;
}

Ptr<Application> ssUdpEchoServerHelper::InstallPriv(Ptr<Node> node) const {
	Ptr<Application> app = m_factory.Create<ssUdpEchoServer>();
	node->AddApplication(app);
	return app;
}
/******************/

TypeId ssUdpEchoServer::GetTypeId(void) {
	static TypeId tid =
			TypeId("ns3::ssUdpEchoServer").SetParent<UdpEchoServer>().SetGroupName(
					"Applications").AddConstructor<ssUdpEchoServer>();
	return tid;
}

ssUdpEchoServer::ssUdpEchoServer() {
	NS_LOG_FUNCTION(this);
	t_tagFound = false;
}

ssUdpEchoServer::~ssUdpEchoServer() {
	NS_LOG_FUNCTION(this);
}

void ssUdpEchoServer::StopApplication(void) {
	NS_LOG_FUNCTION(this);
}

void ssUdpEchoServer::StartApplication(void) {
	NS_LOG_FUNCTION(this);
// call parent method...
	UdpEchoServer::StartApplication();
// overrule the callback functions...to do your custom callback
	m_socket->SetRecvCallback(MakeCallback(&ssUdpEchoServer::HandleRead, this));
	m_socket6->SetRecvCallback(
			MakeCallback(&ssUdpEchoServer::HandleRead, this));
}

void ssUdpEchoServer::HandleRead(Ptr<Socket> socket) {
	NS_LOG_FUNCTION(this << socket);
	// if simulation has stopped, then STOP...
	if (Simulator::IsFinished())
		return;

	Ptr<Packet> packet;
	Address from;
	while ((packet = socket->RecvFrom(from))) {
		if (InetSocketAddress::IsMatchingType(from)) {
			NS_LOG_INFO(
					"At time " << Simulator::Now ().GetSeconds () << "s server " << GetNode()->GetId() << " received " << packet->GetSize () << " bytes from " << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " << InetSocketAddress::ConvertFrom (from).GetPort () << " flowid [" << packet->flow_id << "] packetid [" << packet->packet_id << "] packetUid ["<< packet->GetUid() << "]");
		} else if (Inet6SocketAddress::IsMatchingType(from)) {
			NS_LOG_INFO(
					"At time " << Simulator::Now ().GetSeconds () << "s server " << GetNode()->GetId() << " received " << packet->GetSize () << " bytes from " << Inet6SocketAddress::ConvertFrom (from).GetIpv6 () << " port " << Inet6SocketAddress::ConvertFrom (from).GetPort ());
		}
		// sanjeev, debug- SS, May 9th
//		NS_ASSERT_MSG(GetNode()->GetId() == packet->dstNodeId,
//				"packet reached wrong destination, packetDstId [" << packet->dstNodeId << "] thisNodeId [" << GetNode()->GetId() <<"]");
		// debug end packet, SS,JB: Mar 30
		if (WRITE_PACKET_TO_FILE)
			ssTOSPointToPointNetDevice::tracePacket(
					Simulator::Now().ToDouble(Time::US), packet->flow_id,
					packet->packet_id, packet->GetUid(), packet->is_Last,
					GetNode()->GetId(), 0,
					DynamicCast<ssNode>(GetNode())->nodeName, "DST",
					packet->required_bandwidth);

		// sanjeev debug...3/25..trace packetinfo at server !!! Apr 24, dont send last 2 params in func()
		if (packet->is_First || packet->is_Last)
			writeFlowDataToFile(Simulator::Now().ToDouble(Time::US),
					packet->flow_id, ssNode::SERVER, packet->is_First,
					packet->is_Last, packet->packet_id, GetNode()->GetId(),
					DynamicCast<ssNode>(GetNode())->GetNodeIpv4Address(), -999,
					packet->required_bandwidth);
		// sanjeev ...4/17..decrement flow metric from server !!!
		if (packet->is_Last && !m_decrementFlowMetric_CallbackSS.IsNull()) {
			SS_APPLIC_LOG(
					"Last packet for flow [" << packet->flow_id << "] at dst [" << GetNode()->GetId() << "] coming from node [" << packet->srcNodeId << "] time [" << Simulator::Now().ToDouble(Time::MS) << "ms] #pkts " << packet->packet_id);
			m_decrementFlowMetric_CallbackSS(packet->flow_id, packet->srcNodeId,
					GetNode()->GetId(), packet->required_bandwidth);
		}
	} 		// end of while loop

}

void ssUdpEchoServer::writeFlowDataToFile(const double &timeNow,
		const int &flowNumber, const ssNode::NodeType nodeType,
		const bool &isFirst, const bool &isLast, const int &packetNumber,
		const int &nodeId, const Ipv4Address &nodeAddress,
		const double &flowDuration, const double &flowBW) {
	// my route trace debug statement....
	// "Time,FlowNumber,Client/Server,PacketNumber,Node";
	int row = flowNumber;
	TRACE_ROUTING("Trace Routing: @time: " << timeNow << "ns " << nodeType
			<< " received FlowNumber " << flowNumber << " PacketNumber " << packetNumber
			<< " First/Last " << isFirst << "/" << isLast
			<< " at Node " << nodeId);
	//
	if (row < MAXFLOWDATA_TO_COLLECT && row >= 0) {
		BaseTopology::m_flowData[row].flowid = flowNumber;
		if (nodeType == ssNode::SERVER) {
			BaseTopology::m_flowData[row].dstNodeid = nodeId;
			BaseTopology::m_flowData[row].dstNodeIpv4 = nodeAddress;
			BaseTopology::m_flowData[row].packetid = packetNumber;
			if (isFirst) {
				BaseTopology::m_flowData[row].packet1_dstTime = timeNow;
			} else if (isLast) {
				BaseTopology::m_flowData[row].packetL_dstTime = timeNow;
			}
		} else if (nodeType == ssNode::CLIENT) {
			BaseTopology::m_flowData[row].srcNodeid = nodeId;
			BaseTopology::m_flowData[row].srcNodeIpv4 = nodeAddress;
			BaseTopology::m_flowData[row].packetid = packetNumber;
			if (isFirst) {
				BaseTopology::m_flowData[row].packet1_srcTime = timeNow;
				BaseTopology::m_flowData[row].flowBandwidth = flowBW;
				BaseTopology::m_flowData[row].flowDuration = flowDuration;
			} else if (isLast) {
				BaseTopology::m_flowData[row].packetL_srcTime = timeNow;
			}
		}
	}
	//
}

void ssUdpEchoServer::RegisterDecrementFlowMetric_CallbackSS(
		decrementFlowMetric_CallbackSS cbSS) {
	NS_LOG_FUNCTION(this);
	m_decrementFlowMetric_CallbackSS = cbSS;
}

/**********************************/

} // namespace ns3
