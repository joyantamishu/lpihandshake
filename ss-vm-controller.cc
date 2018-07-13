/*
 * ss-vm-controller.cc
 *
 *  Created on: Mar 18, 2017
 *      Author: sanjeev
 */

#include <stdlib.h>
#include "ns3/log.h"
#include "ss-base-topology.h"
#include "ss-fat-tree-topology.h"
#include "ss-device-energy-model.h"
#include "ss-energy-source.h"
#include "ss-udp-echo-client.h"
#include "ss-udp-echo-server.h"
#include "parameters.h"
#include "flow-duration-random-variable-stream.h"
#include "ss-log-file-flags.h"

#include "ns3/ipv4-global-routing.h"

#define HOST_NOT_FOUND					-1

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("VMController");

/*************************************************************/
int BaseTopology::getRandomServerNode(const int &conflictClientNodeId) {
	NS_LOG_FUNCTION(this);
	do {
		t_x = m_randomServerNodeSelector->GetInteger();
	} while (t_x == conflictClientNodeId);
	return t_x;
}

/*************************************************************/
int BaseTopology::getRandomClientNode(void) {
	NS_LOG_FUNCTION(this);
	return m_randomClientNodeSelector->GetInteger();
}

uint32_t BaseTopology::getCustomizedRandomClientNode(uint32_t &application_id)
{
	//NS_LOG_UNCOND("The current application id "<<application_count);

	application_count ++;

	uint32_t total_hosts = hosts.GetN();

	application_id = application_count - 1;
	for (uint32_t i =0;i<total_hosts;i++)
	{
		for(uint32_t index =1; index <=ns3::BaseTopology::application_assignment_to_node[i][0];index++)
		{
			if(application_id == ns3::BaseTopology::application_assignment_to_node[i][index])
			{
				return i;
			}
		}
	}

	return 0;
}

