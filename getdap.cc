
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
 
// (c) COPYRIGHT URI/MIT 1997-1999
// Please read the full copyright statement in the file COPYRIGHT_URI.
//
// Authors:
//	jhrg,jimg	James Gallagher <jgallagher@gso.uri.edu>

// This is the source to `geturl'; a simple tool to exercise the Connect
// class. It can be used to get naked URLs as well as the DODS DAS and DDS
// objects.  jhrg.

#include "config_dap.h"

static char rcsid[] not_used = {"$Id: getdap.cc,v 1.65 2003/02/27 23:41:49 jimg Exp $"};

#include <stdio.h>
#include <assert.h>
#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#endif

#include <GetOpt.h>
#include <string>

#include "AISConnect.h"

#ifdef WIN32
#define MAIN_RETURN void
#else
#define MAIN_RETURN int
#endif

using std::cerr;
using std::endl;

const char *version = "$Revision: 1.65 $";

extern int keep_temps;		// defined in Connect.cc

void
usage(string name)
{
    fprintf( stderr, "Usage: %s", name.c_str() ) ;
    fprintf( stderr, " [dDaVvk] [-c <expr>] [-m <num>] <url> [<url> ...]\n" ) ;
    fprintf( stderr, "[Vvk] <file> [<file> ...]\n" ) ;
    fprintf( stderr, "\n" ) ;
    fprintf( stderr,
	     "In the first form of the command, dereference the URL and\n" ) ;
    fprintf( stderr,
	     "perform the requested operations. This include routing\n" ) ;
    fprintf( stderr,
	     "the returned information through the DAP processing\n" ) ;
    fprintf( stderr,
	     "library (parsing the returned objects, et c.). If none\n" ) ;
    fprintf( stderr,
	     "of a, d, or D are used with a URL, then the DAP library\n" ) ;
    fprintf( stderr,
	     "routines are NOT used and the URLs contents are dumped\n" ) ;
    fprintf( stderr,
	     "to standard output.\n" ) ;
    fprintf( stderr,
	     "In the second form of the command, assume the files are\n" ) ;
    fprintf( stderr,
	     "DODS data objects (stored in files or read from pipes)\n" ) ;
    fprintf( stderr,
	     "and process them as if -D were given. In this case the\n" ) ;
    fprintf( stderr,
	     "information *must* contain valid MIME header in order\n" ) ;
    fprintf( stderr, "to be processed." ) ;
    fprintf( stderr, "\n" ) ;
    fprintf( stderr, "Options:\n" ) ;
    fprintf( stderr, "        d: For each URL, get the DODS DDS.\n" ) ;
    fprintf( stderr, "        a: For each URL, get the DODS DAS.\n" ) ;
    fprintf( stderr, "        D: For each URL, get the DODS Data.\n" ) ;
    fprintf( stderr, "        v: Verbose.\n" ) ;
    fprintf( stderr, "        V: Version.\n" ) ;
    fprintf( stderr, "        c: <expr> is a contraint expression. Used with -D.\n" ) ;
    fprintf( stderr, "           NB: You can use a `?' for the CE also.\n" ) ;
    fprintf( stderr, "        k: Keep temporary files created by DODS core\n" );
    fprintf( stderr, "        m: Request the same URL <num> times.\n" ) ;
    fprintf( stderr, "        z: Don't ask the server to compress data.\n" ) ;
    fprintf( stderr, "        s: Print Sequences using numbered rows.\n" ) ;
}

bool
read_data(FILE *fp)
{
    if (!fp) {
	fprintf( stderr, "geturl: Whoa!!! Null stream pointer.\n" ) ;
	return false;
    }

    // Changed from a loop that used getc() to one that uses fread(). getc()
    // worked fine for transfers of text information, but *not* for binary
    // transfers. fread() will handle both.
    char c;
    while (fp && !feof(fp) && fread(&c, 1, 1, fp))
	printf("%c", c);	// stick with stdio 

    return true;
}

