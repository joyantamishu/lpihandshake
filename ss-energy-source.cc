/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Andrea Sacco
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Andrea Sacco <andrea.sacco85@gmail.com>
 */

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/trace-source-accessor.h"

#include "ss-energy-source.h"
#include "ss-base-topology-simulation-variables.h"
#include "ss-base-topology.h"
#include "ss-log-file-flags.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("ssEnergySource");
NS_OBJECT_ENSURE_REGISTERED(ssEnergySource);

TypeId ssEnergySource::GetTypeId(void) {
	static TypeId tid = TypeId("ns3::ssEnergySource").SetParent<
			BasicEnergySource>().SetGroupName("Energy").AddConstructor<
			ssEnergySource>();
	return tid;
}

ssEnergySource::ssEnergySource() {
	NS_LOG_FUNCTION(this);
	m_lowPowerStateVoltageV = 0;
	DeviceEnergyModelContainer t_models;
	t_i = 0;
	t_nD = 0;
	t_iL = 0;
	t_iA = 0, t_sV = 0;
	m_EnergyDataptr = new ModelEnergyData;

	// disable base class simulate event with some junk value, after sim_end_time...
	SetEnergyUpdateInterval(
			Seconds(
					simulationRunProperties::simStop
							* simulationRunProperties::simStop));
}

ssEnergySource::~ssEnergySource() {
	NS_LOG_FUNCTION(this);
}

void ssEnergySource::WriteStatisticsToFile(void) {
	NS_LOG_FUNCTION(this);
	if (WRITE_DEVICE_ENERGY_TO_FILE) {
		BaseTopology::fpDeviceEnergy << "ssEnergySrc,Node,"
				<< GetNode()->GetId() << ",Device,ALL,totalTime[s],"
				<< m_EnergyDataptr->m_totalDeviceTimeNanoSec / NanoSec_to_Secs
				<< ",A_totalTransmitTime[ns],"
				<< m_EnergyDataptr->m_totalTransmitTimeNanoSec
				<< ",A_totalIdleTime[ns],"
				<< m_EnergyDataptr->m_totalIdleTimeNanoSec
				<< ",A_totalSleepTime[ns],"
				<< m_EnergyDataptr->m_totalSleepTimeNanoSec << "\n";
		BaseTopology::fpDeviceEnergy << "ssEnergySrc,Node,"
				<< GetNode()->GetId() << ",Device,ALL,totalConsumedEnergyJ,"
				<< m_EnergyDataptr->m_totalConsumedEnergy_nJ / NanoSec_to_Secs
				<< ",totalTransmitTimeEnergyJ,"
				<< m_EnergyDataptr->m_totalTransmitTimeEnergy_nJ
						/ NanoSec_to_Secs << ",totalSleepStateTimeEnergyJ,"
				<< m_EnergyDataptr->m_totalSleepStateTimeEnergy_nJ
						/ NanoSec_to_Secs << ",totalIdleTimeEnergyJ,"
				<< m_EnergyDataptr->m_totalIdleTimeEnergy_nJ / NanoSec_to_Secs
				<< "\n";
	}
}

void ssEnergySource::DoDispose() {
	NS_LOG_FUNCTION(this);
	WriteStatisticsToFile();
	SS_ENEGRY_LOG(
			"ssEnergySrc node [" << GetNode()->GetId() << "] device ALL"
			<< " totalDeviceTime [" << m_EnergyDataptr->m_totalDeviceTimeNanoSec/NanoSec_to_Secs
			<< "s] A_totalTransmitTime [" << m_EnergyDataptr->m_totalTransmitTimeNanoSec/NanoSec_to_Secs
			<< "s] A_totalIdleTime [" << m_EnergyDataptr->m_totalIdleTimeNanoSec/NanoSec_to_Secs
			<< "s] A_totalSleepTime [" << m_EnergyDataptr->m_totalSleepTimeNanoSec/NanoSec_to_Secs
			<< "s]");
	SS_ENEGRY_LOG(
			"ssEnergySrc node [" << GetNode()->GetId() << "] device ALL"
			<< " m_totalConsumedEnergyJ [" << m_EnergyDataptr->m_totalConsumedEnergy_nJ /NanoSec_to_Secs<< "]"
			<< " m_totalTransmitTimeEnergyJ [" << m_EnergyDataptr->m_totalTransmitTimeEnergy_nJ/NanoSec_to_Secs << "]"
			<< " m_totalSleepStateTimeEnergyJ [" << m_EnergyDataptr->m_totalSleepStateTimeEnergy_nJ/NanoSec_to_Secs << "]"
			<< " m_totalIdleTimeEnergyJ [" << m_EnergyDataptr->m_totalIdleTimeEnergy_nJ/NanoSec_to_Secs << "]\n");
}

DeviceEnergyModelContainer ssEnergySource::GetDeviceEnergyModels(void) {
	return EnergySource::m_models;
}

