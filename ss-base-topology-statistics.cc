/*
 * ss-fat-tree-monitor4.cc
 *
 *  Created on: Aug 8, 2016
 *      Author: sanjeev
 */

#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/log.h"
#include "ss-base-topology.h"
#include "ss-node.h"
#include "ss-log-file-flags.h"
#include <ctime>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("BaseTopologyS");

void BaseTopology::CollectAllStatisticsAllNodes(void) {
	NS_LOG_FUNCTION(this);
	UpdateNodeCounters(); // not a problem???...resolved, sanjeev 3/10/17
#if 0
	WriteStateDurationEnergyGranularAllNodesToFile(); // these stats not needed now...
#endif
	Displayns3FlowMonitorStatistics();	// donot skip this step..
	DisplayJBFlowStatistics();
	Writens3FlowMonitorStatisticsToFile();
	WriteCustomFlowStatisticsToFile();
}

void BaseTopology::EnableAllStatisticsAllNodes(void) {
	NS_LOG_FUNCTION(this);
	// collect one final statistics before closing the simulation.!
	UpdateNodeCounters(); // not a problem???...resolved, sanjeev 3/10/17

	EnableFlowMonitorStatisticsAllNodes();
	EnablePcapAllDevices();
	EnableTrackRoutingTable();
}

/**********************************************/
void BaseTopology::DisplayJBFlowStatistics(void) {
#if COMPILE_CUSTOM_ROUTING_CODE
	NS_LOG_FUNCTION(this);
	int flow_count = 0;
	int thresehold_flow = 0;
	double sum_avg_delay;

	FILE * fp_flow;

	fp_flow = fopen ("flow_info.csv","w");
	for (std::map<uint32_t, flow_info>::const_iterator it = Ipv4GlobalRouting::flow_map.begin(); it != Ipv4GlobalRouting::flow_map.end(); ++it) {


		if(it->second.total_packet_to_destination > 0)
		{
			flow_count++;
			//NS_LOG_UNCOND("it->second.delaysum "<<it->second.delaysum);
			fprintf(fp_flow,"%d,%d,%d\n",it->first,it->second.total_packet_to_destination, it->second.bandwidth);

			sum_avg_delay+= (it->second.delaysum / (double)it->second.total_packet_to_destination);

			//NS_LOG_UNCOND("sum_avg_delay "<<sum_avg_delay<<" it->second.total_packet_to_destination "<<it->second.total_packet_to_destination);
		}

		else
		{
			NS_LOG_UNCOND("The Flow id not having any packets "<<it->first);
		}

		if (it->second.has_faced_threshold)
			thresehold_flow++;
	}

	fclose(fp_flow);
	NS_LOG_UNCOND(
			"\nStatistics: JB: The Total number of flow " << flow_count << " The number of dropped flow " << thresehold_flow);

	NS_LOG_UNCOND(
				"\nStatistics: JB: The avg delay of flows " << sum_avg_delay/(double)flow_count<<" Flow Count "<<flow_count<<" sum_avg_delay "<<sum_avg_delay);

	NS_LOG_UNCOND(
					"\nThe number of inconsistent packets "<<BaseTopology::num_of_retried_packets);

	NS_LOG_UNCOND(
						"\nThe number of versions "<<BaseTopology::chunkTracker.at(0).version_number);
	NS_LOG_UNCOND(
							"\nnum_of_retired_packets_specific_node "<<BaseTopology::num_of_retired_packets_specific_node);


	FILE * fp;

	fp = fopen ("application_statistics.csv","w");

	fprintf (fp, "Application,delay,totalFlow,AverageDelay\n");
	for(uint32_t application=0;application<simulationRunProperties::total_applications;application++)
	{
		if(BaseTopology::application_metrics[application].total_flows > 0)
		{
			fprintf (fp, "%d,%lf,%d,%lf\n",application, BaseTopology::application_metrics[application].avg_delay,BaseTopology::application_metrics[application].total_flows,(BaseTopology::application_metrics[application].avg_delay/BaseTopology::application_metrics[application].total_flows) );
			NS_LOG_UNCOND("The delay associated with application "<<application<<" "<<(BaseTopology::application_metrics[application].avg_delay/BaseTopology::application_metrics[application].total_flows));
		}
	}
	fclose (fp);

	NS_LOG_UNCOND("Ipv4GlobalRouting::total_number_of_packets_to_destination "<<Ipv4GlobalRouting::total_number_of_packets_to_destination);

#endif
}

