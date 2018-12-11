/*
 * ss-base-topology-application.cc
 *
 *  Created on: Jul 12, 2016
 *      Author: sanjeev
 */

#include <stdlib.h>
#include <ctime>

#include "ns3/log.h"
#include "ns3/ipv4-global-routing.h"
#include "ss-base-topology.h"
#include "ss-device-energy-model.h"
#include "ss-energy-source.h"
#include "ss-udp-echo-client.h"
#include "ss-udp-echo-server.h"
#include "ss-log-file-flags.h"

// parameters to stagger flow, sanjeev, mar 17
#define STAGGERFLOW								false
#define STAGGERFLOW_MINTIME_MILLISEC			0.1
#define STAGGERFLOW_MAXTIME_MILLISEC			1

#define UDP_ECHO_SERVER_PORT 					4500
#define NEWFLOW_START_DELAY_MILLISEC			0.0

#define ENABLE_SANJEEV_SAWTOOTH_MODEL 			true

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("BaseTopologyA");

/*************************************************************/
void BaseTopology::InstallSampleApplications(void) {
	NS_LOG_FUNCTION(this);

	// simplifiled,sanjeev 2/25
	InitializeRandomVariables();
	// simplified. sanjeev 2/25
	InstallUdpApplicationOnRandomClientServer();
}
// simplified. sanjeev 2/25, sanjeev 3/18
/*************************************************************/
void BaseTopology::openApplicationDebugFiles(void) {
	// write the flowduration to a file for debugging or plotting later...
	char* aFileName;
	if (WRITE_FLOW_DURATION_TO_FILE) {
		aFileName = (char*) malloc(
				std::strlen(FLOW_DURATION_DEBUG_FILENAME1)
						+ std::strlen(simulationRunProperties::arg0) + 50);
		std::sprintf(aFileName, "%s_%s", simulationRunProperties::arg0,
		FLOW_DURATION_DEBUG_FILENAME1);
		fp1.open(aFileName);
		if (fp1 == NULL)
			NS_LOG_UNCOND(
					"Statistics: Failed to open Flow Duration file " << aFileName);
		else {
			NS_LOG_UNCOND(
					"Statistics: Flow Duration will be written file " << aFileName);
			fp1 << "Statistics: Flow Duration & BW will be written here:: ";
			fp1 << ",Custom Routing:"
					<< (simulationRunProperties::enableCustomRouting ?
							"true" : "false");
			fp1 << ",Tick Model:"
					<< (simulationRunProperties::enableTickModel ?
							"true" : "false");
			fp1 << ",Markov Model:"
					<< (simulationRunProperties::enableMarkovModel ?
							"true" : "false");
			fp1 << ",Energy Model:"
					<< (simulationRunProperties::enableDeviceTOSManagement ?
							"TOS" : "TRS");
			fp1 << ",**Verify Data**\n";
			fp1
					<< "Time(us),FlowId,FlowDuration(ms), BW(Mbps),FlowEndTime(ms),";
			if (simulationRunProperties::enableMarkovModel) {
				fp1
						<< "MarkovState,NextStateChangeTime(ms),Dummy,ActiveFlowCount\n";
			} else if (simulationRunProperties::enableSawToothModel) {
				fp1
						<< "SawToothModel,NextFlowsToStop,NextFlowToInject,ActiveFlowCount\n";
			} else
				fp1 << "ConstantState,Dummy,Dummy,ActiveFlowCount\n";
		}
	}

	// write VM placement to a file for debugging or plotting later...
	if (WRITE_DEVICE_ENERGY_TO_FILE) {
		// sanjeev Jun 18, store files uniquely for each run
		aFileName = (char*) malloc(
				std::strlen(DEVICE_DEBUG_FILENAME1)
						+ std::strlen(simulationRunProperties::arg0) + 50);
#if 0
		time_t rawtime;
		struct tm * timeinfo;
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		std::sprintf(aFileName, "%s_%s_%.4d%.2d%.2d%.2d%.2d.csv",
				simulationRunProperties::arg0,
				DEVICE_DEBUG_FILENAME1, timeinfo->tm_year+1900,
				timeinfo->tm_mon+1, timeinfo->tm_mday, timeinfo->tm_hour,
				timeinfo->tm_min);
#else
		std::sprintf(aFileName, "%s_%s_%s.csv", simulationRunProperties::arg0,
		DEVICE_DEBUG_FILENAME1,
				(simulationRunProperties::enableCustomRouting ? "true" : "false"));
#endif
		fpDeviceEnergy.open(aFileName);
		if (fpDeviceEnergy == NULL)
			NS_LOG_UNCOND(
					"Statistics: Failed to open Device Energy Statistics file " << aFileName);
		else {
			NS_LOG_UNCOND(
					"Statistics: Device Energy Statistics will be written file " << aFileName);
			fpDeviceEnergy
					<< "Energy Metrics from all devices will be written here\n";
			fpDeviceEnergy << ",Custom Routing:"
					<< (simulationRunProperties::enableCustomRouting ?
							"true" : "false");
			fpDeviceEnergy << ",Tick Model:"
					<< (simulationRunProperties::enableTickModel ?
							"true" : "false");
			fpDeviceEnergy << ",Markov Model:"
					<< (simulationRunProperties::enableMarkovModel ?
							"true" : "false");
			fpDeviceEnergy << ",Energy Model:"
					<< (simulationRunProperties::enableDeviceTOSManagement ?
							"TOS" : "TRS");
			fpDeviceEnergy << ",**Verify Data**\n\n";
		}
	}

	if (WRITE_VM_EFFICIENY_TO_FILE) {
		aFileName = (char*) malloc(
				std::strlen(VM_EFFICIENY_DEBUG_FILENAME1)
						+ std::strlen(simulationRunProperties::arg0) + 50);
		std::sprintf(aFileName, "%s_%s", simulationRunProperties::arg0,
		VM_EFFICIENY_DEBUG_FILENAME1);
		fpVMCEff.open(aFileName);
		if (fpVMCEff == NULL)
			NS_LOG_UNCOND(
					"Statistics: Failed to open VMC Efficiency Metrics file " << aFileName);
		else {
			NS_LOG_UNCOND(
					"Statistics: VMC Efficiency Metrics will be written file " << aFileName);
			fpVMCEff << "Statistics: VMC Efficiency Metrics will be here\n";
			fpVMCEff
					<< "Time(us),FlowId,SelectedHostId,SrcRequest,SrcSuccess,DstRequest,DstSuccess,PercentSrcSuccess,PercentDstSuccess\n";
		}
	}

	if (WRITE_VM_PLACEMENT_TO_FILE) {
		aFileName = (char*) malloc(
				std::strlen(VM_PLACEMENT_DEBUG_FILENAME1)
						+ std::strlen(simulationRunProperties::arg0) + 50);
		std::sprintf(aFileName, "%s_%s", simulationRunProperties::arg0,
		VM_PLACEMENT_DEBUG_FILENAME1);
		fpVMPlacement.open(aFileName);
		if (fpVMPlacement == NULL)
			NS_LOG_UNCOND(
					"Statistics: Failed to open VMC Placement Metrics file " << aFileName);
		else {
			NS_LOG_UNCOND(
					"Statistics: VMC Placement Metrics will be written file " << aFileName);
			int n = hosts.GetN();
			fpVMPlacement
					<< "Statistics: VMC Placement Metrics will be written for each host (SrcSideOut + DstSideIn)\n";
			fpVMPlacement << "Time(us),FlowId";
			for (int h, i = 0; i < n; i++) {
				h = hosts.Get(i)->GetId();
				fpVMPlacement << ",SrcAppOut[" << h << "],SrcUtilBW[" << h
						<< "],DstAppIn[" << h << "],DstUtilBW[" << h << "]";
			}
			fpVMPlacement
					<< ",TotalSrcBW,TotalDstBW,totalSrcApps,totalDstApps,AvgSrcBW/Flow,AvgDstBW/Flow,AvgSrcBW/Flow/Host,AvgDstBW/Flow/Host\n";
		}
	}
	// sanjeev, mar 23, write debug file netanim counters,
	// cannot depend on netanim to run & show these numbers in time scale
	if (WRITE_NETANIM_COUNTERS_TO_FILE) {
		aFileName = (char*) malloc(
				std::strlen(NETANIM_COUNTERS_FILENAME)
						+ std::strlen(simulationRunProperties::arg0) + 50);
		std::sprintf(aFileName, "%s_%s", simulationRunProperties::arg0,
		NETANIM_COUNTERS_FILENAME);
		fpNetAnimCounters.open(aFileName);
		if (fpNetAnimCounters == NULL)
			NS_LOG_UNCOND(
					"Statistics: Failed to open NetAnim Counters file " << aFileName);
		else {
			NS_LOG_UNCOND(
					"Statistics: NetAnim Counters Metrics will be written file " << aFileName);
			fpNetAnimCounters
					<< "Statistics: NetAnim Counters Metrics will be written for each counter at every unit time\n";
			fpNetAnimCounters
					<< "Time(ms),FlowCount,totalConsumedEnergy(J),totalIdleTimeEnergy(J),totalTransmitTimeEnergy(J),totalSleepStateTimeEnergy(J),totalTransmitTime(ms),totalSleepTime(ms),totalIdleTime(ms),totalTime(ms)\n";
		}
	}

}

