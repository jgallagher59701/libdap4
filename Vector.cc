
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
 
// (c) COPYRIGHT URI/MIT 1995-1999
// Please read the full copyright statement in the file COPYRIGHT_URI.
//
// Authors:
//      jhrg,jimg       James Gallagher <jgallagher@gso.uri.edu>

// Implementation for class Vector. This class is the basis for all the
// vector-type classes in DODS <Array, List>. 
//
// 11/21/95 jhrg

#include "config_dap.h"

static char rcsid[] not_used = {"$Id: Vector.cc,v 1.59 2005/02/24 18:05:17 jimg Exp $"};

#ifdef __GNUG__
// #pragma implementation
#endif

#include <algorithm>

#include "Vector.h"
#include "escaping.h"
#include "util.h"
#include "debug.h"
#include "InternalErr.h"

#ifdef TRACE_NEW
#include "trace_new.h"
#endif

using std::cerr;
using std::endl;

void
Vector::_duplicate(const Vector &v)
{
    _length = v._length;
    
    // _var holds the type of the elements. That is, it holds a BaseType
    // which acts as a template for the type of each element.
    if (v._var) {
        _var = v._var->ptr_duplicate(); // use ptr_duplicate() 
        _var->set_parent(this);	// ptr_duplicate does not set d_parent.
    }
    else {
        _var = 0;
    }
    
    // _vec and _buf (futher down) hold the values of the Vector. The field
    // _vec is used when the Vector holds non-numeric data (including strings
    // although it used to be that was not the case jhrg 2/10/05) while _buf
    // holds numeric values.
    if (v._vec.empty()) {
	_vec = v._vec;
    }
    else {
        // Failure to set the size will make the [] operator barf on the LHS
        // of the assignment inside the loop.
        _vec.resize(_length);
        for (int i = 0; i < _length; ++i) {
            // There's no need to call set_parent() for each element; we
            // maintain the back pointer using the _var member. These
            // instances are used to hold _values_ only while the _var
            // field holds the type of the elements.
            _vec[i] = v._vec[i]->ptr_duplicate();
        }
    }

    // copy the strings. This copies the values.
    d_str = v.d_str;
    
    // copy numeric values if there are any.
    _buf = 0;			// init to null
    if (v._buf)			// only copy if data present
    	val2buf(v._buf);	// store v's value in this's _BUF.
}

/** The Vector constructor requires the name of the variable to be
    created, and a pointer to an object of the type the Vector is to
    hold.  The name may be omitted, which will create a nameless
    variable.  The template object may not be omitted.
      
    @param n A string containing the name of the variable to be
    created. 
    @param v A pointer to a variable of the type to be included 
    in the Vector. 
    @param t The type of the resulting Vector object, from the Type
    enum list.  There is no DODS Vector object, so all uses of this
    method will be from the List or Array classes.  This defaults to
    <tt>dods_null_c</tt>.

    @see Type
    @brief The Vector constructor.  */
Vector::Vector(const string &n, BaseType *v, const Type &t) 
    :BaseType(n, t), _length(-1), _var(0), _buf(0), _vec(0)
{
    if (v)
        add_var(v);
        
    DBG(cerr << "Entering Vector ctor for object: " << this << endl);
    if (_var)
        _var->set_parent(this);
}

/** The Vector copy constructor. */
Vector::Vector(const Vector &rhs) : BaseType(rhs)
{
    DBG(cerr << "Entering Vector const ctor for object: " << this << endl);
    DBG(cerr << "RHS: " << &rhs << endl);

    _duplicate(rhs);
}

Vector::~Vector()
{
    DBG(cerr << "Entering ~Vector (" << this << ")" << endl);

    delete _var; _var = 0;

    if (_buf) {
        delete[] _buf; _buf = 0;
    }
    else {
    	for (unsigned int i = 0; i < _vec.size(); ++i) {
    	    delete _vec[i]; _vec[i] = 0;
    	}
    }

    DBG(cerr << "Exiting ~Vector" << endl);
}

Vector &
Vector::operator=(const Vector &rhs)
{
    if (this == &rhs)
	return *this;

    dynamic_cast<BaseType &>(*this) = rhs;

    _duplicate(rhs);

    return *this;
}

int
Vector::element_count(bool leaves)
{
    if (!leaves)
	   return 1;
    else
	   // var() only works for simple types! 
	   return var(0)->element_count(leaves);
}

// These mfuncs set the _send_p and _read_p fields of BaseType. They differ
// from BaseType's version in that they set both the Vector object's copy of
// _send_p (_read_p) but also _VAR's copy. This does not matter much when _VAR
// is a scalar, but does matter when it is an aggregate.

/** This function sets the <tt>send_p</tt> flag for both the Vector itself
    and its element template.  This does not matter much when the
    Vector contains simple data types, but does become significant
    when the Vector contains compound types.  

    @brief Indicates that the data is ready to send. */
void
Vector::set_send_p(bool state)
{
    _var->set_send_p(state);
    BaseType::set_send_p(state);
}

/** This function sets the <tt>read_p</tt> flag for both the Vector itself
    and its element template.  This does not matter much when the
    Vector contains simple data types, but does become significant
    when the Vector contains compound types.

    @brief Indicates that the data is ready to send.  */
void 
Vector::set_read_p(bool state)
{
    _var->set_read_p(state);
    BaseType::set_read_p(state);
}