/**************************************************************/
void BaseTopology::EnableTrackRoutingTable() {
	NS_LOG_FUNCTION(this);

	const char *ext3 = NS3_ROUTING_TABLE_FILENAME1;
	char* aFileName = (char*) malloc(
			std::strlen(simulationRunProperties::arg0) + std::strlen(ext3) + 3);
	std::sprintf(aFileName, "%s%s", simulationRunProperties::arg0, ext3);
	if (m_anim) {
		m_anim->EnablePacketMetadata(true);
		m_anim->EnableIpv4RouteTracking(aFileName, Seconds(0),
				Seconds(simulationRunProperties::simStop), Seconds(0.50));
		NS_LOG_UNCOND(
				"Statistics: Routing Table output is at \t[" << aFileName << "]\n");
	}
}

void BaseTopology::WriteStateDurationEnergyGranularAllNodesToFile(void) {
	NS_LOG_FUNCTION(this);
	const char *ext1 = DEVICE_STATE_CHANGE_FILENAME1;
	const char *ext2 = DEVICE_ENERGY_USED_FILENAME1;
	const char *ext3 = DEVICE_STATE_DURATION_FILENAME1;

	char* aFileName = (char*) malloc(
			std::strlen(simulationRunProperties::arg0) + std::strlen(ext3) + 3);
	std::sprintf(aFileName, "%s%s", simulationRunProperties::arg0, ext1);
	std::ofstream fp1;
	fp1.open(aFileName);
	NS_ASSERT_MSG(fp1 != NULL,
			"Failed to get open write file StatesFileName" << aFileName);
	NS_LOG_UNCOND(
			"\nStatistics:: Device State statistics output at \t[" << aFileName << "] ***Caution: This is rotating buffer");

	std::sprintf(aFileName, "%s%s", simulationRunProperties::arg0, ext2);
	std::ofstream fp2;
	fp2.open(aFileName);
	NS_ASSERT_MSG(fp2 != NULL,
			"Failed to get open write file EnergyFileName" << aFileName);
	NS_LOG_UNCOND(
			"Statistics:: Energy statistics output is at \t[" << aFileName << "] ***Caution: This is rotating buffer");

	std::sprintf(aFileName, "%s%s", simulationRunProperties::arg0, ext3);
	std::ofstream fp3;
	fp3.open(aFileName);
	NS_ASSERT_MSG(fp1 != NULL,
			"Failed to get open write file DurationFileName" << aFileName);
	NS_LOG_UNCOND(
			"Statistics:: Durations statistics output is at \t[" << aFileName << "] ***Caution: This is rotating buffer");
	if (simulationRunProperties::enableDefaultns3EnergyModel) {
		// USE_DEFAULT_NS3_ENERGY_MODEL
		fp1 << "UN-USED\n";
		fp2 << "UN-USED\n";
		fp3 << "UN-USED\n";
	} else {
		int nn = allNodes.GetN();
		for (int i = 0; i < nn; i++) {
			t_node = DynamicCast<ssNode>(allNodes.Get(i));
			t_node->WriteStateDurationEnergyGranular(&fp1, &fp2, &fp3);
		}
	}

	fp1.close();
	fp2.close();
	fp3.close();
}

void BaseTopology::EnablePcapAllDevices(void) {
	NS_LOG_FUNCTION(this);
// sanjeev simplified 2/25
	if (simulationRunProperties::k > 4)
		NS_LOG_UNCOND(
				"Statistics:: Packet EnableAsciiAll & EnablePcapAll output disabled, for large flows ***");
	return;

	const char *ext3 = "-inValid.void";
	char* aFileName = (char*) malloc(
			std::strlen(simulationRunProperties::arg0) + std::strlen(ext3) + 3);
	PointToPointHelper p2p;
	std::sprintf(aFileName, "%s-ascii", simulationRunProperties::arg0);
	p2p.EnableAsciiAll(aFileName);
	NS_LOG_UNCOND(
			"Statistics:: All devices ascii output is at \t[" << aFileName << "*.tr]");

	std::sprintf(aFileName, "%s-pcap-", simulationRunProperties::arg0);
	p2p.EnablePcapAll(simulationRunProperties::arg0, true);
	NS_LOG_UNCOND(
			"Statistics:: All devices pcap output is at \t[" << aFileName << "*.pcap]");
}

