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
#ifndef HEADER_mcsv1_udaf
#define HEADER_mcsv1_udaf

#include <cstdlib>
#include <string>
#include <vector>
#include <map>
#include <boost/any.hpp>
#ifdef _MSC_VER
#include <unordered_map>
#else
#include <tr1/unordered_map>
#endif

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
/** 
 * A map from name to function object.
 *  
 * This is temporary until we get the library loading task 
 * complete 
 *  
 * TODO: Remove when library loading is enabled. 
 */
class mcsv1_UDAF;
typedef std::tr1::unordered_map<std::string, mcsv1_UDAF*> UDAF_MAP;

class UDAFMap
{
public:
	EXPORT UDAFMap(){};

	EXPORT ~UDAFMap(){};

	static EXPORT UDAF_MAP& getMap();
private:
	static UDAF_MAP fm;
};


// Flags to define the type and limitations of a UDA(n)F
// Used in context->fRunFlags
static uint64_t	UDAF_OVER_REQUIRED __attribute__ ((unused))        = 1;      // May only be used as UDAnF
static uint64_t	UDAF_OVER_ALLOWED  __attribute__ ((unused))        = 1 << 1; // May be used as UDAF or UDAnF
static uint64_t	UDAF_ORDER_REQUIRED __attribute__ ((unused))       = 1 << 2; // If used as UDAnF, ORDER BY is required
static uint64_t	UDAF_ORDER_ALLOWED  __attribute__ ((unused))       = 1 << 3; // If used as UDAnF, ORDER BY is optional
static uint64_t	UDAF_WINDOWFRAME_REQUIRED __attribute__ ((unused)) = 1 << 4; // If used as UDAnF, a WINDOW FRAME is required
static uint64_t	UDAF_WINDOWFRAME_ALLOWED __attribute__ ((unused))  = 1 << 5; // If used as UDAnF, a WINDOW FRAME is optional
static uint64_t	UDAF_MAYBE_NULL     __attribute__ ((unused))       = 1 << 6; // If UDA(n)F might return NULL.
static uint64_t	UDAF_MAYBE_DISTRIBUTED __attribute__ ((unused))    = 1 << 7; // If UDAF is designed to be distributed.

// Flags set by the framework to define the context of the call
// used in context->fContextFlags
static uint64_t	CONTEXT_IS_ANALYTIC __attribute__ ((unused))          = 1;      // If called using OVER
static uint64_t	CONTEXT_UNBOUNDED_PRECEEDING __attribute__ ((unused)) = 1 << 1; // Called using UNBOUNDED_PRECEEDING
static uint64_t	CONTEXT_HAS_CURRENT_ROW __attribute__ ((unused))      = 1 << 2; // The current window contains the current row.
static uint64_t	CONTEXT_IS_RANGE_BASED __attribute__ ((unused))       = 1 << 3; // The Window is RANGE based.

// Flags that describe the contents of a specific input parameter
// These will be set in context->dataFlags for each method call by the framework.
// The data referred to is different depending on the method called.
static uint64_t	PARAM_IS_NULL     __attribute__ ((unused)) = 1;
static uint64_t	PARAM_IS_CONSTANT __attribute__ ((unused)) = 1 << 1;

// shorthand for the list of columns in the call sent to init()
// first is the actual column name and second is the data type in Columnstore.
typedef std::multimap<std::string, CalpontSystemCatalog::ColDataType> COL_TYPES;

// This is the context class that is passed to all API callbacks
// The framework potentially sets data here for each invocation of 
// mcsv1_UDAF methods. Access methods are given for data useful to UDA(n)F.
// Don't modify anything directly except data retrieved with getUserData().

