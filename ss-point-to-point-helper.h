/*
 * ssPointToPointHelper.h
 *
 *  Created on: Jul 25, 2016
 *      Author: sanjeev
 */

#ifndef SSPOINTTOPOINTHELPER_H_
#define SSPOINTTOPOINTHELPER_H_

#include <string>

#include "ns3/object-factory.h"
#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/trace-helper.h"
#include "ns3/log.h"

namespace ns3 {

class ssPointToPointHelper : public PointToPointHelper {

public:
	ssPointToPointHelper();
	virtual ~ssPointToPointHelper();

	NetDeviceContainer Install(Ptr<Node> a, Ptr<Node> b);

};

} // namespace

#endif /* SSPOINTTOPOINTHELPER_H_ */
