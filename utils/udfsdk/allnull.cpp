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

#include <sstream>
#include <cstring>
#include "allnull.h"
#include "bytestream.h"
#include "objectreader.h"

using namespace mcsv1sdk;

struct allnull_data
{
    uint64_t	totalQuantity;
    uint64_t	totalNulls;
};

#define OUT_TYPE char
mcsv1_UDAF::ReturnCode allnull::init(mcsv1Context* context,
									 COL_TYPES& colTypes)
{
	context->allocUserData(sizeof(allnull_data));
	if (colTypes.size() < 1)
	{
		// The error message will be prepended with
		// "The storage engine for the table doesn't support "
		context->setErrorMessage("allnull() with 0 atguments");
		return mcsv1_UDAF::ERROR;
	}
	context->setResultType(CalpontSystemCatalog::BIT);

	return mcsv1_UDAF::SUCCESS;
}

mcsv1_UDAF::ReturnCode allnull::finish(mcsv1Context* context)
{
	return mcsv1_UDAF::SUCCESS;
}

mcsv1_UDAF::ReturnCode allnull::reset(mcsv1Context* context)
{
	struct allnull_data* data = (struct allnull_data*)context->getUserData();
	data->totalQuantity = 0;
	data->totalNulls    = 0;
	return mcsv1_UDAF::SUCCESS;
}

mcsv1_UDAF::ReturnCode allnull::nextValue(mcsv1Context* context, 
										  std::vector<ColumnDatum>& valsIn)
{
	struct allnull_data* data = (struct allnull_data*)context->getUserData();
	
	for (size_t i = 0; i < context->getParameterCount(); i++)
	{
		data->totalQuantity++;
		if (context->isParamNull(0))
		{
			data->totalNulls++;
		}
	}
	return mcsv1_UDAF::SUCCESS;
}

mcsv1_UDAF::ReturnCode allnull::evaluate(mcsv1Context* context, boost::any& valOut, bool& isNull)
{
	OUT_TYPE allNull;
	struct allnull_data* data = (struct allnull_data*)context->getUserData();
	allNull = data->totalQuantity > 0 && data->totalNulls == data->totalQuantity;
	valOut = allNull;
	return mcsv1_UDAF::SUCCESS;
}

mcsv1_UDAF::ReturnCode allnull::subEvaluate(mcsv1Context* context, const void* valIn)
{
	struct allnull_data* outData = (struct allnull_data*)context->getUserData();
	struct allnull_data* inData = (struct allnull_data*)valIn;
	outData->totalQuantity += inData->totalQuantity;
	outData->totalNulls += inData->totalNulls;
	return mcsv1_UDAF::SUCCESS;
}

mcsv1_UDAF::ReturnCode allnull::superEvaluate(mcsv1Context* context, 
											  std::vector<boost::any>& valsIn,
											  boost::any& valOut, bool& isNull)
{
	OUT_TYPE allNull = 1;
	for (size_t i = 0; i < valsIn.size(); ++i)
	{
		if (boost::any_cast<int>(&valsIn[i]) == 0)
		{
			allNull = 0;
		}
	}
	valOut = allNull;
	return mcsv1_UDAF::SUCCESS;
}



