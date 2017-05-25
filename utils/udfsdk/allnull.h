/* Copyright (C) 2017 MariaDB Corporaton

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; version 2 of
   the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301, USA. */

/***********************************************************************
*   $Id$
*
*   mcsv1_UDAF.h
***********************************************************************/

/** 
 * Columnstore interface for writing a User Defined Aggregate 
 * Functions (UDAF) and User Defined Analytic Functions (UDAnF) 
 * or a function that can act as either - UDA(n)F 
 *  
 * The basic steps are:
 *
 * 1. Create a the UDA(n)F function interface in some .h file. 
 * 2. Create the UDF function implementation in some .cpp file 
 * 3. Create the connector stub (MariaDB UDAF definition) for 
 * this UDF function.  
 * 4. build the dynamic library using all of the source. 
 * 5  Put the library in $COLUMNSTORE_INSTALL/lib of 
 * all modules 
 * 6. restart the Columnstore system. 
 * 7. notify mysqld about the new functions with commands like:
 *  
 *    // An example of xor over a range for UDAF and UDAnF
 *    CREATE AGGREGATE FUNCTION mcs_bit_xor returns BOOL soname
 *    'libudfsdk.so';
 *  
 *    // An example that only makes sense as a UDAnF
 *    CREATE AGGREGATE FUNCTION mcs_interpolate returns REAL
 *    soname 'libudfsdk.so';
 *
 * The UDAF functions may run distributed in the Columnstore 
 * engine. UDAnF do not run distributed. 
 *  
 * UDAF is User Defined Aggregate Function. 
 * UDAnF is User Defined Analytic Function. 
 * UDA(n)F is an acronym for a function that could be either. It 
 * is also used to describe the interface that is used for 
 * either. 
 */
#ifndef HEADER_allnull
#define HEADER_allnull

#include <cstdlib>
#include <string>
#include <vector>
#include <boost/any.hpp>
#ifdef _MSC_VER
#include <unordered_map>
#else
#include <tr1/unordered_map>
#endif

#include "mcsv1_udaf.h"
#include "calpontsystemcatalog.h"
#include "windowfunctioncolumn.h"
using namespace execplan;

#if defined(_MSC_VER) && defined(xxxRGNODE_DLLEXPORT)
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

namespace mcsv1sdk
{

// Override mcsv1_UDAF to build your User Defined Aggregate (UDAF) and/or 
// User Defined Analytic Function (UDAnF).
// These will be singleton classes, so don't put any instance
// specific data in here. All instance data is stored in mcsv1Context
// passed to each user function and retrieved by the getUserData() method.
// 
// Each API function returns a ReturnCode. If ERROR is returned at any time, 
// the query is aborted, getInterrupted() will begin to return true and the 
// message set in config->setErrorMessage() is returned to MariaDB. 
class allnull : public  mcsv1_UDAF
{
public:
	// Defaults OK
	allnull() : mcsv1_UDAF(){};
	virtual ~allnull(){};

	/**
	 * Mandatory. Implement this to initialize flags and instance 
	 * data. Called once per SQL statement. You can do any sanity 
	 * checks here. 
	 *  
	 * colTypes (in) - A vector of ColTypes defining the parameters 
	 * of the UDA(n)F call. 
	 *  
	 * Return mcsv1_UDAF::ERROR on any error, such as non-compatible
	 * colTypes or wrong number of arguments. Else return 
	 * mcsv1_UDAF::SUCCESS. 
	 */
	virtual ReturnCode init(mcsv1Context* context,
							COL_TYPES& colTypes);

	/**
	 * Mandatory. Completes the UDA(n)F. 
	 * Called once per SQL statement. Do not free any memory 
	 * allocated by context->allocUserData(). The SDK Framework owns 
	 * that memory and will handle that. 
	 */
	virtual ReturnCode finish(mcsv1Context* context);

	/**
	 * Mandatory. Reset the UDA(n)F for a new group, partition or, 
	 * in some cases, new Window Frame. Do not free any memory 
	 * allocated by context->allocUserData(). The SDK Framework owns 
	 * that memory and will handle that. Use this opportunity to 
	 * reset any variables in context->getUserData() needed for the 
	 * next aggregation. 
	 */
	virtual ReturnCode reset(mcsv1Context* context);

	/**
	 * Mandatory. Handle a single row. 
	 *  
	 * colsIn - A vector of data structure describing the input 
	 * data. 
	 *  
	 * This function is called once for every row in the filtered 
	 * result set (before aggregation). It is very important that 
	 * this function is efficient. 
	 *  
	 * If the UDAF is running in a distributed fashion, nextValue 
	 * cannot depend on order, as it will only be called for each 
	 * row found on the specific PM. 
	 *  
	 * valsIn (in) - a vector of the parameters from the row.
	 */
	virtual ReturnCode nextValue(mcsv1Context* context, 
								 std::vector<ColumnDatum>& valsIn);