/** Returns a copy of the template array element. If the Vector contains
    simple data types, the template will contain the value of the last
    vector element accessed with the <code>Vector::var(int i)</code> function,
    if any. If no such access has been made, or if the Vector contains
    compound data types, the value held by the template instance is
    undefined.

    Note that the parameter <i>exact_match</i> is not used by this mfunc.

    @param n The name of the variabe to find.
    @param exact Unused.
    @return A pointer to the BaseType if found, otherwise null.
    @see Vector::var */
BaseType *
Vector::var(const string &n, bool exact, btp_stack *s)
{
    string name = www2id(n);

    // Make sure to check for the case where name is the default (the empty
    // string). 9/1/98 jhrg
    if (_var->is_constructor_type()) {
    	if (name == "" || _var->name() == name)
    	    return _var;
    	else
    	    return _var->var(name, exact, s);
    }
    else
	   return _var;
}

/** This version of var(...) searches for <i>name</i> and returns a
    pointer to the BaseType object if found. It uses the same search
    algorithm as above when <i>exact_match</i> is false. In addition to
    returning a pointer to the variable, it pushes onto <i>s</i> a
    BaseType pointer to each constructor type that ultimately contains
    <i>name</i>.

    @param n Find the variable whose name is <i>name</i>.
    @param s Record the path to <i>name</i>.
    @return A pointer to the named variable. */
BaseType *
Vector::var(const string &n, btp_stack &s)
{
    string name = www2id(n);

    if (_var->is_constructor_type())
	   return _var->var(name, s);
    else {
	   s.push((BaseType *)this);
	   return _var;
    }
}

// Return a pointer the the BaseType object for element I. If the Vector is
// of a cardinal type, store the Ith element's value in the BaseType
// object. If it is a Vector of a non-cardinal type, then this mfunc returns
// _vec[i]. 
//
// NB: I defaults to zero.
//
// Returns: A BaseType pointer to the Ith element of the Vector.

/** Returns a pointer to the specified Vector element.  For Vectors
    containing simple data types, the element returned will be a
    copy of the indicated element.  For compound types, the return
    pointer will indicate the element itself.

    @param i The index of the desired Vector element.  Zero
    indicates the first element of the Vector.
    @return A pointer to a BaseType class instance containing
    the value of the indicated element.
    @see BaseType::var */
BaseType *
Vector::var(unsigned int i)
{
    
    switch (_var->type()) {
      case dods_byte_c:
      case dods_int16_c:
      case dods_uint16_c:
      case dods_int32_c:
      case dods_uint32_c:
      case dods_float32_c:
      case dods_float64_c: {
	  // Transfer the ith value to the BaseType *_var; There are more
	  // efficient ways to get a whole array using buf2val() but this is
	  // an OK way to get a single value or several non-contiguous values.
	  unsigned int sz = _var->width();
	  _var->val2buf((char *)_buf + (i * sz));
	  return _var;
	  break;
      }

      case dods_str_c:
      case dods_url_c:
	_var->val2buf( &d_str[i] );
	return _var;
	break ;

      case dods_array_c:
      case dods_structure_c:
      case dods_sequence_c:
      case dods_grid_c:
	return _vec[i];
	break;

      default:
	cerr << "Vector::var: Unrecognized type" << endl;
    }

    return 0;
}

// Return: The number of bytes required to store the vector `in a C
// program'. For an array of cardinal types this is the same as the storage
// used by _BUF. For anything else, it is the product of length() and the
// element width(). It turns out that both values can be computed the same
// way. 
//
// Returns: The number of bytes used to store the vector.

/** Returns the number of bytes needed to hold the <i>entire</i>
    array.  This is equal to <tt>length()</tt> times the width of each
    element. 

    @brief Returns the width of the data, in bytes. */
unsigned int
Vector::width()
{
    // Jose Garcia
    if (!_var)
      throw InternalErr(__FILE__, __LINE__, 
	      "Cannot get width since *this* object is not holding data.");

    return length() * _var->width();
}

// Returns: the number of elements in the vector. 

    /** Returns the number of elements in the vector. Note that some
	child classes of Vector use the length of -1 as a flag value.

	@see Array::append_dim */
int
Vector::length()
{
    return _length;
}

// set the number of elements in the vector.
//
// Returns: void

    /** Sets the length of the vector.  This function does not allocate
	any new space. */
void
Vector::set_length(int l)
{
    _length = l;
}

// \e l is the number of elements the vector can hold (e.g., if l == 20, then
// the vector can hold elements 0, .., 19).

/** Resizes a Vector.  If the input length is greater than the
    current length of the Vector, new memory is allocated (the
    Vector moved if necessary), and the new entries are appended to
    the end of the array and padded with Null values.  If the input
    length is shorter, the tail values are discarded. */
void
Vector::vec_resize(int l)
{
    _vec.resize((l > 0) ? l : 0, 0); // Fill with NULLs
}

/** @brief Serialize a Vector. 

    This uses the BaseType member xdr_coder to encode each element of
    a cardinal array. See Sun's XDR manual. For Arrays of Str and Url
    types, send the element count over as a prefix to the data so that
    deserialize will know how many elements to read. 

    NB: Arrays of cardinal types must already be in BUF (in the local machine's
    representation) <i>before</i> this call is made.
*/