// UDA(n)F devlopers should not modify this class. The framework and other UDA(n)F
// rely on it being as it was when they were compiled.
//
// It's probable that future versions of Columnstore will add functionality to
// the context. UDA(n)F may need to be re-compiled in this case.
class mcsv1Context
{
public:
	EXPORT mcsv1Context();
	EXPORT mcsv1Context(mcsv1Context& rhs);
	// The destructor is not virtual. mcsv1Context should never be subclassed
	EXPORT ~mcsv1Context();

	// Set an error message if something goes wrong
	EXPORT void setErrorMessage(std::string errmsg);

	// Get the previously set error message
	EXPORT std::string& getErrorMessage();

	// Set the flags as a set. Return the previous flags.
	EXPORT uint64_t setRunFlags(uint64_t flags);
	// return the flags
	EXPORT uint64_t getRunFlags();

	// The following set, get, clear and toggle methods can be used to manipulate
	// multiple flags by ORing them together in the call sequence.
	// Ex setRunFlag(UDAF_OVER_REQUIRED | UDAF_ORDER_REQUIRED);
	// sets both flags and returns true if BOTH flags are already set.
	// 
	// Set a specific flag and return its previous setting
	EXPORT bool setRunFlag(uint64_t flag);
	// Get a specific flag
	EXPORT bool getRunFlag(uint64_t flag);
	// clear a specific flag and return its previous setting
	EXPORT bool clearRunFlag(uint64_t flag);
	// toggle a specific flag and return its previous setting
	EXPORT bool toggleRunFlag(uint64_t flag);

	// Use these to determine the way your UDA(n)F was called
	// Valid in all method calls
	EXPORT bool isAnalytic();
	EXPORT bool isUnboundedPreceding();
	EXPORT bool isWindowHasCurrentRow();
	EXPORT bool isRangeBased();

	// Parameter refinement description accessors
	// valid in nextValue, dropValue, evaluateCumulative and superEvaluate
	size_t getParameterCount();

	// Determine if an input parameter is NULL
	// valid in nextValue, dropValue, evaluateCumulative and superEvaluate
	EXPORT bool isParamNull(int paramIdx);

	// If a parameter is a constant, the UDA(n)F could presumably optimize its workings.
	// During the first call to nextValue() or evaluateCumulative().
	// Is there a better way to determine this?
	// valid in nextValue, dropValue and evaluateCumulative
	EXPORT bool isParamConstant(int paramIdx);

	// For getting the result type.
	EXPORT CalpontSystemCatalog::ColDataType getResultType();

	// For getting the decimal characteristics for the return value.
	// These will be set by the Framework if DECIMAL is the default return type
	EXPORT void getResultDecimalCharacteristics(int32_t decimals, int32_t precision);

	// If you want to change the result type
	// valid in init()
	EXPORT bool setResultType(CalpontSystemCatalog::ColDataType resultType);

	// For setting the decimal characteristics for the return value.
	// This only makes sense if the return type is decimal. 
	// valid in init()
	EXPORT bool setResultDecimalCharacteristics(int32_t decimals, int32_t precision);

	// For all types, get the return column width in bytes. Ex. INT will return 4.
	EXPORT int32_t getColWidth();

	// For non-numric return types, set the return column width. This will defaults
	// to the the length of the input.
	// valid in init()
	EXPORT bool setColWidth(int32_t colWidth);

	// If a method is known to take a while, call this periodically to see if something
	// interupted the processing. If getInterrupted() returns true, then the executing
	// method should clean up and exit.
	EXPORT bool getInterrupted();

	// Returns the actual number of rows in the partition. If no partitioning, returns 0.
	// valid in reset()
	EXPORT uint64_t getRowsInPartition();

	// Allocate instance specific memory. This should be type cast to a structure overlay
	// defined by the function. The actual allocatoin occurs in the various modules that
	// do the aggregation. If the UDAF is being calculated in a distributed fashion, then
	// multiple instances of this data may be allocated. Calls to the subaggregate functions
	// do not share a context.
	// You do not need to worry about freeing this memory. The framework handles all management.
	// Call this during init()
	EXPORT void  allocUserData(int bytes);
	// Call this everywhere except init()
	EXPORT uint8_t* getUserData();

