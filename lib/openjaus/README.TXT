RELEASE NOTES: OpenJAUSv3.3 (07/10/08)
==========================================

OpenJAUSv3.3 beta is an open source implementation of the JAUS Reference
Architecture Version 3.3. The latest code, documentation and support groups 
are available at the OpenJAUS web site:  http://www.openjaus.com/.

This release contains the following projects:

libjaus:		A C99 compliant library that contains JAUS constructs, data 
				types and messages.
libopenJaus:	A C/C++ library containing a runtime JAUS node manager library,
				node manager communication interface, and component framework 
				library.
ojNodeManager:	The OpenJAUS node manager executable with text interface
ojVehicleSim:	A collection of JAUS components that work together to simulate
				a JAUS ground vehicle subsystem. These components provide a set
				of services that are commonly used in real ground vehicle
				implementations.

If you experience problems in compiling or developing with this code, please
read the tutorials and online FAQ before contacting the OpenJAUS development
group. Please attempt to locate where the problem exists and determine how the
problem can be replicated before seeking additional support. Code patches can be
uploaded at the OpenJAUS web site: http://www.openjaus.com.


Target Operating Environment:
-----------------------------

	This software is designed to be cross platform compatible. The code base has 
been tested on a variety of operating systems and development environments including: 
Linux (Ubuntu & Fedora Core) w/gnu toolchain, Mac OS X w/gnu toolchain, and Windows w/MS 
Visual Studio. All test machines have been little endian byte order based. The libjaus 
library is C99 compliant and should be compatible with any standard C compiler for any 
processing platform. The libopenJaus library has some operating system dependent 
abstractions, which may require different implementations for systems not using Linux 
or Windows. The library was implemented with POSIX standard programming interfaces where 
applicable, to provide a more standardized approach to multi-threading and mutex 
implementation.


Installation (GNU Toolchain / Linux & Macintosh OS X):
------------------------------------------------------

Requirements:
	make
	gcc
	g++

To build the library and executable code:
	Execute the command: "make" in the root OpenJAUSv3.3.0 directory. The make
command will use the Makefile found in that directory to recursively make each
of the subprojects. Individual Makefiles are located in each subproject
directory to allow for building one project at a time if desired.

Installation:
	It is up to the individual developer to decide how to link their code with
the OpenJAUS libraries. Some developers may prefer to install the OpenJAUS
header files and libraries to globally accessible directories. For example, on a
Linux system: /usr/include/ and /usr/lib/. To minimize system invasiveness, 
OpenJAUS  does not install itself into system directories, rather, all code and 
binaries remain in the folders included in the release download.

	To execute code developed with these libraries it is recommended that the
developer configure the dynamic linker runtime bindings to reference the 
OpenJAUS library directories /libjaus/lib/ and /libopenJaus/lib/. This is done
by editing the systems ld.so.conf (most likely located at /etc/ld.so.conf) to
include these directories and then by running the command: "ldconfig". Another 
option to this method would be to simply include the files libjaus.so and 
libopenJaus.so in the executable's working directory.

	*Note: The default build of the libraries includes all symbols and debug 
information within the shared object (.so) files. This is to facilitate
efficient  development and use of a run-time debugger. To reduce the size of the
installed  libraries, the information can be removed by using the "strip"
command. 


Installation (Microsoft Visual Studio / Windows):
------------------------------------------------------

Requirements:
	Microsoft Visual Studio
	Pthreads-win32 (included in distribution)
	pdcurses (included in distribution)

To build the library and executable code:

In the top level directory, open the Microsoft Visual Studio solution 
file "OpenJAUSv3.3.0.sln." This solution contains 4 individual projects: 
libjaus, libopenJaus, ojNodeManager and ojVehicleSim. These projects correspond 
to the descriptions above. Once open, the solution should be able to be 
built. Each project has the appropriate dependencies setup using relative paths, 
therefore it should compile for the developer in any configuration. Also, project 
references have been setup, so a referenced code base will be compiled prior to 
the compilation of any child project.

Once built, the ojNodeManager should run in a console window if it is set
as the default project. Ensure that the nodeManager.conf file is configured correctly.
The ojVehicleSim should run as well if it is set to the default project, however it 
is important that the ojNodeManager be running first.

The libraries libjaus and libopenJaus can be linked against other projects using 
the .lib files. To run projects built against them, ensure the appropriate .dll 
files are available either locally or globally on the system.

