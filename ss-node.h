/*
 * ssNode.h
 *
 *  Created on: Jul 25, 2016
 *      Author: sanjeev
 */

#ifndef SSNODE_H_
#define SSNODE_H_

#include <ns3/node.h>
#include <ns3/event-id.h>
#include <ns3/nstime.h>

namespace ns3 {

struct NodeDataCollected {
	uint64_t m_totalSleepTimeNanoSec;
	uint64_t m_totalReadyTimeNanoSec;
	uint64_t m_totalDeviceTimeNanoSec;
	Time m_totalFabricWakeUpTimeAdded_displayOnly;
	int m_countSleepState;
};
// forward declaration...
class BasicEnergySource;

class ssNode: public Node {
public:
	static TypeId GetTypeId(void);
	ssNode();
	virtual ~ssNode();
	// Attributes - for now all are public....to make it simple to use..
	char *nodeName;
	uint8_t nodeType;
	uint8_t rColor;
	uint8_t gColor;
	uint8_t bColor;
	int i, m_NumberOfApps_In, m_NumberOfApps_Out;
	// I need to make these public for fast access, repeatedly without stacking func_calls
	// bandwidth statistics all in MBs...sanjeev..
	// bandwidth available, currentlyUtlized, and if overflow -how much as maxDemandedBW?
	int m_currentUtilizedBW_In, m_maxAvailableBW_In, m_currentAvailableBW_In;
	int m_currentUtilizedBW_Out, m_maxAvailableBW_Out, m_currentAvailableBW_Out;
	int m_maxFlowPerNode;

	enum NodeType {
		NODE, CORE, AGGR, EDGE, HOST, SERVER, CLIENT
	};
	enum TxNodeFabricState {
		INIT, SLEEP,		// sanjeev
		READY, /**< The transmitter is ready to begin transmission of a packet */
		BUSY /**< The transmitter is busy transmitting a packet */
	};
	/**
	 * The state of the Net Device transmit state machine.
	 */
	TxNodeFabricState m_nodeState;
	Ptr<BasicEnergySource> ptr_source;		// to make netanim life easy...

	//	<May 26> Node Fabric
	EventId m_nodeStateEvent;
	bool *m_deviceSleepStates;
	uint64_t m_lastUpdateTimeNanoSec;
	uint32_t m_NumberOfNetDevices;				// sanjeev May26
	Time m_nodeRunwayTime;
	Time m_nodeFabricWakeUpTime;
	NodeDataCollected m_NodeDataCollected;
	//
	void ComputeLatestStateDurationCalculations(void);		// sanjeev Jun 13, rename the method
	bool IsFabricAsleep(void);
	void setNodeType(int n);
	void MonitorNodeRunway(void);			// sanjeev May 26
	double GetNodeFabricEnergy(void);		//sanjeev Jun 13
	void SetDeviceState(const uint32_t &deviceIndex, const bool &isSleepState);
	virtual Time GetNodeFabricStateAdditionalTime(void);
	virtual Time GetNodeFabricStateAdditionalTimeForThisPacket(const uint32_t &flowId, const uint32_t &packetId);
	void SetEnergySource(Ptr<BasicEnergySource> s);
	void WriteStateDurationEnergyGranular(std::ostream* fpLinkState,
			std::ostream* fpLinkEnergy, std::ostream* fpLinkDuration);
	Ipv4Address GetNodeIpv4Address(void);
	// get Channel dataRate
	void InitializeNode(void);
	virtual void DoDispose(void);
	virtual void WriteStatisticsToFile(void);

};

} /* namespace ns3 */

#endif /* SSNODE_H_ */