bool
Vector::serialize(const string &dataset, DDS &dds, XDR *sink, 
		  bool ce_eval)
{
	int i = 0;

    dds.timeout_on();

    if (!read_p())
	read(dataset);		// read() throws Error and InternalErr

    if (ce_eval && !dds.eval_selection(dataset))
	return true;

    dds.timeout_off();

    // length() is not capacity; it must be set explicitly in read().
    int num = length();

    switch (_var->type()) {
      case dods_byte_c:
      case dods_int16_c:
      case dods_uint16_c:
      case dods_int32_c:
      case dods_uint32_c:
      case dods_float32_c:
      case dods_float64_c:
	// Jose Garcia
	// If we are trying to serialize data and the internal buffer is
	// unset, then the read method failed to do its job.
	if(!_buf)
	    throw InternalErr(__FILE__, __LINE__, 
			      "Buffer pointer is not set.");

	if ((0 == xdr_int(sink, (int *)&num))) // send vector length
	    throw Error(
"Network I/O Error. This may be due to a bug in DODS or a\n\
problem with the network connection.");

	// Note that xdr_bytes and xdr_array both send the length themselves,
	// so the length is actually sent twice. 12/31/99 jhrg
	bool status;
	if (_var->type() == dods_byte_c)
	    status = (0 != xdr_bytes(sink, (char **)&_buf, 
				     (unsigned int *)&num,
				     DODS_MAX_ARRAY)); 
	else
	    status = (0 != xdr_array(sink, (char **)&_buf,
				     (unsigned int *)&num,
				     DODS_MAX_ARRAY, _var->width(),
				     (xdrproc_t)(_var->xdr_coder())));
	if (!status)
	    throw Error(
"Network I/O Error. This may be due to a bug in DODS or a\n\
problem with the network connection.");
	
	break;

      case dods_str_c:
      case dods_url_c:
	if(d_str.capacity() == 0)
	    throw InternalErr(__FILE__, __LINE__, 
			      "The capacity of the string vector is 0");

	if ((0 == xdr_int(sink, (int *)&num)))
	    throw Error(
"Network I/O Error. This may be due to a bug in DODS or a\n\
problem with the network connection.");

	for (i = 0; i < num; ++i)
	    if (!xdr_str(sink, d_str[i]))
		throw Error(
"Network I/O Error. Could not send string data.\n\
This may be due to a bug in DODS, on the server or a\n\
problem with the network connection.");

	break ;

      case dods_array_c:
      case dods_structure_c:
      case dods_sequence_c:
      case dods_grid_c:
	//Jose Garcia
	// Not setting the capacity of _vec is an internal error.
	if(_vec.capacity() == 0)
	    throw InternalErr(__FILE__, __LINE__, 
			      "The capacity of *this* vector is 0.");

	if ((0 == xdr_int(sink, (int *)&num)))
	    throw Error(
"Network I/O Error. This may be due to a bug in DODS or a\n\
problem with the network connection.");

	for (i = 0; i < num; ++i)
	    _vec[i]->serialize(dataset, dds, sink, false);

	break;

      default:
	throw InternalErr(__FILE__, __LINE__, "Unknown datatype.");
	break;
    }

    return true;
}

// Read an object from the network and internalize it. For a Vector this is
// handled differently for a `cardinal' type. Vectors of Cardinals are
// stored using the `C' representations because these objects often are used
// to build huge arrays (e.g., an array of 1024 by 1024 bytes). However,
// arrays of non-cardinal types are stored as Vectors of the C++ objects or
// DODS objects (Str and Url are vectors of the string class, Structure, ...,
// Grid are vectors of the DODS Structure, ... classes). 
//
// The boolean parameter REUSE determines whether internal storage is reused
// or not. If true, the _buf member is assumed to be large enough to hold the
// incoming cardinal data and is *not* reallocated. If false, new storage is
// allocated. If the internal buffer has not yet been allocated, then this
// parameter has no effect (i.e., storage is allocated). This parameter
// effects storage for cardinal data only.
//
// Returns: True is successful, false otherwise.

bool
Vector::deserialize(XDR *source, DDS *dds, bool reuse)
{
    bool status;
    unsigned int num;
	unsigned i = 0;

    switch (_var->type()) {
      case dods_byte_c:
      case dods_int16_c:
      case dods_uint16_c:
      case dods_int32_c:
      case dods_uint32_c:
      case dods_float32_c:
      case dods_float64_c:
	if (_buf && !reuse)
	    delete[] _buf; _buf = 0;

	if ((0 == xdr_int(source, (int *)&num)))
	    throw Error(
"Network I/O error. Could not read the array length.\n\
This may be due to a bug in DODS or a problem with\n\
the network connection.");

	DBG(cerr << "Vector::deserialize: num = " << num << endl);
	DBG(cerr << "Vector::deserialize: length = " << length() << endl);

	if (length() == -1)
	    set_length(num);
	
	if (num != (unsigned int)length())
	    throw InternalErr(__FILE__, __LINE__,
	      "The server sent declarations and data with mismatched sizes.");

	if (!_buf) {
	    _buf = new char[width()]; // we always do the allocation!
	    DBG(cerr << "Vector::deserialize: allocating " \
		<< width() << " bytes for an array of " \
		<< length() << " " << _var->type_name() << endl);
	}

	if (_var->type() == dods_byte_c)
	    status = (0 != xdr_bytes(source, (char **)&_buf, &num,
				     DODS_MAX_ARRAY)); 
	else
	    status = (0 != xdr_array(source, (char **)&_buf, &num,
				     DODS_MAX_ARRAY, _var->width(),
				     (xdrproc_t)(_var->xdr_coder())));

	if (!status)
	    throw Error(
"Network I/O error. Could not read packed array data.\n\
This may be due to a bug in DODS or a problem with\n\
the network connection.");

	DBG(cerr << "Vector::deserialize: read " << num <<  " elements\n");

	break;

      case dods_str_c:
      case dods_url_c:
	if ((0 == xdr_int(source, (int *)&num)))
	    throw Error(
"Network I/O error. Could not read the array length.\n\
This may be due to a bug in DODS or a problem with\n\
the network connection.");

	if (length() == -1)
	    set_length(num);
	
	if (num != (unsigned int)length())
	    throw InternalErr(__FILE__, __LINE__,
	      "The client sent declarations and data with mismatched sizes.");

	d_str.resize((num > 0) ? num : 0); // Fill with NULLs

	for (i = 0; i < num; ++i) {
	    string str ;
	    if (!xdr_str(source, str))
		throw Error(
"Network I/O Error. Could not read string data. This may be due to a\n\
bug in DODS or a problem with the network connection.");
	    d_str[i] = str ;

	}

	break ;

      case dods_array_c:
      case dods_structure_c:
      case dods_sequence_c:
      case dods_grid_c:
	if ((0 == xdr_int(source, (int *)&num)))
	    throw Error(
"Network I/O error. Could not read the array length.\n\
This may be due to a bug in DODS or a problem with\n\
the network connection.");

	if (length() == -1)
	    set_length(num);
	
	if (num != (unsigned int)length())
	    throw InternalErr(__FILE__, __LINE__,
	      "The client sent declarations and data with mismatched sizes.");

	vec_resize(num);

	for (i = 0; i < num; ++i) {
	    _vec[i] = _var->ptr_duplicate();
	    _vec[i]->deserialize(source, dds);
	}

	break;

      default:
	throw InternalErr(__FILE__, __LINE__, "Unknow type!");
	break;
    }

    return false;
}