ModelEnergyData* ssEnergySource::CalculateEnergyConsumed(void) {
	NS_LOG_FUNCTION(this);
	// free last pointer contents,  clear each field.
	m_EnergyDataptr->m_totalConsumedEnergy_nJ = 0;
	m_EnergyDataptr->m_totalIdleTimeEnergy_nJ = 0;
	m_EnergyDataptr->m_totalTransmitTimeEnergy_nJ = 0;
	m_EnergyDataptr->m_totalSleepStateTimeEnergy_nJ = 0;
	m_EnergyDataptr->m_totalIdleTimeNanoSec = 0;
	m_EnergyDataptr->m_totalSleepTimeNanoSec = 0;
	m_EnergyDataptr->m_totalTransmitTimeNanoSec = 0;
	m_EnergyDataptr->m_totalDeviceTimeNanoSec = 0;
	m_EnergyDataptr->m_packetCount = 0;
	m_EnergyDataptr->m_countSleepState = 0;
	m_EnergyDataptr->m_DeviceMaxQueueDepth = 0;
	m_EnergyDataptr->m_DeviceTotalObservedQueueDepth = 0;

	t_models = GetDeviceEnergyModels();
	t_nD = t_models.GetN();
	t_sV = BasicEnergySource::GetSupplyVoltage();

	NS_ASSERT_MSG(t_nD >= 1, "Failed to get correct ssDeviceEnergyModel");

	for (t_i = 0; t_i < t_nD; t_i++) {
		Ptr<ssDeviceEnergyModel> t_ptrModel = DynamicCast<ssDeviceEnergyModel>(
				t_models.Get(t_i));
		NS_ASSERT_MSG(t_ptrModel != NULL,
				"Failed to get correct DeviceEnergyModel");
		ModelEnergyData* t_modelEnergyData =
				t_ptrModel->CalculateEnergyConsumption();
		NS_ASSERT_MSG(t_modelEnergyData != NULL,
				"Failed to get correct ModelEnergyData");

		t_iA = t_ptrModel->DeviceEnergyModel::GetCurrentA();

		//ENABLE_DEVICE_ENERGY_MANAGEMENT
		if (simulationRunProperties::enableDeviceTOSManagement) {
			t_iL = t_ptrModel->GetCurrentL();
		} else {
			t_iL = t_iA;
			m_lowPowerStateVoltageV = t_sV;
		}

//***********************************This code has been changed by madhurima as on April 21*******************************************************
		//m_EnergyDataptr->m_totalIdleTimeEnergy_nJ +=
		//	(t_modelEnergyData->m_totalIdleTimeNanoSec * t_sV * t_iL);
		m_EnergyDataptr->m_totalIdleTimeEnergy_nJ +=
				(t_modelEnergyData->m_totalIdleTimeNanoSec * t_sV * t_iA); //changed the idle current same as the active current
//************************************************************************************************************************************************
		m_EnergyDataptr->m_totalTransmitTimeEnergy_nJ +=
				(t_modelEnergyData->m_totalTransmitTimeNanoSec * t_sV * t_iA);
		m_EnergyDataptr->m_totalSleepStateTimeEnergy_nJ +=
				(t_modelEnergyData->m_totalSleepTimeNanoSec
						* (m_lowPowerStateVoltageV * t_iL));

		m_EnergyDataptr->m_packetCount += t_modelEnergyData->m_packetCount;
		m_EnergyDataptr->m_countSleepState +=
				t_modelEnergyData->m_countSleepState;

		m_EnergyDataptr->m_totalDeviceTimeNanoSec +=
				t_modelEnergyData->m_totalDeviceTimeNanoSec;
		m_EnergyDataptr->m_totalIdleTimeNanoSec +=
				t_modelEnergyData->m_totalIdleTimeNanoSec;
		m_EnergyDataptr->m_totalSleepTimeNanoSec +=
				t_modelEnergyData->m_totalSleepTimeNanoSec;
		m_EnergyDataptr->m_totalTransmitTimeNanoSec +=
				t_modelEnergyData->m_totalTransmitTimeNanoSec;
		if (m_EnergyDataptr->m_DeviceMaxQueueDepth
				< t_modelEnergyData->m_DeviceMaxQueueDepth)
			m_EnergyDataptr->m_DeviceMaxQueueDepth =
					t_modelEnergyData->m_DeviceMaxQueueDepth;
		m_EnergyDataptr->m_DeviceTotalObservedQueueDepth +=
				t_modelEnergyData->m_DeviceTotalObservedQueueDepth;
	}
	m_EnergyDataptr->m_totalConsumedEnergy_nJ =
			m_EnergyDataptr->m_totalSleepStateTimeEnergy_nJ
					+ m_EnergyDataptr->m_totalIdleTimeEnergy_nJ
					+ m_EnergyDataptr->m_totalTransmitTimeEnergy_nJ;

	return m_EnergyDataptr;
}

void ssEnergySource::SetLowPowerStateVoltage(const double &p1) {
	NS_LOG_FUNCTION(this);
	m_lowPowerStateVoltageV = p1;
}

double ssEnergySource::GetLowPowerStateVoltage(void) {
	NS_LOG_FUNCTION(this);
	return m_lowPowerStateVoltageV;
}

}  // namespace
