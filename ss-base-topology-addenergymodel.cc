/*
 * ss-fat-tree-addenergymodel.cc
 *
 *  Created on: Sep 1, 2016
 *      Author: sanjeev
 */

#include "ns3/log.h"
#include "parameters.h"
#include "ss-base-topology.h"
#include "ss-device-energy-model.h"
#include "ss-energy-source.h"
#include "ss-node.h"
#include "ss-tos-point-to-point-netdevice.h"

// Energy source/model values
#define DefaultInitialEnergy_ns3					10000.0
#define DefaultInitialVoltage_ns3					1.10
//************************************************************************************************************************************
// Sanjeev:: May 26
#define DefaultInitialVoltage_Fabric					1.10
#define DefaultActiveCurrentA_Fabric					54.5//4.54545  Madhurima Changed the parameter as on April 21
#define	DefaultLeakageCurrentA_Fabric					0.4*DefaultActiveCurrentA_Fabric

//************************************************************************************************************************************
#define DefaultActiveCurrentA_ns3					13.5//4.54545   Madhurima Changed the parameter as on April 21

#define DefaultInitialVoltage_TOS					1.10
#define DefaultSleepStateVoltage_TOS				1.10 //Madhurima Changed the value on May 7  earlier it was .4......0.70   Madhurima Changed the parameter as on April 21
#define DefaultActiveCurrentA_TOS					13.5//4.54545  Madhurima Changed the parameter as on April 21
#define	DefaultLeakageCurrentA_TOS					0.4*DefaultActiveCurrentA_TOS

#define DefaultInitialVoltage_TRS					1.10
#define DefaultSleepStateVoltage_TRS				1.10 //Madhurima Changed the value on May 7  earlier it was .1......0.70     Madhurima Changed the parameter as on April 21
#define DefaultActiveCurrentA_TRS					13.5 //4.54545  Madhurima Changed the parameter as on April 21
#define	DefaultLeakageCurrentA_TRS					0.1*DefaultActiveCurrentA_TRS
//************************************************************************************************************************************
#define DefaultEnergyMeterUpdateIntervalSec		0.01			// sanjeev. mar 23

