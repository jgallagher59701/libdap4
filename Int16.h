// -*- C++ -*-

// (c) COPYRIGHT URI/MIT 1996
// Please read the full copyright statement in the file COPYRIGH.  
//
// Authors:
//      jhrg,jimg       James Gallagher (jgallagher@gso.uri.edu)

// Interface for Int32 type. 
//
// jhrg 9/7/94

// $Log: Int16.h,v $
// Revision 1.2  1996/12/02 23:10:20  jimg
// Added dataset as a parameter to the ops member function.
//
// Revision 1.1  1996/08/26 20:17:52  jimg
// Added.
//

#ifndef _Int16_h
#define _Int16_h 1

#ifdef __GNUG__
#pragma interface
#endif

#include <rpc/types.h>
#include <netinet/in.h>
#include <rpc/xdr.h>

#include "BaseType.h"

class Int16: public BaseType {
protected:
    dods_int16 _buf;

public:
    Int16(const String &n = (char *)0);
    virtual ~Int16() {}

    virtual BaseType *ptr_duplicate() = 0;
    
    virtual unsigned int width();

    virtual bool serialize(const String &dataset, DDS &dds, XDR *sink,
			   bool ce_eval = true);
    virtual bool deserialize(XDR *source, bool reuse = false);

    virtual bool read(const String &dataset, int &error) = 0;

    virtual unsigned int val2buf(void *buf, bool reuse = false);
    virtual unsigned int buf2val(void **val);

    virtual void print_val(ostream &os, String space = "",
			   bool print_decl_p = true);

    virtual bool ops(BaseType &b, int op, const String &dataset);
};

#endif

