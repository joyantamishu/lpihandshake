/*
 * main.cc
 *
 *  Created on: Jul 12, 2016
 *      Author: sanjeev
 */

#include <string.h>
#include <iostream>
#include <ctime>

#include "parameters.h"
#include "ss-fat-tree-topology.h"
#include "ns3/log.h"

#include "ns3/wimax-module.h"
#include "ns3/csma-module.h"
#include "ns3/uan-module.h"
#include "ns3/wifi-module.h"

using namespace ns3;

// There is an execution time bug (coredump), so you need the next few lines as-is (donot modify) - sanjeev
BaseStationNetDevice b;
SubscriberStationNetDevice s;
CsmaNetDevice c;
UanNetDevice u;

//
void setStaticVariablesFromCommandLine(int argc1, char* argv1[]);

/******************************************************/

int main(int argc, char *argv[]) {
	time_t now = time(0);

	// convert now to string form
	char* date = ctime(&now);
	date[strlen(date) - 1] = '\0';
	char startTime[50];
	std::sprintf(startTime, "%s", date);
	NS_LOG_UNCOND(
			"\nStart Program:: My E Topology Simulation Program #25 June 2017 " << date << "\n");
	//
	// Allow the user to override any of the defaults at
	// run-time, via command-line arguments
	//
	BaseTopology* t1;

	setStaticVariablesFromCommandLine(argc, argv);
	// simplified, sanjeev 2/25
	t1 = new FatTreeTopology();
	t1->InstallSampleApplications();
	t1->InitializeNetAnim();
	t1->AddEnergyModel();
	/* runs the simulation.*/
	t1->DoRun();
	delete t1;		// delete/destroying objects prints several statistics

	NS_LOG_UNCOND(
			"\nStart Program:: My E Topology Simulation Program #25 June 2017 " << startTime);
	now = time(0);
	date = ctime(&now);date[strlen(date) - 1] = '\0';
	NS_LOG_UNCOND(
			"End  Program :: My E Topology Simulation Program #25 June 2017 " << date << "\n");

	NS_LOG_UNCOND("Total number of applications "<<BaseTopology::total_appication);
	return 0;
}