	/**
	 * Mandatory. Get the aggregated value.
	 *  
	 * Called for every new group if UDAF GROUP BY, UDAnF partition 
	 * or, in some cases, new Window Frame. 
	 *  
	 * Set the aggregated value into valOut. The datatype is assumed
	 * to be the same as that set in the init() function; 
	 *  
	 * If the UDAF is running in a distributed fashion, evaluate is 
	 * not called. 
	 *  
	 * valOut (out) - Set the aggregated value here. The datatype is
	 * assumed to be the same as that set in the init() function; 
	 *  
	 * isNull (out) set to true if the result is NULL. valOut will 
	 * be ignored. 
	 */
	virtual ReturnCode evaluate(mcsv1Context* context, boost::any& valOut, bool& isNull);

	/** 
	 * Optional -- If defined, the server may run the UDAF in a 
	 * distributed fashion. Use setRunFlag(UDAF_MAYBE_DISTRIBUTED) 
	 * to enable distributed UDAF. 
	 *  
	 * Perform an agggregation on rows partially aggregated by 
	 * nextValue. Columnstore calls nextValue for each row on a 
	 * given PM for a group (GROUP BY). subEvaluateis called to 
	 * consolodate those values into a single instance of userData. 
	 * As usual, keep your aggregated totals in context. The first 
	 * time this is called for a group, reset() would have been 
	 * called with this version of userData. 
	 *  
	 * Called for every partial data set in each group in GROUP BY
	 *  
	 * When subEvaluate has been called for all subAggregated data 
	 * sets, Evaluate will be called with the same context as here.
	 *  
	 * valIn (In) - This is a pointer to a memory block of the size 
	 * set in allocUserData. It will contain the value of userData 
	 * as seen in the last call to NextValue for a given PM.  Don't 
	 * assume it will be in the same place in memory -- it won't be.
	 *  
	 */
	virtual ReturnCode subEvaluate(mcsv1Context* context, const void* valIn);

	 /** 
	  * Optional -- If defined, the server may run the UDAF in a 
	  * distributed fashion. 
	  *  
	  * Get the final aggregated value from a set of sub aggregations 
	  * performed on the PMs or multi-threaded aggregation on the UM.
	  *  
	  * Called for every new group if UDAF GROUP BY
	  *  
	  * If the UDAF is running in a distributed fashion, 
	  * superEvaluate is called on the UM for each group. context is 
	  * not shared between subEvaluate and superEvaluate. 
	  *  
	  * valsIn (in) - a vector of the return values of each 
	  * subEvaluate for this GROUP 
	  *  
	  * valOut (out) - set this to the final aggregation value for 
	  * the GROUP. The datatype is assumed to be the same as that set 
	  * in the init() function; 
	  *  
	  * isNull (out) set to true if the result is NULL. valOut will 
	  * be ignored. 
	  */
	virtual ReturnCode superEvaluate(mcsv1Context* context, 
									 std::vector<boost::any>& valsIn,
		                             boost::any& valOut, bool& isNull);

	 /** 
	  * Optional -- If defined, the server will call this instead of 
	  * reset for UDAnF. 
	  *  
	  * Don't implement if a UDAnF has one or more of the following: 
	  * The UDAnF can't be used with a Window Frame
	  * The UDAnF is not reversable in some way 
	  * The UDAnF is not interested in optimal performance 
	  *  
	  * If not implemented, reset() followed by a series of 
	  * nextValue() will be called for each movement of the Window 
	  * Frame. 
	  *  
	  * If implemented, then each movement of the Window Frame will 
	  * result in dropValue() being called for each row falling out 
	  * of the Frame and nextValue() being called for each new row 
	  * coming into the Frame. 
	  *  
	  * valsDropped (in) - a vector of the parameters from the row 
	  * leaving the Frame 
	  */
//	 virtual ReturnCode dropValue(mcsv1Context* context, 
//								  std::vector<boost::any>& valsDropped);

	/**
	 * Optional - for UDAnF that have a Frame of UNBOUNDED PRECEDING 
	 * to CURRENT ROW. 
	 *  
	 * if implemented, evaluateCumulative() is called instead of
	 * nextValue() and evaluate(). 
	 *  
	 * If not implemented, nextValue() and evaluate() will each be 
	 * called for each Frame movement. 
	 *  
	 * valsIn (in) - a vector of the parameters from the row.
	 *  
	 * valOut (out) - set this to the final aggregation value for 
	 * the row. The datatype is assumed to be the same as that set 
	 * in the init() function; 
	 *  
	 * isNull (out) set to true if the result is NULL. valOut will 
	 * be ignored. 
	 */
//	virtual ReturnCode evaluateCumulative(mcsv1Context* context, 
//									std::vector<boost::any>& valsIn,
//		                            boost::any& valOut, bool& isNull);

protected:

};

};  // namespace

#undef EXPORT

#endif // HEADER_allnull.h

