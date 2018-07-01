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
#ifndef FLOW_DURATION_RANDOM_VARIABLE_STREAM_H
#define FLOW_DURATION_RANDOM_VARIABLE_STREAM_H

#include "ns3/type-id.h"
#include "ns3/object.h"
#include "ns3/attribute-helper.h"
#include "ns3/random-variable-stream.h"
#include <stdint.h>

/**
 * Expanded from ns3::RandomVariableStream and derivatives.
 */

namespace ns3 {

class FlowDurationRandomVariable: public RandomVariableStream {
public:
	/**
	 * \brief Register this type.
	 * \return The object TypeId.
	 */
	static TypeId GetTypeId(void);

	/**
	 * \brief Creates a Pareto distribution RNG with the default
	 * values for the mean, the shape, and upper bound.
	 */
	FlowDurationRandomVariable();

	/**
	 * \brief Returns the mean parameter for the Pareto distribution returned by this RNG stream.
	 * \return The mean parameter for the Pareto distribution returned by this RNG stream.
	 */
	double GetMean(void);

	/**
	 * \brief Returns the shape parameter for the Pareto distribution returned by this RNG stream.
	 * \return The shape parameter for the Pareto distribution returned by this RNG stream.
	 */
	double GetShape(void) const;

	/**
	 * \brief Returns the upper bound on values that can be returned by this RNG stream.
	 * \return The upper bound on values that can be returned by this RNG stream.
	 */
	double GetBound(void) const;
	double GetLowerBound(void);
	virtual double GetValue(void);
	virtual double GetValueTest(double yValue);
	// not implemented

	double GetValue(double mean, double shape, double bound);
	uint32_t GetInteger(uint32_t mean, uint32_t shape, uint32_t bound);
	virtual uint32_t GetInteger(void);

private:
	// strictly private..
	float f(float min, double alpha, double EX, double max);
	float df(float min, double alpha, double EX);

protected:
	virtual bool initializeStream(void);

	bool m_streamInitialized;
	// attributes for uniform random distribution..
	Ptr<RandomVariableStream> m_probability;
	double m_intermediateFirstTerm, m_new_C;
	/** The mean parameter for the Pareto distribution returned by this RNG stream. */
	double m_mean;
	/** The shape parameter for the Pareto distribution set for this RNG stream. */
	double m_shape;
	/** The upper bound on values set for this RNG stream. */
	double m_bound;
	/** The lower bound on values set for this RNG stream. */
	double m_lowerbound;

};
// class FlowDurationRandomVariable

}// namespace ns3

#endif /* FLOW_DURATION_RANDOM_VARIABLE_STREAM_H */
