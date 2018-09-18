/*
 * ss-fat-tree-topology-callback.cc
 *
 *  Created on: Jul 12, 2016
 *      Author: sanjeev
 */

#include "ns3/log.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv4-global-routing.h"
#include "ns3/node-list.h"
#include "ss-base-topology.h"
#include "ss-node.h"
#include "ss-tos-point-to-point-netdevice.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("BaseTopologyC");
/*
 * Very careful fine tuning.. It calls the func -but return value is important for executing default ns3_routing_logic
 */

//
// Callback to invoke whenever a packet has been received and must be forwarded to the higher layers.
//
// Set the callback to be used to notify higher layers when a packet has been received.
//
// old code delete this block after Mar 17
// sanjeev mar 18, removed old code block
bool ssTOSPointToPointNetDevice::NetDeviceReceiveCallBack(
		Ptr<const Packet> packet) {
	//NS_ASSERT(simulationRunProperties::enableCustomRouting);
	//NS_LOG_UNCOND("Hello Callback");
	NS_LOG_FUNCTION(
			this << "Sanjeev: Add your custom ROUTING logic, and decide how to RECEIVE this packet routing");
#if COMPILE_CUSTOM_ROUTING_CODE
	Ptr<Node> n = GetNode();

	//NS_LOG_UNCOND("The node id is "<<n->GetId());

	flow_info flow_entry;
	//
	// Major: Sanjeev: May 9: Remmoved dependency on FlowTag completely. I get all values from packet directly....
	//
	Ipv4Address m_src = DynamicCast<ssNode>(
			NodeList::GetNode(packet->srcNodeId))->GetNodeIpv4Address();
	Ipv4Address m_dst = DynamicCast<ssNode>(
			NodeList::GetNode(packet->dstNodeId))->GetNodeIpv4Address();
	uint32_t m_flowId = packet->flow_id;
	uint16_t m_requiredBW = packet->required_bandwidth;
	bool m_lastPacket = packet->is_Last;

	//NS_LOG_UNCOND("Packet required bw "<< pFlowInfo1.m_requiredBW);

	if (Ipv4GlobalRouting::flow_map.count(m_flowId) <= 0)
	{
		flow_entry = flow_info();
		flow_entry.set_flow_id(m_flowId);
		flow_entry.set_source(m_src);
		flow_entry.set_destination(m_dst);
		flow_entry.set_bandwidth(m_requiredBW);
		flow_entry.destination_node = FindCorrespondingNodeId(m_dst);
		Ipv4GlobalRouting::flow_map[m_flowId] = flow_entry;
	}
	else
	{

		//NS_LOG_UNCOND("destination_node "<<packet->dstNodeId);
		if (packet->sub_flow_dest_physical == n->GetId()) //packet has reached the destination
		{

			//NS_LOG_UNCOND("packet->sub_flow_dest_physical "<<packet->sub_flow_dest_physical);

			BaseTopology::total_packet_count--;

			BaseTopology::total_packets_to_hosts_bits[packet->sub_flow_dest] ++;

			//NS_LOG_UNCOND("total_packet_count "<<BaseTopology::total_packet_count);
			//NS_LOG_UNCOND("The Destination has reached");
			Ipv4GlobalRouting::flow_map.at(m_flowId).total_packet_to_destination = Ipv4GlobalRouting::flow_map.at(m_flowId).total_packet_to_destination +1;

			double current_simulation_time = Simulator::Now().ToDouble(Time::US);

			double delay_by_packet = current_simulation_time - packet->creation_time;

			Ipv4GlobalRouting::flow_map.at(m_flowId).delaysum += delay_by_packet + DEFAULT_LOCAL_ACCESS_LATENCY;

			//NS_LOG_UNCOND("BaseTopology::sum_delay_ms "<<BaseTopology::sum_delay_ms);

			if(Ipv4GlobalRouting::has_system_learnt)
			{



				if(BaseTopology::last_point_of_entry <= 0.0)
				{
					BaseTopology::last_point_of_entry = Simulator::Now().ToDouble(Time::MS);
				}
				else
				{
					double time_diff = Simulator::Now().ToDouble(Time::MS) - BaseTopology::last_point_of_entry;

					BaseTopology::last_point_of_entry = Simulator::Now().ToDouble(Time::MS);

					BaseTopology::sum_of_number_time_packets += (BaseTopology::total_packet_count + 1) * time_diff;

					BaseTopology::total_sum_of_entry += time_diff;
				}




				BaseTopology::sum_delay_ms += current_simulation_time - packet->creation_time;

				BaseTopology::total_events_learnt++;

				BaseTopology::total_packet_count_inc += BaseTopology::total_packet_count;

				BaseTopology::total_events++;

			}

			if(packet->is_phrase_changed)
			{
				BaseTopology::sum_delay_ms_burst += current_simulation_time - packet->creation_time;

				BaseTopology::total_events_learnt_burst++;
			}
			else
			{
				BaseTopology::sum_delay_ms_no_burst += current_simulation_time - packet->creation_time;

				BaseTopology::total_events_learnt_no_burst++;
			}

			BaseTopology::total_packets_to_chunk_destination[packet->sub_flow_id]++;



			//NS_LOG_UNCOND("delay_by_packet "<<delay_by_packet);

			// gonna change it later
//			if(BaseTopology::chunkTracker.at(packet->sub_flow_id).number_of_copy > 0 && !m_lastPacket)
//			{
//				std::vector<MultipleCopyOfChunkInfo>::iterator itr = BaseTopology::chunkCopyLocations.begin();
//
//				for( ; itr != BaseTopology::chunkCopyLocations.end(); ++itr)
//				{
//					//track the retrying packets
//					if(itr->chunk_id == packet->sub_flow_id && itr->location == packet->sub_flow_dest)
//					{
//						//NS_LOG_UNCOND("&&&&&&&&&&^^^^^^^^^^^^&(&&&&&&&&&^^%^"<<packet->sub_flow_id<<" "<<packet->sub_flow_dest);
//						break;
//
//					}
//				}
//
//				if(packet->is_write)
//				{
//					if(BaseTopology::chunkTracker.at(packet->sub_flow_id).version_number == itr->version)
//					{
//						BaseTopology::chunkTracker.at(packet->sub_flow_id).version_number++;
//						//itr->version++;
//					}
//					if(BaseTopology::chunkTracker.at(packet->sub_flow_id).version_number > itr->version) itr->version++;
//
//
//
//				}
//
//				else
//				{
//
//					if(BaseTopology::chunkTracker.at(packet->sub_flow_id).version_number > itr->version)
//					{
//						//NS_LOG_UNCOND("&&&&&&&&&&%^^^^^^^^^^^^&(&&&&&&&&&^^%^ Read version "<<BaseTopology::chunkTracker.at(packet->sub_flow_id).version_number<<" "<<itr->version);
//						BaseTopology::num_of_retried_packets++;
//
//
//					}
//				}
//			}

			//NS_LOG_UNCOND("creation_time "<<packet->creation_time);
			if (m_lastPacket) {
				double delay = Ipv4GlobalRouting::flow_map.at(m_flowId).delaysum/Ipv4GlobalRouting::flow_map.at(m_flowId).total_packet_to_destination;
				NS_LOG_UNCOND("^^^^^^^^^^The avg delay is^^^^^^^^^^^^^^^ "<<delay);
				BaseTopology::application_metrics[packet->application_id].avg_delay += delay;
				BaseTopology::application_metrics[packet->application_id].total_flows++;
				//NS_LOG_UNCOND("The application id is "<<packet->application_id);

				Ipv4GlobalRouting::count_deleted_flow =
						Ipv4GlobalRouting::count_deleted_flow + 1;
			}

			Ipv4GlobalRouting::total_number_of_packets_to_destination++;
			//
		}



	}

	if(Simulator::Now().ToDouble(Time::MS) >= 1000.00 * 0.2 && !Ipv4GlobalRouting::has_system_learnt)
	{
		Ipv4GlobalRouting::has_system_learnt = true;

		if (!Ipv4GlobalRouting::has_drop_occured) //so drop has been made
		{
			NS_LOG_UNCOND("Entered here for threshold");
			Ipv4GlobalRouting::lower_utilization_threshold = 0.4 * Ipv4GlobalRouting::upper_utilization_threshold;
		}
	}
//	if(!Ipv4GlobalRouting::has_system_learnt)
//	{
//		Ipv4GlobalRouting::has_system_learnt = true;
//
//		if (!Ipv4GlobalRouting::has_drop_occured) //so drop has been made
//		{
//			NS_LOG_UNCOND("Entered here for threshold");
//			Ipv4GlobalRouting::lower_utilization_threshold = 0.4 * Ipv4GlobalRouting::upper_utilization_threshold;
//		}
//	}

	//NS_LOG_UNCOND("The simulation time "<<Simulator::Now().ToDouble(Time::MS));
	//	if(Ipv4GlobalRouting::total_number_of_packets_to_destination >= 10000)
#endif
	return false;
}