static void
process_data(Connect *url, DDS &dds, bool verbose = false, 
	     bool print_rows = false)
{
    switch (url->type()) {
      case dods_error:
	throw url->error();
	return;

      case web_error:
	// Web errors (those reported in the return document's MIME header)
	// are processed by the WWW library.
	return;

      case dods_data: 
      default: {
	  if (verbose)
	      fprintf( stderr, "Server version: %s\n",
			       url->server_version().c_str() ) ;

	  fprintf( stdout, "The data:\n" ) ;

	  for (DDS::Vars_iter qiter = dds.var_begin();
	       qiter != dds.var_end(); qiter++)
	  {
	      BaseType *v = (*qiter) ;
	      if (print_rows && v->type() == dods_sequence_c)
		  dynamic_cast<Sequence*>(v)->print_val_by_rows(stdout);
	      else
		  v->print_val(stdout);
	  }
      }

      fprintf( stdout, "\n" ) ;
      fflush( stdout ) ;
    }
}

MAIN_RETURN
main(int argc, char * argv[])
{
    GetOpt getopt (argc, argv, "daDAVvkc:m:zsh?");
    int option_char;

    bool get_das = false;
    bool get_dds = false;
    bool get_data = false;
    bool cexpr = false;
    bool verbose = false;
    bool multi = false;
    bool accept_deflate = true;
    bool print_rows = false;
    bool use_ais = false;
    int times = 1;
    string expr = "";
#if 0
    char *expr = "";  // can't use NULL or C++ string conversion will crash
#endif

#ifdef WIN32
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    while ((option_char = getopt()) != EOF)
	switch (option_char) {
	  case 'd': get_dds = true; break;
	  case 'a': get_das = true; break;
	  case 'D': get_data = true; break;
	  case 'A': use_ais = true; break;
	  case 'V': fprintf( stderr, "geturl version: %s\n", version) ; exit(0);
	  case 'v': verbose = true; break;
	  case 'k': keep_temps =1; break; // keep_temp is in Connect.cc
	  case 'c':
	    cexpr = true; expr = getopt.optarg; break;
	  case 'm': multi = true; times = atoi(getopt.optarg); break;
	  case 'z': accept_deflate = false; break;
	  case 's': print_rows = true; break;
	  case 'h':
	  case '?':
	  default:
	    usage(argv[0]); exit(1); break;
	}

    try {
	// If after processing all the command line options there is nothing
	// left (no URL or file) assume that we should read from stdin.
	for (int i = getopt.optind; i < argc; ++i) {
	    if (verbose)
		fprintf( stderr, "Fetching: %s\n", argv[i] ) ;
	
	    string name = argv[i];
	    Connect *url;
	    if (use_ais)
		url = new AISConnect(name);
	    else
		url = new Connect(name);

	    url->set_accept_deflate(accept_deflate);

	    if (url->is_local()) {
		if (verbose) {
		    fprintf(stderr,
			    "Assuming that the argument %s is a file\n",
			    argv[i]) ;

		    fprintf(stderr,
			    "that contains a DODS data object; decoding.\n" );
		}

		FILE *source;
		if (strcmp(argv[i], "-") == 0)
		    source = stdin;
		else
		    source = fopen(argv[i], "r");
	    
		if (!source) {
		    fprintf( stderr,
			     "The input source: %s could not be opened",
			     argv[i] ) ;
		    break;
		}

		// NB: local access should never use the popup gui.
		try {
		    DataDDS dds;
		    url->read_data(dds, source);
		    process_data(url, dds, verbose, print_rows);
		}
		catch (Error &e) {
		    e.display_message();
		    break;
		}
		if (source != stdin)
		    fclose(source);
	    }

	    else if (get_das) {
		for (int j = 0; j < times; ++j) {
		    DAS das;
		    try {
			url->request_das(das);
		    }
		    catch (Error &e) {
			e.display_message();
			continue;
		    }
		    if (verbose) {
			fprintf( stderr, "Server version: %s\n",
					 url->server_version().c_str() ) ; 
			fprintf( stderr, "DAS:\n" ) ;
		    }
		    das.print(stdout);
		}
	    }

	    else if (get_dds) {
		for (int j = 0; j < times; ++j) {
		    DDS dds;
		    try {
			url->request_dds(dds);
		    }
		    catch (Error &e) {
			e.display_message();
			continue;	// Goto the next URL or exit the loop.
		    }
		    if (verbose) {
			fprintf( stderr, "Server version: %s\n",
					 url->server_version().c_str() ) ; 
			fprintf( stderr, "DDS:\n" ) ;
		    }
		    dds.print(stdout);
		}
	    }

	    else if (get_data) {
		if (!(expr == "" || name.find('?') != string::npos)) {
		    fprintf( stderr,
			 "Must supply a constraint expression with -D.\n" ) ;
		    continue;
		}
		for (int j = 0; j < times; ++j) {
		    DataDDS dds;
		    try {
			url->request_data(dds, expr);
			process_data(url, dds, verbose, print_rows);
		    }
		    catch (Error &e) {
			e.display_message();
			continue;
		    }
		}
	    }

	    else { // if (!get_das && !get_dds && !get_data)
		HTTPConnect http(RCReader::instance());
		string url_string = argv[i];
		for (int j = 0; j < times; ++j) {
		    try {
			FILE *fp = http.fetch_url(url_string);
			if (verbose)
			    fprintf( stderr, "Server version: %s\n",
				     http.server_version().c_str() ) ; 
			if (!read_data(fp))
			    continue;
			fclose(fp);
		    }
		    catch (Error &e) {
			e.display_message();
			continue;
		    }
		}
	    }	    
	}
    }
    catch (Error &e) {
	e.display_message();
    }

    return 0;

#if 0
    exit(0); //  Running DejaGun/Cygwin based test suites require this.
#ifdef WIN32
    return;  //  Visual C++ asks for this.
#endif
#endif
}