/*************************************************************/
void BaseTopology::InstallUdpApplicationOnRandomClientServer(void) { //this is the methos used to inject the initial sets of flows
	NS_LOG_FUNCTION(this);
	// sanjeev 3/18
	openApplicationDebugFiles();
//
// Install servers on all hosts...
//
	ApplicationContainer allSrvApps;
	ssUdpEchoServerHelper* echoSrv = new ssUdpEchoServerHelper(
	UDP_ECHO_SERVER_PORT);
	allSrvApps = echoSrv->Install(hosts);
	int nSrvs = allSrvApps.GetN();
	for (int i = 0; i < nSrvs; i++)
		DynamicCast<ssUdpEchoServer>(allSrvApps.Get(i))->RegisterDecrementFlowMetric_CallbackSS(
				MakeCallback(&BaseTopology::markFlowStoppedMetric, this));
	/* .Mar 15............*/
	allSrvApps.Start(Seconds(0.));
	allSrvApps.Stop(Seconds(simulationRunProperties::simStop));
	// sanjeev simplified, 2/25

	SS_APPLIC_LOG(
			"Server installed on all [" << hosts.GetN() << "] hosts, initial [" << simulationRunProperties::initialFlowCount << "] flows installed as below");
	// sanjeev Apr 18
	// this should be done intelligently to attempt to start "n" initial flows, keeping it simple now
	int nLoopCount = simulationRunProperties::initialFlowCount;
	//
	// now install appl_(flows) ie clientApps on all nodes...
	// and randomly set a random client to talk to random server !!!
	//
#if STAGGERFLOW
	m_newStaggeredflowTime = CreateObject<UniformRandomVariable>();
	m_newStaggeredflowTime->SetAttribute("Min",
			DoubleValue(STAGGERFLOW_MINTIME_MILLISEC));
	m_newStaggeredflowTime->SetAttribute("Max",
			DoubleValue(STAGGERFLOW_MAXTIME_MILLISEC));
	Time timeOfStaggeredFlow;
#endif
	for (t_a = 0; t_a < nLoopCount; t_a++) {
		// sanjeev , mar 17, stagger calling the new flows...
#if STAGGERFLOW
		timeOfStaggeredFlow = Time::FromDouble(
				m_newStaggeredflowTime->GetValue(), Time::MS);
		Simulator::ScheduleWithContext(t_a, timeOfStaggeredFlow,
				&BaseTopology::InjectInitialFlowsMR, this,
				simulationRunProperties::initialFlowCount);
#else
		InjectANewRandomFlow();
#endif
	}

}
/*************************************************************/
void BaseTopology::InjectInitialFlowsMR(const int &requiredInitialFlowCount) {
	NS_LOG_FUNCTION(this);
	// we only inject the required number of flows- if other events have succeeded then do nothing
	if (m_flowCount >= requiredInitialFlowCount)
		return;
	InjectANewRandomFlow();
}