void BaseTopology::EnableFlowMonitorStatisticsAllNodes(void) {
	NS_LOG_FUNCTION(this);
	//monitor = flowmon.InstallAll();
	//monitor = flowmon.GetMonitor();
}

void BaseTopology::Displayns3FlowMonitorStatistics() {
//	NS_LOG_FUNCTION(this);
//	// get the max packets that can be stored in device buffer queue, for user display
//	// get it from some device, from any node.. I have to collect this statistics somewhere
//	// before objects are destroyed, and display this on screen (Sanjeev Apr 4)
//	Ptr<PointToPointNetDevice> pd = DynamicCast<PointToPointNetDevice>(
//			hosts.Get(0)->GetDevice(1));
//	m_totalTreeEnergy->m_deviceSetQdepth = pd->GetQueue()->GetMaxPackets();
//
//	Ptr<Ipv4FlowClassifier> classifier;
//	std::map<FlowId, FlowMonitor::FlowStats> stats;
//	std::map<FlowId, FlowMonitor::FlowStats>::const_iterator pathStat;
//	Ipv4FlowClassifier::FiveTuple path1;
//	// Sanjeev: MAy 9th, All below code is taken from ns3 webpage given below
//	monitor->CheckForLostPackets();
//	// Tracing, added by martin:
//	// Print per flow statistics
//	// following added from: https://bitbucket.org/mezza84/ns-3-lte-dev/src/18bb8e694652/examples/wireless/wifi-hidden-terminal.cc
//	classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
//
//	stats = monitor->GetFlowStats();
//	NS_ASSERT_MSG(classifier != NULL, "Failed to get classifier");
//
//	double Throughput = 0.0, totalThroughput = 0;
//	double offeredThroughput = 0.0, totalOfferedThroughput = 0;
//	double consumedThroughput = 0.0, totalConsumedThroughput = 0;
//	Time flowLatency = Seconds(0.0), totalLatency = Seconds(0.0);
//	Time meanDelay = Seconds(0.0), totalDelay = Seconds(0.0);
//	Time meanJitter = Seconds(0.0), totalJitter = Seconds(0.0);
//	int noOfFlows = 0;
//
//	SS_FINAL_FLOW_STAT_LOG("Statistics: Start FLOW MONITOR Statistics\n");
//
//	for (pathStat = stats.begin(); pathStat != stats.end(); ++pathStat) {
//		path1 = classifier->FindFlow(pathStat->first);
//
//		// verify if any packets are lost in this flow....bug sanjeev 7 Feb
//		// sanjeev. rearrange- calc. and boundary conditio check Apr 22
//		if (pathStat->second.txPackets != pathStat->second.rxPackets) {
//			//if (pathStat->second.lostPackets > 0) {
////			SS_FINAL_FLOW_STAT_LOG(
////					"\t\t***Warning;: lost packet in flow*** (packets lost=" << pathStat->second.txPackets - pathStat->second.rxPackets << ")");
//			m_totalTreeEnergy->m_lostPacketCount += (pathStat->second.txPackets
//					- pathStat->second.rxPackets);
//			m_totalTreeEnergy->m_FlowsWithLostPacket++;
//			Throughput = offeredThroughput = totalConsumedThroughput = 0;
//			meanDelay = Seconds(0);
//			meanJitter = Seconds(0);
//			flowLatency = Seconds(0);
//		} else if (pathStat->second.rxPackets) {
//			offeredThroughput = pathStat->second.txBytes * 8.0
//					/ (pathStat->second.timeLastTxPacket.GetSeconds()
//							- pathStat->second.timeFirstTxPacket.GetSeconds())
//					/ 1000;
//			consumedThroughput = pathStat->second.rxBytes * 8.0
//					/ (pathStat->second.timeLastRxPacket.GetSeconds()
//							- pathStat->second.timeFirstRxPacket.GetSeconds())
//					/ 1000;
//			Throughput = pathStat->second.rxBytes * 8.0
//					/ (pathStat->second.timeLastRxPacket.GetSeconds()
//							- pathStat->second.timeFirstTxPacket.GetSeconds())
//					/ 1000;
//			meanDelay = pathStat->second.delaySum / pathStat->second.rxPackets;
//			// Apr 22. Sanjeev bug? in meanJitter calc?
//			meanJitter = pathStat->second.jitterSum
//					/ pathStat->second.rxPackets;
//
//			/* Note on this calculation, this includes inter-packet time, when no packet is transmitted
//			 * so gives full flow latency information. This measures 1st txn & last rcv, plus interm-no-packet-times
//			 * Packet level latency is to use totalDelay, this metric seems to be working right.
//			 */
//			flowLatency = (pathStat->second.timeLastRxPacket
//					- pathStat->second.timeFirstTxPacket);
//		}
//		totalThroughput += Throughput;
//		totalConsumedThroughput += consumedThroughput;
//		totalOfferedThroughput += offeredThroughput;
//		totalDelay += meanDelay;
//		totalJitter += meanJitter;
//		totalLatency += flowLatency;
//
//		noOfFlows++;
//
////		SS_FINAL_FLOW_STAT_LOG(
////				"Flow ID: " << pathStat->first << " Src Addr " << path1.sourceAddress << " Dst Addr " << path1.destinationAddress);
////		SS_FINAL_FLOW_STAT_LOG(
////				"\tTx Packets = " << pathStat->second.txPackets << "\tRx Packets = " << pathStat->second.rxPackets << " Throughput:  " << Throughput << " Kbps" << " Mean delay:  " << meanDelay << " Mean jitter: " << meanJitter);
////
////		SS_FINAL_FLOW_STAT_LOG(
////				"\t1st packet txn: " << pathStat->second.timeFirstTxPacket << " rcv:" << pathStat->second.timeFirstRxPacket << " delay: " << pathStat->second.timeFirstRxPacket - pathStat->second.timeFirstTxPacket << ",  last packet txn: " << pathStat->second.timeLastTxPacket << " rcv:" << pathStat->second.timeLastRxPacket << " delay: " << pathStat->second.timeLastRxPacket - pathStat->second.timeLastTxPacket << " delaySum: " << pathStat->second.delaySum);
//
//	} // end of for loop....
//
//	SS_FINAL_FLOW_STAT_LOG("\nStatistics: -End- FLOW MONITOR Statistics\n");
//
//// count averagess...
//	if (noOfFlows) {
//		m_totalTreeEnergy->m_AvgLatency = totalLatency / noOfFlows;
//		m_totalTreeEnergy->m_AvgDelay = totalDelay / noOfFlows;
//		m_totalTreeEnergy->m_AvgJitter = totalJitter / noOfFlows;
//		// convert to Mbps...
//		m_totalTreeEnergy->m_AvgThroughPut = totalThroughput
//				/ (noOfFlows * 1000);
//		m_totalTreeEnergy->m_AvgOfferedThroughPut = totalOfferedThroughput
//				/ (noOfFlows * 1000);
//		m_totalTreeEnergy->m_AvgConsumedThroughPut = totalConsumedThroughput
//				/ (noOfFlows * 1000);
//		m_totalTreeEnergy->m_FlowCount = noOfFlows;
//	}
}

