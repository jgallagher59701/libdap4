

// -*- C++ -*-

// (c) COPYRIGHT URI/MIT 1997-1999
// Please first read the full copyright statement in the file COPYRIGHT.
//
// Authors:
//	jhrg,jimg	James Gallagher (jgallagher@gso.uri.edu)

#ifndef _dodsfilter_h
#define _dodsfilter_h

#ifdef __GNUG__
#pragma interface
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <string>

#ifndef _das_h
#include "DAS.h"
#endif

#ifndef _dds_h
#include "DDS.h"
#endif


/** When a DODS server receives a request from a DODS client, the
    server CGI script dispatches the request to one of several
    ``filter'' programs.  Each filter is responsible for returning a
    different aspect of the dataset information: one is for data, one
    is for the dataset DDS, one is for the dataset DAS, and a fourth
    is for a usage message describing the server itself.  Some
    installations may have additional optional filters.

    The filter program receives a data request from the dispatch
    script. It receives its operating parameters from the command
    line, like any UNIX command, and it returns its output to standard
    output, which the httpd server packages up into a reply to the
    client. 

    This class contains some common functions for the filter programs
    used to make up the DODS data servers. The filter programs do not
    {\it have} to be called by a CGI program, but that is the normal
    mechanism by which they are invoked.
  
    @memo Common functions for DODS server filter programs.
    @author jhrg 8/26/97 */

class DODSFilter {
private:
    bool comp;			// True if the output should be compressed.
    bool ver;			// True if the caller wants version info.
    bool bad_options;		// True if the options (argc,argv) are bad.
    bool d_conditional_request;

    string program_name;	// Name of the filter program
    string dataset;		// Name of the dataset/database 
    string ce;			// Constraint expression 
    string cgi_ver;		// Version of CGI script (caller)
    string anc_dir;		// Look here for ancillary files
    string anc_file;		// Use this for ancillary file name
    string cache_dir;		// Use this for cache files
    string accept_types;	// List of types the client understands.

    time_t d_anc_das_lmt;	// Last modified time of the anc. DAS.
    time_t d_anc_dds_lmt;	// Last modified time of the anc. DDS.
    time_t d_if_modified_since;	// Time from a conditional request.

    DODSFilter() {}		// Private default ctor.

public:
    /** Create an instance of DODSFilter using the command line
	arguments passed by the CGI (or other) program.  The default
	constructor is private; this and the copy constructor (which is
	just the default copy constructor) are the only way to create an
	instance of DODSFilter.

	These are the valid options:
	\begin{description}
	\item[{\it filename}]
	The name of the file on which the filter is to operate.  Usually
	this would be the file whose data has been requested.

	\item[#-c#]
	Send compressed data. Data are compressed using the deflate program.
	The W3C's libwww will recognize this and automatically decompress
	these data.

	\item[#-e# {\it expression}]
	This option specifies a non-blank constraint expression used to
	subsample a dataset.

	\item[#-v# {\it cgi-version}] Set the CGI/Server version to
	#cgi-version#. This is a way for the caller to set version
	information passed back to the client either as the response to a
	version request of in the response headers.

	\item[#-V#] Specifies that this request is just for version
	information. Servers can check to see if this was given using the
	#version# mfunc. Note that version information is sent from within
	DODSFilter so that sophisticated servers can support versioning data
	sources inaddition to the server software.

	\item[#-d# {\it ancdir}]
	Specifies that ancillary data be sought in the {\it ancdir}
	directory. 

	\item[#-f# {\it ancfile}]
	Specifies that ancillary data may be found in a file called {\it
	ancfile}.

	\item[#-r# {\it cache directory}]
	Specify a directory to use if/when files are to be cached. Not all
	handlers support caching and each uses its own rules tailored to a
	specific file or data type.

	\item[#-t# {\it list of types}] Specifies a list of types accepted by
	the client. This information is passed to a server by a client using
	the XDODS-Accept-Types header. The comma separated list contains each
	type the client can understand \emph{or}, each type the client does
	\emph{not} understand. In the latter case the type names are prefixed
	by a {\tt !}. If the list contains only the keyword `All', then the
	client is declaring that it can understand all DODS types.

	\item[#-l# {\it time}] Indicates that the request is a conditional
	request; send a complete response if and only if the data has changed
	since {\it time}. If it has not changed since {\it time}, then send a
	304 (Not Modified) response. The {\it time} parameter is the
	#Last-Modified# time from an If-Modified-Since condition GET request.
	It is given in seconds since the start of the Unix epoch (Midnight, 1
	Jan 1970). 

	\end{description}

	@memo DODSFilter constructor. */
    DODSFilter(int argc, char *argv[]);