/*
 *
 */
uint32_t ssTOSPointToPointNetDevice::FindCorrespondingNodeId(
		Ipv4Address lsa_ip) {
#if COMPILE_CUSTOM_ROUTING_CODE
	for (std::list<MetricListClass>::iterator it =
			Ipv4GlobalRouting::MetricList.begin();
			it != Ipv4GlobalRouting::MetricList.end(); it++) {
		if (it->GetFirstIp() == lsa_ip) {
			return it->GetfirstNodeId();
		}
	}
#endif
	return -1;
}

//
// Callback to invoke whenever a packet is about to be SENT and must be processed at the higher layers.
// Set the callback to be used to notify higher layers when a packet is to be sent.
//
bool ssTOSPointToPointNetDevice::NetDeviceSendCallBack(
		Ptr<const Packet> packet) {
	//NS_ASSERT(simulationRunProperties::enableCustomRouting);
	NS_LOG_FUNCTION(
			this << "Sanjeev: Add your custom ROUTING logic, and decide how to RECEIVE this packet routing");
#if COMPILE_CUSTOM_ROUTING_CODE
	Ptr<Node> n = GetNode();
	//
	// Major: Sanjeev: May 9: Remmoved dependency on FlowTag completely. I get all values from packet directly....
	//
	Ipv4Address m_src = DynamicCast<ssNode>(
			NodeList::GetNode(packet->srcNodeId))->GetNodeIpv4Address();
	Ipv4Address m_dst = DynamicCast<ssNode>(
			NodeList::GetNode(packet->dstNodeId))->GetNodeIpv4Address();
	uint32_t m_flowId = packet->flow_id;
	uint16_t m_requiredBW = packet->required_bandwidth;
	//
	flow_info flow_entry;


	if (Ipv4GlobalRouting::flow_map.count(m_flowId) <= 0) { //first packet of any flow
		flow_entry = flow_info();
		flow_entry.set_flow_id(m_flowId);
		flow_entry.set_source(m_src);
		flow_entry.set_destination(m_dst);
		flow_entry.set_bandwidth(m_requiredBW);
		Ipv4GlobalRouting::flow_map[m_flowId] = flow_entry;
	}

	if (packet->sub_flow_dest_physical == n->GetId())
	{
		//Ipv4GlobalRouting::total_number_of_packets_to_destination++;
	}
	//
#endif
	return false;
}

/**************************************************************/

} // namespace