void BaseTopology::InjectANewRandomFlowCopyCreation(uint32_t src, uint32_t dest, uint32_t number_of_packets, bool read_flow, uint32_t required_bandwidth, bool non_consistent_read_flow, double finish_time, uint32_t app_id)
{
	NS_LOG_UNCOND("Inside InjectANewRandomFlowCopyCreation "<<" src "<<src<<" dest "<<dest<<" number_of_packets "<<number_of_packets<<" required_bandwidth "<<required_bandwidth);

	Time static_t_appStopTimeRandom = Time::FromDouble(finish_time, Time::MS);

	ApplicationContainer static_t_allClientApps;
	ssUdpEchoClientHelper *static_t_echoClient;

	Ptr<ssNode> static_t_client, static_t_server;

	uint32_t static_t_x, static_t_b;

	Ipv4Address static_t_addr;

	static_t_b = src;

	static_t_x = dest;

	static_t_client = DynamicCast<ssNode>(BaseTopology::hosts_static.Get(static_t_b));

	static_t_server = DynamicCast<ssNode>(BaseTopology::hosts_static.Get(static_t_x));

	static_t_addr = static_t_server->GetNodeIpv4Address();
	static_t_echoClient = new ssUdpEchoClientHelper(static_t_addr, UDP_ECHO_SERVER_PORT);

	static_t_echoClient->SetAttribute("PacketSize", UintegerValue(simulationRunProperties::packetSize)); // run continuous till sim time ends
	static_t_echoClient->SetAttribute("RemoteHost", UintegerValue(static_t_server->GetId()));
	static_t_echoClient->SetAttribute("CurrentFlowNumber", UintegerValue(BaseTopology::consistency_flow));
	static_t_echoClient->SetAttribute("RequiredFlowBW", UintegerValue(required_bandwidth));
	static_t_echoClient->SetAttribute("RequiredReadFlowBW", UintegerValue(required_bandwidth));
	static_t_allClientApps = static_t_echoClient->Install(static_t_client,true, static_t_b, simulationRunProperties::total_applications + 1,dest, number_of_packets, read_flow, non_consistent_read_flow, app_id);

	static_t_allClientApps.Start(MilliSeconds(NEWFLOW_START_DELAY_MILLISEC));
	static_t_allClientApps.Stop(static_t_appStopTimeRandom);

	BaseTopology::consistency_flow++;

	//m_flowCount++;
}

