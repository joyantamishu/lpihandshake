/*
 * ss-flow-parameters.cc
 *
 *  Created on: Mar 11, 2017
 *      Author: sanjeev
 */

#include <stdlib.h>
#include "ns3/log.h"
#include "ss-fat-tree-topology.h"
#include "ss-device-energy-model.h"
#include "ss-energy-source.h"
#include "ss-udp-echo-client.h"
#include "ss-udp-echo-server.h"
#include "parameters.h"
#include "ss-log-file-flags.h"
#include "flow-duration-random-variable-stream.h"
#include "ns3/ipv4-global-routing.h"

#define beta_lower_limit_bw			0.5
#define gamma_upper_limit_bw		1.5
#define markovInterval_upper_limit	(simulationRunProperties::markovStateInterval*4)
#define tickInterval_upper_limit	(simulationRunProperties::tickStateInterval*4)

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("FlowParameters");

void BaseTopology::InitializeRandomVariables(void) {
	NS_LOG_FUNCTION(this);

	// under constant mode: node to select for the flow, clients from 0---n-2, srv from 1..n-1
	/*m_randomClientNodeSelector = CreateObject<UniformRandomVariable>();
	m_randomClientNodeSelector->SetAttribute("Min", DoubleValue(0));
	m_randomClientNodeSelector->SetAttribute("Max",
			DoubleValue(7));
	m_randomServerNodeSelector = CreateObject<UniformRandomVariable>();
	m_randomServerNodeSelector->SetAttribute("Min", DoubleValue(8));
	m_randomServerNodeSelector->SetAttribute("Max",
			DoubleValue(15));
*/
	m_randomClientNodeSelector = CreateObject<UniformRandomVariable>();
		m_randomClientNodeSelector->SetAttribute("Min", DoubleValue(0));
		m_randomClientNodeSelector->SetAttribute("Max",
				DoubleValue(hosts.GetN() - 1));
		m_randomServerNodeSelector = CreateObject<UniformRandomVariable>();
		m_randomServerNodeSelector->SetAttribute("Min", DoubleValue(0));
		m_randomServerNodeSelector->SetAttribute("Max",
				DoubleValue(hosts.GetN() - 1));


	if (useRandomIntervals) {
		// flow bandwidth selector...
		int product = simulationRunProperties::product;
		double paretoAlpha = simulationRunProperties::alpha;
		double paretoMax = MAXFLOWSTOPTIME_MILLISEC;
		m_flowStopDurationTimer = CreateObject<FlowDurationRandomVariable>();
		m_flowStopDurationTimer->SetAttribute("Shape",
				DoubleValue(paretoAlpha));
		m_flowStopDurationTimer->SetAttribute("Bound", DoubleValue(paretoMax));
		double paretoMean = DynamicCast<FlowDurationRandomVariable>(
				m_flowStopDurationTimer)->GetMean();


		//NS_LOG_UNCOND("^^^^^^^^^^^^^The pareto mean is^^^^^^^^^^^^^^^^"<<paretoMean);


		int flowBWmean = product / paretoMean;
		int flowBWmin = flowBWmean * beta_lower_limit_bw;
		int flowBWmax = flowBWmean * gamma_upper_limit_bw;

		m_randomBWVariable = CreateObject<TriangularRandomVariable>();
		m_randomBWVariable->SetAttribute("Min", DoubleValue(flowBWmin));
		m_randomBWVariable->SetAttribute("Max", DoubleValue(flowBWmax));
		m_randomBWVariable->SetAttribute("Mean", DoubleValue(flowBWmean));
		// show output to user for clarity...
		NS_LOG_UNCOND("FlowDuration::type [" << m_flowStopDurationTimer->GetInstanceTypeId()
				<< "] FlowDuration::mean [" << paretoMean
				<< "] RandomBW::type [" << m_randomBWVariable->GetInstanceTypeId()
				<< "] RandomBW::mean [" << flowBWmean << "]\n");
	} else {
		// BW for each flow, override to random later
		m_randomBWVariable = CreateObject<ConstantRandomVariable>();
		m_randomBWVariable->SetAttribute("Constant",
				DoubleValue(TESTFLOWBANDWIDTH));
		// flow duration, override to random later
		m_flowStopDurationTimer = CreateObject<ConstantRandomVariable>();
		m_flowStopDurationTimer->SetAttribute("Constant",
				DoubleValue(TESTFLOWSTOPTIME_MILLISEC));
		// show output to user for clarity...
		NS_LOG_UNCOND("FlowDuration::type " << m_flowStopDurationTimer->GetInstanceTypeId()
				<< " constant " << TESTFLOWSTOPTIME_MILLISEC << "ms, RandomBW::type " << m_randomBWVariable->GetInstanceTypeId()
				<< " constant " << TESTFLOWBANDWIDTH << "\n");
	}

	// how many flows to stop inject, after random_flows have stopped
	// sawtooth Model , sanjeev simplified, 2/25
	if (simulationRunProperties::enableSawToothModel) {
		m_SawToothModel.m_IsPhase0_or_Phase1_State = false;		// Apr 25 extended sawtooth-sanjeev
		m_SawToothModel.m_OldFlowStopCounter = CreateObject<
				UniformRandomVariable>();
		m_SawToothModel.m_OldFlowStopCounter->SetAttribute("Min",
				DoubleValue(simulationRunProperties::flowMin));
		m_SawToothModel.m_OldFlowStopCounter->SetAttribute("Max",
				DoubleValue(simulationRunProperties::flowMax));
		m_SawToothModel.m_newFlowCountToInject = CreateObject<
				UniformRandomVariable>();
		m_SawToothModel.m_newFlowCountToInject->SetAttribute("Min",
				DoubleValue(simulationRunProperties::flowMin));
		m_SawToothModel.m_newFlowCountToInject->SetAttribute("Max",
				DoubleValue(simulationRunProperties::flowMax));
		// generate next set of parameters to wait..&..inject..
		m_SawToothModel.m_total_flows_to_stop =
				m_SawToothModel.m_OldFlowStopCounter->GetInteger();
		m_SawToothModel.m_total_flows_to_inject =
				m_SawToothModel.m_newFlowCountToInject->GetInteger();
		NS_LOG_UNCOND("Using SawToothModel:: y':type " << m_SawToothModel.m_OldFlowStopCounter->GetInstanceTypeId()
				<< " & y\":type " << m_SawToothModel.m_newFlowCountToInject->GetInstanceTypeId()
				<< " between [" << simulationRunProperties::flowMin
				<< "] & [" << simulationRunProperties::flowMax << "]");
	} else
	// Markov State Model
	if (simulationRunProperties::enableMarkovModel
			|| simulationRunProperties::enableTickModel) {
		m_MarkovModelVariable.m_IsT0_State = true;
		m_MarkovModelVariable.m_t0t1_stateInterval = CreateObject<
				ExponentialRandomVariable>();
		m_MarkovModelVariable.m_t0t1_stateInterval->SetAttribute("Mean",
				DoubleValue(simulationRunProperties::markovStateInterval));
		m_MarkovModelVariable.m_t0t1_stateInterval->SetAttribute("Bound",
				DoubleValue(markovInterval_upper_limit));
		m_MarkovModelVariable.m_eta_0 = CreateObject<UniformRandomVariable>();
		m_MarkovModelVariable.m_eta_0->SetAttribute("Min", DoubleValue(0.0));
		m_MarkovModelVariable.m_eta_0->SetAttribute("Max", DoubleValue(1.0));
		m_MarkovModelVariable.m_eta_1 = CreateObject<UniformRandomVariable>();
		m_MarkovModelVariable.m_eta_1->SetAttribute("Min", DoubleValue(0.0));
		m_MarkovModelVariable.m_eta_1->SetAttribute("Max", DoubleValue(1.0));
		m_MarkovModelVariable.tempNextStateChangeAt =
				m_MarkovModelVariable.m_t0t1_stateInterval->GetValue();
		// start metrics collection..
		m_MarkovModelVariable.metric_time_inT0state =
				m_MarkovModelVariable.tempNextStateChangeAt;
		m_MarkovModelVariable.metric_total_state_changes = 1;
		NS_LOG_UNCOND(
				"Using Markov Model, T0/T1 state interval:type [" << m_MarkovModelVariable.m_t0t1_stateInterval->GetInstanceTypeId()
				<< "] Mean [" << (DynamicCast<ExponentialRandomVariable>(
						m_MarkovModelVariable.m_t0t1_stateInterval))->GetMean()
				<< "] next state change @ [" << m_MarkovModelVariable.tempNextStateChangeAt << "ms]");
		NS_LOG_UNCOND("Using Markov Model, ETA0:type [" << m_MarkovModelVariable.m_eta_0->GetInstanceTypeId() << "] Value [" << simulationRunProperties::markovETA0
				<< "] ETA1:type ["<< m_MarkovModelVariable.m_eta_1->GetInstanceTypeId() << "] Value [" << simulationRunProperties::markovETA1
				<< "]");
		// trigger Markov State Transitions..
		Simulator::Schedule(
				Time::FromDouble(m_MarkovModelVariable.tempNextStateChangeAt,
						Time::MS), &BaseTopology::generateNextMarkovState,
				this);
	}
	// Tick Based Model
	if (simulationRunProperties::enableTickModel) {
		// trigger Tick based flow injection..
		m_TickModel.m_TickCounter = CreateObject<ExponentialRandomVariable>();
		m_TickModel.m_TickCounter->SetAttribute("Mean",
				DoubleValue(simulationRunProperties::tickStateInterval));
		m_TickModel.m_TickCounter->SetAttribute("Bound",
				DoubleValue(tickInterval_upper_limit));
		Simulator::Schedule(
				Time::FromDouble(m_TickModel.m_TickCounter->GetValue(),
						Time::MS), &BaseTopology::InjectNewFlow_TickBasedModel,
				this);
		NS_LOG_UNCOND(
				"Using Tick Model, Type [" << m_TickModel.m_TickCounter->GetInstanceTypeId() << "(mean)interval [" << simulationRunProperties::tickStateInterval << "]");

	}
}
/****************************************************/
void BaseTopology::generateNextMarkovState(void) {
	NS_LOG_FUNCTION(this);
	//flip state T0-->T1-->T0-->T1....
	m_MarkovModelVariable.m_IsT0_State = !m_MarkovModelVariable.m_IsT0_State;
	m_MarkovModelVariable.tempNextStateChangeAt =
			m_MarkovModelVariable.m_t0t1_stateInterval->GetValue();

	m_MarkovModelVariable.metric_total_state_changes++;
	if (m_MarkovModelVariable.m_IsT0_State) {
		m_MarkovModelVariable.metric_time_inT0state +=
				m_MarkovModelVariable.tempNextStateChangeAt;
	} else
		m_MarkovModelVariable.metric_time_inT1state +=
				m_MarkovModelVariable.tempNextStateChangeAt;

	// if simulation has stopped, then STOP...
	if (!Simulator::IsFinished())
		Simulator::Schedule(
				Time::FromDouble(m_MarkovModelVariable.tempNextStateChangeAt,
						Time::MS), &BaseTopology::generateNextMarkovState,
				this);
}
/*
 * Sanjeev, Apr 23 - clean logging, plu some additional details on no of active flows in system
 */
void BaseTopology::WriteFlowDurationToFile(const int &flowCount,
		const double &appDuration, const int &reqBW, const double &appStopTime,
		const char *modelState, const double &param1, const double &param2) {
	NS_LOG_FUNCTION(this);
	if (! WRITE_FLOW_DURATION_TO_FILE)
		return;
	// fp1 << "Time(ms),FlowId,FlowDuration(ms), BW(Mbps),FlowEndTime(ms),MarkovState,NextStateChangeTime(ms),ActiveFlowCount\n";
	fp1 << Simulator::Now().ToDouble(Time::US) << "," << flowCount << ","
			<< appDuration << "," << reqBW << "," << appStopTime << ","
			<< modelState << "," << param1 << "," << param2 << ","
			<< m_flowCount - m_totalStoppedFlowCounter << "\n";
}

} // namespace
