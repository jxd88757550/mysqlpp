/// \file type_info.h
/// \brief Declares classes that provide an interface between the SQL
/// and C++ type systems.
///
/// These classes are mostly used internal to the library.

/***********************************************************************
 Copyright (c) 1998 by Kevin Atkinson, (c) 1999, 2000 and 2001 by
 MySQL AB, and (c) 2004, 2005 by Educational Technology Resources, Inc.
 Others may also hold copyrights on code in this file.  See the CREDITS
 file in the top directory of the distribution for details.

 This file is part of MySQL++.

 MySQL++ is free software; you can redistribute it and/or modify it
 under the terms of the GNU Lesser General Public License as published
 by the Free Software Foundation; either version 2.1 of the License, or
 (at your option) any later version.

 MySQL++ is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with MySQL++; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
 USA
***********************************************************************/

#ifndef MYSQLPP_TYPE_INFO_H
#define MYSQLPP_TYPE_INFO_H

#include "defs.h"

#include <mysql.h>

#include <typeinfo>
#include <map>

namespace mysqlpp {

class mysql_type_info;
class mysql_ti_sql_type_info_lookup;

/// \if INTERNAL
// Doxygen will not generate documentation for this section.

class mysql_ti_sql_type_info
{
	friend class mysql_type_info;
	friend class mysql_ti_sql_type_info_lookup;

private:
	const char* _sql_name;
	const std::type_info* _c_type;
	const unsigned char _base_type;
	const bool _default;
	
	mysql_ti_sql_type_info& operator=(const mysql_ti_sql_type_info& b);
	
	// Not initting _base_type and _default because only mysql_type_info
	// can create them.  There *must* be only one copy of each.
	mysql_ti_sql_type_info() :
	_base_type(0),
	_default(false) 
	{
	}
	
	mysql_ti_sql_type_info(const char* s, const std::type_info& t,
			const unsigned char bt = 0, const bool d = false) :
	_sql_name(s),
	_c_type(&t),
	_base_type(bt),
	_default(d)
	{
	}
};


struct type_info_cmp
{
	bool operator() (const std::type_info* lhs,
			const std::type_info* rhs) const
	{
		return lhs->before(*rhs) != 0;
	}
};

class mysql_ti_sql_type_info_lookup
{
	friend class mysql_type_info;

private:
	typedef mysql_ti_sql_type_info sql_type_info;

	std::map<const std::type_info*, unsigned char, type_info_cmp> _map;

	mysql_ti_sql_type_info_lookup(const sql_type_info types[],
			const int size);

	const unsigned char& operator [](const std::type_info& ti) const
	{
		return _map.find(&ti)->second;
	}
};

/// \endif


/// \brief Holds basic type information for ColData.
///
/// Class to hold basic type information for mysqlpp::ColData.
class mysql_type_info
{
private:
	typedef mysql_ti_sql_type_info sql_type_info;
	typedef mysql_ti_sql_type_info_lookup sql_type_info_lookup;

	static const sql_type_info types[62];

	static const unsigned char offset = 0;
	static const unsigned char unsigned_offset = 21;
	static const unsigned char null_offset = 31;
	static const unsigned char unsigned_null_offset = 52;

	static const sql_type_info_lookup lookups;

	/// \brief Return an index into mysql_type_info::types array given
	/// MySQL type information.
	///
	/// This function is used in mapping from MySQL type information
	/// (a type enum, and flags indicating whether it is unsigned and
	/// whether it can be 'null') to the closest C++ types available
	/// within MySQL++.  Notice that nulls have to be handled specially:
	/// the SQL null concept doesn't map directly onto the C++ type
	/// system.  See null.h for details.
	///
	/// \param t MySQL C API type constant, from mysql_com.h
	/// \param _unsigned if true, indicates the unsigned variant of a
	/// MySQL type
	/// \param _null if true, indicates the variant of the MySQL type
	/// that can also hold an SQL 'null' instead of regular data.
	static unsigned char type(enum_field_types t, bool _unsigned,
			bool _null = false);

public:
	static const unsigned char string_type = 20;
	unsigned int _length;
	unsigned int _max_length;

private:
	unsigned char num;
	inline const sql_type_info& deref() const;

public:
	mysql_type_info()
	{
	}
	
	mysql_type_info(unsigned char n) :
	num(n)
	{
	}