	// Many UDAnF need a default Window Frame. If none is set here, the default is 
	// UNBOUNDED PRECEDING to CURRENT ROW. 
	// It's possible to not allow the the WINDOW FRAME phrase in the UDAnF by setting
	// the UDAF_WINDOWFRAME_REQUIRED and UDAF_WINDOWFRAME_ALLOWED both to false. Columnstore
	// requires a Window Frame tin order to process UDAnF. In this case, the default will
	// be used for all calls.
	// Possible values for start frame are
	// WF_UNBOUNDED_PRECEDING, WF_CURRENT_ROW, WF_PRECEDING or WF_FOLLOWING
	// possible values for end frame are
	// WF_CURRENT_ROW, WF_UNBOUNDED_FOLLOWING, WF_PRECEDING or WF_FOLLOWING
	// If WF_PRECEEdING and/or WF_FOLLOWING, a start or end constant should
	// be included to say how many precedding or following is the default
	// Set this during init()
	EXPORT bool  setDefaultWindowFrame(WF_FRAME defaultStartFrame, 
									   WF_FRAME defaultEndFrame,
									   int32_t startConstant = 0, // For WF_PRECEEDING or WF_FOLLOWING
									   int32_t endConstant = 0);  // For WF_PRECEEDING or WF_FOLLOWING

	// There may be times you want to know the actual frame set by the caller
    EXPORT void  getStartFrame(WF_FRAME& startFrame, int32_t& startConstant);
	EXPORT void  getEndFrame(WF_FRAME& endFrame, int32_t& endConstant);
	
	// Deep Equivalence
	bool operator==(const mcsv1Context& c) const;
	bool operator!=(const mcsv1Context& c) const;

	// stream operator for debugging
	EXPORT const std::string toString() const;

	// Get the name of the function
	EXPORT const std::string& getName() const;

	EXPORT mcsv1Context& operator=(mcsv1Context& rhs);
	EXPORT mcsv1Context& copy(mcsv1Context& rhs);
	
private:

	uint64_t fRunFlags;       // Set by the user to define the type of UDA(n)F
	uint64_t fContextFlags;   // Set by the framework to define this specific call.
	uint32_t fUserDataSize;
	uint8_t* fUserData;
	CalpontSystemCatalog::ColDataType fResultType;
	int32_t  fColWidth;        // The length in bytes of the return type
	int32_t  fResultDecimals;  // For Decimals, the number of digits to the right of the decimal
	int32_t  fResultPrecision; // The max number of digits allowed in the decimal value
	std::string errorMsg;
	std::vector<uint32_t> dataFlags;  // one entry for each parameter
	bool*    bInterrupted;            // Gets set to true by the Framework if something happens
	uint64_t fRowsInPartition;        // Only valid in reset()
	WF_FRAME fStartFrame;     // Is set to default to start, then modified by the actual frame in the call
	WF_FRAME fEndFrame;       // Is set to default to start, then modified by the actual frame in the call
	int32_t  fStartConstant;  // for start frame WFPRECEEDOMG or WF_FOLLOWING
	int32_t  fEndConstant;    // for end frame WFPRECEEDOMG or WF_FOLLOWING
	std::string functionName;

public:
	// For use by the framework
	EXPORT void serialize(messageqcpp::ByteStream& b) const;
	EXPORT void unserialize(messageqcpp::ByteStream& b);
	EXPORT void createUserData();
	EXPORT void freeUserData();
	EXPORT void setName(std::string name);
	EXPORT void setContextFlags(uint64_t flags);
	EXPORT uint64_t getContextFlags();
	EXPORT uint32_t getUserDataSize();
	EXPORT std::vector<uint32_t>& getDataFlags();
	EXPORT void setInterrupted(bool interrupted);
	EXPORT void setInterrupted(bool* interrupted);
};