/*************************************************************/

void BaseTopology::InjectANewRandomFlow(bool dummy_application) {


	/////We need to put here the application selection logic


	FILE *fp_flow_duration;
	NS_LOG_FUNCTION(this);

	double finish_time = m_flowStopDurationTimer->GetValue();
	t_appStopTimeRandom = Time::FromDouble(finish_time,
			Time::MS);
	fp_flow_duration = fopen ("flow_duration_madhurima.csv","a");
	fprintf(fp_flow_duration,"%f\n",t_appStopTimeRandom.ToDouble(Time::MS) );
	fclose(fp_flow_duration);


	if (m_internalClientStopTime < Simulator::Now() + t_appStopTimeRandom) {
		SS_APPLIC_LOG("New flow [" << m_flowCount << "] **disabled** because appDuration [" << t_appStopTimeRandom.ToDouble(Time::MS)
				<< "] exceeds simStop time");
		return;
	}
	// removed MR while loop... (take from previous backup code if needed),,,sanjeev May 10
	t_reqBW = m_randomBWVariable->GetInteger();

	///////////Chunk Specific Change/////////////
	NS_LOG_UNCOND("=======The t_reqBW prev====== "<<t_reqBW);
if(simulationRunProperties::uniformBursts)
	t_reqBW = t_reqBW * BaseTopology::intensity_change_scale; //uncomment this part

	NS_LOG_UNCOND("======The t_reqBW after======="<<t_reqBW);

	NS_LOG_UNCOND("BaseTopology::total_phrase_changed "<<BaseTopology::total_phrase_changed);
	//////////////////////////////////////////////////////
	// show output to user for clarity...
	NS_LOG_LOGIC(
			"FlowDuration::type [" << m_flowStopDurationTimer->GetInstanceTypeId() << "] t_appStopTimeRandom [" << t_appStopTimeRandom << "] RandomBW::type [" << m_randomBWVariable->GetInstanceTypeId() << "] t_reqBW [" << t_reqBW << "]\n");

	// get random client & server nodes

	uint32_t application_id;
	//t_b = getCustomizedRandomClientNode(application_id);
	if (!dummy_application) t_b = getCustomizedRandomClientNode(application_id);
	else t_b = getCustomizedRandomClientNodeDummy(application_id);

	if (t_b < 0) {
		// if returns NO suitable host found, abandon new flow...
		SS_APPLIC_LOG(
				"*********************** New flow NOT injected:: Failed to find srcNode[" << t_b << "]");
		return;
	}
	NS_LOG_UNCOND("Be careful, there could be some issue here");
	t_x = t_b+1;


	uint32_t write_bandwidth = (uint32_t) t_reqBW * ( 1.0 - simulationRunProperties::RWratio);

	uint32_t read_bandwidth = (uint32_t) (t_reqBW - write_bandwidth);

	if(dummy_application)
	{
		write_bandwidth = (uint32_t) t_reqBW * ( 1.0 - DUMMY_READ_WRITE_RATIO);

		read_bandwidth = (uint32_t) (t_reqBW - write_bandwidth);
	}

	//Create Write Flows

	t_client = DynamicCast<ssNode>(hosts.Get(t_b));
	t_server = DynamicCast<ssNode>(hosts.Get(t_x));
	NS_ASSERT_MSG(t_client != NULL && t_server != NULL,
			"Error in getting install client=" << t_b << " server=" << t_x);

	t_addr = t_server->GetNodeIpv4Address();
	t_echoClient = new ssUdpEchoClientHelper(t_addr,
	UDP_ECHO_SERVER_PORT);
	t_echoClient->SetAttribute("PacketSize",
			UintegerValue(simulationRunProperties::packetSize)); // run continuous till sim time ends
	t_echoClient->SetAttribute("RemoteHost", UintegerValue(t_server->GetId()));
	t_echoClient->SetAttribute("CurrentFlowNumber", UintegerValue(m_flowCount));
	t_echoClient->SetAttribute("RequiredFlowBW", UintegerValue(write_bandwidth));
	t_echoClient->SetAttribute("RequiredReadFlowBW", UintegerValue(read_bandwidth));
	t_echoClient->SetAttribute("FinishTime", DoubleValue(finish_time));
	t_allClientApps = t_echoClient->Install(t_client,false, t_b, application_id,0);

	t_clientApp = DynamicCast<ssUdpEchoClient>(t_allClientApps.Get(0));
	t_clientApp->RegisterStartNewFlow_CallbackSS(
			MakeCallback(&BaseTopology::InjectNewFlowCallBack, this));

	// set app start & stop time for apps..flows..
	t_allClientApps.Start(MilliSeconds(NEWFLOW_START_DELAY_MILLISEC));
	t_allClientApps.Stop(t_appStopTimeRandom);

	//end Creating Write Flow, Read Flow would be created at the udp-echo client part

	NS_LOG_UNCOND("The app id in injectRandom Flow "<<application_id);



	// for flow generation adjustment. sanjeev 5/9
	markFlowStartedMetric(m_flowCount, t_client->GetId(), t_server->GetId(),
			t_reqBW);

	SS_APPLIC_LOG(
			"New flow [" << m_flowCount << "] at node [" << t_client->GetId()
			<< "] ["<< t_client->GetNodeIpv4Address() << "] connected to node ["
			<< t_server->GetId() << "] ["<< t_addr << "] stop-time ["
			<< Simulator::Now().ToDouble(Time::MS) << " + " << t_appStopTimeRandom.ToDouble(Time::MS) << "]ms"
			<< (t_x == t_b ? "*" : ""));

	if (simulationRunProperties::enableMarkovModel) {
		if (m_MarkovModelVariable.m_IsT0_State) {
			m_MarkovModelVariable.metric_totalFlowDuration_inT0time +=
					t_appStopTimeRandom.ToDouble(Time::MS);
			m_MarkovModelVariable.metric_flows_inT0state++;
		} else {
			m_MarkovModelVariable.metric_totalFlowDuration_inT1time +=
					t_appStopTimeRandom.ToDouble(Time::MS);
			m_MarkovModelVariable.metric_flows_inT1state++;
		}
	}
	// capture metrics
	m_totalBW += t_reqBW;

	// finally increment the flowCount !! May 2, line moved May 9
	m_flowCount++;

	if (WRITE_FLOW_DURATION_TO_FILE) {
		const char* modelState;
		double param1 = 0, param2 = 0;
		if (simulationRunProperties::enableTickModel
				|| simulationRunProperties::enableMarkovModel) {
			modelState = (m_MarkovModelVariable.m_IsT0_State ? "T0" : "T1");
			param1 = m_MarkovModelVariable.tempNextStateChangeAt;
		} else if (simulationRunProperties::enableSawToothModel) {
			modelState = "SawTooth";
			param1 = m_SawToothModel.m_total_flows_to_inject;
			param2 = m_SawToothModel.m_total_flows_to_stop;
		} else
			modelState = "ConstantModel";

		WriteFlowDurationToFile(m_flowCount,
				t_appStopTimeRandom.ToDouble(Time::MS), t_reqBW,
				Simulator::Now().ToDouble(Time::MS)
						+ t_appStopTimeRandom.ToDouble(Time::MS), modelState,
				param1, param2);
	}
#if COMPILE_CUSTOM_ROUTING_CODE
	// sanjeev Mar 27...count flows placed on same host
	if (simulationRunProperties::enableVMC & (t_b == t_x)
			& HostWeightList::isInitialized)
		m_hwlDataMetric.totalSrcHostEqual++;
#endif
	if (WRITE_VM_PLACEMENT_TO_FILE)
		WriteVMPlacementMetricsToFile();

	NS_LOG_UNCOND("-------------------Exiting InjectANewRandomFlow--------------------------");
}