void BaseTopology::Writens3FlowMonitorStatisticsToFile() {
//	NS_LOG_FUNCTION(this);
//
//	const char *ext1 = NS3_FLOW_MONITOR_FILENAME1;
//	char* fm_FileName = (char*) malloc(
//			std::strlen(simulationRunProperties::arg0) + std::strlen(ext1) + 3);
//	std::sprintf(fm_FileName, "%s%s", simulationRunProperties::arg0, ext1);
//	//NS_ASSERT_MSG(monitor != NULL, "Failed to get open flowmonitor");
//	//monitor->SerializeToXmlFile(fm_FileName, true, true);
//	NS_LOG_UNCOND(
//			"Statistics: Flow Monitor output is at \t[" << fm_FileName << "]");
////
//	const char *ext2 = "-ns3FlowStatistics.csv";
//	char* aFileName = (char*) malloc(
//			std::strlen(ext1) + std::strlen(simulationRunProperties::arg0)
//					+ 10);
//	std::sprintf(aFileName, "%s%s", simulationRunProperties::arg0, ext2);
//	std::ofstream fpF;
//	fpF.open(aFileName);
//	if (fpF == NULL) {
//		NS_LOG_UNCOND("Failed to open ns3 Flow Statistics file " << aFileName);
//		return;
//	}
//	/* ---Write ns2 collected all flow statistics to file---*/
//	NS_LOG_UNCOND(
//			"Statistics: ns3 Flow Statistics output is at \t[" << aFileName << "]");
//	fpF
//			<< "Flow ID,Src Addr,Dst Addr,Tx Packets,Rx Packets,Lost Packets,Throughput(Kbps),Mean Latency[ns],Mean Jitter[ns],";
//	fpF
//			<< "timeFirstTxPacket[ns],timeFirstRxPacket[ns],FirstPacketLatency[ns],timeLastTxPacket[ns],timeLastRxPacket[ns],LastPacketLatency[ns],delaySum[ns],*flowLatency[ns]\n";
//
//	Ptr<Ipv4FlowClassifier> classifier;
//	std::map<FlowId, FlowMonitor::FlowStats> stats;
//	std::map<FlowId, FlowMonitor::FlowStats>::const_iterator pathStat;
//	Ipv4FlowClassifier::FiveTuple path1;
//	// Sanjeev: MAy 9th, All below code is taken from ns3 webpage given below
//	monitor->CheckForLostPackets();
//	// Tracing, added by martin:
//	// Print per flow statistics
//	// following added from: https://bitbucket.org/mezza84/ns-3-lte-dev/src/18bb8e694652/examples/wireless/wifi-hidden-terminal.cc
//	classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
//
//	stats = monitor->GetFlowStats();
//	NS_ASSERT_MSG(classifier != NULL, "Failed to get classifier");
//
//	double Throughput = 0.0;
//	Time flowLatency = Seconds(0.0);
//	Time meanDelay = Seconds(0.0);
//	Time meanJitter = Seconds(0.0);
//
//	for (pathStat = stats.begin(); pathStat != stats.end(); ++pathStat) {
//		path1 = classifier->FindFlow(pathStat->first);
//
//		// verify if any packets are lost in this flow....bug sanjeev 7 Feb
//		if (pathStat->second.txPackets != pathStat->second.rxPackets) {
//			meanDelay = Seconds(0);
//			meanJitter = Seconds(0);
//			Throughput = 0;
//			flowLatency = Seconds(0);
//		} else if (pathStat->second.rxPackets) {
//			Throughput = pathStat->second.rxBytes * 8.0
//					/ (pathStat->second.timeLastRxPacket.GetSeconds()
//							- pathStat->second.timeFirstTxPacket.GetSeconds())
//					/ 1000;
//			// Better option for latency, this metric seems to be working right, although NO documentation
//			meanDelay = pathStat->second.delaySum / pathStat->second.rxPackets;
//			// Apr 22. Sanjeev bug? in meanJitter calc?
//			meanJitter = pathStat->second.jitterSum
//					/ pathStat->second.rxPackets;
//			/* this calculatation includes inter-packet time, when no packet is transmitted
//			 * so gives full flow information. This measures 1st txn & last rcv plus interm-no-packet-times
//			 */
//			flowLatency = (pathStat->second.timeLastRxPacket
//					- pathStat->second.timeFirstTxPacket);
//		}
//		// "Flow ID,Src Addr,Dst Addr,Tx Packets,Rx Packets,Lost Packets,Throughput(Kbps),Mean Latency,Mean jitter,"
//		// "timeFirstTxPacket,timeFirstRxPacket,FirstPacketLatency,timeLastTxPacket,timeLastRxPacket,LastPacketLatency,delaySum,flowLatency,"
//		fpF << pathStat->first << "," << path1.sourceAddress << ","
//				<< path1.destinationAddress << "," << pathStat->second.txPackets
//				<< "," << pathStat->second.rxPackets << ","
//				<< pathStat->second.txPackets - pathStat->second.rxPackets
//				<< "," << Throughput << "," << meanDelay.ToDouble(Time::NS)
//				<< "," << meanJitter.ToDouble(Time::NS) << ","
//				<< pathStat->second.timeFirstTxPacket.ToDouble(Time::NS) << ","
//				<< pathStat->second.timeFirstRxPacket.ToDouble(Time::NS) << ","
//				<< pathStat->second.timeFirstRxPacket.ToDouble(Time::NS)
//						- pathStat->second.timeFirstTxPacket.ToDouble(Time::NS)
//				<< "," << pathStat->second.timeLastTxPacket.ToDouble(Time::NS)
//				<< "," << pathStat->second.timeLastRxPacket.ToDouble(Time::NS)
//				<< ","
//				<< pathStat->second.timeLastRxPacket.ToDouble(Time::NS)
//						- pathStat->second.timeLastTxPacket.ToDouble(Time::NS)
//				<< "," << pathStat->second.delaySum.ToDouble(Time::NS) << ","
//				<< flowLatency.ToDouble(Time::NS) << "\n";
//
//	} // end of for loop....
//
//	fpF.close();
}

