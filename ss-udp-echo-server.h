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
#ifndef SS_ECHO_UDP_SERVER_H
#define SS_ECHO_UDP_SERVER_H

#include <stdint.h>
#include "ns3/socket.h"
#include "ns3/udp-echo-server.h"
#include "ns3/udp-echo-helper.h"
#include "ns3/node-container.h"
#include "ss-node.h"

namespace ns3 {

/*
 * COPIED FROM UdpEchoServerHelper
 * \ingroup udpecho
 * \brief Create a server application which waits for input UDP packets
 *        and sends them back to the original sender.
 */
class ssUdpEchoServerHelper {
public:
	/**
	 * Create UdpEchoServerHelper which will make life easier for people trying
	 * to set up simulations with echos.
	 *
	 * \param port The port the server will wait on for incoming packets
	 */
	ssUdpEchoServerHelper(uint16_t port);
	void SetAttribute(std::string name, const AttributeValue &value);

	ApplicationContainer Install(Ptr<Node> node) const;
	ApplicationContainer Install(NodeContainer nodeC) const;
protected:
	Ptr<Application> InstallPriv(Ptr<Node> node) const;
	ObjectFactory m_factory; //!< Object factory.

};

/**
 * \ingroup udpecho
 * \brief A Udp Echo server
 *
 * Every packet received is sent back.
 */
class ssUdpEchoServer: public UdpEchoServer {
public:
	/**
	 * \brief Get the type ID.
	 * \return the object TypeId
	 */
	static TypeId GetTypeId(void);
	ssUdpEchoServer();
	~ssUdpEchoServer();
	/**
	 * \brief Handle a packet reception.
	 *
	 * This function is called by lower layers.
	 *
	 * \param socket the socket the packet was received to.
	 */
	void HandleRead(Ptr<Socket> socket);
	// Apr 24
	static void writeFlowDataToFile(const double &timeNow,
			const int &flowNumber, const ssNode::NodeType nodeType,
			const bool &isFirst, const bool &isLast, const int &packetNumber,
			const int &nodeId, const Ipv4Address &nodeAddress,
			const double &flowDuration, const double & flowBW);
	// param device a pointer to the topology class which will decrement the flow BW on src & dst
	// Sanjeev 4/17...	markFlowStoppedMetric()
	typedef Callback<bool, const uint32_t &, const uint16_t &, const uint16_t &,
			const uint16_t &> decrementFlowMetric_CallbackSS;

	// this should come after the declaration of typedef...
	virtual void RegisterDecrementFlowMetric_CallbackSS(
			decrementFlowMetric_CallbackSS cbSS);

protected:
	virtual void StartApplication(void);
	virtual void StopApplication(void);
	Ipv4Address tempIPv4;
	bool t_tagFound;

	decrementFlowMetric_CallbackSS m_decrementFlowMetric_CallbackSS;

};

} // namespace ns3

#endif /* SS_ECHO_UDP_SERVER_H */