    virtual ~DODSFilter();

    /** Use this function to test whether the options passed via argc
	and argv are valid. 

	@memo Check whether the DODSFilter was initialized with valid
	arguments. 
	@return True if the class state is OK, false otherwise. */
    bool OK();

    /** Use this function to check whether the client requested version
	information.  In addition to returning version information about
	the DODS software, the server can also provide version
	information about the dataset itself.

	@memo Should the filter send version information to the client
	program?

	@return TRUE if the -v option was given indicating that the filter
	should send version information back to the client, FALSE
	otherwise. 
	@see DODSFilter::send_version_info */
    bool version();

    /** Is this request conditional? 

	@return True if the request is conditional.
	@see get_request_if_modified_since(). */
    virtual bool is_conditional();

    /** Return the version information passed to the instance when it was
	created. This string is passed to the DODSFilter ctor using the -v
	option.

	@return The version string supplied at initialization. */
    string get_cgi_version();

    /** Set the CGI/Server version number. Servers use this when answering
	requests for version information. The vesion `number' should include
	both the name of the server (e.g., ff_dods) as well as the version
	number. Since this information is typically divined by configure,
	it's up to the executable to poke the correct value in using this
	method.

	Note that the -v switch that this class understands is deprecated
	since it is usually called by Perl code. It makes more sense to have
	the actual C++ software set the version string. 

	@param version A version string for this server. */
    void set_cgi_version(string version);

    /** Return the entire constraint expression in a string.  This
	includes both the projection and selection clauses, but not the
	question mark.

	@memo Get the constraint expression. 
	@return A string object that contains the constraint expression. */
    string get_ce();

    virtual void set_ce(string _ce);

    /** The ``dataset name'' is the filename or other string that the
	filter program will use to access the data. In some cases this
	will indicate a disk file containing the data.  In others, it
	may represent a database query or some other exotic data
	access method. 

	@memo Get the dataset name. 
	@return A string object that contains the name of the dataset. */
    string get_dataset_name();

    /** To read version information that is specific to a certain
	dataset, override this method with an implementation that does
	what you want. By default, this returns an empty string.

	@memo Get the version information for the dataset.  
	@return A string object that contains the dataset version
	information.  */ 
    virtual string get_dataset_version();

    /** Get the dataset's last modified time. This returns the time at which
	the dataset was last modified as defined by UNIX's notion of
	modification. This does not take into account the modification of an
	ancillary DAS or DDS. Time is given in seconds since the epoch (1 Jan
	1970 00:00:00 GMT).

	This method perform a simple check on the file named by the dataset
	given when the DODSFilter instance was created. If the dataset is not
	a filter, this method returns the current time. Servers which provide
	access to non-file-based data should subclass DODSFilter and supply a
	more suitable version of this method.

	From the stat(2) man page: ``Traditionally, st_mtime is changed by
	mknod(2), utime(2), and write(2). The st_mtime is not changed for
	changes in owner, group, hard link count, or mode.''
	
	@return Time of the last modification in seconds since the epoch.
	@see get_das_last_modified_time()
	@see get_dds_last_modified_time() */
    virtual time_t get_dataset_last_modified_time();

    /** Get the last modified time for the dataset's DAS. This time, given in
	seconds since the epoch (1 Jan 1970 00:00:00 GMT), is the greater of
	the datasets's and any ancillary DAS' last modified time.

	@param anc_location A directory to search for ancillary files (in
	addition to the CWD).
	@return Time of last modification of the DAS.
	@see get_dataset_last_modified_time()
	@see get_dds_last_modified_time() */
    virtual time_t get_das_last_modified_time(const string &anc_location="");

    /** Get the last modified time for the dataset's DDS. This time, given in
	seconds since the epoch (1 Jan 1970 00:00:00 GMT), is the greater of
	the datasets's and any ancillary DDS' last modified time.

	@return Time of last modification of the DDS.
	@see get_dataset_last_modified_time()
	@see get_dds_last_modified_time() */
    virtual time_t get_dds_last_modified_time(const string &anc_location="");

