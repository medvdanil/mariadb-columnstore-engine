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
#include "mcsv1_udaf.h"
#include "bytestream.h"
#include "objectreader.h"

using namespace mcsv1sdk;
/**
 * All UDA(n)F functions must be registered in the function map.
 * They will be picked up by the Columnstore modules during 
 * startup. 
 *  
 * This is a temporary kludge until we get the library loader 
 * task complete 
 */
UDAF_MAP UDAFMap::fm;
#include "allnull.h"
UDAF_MAP& UDAFMap::getMap()
{
	if (fm.size() > 0)
	{
		return fm;
	}
	// first: function name
	// second: Function pointer
	// please use lower case for the function name. Because the names might be 
	// case-insensitive in MySQL depending on the setting. In such case, 
	// the function names passed to the interface is always in lower case.
	fm["allnull"] = new allnull();
//	fm["idb_isnull"] = new IDB_isnull();
	
	return fm;
}

int32_t mcsv1Context::getColWidth()
{
	if (fColWidth > 0)
	{
		return fColWidth;
	}
	// JIT initialization for types that have a defined size.
	switch (fResultType)
	{
	case CalpontSystemCatalog::BIT:
	case CalpontSystemCatalog::TINYINT:
	case CalpontSystemCatalog::UTINYINT:
	case CalpontSystemCatalog::CHAR:
		fColWidth = 1;
		break;
	case CalpontSystemCatalog::SMALLINT:
	case CalpontSystemCatalog::USMALLINT:
		fColWidth = 2;
		break;
	case CalpontSystemCatalog::MEDINT:
	case CalpontSystemCatalog::INT:
	case CalpontSystemCatalog::UMEDINT:
	case CalpontSystemCatalog::UINT:
	case CalpontSystemCatalog::FLOAT:
	case CalpontSystemCatalog::UFLOAT:
	case CalpontSystemCatalog::DATE:
		fColWidth = 4;
		break;
	case CalpontSystemCatalog::BIGINT:
	case CalpontSystemCatalog::UBIGINT:
	case CalpontSystemCatalog::DOUBLE:
	case CalpontSystemCatalog::UDOUBLE:
	case CalpontSystemCatalog::DATETIME:
	case CalpontSystemCatalog::STRINT:
		fColWidth = 8;
		break;
	case CalpontSystemCatalog::LONGDOUBLE:
		fColWidth = sizeof(long double);
		break;
	default:
		break;
	}
	return fColWidth;
}

bool mcsv1Context::operator==(const mcsv1Context& c) const
{
	// We don't test the per row data fields. They don't determine
	// if it's the same Context.
	if (getName()        != c.getName()
	 ||	fRunFlags        != c.fRunFlags
	 || fContextFlags    != c.fContextFlags
	 || fUserDataSize    != c.fUserDataSize
	 || fUserData        != c.fUserData
	 || fResultType      != c.fResultType
	 || fResultDecimals  != c.fResultDecimals
	 || fResultPrecision != c.fResultPrecision
	 || fRowsInPartition != c.fRowsInPartition
	 || fStartFrame      != c.fStartFrame
	 || fEndFrame        != c.fEndFrame
	 || fStartConstant   != c.fStartConstant
	 || fEndConstant     != c.fEndConstant)
		return false;
	return true;
}

bool mcsv1Context::operator!=(const mcsv1Context& c) const
{
	return (!(*this == c));
}

const std::string mcsv1Context::toString() const
{
	std::ostringstream output;
	output << "mcsv1Context: " << getName() << std::endl;
	output << "  RunFlags=" << fRunFlags << " ContextFlags=" << fContextFlags << std::endl;
	output << "  UserDataSize=" << fUserDataSize << " ResultType=" << colDataTypeToString(fResultType) << std::endl;
	output << "  ResultDecimals=" << fResultDecimals << " ResultPrecision=" << fResultPrecision << std::endl;
	output << "  ErrorMsg=" << errorMsg << std::endl;
	output << "  bInterrupted=" << bInterrupted << " RowsInPartition=" << fRowsInPartition << std::endl;
	output << "  StartFrame=" << fStartFrame << " EndFrame=" << fEndFrame << std::endl;
	output << "  StartConstant=" << fStartConstant << " EndConstant=" << fEndConstant << std::endl;
	return output.str();
}

void mcsv1Context::serialize(messageqcpp::ByteStream& b) const
{
	b.needAtLeast(sizeof(mcsv1Context));
	b << (ObjectReader::id_t) ObjectReader::MCSV1_CONTEXT;
	b << functionName;
	b << fRunFlags;
	// Dont send context flags, These are set for each call
	b << fUserDataSize;
	b << (uint32_t)fResultType;
	b << fResultDecimals;
	b << fResultPrecision;
	b << errorMsg;
	// Don't send dataflags. These are set for each call
	// bInterrupted is set internally.
	b << fRowsInPartition;
	b << (uint32_t)fStartFrame;
	b << (uint32_t)fEndFrame;
	b << fStartConstant;
	b << fEndConstant;
}

void mcsv1Context::unserialize(messageqcpp::ByteStream& b)
{
	ObjectReader::checkType(b, ObjectReader::MCSV1_CONTEXT);
	b >> functionName;
	b >> fRunFlags;
	b >> fUserDataSize;
	createUserData();
	uint32_t iResultType;
	b >> iResultType;
	fResultType = (CalpontSystemCatalog::ColDataType)iResultType;
	b >> fResultDecimals;
	b >> fResultPrecision;
	b >> errorMsg;
	b >> fRowsInPartition;
	uint32_t frame;
	b >> frame;
	fStartFrame = (WF_FRAME)frame;
	b >> frame;
	fEndFrame = (WF_FRAME)frame;
	b >> fStartConstant;
	b >> fEndConstant;
}