// Since aggregate functions can operate on any data type, we use the following structure
// to define the input row data. To be type insensiteve, data is stored in type boost::any.
// 
// To access the data it must be type cast to the correct type using boost::any_cast.
// example for int data:
// 
// if (dataType == CalpontSystemCatalog::INT)
//     int myint = boost::any_cast<int>columnData;
//
// For multi-paramter aggregations, the colsIn vector of next_value()
// contains the ordered set of row parameters.
//
// For char, varchar, text, varbinary and blob types, columnData will be std::string.
struct ColumnDatum
{
	CalpontSystemCatalog::ColDataType dataType;   // defined in calpontsystemcatalog.h
	boost::any  columnData;
	uint32_t    decimals;   // If dataType is a DECIMAL type
	ColumnDatum() : dataType(CalpontSystemCatalog::UNDEFINED), decimals(0){};
};

// Override mcsv1_UDAF to build your User Defined Aggregate (UDAF) and/or 
// User Defined Analytic Function (UDAnF).
// These will be singleton classes, so don't put any instance
// specific data in here. All instance data is stored in mcsv1Context
// passed to each user function and retrieved by the getUserData() method.
// 
// Each API function returns a ReturnCode. If ERROR is returned at any time, 
// the query is aborted, getInterrupted() will begin to return true and the 
// message set in config->setErrorMessage() is returned to MariaDB. 
class mcsv1_UDAF
{
public:
	enum ReturnCode
	{
		ERROR = 0,
		SUCCESS = 1,
		NOT_IMPLEMENTED = 2   // User UDA(n)F shouldn't return this
	};
	// Defaults OK
	mcsv1_UDAF(){};
	virtual ~mcsv1_UDAF(){};

	/**
	 * Mandatory. Implement this to initialize flags and instance 
	 * data. Called once per SQL statement. You can do any sanity 
	 * checks here. 
	 *  
	 * colTypes (in) - A vector of ColDataType defining the 
	 * parameters of the UDA(n)F call. These can be used to decide 
	 * to override the default return type. If desired, the new 
	 * return type can be set by context->setReturnType() and 
	 * decimal precision can be set in context-> 
	 * setResultDecimalCharacteristics. 
	 *  
	 * Return mcsv1_UDAF::ERROR on any error, such as non-compatible
	 * colTypes or wrong number of arguments. Else return 
	 * mcsv1_UDAF::SUCCESS. 
	 */
	virtual ReturnCode init(mcsv1Context* context,
							COL_TYPES& colTypes) = 0;

	/**
	 * Mandatory. Completes the UDA(n)F. 
	 * Called once per SQL statement. Do not free any memory 
	 * allocated by context->allocUserData(). The SDK Framework owns 
	 * that memory and will handle that. 
	 */
	virtual ReturnCode finish(mcsv1Context* context) = 0;

	/**
	 * Mandatory. Reset the UDA(n)F for a new group, partition or, 
	 * in some cases, new Window Frame. Do not free any memory 
	 * allocated by context->allocUserData(). The SDK Framework owns 
	 * that memory and will handle that. Use this opportunity to 
	 * reset any variables in context->getUserData() needed for the 
	 * next aggregation. 
	 */
	virtual ReturnCode reset(mcsv1Context* context) = 0;

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
								 std::vector<ColumnDatum>& valsIn) = 0;

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
	virtual ReturnCode evaluate(mcsv1Context* context, boost::any& valOut, bool& isNull) = 0;

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
	  * distributed fashion. set UDAF_MAYBE_DISTRIBUTED in u
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
	 virtual ReturnCode dropValue(mcsv1Context* context, 
								  std::vector<boost::any>& valsDropped);

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
	virtual ReturnCode evaluateCumulative(mcsv1Context* context, 
									std::vector<boost::any>& valsIn,
		                            boost::any& valOut, bool& isNull);