/************************************************/
void BaseTopology::WriteCustomFlowStatisticsToFile() {
	NS_LOG_FUNCTION(this);

	/* ---Write manually collected all flow statistics to file---*/
	time_t rawtime;
	struct tm * timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	// sanjeev Jun 18, store files uniquely for each run
	char* aFileName = (char*) malloc(
			std::strlen(CUSTOM_FLOW_STATISTICS_FILENAME1)
					+ std::strlen(simulationRunProperties::arg0) + 50);
	std::sprintf(aFileName, "%s_%.4d%.2d%.2d%.2d%.2d_%s",
			simulationRunProperties::arg0, timeinfo->tm_year + 1900,
			timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour,
			timeinfo->tm_min, CUSTOM_FLOW_STATISTICS_FILENAME1);

	int m_totalPacketCount = 0;   //Madhurima Added on May 5th for utilization
	std::ofstream fp;
	fp.open(aFileName);

	if (fp == NULL) {
		NS_LOG_UNCOND(
				"Failed to open Manual Flow Statistics file " << aFileName);
		return;
	}
	fp
			<< "FlowNumber,SrcNodeId,SrvIpv4,DstNodeId,DstIpv4,PacketCount,1stPacketSrcTime[us],1stPacketDstTime[us],";
	fp
			<< "LastPacketSrcTime[us],LastPacketDstTime[us],1stPacketLatency[us],LastPacketLatency[us],*FlowLatency[us],FlowDuration[ms],BW[Mbps]\n";
	NS_LOG_UNCOND(
			"Statistics: Writing Manual Flow Statistics file " << aFileName << "\n");
	int limit = (
			m_flowCount <= MAXFLOWDATA_TO_COLLECT ?
					m_flowCount : MAXFLOWDATA_TO_COLLECT);
// do not start from zero
	for (int i = 0; i < limit; i++) {
		fp << BaseTopology::m_flowData[i].flowid << ","
				<< BaseTopology::m_flowData[i].srcNodeid << ","
				<< BaseTopology::m_flowData[i].srcNodeIpv4 << ","
				<< BaseTopology::m_flowData[i].dstNodeid << ","
				<< BaseTopology::m_flowData[i].dstNodeIpv4 << ","
				<< BaseTopology::m_flowData[i].packetid + 1 << ","
				<< BaseTopology::m_flowData[i].packet1_srcTime << ","
				<< BaseTopology::m_flowData[i].packet1_dstTime << ","
				<< BaseTopology::m_flowData[i].packetL_srcTime << ","
				<< BaseTopology::m_flowData[i].packetL_dstTime << ","
				<< BaseTopology::m_flowData[i].packet1_dstTime
						- BaseTopology::m_flowData[i].packet1_srcTime << ","
				<< BaseTopology::m_flowData[i].packetL_dstTime
						- BaseTopology::m_flowData[i].packetL_srcTime << ","
				<< (BaseTopology::m_flowData[i].packetL_dstTime
						- BaseTopology::m_flowData[i].packet1_srcTime) << ","
				<< BaseTopology::m_flowData[i].flowDuration << ","
				<< BaseTopology::m_flowData[i].flowBandwidth << "\n";
		m_totalPacketCount = m_totalPacketCount
				+ BaseTopology::m_flowData[i].packetid + 1;
	}

	fp.close();

	//Madhurima added this on May 5th , i slpit it into step other wise it was calculating something else which is not correct
	double utilization = (m_totalPacketCount * 8);
	utilization = utilization * simulationRunProperties::packetSize;
	utilization = utilization / (MEGASEC_TO_SECS);
	utilization = utilization / 16000;
	double m_averageQueueLength = (utilization * utilization);
	m_averageQueueLength = m_averageQueueLength / (1 - utilization);
	NS_LOG_UNCOND(
			"Statistics: MR: Total Packet Count is "<< m_totalPacketCount << " The network utilization at the lowest level is: "<< utilization);
	NS_LOG_UNCOND(
			"Statistics: MR: Estimated Queue length at the end points(this is the figure we are trying to achieve) " << m_averageQueueLength);
	int numberOfSamples = simulationRunProperties::simStop
			/ DefaultEnergyMeterUpdateIntervalSec;
	double avgQueueLenghtObserved =
			m_totalTreeEnergy->m_DeviceTotalObservedQueueDepth
					/ numberOfSamples;
	double avgQueueLengthObservedPerDevice = avgQueueLenghtObserved
			/ m_totalTreeEnergy->m_deviceCount;
	NS_LOG_UNCOND(
			"Statistics: SS: (all devices) m_DeviceTotalObservedQueueDepth ["<< m_totalTreeEnergy->m_DeviceTotalObservedQueueDepth << "] avgQueueLengthObserved [" << avgQueueLenghtObserved << "] avgQueueLengthObservedPerDevice ["<< avgQueueLengthObservedPerDevice << "]");
}

