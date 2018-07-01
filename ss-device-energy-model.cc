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

#include "ns3/simulator.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/energy-source.h"
#include "ns3/log.h"
#include "ss-energy-source.h"
#include "ss-device-energy-model.h"
#include "ss-base-topology-simulation-variables.h"
#include "ss-base-topology.h"
#include "ss-log-file-flags.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("ssDeviceEnergyModel");
NS_OBJECT_ENSURE_REGISTERED(ssDeviceEnergyModel);
/*****************
 *
 *
 *****************/
TypeId ssDeviceEnergyModel::GetTypeId(void) {
	static TypeId tid = TypeId("ns3::ssDeviceEnergyModel").SetParent<
			SimpleDeviceEnergyModel>().SetGroupName("Energy").AddConstructor<
			ssDeviceEnergyModel>();
	return tid;
}

ssDeviceEnergyModel::ssDeviceEnergyModel() :
		SimpleDeviceEnergyModel() {
	NS_LOG_FUNCTION(this);
	m_actualCurrentL = 0.0;
	m_actualCurrentA = 0.0;
	pDevice = 0;
	mData = new ModelEnergyData;
	mData->m_totalConsumedEnergy_nJ = mData->m_totalSleepStateTimeEnergy_nJ =
			mData->m_totalIdleTimeEnergy_nJ =
					mData->m_totalTransmitTimeEnergy_nJ = 0;
	mData->m_totalSleepTimeNanoSec = mData->m_totalIdleTimeNanoSec =
			mData->m_totalTransmitTimeNanoSec =
					mData->m_totalDeviceTimeNanoSec = 0;
	mData->m_packetCount = mData->m_countSleepState = 0;
	// Apr 17, sanjeev. May 1 ..initialization, I dont want to be caught off-guard
	mData->m_DeviceMaxQueueDepth = mData->m_DeviceTotalObservedQueueDepth = 0;
}

ssDeviceEnergyModel::~ssDeviceEnergyModel() {
	NS_LOG_FUNCTION(this);
}

void ssDeviceEnergyModel::WriteStatisticsToFile(void) {
	NS_LOG_FUNCTION(this);
	if (WRITE_DEVICE_ENERGY_TO_FILE) {
		BaseTopology::fpDeviceEnergy << "EnergyModel,Node,"
				<< pDevice->GetNode()->GetId() << ",Device,"
				<< pDevice->GetIfIndex() << ",totalDeviceTime[s],"
				<< mData->m_totalDeviceTimeNanoSec / NanoSec_to_Secs
				<< ",totalTransmitTime[ns],"
				<< mData->m_totalTransmitTimeNanoSec << ",totalIdleTime[ns],"
				<< mData->m_totalIdleTimeNanoSec << ",totalSleepTime[ns],"
				<< mData->m_totalSleepTimeNanoSec << "\n";
	}
}

void ssDeviceEnergyModel::DoDispose(void) {
	NS_LOG_FUNCTION(this);
	WriteStatisticsToFile();
	SS_ENEGRY_LOG(
			"EnergyModel node [" << pDevice->GetNode()->GetId() << "] device [" << pDevice->GetIfIndex() <<
			"] totalDeviceTime [" << mData->m_totalDeviceTimeNanoSec/NanoSec_to_Secs <<
			"s] m_totalTransmitTimeNanoSec [" << mData->m_totalTransmitTimeNanoSec <<
			"] m_totalIdleTimeNanoSec [" << mData->m_totalIdleTimeNanoSec <<
			"] m_totalSleepTimeNanoSec [" << mData->m_totalSleepTimeNanoSec << "]");
}

/*
 *
 */
void ssDeviceEnergyModel::SetNetDevice(
		const Ptr<ssTOSPointToPointNetDevice> &p) {
	pDevice = p;
	pDevice->SetDeviceEnergyModel(this);
}

ModelEnergyData* ssDeviceEnergyModel::CalculateEnergyConsumption() {

	NS_LOG_FUNCTION(this);

	NS_ASSERT_MSG(pDevice != NULL, "Failed to get correct pDevice");

	// update total energy consumption
	//LinkData* d = DynamicCast<ssTOSPointToPointNetDevice>(pDevice)->getLinkData();
	DeviceData* t_d = pDevice->getDeviceData();
	NS_ASSERT_MSG(t_d != NULL, "Failed to get correct netDeviceData");
	NS_ASSERT_MSG(mData != NULL, "ModelEnergyData not initialized");

	mData->m_totalIdleTimeNanoSec = t_d->m_totalIdleTimeNanoSec;
	mData->m_totalSleepTimeNanoSec = t_d->m_totalSleepTimeNanoSec;
	mData->m_totalTransmitTimeNanoSec = t_d->m_totalTransmitTimeNanoSec;
	mData->m_totalDeviceTimeNanoSec = t_d->m_totalDeviceTimeNanoSec;
	mData->m_packetCount = t_d->m_packetCount;
	mData->m_countSleepState = t_d->m_countSleepState;
	mData->m_DeviceMaxQueueDepth = t_d->m_DeviceMaxQueueDepth;
	mData->m_DeviceTotalObservedQueueDepth = t_d->m_DeviceObservedQueueDepth;
	return mData;
}

void ssDeviceEnergyModel::SetCurrentL(const double &current) {
	NS_LOG_FUNCTION(this);
	m_actualCurrentL = current;
}

void ssDeviceEnergyModel::SetCurrents(const double &currentA,
		const double &currentL) {
	NS_LOG_FUNCTION(this);
	m_actualCurrentA = currentA;
	m_actualCurrentL = currentL;
}

double ssDeviceEnergyModel::GetCurrentL(void) {
	return m_actualCurrentL;
}

} // namespace
