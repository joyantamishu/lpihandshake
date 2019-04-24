/*
 * ssPointToPointNetDevice.h
 *
 *  Created on: Jul 25, 2016
 *      Author: sanjeev
 */

#ifndef ssTOSPointToPointNetDevice_H_
#define ssTOSPointToPointNetDevice_H_

#include "ns3/point-to-point-net-device.h"
#include "ss-point-to-point-channel.h"
#include "ss-udp-echo-server.h"

#define Ipv4_Protocol 0x0800  		//IPv4
// we collect device transition statistics...
#define DefaultDataCollectionBufferSize					20					// rotating buffer size to collect stats

#define DELTA_COMMIT_MICROSECOND 0

#define WORST_CASE_SYNC_PACKET_TRAVEL_TIME_MICROSECOND  5


//************************************************************************************************************************************
#define DefaultDeviceRunwayIntervalMicroSec_TOS					5.0
#define DefaultDeviceReadyToTransmitTimeMicroSec_TOS			0.000
#define DefaultDeviceSleepToTransmitTimeMicroSec_TOS			0.1
#define DefaultAdditionalDelayForQueuedPacketsNanoSec_TOS		5

// over-ride these values from base class
#define DefaultDeviceRunwayIntervalMicroSec_TRS					23
#define DefaultDeviceReadyToTransmitTimeMicroSec_TRS			0.000
#define DefaultDeviceSleepToTransmitTimeMicroSec_TRS			0.5
//************************************************************************************************************************************
namespace ns3 {

struct DeviceData {
	uint64_t m_totalSleepTimeNanoSec;
	uint64_t m_totalIdleTimeNanoSec;
	uint64_t m_totalTransmitTimeNanoSec;
	uint64_t m_totalDeviceTimeNanoSec;
	uint64_t m_lastQDepthTimeNanoSec;				// Jun 24
	double t_additionalTimeFromNodeFabricNanoSec;
	double t_additionalTimeFromDeviceStateNanoSec;
	int m_sendPacketCount;
	int m_rcvPacketCount;
	int m_packetCount;
	int m_countSleepState;
	int m_DeviceState;
	int m_DeviceTotalObservedQueueDepthIntegralns;	// Apr 17,May 1, sanjeev, Jun 24
	int m_DeviceMaxQueueDepth;						// Apr 17,May 1 sanjeev
	int m_DeviceObservedQueueDepth;					// Apr 17,May 1 sanjeev
	bool m_isDeviceUp;
};

struct DeviceStatisticsBuffers {
	int* m_stateBuffer;
	uint64_t* m_durationBuffer;
	int m_currentBufferPointer;
	int m_maxBufferSize;
	int m_deviceStateUpdateIntervalNanoSec;
	int m_srcNodeId;
};

/*
500 customers come in , total time = 50millisec
arrival rate = 500/50milli,.... (x customer per sec)
say, total customers response time = 100millisec
response time(incl processing time+waitingtime) = 100milli/500 ; response time = Y
processing time is same as service time
response time (see below)

queue metrics, taking queue length
every time a customer comes, take the current queue lenght * timediff (from last observation);
sum it up till end of simulation...(time T)
everytime a customer is sent out,,,,take the current q lenght metric

throuughput = (time last customer came out - time 1st customer came in)/customer departed
responsetime= queuelength/throughput
*/

struct DeviceLevelQueuingMetrics {
	int64_t m_totalProcessingTime_ns;
	int64_t m_totalWaitingTime_ns;
	int64_t m_firstPacketArriveTime_ns;
	int64_t m_lastPacketDepartTime_ns;
};

class ssDeviceEnergyModel;
class BasicEnergySource;

class ssTOSPointToPointNetDevice: public PointToPointNetDevice {

public:
	/**
	 * \brief Get the TypeId
	 *
	 * \return The TypeId for this class
	 */
	static TypeId GetTypeId(void);

	ssTOSPointToPointNetDevice();
	virtual ~ssTOSPointToPointNetDevice();

	virtual void Receive(Ptr<Packet> p);

	virtual bool Send(Ptr<Packet> packet, const Address &dest,
			uint16_t protocolNumber);

	virtual bool TransmitStart(Ptr<Packet> p);

	virtual void TransmitComplete(void);
	virtual bool Attach(Ptr<PointToPointChannel> ch);

	LinkData* getLinkData(void);

// device state control and state capture part...
	Ipv4Address getIpv4Address(void) const;
	int getNodeId(void) const;
	// get sendChannel dataRate, or rcvChannel dataRate
	int getChannelDataRate(void);

	void setCaptureDeviceStateBufferSize(int bufferSize);
	DeviceStatisticsBuffers* getDeviceStatisticsBuffer();
	DeviceData* getDeviceData(void);
	bool IsDeviceUp(void);

