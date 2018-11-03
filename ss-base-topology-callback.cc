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
	uint32_t dest = packet->sub_flow_dest;

	uint32_t total_hosts_in_system = (SSD_PER_RACK + 1) * (simulationRunProperties::k/2) * (simulationRunProperties::k/2) * simulationRunProperties::k;

	uint32_t source = (uint32_t)(packet->srcNodeId -20);
	if(packet->sync_packet)
	{
		if(BaseTopology::transaction_rollback_write_tracker[dest][source] <=0)
		{
			NS_LOG_UNCOND(" dest "<<dest<<" source "<<source);
			exit(0);
		}
		BaseTopology::transaction_rollback_write_tracker[dest][source]--; //sync packet has reached the destination

		NS_LOG_UNCOND("This is the case for sync packets");
		for(uint32_t index = 0; index < total_hosts_in_system; index++)
		{
			if(BaseTopology::transaction_rollback_packets[dest][index] > 0)
			{

				uint32_t num_of_packets = BaseTopology::transaction_rollback_packets[dest][index];

				BaseTopology::rollback_packets +=  num_of_packets;
				BaseTopology::transaction_rollback_packets[dest][index] = 0;
				BaseTopology::InjectANewRandomFlowCopyCreation (index, dest, num_of_packets);

			}
		}
	}
	else
	{
		if(!packet->copy_creation_packet && BaseTopology::transaction_rollback_write_tracker[dest][source] > 0)
		{
			if(BaseTopology::chunk_version_tracker[packet->sub_flow_id] != BaseTopology::chunk_version_node_tracker[packet->sub_flow_id][packet->sub_flow_dest]) BaseTopology::transaction_rollback_packets[dest][source]++;
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
	double alpha=0.75;
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
		NS_LOG_UNCOND(" flow id "<< m_flowId<<" flow source "<< m_src << " flow destination "<<m_dst<< " flow_entry.destination_node "<< flow_entry.destination_node);
	}
	else
	{
		uint32_t total_hosts_in_system = (SSD_PER_RACK + 1) * (simulationRunProperties::k/2) * (simulationRunProperties::k/2) * simulationRunProperties::k;

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

			fprintf(fp_packet,"%d, %d, %d , %d, %d, %d, %d, %d,%d, %d, %f, %f,%f\n", packet->flow_id, packet->required_bandwidth, packet->sub_flow_dest ,packet->sub_flow_dest_physical, packet->application_id, packet->is_First, packet->packet_id, packet->dstNodeId , packet->srcNodeId, packet->is_write, packet->creation_time, current_simulation_time, (current_simulation_time - packet->creation_time));

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



			//keeping track of the tail latency
			if ((current_simulation_time - packet->creation_time)>BaseTopology::tail_latency)
				BaseTopology::tail_latency=(current_simulation_time - packet->creation_time);


			//This is to keep chunk level read and write statistics------------------------
			if(packet->is_write)
			{
				BaseTopology::chnkCopy[packet->sub_flow_id].writeCount++;

//				bool update_required = true;
//
//				uint32_t result = 0;
//
//				for(uint32_t host_index=0; host_index<total_hosts_in_system;host_index++)
//				{
//					if(BaseTopology::chunk_copy_node_tracker[packet->sub_flow_id][host_index])
//					{
//						result += BaseTopology::chunk_version_node_tracker[packet->sub_flow_id][host_index] ^ BaseTopology::chunk_version_tracker[packet->sub_flow_id];
//
//						if(result > 0)
//						{
//							update_required = false;
//
//							break;
//						}
//					}
//				}

				//if(update_required) BaseTopology::chunk_version_tracker[packet->sub_flow_id]++;
				BaseTopology::chunk_version_node_tracker[packet->sub_flow_id][packet->sub_flow_dest]++;
				//BaseTopology::chnkCopy[packet->sub_flow_id].writeUtilization+=simulationRunProperties::packetSize;
			}
			else
			{
				BaseTopology::chnkCopy[packet->sub_flow_id].readCount++;
				//BaseTopology::chnkCopy[packet->sub_flow_id].readUtilization+=simulationRunProperties::packetSize;
			}

			if(BaseTopology::chnkCopy[packet->sub_flow_id].first_time_entered==0)
				BaseTopology::chnkCopy[packet->sub_flow_id].first_time_entered=current_simulation_time;



			if(BaseTopology::chnkCopy[packet->sub_flow_id].first_time_entered!=0 && (current_simulation_time-BaseTopology::chnkCopy[packet->sub_flow_id].first_time_entered)>100000)//calculate after 100 millisec
			{
				//BaseTopology::chnkCopy[packet->sub_flow_id].readUtilization=(BaseTopology::chnkCopy[packet->sub_flow_id].readCount*8*simulationRunProperties::packetSize)/Count*(current_simulation_time-BaseTopology::chnkCopy[packet->sub_flow_id].first_time_entered);
				FILE * fp_chunk_utilization;
				fp_chunk_utilization = fopen("chunk_readwrite_utilization.csv","a");

				double numerator_r=(BaseTopology::chnkCopy[packet->sub_flow_id].readCount*8*simulationRunProperties::packetSize);
				double denominator_r=Count*(current_simulation_time-BaseTopology::chnkCopy[packet->sub_flow_id].first_time_entered);
				BaseTopology::chnkCopy[packet->sub_flow_id].readUtilization=((numerator_r/denominator_r)*100)*alpha+(1-alpha)*BaseTopology::chnkCopy[packet->sub_flow_id].readUtilization;

				double numerator_w=(BaseTopology::chnkCopy[packet->sub_flow_id].writeCount*8*simulationRunProperties::packetSize);
				double denominator_w=Count*(current_simulation_time-BaseTopology::chnkCopy[packet->sub_flow_id].first_time_entered);
				BaseTopology::chnkCopy[packet->sub_flow_id].writeUtilization=((numerator_w/denominator_w)*100)*alpha+(1-alpha)*BaseTopology::chnkCopy[packet->sub_flow_id].writeUtilization;


				//NS_LOG_UNCOND("chunk id "<<packet->sub_flow_id<<" BaseTopology::chnkCopy[packet->sub_flow_id].readCount  "<<BaseTopology::chnkCopy[packet->sub_flow_id].readCount<<"numereator read " <<numerator_r <<" denominator read "<< denominator_r<<"numereator write " <<numerator_w <<" denominator write "<< denominator_w<<" BaseTopology::chnkCopy[packet->sub_flow_id].readUtilization  "<<BaseTopology::chnkCopy[packet->sub_flow_id].readUtilization<<" BaseTopology::chnkCopy[packet->sub_flow_id].writeUtilization  "<<BaseTopology::chnkCopy[packet->sub_flow_id].writeUtilization);
				BaseTopology::chnkCopy[packet->sub_flow_id].cumulative_write_sum+=BaseTopology::chnkCopy[packet->sub_flow_id].writeCount;

				//calculate utilization and reset the counters ; need smoothing as well
				if (packet->is_write)
				{
					fprintf(fp_chunk_utilization,"%d,write,%d,%d,%f,%d,%d,%d,%f\n",packet->sub_flow_id,BaseTopology::chnkCopy[packet->sub_flow_id].count,BaseTopology::chnkCopy[packet->sub_flow_id].readCount,BaseTopology::chnkCopy[packet->sub_flow_id].readUtilization,BaseTopology::chnkCopy[packet->sub_flow_id].uniqueWrite,BaseTopology::chnkCopy[packet->sub_flow_id].cumulative_write_sum,BaseTopology::chnkCopy[packet->sub_flow_id].writeCount,BaseTopology::chnkCopy[packet->sub_flow_id].writeUtilization);
					BaseTopology::chnkCopy[packet->sub_flow_id].readCount=0;
					BaseTopology::chnkCopy[packet->sub_flow_id].first_time_entered=current_simulation_time;
					BaseTopology::chnkCopy[packet->sub_flow_id].writeCount=1;

				}
				else
				{
					fprintf(fp_chunk_utilization,"%d,read,%d,%d,%f,%d,%d,%d,%f\n",packet->sub_flow_id,BaseTopology::chnkCopy[packet->sub_flow_id].count,BaseTopology::chnkCopy[packet->sub_flow_id].readCount,BaseTopology::chnkCopy[packet->sub_flow_id].readUtilization,BaseTopology::chnkCopy[packet->sub_flow_id].uniqueWrite,BaseTopology::chnkCopy[packet->sub_flow_id].cumulative_write_sum,BaseTopology::chnkCopy[packet->sub_flow_id].writeCount,BaseTopology::chnkCopy[packet->sub_flow_id].writeUtilization);
					BaseTopology::chnkCopy[packet->sub_flow_id].readCount=1;
					BaseTopology::chnkCopy[packet->sub_flow_id].first_time_entered=current_simulation_time;
					BaseTopology::chnkCopy[packet->sub_flow_id].writeCount=0;
				}
				//BaseTopology::chnkCopy[packet->sub_flow_id].uniqueWrite=0;
				fclose(fp_chunk_utilization);

			}

			//ManageOppurtunisticTransaction(packet);

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
