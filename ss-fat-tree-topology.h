/*
 * ss-fat-tree-topology.h
 *
 *  Created on: Jul 24, 2016
 *      Author: sanjeev
 */

#ifndef SS_FATTREE_TOPOLOGY_H_
#define SS_FATTREE_TOPOLOGY_H_

#define MAXCHAR 			10000

#include "ss-base-topology.h"

#include "parameters.h"

namespace ns3 {

class FatTreeTopology: public BaseTopology {
public:
	/**
	 * \brief Constructor. (k = number of hosts per pod)
	 */
	FatTreeTopology(void);

	virtual ~FatTreeTopology(void);

protected:
	/*
	 * * Called from Constructor, k = number of pods !!!
	 * ***** Core function, builds initial fat tree *****
	 */
	virtual void BuildInitialTopology(void);
	virtual void SetNodeAnimPosition(void);
	virtual void SetUpInitialBandwidthMetric(Ipv4Address ip1, Ipv4Address ip2,
			uint32_t total_bandwidth, uint32_t node_id1, uint32_t node_id2);

	/******Chunk Specific Change *******************/
	virtual void SetUpInitialChunkPosition();
	virtual Ipv4Address GetIpv4Address(uint32_t node_id);

	virtual void SetUpApplicationAssignment();

	virtual void SetUpInitialApplicationPosition();

	virtual void SetUpNodeUtilizationStatistics();

	virtual void SetUpInitialOpmizationVariables();

	virtual void SetUpIntensityPhraseChangeVariables();

	virtual void SetUpRealTracesVariables();

	/***********************************************/

	int hosts_per_pod;	// number of hosts switch in a pod
	int n_edge_routers;	// number of bridge in a pod
	int n_aggregate_routers;	// number of core switch in a group
	int n_core_routers;	// no of core switches
// these are TOTAL nodes....
	int total_hosts;	// number of hosts in the entire network
	int total_aggregate_routers;
	int total_edge_routers;
	int total_core_routers;
	int hosts_per_edge;

	int hypercuber_index;

// all these are TOTAL nodes in topology.....
	NodeContainer coreRouters;
	NodeContainer aggrRouters;
	NodeContainer edgeRouters;
	NodeContainer levelRouters1;
	NodeContainer levelRouters2;
	NodeContainer hostNodes;

};

}  //namespace

#endif /* SS_FATTREE_TOPOLOGY_H_ */
