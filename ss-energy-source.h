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

#ifndef SS_DEVICE_ENERGY_MODEL_SOURCE_H
#define SS_DEVICE_ENERGY_MODEL_SOURCE_H

#include "ns3/netanim-module.h"
#include "ns3/device-energy-model.h"
#include "ns3/nstime.h"
#include "ns3/traced-value.h"
#include "ns3/basic-energy-source.h"

#include "ss-device-energy-model.h"

namespace ns3 {

/**
 * \ingroup energy
 * BasicEnergySource decreases/increases remaining energy stored in itself in
 * linearly.
 *
 */
class ssEnergySource: public BasicEnergySource {
public:
	static TypeId GetTypeId(void);
	ssEnergySource();
	virtual ~ssEnergySource();

	/**
	 * \returns Supply voltage at the energy source.
	 *
	 * Implements GetSupplyVoltage.
	 */
	double GetLowPowerStateVoltage(void);

	/**
	 * \param supplyVoltageV Supply voltage at the energy source, in Volts.
	 *
	 * Sets low supply voltage of the energy source.
	 */
	void SetLowPowerStateVoltage(const double &l);

	/**
	 * \param interval Energy update interval.
	 *
	 * This function sets the interval between each energy update.
	 */
	// void SetEnergyUpdateInterval (Time interval);
	ModelEnergyData* CalculateEnergyConsumed(void);

	DeviceEnergyModelContainer GetDeviceEnergyModels(void);

private:

	/**
	 * Calculates remaining energy. This function uses the total current from all
	 * device models to calculate the amount of energy to decrease. The energy to
	 * decrease is given by:
	 *    energy to decrease = total current * supply voltage * time duration
	 * This function subtracts the calculated energy to decrease from remaining
	 * energy.
	 */
	//void CalculateRemainingEnergy (void);
protected:
	virtual void DoDispose(void);
	virtual void WriteStatisticsToFile(void);
	double m_lowPowerStateVoltageV;                	// supply voltage, in Volts
	ModelEnergyData* m_EnergyDataptr;// to speed up and avoid memory allocation each time..

	// all temp variables used by function, life-span is within a func
	DeviceEnergyModelContainer t_models;
	int t_i, t_nD;
	double t_iL, t_iA, t_sV;
};

} // namespace ns3

#endif /* SS_DEVICE_ENERGY_MODEL_SOURCE_H */