/** Copies data into the class instance buffer.  This function
    assumes that the input \e val points to memory which
    contains, in row major order, enough elements of the correct
    type to fill the array. For an array of a cardinal type the
    memory is simply copied in whole into the Vector buffer.  
    
    For a Vector of Str (OPeNDAP Strings), this assumes \e val points to an
    array of C++ strings.
    
    This method should not be used for Structure, Sequence or Grid.
    
    @brief Reads data into the Vector buffer.
    @exception InternalErr Thrown if called for Structure, Sequence or 
    Grid.
    @return The number of bytes used by the array.
    @param val A pointer to the input data.
    @param reuse A boolean value, indicating whether the class
    internal data storage can be reused or not.  If this argument is
    TRUE, the class buffer is assumed to be large enough to hold the
    incoming data, and it is <i>not</i> reallocated.  If FALSE, new
    storage is allocated.  If the internal buffer has not been
    allocated at all, this argument has no effect. */
unsigned int
Vector::val2buf(void *val, bool reuse)
{
    // Jose Garcia

    // I *think* this method has been mainly designed to be use by read which
    // is implemented in the surrogate library. Passing NULL as a pointer to
    // this method will be an error of the creator of the surrogate library.
    // Even though I recognize the fact that some methods inside libdap++ can
    // call val2buf, I think by now no coding bugs such as missusing val2buf
    // will be in libdap++, so it will be an internal error from the
    // surrogate library.
    if(!val)
	throw InternalErr(__FILE__, __LINE__, 
			  "The incoming pointer does not contain any data.");

    switch (_var->type()) {
      case dods_byte_c:
      case dods_int16_c:
      case dods_uint16_c:
      case dods_int32_c:
      case dods_uint32_c:
      case dods_float32_c:
      case dods_float64_c: {
	  unsigned int array_wid = width();

	  if (_buf && !reuse) {
	      delete[] _buf; _buf = 0;
	  }

	  if (!_buf) {		// First time or no reuse (free'd above)
	      _buf = new char[array_wid];
	      memcpy(_buf, val, array_wid);
	  }
	  else { 
	      memcpy(_buf, val, array_wid);
	  }

	  break;
      }

      case dods_str_c:
      case dods_url_c: {
          // Assume val points to an array of C++ string objects. Copy
          // them into the vector<string> field of this object.
          d_str.resize(_length);
          for (int i = 0; i < _length; ++i)
              d_str[i] = *(static_cast<string*>(val)+i);
#if 0              
	  unsigned elem_wid = _var->width();
	  unsigned len = length();

	  vec_resize(len);

	  for (unsigned i = 0; i < len; ++i) {
	      _vec[i] = _var->ptr_duplicate(); //changed, reza
	      _vec[i]->val2buf((char *)val + i * elem_wid, reuse);
	  }
#endif

	  break;
      }

      default:
	throw InternalErr(__FILE__, __LINE__, "Vector::val2buf: bad type");
	return 0;
    }

    return width();
}
 
/** Copies data from the Vector buffer.  This function assumes that
    <i>val</i> points to an array large enough to hold N instances of
    the `C' representation of the \e numeric element type or C++ string
    objects. Never call this method for constructor types Structure,
    Sequence or Grid.

    In the case of a Vector of Str objects, this method will return an array
    of C++ std::string objects.

    @return The number of bytes used to store the array.
    @param val A pointer to a pointer to the memory into which the
    class data will be copied.  If the value pointed to is NULL,
    memory will be allocated to hold the data, and the pointer value
    modified accordingly.  The calling program is responsible for
    deallocating the memory indicated by this pointer.  
    @exception InternalErr Thrown if \e val is null.
    @see Vector::set_vec */