    /** Get the last modified time to be used for a particular data request.
	This method should look at both the contraint expression and any
	ancillary files for this dataset. The implementation provided here
	returns the latest time returned by the get_dataset...(),
	get_das...() and get_dds...() methods and does not currently check
	the CE.

	@param anc_location A directory to search for ancillary files (in
	addition to the CWD).
	@return Time of last modification of the data.
	@see get_dataset_last_modified_time()
	@see get_das_last_modified_time()
	@see get_dds_last_modified_time() */
    virtual time_t get_data_last_modified_time(const string &anc_location="");

    /** Get the value of a conditional request's If-Modified-Since header.
	This value is used to determine if the request should get a full
	response or a Not Modified (304) response. The time is given in
	seconds since the Unix epoch (midnight, 1 Jan 1970). If no time was
	given with the request, this methods returns -1.

	@return If-Modified-Since time from a condition GET request. */
    virtual time_t get_request_if_modified_since();

    /** The #cache_dir# is used to hold the cached .dds and .das files.
	By default, this returns an empty string (store cache files in
	current directory.

	@memo Get the cache directory.
	@return A string object that contains the cache file directory.  */
    virtual string get_cache_dir();

    /** Get the list of accepted datatypes sent by the client. If no list was
	sent, return the string `All'. 

	NB: The funny spelling `accept types' instead of `accepted types'
	mirrors the name of the HTTP request header field name which in turn
	mirrors the common practice of using `accept' over `accepted'.

	@see DODSFilter
	@return A string containing a list of the accepted types. */
    string get_accept_types();

    /** Read the ancillary DAS information and merge it into the input
	DAS object.

	@memo Test if ancillary data must be read.
	@param das A DAS object that will be augmented with the
	ancillary data attributes.
	@return void
	@see DAS */
    void virtual read_ancillary_das(DAS &das, string anc_location = "");

    /** Read the ancillary DDS information and merge it into the input
	DDS object. 

	@memo Test if ancillary data must be read.
	@param dds A DDS object that will be augmented with the
	ancillary data properties.
	@return void
	@see DDS */
    void virtual read_ancillary_dds(DDS &dds, string anc_location = "");

    /** This message is printed when the filter program is incorrectly
	invoked by the dispatch CGI.  This is an error in the server
	installation or the CGI implementation, so the error message is
	written to stderr instead of stdout.  A server's stderr messages
	show up in the httpd log file. In addition, an error object is
	sent back to the client program telling them that the server is
	broken. 

	@memo Print usage information for a filter program. */
    void print_usage();

    /** This function formats and sends to stdout version
	information from the httpd server, the server dispatch scripts,
	the DODS core software, and (optionally) the dataset.

	@memo Send version information back to the client program. */ 
    void send_version_info();

    /** This function formats and prints an ASCII representation of a
	DAS on stdout.  This has the effect of sending the DAS object
	back to the client program.

	@memo Transmit a DAS.
	@param das The DAS object to be sent.
	@return void
	@see DAS */
    void virtual send_das(DAS &das, const string &anc_location = "");
    void virtual send_das(ostream &os, DAS &das,
			  const string &anc_location="");

    /** This function formats and prints an ASCII representation of a
	DDS on stdout.  When called by a CGI program, this has the
	effect of sending a DDS object back to the client
	program. Either an entire DDS or a constrained DDS may be sent.

	@memo Transmit a DDS.
	@param dds The DDS to send back to a client.
	@param constrained If this argument is true, evaluate the
	current constraint expression and send the `constrained DDS'
	back to the client. 
	@return void
	@see DDS */
    void virtual send_dds(DDS &dds, bool constrained = false,
			  const string &anc_location = "");
    void virtual send_dds(ostream &os, DDS &dds, bool constrained = false,
			  const string &anc_location = "");

    /** Send the data in the DDS object back to the client
	program.  The data is encoded in XDR format, and enclosed in a
	MIME document which is all sent to stdout.  This has the effect
	of sending it back to the client.

	@memo Transmit data.
	@param dds A DDS object containing the data to be sent.
	@param data_stream A pointer to the XDR sink into which the data
	is to be put for encoding and transmission.
	@return void
	@see DDS */
    void virtual send_data(DDS &dds, FILE *data_stream,
			   const string &anc_location = "");
};

