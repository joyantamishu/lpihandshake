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


void ssTOSPointToPointNetDevice::ManageOppurtunisticTransaction(Ptr<const Packet> packet)
{

	if(!packet->is_write && !packet->copy_creation_packet)
	{
		uint32_t dest =  (uint32_t)(packet->srcNodeId -20);

		uint32_t src = packet->sub_flow_dest;

		if(BaseTopology::chunk_version_node_tracker[packet->sub_flow_id][dest] < BaseTopology::chunk_reference_version_tracker[packet->sub_flow_id])
		{
			BaseTopology::transaction_rollback_packets[src][dest] ++;
		}
	}
	else if(packet->is_write)
	{
		uint32_t src = (uint32_t)(packet->srcNodeId -20);

		uint32_t dest = packet->sub_flow_dest;

		uint32_t num_of_packets = BaseTopology::transaction_rollback_packets[src][dest];

		BaseTopology::rollback_packets +=  num_of_packets;
		BaseTopology::transaction_rollback_packets[src][dest] = 0;
		if(num_of_packets >= 1) BaseTopology::InjectANewRandomFlowCopyCreation (dest, src, num_of_packets);


	}
//	uint32_t dest = packet->sub_flow_dest;
//
//	uint32_t total_hosts_in_system = (SSD_PER_RACK + 1) * (simulationRunProperties::k/2) * (simulationRunProperties::k/2) * simulationRunProperties::k;
//
//	uint32_t source = (uint32_t)(packet->srcNodeId -20);
//
//	if(packet->sync_packet)
//	{
//		NS_LOG_UNCOND("Sync traffic");
//		if(BaseTopology::transaction_rollback_write_tracker[dest][source] <=0)
//		{
//			NS_LOG_UNCOND(" dest "<<dest<<" source "<<source);
//			exit(0);
//		}
//		BaseTopology::transaction_rollback_write_tracker[dest][source]--; //sync packet has reached the destination
//
//		//NS_LOG_UNCOND("This is the case for sync packets");
//		for(uint32_t index = 0; index < total_hosts_in_system; index++)
//		{
//			if(BaseTopology::transaction_rollback_packets[dest][index] > 0)
//			{
//
//				uint32_t num_of_packets = BaseTopology::transaction_rollback_packets[dest][index];
//
//				BaseTopology::rollback_packets +=  num_of_packets;
//				BaseTopology::transaction_rollback_packets[dest][index] = 0;
//				BaseTopology::InjectANewRandomFlowCopyCreation (index, dest, num_of_packets);
//
//			}
//		}
//	}
//	else
//	{
//		dest = (uint32_t)(packet->srcNodeId -20);
//		source = packet->sub_flow_dest;
//
//		if(!packet->copy_creation_packet && BaseTopology::transaction_rollback_write_tracker[dest][source] > 0)
//		{
//			//NS_LOG_UNCOND("------Entered Here----------");
//			if(BaseTopology::chunk_version_tracker[packet->sub_flow_id] != BaseTopology::chunk_version_node_tracker[packet->sub_flow_id][packet->sub_flow_dest]) BaseTopology::transaction_rollback_packets[dest][source]++;
//		}
//	}
}

double ssTOSPointToPointNetDevice::GetSyncPacketTransmissionTime(uint32_t src_node, uint32_t first_received_node, Ptr<const Packet> packet)
{
	uint32_t total_host = (SSD_PER_RACK + 1) * (simulationRunProperties::k/2) * (simulationRunProperties::k/2) * simulationRunProperties::k;

	uint32_t source_node, source_node1, source_node2;



	source_node1 = packet->srcNodeId - 20;

	source_node2 = first_received_node;

	uint32_t total_distance = ns3::BaseTopology::distance_matrix[source_node1][source_node2];


	for(uint32_t distance = 0; distance < total_host; distance++)
	{
		source_node = ns3::BaseTopology::distance_node[first_received_node][distance];
		if(ns3::BaseTopology::chunk_copy_node_tracker[packet->sub_flow_id][source_node])
		{
			source_node1 = source_node2;

			source_node2 = source_node;

			total_distance += ns3::BaseTopology::distance_matrix[source_node1][source_node2];

			if(source_node == src_node) return total_distance;
		}

	}
	NS_LOG_UNCOND("If you ever found this message, that means something is obviously wrong, most probable is that, the chunk has been deleted already by the GA");

	exit(0); //I will remove it later

	return -1.0;
}

