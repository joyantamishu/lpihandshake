/*
 * ss-fat-tree-topology4.cc
 *
 *  Created on: Jul 12, 2016
 *      Author: sanjeev
 */

#include "ns3/log.h"
#include "ss-base-topology.h"
#include "ss-node.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("BaseTopologyD");

/*************************************************************/
void BaseTopology::printDebugInfo1() {
	NS_LOG_FUNCTION(this);
	NS_LOG_UNCOND(
			"no of nodes= " << allNodes.GetN() << ", no of hosts= " << hosts.GetN());
}

/*****************************************************/
void BaseTopology::DebugIpv4Address(void) {
	NS_LOG_FUNCTION(this);
	Ptr<Ipv4> ipv4; // Get Ipv4 instance of the node
	Ipv4Address addr; // Get Ipv4InterfaceAddress of xth interface.
	Ipv4InterfaceAddress a1;
	int a, c, nd, i;
	Ptr<ssNode> n;
	c = allNodes.GetN();
	printDebugInfo1();

	for (a = 0; a < c; a++) {
		n = DynamicCast<ssNode>(allNodes.Get(a));
		NS_ASSERT_MSG(n != NULL, "node is NULL");
		ipv4 = n->GetObject<Ipv4>();
		NS_ASSERT_MSG(ipv4 != NULL, "ipv4 is NULL");
		nd = ipv4->GetNInterfaces();
		// start with 1, 0 is loop back 127.0.0.1
		for (i = 1; i < nd; i++) {
			a1 = ipv4->GetAddress(i, 0);
			addr = a1.GetLocal();
			NS_LOG_UNCOND(
					"Node [" << a << "] [" << n->nodeName << "], ipv4Address [" << i << "] " << addr);
		}
	}
}

/*****************************************************/
char* BaseTopology::getNewIpv4Address(void) {
	char *address = new char[30];
	int ipv4_maxAddress = 255;
	if (ipv4Address.sn4++ >= ipv4_maxAddress) {
		ipv4Address.sn3++;
		ipv4Address.sn4 = 0;
		if (ipv4Address.sn3 >= ipv4_maxAddress) {
			ipv4Address.sn4 = 0;
			ipv4Address.sn3 = 0;
			ipv4Address.sn2++;
		}
		if (ipv4Address.sn2 >= ipv4_maxAddress) {
			NS_ABORT_MSG(
					"ipv4_maxAddress exceeded::" << ipv4Address.sn1 << ":" << ipv4Address.sn2 << ":" << ipv4Address.sn3 << ":" << ipv4Address.sn4);
		}
	}
	// my short-cut to generate IPv4Address
	sprintf(address, "%d.%d.%d.%d", ipv4Address.sn2, ipv4Address.sn3,
			ipv4Address.sn4, 0);
	return address;
}

/**************************************************************/
void BaseTopology::WriteConfiguration() {
	NS_LOG_FUNCTION(this);
	const char *ext2 = "-ns3config-output.txt";
	char* configFileName = (char*) malloc(
			std::strlen(simulationRunProperties::arg0) + std::strlen(ext2) + 3);
	std::sprintf(configFileName, "%s%s", simulationRunProperties::arg0, ext2);

	// Output config store to txt format
	Config::SetDefault("ns3::ConfigStore::Filename",
			StringValue(configFileName));
	Config::SetDefault("ns3::ConfigStore::FileFormat", StringValue("RawText"));
	Config::SetDefault("ns3::ConfigStore::Mode", StringValue("Save"));
	ConfigStore outputConfig;
	outputConfig.ConfigureDefaults();
	//CORE DUMP outputConfig.ConfigureAttributes ();

	NS_LOG_UNCOND(
			"Statistics:: Writing Current Configuration To\t[" << configFileName << "]");
}

/*****************************************************/
void BaseTopology::ReadConfiguration() {

	const char *ext2 = "-config.ini";
	char* configFileName = (char*) malloc(
			std::strlen(simulationRunProperties::arg0) + std::strlen(ext2) + 3);
	std::sprintf(configFileName, "%s%s", simulationRunProperties::arg0, ext2);

	NS_LOG_UNCOND(
			"Statistics:: Reading Configuration From File\t[" << configFileName << "] ***NOT IMPLEMENTED***");

	// these two are for reading /loading the configs??
	/*
	 ConfigStore inputConfig;
	 inputConfig.SetFilename(configFileName);
	 inputConfig.SetFileFormat(ConfigStore::RAW_TEXT);
	 inputConfig.SetMode(ConfigStore::LOAD);
	 inputConfig.ConfigureDefaults();
	 inputConfig.ConfigureAttributes();
	 */
}

} // namespace