/*************************************************************/
int BaseTopology::getRandomClientNode(const int &reqBW) {
	NS_LOG_FUNCTION(this << reqBW);
#if 0
	if (!simulationRunProperties::enableVMC) {
		// VMC not used, just return a random host...
		return getRandomClientNode();
	}
#endif
	// now do the VMC logic...sanjeev , mar 18.
	int hostId, requiredHost = HOST_NOT_FOUND;
	Ptr<ssNode> node;
	int h, n = hosts.GetN();
	//
#if COMPILE_CUSTOM_ROUTING_CODE
	if (HostWeightList::isInitialized) {
		m_hwlDataMetric.totalSrcRequests++;

		NS_ASSERT_MSG(HostWeightList::hostsCount == n,
				"getRandomClientNode HostWeightList::hostsCount count mismatch " << n << " vs " << HostWeightList::hostsCount);
		// search the recommendation list first and check if the given host satisfies the BW needed
		for (h = 0; h < n; h++) {
			if (Ipv4GlobalRouting::Unifrombandwidth) {
				hostId = HostWeightList::getSourceHostIdNew(reqBW);
				if (hostId == -1) {
					NS_LOG_LOGIC(
							"BaseTopology::getRandomClientNode -->HostWeightList::getSourceHostIdNew(reqBW) returned -1");
					continue;
				}
			} else {
				hostId = HostWeightList::getSourceHostId(h);
			}
			NS_ASSERT_MSG(hostId < n,
					"HostWeightList::hostsCount returned illegal hostId{" << hostId << "] vs [" << n << "]");
			node = DynamicCast<ssNode>(hosts.Get(hostId));
			NS_ASSERT_MSG((node->nodeType == ssNode::HOST),
					"HostWeightList::getSourceHostId() returned illegal hostId[" << hostId << "] nodeType[" << node->nodeType << "] at index [" << h << "]");
			// check additional constraint,
			// sanjeev apr 3, VMC logic
			if ((node->m_currentAvailableBW_Out >= reqBW)
					&& (node->m_NumberOfApps_Out < node->m_maxFlowPerNode)) {
				m_hwlDataMetric.totalSrcRequestSuccess++;
				requiredHost = hostId;
				NS_LOG_LOGIC(
						"Found required src host, hostId [" << requiredHost << "] nodeId [" << node->GetId() << "]");
				break; // we found the required host node
			}

		} 	// end for
		// VMC enabled & HostWeightList initialized, yet we did not find a solution?
		if (requiredHost == HOST_NOT_FOUND) {
			NS_LOG_LOGIC(
					"VMCController: Source Host NOT found from recommended list for matching BW " << reqBW);
			m_hwlDataMetric.totalSrcRequestFailed++;
			return requiredHost;
		}
		WriteHWLEfficiencyMetricsToFile(requiredHost);
	} else // VMC initialized, HostWeightList is NOT yet ready, find a host matching reqBW.
#endif
	{
		NS_LOG_LOGIC(
				"VMCController: HostList is NOT initialized, trying to find DST host for matching BW " << reqBW);
		// again go thro list, and find a suitable host with matching constraints.
		requiredHost = HOST_NOT_FOUND;
		hostId = HOST_NOT_FOUND;
		m_hwlDataMetric.totalNonHWL_srcRequests++;
		for (h = 0; h < n * ARBITRARY_FORLOOP_NUMBER; h++) {
			// try our random selector for 'n times only...
			hostId = getRandomClientNode();
			node = DynamicCast<ssNode>(hosts.Get(hostId));
			// sanjeev, additional check May 8
			NS_ASSERT_MSG((node->nodeType == ssNode::HOST),
					"getRandomClientNode:: returned wrong host node " << hostId);
			// check additional constraint, (see ssNode:maxAppAllowed also)
			// sanjeev apr 3, VMC logic
			if ((node->m_currentAvailableBW_Out >= reqBW)
					&& (node->m_NumberOfApps_Out < node->m_maxFlowPerNode)) {
				requiredHost = hostId;
				break; // we found the required host node
			} else {
				// sanjeev, debug- SS, May 9th
				NS_LOG_LOGIC(
						"getRandomClientNode returned [" << hostId << "] constrains failed noOfApps_Out [" << node->m_NumberOfApps_Out << "] AvailBW_Out [" << node->m_currentAvailableBW_Out << "] retry again RequiredBW: " << reqBW << "]");
			}

		}  // end 2nd for loop..
		if (requiredHost == HOST_NOT_FOUND) {
			NS_LOG_LOGIC(
					"VMCController: HostList is NOT initialized, failed in finding SRC host for matching BW " << reqBW);
			NS_LOG_LOGIC(
					"VMCController: HostList is NOT initialized, mostly either all nodes exceeded Apps/VM &  VMs/host");
			m_hwlDataMetric.totalNonHWL_srcRequestFailed++;
			// if HOST_NOT_FOUND & currentFlow is less than NoOfHosts -then !! Sanjeev. May 9
			NS_ASSERT_MSG(NUMBER_OF_ACTIVE_FLOWS > n,
					"WRONG getRandomClientNode, at least one CLIENT should be free???");
			// total dst failed without HWL, by Sanjeev:random src host
		}
	} // end else
	return requiredHost;
}