unsigned int
Vector::buf2val(void **val)
{
    // Jose Garcia
    // The same comment in Vector::val2buf applies here!
    if (!val)
	throw InternalErr(__FILE__, __LINE__, "NULL pointer.");
    
    int wid = width();

    switch (_var->type()) {
      case dods_byte_c:
      case dods_int16_c:
      case dods_uint16_c:
      case dods_int32_c:
      case dods_uint32_c:
      case dods_float32_c:
      case dods_float64_c:
	if (!*val)
	    *val = new char[wid];

	(void)memcpy(*val, _buf, wid);

	break;

      case dods_str_c:
      case dods_url_c: {
        if (!*val)
            *val = new string[_length];
            
        for (int i = 0; i < _length; ++i)
            *(static_cast<string*>(*val) + i) = d_str[i];
#if 0            
	unsigned int elem_wid = _var->width();
	unsigned int len = length();

 	if (!*val) {
	    *val = new string *[len];
	    memcpy(*val, 0, len*elem_wid);
	}

#if 0
 	if (!*val)
	    *val = new string [len]; 
#endif

	for (unsigned i = 0; i < len; ++i) {
	    void *val_elem = (char *)*val + i * elem_wid;
	    _vec[i]->buf2val(&val_elem);
	}
#endif
	break;
      }

      default:
	throw InternalErr(__FILE__, __LINE__, "Vector::buf2val: bad type");
	return 0;
    }

    return wid;
}

/** Sets an element of the vector to a given value.  If the type of
    the input and the type of the Vector do not match, an error
    condition is returned.

    Use this function only with Vectors containing compound DODS
    types.  See <tt>buf2val()</tt> to access members of Vectors containing
    simple types.

    @note This method copies \e val; the caller is responsible for deleting
    instance passed as the actual parameter.
    
    @brief Sets element <i>i</i> to value <i>val</i>.
    @return void
    @exception InternalErr Thrown if \e i is out of range, \e val is null or
    there was a type mismatch between the BaseType referenced by \e val and
    the \e ith element of this Vector.
    @param i The index of the element to be changed.
    @param val A pointer to the value to be inserted into the
    array.  
    @see Vector::buf2val */
void
Vector::set_vec(unsigned int i, BaseType *val)
{
    // Jose Garcia
    // This is a public method which allows users to set the elements
    // of *this* vector. Passing an invalid index, a NULL pointer or 
    // missmatching the vector type are internal errors.
    if (i<0)
	throw InternalErr(__FILE__, __LINE__, "Invalid data: negative index.");
    if (i>=static_cast<unsigned int>(_length))
        throw InternalErr(__FILE__, __LINE__, "Invalid data: index too large.");
    if (!val)
	throw InternalErr(__FILE__, __LINE__, 
			  "Invalid data: null pointer to BaseType object.");
    if (val->type() != _var->type())
	throw InternalErr(__FILE__, __LINE__, 
  "invalid data: type of incoming object does not match *this* vector type.");

    if (i >= _vec.capacity())
	vec_resize(i + 10);
   
    _vec[i] = val->ptr_duplicate();
}
 
/** @brief Add the BaseType pointer to this constructor type
    instance. 

    Propagate the name of the BaseType instance to this instance. This
    ensures that variables at any given level of the DDS table have
    unique names (i.e., that Arrays do not have their default name ""). If
    <tt>v</tt>'s name is null, then assume that the array \i is named and
    don't overwrite it with <tt>v</tt>'s null name.

    @param v The template variable for the array
    @param p The Part parameter defaults to nil and is ignored by this method.
*/
void
Vector::add_var(BaseType *v, Part)
{
    // Delete the current template variable
    delete _var;
    
    // if 'v' is null, just set _var to null and exit.
    if (!v) {
        _var = 0;
    }
    else {
        // Jose Garcia
        // By getting a copy of this object to be assigned to _var
        // we let the owner of 'v' to deallocate it as necessary.
        _var = v->ptr_duplicate();

	// If 'v' has a name, use it as the name of the array. If it *is*
	// empty, then make sure to copy the array's name to the template
	// so that software which uses the template's name will still work.
        if (!v->name().empty())
	    set_name(v->name());
        else
	    _var->set_name(name());

        _var->set_parent(this);	// Vector --> child
      
        DBG(cerr << "Vector::add_var: Added variable " << v << " ("
            << v->name() << " " << v->type_name() << ")" << endl);
    }
}

bool
Vector::check_semantics(string &msg, bool) 
{
    return BaseType::check_semantics(msg);
}

