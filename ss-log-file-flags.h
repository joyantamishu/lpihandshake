/*
 * ss-log-file-flags.h
 *
 *  Created on: Apr 4, 2017
 *      Author: sanjeev
 */

#ifndef LPIHANDSHAKE_SS_LOG_FILE_FLAGS_H_
#define LPIHANDSHAKE_SS_LOG_FILE_FLAGS_H_

// set below conditional log to NS_LOG_UNCOND to show the display strings.
#define TRACE_ROUTING							NS_LOG_LOGIC		// packet tracing
#define SS_ENEGRY_LOG							NS_LOG_UNCOND		// final energy metrics show on screen
#define SS_FINAL_FLOW_STAT_LOG					NS_LOG_UNCOND		// clearer naming only, sanjeev, may 10, final flow metrics show on screen
#define SS_APPLIC_LOG							NS_LOG_UNCOND 		// shows flow start/stop..

#define PACKETINTERVAL_DEBUG_FILENAME2			"ssUdpEchoClient_Flow_InterPacketTiming.csv"
#define WRITE_PACKET_TIMING_TO_FILE				 false

#define PACKET_DEBUG_FILENAME2					"PacketTrace"
#define TRACE_EVERY_PACKET						false			// T=trace every packets?  or F=trace only last packet
#define WRITE_PACKET_TO_FILE					true			// trace all packets(for logging or not)..Not too elligent

#define WRITE_DEVICE_ENERGY_TO_FILE				true
#define DEVICE_DEBUG_FILENAME1					"_AllDeviceEnergyStatistic"		// no extension, appended with timestamp

#define	WRITE_FLOW_DURATION_TO_FILE				true
#define FLOW_DURATION_DEBUG_FILENAME1			"_EachFlowDurationTiming.csv"

#define	WRITE_VM_PLACEMENT_TO_FILE				true
#define VM_PLACEMENT_DEBUG_FILENAME1			"_VMPlacementDetails.csv"

#define	WRITE_VM_EFFICIENY_TO_FILE				true
#define VM_EFFICIENY_DEBUG_FILENAME1			"_VMEfficiencyMetrics.csv"

#define WRITE_NETANIM_COUNTERS_TO_FILE			false
#define NETANIM_COUNTERS_FILENAME				"_ns3NetAnimCounters.csv"

#define WRITE_NETANIM_SIMULATION_TO_FILE		false
#define NS3_NETANIM_SIMULATION_FILENAME1		"_ns3NetAnim.xml"

#define WRITE_NS3_FLOW_STATISTICS_TO_FILE		true
#define NS3_FLOW_STATISTICS_FILENAME1			"_ns3FlowStatistics.csv"

#define WRITE_CUSTOM_FLOW_STATISTICS_TO_FILE	true
#define CUSTOM_FLOW_STATISTICS_FILENAME1		"_customFlowStatistics.csv"

#define NS3_ROUTING_TABLE_FILENAME1				"_ns3RoutingTable.xml"
#define NS3_FLOW_MONITOR_FILENAME1				"_ns3FlowMonitor.xml"

#define DEVICE_STATE_CHANGE_FILENAME1			"_States.csv"
#define DEVICE_ENERGY_USED_FILENAME1			"_Energy.csv"
#define DEVICE_STATE_DURATION_FILENAME1			"_Duration.csv"

#define FINAL_TEST_OUTPUT_FILENAME1				"__testOutput.csv"

#endif /* LPIHANDSHAKE_SS_LOG_FILE_FLAGS_H_ */