	inline mysql_type_info(enum_field_types t, bool _unsigned, bool _null);

	inline mysql_type_info(const MYSQL_FIELD& f);

	mysql_type_info(const mysql_type_info& t) :
	num(t.num)
	{
	}

	mysql_type_info(const std::type_info& t)
	{
		num = lookups[t];
	}

	mysql_type_info& operator =(unsigned char n)
	{
		num = n;
		return *this;
	}

	mysql_type_info& operator =(const mysql_type_info& t)
	{
		num = t.num;
		return *this;
	}

	mysql_type_info& operator =(const std::type_info& t)
	{
		num = lookups[t];
		return *this;
	}

	/// \brief Returns an implementation-defined name of the C++ type.
	///
	/// Returns the name that would be returned by typeid().name() for
	/// the C++ type associated with the SQL type.
	inline const char* name() const;

	/// \brief Returns the name of the SQL type.
	///
	/// Returns the SQL name for the type.
	inline const char* sql_name() const;

	/// \brief Returns the type_info for the C++ type associated with
	/// the SQL type.
	///
	/// Returns the C++ type_info record corresponding to the SQL type.
	inline const std::type_info& c_type() const;
	inline const unsigned int length() const;
	inline const unsigned int max_length() const;

	/// \brief Returns the type_info for the C++ type inside of the
	/// mysqlpp::Null type.
	///
	/// Returns the type_info for the C++ type inside the mysqlpp::Null
	/// type.  If the type is not Null then this is the same as c_type().
	inline const mysql_type_info base_type() const;

	/// \brief Returns the ID of the SQL type.
	///
	/// Returns the ID number MySQL uses for this type.  Note: Do not
	/// depend on the value of this ID as it may change between MySQL
	/// versions.
	int id() const
	{
		return num;
	}
	
	/// \brief Returns true if the SQL type is of a type that needs to
	/// be quoted.
	///
	/// \return true if the type needs to be quoted for syntactically
	/// correct SQL.
	bool quote_q() const;

	/// \brief Returns true if the SQL type is of a type that needs to
	/// be escaped.
	///
	/// \return true if the type needs to be escaped for syntactically
	/// correct SQL.
	bool escape_q() const;

	/// \brief Provides a way to compare two types for sorting.
	///
	/// Returns true if the SQL ID of this type is lower than that of
	/// another.  Used by mysqlpp::type_info_cmp when comparing types.
	bool before(mysql_type_info& b)
	{
		return num < b.num;
	}
};

inline const mysql_type_info::sql_type_info& mysql_type_info::deref() const
{
	return types[num];
}

inline const char* mysql_type_info::name() const 
{
	return deref()._c_type->name();
}

inline const char* mysql_type_info::sql_name() const
{
	return deref()._sql_name;
}

inline const unsigned int mysql_type_info::length() const
{
	return _length;
}

inline const unsigned int mysql_type_info::max_length() const
{
	return _max_length;
}

inline const std::type_info& mysql_type_info::c_type() const
{
	return *deref()._c_type;
}

inline const mysql_type_info mysql_type_info::base_type() const
{
	return mysql_type_info(deref()._base_type);
}

inline mysql_type_info::mysql_type_info(enum_field_types t,
		bool _unsigned, bool _null)
{
	num = type(t, _unsigned, _null);
}

inline mysql_type_info::mysql_type_info(const MYSQL_FIELD& f)
{
	num = type(f.type, (f.flags & UNSIGNED_FLAG) != 0,
			(f.flags & NOT_NULL_FLAG) == 0);
	_length = f.length;
	_max_length = f.max_length;
}

inline bool operator ==(const mysql_type_info& a, const mysql_type_info& b)
{
	return a.id() == b.id();
}

inline bool operator !=(const mysql_type_info& a, const mysql_type_info& b)
{
	return a.id() != b.id();
}

inline bool operator ==(const std::type_info& a, const mysql_type_info& b)
{
	return a == b.c_type();
}

inline bool operator !=(const std::type_info& a, const mysql_type_info& b)
{
	return a != b.c_type();
}

inline bool operator ==(const mysql_type_info& a, const std::type_info& b)
{
	return a.c_type() == b;
}

inline bool operator !=(const mysql_type_info& a, const std::type_info& b)
{
	return a.c_type() != b;
}

}								// end namespace mysqlpp

#endif