// $Log: Vector.cc,v $
// Revision 1.59  2005/02/24 18:05:17  jimg
// A throw Error line was left in deserialize() by mistake; I removed it.
//
// Revision 1.58  2005/02/16 17:28:48  pwest
// d_str changes to ::var, ::serialize and ::deserialize
//
// Revision 1.57  2005/02/14 22:11:27  jimg
// Added code to use the new d_str field; a special field to hold strings.
//
// Revision 1.56  2005/02/11 00:52:45  jimg
// Fixed eclipse/cvs comment.
//
// Revision 1.55  2005/02/11 00:50:18  jimg
// _duplicate() now performs a deep copy on the BaseType* instances used to
// hold values in the field _vec (used when there's a Vector of Structures, 
// et c.). set_vec() now calls ptr_duplicate() on the BaseType* passed in to
// be used as a value for element i. This might seem odd (and it might hose 
// performance, we may have to add another method to skirt the copy for some
// people) but it fits with the memory model where anything allocated outside
// the library can be freed outside the library. Not performing the deep 
// copy and then freeing the objects in _vec in the dtor caused all sorts of
// problems, as one might imagine. I also fixed up the comments.
//
// Revision 1.54  2005/02/08 20:43:12  jimg
// Removed parens around 'string *' at line 735; gcc-3.4 complained.
//
// Revision 1.53  2005/01/28 17:25:12  jimg
// Resolved conflicts from merge with release-3-4-9
//
// Revision 1.45.2.9  2005/01/18 23:26:18  jimg
// FIxed documentation.
//
// Revision 1.52  2004/11/16 22:50:20  jimg
// Fixed tests. Also fixed a bug intorduced in Vector where a template
// with no name caused some software (any code which depends on the
// template having the same name as the array) to fail.
//
// Revision 1.51  2004/11/16 18:04:23  jimg
// Modified the ctor and add_var() method so that a null variable can be added
// as a template.  This makes it possible for a client of the library to create an
// Array without allocating its template. This is used by the netCDF CL's
// translation software when it needs to create a set of arrays that have the
// same dimensions. The 'master' array object is created and then duplicated
// when each of the different template variables are created/added.
//
// Revision 1.50  2004/09/16 15:19:42  jimg
// Corrected the commets for set_vec() and buf2val().
//
// Revision 1.49  2004/07/07 21:08:48  jimg
// Merged with release-3-4-8FCS
//
// Revision 1.45.2.8  2004/07/02 20:41:53  jimg
// Removed (commented) the pragma interface/implementation lines. See
// the ChangeLog for more details. This fixes a build problem on HP/UX.
//
// Revision 1.45.2.7  2004/05/18 20:26:59  jimg
// Fixed some errors in the debug messages that made them hard(er) to decipher.
//
// Revision 1.48  2003/12/08 18:02:30  edavis
// Merge release-3-4 into trunk
//
// Revision 1.45.2.6  2003/09/28 20:57:22  rmorris
// Discontinued use of XDR_PROC typedef, using xdrproc_t instead - a
// define from the xdr portion of the rpc library.
//
// Revision 1.45.2.5  2003/09/06 22:58:06  jimg
// Updated the documentation. Now uses the XDR_PROC typedef.
//
// Revision 1.45.2.4  2003/08/18 00:31:45  rmorris
// Removed casting xdr_* routines to bool.  bool is size of 1
// under win32 yet size of 4 under unix.  This is cleanup in regards
// to win32 and 'performance warnings' at compile time.
//
// Revision 1.45.2.3  2003/08/17 01:34:26  rmorris
// Account for trivial differences to scope of variables in case
// statements under win32.  It's not in the case block, but the enclosing
// scope.
//
// Revision 1.45.2.2  2003/07/25 06:04:28  jimg
// Refactored the code so that DDS:send() is now incorporated into
// DODSFilter::send_data(). The old DDS::send() is still there but is
// depracated.
// Added 'smart timeouts' to all the variable classes. This means that
// the new server timeouts are active only for the data read and CE
// evaluation. This went inthe BaseType::serialize() methods because it
// needed to time both the read() calls and the dds::eval() calls.
//
// Revision 1.45.2.1  2003/07/18 01:07:15  jimg
// I fixed up the documentation.
//
// Revision 1.47  2003/05/30 16:32:32  jimg
// Modified add_var() so that the array only adopts the name of the template
// variable when that name is not null!
//
// Revision 1.46  2003/05/23 03:24:57  jimg
// Changes that add support for the DDX response. I've based this on Nathan
// Potter's work in the Java DAP software. At this point the code can
// produce a DDX from a DDS and it can merge attributes from a DAS into a
// DDS to produce a DDX fully loaded with attributes. Attribute aliases
// are not supported yet. I've also removed all traces of strstream in
// favor of stringstream. This code should no longer generate warnings
// about the use of deprecated headers.
//
// Revision 1.45  2003/04/22 19:40:28  jimg
// Merged with 3.3.1.
//
// Revision 1.44  2003/02/21 00:14:25  jimg
// Repaired copyright.
//
// Revision 1.43.2.1  2003/02/21 00:10:07  jimg
// Repaired copyright.
//
// Revision 1.43  2003/01/23 00:22:24  jimg
// Updated the copyright notice; this implementation of the DAP is
// copyrighted by OPeNDAP, Inc.
//
// Revision 1.42  2003/01/10 19:46:40  jimg
// Merged with code tagged release-3-2-10 on the release-3-2 branch. In many
// cases files were added on that branch (so they appear on the trunk for
// the first time).
//
// Revision 1.35.4.12  2002/12/27 19:34:42  jimg
// Modified the var() methods so that www2id() is called before looking
// up identifier names. See bug 563.
//
// Revision 1.35.4.11  2002/12/17 22:35:03  pwest
// Added and updated methods using stdio. Deprecated methods using iostream.
//
// Revision 1.35.4.10  2002/08/08 06:54:57  jimg
// Changes for thread-safety. In many cases I found ugly places at the
// tops of files while looking for globals, et c., and I fixed them up
// (hopefully making them easier to read, ...). Only the files RCReader.cc
// and usage.cc actually use pthreads synchronization functions. In other
// cases I removed static objects where they were used for supposed
// improvements in efficiency which had never actually been verifiied (and
// which looked dubious).
//
// Revision 1.41  2002/06/18 15:36:24  tom
// Moved comments and edited to accommodate doxygen documentation-generator.
//
// Revision 1.40  2002/06/03 22:21:15  jimg
// Merged with release-3-2-9
//
// Revision 1.35.4.9  2002/05/22 16:57:51  jimg
// I modified the `data type classes' so that they do not need to be
// subclassed for clients. It might be the case that, for a complex client,
// subclassing is still the best way to go, but you're not required to do
// it anymore.
//
// Revision 1.35.4.8  2002/01/30 18:52:26  jimg
// Vector now throws an InternalErr object when a DataDDS arrives and the amount
// of data differs from the declarations. Note that List (a subclass of Vector)
// does not include any size info, so we assume that the size information in the
// binary object is correct.
//
// Revision 1.35.4.7  2001/10/30 06:55:45  rmorris
// Win32 porting changes.  Brings core win32 port up-to-date.
//
// Revision 1.39  2001/10/14 01:28:38  jimg
// Merged with release-3-2-8.
//
// Revision 1.35.4.6  2001/10/02 17:01:52  jimg
// Made the behavior of serialize and deserialize uniform. Both methods now
// use Error exceptions to signal problems with network I/O and InternalErr
// exceptions to signal other problems. The return codes, always true for
// serialize and always false for deserialize, are now meaningless. However,
// by always returning a code that means OK, old code should continue to work.
//
// Revision 1.38  2001/09/28 17:50:07  jimg
// Merged with 3.2.7.
//
// Revision 1.35.4.5  2001/09/07 00:38:35  jimg
// Sequence::deserialize(...) now reads all the sequence values at once.
// Its call semantics are the same as the other classes' versions. Values
// are stored in the Sequence object using a vector<BaseType *> for each
// row (those are themselves held in a vector). Three new accessor methods
// have been added to Sequence (row_value() and two versions of var_value()).
// BaseType::deserialize(...) now always returns true. This matches with the
// expectations of most client code (the seqeunce version returned false
// when it was done reading, but all the calls for sequences must be changed
// anyway). If an XDR error is found, deserialize throws InternalErr.
//
// Revision 1.37  2001/08/24 17:46:22  jimg
// Resolved conflicts from the merge of release 3.2.6
//
// Revision 1.35.4.4  2001/08/18 00:13:21  jimg
// Removed WIN32 compile guards from using statements.
//
// Revision 1.35.4.3  2001/07/28 01:10:42  jimg
// Some of the numeric type classes did not have copy ctors or operator=.
// I added those where they were needed.
// In every place where delete (or delete []) was called, I set the pointer
// just deleted to zero. Thus if for some reason delete is called again
// before new memory is allocated there won't be a mysterious crash. This is
// just good form when using delete.
// I added calls to www2id and id2www where appropriate. The DAP now handles
// making sure that names are escaped and unescaped as needed. Connect is
// set to handle CEs that contain names as they are in the dataset (see the
// comments/Log there). Servers should not handle escaping or unescaping
// characters on their own.
//
// Revision 1.36  2001/06/15 23:49:03  jimg
// Merged with release-3-2-4.
//
// Revision 1.35.4.2  2001/06/05 06:49:19  jimg
// Added the Constructor class which is to Structures, Sequences and Grids
// what Vector is to Arrays and Lists. This should be used in future
// refactorings (I thought it was going to be used for the back pointers).
// Introduced back pointers so children can refer to their parents in
// hierarchies of variables.
// Added to Sequence methods to tell if a child sequence is done
// deserializing its data.
// Fixed the operator=() and copy ctors; removed redundency from
// _duplicate().
// Changed the way serialize and deserialize work for sequences. Now SOI and
// EOS markers are written for every `level' of a nested Sequence. This
// should fixed nested Sequences. There is still considerable work to do
// for these to work in all cases.
//
// Revision 1.35.4.1  2001/05/23 16:23:17  jimg
// Replaced errors written to cerr with Error exceptions.
// Removed some WIN32 specific code that could be generalized for both Win32 and
// Unix.
//
// Revision 1.35  2000/10/06 01:26:05  jimg
// Changed the way serialize() calls read(). The status from read() is
// returned by the Structure and Sequence serialize() methods; ignored by
// all others. Any exceptions thrown by read() are caught and discarded.
// serialize() returns false if read() throws an exception. This should
// be fixed once all the servers build using the new read() definition.
//
// Revision 1.34  2000/10/02 18:48:59  jimg
// Changed type of num in the serialize method to int
//
// Revision 1.33  2000/09/22 02:17:22  jimg
// Rearranged source files so that the CVS logs appear at the end rather than
// the start. Also made the ifdef guard symbols use the same naming scheme and
// wrapped headers included in other headers in those guard symbols (to cut
// down on extraneous file processing - See Lakos).
//
// Revision 1.32  2000/09/21 16:22:09  jimg
// Merged changes from Jose Garcia that add exceptions to the software.
// Many methods that returned error codes now throw exectptions. There are
// two classes which are thrown by the software, Error and InternalErr.
// InternalErr is used to report errors within the library or errors using
// the library. Error is used to reprot all other errors. Since InternalErr
// is a subclass of Error, programs need only to catch Error.
//
// Revision 1.31  2000/07/09 22:05:36  rmorris
// Changes to increase portability, minimize ifdef's for win32 and account
// for differences in the iostreams implementations.
//
// Revision 1.30  2000/06/16 18:15:00  jimg
// Merged with 3.1.7
//
// Revision 1.27.6.2  2000/06/14 16:59:01  jimg
// Added instrumentation for the dtor.
//
// Revision 1.27.6.1  2000/06/07 23:08:31  jimg
// Added code to explicitly delete BaseType *s in _vec.
// Also tried recoding using DLList, but that didn't fix the problem I was
// after---fixed in the client code but decided to leave this is with #if 0
// just in case.
//
// Revision 1.29  2000/06/07 18:06:59  jimg
// Merged the pc port branch
//
// Revision 1.28.8.1  2000/06/02 18:29:32  rmorris
// Mod's for port to Win32.
//
// Revision 1.28.2.2  2000/02/17 05:03:16  jimg
// Added file and line number information to calls to InternalErr.
// Resolved compile-time problems with read due to a change in its
// parameter list given that errors are now reported using exceptions.
//
// Revision 1.28.2.1  2000/01/28 22:14:07  jgarcia
// Added exception handling and modify add_var to get a copy of the object
//
// Revision 1.28  2000/01/05 22:37:18  jimg
// Added a comment about the odd `protocol' for sending array/list lengths twice
// for arrays/lists of simple types.
// Removed some cruft.
//
// Revision 1.27  1999/05/04 19:47:23  jimg
// Fixed copyright statements. Removed more of the GNU classes.
//
// Revision 1.26  1999/04/29 02:29:34  jimg
// Merge of no-gnu branch
//
// Revision 1.25  1999/03/24 23:34:49  jimg
// Added support for the new Int16, UInt16 and Float32 types.
//
// Revision 1.24  1998/09/17 17:05:46  jimg
// Changes for the new variable lookup scheme. Fields of ctor types no longer
// need to be fully qualified. my.thing.f1 can now be named `f1' in a CE. Note
// that if there are two `f1's in a dataset, the first will be silently used;
// There's no warning about the situation. The new code in the var member
// function passes a stack of BaseType pointers so that the projection
// information (send_p field) can be set properly.
//
// Revision 1.23.2.2  1999/02/05 09:32:35  jimg
// Fixed __unused__ so that it not longer clashes with Red Hat 5.2 inlined
// math code. 
//
// Revision 1.23.2.1  1999/02/02 21:57:04  jimg
// String to string version
//
// Revision 1.23  1998/08/06 16:22:38  jimg
// Fixed the misuse of the read(...) member function. See Grid.c (from jeh).
//
// Revision 1.22  1998/03/17 17:51:06  jimg
// Added an implementation of element_count().
//
// Revision 1.21  1998/02/05 20:13:58  jimg
// DODS now compiles with gcc 2.8.x
//
// Revision 1.20  1997/12/31 21:48:12  jimg
// Enclosed print_basetype_pointer() function in #if DODS_DEBUG == 1 #endif
// to stop compiler warnings about it being unused.
//
// Revision 1.19  1997/12/15 22:33:00  jimg
// Added type checking set_vec. If the type of the element to include in the
// vector does not match the vector's type, set_vec() returns false.
//
// Revision 1.18  1997/09/22 22:37:53  jimg
// Fixed a bug in vec_resize.
//
// Revision 1.17  1997/03/08 19:02:11  jimg
// Changed default param to check_semantics() from  to String()
// and removed the default from the argument list in the mfunc definition
//
// Revision 1.16  1997/02/28 01:28:03  jimg
// Changed check_semantics() so that it now returns error messages in a String
// object (passed by reference).
//
// Revision 1.15  1996/12/02 18:22:08  jimg
// Added cases for uint32 to various parts of Vector.
//
// Revision 1.14  1996/11/13 19:23:10  jimg
// Fixed debugging.
//
// Revision 1.13  1996/08/13 18:39:25  jimg
// Added not_used to definition of char rcsid[].
// Fixed int -vs- unsigned int discrepancies.
//
// Revision 1.12  1996/06/04 21:33:50  jimg
// Multiple connections are now possible. It is now possible to open several
// URLs at the same time and read from them in a round-robin fashion. To do
// this I added data source and sink parameters to the serialize and
// deserialize mfuncs. Connect was also modified so that it manages the data
// source `object' (which is just an XDR pointer).
//
// Revision 1.11  1996/05/31 23:30:42  jimg
// Updated copyright notice.
//
// Revision 1.10  1996/05/30 17:14:56  jimg
// Fixed the allocation of vectors of DODS variable objects; use the
// ptr_duplicate member function instead of the copy ctor in val2buf().
// Fixed allocation of vector of String in buf2val(). From Reza.
//
// Revision 1.9  1996/05/29 22:08:53  jimg
// Made changes necessary to support CEs that return the value of a function
// instead of the value of a variable. This was done so that it would be
// possible to translate Sequences into Arrays without first reading the
// entire sequence over the network.
//
// Revision 1.8  1996/05/16 22:49:53  jimg
// Dan's changes for version 2.0. Added a parameter to read that returns
// an error code so that EOF can be distinguished from an actual error when
// reading sequences. This *may* be replaced by an error member function
// in the future.
//
// Revision 1.7  1996/05/14 15:38:46  jimg
// These changes have already been checked in once before. However, I
// corrupted the source repository and restored it from a 5/9/96 backup
// tape. The previous version's log entry should cover the changes.
//
// Revision 1.6  1996/04/05 00:22:09  jimg
// Compiled with g++ -Wall and fixed various warnings.
//
// Revision 1.5  1996/03/05 01:09:09  jimg
// Added to the Vector dtor (now the BaseType * vector is properly deleted.
// Created the vec_resize() member function.
// Modified serialize() member function so that the ce_eval flag is used.
//
// Revision 1.4  1996/02/01 17:43:14  jimg
// Added support for lists as operands in constraint expressions.
//
// Revision 1.3  1995/12/09  01:07:33  jimg
// Added changes so that relational operators will work properly for all the
// datatypes (including Sequences). The relational ops are evaluated in
// DDS::eval_constraint() after being parsed by DDS::parse_constraint().
//
// Revision 1.2  1995/12/06  19:52:26  jimg
// Modified print_decl() so that the declaration is printed only if the
// variable is selected.
//
// Revision 1.1  1995/11/22  22:30:18  jimg
// Created.
//

