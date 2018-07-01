/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006 Georgia Tech Research Corporation
 * Copyright (c) 2011 Mathieu Lacage
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
 * Authors: Rajib Bhattacharjea<raj.b@gatech.edu>
 *          Hadi Arbabi<marbabi@cs.odu.edu>
 *          Mathieu Lacage <mathieu.lacage@gmail.com>
 *
 * Modified by Mitch Watrous <watrous@u.washington.edu>
 *
 */
#include "ns3/assert.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/integer.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/log.h"
#include "ns3/abort.h"
#include "ns3/rng-stream.h"
#include "ns3/rng-seed-manager.h"
#include <cmath>
#include <iostream>
#include "flow-duration-random-variable-stream.h"
#include "parameters.h"

#define maxmitr								100
#define root								2.0
#define allowedError						0.0001 //increase the convergence accurancy with another decimal place by madhurima

/**
 *
 */

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("FlowDurationRandomVariable");

NS_OBJECT_ENSURE_REGISTERED(FlowDurationRandomVariable);

TypeId FlowDurationRandomVariable::GetTypeId(void) {
	static TypeId tid =
			TypeId("ns3::FlowDurationRandomVariable").SetParent<
					RandomVariableStream>().SetGroupName("Core").AddConstructor<
					FlowDurationRandomVariable>().AddAttribute("Shape",
					"The shape parameter for the Pareto distribution returned by this RNG stream.",
					DoubleValue(DEFAULT_INITIAL_PARETO_ALPHA_DONOT_CHANGE),
					MakeDoubleAccessor(&FlowDurationRandomVariable::m_shape),
					MakeDoubleChecker<double>()).AddAttribute("LowerBound",
					"The lower bound on the values returned by this RNG stream (if non-zero).",
					DoubleValue(MINFLOWSTOPTIME_MILLISEC),
					MakeDoubleAccessor(
							&FlowDurationRandomVariable::m_lowerbound),
					MakeDoubleChecker<double>()).AddAttribute("Bound",
					"The upper bound on the values returned by this RNG stream (if non-zero).",
					DoubleValue(MAXFLOWSTOPTIME_MILLISEC),
					MakeDoubleAccessor(&FlowDurationRandomVariable::m_bound),
					MakeDoubleChecker<double>());
	/* in our example mean is NOT set, but computed within
	 .AddAttribute("Mean",
	 "The mean parameter for the Pareto distribution returned by this RNG stream.",
	 DoubleValue(1.0),
	 MakeDoubleAccessor(&FlowDurationRandomVariable::m_mean),
	 MakeDoubleChecker<double>())
	 */
	return tid;
}

FlowDurationRandomVariable::FlowDurationRandomVariable() {
	// m_mean, m_shape, and m_bound are initialized after constructor
	// by attributes
	NS_LOG_FUNCTION(this);
	int probabilityMin = 0.0;
	int probabilityMax = 1.0;
	m_lowerbound = m_mean = m_shape = m_bound = 0;
	// 1st create a uniform random value U
	m_probability = CreateObject<UniformRandomVariable>();
	m_probability->SetAttribute("Min", DoubleValue(probabilityMin));
	m_probability->SetAttribute("Max", DoubleValue(probabilityMax));
	m_streamInitialized = false;
	m_intermediateFirstTerm = m_new_C = 0;
}

float FlowDurationRandomVariable::f(float min,double alpha,double EX,double max)
{
	double k1= ((alpha-2)/(alpha-1))*EX;
	double k2= pow(max,1-alpha)*(max-k1);
    return  pow(min,2-alpha)-k1*pow(min,1-alpha)-k2;
}

float FlowDurationRandomVariable::df (float min,double alpha,double EX)
{
    return ((alpha-2)*pow(min,-alpha))*(EX-min);
}