void ssTOSPointToPointNetDevice::ManageOppurtunisticTransactionv2(Ptr<const Packet> packet)
{

	uint32_t flow_id = packet->flow_id;
	double current_simulation_time;
	double commit_time;
	double base_commit_time;
	double sync_packet_transmission_time;
	if(!packet->is_write && !packet->copy_creation_packet)
	{
		uint32_t dest =  (uint32_t)(packet->srcNodeId -20);

		uint32_t src = packet->sub_flow_dest;

		if(BaseTopology::chunk_version_node_tracker[packet->sub_flow_id][dest] < BaseTopology::chunk_reference_version_tracker[packet->sub_flow_id])
		{
			BaseTopology::transaction_rollback_packets[src][dest] ++;
		}
	}
	else if(packet->is_write)
	{

		if(BaseTopology::chunkTracker.at(packet->sub_flow_id).number_of_copy >= 1)
		{
			sprintf (concurrency_hash_entry, "%u %u %u", (uint32_t)(packet->srcNodeId -20),packet->sub_flow_id, packet->version);

			printf("^^^^^%s\n",concurrency_hash_entry);

			std::string key = concurrency_hash_entry;
			if(ns3::BaseTopology::concurrency_tracker.count(key) > 0)
			{
				current_simulation_time = Simulator::Now().ToDouble(Time::US);
				base_commit_time = ns3::BaseTopology::concurrency_tracker[key].commit_time;

				sync_packet_transmission_time = GetSyncPacketTransmissionTime(GetNode()->GetId() - 20, ns3::BaseTopology::concurrency_tracker[key].first_node, packet);

				printf("********************The timestamp is %lf %lf %lf %lf\n",ns3::BaseTopology::concurrency_tracker[key].first_arrival_time, current_simulation_time, base_commit_time, sync_packet_transmission_time);

				if(current_simulation_time <= base_commit_time)
				{
					if(sync_packet_transmission_time >= current_simulation_time)
					{
						NS_LOG_UNCOND("Sync packet Arrives later");
					}
					else
					{
						NS_LOG_UNCOND("Sync packet Arrives earlier");
					}
					BaseTopology::sum_delay_ms += base_commit_time - current_simulation_time;

				}
				else
				{
					//Do Nothing, Roll-back
				}

			}
			else
			{
				current_simulation_time = Simulator::Now().ToDouble(Time::US);
				commit_time = current_simulation_time + DELTA_COMMIT_MICROSECOND;
				TimeStampTracker tsmp = TimeStampTracker(current_simulation_time, commit_time, packet->srcNodeId -20);
				ns3::BaseTopology::concurrency_tracker[key] = tsmp;
				Ipv4GlobalRouting::flow_map.at(flow_id).delaysum += DELTA_COMMIT_MICROSECOND;

			}
		}



	}
}



