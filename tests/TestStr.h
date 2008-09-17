
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2002,2003 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
 
// (c) COPYRIGHT URI/MIT 1995-1997,1999
// Please read the full copyright statement in the file COPYRIGHT_URI.
//
// Authors:
//      jhrg,jimg       James Gallagher <jgallagher@gso.uri.edu>

// Interface for TestStr type. See TestByte.h
//
// jhrg 1/12/95

#ifndef _teststr_h
#define _teststr_h 1


#include <string>

#include "dods-limits.h"
#include "Str.h"
#include "TestCommon.h"

using namespace libdap ;

class TestStr: public Str, public TestCommon {
    bool d_series_values;
    void _duplicate(const TestStr &ts);

public:
    TestStr(const string &n);
    TestStr(const string &n, const string &d);
    TestStr(const TestStr &rhs);

    virtual ~TestStr() {}

    TestStr &operator=(const TestStr &rhs);

    virtual BaseType *ptr_duplicate();
    
    virtual bool read();
    
    virtual void output_values(std::ostream &out);

    void set_series_values(bool sv) { d_series_values = sv; }
    bool get_series_values() { return d_series_values; }
};

#endif //_teststr_h