*Note: Use of these libraries has only been tested in the Debug configuration 
to date. This is due to the added value of retaining debug information for developers 
of both the OpenJAUS code base and applications based upon that. The creation of release 
targets, if desired, is left up to the individual developer. 


Configuring and Running the Node Manager:
-----------------------------------------

	The node manager executable is currently designed to require manual
configuration. This means the user must edit the text file nodeManager.conf to
enter the appropriate JAUS identification numbers and names, and enter the 
correct IP addresses for the intended hardware communication interfaces. The
user is also required to configure the node manager to enable or disable node
level and subsystem level communication. These configurations will vary 
depending on the intended design of the JAUS system on which the node manager is
being used. Examples of different JAUS system setups are beyond the scope of 
this document. The user is encouraged to read the JAUS reference architecture
and online OpenJAUS tutorials for a more detailed explanation.

Format:
	The nodeManager.conf file is made up of section tags and key - value pairs.
The tags are identified within straight braces, ex: "[SECTION]". The key value
pairs are delimited by a colon and a space character, ex: "key: value".

	To configure the JAUS node manager properties, edit the "SubsystemId" and
"NodeId" values found under the [JAUS] section tag. Each node manager on a 
subsystem's node network should be configured with a unique NodeId number. 
Likewise each subsystem on a network of JAUS subsystems should have a unique
SubsystemId number. Failure to make these values unique results in erratic and
undefined behavior when multiple node managers are running at the same time. 
Only one node manager process should be run at any given time on a single node.
	Set the "Subsystem_Identification" and "Node_Identification" values to any
text string that you would like for your particular system setup. These values
are not required to be unique, as they are only used for text description.

	To configure communication interface settings the user is only required to
edit the [Node_Communications] and [Subsystem_Communications] sections. By 
default, component communications are always on and the configuration needs to 
be kept fixed to use the loopback IP address 127.0.0.1. 
	Each communications section has one or more sets of transport interfaces
settings, for example "JAUS_UDP" and "OpenJAUS_UDP". This allows for multiple
transport interfaces to be used at a given communications level (component,
node, or subsystem). At present only the JAUS_UDP interface type is used at the
node and component level. However, this allows for other common transport types
such as TCP or Serial to be included in the future.
	The user has the option to enable/disable a given transport interface, or
enable/disable an entire level of communication either node or subsystem. The
node manager will startup a transport interface if both it and its 
communications level is enabled. To enable a communications level, simply set
the value of the "Enabled" key to "true". To disable a level, set the value to 
"false". These values are case sensitive, ex: "TRUE" or "True" would not work.
	When each JAUS_UDP interface starts, it attempts to open the network
interface specified with the user configured IP address. If such an interface
exists on the system, the node manager will open the JAUS port 3794 on that 
interface. If not, the node manager will fail at startup and an error message
will appear. To configure each JAUS_UDP interface simply set the key 
"JAUS_UDP_IP" value to the IP address of the network interface that you would
like it to use. The remaining key values should be left configured as in the
release download. Changing these values will lead to undefined behavior. They
are included only for completeness, and allow for future changes to be made if
necessary.


Further Information:
--------------------
Tutorials: http://www.openjaus.com/tutorials.html
Forums: http://www.openjaus.com/trac/openjaus/discussion
Support contracting: 

Known Issues:
-------------

*  There is no standalone API documentation, which describes all data
   structures, functions, or default behavior. Please read the tutorials for 
   detailed information on how to perform common tasks. The header files are 
   organized by function breakdown and allow for easy access to data structure
   definitions and function prototypes. Often the functions you seek can be
   found quickly by browsing the include/ directories.

*  A "ToString" method for each JAUS message and data structure is lacking from 
   this implementation and thus makes displaying critical information to the
   user more cumbersome.


Copyright Notices
-----------------

Portions of the Windows distribution are based on pdcurses and Pthreads-win32. 
These binaries packages have their own copyright. They are redistributed with the 
OpenJAUS codebase as a convenience to developers. More information about these 
packages can be found on their respective websites below:

Pthreads-win32: http://sourceware.org/pthreads-win32/
pdcurses: http://pdcurses.sourceforge.net/

The remainder of the code is copyright OpenJAUS.com and is distributed under the 
BSD license. See the LICENSE file for details.