bool ssTOSPointToPointNetDevice::NetDeviceReceiveCallBack(
		Ptr<const Packet> packet) {
	//NS_ASSERT(simulationRunProperties::enableCustomRouting);
	//NS_LOG_UNCOND("Hello Callback");
	NS_LOG_FUNCTION(
			this << "Sanjeev: Add your custom ROUTING logic, and decide how to RECEIVE this packet routing");
#if COMPILE_CUSTOM_ROUTING_CODE
	Ptr<Node> n = GetNode();
	//double alpha=0.75;
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
		//NS_LOG_UNCOND(" flow id "<< m_flowId<<" flow source "<< m_src << " flow destination "<<m_dst<< " flow_entry.destination_node "<< flow_entry.destination_node);
	}
	else
	{
		//uint32_t total_hosts_in_system = (SSD_PER_RACK + 1) * (simulationRunProperties::k/2) * (simulationRunProperties::k/2) * simulationRunProperties::k;

	//	NS_LOG_UNCOND("destination_node "<<packet->dstNodeId);
		if (packet->sub_flow_dest_physical == n->GetId()) //packet has reached the destination
		{

			FILE *fp_packet;
			fp_packet= fopen("all_packets_rcv.csv","a");

		//	NS_LOG_UNCOND("packet->sub_flow_dest_physical "<<packet->sub_flow_dest_physical);

			BaseTopology::total_packet_count--;


			BaseTopology::total_packets_to_hosts_bits[packet->sub_flow_dest] ++;

			//NS_LOG_UNCOND("total_packet_count "<<BaseTopology::total_packet_count);
			//NS_LOG_UNCOND("The Destination has reached");
			Ipv4GlobalRouting::flow_map.at(m_flowId).total_packet_to_destination = Ipv4GlobalRouting::flow_map.at(m_flowId).total_packet_to_destination +1;

			double current_simulation_time = Simulator::Now().ToDouble(Time::US);
			double time_now=Simulator::Now().ToDouble(Time::MS);
			if(time_now<2000.0)
				BaseTopology::pkt_rcv_during_phase1++;
			else if(time_now>=2000.00 && time_now<4000.00)
				BaseTopology::pkt_rcv_during_phase2++;
			else
				BaseTopology::pkt_rcv_during_phase3++;
			//fprintf(fp_packet,"%d, %d, %d , %d, %d, %d, %d, %d,%d, %d, %f, %f,%f\n", packet->flow_id, packet->required_bandwidth, packet->sub_flow_dest ,packet->sub_flow_dest_physical, packet->application_id, packet->is_First, packet->packet_id, packet->dstNodeId , packet->srcNodeId, packet->is_write, packet->creation_time, current_simulation_time, (current_simulation_time - packet->creation_time));

			fclose(fp_packet);

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




				BaseTopology::sum_delay_ms += (current_simulation_time - packet->creation_time);

				BaseTopology::total_events_learnt++;

				BaseTopology::total_packet_count_inc += BaseTopology::total_packet_count;

				BaseTopology::total_events++;

			}

			if(packet->is_phrase_changed)
			{
				BaseTopology::sum_delay_ms_burst += (current_simulation_time - packet->creation_time);

				BaseTopology::total_events_learnt_burst++;
			}
			else
			{
				BaseTopology::sum_delay_ms_no_burst += (current_simulation_time - packet->creation_time);

				BaseTopology::total_events_learnt_no_burst++;
			}



			BaseTopology::total_packets_to_chunk_destination[packet->sub_flow_id]++;
			//NS_LOG_UNCOND("ffndjsfnsdjfsjfnjsfn   "<<packet->sub_flow_id<<" count of packets "<<BaseTopology::total_packets_to_chunk_destination[packet->sub_flow_id]);


			//keeping track of the tail latency
			if ((current_simulation_time - packet->creation_time)>BaseTopology::tail_latency)
				{
				    BaseTopology::write_flow_tail=false;
					BaseTopology::tail_latency=(current_simulation_time - packet->creation_time);
					if(packet->is_write)  BaseTopology::write_flow_tail =true;
					else BaseTopology::write_flow_tail =false;
				}


			//This is to keep chunk level read and write statistics------------------------
			if(packet->is_write)
			{
				BaseTopology::totalWriteCount++;
				BaseTopology::chnkCopy[packet->sub_flow_id].writeCount++;

				if(BaseTopology::chunk_reference_version_tracker[packet->sub_flow_id] < BaseTopology::chunk_version_tracker[packet->sub_flow_id])
				{
					BaseTopology::chunk_reference_version_tracker[packet->sub_flow_id] ++;
				}
				BaseTopology::chunk_version_node_tracker[packet->sub_flow_id][packet->sub_flow_dest]++;

			}
			else
			{
				BaseTopology::chnkCopy[packet->sub_flow_id].readCount++;
				//BaseTopology::chnkCopy[packet->sub_flow_id].readUtilization+=simulationRunProperties::packetSize;
			}

//			/ManageOppurtunisticTransaction(packet);
			ManageOppurtunisticTransactionv2(packet);

				//This is to keep chunk level read and write statistics------------------------
		
			//NS_LOG_UNCOND("delay_by_packet "<<delay_by_packet);

			// gonna change it later


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

			//NS_LOG_UNCOND("End Find Destination");
			//
		}

	}

	if(Simulator::Now().ToDouble(Time::MS) >= 1000.00 * 0 && !Ipv4GlobalRouting::has_system_learnt)
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