/*************************************************************/
void BaseTopology::InjectNewFlowCallBack(const uint32_t &flowId,
		const uint16_t &srcNodeId, const uint16_t &dstNodeId,
		const uint16_t &flowBW) {
	NS_LOG_FUNCTION(this);
	// sanjeev VMC logic, Apr 3rd, decrement host BW even if VMC is OFF
	// sanjeev removed this logic, this will be called from Server:GotLastPacket
	// sanjeev so it is now member function of ssUDPServer.h  Apr 17
	// markFlowStoppedMetric(flowId, srcNodeId, dstNodeId, flowBW);

	// m_totalStoppedFlowCounter++; apr 17
	if (simulationRunProperties::enableMarkovModel)
		InjectNewFlow_MarkovModel();
	else if (simulationRunProperties::enableSawToothModel)
		InjectNewFlow_SawToothModel();
	else if (simulationRunProperties::enableTickModel) {
		// do nothing..
	} else {
		// InjectANewRandomFlow();
		SS_APPLIC_LOG("Constant Model: No of flows stopped " << m_totalStoppedFlowCounter);
	}
}
/*************************************************************/
void BaseTopology::InjectNewFlow_MarkovModel(void) {
	NS_LOG_FUNCTION(this);

	SS_APPLIC_LOG(
			"This MarkovModel state [" << (m_MarkovModelVariable.m_IsT0_State == true? "T0":"T1") << "] prev_Random0 ["
			<< m_MarkovModelVariable.tempETA0 << "] prev_Random1 [" << m_MarkovModelVariable.tempETA1
			<< "] next state change @ [" << m_MarkovModelVariable.tempNextStateChangeAt
			<< "ms] current_flowsStopped [" << m_totalStoppedFlowCounter
			<< "] Active Flows [" << NUMBER_OF_ACTIVE_FLOWS << "]");

	// if Markov State T0
	if (m_MarkovModelVariable.m_IsT0_State) {
		// new flow injection based on Markov Model
		m_MarkovModelVariable.tempETA0 =
				m_MarkovModelVariable.m_eta_0->GetValue();
		if (m_MarkovModelVariable.tempETA0
				<= simulationRunProperties::markovETA0)
			InjectANewRandomFlow();
	}
	// else Markov State T1
	else {
		// see Dr.Kant's email always generate a new flow and then add another conditionally
		InjectANewRandomFlow();
		// new flow injection based on Markov Model
		m_MarkovModelVariable.tempETA1 =
				m_MarkovModelVariable.m_eta_1->GetValue();
		if (m_MarkovModelVariable.tempETA1
				<= simulationRunProperties::markovETA1)
			InjectANewRandomFlow();
	}
if (!simulationRunProperties::uniformBursts)
{	if(BaseTopology::intensity_change_scale> 1.0) //comment this  part
	{
		NS_LOG_UNCOND("%%%%%%%%%%%%%%%%%%%%%%%%%%%%Inject new random flow by dummy application%%%%%%%%%%%%%%%%%%%%%%%");
		InjectANewRandomFlow(true);
	}
}
}