namespace ns3 {
NS_LOG_COMPONENT_DEFINE("BaseTopologyE");

void BaseTopology::AddEnergyModel(void) {
	//USE_DEFAULT_NS3_ENERGY_MODEL
	if (simulationRunProperties::enableDefaultns3EnergyModel)
		AddEnergyModelns3();
	else
		AddEnergyModelCustom();
}

/***************************************************************************/
/** Using Standard Energy Model from ns3 package **/
/***************************************************************************/
void BaseTopology::AddEnergyModelns3() {
	NS_LOG_FUNCTION(this);
	int i, j, nD, nn = allNodes.GetN();
	Ptr<Node> pNode;
	Ptr<NetDevice> pNetDevice;
	Ptr<BasicEnergySource> pEnergySource;
	Ptr<SimpleDeviceEnergyModel> model;

	/* energy source for each node*/
	for (i = 0; i < nn; i++) {
		pNode = allNodes.Get(i);
		// device energy model
		pEnergySource = CreateObject<BasicEnergySource>();
		NS_ASSERT(pEnergySource !=NULL);
		pEnergySource->SetInitialEnergy(DefaultInitialEnergy_ns3);
		pEnergySource->SetSupplyVoltage(DefaultInitialVoltage_ns3);
		pEnergySource->SetNode(pNode);
		pEnergySource->SetEnergyUpdateInterval(
				Seconds(DefaultEnergyMeterUpdateIntervalSec));
		DynamicCast<ssNode>(pNode)->SetEnergySource(pEnergySource);
		nD = pNode->GetNDevices();
		//
		// remember - zero is loop back connector..
		//
		for (j = 1; j < nD; j++) {
			pNetDevice = pNode->GetDevice(j);
			model = CreateObject<SimpleDeviceEnergyModel>();
			NS_ASSERT(model !=NULL);
			// set energy source pointer
			// note: these two statements should come in this sequence, else coredumps
			model->SetEnergySource(pEnergySource);
			model->SetCurrentA(DefaultActiveCurrentA_ns3);

			pEnergySource->AppendDeviceEnergyModel(model);

			NS_LOG_LOGIC(
					"Installed energy source [" << pEnergySource->GetTypeId().GetName() << "] on node [" << pNode->GetId() << "] & Model [" << model->GetInstanceTypeId().GetName() << "] on Device [" << pNetDevice->GetInstanceTypeId().GetName() << " " << j << "] Channel [" << pNetDevice->GetChannel()->GetInstanceTypeId().GetName());
		}
	}

}

/***************************************************************************/
/** Energy Model **/
/***************************************************************************/
void BaseTopology::AddEnergyModelCustom() {
	NS_LOG_FUNCTION(this);
	int i, j, nD, nn = allNodes.GetN();
	Ptr<Node> pNode;
	Ptr<NetDevice> pNetDevice;
	Ptr<ssEnergySource> pEnergySource;
	Ptr<ssDeviceEnergyModel> model;
	double iActiveCurrentAmps, iPassiveCurrentAmps, initialVoltage,
			sleepStateVoltage;
	if (simulationRunProperties::enableDeviceTOSManagement
			&& !simulationRunProperties::enableDeviceTRSManagement) { //Madhurima Changed the line on May 7 the if condition was incorrect
		iPassiveCurrentAmps = DefaultLeakageCurrentA_TOS;
		iActiveCurrentAmps = DefaultActiveCurrentA_TOS;
		initialVoltage = DefaultInitialVoltage_TOS;
		sleepStateVoltage = DefaultSleepStateVoltage_TOS;
	} else if (simulationRunProperties::enableDeviceTRSManagement
			&& simulationRunProperties::enableDeviceTOSManagement) { //Madhurima Changed the line on May 7 the if condition was incorrect
		iPassiveCurrentAmps = DefaultLeakageCurrentA_TRS;
		iActiveCurrentAmps = DefaultActiveCurrentA_TRS;
		initialVoltage = DefaultInitialVoltage_TRS;
		sleepStateVoltage = DefaultSleepStateVoltage_TRS;
	} else
		NS_ABORT_MSG(
				"simulationRunProperties for EnergyModel is not set correctly");
	/* energy source for each node*/
	for (i = 0; i < nn; i++) {
		pNode = allNodes.Get(i);
		// device energy model
		pEnergySource = CreateObject<ssEnergySource>();
		NS_ASSERT(pEnergySource !=NULL);
		pEnergySource->SetSupplyVoltage(initialVoltage);
		pEnergySource->SetLowPowerStateVoltage(sleepStateVoltage);
		pEnergySource->SetNode(pNode);

		DynamicCast<ssNode>(pNode)->SetEnergySource(pEnergySource);
		nD = pNode->GetNDevices();
		//
		// remember - zero is loop back connector..
		//
		for (j = 1; j < nD; j++) {
			pNetDevice = pNode->GetDevice(j);
			model = CreateObject<ssDeviceEnergyModel>();
			NS_ASSERT(model !=NULL);
			// set energy source pointer
			// note: these two statements should come in this sequence, else coredumps
			model->SetEnergySource(pEnergySource);
			model->SetCurrents(iActiveCurrentAmps, iPassiveCurrentAmps);
			model->SetNetDevice(
					DynamicCast<ssTOSPointToPointNetDevice>(pNetDevice));
			pEnergySource->AppendDeviceEnergyModel(model);

			NS_LOG_LOGIC(
					"Installed energy source [" << pEnergySource->GetInstanceTypeId().GetName() << "] on node [" << pNode->GetId() << "] & Model [" << model->GetInstanceTypeId().GetName() << "] on Device [" << pNetDevice->GetInstanceTypeId().GetName() << " " << j << "] Channel [" << pNetDevice->GetChannel()->GetInstanceTypeId().GetName());
		}
	}

}
/*
 * Jun 13, sanjeev - this method is NOT used
 */
double BaseTopology::GetNodeFabricEnergy_NotUsed(Ptr<ssNode> pNode) {
	NS_LOG_FUNCTION(this);

	if (COMPLETELY_DISABLE_NODE_FABRIC_ENERGY_MODEL)
		return 0;

	double nodeFabricVoltage = DefaultInitialVoltage_Fabric;
	double nodeFabricLeakageCurrent = DefaultLeakageCurrentA_Fabric;
	double nodeFabricActiveCurrent = DefaultActiveCurrentA_Fabric;

	//
	if (!simulationRunProperties::enableNodeFabricManagement)
		nodeFabricLeakageCurrent = nodeFabricActiveCurrent;
	//
	pNode->ComputeLatestStateDurationCalculations();

	double tFabric_ReadyStateEnergy =
			pNode->m_NodeDataCollected.m_totalReadyTimeNanoSec
					* nodeFabricVoltage * nodeFabricActiveCurrent;
	double tFabric_SleepStateEnergy =
			pNode->m_NodeDataCollected.m_totalSleepTimeNanoSec
					* nodeFabricVoltage * nodeFabricLeakageCurrent;
	// return '0' if you want to test without this energy..
	return tFabric_ReadyStateEnergy + tFabric_SleepStateEnergy;
}

}
// namespace