/*************************************************************/
int BaseTopology::getRandomServerNode(const int &conflictClientNodeId,
		const int &reqBW) {
	NS_LOG_FUNCTION(this << reqBW << conflictClientNodeId);
#if 0
	if (!simulationRunProperties::enableVMC) {
		// VMC not used, just return a random host...
		return getRandomServerNode(conflictClientNodeId);
	}
#endif
// now do the VMC logic...sanjeev , mar 18.
	int hostId, requiredHost = HOST_NOT_FOUND;
	Ptr<ssNode> node;
	int h, n = hosts.GetN();
//
#if COMPILE_CUSTOM_ROUTING_CODE
	if (HostWeightList::isInitialized) {
		m_hwlDataMetric.totalDstRequests++;
		NS_ASSERT_MSG(HostWeightList::hostsCount == n,
				"getRandomServerNode HostWeightList::hostsCount count mismatch" << n << " vs " << HostWeightList::hostsCount);
		// search the recommendation list first and check if the given host satisfies the BW needed
		for (h = 0; h < n; h++) {
			if (Ipv4GlobalRouting::Unifrombandwidth) {
				hostId = HostWeightList::getDestinationHostIdNew(conflictClientNodeId, reqBW);
				//hostId = HostWeightList::getDestinationHostIdNewProb(conflictClientNodeId, reqBW);
				if (hostId == -1) {
					NS_LOG_LOGIC(
							"BaseTopology::getRandomServerNode -->HostWeightList::getDestinationHostIdNew(clineNode,reqBW) returned -1");
					continue;
				}
			} else {
				hostId = HostWeightList::getDestinationHostId(h);
			}
			NS_ASSERT_MSG(hostId < n,
					"HostWeightList::getDestinationHostId(h) returned illegal dst hostId[" << hostId << "] vs [" << n << "]");
			node = DynamicCast<ssNode>(hosts.Get(hostId));
			NS_ASSERT_MSG((node->nodeType == ssNode::HOST),
					"HostWeightList::getDestinationHostId(h) returned illegal hostId[" << hostId << "] nodeType[" << node->nodeType << "] at index [" << h << "]");
			// check additional constraint, (see ssNode:maxAppAllowed also)
			// sanjeev apr 3, VMC logic
			if ((node->m_currentAvailableBW_In >= reqBW)
					&& (node->m_NumberOfApps_In < node->m_maxFlowPerNode)
					// from MR, Apr 4th
					&& (hostId != conflictClientNodeId)) {
				m_hwlDataMetric.totalDstRequestSuccess++;
				requiredHost = hostId;
				NS_LOG_LOGIC(
						"Found required dst host, hostId [" << requiredHost << "] nodeId [" << node->GetId() << "]");
				break; // we found the required host node
			}
		} // end for
		  // VMC enabled & HostWeightList initialized, yet we did not find a solution?
		if (requiredHost == HOST_NOT_FOUND) {
			NS_LOG_LOGIC(
					"VMCController: Destination Host NOT found from recommended list for matching BW " << reqBW);
			m_hwlDataMetric.totalDstRequestFailed++;
			return requiredHost;
		}
		WriteHWLEfficiencyMetricsToFile(requiredHost);
	} else // VMC initialized, HostWeightList is NOT yet ready, find a host matching reqBW.
#endif
	{
		// again go thro list, and find a suitable host with matching constraints.
		NS_LOG_LOGIC(
				"VMCController: HostList is NOT initialized, trying to find DST host for matching BW " << reqBW);
		m_hwlDataMetric.totalNonHWL_dstRequests++;
		requiredHost = HOST_NOT_FOUND;
		hostId = HOST_NOT_FOUND;
		for (h = 0; h < n * ARBITRARY_FORLOOP_NUMBER; h++) {
			// try our random selector for 'n times only...
			hostId = getRandomServerNode(conflictClientNodeId);
			node = DynamicCast<ssNode>(hosts.Get(hostId));
			// sanjeev, additional check May 8
			NS_ASSERT_MSG((node->nodeType == ssNode::HOST),
					"getRandomServerNode:: returned wrong host node " << hostId);
			// check additional constraint, (see ssNode:maxAppAllowed also)
			// sanjeev apr 3, VMC logic
			if ((node->m_currentAvailableBW_In >= reqBW)
					& (node->m_NumberOfApps_In < node->m_maxFlowPerNode)) {
				requiredHost = hostId;
				break; // we found the required dst host node
			} else {
				// sanjeev, debug- SS, May 9th
				NS_LOG_LOGIC(
						"getRandomServerNode returned [" << hostId << "] constrains failed, noOfApps_In [" << node->m_NumberOfApps_In << "] AvailBW_In [" << node->m_currentAvailableBW_In << "] retry again RequiredBW: [" << reqBW << "]");
			}
		}  // end 2nd for loop..
		if (requiredHost == HOST_NOT_FOUND) {
			NS_LOG_LOGIC(
					"VMCController: HostList is NOT initialized, failed in finding DST host for matching BW " << reqBW);
			NS_LOG_LOGIC(
					"VMCController: HostList is NOT initialized, mostly either all nodes exceeded Apps/VM &  VMs/host");
			m_hwlDataMetric.totalNonHWL_dstRequestFailed++;
			// if HOST_NOT_FOUND & currentFlow is less than NoOfHosts -then !! Sanjeev. May 9
			NS_ASSERT_MSG(NUMBER_OF_ACTIVE_FLOWS > n,
					"WRONG getRandomServerNode, at least one SERVER should be free???");
		}
	} // end else
	return requiredHost;
}
// Sanjeev May 9th, adjust available & utilized.
/*************************************************************/
bool BaseTopology::markFlowStartedMetric(const uint32_t &notUsedflowId,
		const uint16_t &srcNodeId, const uint16_t &dstNodeId,
		const uint16_t &flowBW) {
	NS_LOG_FUNCTION(this);
	uint32_t nN = allNodes.GetN();
	// sanity check
	NS_ASSERT_MSG(srcNodeId < nN,
			"markFlowStartedMetric, illegal src host [" << srcNodeId << "]");
	NS_ASSERT_MSG(dstNodeId < nN,
			"markFlowStartedMetric, illegal dst host [" << dstNodeId << "]");
	// adjust client parameters
	Ptr<ssNode> node = DynamicCast<ssNode>(allNodes.Get(srcNodeId));
	NS_ASSERT_MSG((node->nodeType == ssNode::HOST),
			"Messed up on src node hostId[" << srcNodeId << "] and nodeType [" << node->nodeType << "]");
	// check additional constraint, (see ssNode:maxAppAllowed also)
	// sanjeev apr 3, VMC logic
	node->m_NumberOfApps_Out++;
	node->m_currentAvailableBW_Out -= flowBW;	// decrease...
	node->m_currentUtilizedBW_Out += flowBW;	// increase...
	// do a sanity check
//	NS_ASSERT_MSG(node->m_currentAvailableBW_Out >= 0,
//			"Messed up on src node [" << srcNodeId << "] m_currentAvailableBW_Out [" << node->m_currentAvailableBW_Out << "] is less than 0");
//	NS_ASSERT_MSG(node->m_currentUtilizedBW_Out <= node->m_maxAvailableBW_Out,
//			"Messed up on src node [" << srcNodeId << "] m_currentUtilizedBW_Out [" << node->m_currentUtilizedBW_Out << "] is more than maxAvailable [" << node->m_maxAvailableBW_Out << "]");
	// adjust server parameters
	node = DynamicCast<ssNode>(allNodes.Get(dstNodeId));
	NS_ASSERT_MSG((node->nodeType == ssNode::HOST),
			"Messed up on dst node hostId[" << dstNodeId << "] and nodeType [" << node->nodeType << "]");
	// sanjeev apr 3, VMC logic
	node->m_NumberOfApps_In++;
	node->m_currentAvailableBW_In -= flowBW;	// decrease...
	node->m_currentUtilizedBW_In += flowBW;	// increase...
	// do a sanity check
	NS_ASSERT_MSG(node->m_currentAvailableBW_In >= 0,
			"Messed up on dst node [" << dstNodeId << "] m_currentAvailableBW_In [" << node->m_currentAvailableBW_In << "] is less than 0");
	NS_ASSERT_MSG(node->m_currentUtilizedBW_Out <= node->m_maxAvailableBW_In,
			"Messed up on dst node [" << dstNodeId << "] m_currentUtilizedBW_In [" << node->m_currentUtilizedBW_In << "] is more than maxAvailable [" << node->m_maxAvailableBW_In << "]");

	return true;
}
/*************************************************************/
bool BaseTopology::markFlowStoppedMetric(const uint32_t &flowId,
		const uint16_t &srcNodeId, const uint16_t &dstNodeId,
		const uint16_t &flowBW) {
	NS_LOG_FUNCTION(this);
//These two lines has been added by madhurima on July 21
#if COMPILE_CUSTOM_ROUTING_CODE
	if (Ipv4GlobalRouting::is_enable_GA && Ipv4GlobalRouting::Unifrombandwidth && ns3::HostWeightList::isInitialized )
	{

	ns3::HostWeightList::stopflow(flowBW,srcNodeId,dstNodeId);

	}
#endif
	m_totalStoppedFlowCounter++;// Apr 17, increment only after flow last packet reaches destination
	uint32_t nN = allNodes.GetN();
	// sanity check
	NS_ASSERT_MSG(srcNodeId < nN,
			"markFlowStoppedMetric, illegal src host [" << srcNodeId << "]");
	NS_ASSERT_MSG(dstNodeId < nN,
			"markFlowStoppedMetric, illegal dst host [" << dstNodeId << "]");
	NS_ASSERT_MSG(flowId <= (uint32_t )m_flowCount,
			"markFlowStoppedMetric, illegal Flow Id [" << flowId << "]");

	Ptr<ssNode> node = DynamicCast<ssNode>(allNodes.Get(srcNodeId));
	NS_ASSERT_MSG((node->nodeType == ssNode::HOST),
			"markFlowStoppedMetric, illegal SRC node [" << srcNodeId << "] and nodeType [" << node->nodeType << "]");

// sanjeev apr 3, VMC logic
// found the correct node with BW, adjust available & utilized.(see ssNode:maxAppAllowed also)
	node->m_NumberOfApps_Out--;
	node->m_currentAvailableBW_Out += flowBW;		// increase...
	node->m_currentUtilizedBW_Out -= flowBW;		// decrease...

// do a sanity check
//	NS_ASSERT_MSG(node->m_currentAvailableBW_Out <= node->m_maxAvailableBW_Out,
//			"Messed up on src host [" << srcNodeId << "] m_currentAvailableBW_Out [" << node->m_currentAvailableBW_Out << "] is more than maxAvailable [" << node->m_maxAvailableBW_Out << "]");
//	NS_ASSERT_MSG(node->m_currentUtilizedBW_Out >= 0,
//			"Messed up on src host [" << srcNodeId << "] m_currentUtilizedBW_Out [" << node->m_currentUtilizedBW_Out << "] is less than 0");

	node = DynamicCast<ssNode>(allNodes.Get(dstNodeId));
	NS_ASSERT_MSG((node->nodeType == ssNode::HOST),
			"markFlowStoppedMetric, illegal DST node [" << srcNodeId << "] and nodeType [" << node->nodeType << "]");

// sanjeev apr 3, VMC logic
// found the correct node with BW, adjust available & utilized...(see ssNode:maxAppAllowed also)
	node->m_NumberOfApps_In--;
	node->m_currentAvailableBW_In += flowBW;		// decrease...
	node->m_currentUtilizedBW_In -= flowBW;		// increase...

// do a sanity check
//	NS_ASSERT_MSG(node->m_currentAvailableBW_In <= node->m_maxAvailableBW_In,
//			"Messed up on dst host [" << dstNodeId << "] m_currentAvailableBW_In [" << node->m_currentAvailableBW_In << "] is more than maxAvailable [" << node->m_maxAvailableBW_In << "]");
//	NS_ASSERT_MSG(node->m_currentUtilizedBW_In >= 0,
//			"Messed up on dst host [" << dstNodeId << "] m_currentUtilizedBW_In [" << node->m_currentUtilizedBW_In << "] is less than 0");

	// capture metrics, MR Apr 28
	if (WRITE_FLOW_DURATION_TO_FILE) {
		const char* modelState = "Flow Stopped";
		// write flow left/stopped to file
		WriteFlowDurationToFile(flowId, 0, flowBW, 0, modelState, 0, 0);
	}
	NS_LOG_LOGIC(
			"Release resource flow [" << flowId << "] at dst [" << dstNodeId << "] coming from node [" << srcNodeId << "] time [" << Simulator::Now().ToDouble(Time::MS) << "ms] flowBW " << flowBW);
	return true;
}