/**************************************************************/
void BaseTopology::WriteFinalTestOutputToFile() {
	NS_LOG_FUNCTION(this);
	bool fileExists = false;
	const char *ext1 = FINAL_TEST_OUTPUT_FILENAME1;
	char* fm_FileName = (char*) malloc(
			std::strlen(simulationRunProperties::arg0) + std::strlen(ext1) + 3);
	std::sprintf(fm_FileName, "%s%s", simulationRunProperties::arg0, ext1);
	std::ofstream fp;
	std::ifstream f(fm_FileName);
	// file already exists?
	fileExists = f.good();
	f.close();

	fp.open(fm_FileName, std::ofstream::out | std::ofstream::app);
	if (fp == NULL) {
		NS_LOG_UNCOND("\nFailed to open results output file: " << fm_FileName);
		return;
	} else
		NS_LOG_UNCOND(
				"\nStatistics: Final results output to file: " << fm_FileName);

	double hwlsrcEfficiency = (
			m_hwlDataMetric.totalSrcRequests > 0 ?
					m_hwlDataMetric.totalSrcRequestSuccess * 100
							/ m_hwlDataMetric.totalSrcRequests :
					0);
	double hwldstEfficiency = (
			m_hwlDataMetric.totalDstRequests > 0 ?
					m_hwlDataMetric.totalDstRequestSuccess * 100
							/ m_hwlDataMetric.totalDstRequests :
					0);
	// Sanjeev Apr21
	const char *modelName;
	double modelValue0 = 0, modelValue1 = 0, modelValue3 = 0;

	if (simulationRunProperties::enableMarkovModel) {
		modelName = "Markov";
		modelValue0 = simulationRunProperties::markovETA0;
		modelValue1 = simulationRunProperties::markovETA1;
		modelValue3 = simulationRunProperties::markovStateInterval;
	} else if (simulationRunProperties::enableSawToothModel) {
		modelName = "SawTooth";
		modelValue0 = simulationRunProperties::flowMin;
		modelValue1 = simulationRunProperties::flowMax;
	} else if (simulationRunProperties::enableTickModel) {
		modelName = "TickModel";
		modelValue0 = simulationRunProperties::markovETA0;
		modelValue1 = simulationRunProperties::markovETA1;
		modelValue3 = simulationRunProperties::tickStateInterval;
	} else {
		modelName = "Constant";
	}
	time_t now = time(0);
	// convert now to string form & remove new line char.
	char* dt = ctime(&now);
	dt[strlen(dt) - 1] = '\0';
	// 1st time open? write header of file
	if (!fileExists) {
		fp
				<< "Time,k & simStop & BW Thr,TOS Energy Model,TRS Energy Model,Model Name[interval],";
		fp
				<< "Model Values,Custom Routing,VMC Enabled,GA Enabled,Node Energy Enabled,";
		fp
				<< "Max Flows/Node,Initial Flows,Total Flows Executed,Flows Dropped,Failed To Start [Src+Dst],";
		fp
				<< "NonHW Failed [Src+Dst],QdepthSet,QdepthMaxUsed,Q Packets Lost,Q Flows Lost,";
		fp
				<< "totalConsumedPower(W),totalConsumedEnergy(J),totalTransmitTimeEnergy(J),";
		fp << "totalSleepStateTimeEnergy(J),totalIdleTimeEnergy(J),";
		fp << "totalTime(s),Idle time(s),Sleep time(s),Txn time(s),";
		fp
				<< "ns3:AvgDelay(us),AvgJitter(us),*AvgFlowLatency(us),AvgThroughPut(Mbps),";
		fp << "OfferedThroughput(Mbps),ConsumedThroughput(Mbps),";
		fp << "HostWeightList Src Efficiency,HostWeightList Dst Efficiency\n";
		//" Equal=" << m_hwlDataMetric.totalSrcHostEqual);
	}
	double runTime = (m_totalTreeEnergy->m_totalTransmitTimeNanoSec
			+ m_totalTreeEnergy->m_totalSleepTimeNanoSec
			+ m_totalTreeEnergy->m_totalIdleTimeNanoSec) / NanoSec_to_Secs;
	// sanjeev Jun 18 logging...
	if (simulationRunProperties::enableDefaultns3EnergyModel)
		runTime = simulationRunProperties::simStop;
	int maxFlowsPerNode = DynamicCast<ssNode>(hosts.Get(0))->m_maxFlowPerNode;
	// sanjeev Apr 17- statistics format changed... Sanjeev Apr21
	fp << dt << "," << simulationRunProperties::k << " & "
			<< simulationRunProperties::simStop << " & "
			<< simulationRunProperties::bandwidth_thresehold_percentage << ","
			<< (simulationRunProperties::enableDeviceTOSManagement ?
					"True" : "False") << ","
			<< (simulationRunProperties::enableDeviceTRSManagement ?
					"True" : "False") << "," << modelName << "," << modelValue0
			<< " & " << modelValue1 << " [" << modelValue3 << "ms],"
			<< (simulationRunProperties::enableCustomRouting ? "True" : "False")
			<< "," << (simulationRunProperties::enableVMC ? "True" : "False")
			<< ","
			<< (simulationRunProperties::enable_global_algortihm ?
					"true" : "false") << ","
			<< (simulationRunProperties::enableNodeFabricManagement ?
					"true" : "false") << "," << maxFlowsPerNode << ","
			<< simulationRunProperties::initialFlowCount << ","
			<< m_totalTreeEnergy->m_FlowCount << ","
			<< ssUdpEchoClient::flows_dropped << ","
			<< m_hwlDataMetric.totalSrcRequestFailed << " + "
			<< m_hwlDataMetric.totalDstRequestFailed << ","
			<< m_hwlDataMetric.totalNonHWL_srcRequestFailed << " + "
			<< m_hwlDataMetric.totalNonHWL_dstRequestFailed << ","
			<< m_totalTreeEnergy->m_deviceSetQdepth << ","
			<< m_totalTreeEnergy->m_deviceMaxQdepth << ","
			<< m_totalTreeEnergy->m_lostPacketCount << ","
			<< m_totalTreeEnergy->m_FlowsWithLostPacket << ","
			<< m_totalTreeEnergy->m_totalConsumedEnergy_nJ
					/ (simulationRunProperties::simStop * NanoSec_to_Secs)
			<< ","
			<< m_totalTreeEnergy->m_totalConsumedEnergy_nJ / NanoSec_to_Secs
			<< ","
			<< m_totalTreeEnergy->m_totalTransmitTimeEnergy_nJ / NanoSec_to_Secs
			<< ","
			<< m_totalTreeEnergy->m_totalSleepStateTimeEnergy_nJ
					/ NanoSec_to_Secs << ","
			<< m_totalTreeEnergy->m_totalIdleTimeEnergy_nJ / NanoSec_to_Secs
			<< "," << runTime << ","
			<< m_totalTreeEnergy->m_totalIdleTimeNanoSec / NanoSec_to_Secs
			<< ","
			<< m_totalTreeEnergy->m_totalSleepTimeNanoSec / NanoSec_to_Secs
			<< ","
			<< m_totalTreeEnergy->m_totalTransmitTimeNanoSec / NanoSec_to_Secs
			<< "," << m_totalTreeEnergy->m_AvgDelay.ToDouble(Time::US) << ","
			<< m_totalTreeEnergy->m_AvgJitter.ToDouble(Time::US) << ","
			<< m_totalTreeEnergy->m_AvgLatency.ToDouble(Time::US) << ","
			<< m_totalTreeEnergy->m_AvgThroughPut << ","
			<< m_totalTreeEnergy->m_AvgOfferedThroughPut << ","
			<< m_totalTreeEnergy->m_AvgConsumedThroughPut << ","
			<< hwlsrcEfficiency << "," << hwldstEfficiency << "\n";
	fp.close();
}

}  // using namespace