protected:

};

/***********************************************************************
 * There is no user modifiable code past this point
 ***********************************************************************/
// Function definitions for mcsv1Context
inline mcsv1Context::mcsv1Context() : 
    fRunFlags(0),
    fContextFlags(0),
	fUserDataSize(0),
    fUserData(NULL),
	fResultType(CalpontSystemCatalog::UNDEFINED),
	fColWidth(0),
    fResultDecimals(0),
    fResultPrecision(18),
	bInterrupted(NULL),
    fRowsInPartition(0),
	fStartFrame(WF_UNBOUNDED_PRECEDING),
	fEndFrame(WF_CURRENT_ROW),
	fStartConstant(0),
	fEndConstant(0)
{
}

inline mcsv1Context::mcsv1Context(mcsv1Context& rhs) :
    fUserData(NULL),
	bInterrupted(NULL)
{
	copy(rhs);
}

// The destructor is not virtual. mcsv1Context should never be subclassed
inline mcsv1Context::~mcsv1Context()
{
	if (fUserData) freeUserData();
}

inline mcsv1Context& mcsv1Context::operator=(mcsv1Context& rhs)
{
	fUserData = NULL;
	bInterrupted = NULL;
	return copy(rhs);
}

inline void mcsv1Context::setErrorMessage(std::string errmsg) 
{
	errorMsg = errmsg;
}

inline std::string& mcsv1Context::getErrorMessage()
{
	return errorMsg;
}

inline uint64_t mcsv1Context::setRunFlags(uint64_t flags) 
{
	uint64_t f = fRunFlags; 
	fRunFlags = flags; 
	return f;
}

inline uint64_t mcsv1Context::getRunFlags() 
{
	return fRunFlags;
}

inline bool mcsv1Context::setRunFlag(uint64_t flag) 
{
	bool b = fRunFlags & flag; 
	fRunFlags |= flag; 
	return b;
}

inline bool mcsv1Context::getRunFlag(uint64_t flag) 
{
	return fRunFlags & flag;
}

inline bool mcsv1Context::clearRunFlag(uint64_t flag) 
{
	bool b = fRunFlags & flag; 
	fRunFlags &= ~flag; 
	return b;
}

inline bool mcsv1Context::toggleRunFlag(uint64_t flag) 
{
	bool b = fRunFlags & flag; 
	fRunFlags ^= flag; 
	return b;
}

inline bool mcsv1Context::isAnalytic() 
{
	return fContextFlags & CONTEXT_IS_ANALYTIC;
}

inline bool mcsv1Context::isUnboundedPreceding() 
{
	return fContextFlags & CONTEXT_UNBOUNDED_PRECEEDING;
}

inline bool mcsv1Context::isWindowHasCurrentRow() 
{
	return fContextFlags & CONTEXT_HAS_CURRENT_ROW;
}

inline bool mcsv1Context::isRangeBased() 
{
	return fContextFlags & CONTEXT_IS_RANGE_BASED;
}

inline size_t mcsv1Context::getParameterCount() 
{
	return dataFlags.size();
}

inline bool mcsv1Context::isParamNull(int paramIdx) 
{
	return dataFlags[paramIdx] & PARAM_IS_NULL;
}

inline bool mcsv1Context::isParamConstant(int paramIdx) 
{
	return dataFlags[paramIdx] & PARAM_IS_CONSTANT;
}

inline CalpontSystemCatalog::ColDataType mcsv1Context::getResultType() 
{
	return fResultType;
}

inline bool mcsv1Context::setResultType(CalpontSystemCatalog::ColDataType resultType) 
{
	fResultType = resultType;
	return true;  // We may want to sanity check here.
}

inline void mcsv1Context::getResultDecimalCharacteristics(int32_t decimals, int32_t precision)
{
	decimals = fResultDecimals; 
	precision = fResultPrecision; 
	return;
}