/*************************************************************/
void BaseTopology::WriteVMPlacementMetricsToFile(void) {
	NS_LOG_FUNCTION(this);
// sanjeev apr 3, VMC logic
	if (fpVMPlacement == NULL)
		return;
	int n = hosts.GetN();
	int totalSrcBW = 0;
	int totalDstBW = 0;
	int totalSrcAppOut = 0;
	int totalDstAppIn = 0;
	fpVMPlacement << Simulator::Now().ToDouble(Time::US) << "," << m_flowCount
			<< ",";
	for (int a = 0; a < n; a++) {
		fpVMPlacement << DynamicCast<ssNode>(hosts.Get(a))->m_NumberOfApps_Out
				<< ","
				<< DynamicCast<ssNode>(hosts.Get(a))->m_currentUtilizedBW_Out
				<< "," << DynamicCast<ssNode>(hosts.Get(a))->m_NumberOfApps_In
				<< ","
				<< DynamicCast<ssNode>(hosts.Get(a))->m_currentUtilizedBW_In
				<< ",";
		totalSrcBW +=
				DynamicCast<ssNode>(hosts.Get(a))->m_currentUtilizedBW_Out;
		totalDstBW += DynamicCast<ssNode>(hosts.Get(a))->m_currentUtilizedBW_In;
		totalSrcAppOut += DynamicCast<ssNode>(hosts.Get(a))->m_NumberOfApps_Out;
		totalDstAppIn += DynamicCast<ssNode>(hosts.Get(a))->m_NumberOfApps_In;
	}
	// sanjeev sanity check added. May 9th/ Src & Dst placement metrics should match
	NS_ASSERT_MSG(totalSrcBW == totalDstBW,
			"WriteVMPlacementMetricsToFile messed up, totalSrcBW != totalDstBW [" << totalSrcBW << " != "<< totalDstBW << "]");
	NS_ASSERT_MSG(totalSrcAppOut == totalDstAppIn,
			"WriteVMPlacementMetricsToFile messed up, totalSrcAppOut != totalDstAppIn [" << totalSrcAppOut << " != " << totalDstAppIn << "]");

	double avgSrcBW_per_flow = totalSrcBW / totalSrcAppOut;
	double avgDstBW_per_flow = totalDstBW / totalDstAppIn;
	fpVMPlacement << totalSrcBW << "," << totalDstBW << "," << totalSrcAppOut
			<< "," << totalDstAppIn << ",";
	fpVMPlacement << avgSrcBW_per_flow << "," << avgDstBW_per_flow << ","
			<< avgSrcBW_per_flow / n << "," << avgDstBW_per_flow / n << "\n";
}

