/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 University of Washington
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
 */

#ifndef SS_POINT_TO_POINT_CHANNEL_H
#define SS_POINT_TO_POINT_CHANNEL_H

#include <list>
#include "ns3/point-to-point-channel.h"
#include "ns3/ptr.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/traced-callback.h"
#include "ns3/simulator.h"

namespace ns3 {

struct LinkData { // duplicated elsewhere - to avoid unnecessary include headers
	uint64_t m_totalSleepTimeNanoSec;
	uint64_t m_totalIdleTimeNanoSec;
	uint64_t m_totalTransmitTimeNanoSec;
	int m_packetCount;
	int m_countSleepState;
	int m_linkState;
	bool IsLinkUp; // unused
};

struct LinkStatisticsBuffer {
	int* m_stateBuffer;
	uint64_t* m_durationBuffer;
	int m_currentBufferPointer;
	int m_maxBufferSize;
	int m_srcNodeId;
	int m_dstNodeId;

};

class PointToPointNetDevice;
class Packet;

/**
 * \ingroup point-to-point
 * \brief Simple Point To Point Channel.
 *
 * This class represents a very simple point to point channel.  Think full
 * duplex RS-232 or RS-422 with null modem and no handshaking.  There is no
 * multi-drop capability on this channel -- there can be a maximum of two 
 * point-to-point net devices connected.
 *
 * There are two "wires" in the channel.  The first device connected gets the
 * [0] wire to transmit on.  The second device gets the [1] wire.  There is a
 * state (IDLE, TRANSMITTING) associated with each wire.
 *
 * \see Attach
 * \see TransmitStart
 */
class ssPointToPointChannel: public PointToPointChannel {
public:
	/**
	 * \brief Get the TypeId
	 *
	 * \return The TypeId for this class
	 */
	static TypeId GetTypeId(void);

	/**
	 * \brief Create a PointToPointChannel
	 *
	 * By default, you get a channel that has an "infinitely" fast
	 * transmission speed and zero delay.
	 */
	ssPointToPointChannel();
	~ssPointToPointChannel();
	virtual void DoDispose(const Ptr<PointToPointNetDevice> &device);		// sanjeev
	virtual void WriteStatisticsToFile(void); 						// sanjeev
	virtual void DoInitialize(void);								// sanjeev
	/**
	 * \brief Attach a given netdevice to this channel
	 * \param device pointer to the netdevice to attach to the channel
	 */
	virtual void Attach(Ptr<PointToPointNetDevice> device);

	/**
	 * \brief Transmit a packet over this channel
	 * \param p Packet to transmit
	 * \param src Source PointToPointNetDevice
	 * \param txTime Transmit time to apply
	 * \returns true if successful (currently always true)
	 */
	virtual bool TransmitStart(Ptr<Packet> p, Ptr<PointToPointNetDevice> src,
			Time txTime);

	virtual uint32_t GetNDevices(void) const;
	virtual Ptr<PointToPointNetDevice> GetPointToPointDevice(uint32_t i) const;
	virtual Ptr<PointToPointNetDevice> GetDestination(uint32_t i) const;

	void SetDelay(Time t);
	void setLinkWakeupTime(Time wakeUpTime);
	void setCaptureLinkStateBufferSize(int bufferSize);

	LinkStatisticsBuffer* getLinkStatisticsBuffer(int wire);
	LinkData* getLinkData(int wire);
	bool isChannelUp();

	Time t_additionalTime;
	uint64_t t_now, t_duration, t_remainingDuration, t_transmitDuration;
	int t_durationArrayIndex;

	/** \brief Wire states
	 *
	 */

	enum WireState {
		/** Initializing state */
		INITIALIZING = PointToPointChannel::INITIALIZING,

		/** Sanjeev...Sleep Low Power State */
		SLEEPING,
		/** Idle state (no transmission from NetDevice) */
		IDLE,
		/** Transmitting state (data being transmitted from NetDevice. */
		TRANSMITTING,
		/** Propagating state (data is being propagated in the channel. */
		PROPAGATING
	};

// protected:

	virtual Ptr<PointToPointNetDevice> GetSource(uint32_t i) const;
	bool m_DoDisposed;

	class Link {
	public:
		Link();
		~Link();
		void DoDispose(void);
		virtual void WriteStatisticsToFile(void);

		/** \brief Create the link, it will be in INITIALIZING state
		 *
		 */
		WireState m_state; //!< State of the link
		bool m_enableLinkEnergyManagement;
		Ptr<PointToPointNetDevice> m_src; //!< First NetDevice
		Ptr<PointToPointNetDevice> m_dst; //!< Second NetDeviceLinkState

		LinkData linkData; // buffer to capture TOTAL times & states (cumulative)
		uint64_t m_lastUpdateTimeNanoSec; // sanjeev...last link state update time
		int m_srcNodeId; // sanjeev
		int m_dstNodeId; // sanjeev

		EventId m_linkStateUpdateEvent;            // link state update event

		int m_BufferSize;
		// this is a rotating buffer, so if the currentBufferPointer exceeds bufferSize, reset to 0 and start recording again...
		int* m_linkStateBuffer; // we collect the list of all state transitions...
		uint64_t* m_linkDurationBuffer;
		int m_currentBufferPointer;
		LinkStatisticsBuffer* m_linkStatisticsBuffer;

		Time m_linkdelay;    //!< Propagation delay
		Time m_linkStateUpdateInterval;				// link update interval
		Time m_linkWakeUpTimeDelay;				// link wake up time , sanjeev

		// these variables are used temporarily by few methods in this class.
		// avoid instantiating these variables every time in each method,... performance..
		//Time t_additionalTime;
		uint64_t t1_Now, t1_Duration, t1_RemainingDuration, t1_TransmitDuration;

		void startLink();
		void setCaptureLinkStateBufferSize(int bufferSize);
		void recordLinkState(int newState, uint64_t duration);
		void updateLinkState(void);

	};
	Link m_link[N_DEVICES]; //!< Link model
};

}
// namespace ns3

#endif /* SS_POINT_TO_POINT_CHANNEL_H */
