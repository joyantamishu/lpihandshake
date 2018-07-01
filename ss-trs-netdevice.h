/*
 * ssTRSNetDevice.h
 *
 *  Created on: Jul 25, 2016
 *      Author: sanjeev
 */

#ifndef ssTRSNetDevice_H_
#define ssTRSNetDevice_H_

#include "ns3/point-to-point-net-device.h"
#include "ns3/point-to-point-channel.h"
#include "ns3/event-id.h"
#include "ss-tos-point-to-point-netdevice.h"

#define wakeUpMessage_STRING	"WAKEUP"
#define	iamUpMessage_STRING 	"I'M_UP"
#define iamAsleep_STRING  		"SLEEP"
#define STRING_LENGTH1			64

namespace ns3 {
class ssTRSNetDevice: public ssTOSPointToPointNetDevice {

public:
	/**
	 * \brief Get the TypeId
	 *
	 * \return The TypeId for this class
	 */
	static TypeId GetTypeId(void);

	ssTRSNetDevice();
	virtual ~ssTRSNetDevice();

	virtual void Receive(Ptr<Packet> p);

	virtual bool Send(Ptr<Packet> packet, const Address &dest,
			uint16_t protocolNumber);

protected:

	virtual void DoDispose(void);
	virtual void WriteStatisticsToFile(void);
	virtual bool IsDestinationUp(void);				// ...here...
	virtual void HandleDeviceStateChange(TxMachineState newState,
			TxMachineState oldState, bool restartMonitor);
	virtual void SendRemainingPackets(void);
	virtual void lpiTransmitSpecialPacket(const char* pMagicPacket, Time processingTime);

	// FOR ::LPI:: MODELLING
	int m_wakeUpPacketCount, m_IamAwakePacketCount, m_warningPacketCount;
	bool m_isDestinationUp;
	Time m_SleepMagicPacketProcessingTime;	// device send sleep magic packet time (txn+process), sanjeev Apr 19
	Time m_WakeUpMagicPacketProcessingTime;	// device awake sleep magic packet time (txn+process), sanjeev Apr 19
	Time m_IamUpMagicPacketProcessingTime;	// device IamUp sleep magic packet time (txn+process), sanjeev Apr 19
	Time m_additionalTimeToWakeUpNodeFabricNanoSec;	// logging only , sanjeev Jun 18
	Ptr<Packet> m_magicPacket;
	bool m_IsentWakeUpMagicPacket;
	const char *wakeUpMessage;
	const char *iamUpMessage;
	const char *iamAsleep;
	uint8_t copyBuffer[STRING_LENGTH1];

	void SendAsleepMagicPacket(void);
	void SendWakeUpMagicPacket(void);
	void SendIamReadyMagicPacket(void);
	bool IsWakeUpMagicPacket(Ptr<Packet> packet);
	bool IsIamReadyMagicPacket(Ptr<Packet> packet);
	bool IsAsleepMagicPacket(Ptr<Packet> packet);

};

} //namespace

#endif /* ssTRSNetDevice_H_ */