/*************************************************************/
void BaseTopology::InjectNewFlow_SawToothModel(void) {
//	NS_LOG_FUNCTION(this);
//
//	// nothing to do till we have enough flows shutdown
//	m_SawToothModel.m_thisStoppedFlowCounter++;
//	if (m_SawToothModel.m_thisStoppedFlowCounter
//			< m_SawToothModel.m_total_flows_to_stop)
//		return;
//#if STAGGERFLOW
//	//Madhurima Changed on May 4 *************************I did not change this, but it needs a review by Sanjeev, if everything seems fine then ok
//	Time timeOfStaggeredFlow = MilliSeconds(m_newStaggeredflowTime->GetValue());
//#endif
//	for (t_i = 0; t_i < m_SawToothModel.m_total_flows_to_inject; t_i++) {
//		// sanjeev , mar 17, stagger calling the new flows...
//#if STAGGERFLOW
//		Simulator::Schedule(timeOfStaggeredFlow,
//				&BaseTopology::InjectANewRandomFlow, this);
//#else
//		InjectANewRandomFlow();
//#endif
//	}
//	// Apr 25 extended sawtooth-sanjeev...flip-flop no of flows_stopped to compensate with equal flows_to_inject
//	if (m_SawToothModel.m_IsPhase0_or_Phase1_State) {
//		// switch stops & starts for this state.
//		t_i = m_SawToothModel.m_total_flows_to_stop;
//		m_SawToothModel.m_total_flows_to_stop =
//				m_SawToothModel.m_total_flows_to_inject;
//		m_SawToothModel.m_total_flows_to_inject = t_i;
//	} else {	// generate next set of parameters to wait..&..inject..
//		m_SawToothModel.m_total_flows_to_stop =
//				m_SawToothModel.m_OldFlowStopCounter->GetInteger();
//		m_SawToothModel.m_total_flows_to_inject =
//				m_SawToothModel.m_newFlowCountToInject->GetInteger();
//	}
//	SS_APPLIC_LOG(
//			"This SawToothModel state, next random total_flows_to_stop [" << m_SawToothModel.m_total_flows_to_stop
//			<< "] total_flows_to_inject [" << m_SawToothModel.m_total_flows_to_inject
//			<< "] current_flowsStopped [" << m_SawToothModel.m_thisStoppedFlowCounter
//			<< "] Active Flows [" << NUMBER_OF_ACTIVE_FLOWS << "]");
//	m_SawToothModel.m_thisStoppedFlowCounter = 0;
//	// (go back to default sawtooth if this line commented off) Apr 24 sanjeev
//	m_SawToothModel.m_IsPhase0_or_Phase1_State =
//			(!m_SawToothModel.m_IsPhase0_or_Phase1_State)
//					& ENABLE_SANJEEV_SAWTOOTH_MODEL;
}

/*************************************************************/
void BaseTopology::InjectNewFlow_TickBasedModel(void) {
	NS_LOG_FUNCTION(this);

	SS_APPLIC_LOG(
			"This TickModel state [" << Simulator::Now().ToDouble(Time::MS)
			<< "ms] injecting a flow as per Markov Model");
	// inject as per Markov State
	InjectNewFlow_MarkovModel();
	// trigger Tick Model Flow Injection..
	// if simulation has stopped, then STOP...
	if (!Simulator::IsFinished())
		Simulator::Schedule(
				Time::FromDouble(m_TickModel.m_TickCounter->GetValue(),
						Time::MS), &BaseTopology::InjectNewFlow_TickBasedModel,
				this);
}

} // namespace