	void SetDeviceEnergyModel(Ptr<ssDeviceEnergyModel> p1);
	void WriteStateDurationEnergyGranular(std::ostream *fpdeviceState,
			std::ostream *fpdeviceEnergy, std::ostream *fpdeviceDuration,
			Ptr<BasicEnergySource> pE);

	// all debug junk here
	static void
	tracePacket(const double &t, const uint32_t &flowId, const uint32_t &packetId,
			const uint64_t& packetGUId, const bool &isLastP,
			const uint32_t &nodeId, const uint32_t &currentQlength,
			const char*nType, const char*nName, const uint32_t &bandwidth);

	virtual double GetSyncPacketTransmissionTime(uint32_t src_node, uint32_t first_received_node, Ptr<const Packet> packet);

protected:
	virtual bool IsDestinationUp_Unused();				// unused...
	Ptr<ssPointToPointChannel> m_channelSS;

	/**
	 * Enumeration of the states of the transmit machine of the net device.
	 */
	enum TxMachineState {
		INIT, SLEEP,		// sanjeev
		READY, /**< The transmitter is ready to begin transmission of a packet */
		BUSY /**< The transmitter is busy transmitting a packet */
	};
	/**
	 * The state of the Net Device transmit state machine.
	 */
	TxMachineState m_txMachineStateSS;
	virtual void HandleDeviceStateChange(TxMachineState newState,
			TxMachineState oldState, bool restartMonitor);
	virtual void DoDispose(void);
	virtual void WriteStatisticsToFile(void);
	virtual void DoInitialize(void);
	virtual void UpdateDeviceState(void);
	virtual void UpdateDeviceQueueData(void);		// Jun 24
	virtual void UpdatePacketMetricsData(const Ptr<const Packet> p);				// Jun 24
	void recordDeviceState(int newState, uint64_t duration);
	void SetDeviceReceiveErrorModel(void);
	// May 1

	virtual bool NetDeviceReceiveCallBack(Ptr<const Packet> p);

	virtual bool NetDeviceSendCallBack(Ptr<const Packet> p);

	virtual void ManageOppurtunisticTransaction(Ptr<const Packet> p);

	virtual void ManageOppurtunisticTransactionv2(Ptr<const Packet> p);

	// added by JB, Mar 17
	virtual uint32_t FindCorrespondingNodeId(Ipv4Address lsa_ip);

	Ipv4Address m_address;
	Ptr<ssDeviceEnergyModel> m_pEnergyModel;
	DeviceData* m_deviceData;	// device data collected from THIS device
	LinkData* m_linkData;// link data collected from the lower level channel & links.
	int m_nodeId;
	int m_rcvPacketcount;
	int m_sendPacketcount;
	int m_countSleepState;
	uint64_t m_totalSleepTimeNanoSec;
	uint64_t m_totalIdleTimeNanoSec;
	uint64_t m_totalTransmitTimeNanoSec;
	int m_BufferSize;
	int m_currentBufferPoint;
	bool m_enableDeviceEnergyManagement;

	// this is a rotating buffer, so if the currentBufferPointer exceeds bufferSize, reset to 0 and start recording again...
	int* m_deviceStateBuffer; // we collect the list of all state transitions...
	uint64_t* m_deviceDurationBuffer;
	DeviceStatisticsBuffers *m_deviceStatisticsBuffer;
	//static QueueMeasurement Myqueue; //Madhurima added on May 5th for queuedepth
	uint64_t m_lastUpdateTimeNanoSec; // sanjeev...last device state update time

	EventId m_deviceStateUpdateEvent;            // device state update event
	Time m_deviceStateUpdateInterval;				// device update interval
	Time m_device_Ready_To_Transmit_TimeDelay;// device wake up time , sanjeev, Apr 19
	Time m_device_Sleep_To_Transmit_TimeDelay;// device wake up time , sanjeev, Apr 19
	Time m_queuedPacketTxnDelayTimeNanoSec; // inter packet delay for queued packets only, As per MR, May 3
	Time m_additionalTimeFromNodeFabric;	// sanjeev Jun 18..additional time from node fabric, if ANY?
	Time m_additionalTimeFromDeviceState;	// sanjeev Jun 18..additional time from device state transition if any...

// these variables are used temporarily by few methods in this class.
// avoid instantiating these variables everytime in each method,... performance..
	uint64_t t_now, t_duration, t_remainingDuration, t_transmitDuration;
	uint64_t t_durationArrayIndex;

	DeviceLevelQueuingMetrics m_deviceQMetrics;	// jun 26

	char concurrency_hash_entry [50];
};

} //namespace

#endif /* ssTOSPointToPointNetDevice_H_ */