/*************************************************************/
void BaseTopology::WriteHWLEfficiencyMetricsToFile(
		const uint16_t &currentSelectedNode) {
	NS_LOG_FUNCTION(this);
// "Time(us),FlowId,SrcRequest,SrcSuccess,DstRequest,DstSuccess,PercentSrcSuccess,PercentDstSuccess\n";
	if (fpVMCEff == NULL)
		return;
	fpVMCEff << Simulator::Now().ToDouble(Time::US) << "," << m_flowCount << ","
			<< currentSelectedNode << "," << m_hwlDataMetric.totalSrcRequests
			<< "," << m_hwlDataMetric.totalSrcRequestSuccess << ","
			<< m_hwlDataMetric.totalDstRequests << ","
			<< m_hwlDataMetric.totalDstRequestSuccess << ","
			<< (m_hwlDataMetric.totalSrcRequests > 0 ?
					(m_hwlDataMetric.totalSrcRequestSuccess * 100
							/ m_hwlDataMetric.totalSrcRequests) :
					0) << ","
			<< (m_hwlDataMetric.totalDstRequests > 0 ?
					(m_hwlDataMetric.totalDstRequestSuccess * 100
							/ m_hwlDataMetric.totalDstRequests) :
					0) << "\n";
}



}  // namespace