// $Log: DODSFilter.h,v $
// Revision 1.19  2001/06/15 23:49:02  jimg
// Merged with release-3-2-4.
//
// Revision 1.18.2.3  2001/06/14 21:32:04  jimg
// Added a method to set the cgi_version property without relying on the ctor.
//
// Revision 1.18.2.2  2001/05/03 19:10:35  jimg
// Added the d_conditional_request and d_if_modified_since fields. These are
// used to indicate that the request from the client is a conditional GET
// request. DODS currently only supports conditional requests based on the Last
// Modified time included by a server in a response. The ctor takes a -l switch
// which expects the time given with the If-Modified-Since header. This provides
// an easy way for servers to handle the conditional request since they can
// simply pass the switches and parameters they receive directly to DODSFilter
// (as if they were opaque objects).
//
// Revision 1.18.2.1  2001/04/23 22:34:46  jimg
// Added support for the Last-Modified MIME header in server responses.`
//
// Revision 1.18  2000/10/30 17:21:27  jimg
// Added support for proxy servers (from cjm).
//
// Revision 1.17  2000/09/22 02:17:19  jimg
// Rearranged source files so that the CVS logs appear at the end rather than
// the start. Also made the ifdef guard symbols use the same naming scheme and
// wrapped headers included in other headers in those guard symbols (to cut
// down on extraneous file processing - See Lakos).
//
// Revision 1.16  2000/09/21 16:22:07  jimg
// Merged changes from Jose Garcia that add exceptions to the software.
// Many methods that returned error codes now throw exectptions. There are
// two classes which are thrown by the software, Error and InternalErr.
// InternalErr is used to report errors within the library or errors using
// the library. Error is used to reprot all other errors. Since InternalErr
// is a subclass of Error, programs need only to catch Error.
//
// Revision 1.15  2000/07/09 21:57:09  rmorris
// Mods's to increase portability, minimuze ifdef's in win32 and account
// for differences between the Standard C++ Library - most notably, the
// iostream's.
//
// Revision 1.14  2000/06/07 19:33:21  jimg
// Merged with verson 3.1.6
//
// Revision 1.13  2000/06/07 18:06:58  jimg
// Merged the pc port branch
//
// Revision 1.12.10.1  2000/06/02 18:16:48  rmorris
// Mod's for port to Win32.
//
// Revision 1.11.2.3  2000/05/18 20:45:27  jimg
// added set_ce(). Maybe add more set methods?
//
// Revision 1.12.4.1  2000/02/07 21:11:36  jgarcia
// modified prototypes and implementations to use exceeption handling
//
// Revision 1.11.2.2  1999/09/08 22:36:03  jimg
// Fixed the -V comment.
//
// Revision 1.12  1999/09/03 22:07:44  jimg
// Merged changes from release-3-1-1
//
// Revision 1.11.2.1  1999/08/28 06:43:04  jimg
// Fixed the implementation/interface pragmas and misc comments
//
// Revision 1.11  1999/05/25 21:57:12  dan
// Added an optional second argument to read_ancillary_dds to support JGOFS
// usage.
//
// Revision 1.10  1999/05/25 21:54:50  dan
// Added an optional second argument to read_ancillary_das to support
// JGOFS usage.
//
// Revision 1.9  1999/05/05 00:48:07  jimg
// Added the get_cgi_version() member function.
// Added documentation about get_cgi_version() and the -V option (new).
//
// Revision 1.8  1999/05/04 19:47:21  jimg
// Fixed copyright statements. Removed more of the GNU classes.
//
// Revision 1.7  1999/04/29 02:29:28  jimg
// Merge of no-gnu branch
//
// Revision 1.6  1999/02/22 22:58:02  jimg
// Added the get_accept_types() accessor. Also added to the ctor so that the -t
// option will be parsed properly and used to set the value of accept_types.
//
// Revision 1.5  1999/01/21 20:42:01  tom
// Fixed comment formatting problems for doc++
//
// Revision 1.4.2.1  1999/02/02 21:56:57  jimg
// String to string version
//
// Revision 1.4  1998/08/06 16:11:47  jimg
// Added cache_dir member (from jeh).
//
// Revision 1.3  1998/02/04 14:55:32  tom
// Another draft of documentation.
//
// Revision 1.2  1997/09/22 23:04:59  jimg
// Added doc++ style comments.
//
// Revision 1.1  1997/08/28 20:39:02  jimg
// Created
//

#endif // _dodsfilter_h
