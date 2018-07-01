/*
 * ssPointToPointHelper.cc
 *
 *  Created on: Jul 25, 2016
 *      Author: sanjeev
 */

#include "ns3/queue.h"
#include "ns3/abort.h"
#include "ns3/log.h"
#include "ss-point-to-point-helper.h"

#include "ss-base-topology-simulation-variables.h"
#include "ss-trs-netdevice.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("ssPointToPointHelper");

ssPointToPointHelper::ssPointToPointHelper() :
		PointToPointHelper() {
	NS_LOG_FUNCTION(this);
}

ssPointToPointHelper::~ssPointToPointHelper() {
	NS_LOG_FUNCTION(this);
}

NetDeviceContainer ssPointToPointHelper::Install(Ptr<Node> a, Ptr<Node> b) {
	NS_LOG_FUNCTION(this);
	NS_ASSERT_MSG(a != NULL, "Node_A is NULL");
	NS_ASSERT_MSG(b != NULL, "Node_B is NULL");
	NS_LOG_LOGIC(
			"ssPointToPointHelper::Install, node [" << a->GetId()<< "] and node [" << b->GetId() << "]");
	NetDeviceContainer container;
	Ptr<PointToPointNetDevice> devA, devB;
	if (simulationRunProperties::enableDeviceTRSManagement) {
		//ENABLE_DEVICE_LPI_MANAGEMENT
		m_deviceFactory.SetTypeId("ns3::ssTRSNetDevice");
		devA = m_deviceFactory.Create<ssTRSNetDevice>();
		devB = m_deviceFactory.Create<ssTRSNetDevice>();
	} else {
		m_deviceFactory.SetTypeId("ns3::ssTOSPointToPointNetDevice");
		devA = m_deviceFactory.Create<ssTOSPointToPointNetDevice>();
		devB = m_deviceFactory.Create<ssTOSPointToPointNetDevice>();
	}
	NS_ASSERT(devA != NULL);
	devA->SetAddress(Mac48Address::Allocate());
	a->AddDevice(devA);
	Ptr<Queue> queueA = m_queueFactory.Create<Queue>();
	devA->SetQueue(queueA);
	NS_ASSERT(devB != NULL);
	devB->SetAddress(Mac48Address::Allocate());
	b->AddDevice(devB);
	Ptr<Queue> queueB = m_queueFactory.Create<Queue>();
	devB->SetQueue(queueB);

	Ptr<PointToPointChannel> channel = 0;
	if (simulationRunProperties::enableLinkEnergyManagement) {
		// ENABLE_LINK_ENERGY_MANAGEMENT
		m_channelFactory.SetTypeId("ns3::ssPointToPointChannel");
		channel = m_channelFactory.Create<ssPointToPointChannel>();
	} else {
		m_channelFactory.SetTypeId("ns3::PointToPointChannel");
		channel = m_channelFactory.Create<PointToPointChannel>();
	}

	devA->Attach(channel);
	devB->Attach(channel);
	container.Add(devA);
	container.Add(devB);
	return container;
}

} // namespace
