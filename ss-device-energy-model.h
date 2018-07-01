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

#ifndef SS_DEVICE_ENERGY_MODEL_H
#define SS_DEVICE_ENERGY_MODEL_H

#include "ns3/simple-device-energy-model.h"
#include "ns3/nstime.h"
#include "ns3/traced-value.h"
#include "ss-tos-point-to-point-netdevice.h"

namespace ns3 {

struct ModelEnergyData {
	double m_totalConsumedEnergy_nJ; // used by energy source, not energy model - design issue?
	double m_totalSleepStateTimeEnergy_nJ; // used by energy source, not energy model - design issue?
	double m_totalIdleTimeEnergy_nJ; // used by energy source, not energy model - design issue?
	double m_totalTransmitTimeEnergy_nJ; // used by energy source, not energy model - design issue?
	uint64_t m_totalSleepTimeNanoSec;
	uint64_t m_totalIdleTimeNanoSec;
	uint64_t m_totalTransmitTimeNanoSec;
	uint64_t m_totalDeviceTimeNanoSec;
	int m_packetCount;
	int m_countSleepState;
	int m_DeviceMaxQueueDepth;		// Apr 17, sanjeev
	int m_DeviceTotalObservedQueueDepth;			// Apr 17,May 1, sanjeev
};

class ssTOSPointToPointNetDevice;

/**
 * \ingroup energy
 *
 * A simple device energy model where current drain can be set by the user.
 *
 * It is supposed to be used as a testing model for energy sources.
 *
 */
class ssDeviceEnergyModel: public SimpleDeviceEnergyModel {
public:
	static TypeId GetTypeId(void);
	ssDeviceEnergyModel();
	virtual ~ssDeviceEnergyModel();
	virtual void DoDispose(void);
	/**
	 * called by netAnim-->node, to calc the total processing energy
	 * includes both active + leakage energy
	 * Ideally, in good design this should be in the node....but my laziness
	 */
	ModelEnergyData* CalculateEnergyConsumption();

	/**
	 * \param current the current draw of device.
	 * Set the LEAKAGE current draw of the device.
	 */
	void SetCurrentL(const double &current);
	void SetCurrents(const double &currentA, const double &currentL);
	void SetNetDevice(const Ptr<ssTOSPointToPointNetDevice> &p);

	double GetCurrentL(void);

protected:
	virtual void WriteStatisticsToFile(void);
	double m_actualCurrentL;	 // leakage current while idle..
	Ptr<ssTOSPointToPointNetDevice> pDevice;
	ModelEnergyData *mData;
};

} // namespace ns3

#endif /* SS_DEVICE_ENERGY_MODEL_SOURCE_H */