inline bool mcsv1Context::setResultDecimalCharacteristics(int32_t decimals, int32_t precision)
{
	fResultDecimals = decimals;
	fResultPrecision = precision;
	return true;
}

inline bool mcsv1Context::setColWidth(int32_t colWidth)
{
	fColWidth = colWidth;
	return true;
}

inline void mcsv1Context::setInterrupted(bool interrupted) 
{
	if (bInterrupted)
	{
		*bInterrupted = interrupted;
	}
}

inline void mcsv1Context::setInterrupted(bool* interrupted) 
{
	bInterrupted = interrupted;
}

inline bool mcsv1Context::getInterrupted() 
{
	if (bInterrupted)
	{
		return bInterrupted;
	}
	return false;
}

inline uint64_t mcsv1Context::getRowsInPartition() 
{
	return fRowsInPartition;
}

inline void mcsv1Context::allocUserData(int bytes)
{
	fUserDataSize = bytes;
}

inline uint8_t* mcsv1Context::getUserData() 
{
	return fUserData;
}

inline void mcsv1Context::createUserData() 
{
	// For now, a simple malloc
	if (fUserData)
	{
		freeUserData();
	}
	fUserData = (uint8_t*)malloc(fUserDataSize);
}

inline bool mcsv1Context::setDefaultWindowFrame(WF_FRAME defaultStartFrame, 
								WF_FRAME defaultEndFrame,
								int32_t startConstant,
								int32_t endConstant)
{
	// TODO: Add sanity checks
	fStartFrame = defaultStartFrame;
	fEndFrame = defaultEndFrame;
	fStartConstant = startConstant;
	fEndConstant = endConstant;
	return true;
}

inline void mcsv1Context::getStartFrame(WF_FRAME& startFrame, int32_t& startConstant)
{
	startFrame = fStartFrame;
	startConstant = fStartConstant;
}

inline void mcsv1Context::getEndFrame(WF_FRAME& endFrame, int32_t& endConstant)
{
	endFrame = fEndFrame;
	endConstant = fEndConstant;
}

inline const std::string& mcsv1Context::getName() const
{
	return functionName;
}

// Not to be used by UDA(n)F developer
inline void mcsv1Context::freeUserData() 
{
	if (fUserData)
	{
		free(fUserData);
	}
	fUserData = NULL;
}

inline void mcsv1Context::setName(std::string name)
{
	functionName = name;
}

inline void mcsv1Context::setContextFlags(uint64_t flags)
{
	fContextFlags = flags;
}

inline uint64_t mcsv1Context::getContextFlags()
{
	return fContextFlags;
}

inline uint32_t mcsv1Context::getUserDataSize()
{
	return fUserDataSize;
}

inline std::vector<uint32_t>& mcsv1Context::getDataFlags()
{
	return dataFlags;
}

inline mcsv1_UDAF::ReturnCode mcsv1_UDAF::subEvaluate(mcsv1Context* context, const void* valIn)
{
	return NOT_IMPLEMENTED;
}

inline mcsv1_UDAF::ReturnCode mcsv1_UDAF::superEvaluate(mcsv1Context* context, 
														std::vector<boost::any>& valsIn,
														boost::any& valOut, bool& isNull)
{
	return NOT_IMPLEMENTED;
}

inline mcsv1_UDAF::ReturnCode mcsv1_UDAF::dropValue(mcsv1Context* context, 
													std::vector<boost::any>& valsDropped)
{
	return NOT_IMPLEMENTED;
}

inline mcsv1_UDAF::ReturnCode mcsv1_UDAF::evaluateCumulative(mcsv1Context* context, 
															 std::vector<boost::any>& valsIn,
															 boost::any& valOut, bool& isNull)
{
	return NOT_IMPLEMENTED;
}

}; // namespace mcssdk

#undef EXPORT

#endif // HEADER_mcsv1_udaf.h