// $Log: getdap.cc,v $
// Revision 1.65  2003/02/27 23:41:49  jimg
// Updated this to use the new, non-deprecated, methods in Connect (and
// AISConnect!). The code is a bit tighter. Added a new option, -A, which tells
// geturl to use the AIS when resolving a URL. In its current incarnation (early
// prototype) this just handles DAS objects and only uses a single AIS database
// found by looking for the AIS_DATABASE entry in the ~/.dodsrc configuration
// file.
//
// Revision 1.64  2003/02/21 00:14:25  jimg
// Repaired copyright.
//
// Revision 1.63  2003/01/23 00:22:25  jimg
// Updated the copyright notice; this implementation of the DAP is
// copyrighted by OPeNDAP, Inc.
//
// Revision 1.62  2003/01/10 19:46:41  jimg
// Merged with code tagged release-3-2-10 on the release-3-2 branch. In many
// cases files were added on that branch (so they appear on the trunk for
// the first time).
//
// Revision 1.52.2.22  2002/12/24 00:26:10  jimg
// Modified to use the current interface of Connect; removed calls to deprecated
// methods.
//
// Revision 1.52.2.21  2002/12/18 23:27:01  pwest
// verbose message was being printed when not in verbose mode.
//
// Revision 1.52.2.20  2002/12/17 22:35:03  pwest
// Added and updated methods using stdio. Deprecated methods using iostream.
//
// Revision 1.52.2.19  2002/10/28 21:17:44  pwest
// Converted all return values and method parameters to use non-const iterator.
// Added operator== and operator!= methods to IteratorAdapter to handle Pix
// problems.
//
// Revision 1.52.2.18  2002/09/05 22:52:55  pwest
// Replaced the GNU data structures SLList and DLList with the STL container
// class vector<>. To maintain use of Pix, changed the Pix.h header file to
// redefine Pix to be an IteratorAdapter. Usage remains the same and all code
// outside of the DAP should compile and link with no problems. Added methods
// to the different classes where Pix is used to include methods to use STL
// iterators. Replaced the use of Pix within the DAP to use iterators instead.
// Updated comments for documentation, updated the test suites, and added some
// unit tests. Updated the Makefile to remove GNU/SLList and GNU/DLList.
//
// Revision 1.52.2.17  2002/08/22 21:23:23  jimg
// Fixes for the Win32 Build made at ESRI by Vlad Plenchoy and myslef.
//
// Revision 1.52.2.16  2002/07/06 19:17:28  jimg
// I fixed the `read from stdin/file' behavior. This was broken when I switched
// from libwww to libcurl->
//
// Revision 1.52.2.15  2002/06/20 03:18:48  jimg
// Fixes and modifications to the Connect and HTTPConnect classes. Neither
// of these two classes is complete, but they should compile and their
// basic functions should work.
//
// Revision 1.52.2.14  2002/06/18 22:59:06  jimg
// As part of the switch to libcurl, I had to edit this file. The raw access to
// HTTP now uses an instnace of HTTPConnect, not Connect.
//
// Revision 1.61  2002/06/03 22:21:16  jimg
// Merged with release-3-2-9
//
// Revision 1.61  2002/06/03 22:21:16  jimg
// Merged with release-3-2-9
// Revision 1.52.2.13  2002/05/28 22:32:32  jimg
// This client no longer links with the Test* classes. The DAP implementation
// was changed so that it no longer requires clients to subclass the data type
// classes.
//
// Revision 1.52.2.12  2002/05/27 01:08:30  jimg
// Removed the last vestiges of the progress indicator code in this simple
// client.
//
// Revision 1.52.2.11  2002/02/04 19:04:06  jimg
// Fixed bad use of delete (shouldhave been delete[])
//
// Revision 1.52.2.10  2001/10/30 06:55:45  rmorris
// Win32 porting changes.  Brings core win32 port up-to-date.
//
// Revision 1.60  2001/10/29 21:17:06  jimg
// Fixed bad job of resolving conflict(s) from the last merge.
//
// Revision 1.59  2001/10/14 01:28:38  jimg
// Merged with release-3-2-8.
//
// Revision 1.52.2.9  2001/10/08 16:53:35  jimg
// Added url.gui() to all calls to Error::display_message(). This prevents a
// seg fault when display_message is called with nothing (gui == null) but
// has been ompiled with GUI defined.
//
// Revision 1.58  2001/09/28 17:50:07  jimg
// Merged with 3.2.7.
//
// Revision 1.52.2.8  2001/09/07 00:38:35  jimg
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
// Revision 1.57  2001/08/24 17:46:22  jimg
// Resolved conflicts from the merge of release 3.2.6
//
// Revision 1.52.2.7  2001/07/28 01:10:42  jimg
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
// Revision 1.52.2.6  2001/07/11 05:31:03  jimg
// If geturl's ``read from file or stdin'' feature is used and the filename or
// redirect is null, an error message is printed. Before geturl dumped core.
//
// Revision 1.56  2001/06/15 23:49:04  jimg
// Merged with release-3-2-4.
//
// Revision 1.52.2.5  2001/06/07 20:43:39  jimg
// Fixed a bug in process_data() when reading structures that contain
// sequences.
//
// Revision 1.52.2.4  2001/05/23 16:56:09  jimg
// Now reads data responses from stdin and files. The responses *must* have
// MIME headers.
// Correctly handles data responses read locally that are actually Error
// objects.
//
// Revision 1.52.2.3  2001/05/04 00:13:33  jimg
// Fixed a bug in where one of Connect's FILE *s was closed w/o using
// Connect::close_output(). This can cause clients to crash later on when
// Connect tries to close the FILE * itself.
//
// Revision 1.52.2.2  2001/02/14 00:10:05  jimg
// Merged code from the trunk's HEAD revision for this/these files onto
// the release-3-2 branch. This moves the authentication software onto the
// release-3-2 branch so that it will be easier to get it in the 3.2 release.
//
// Revision 1.54  2001/02/09 22:07:18  jimg
// Removed some of the #ifdef WIN32 guards since the code works with g++ too.
// Fixed an error in the usage string.
//
// Revision 1.53  2001/01/26 19:48:09  jimg
// Merged with release-3-2-3.
//
// Revision 1.52.2.1  2000/12/05 00:32:58  jimg
// Fixed indentation in places.
//
// Revision 1.52  2000/10/30 17:21:28  jimg
// Added support for proxy servers (from cjm).
//
// Revision 1.51  2000/09/22 02:17:23  jimg
// Rearranged source files so that the CVS logs appear at the end rather than
// the start. Also made the ifdef guard symbols use the same naming scheme and
// wrapped headers included in other headers in those guard symbols (to cut
// down on extraneous file processing - See Lakos).
//
// Revision 1.50  2000/09/11 16:39:23  jimg
// Changed the name of Sequence::get_row_numbers() ...
//
// Revision 1.49  2000/08/31 00:28:02  rmorris
// Fixed lost mod for port to win32 - newlines in string constanct
// not ok with VC++.  (this was in usage()).
//
// Revision 1.48  2000/08/16 00:39:17  jimg
// Added an option (-s) to test the Sequence::getRowNumber method.
//
// Revision 1.47  2000/08/08 20:46:17  rmorris
// Trivial removal of a hack fragment left behind from a earlier fix.
//
// Revision 1.46  2000/08/07 21:41:47  rmorris
// Reversed hack related to 7/19/00 and 7/21/00 changes to get around
// a bug in the core (which has been fixed).
//
// Revision 1.45  2000/07/21 14:29:17  rmorris
// Put a temporary fix in that is in lieu of an upcoming small change
// to the core.  Is in an ifdef with a note to remove it - but the
// temporary hack will work with or without the change to the core.
// See TMPHACK ifdef's to remove.
//
// Revision 1.44  2000/07/19 22:45:21  rmorris
// Delete the right type of dds (a DataDDS) so that when it
// is deleted, the destructors are called in the right sequence.
// This might be the case under win32 only (??), but I would think
// this change is applicable to unix also.
//
// Revision 1.43  2000/07/18 03:43:42  rmorris
// Change to set stdout to binary mode under win32 so that the output of geturl
// yields the exact same thing as it does Unix - i.e., no cr-nl translation
// of nl's within the output.
//
// Revision 1.42  2000/07/09 22:05:36  rmorris
// Changes to increase portability, minimize ifdef's for win32 and account
// for differences in the iostreams implementations.
//
// Revision 1.41  2000/06/07 18:07:01  jimg
// Merged the pc port branch
//
// Revision 1.40.10.1  2000/06/02 18:39:03  rmorris
// Mod's for port to win32.
//
// Revision 1.40  1999/08/23 18:57:46  jimg
// Merged changes from release 3.1.0
//
// Revision 1.39.2.1  1999/08/09 22:57:51  jimg
// Removed GUI code; reactivate by defining GUI
//
// Revision 1.39  1999/08/09 18:27:35  jimg
// Merged changes from Brent for the Gui code (progress indicator)
//
// Revision 1.38  1999/07/22 17:11:52  jimg
// Merged changes from the release-3-0-2 branch
//
// Revision 1.37.4.2  1999/06/11 10:21:50  rmorris
// Removed a couple of debugging lines accidentally returned to cvs
//
// Revision 1.37.4.1  1999/06/04 22:34:36  rmorris
// Fixed a line that was causing core dumps during the generation and running
// of the hdf server test suites.  It looks like there was a simple cut-and-paste
// error made by someone earlier involving the outputting of Structures - they
// were being treated like sequences.
//
// Revision 1.37  1999/05/26 17:35:01  jimg
// The Gui option is now disabled; gui is always false.
// One call to gui()->... has been removed with #if 0 ... #endif.
//
// Revision 1.36  1999/04/29 02:29:36  jimg
// Merge of no-gnu branch
//
// Revision 1.35  1999/04/22 22:30:52  jimg
// Uses dynamic_cast
//
// Revision 1.34  1999/02/22 22:45:10  jimg
// Added -T option: Use this to send a list of accepted types to the server
// using the new XDODS-Accept-Types request header. The list must not contain
// spaces *even* if it is quoted.
//
// Revision 1.33  1998/11/10 00:47:53  jimg
// Delete the dds object (created with new) after calling process_data().
//
// Revision 1.32  1998/09/08 22:23:51  jimg
// Removed PERF macro calls.
//
// Revision 1.31.6.2  1999/02/05 09:32:36  jimg
// Fixed __unused__ so that it not longer clashes with Red Hat 5.2 inlined
// math code. 
//
// Revision 1.31.6.1  1999/02/02 21:57:08  jimg
// String to string version
//
// Revision 1.31  1998/04/03 17:46:04  jimg
// Patch from Jake Hamby; fixed bug where Structures which contained sequences
// did not print properly.
//
// Revision 1.30  1998/03/20 00:23:25  jimg
// Improved the error message reporting.
//
// Revision 1.29  1998/02/19 19:45:00  jimg
// Changed error messages so that ones from geturl that are not obviously from
// geturl are prefixed by `geturl: '.
//
// Revision 1.28  1998/02/11 22:34:23  jimg
// Added support for on-the-fly decompression.
// Added -z flag to suppress requesting data be compressed.
// Added new trace options (libwww core, etc.).
//
// Revision 1.27  1998/02/05 20:14:04  jimg
// DODS now compiles with gcc 2.8.x
//
// Revision 1.26  1997/10/09 22:19:32  jimg
// Resolved conflicts in merge of 2.14c to trunk.
//
// Revision 1.25  1997/10/04 00:33:06  jimg
// Release 2.14c fixes
//
// Revision 1.24  1997/09/22 22:26:34  jimg
// Rewrote the handling of received Sequences in `constrained-trans()'.
//
// Revision 1.23.6.1  1997/09/23 15:44:33  jimg
// Now prints newline at end of data output
//
// Revision 1.23  1997/06/05 23:08:51  jimg
// Modified so that data can be read from stdin (similar to writeval).
//
// Revision 1.22  1997/03/05 08:29:32  jimg
// Silly bug where show_gui() was not called in some cases.
//
// Revision 1.21  1997/02/17 20:28:15  jimg
// Changed output of verbose option so it is consistent for DAS, DDS and data
//
// Revision 1.20  1997/02/13 17:30:17  jimg
// Added printout of the DODS version number stamp (read from the MIME header).
// Use the verbose option to enable display.
//
// Revision 1.19  1997/02/13 00:21:28  jimg
// Added version switch. Made compatible with writeval's command line options.
//
// Revision 1.18  1997/02/12 21:45:03  jimg
// Added use of the optional parameter to Connect's ctor; if -t (trace) is
// given on the command line, then print www library informational messages.
// Changed trace option switch from -v (which is now used for verbose - an
// option that is entirely separate from trace) to -t.
//
// Revision 1.17  1997/02/10 02:36:10  jimg
// Modified usage of request_data() to match new return type.
//
// Revision 1.16  1996/12/18 18:43:32  jimg
// Tried to fix the online help - maybe I succeeded?
//
// Revision 1.15  1996/11/27 22:12:33  jimg
// Expanded help to include all the verbose options.
// Added PERF macros to code around request_data() call.
//
// Revision 1.14  1996/11/20 00:37:47  jimg
// Modified -D option to correctly process the Asynchronous option.
//
// Revision 1.13  1996/11/16 00:20:47  jimg
// Fixed a bug where multiple urls failed.
//
// Revision 1.12  1996/10/31 22:24:05  jimg
// Fix the help message so that -D is not described as -A...
//
// Revision 1.11  1996/10/08 17:13:09  jimg
// Added test for the -D option so that a Ce can be supplied either with the
// -c option or using a ? at the end of the URL.
//
// Revision 1.10  1996/09/19 16:53:35  jimg
// Added code to process sequences correctly when given the -D (get data)
// option.
//
// Revision 1.9  1996/09/05 16:28:06  jimg
// Added a new option to geturl, -D, which uses the Connect::request_data
// member function to get data. Thus -a, -d and -D can be used to get the
// das, dds and data from a DODS server. Because the request_data member
// function requires a constraint expression I also added the option -c; this
// option takes as a single argument a constraint expression and passes it to
// the request_data member function. You must use -c <expr> with -D.
//
// Revision 1.8  1996/08/13 20:13:36  jimg
// Added not_used to definition of char rcsid[].
// Added code so that local `URLs' are skipped.
//
// Revision 1.7  1996/06/22 00:11:20  jimg
// Modified to accomodate the new Gui class.
//
// Revision 1.6  1996/06/18 23:55:33  jimg
// Added support for the GUI progress indicator.
//
// Revision 1.5  1996/06/08 00:18:38  jimg
// Initialized vcode to null.
//
// Revision 1.4  1996/05/29 22:05:57  jimg
// Fixed up the copyright header.
//
// Revision 1.3  1996/05/28 17:35:40  jimg
// Fixed verbose arguments.
//
// Revision 1.2  1996/05/28 15:49:55  jimg
// Removed code that read from the stream after the das/dds parser built the
// internal object (since there was nothing in that stream after the parse
// geturl would always crash).
//
// Revision 1.1  1996/05/22 23:34:21  jimg
// First version. Built to test the new WWW code in the class Connect.
//