bool FlowDurationRandomVariable::initializeStream(void) {
	NS_LOG_FUNCTION(this);
	// sanjeev, do initialization only once. Mar 18
	if (m_streamInitialized)
		return m_streamInitialized;

	double initialParetoAlpha = DEFAULT_INITIAL_PARETO_ALPHA_DONOT_CHANGE;
	double fixedLowerLimit = m_lowerbound;
	double fixedUpperLimit = m_bound;
	double paretoAlpha = m_shape;
	double paretoMean;

	////////////////////////Calculating the constant C from the probability distribution with the initial values of MAX and MIN////////////////////
	double oneTerm = pow(fixedUpperLimit, (-initialParetoAlpha + 1));
	double nextTerm = pow(fixedLowerLimit, (-initialParetoAlpha + 1));
	double numerator = (-initialParetoAlpha + 1);
	double denominator = (oneTerm - nextTerm);
	double C = numerator / denominator;
	NS_LOG_LOGIC("Initial value of C is: " << C);  //Description changed by madhurima
	////////////////////////////Calculating E(X) with the intial alpha, max limit and min limit/////////////////////////
	double firstTerm = pow(fixedUpperLimit, (-initialParetoAlpha + 2));
	double secondTerm = pow(fixedLowerLimit, (-initialParetoAlpha + 2));
	double multiplicativeFactor = C / (-initialParetoAlpha + 2);
	paretoMean = multiplicativeFactor * (firstTerm - secondTerm);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//use the expectation to calculate a new lower limit (given the earlier expectation, upper limit and C, we are only changing the alpha)

    int itr;
	float h, x0, x1;
	double allerr=allowedError;
	x0=root;
	for (itr=1; itr<=maxmitr; itr++)
	{
		h=f(x0,paretoAlpha,paretoMean,fixedUpperLimit)/df(x0,paretoAlpha,paretoMean);
		x1=x0-h;
		NS_LOG_LOGIC(" At Iteration no. " << itr << ", x = " << x1);
		if (fabs(h) < allerr)
		{
			NS_LOG_LOGIC(" After Iteration no. " << itr << ", root = " << x1);
			break;
		}
		x0=x1;
	}
	double newLowerLimit=x0;
	NS_LOG_LOGIC("New lower limit" << newLowerLimit);
	//New code to put restriction Added by madhurima, this restriction is to keep a watch incase this does not hold
	if(newLowerLimit<(((paretoAlpha-2)/(paretoAlpha-1))*paretoMean))
		NS_LOG_LOGIC("We end up in a wired scenario");
	///////////////////////////////////////////////////////Here we have the new minimun////////////////////////////

	////it is time to calculate the new C based on that ////////////////////////////////////////////////////////
	oneTerm = pow(fixedUpperLimit, (-paretoAlpha + 1));
	nextTerm = pow(newLowerLimit, (-paretoAlpha + 1));
	numerator = (-paretoAlpha + 1);
	denominator = (oneTerm - nextTerm);
	C = numerator / denominator;
	// now set the object attributes..
	// m_lowerbound, m_bound, m_shape set at input
	m_lowerbound = newLowerLimit;
	m_mean = paretoMean;
	m_new_C = C;
	m_intermediateFirstTerm = pow(newLowerLimit, (-paretoAlpha + 1));
	m_streamInitialized = true;
	NS_LOG_LOGIC("Changed value of C is: " << C); //added by madhurima
	NS_LOG_LOGIC(
			"Flow Duration:: paretoAlpha [" << paretoAlpha << "] paretoMean [" << paretoMean << "]");
	return m_streamInitialized;
}

double FlowDurationRandomVariable::GetMean(void) {
	NS_LOG_FUNCTION(this);
	if (!m_streamInitialized)
		initializeStream();
	return m_mean;
}
double FlowDurationRandomVariable::GetShape(void) const {
	NS_LOG_FUNCTION(this);
	return m_shape;
}
double FlowDurationRandomVariable::GetBound(void) const {
	NS_LOG_FUNCTION(this);
	return m_bound;
}
double FlowDurationRandomVariable::GetLowerBound(void) {
	NS_LOG_FUNCTION(this);
	if (!m_streamInitialized)
		initializeStream();
	return m_lowerbound;
}
double FlowDurationRandomVariable::GetValue(double mean, double shape,
		double bound) {
	NS_LOG_FUNCTION(this);
	NS_ABORT_MSG("Not implemented");
}

double FlowDurationRandomVariable::GetValueTest(double yValue) {
	NS_LOG_FUNCTION(this);
	if (!m_streamInitialized)
		initializeStream();
	// double intermediateFirstTerm = pow(newLowerLimit, (-paretoAlpha + 1));
	double intermediateSecondTerm = (yValue * (m_shape - 1)) / m_new_C;
	double xValue = pow((m_intermediateFirstTerm - intermediateSecondTerm),
			1.0 / (1 - m_shape));
	// sanjeev...mar 18.. to verify MR doubts..
	NS_ASSERT_MSG(xValue <= m_bound, "FlowDurationRandomVariable::GetValue:: Very wrong, pareto max exceeded");
	return xValue;
}
uint32_t FlowDurationRandomVariable::GetInteger(uint32_t mean, uint32_t shape,
		uint32_t bound) {
	NS_LOG_FUNCTION(this);
	NS_ABORT_MSG("Not implemented");
}

uint32_t FlowDurationRandomVariable::GetInteger(void) {
	NS_LOG_FUNCTION(this);
	NS_ABORT_MSG("Not implemented");
}

double FlowDurationRandomVariable::GetValue(void) {
	NS_LOG_FUNCTION(this);
	if (!m_streamInitialized)
		initializeStream();
	double yValue = m_probability->GetValue();
	// double intermediateFirstTerm = pow(newLowerLimit, (-paretoAlpha + 1));
	double intermediateSecondTerm = (yValue * (m_shape - 1)) / m_new_C;
	double xValue = pow((m_intermediateFirstTerm - intermediateSecondTerm),
			1.0 / (1 - m_shape));
	// sanjeev...mar 18.. to verify MR doubts..
	NS_ASSERT_MSG(xValue <= m_bound, "FlowDurationRandomVariable::GetValue:: Very wrong, pareto max exceeded");
	NS_LOG_LOGIC("FlowDurationRandomVariable::GetValue --> xValue = " << xValue);
	return xValue;
}

} // namespace ns3
