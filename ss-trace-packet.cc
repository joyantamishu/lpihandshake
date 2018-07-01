/*
 * trace-end-packet.cc
 *
 *  Created on: Mar 28, 2017
 *      Author: sanjeev
 */

#include "ns3/log.h"
#include "ns3/core-module.h"
#include "ss-base-topology-simulation-variables.h"
#include "ss-log-file-flags.h"
#include <fstream>
#include "ss-tos-point-to-point-netdevice.h"

std::ofstream fpLPT; // this is for my debugging only delete later & all its usage
bool fileOpen = false;

namespace ns3 {

// this puts all end-packets into trace file
void ssTOSPointToPointNetDevice::tracePacket(const double &t,
		const uint32_t &flowId, const uint32_t &packetId,
		const uint64_t& packetGUId, const bool &isLastP, const uint32_t &nodeId,
		const uint32_t &currentQlength, const char*nType, const char*nName,
		const uint32_t &bandwidth) {
	// trace only end packets....
	if (!fileOpen) {
		fileOpen = true;
		char* aFileName = (char*) malloc(
				std::strlen(PACKET_DEBUG_FILENAME2)
						+ std::strlen(simulationRunProperties::arg0) + 10);
		std::sprintf(aFileName, "%s_%s_%s.csv", simulationRunProperties::arg0,
		PACKET_DEBUG_FILENAME2,
				(simulationRunProperties::enableCustomRouting ? "TRUE" : "FALSE"));
		fpLPT.open(aFileName);
		NS_ASSERT_MSG(fpLPT != NULL,
				"Failed to open last packet trace file " << aFileName);
		fpLPT
				<< "Time[us],FlowId,PacketId,packetGUId,isLastP,nodeId,nodeType,currentQlength,nodeDetails,bandwidth\n";
	}
	// trace ONLY end packet or all packets if marked YES
	if (isLastP || TRACE_EVERY_PACKET) {
		fpLPT << t << "," << flowId << "," << packetId << ","
				<< packetGUId << "," << isLastP << "," << nodeId << "," << nType
				<< "," << currentQlength << "," << nName << "," << bandwidth << "\n";
	}
}

} // end namespace

