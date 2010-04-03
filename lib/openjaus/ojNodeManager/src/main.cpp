/*****************************************************************************
 *  Copyright (c) 2008, University of Florida
 *  All rights reserved.
 *  
 *  This file is part of OpenJAUS.  OpenJAUS is distributed under the BSD 
 *  license.  See the LICENSE file for details.
 * 
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions 
 *  are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of the University of Florida nor the names of its 
 *       contributors may be used to endorse or promote products derived from 
 *       this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ****************************************************************************/
// File Name: ojNodeManager.cpp
//
// Written By: Danny Kent (jaus AT dannykent DOT com)
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: 

#include "openJaus.h"

#if defined(WIN32)
	#include <windows.h>
	#define CLEAR_COMMAND "cls"
#elif defined(__linux) || defined(linux) || defined(__linux__) || defined(__APPLE__)
	#include <cstdlib>
	#include <unistd.h>
	#include <termios.h>
	#define CLEAR_COMMAND "clear"
#endif

void printHelpMenu();
void parseUserInput(char input);

static NodeManager *nm;

class MyHandler : public EventHandler
{
public:
	~MyHandler()
	{

	}
	
	void handleEvent(NodeManagerEvent *e)
	{
		SystemTreeEvent *treeEvent;
		ErrorEvent *errorEvent;
		JausMessageEvent *messageEvent;
		DebugEvent *debugEvent;
		ConfigurationEvent *configEvent;

		switch(e->getType())
		{
			case NodeManagerEvent::SystemTreeEvent:
				treeEvent = (SystemTreeEvent *)e;
				printf("%s\n", treeEvent->toString().c_str());
				delete e;
				break;

			case NodeManagerEvent::ErrorEvent:
				errorEvent = (ErrorEvent *)e;
				printf("%s\n", errorEvent->toString().c_str());
				delete e;
				break;

			case NodeManagerEvent::JausMessageEvent:
				messageEvent = (JausMessageEvent *)e;
				// If you turn this on, the system gets spam-y this is very useful for debug purposes
				if(messageEvent->getJausMessage()->commandCode != JAUS_REPORT_HEARTBEAT_PULSE)
				{
					//printf("%s\n", messageEvent->toString().c_str());
				}
				else
				{
					//printf("%s\n", messageEvent->toString().c_str());
				}
				delete e;
				break;

			case NodeManagerEvent::DebugEvent:
				debugEvent = (DebugEvent *)e;
				//printf("%s\n", debugEvent->toString().c_str());
				delete e;
				break;

			case NodeManagerEvent::ConfigurationEvent:
				configEvent = (ConfigurationEvent *)e;
				printf("%s\n", configEvent->toString().c_str());
				delete e;
				break;

			default:
				delete e;
				break;
		}
	}
};

#if defined(WIN32)
int main(int argc, char **args)
{
	// Console parameters
	HANDLE handleStdin;
    INPUT_RECORD inputEvents[128];
	DWORD eventCount;

	// Control parameter
	bool running = true;
	int i = 0;

	printf("\nOpenJAUS Node Manager Version %s (August 7, 2008)\n\n", OJ_NODE_MANAGER_VERSION); 

	// Setup the console window's input handle
	handleStdin = GetStdHandle(STD_INPUT_HANDLE); 

	MyHandler *handler = new MyHandler();
	FileLoader *configData = new FileLoader("nodeManager.conf");

	try
	{
		nm = new NodeManager(configData, handler);
		printHelpMenu();
	}
	catch(char *exceptionString)
	{
		printf("%s\n", exceptionString);
		printf("Terminating Program...\n");
		running = false;
	}
	catch(...)
	{
		printf("Node Manager Construction Failed. Terminating Program...\n");
		running = false;
	}

	while(running)
	{
		// See how many events are waiting for us, this prevents blocking if none
		GetNumberOfConsoleInputEvents(handleStdin, &eventCount);
		
		if(eventCount > 0)
		{
			// Check for user input here
			ReadConsoleInput( 
					handleStdin,		// input buffer handle 
					inputEvents,		// buffer to read into 
					128,				// size of read buffer 
					&eventCount);		// number of records read 
		}
 
	    // Parse console input events 
        for (i = 0; i < (int) eventCount; i++) 
        {
            switch(inputEvents[i].EventType) 
            { 
				case KEY_EVENT: // keyboard input 
					if(inputEvents[i].Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)
					{
						running = false;
					}
					else if(inputEvents[i].Event.KeyEvent.bKeyDown)
					{
						parseUserInput(inputEvents[i].Event.KeyEvent.uChar.AsciiChar);
					}
					break;
				
				default:
					break;
			}
		}

		Sleep((DWORD)(0.2*1e3));
	}

	delete nm;
	delete configData;
	delete handler;

	system("pause");

	return 0;
}

#elif defined(__linux) || defined(linux) || defined(__linux__) || defined(__APPLE__)

int main(int argc, char **args)
{
	struct termios newTermio;
	struct termios storedTermio;
	bool running = true;
	char choice[8] = {0};
	int count = 0;
	
	tcgetattr(0,&storedTermio);
	memcpy(&newTermio,&storedTermio,sizeof(struct termios));
	
	// Disable canonical mode, and set buffer size to 0 byte(s)
	newTermio.c_lflag &= (~ICANON);
	newTermio.c_lflag &= (~ECHO);
	newTermio.c_cc[VTIME] = 0;
	newTermio.c_cc[VMIN] = 1;
	tcsetattr(0,TCSANOW,&newTermio);

	printf("\nOpenJAUS Node Manager Version %s (JULY 9, 2008)\n\n", OJ_NODE_MANAGER_VERSION); 

	FileLoader *configData = new FileLoader("nodeManager.conf");
	MyHandler *handler = new MyHandler();

	try
	{
		nm = new NodeManager(configData, handler);
		printHelpMenu();
	}
	catch(char *exceptionString)
	{
		printf("%s", exceptionString);
		printf("Terminating Program...\n");
		running = false;
	}
	catch(...)
	{
		printf("Node Manager Construction Failed. Terminating Program...\n");
		running = false;
	}

	while(running)
	{
		bzero(choice, 8);
		count = read(0, &choice, 8);
		if(count == 1 && choice[0] == 27) // ESC
		{
			running = false;
		}
		else if(count == 1)
		{
			parseUserInput(choice[0]);
		}
	}

	delete nm;
	delete handler;
	delete configData;
	
	tcsetattr(0, TCSANOW, &storedTermio);
	return 0;
}
#endif

void parseUserInput(char input)
{
	switch(input)
	{
		case 'T':
			printf("\n\n%s", nm->systemTreeToDetailedString().c_str());
			break;

		case 't':
			printf("\n\n%s", nm->systemTreeToString().c_str());
			break;
		
		case 'c':
		case 'C':
			system(CLEAR_COMMAND);
			break;

		case '?':
			printHelpMenu();
	
		default:
			break;
	}

}

void printHelpMenu()
{
	printf("\n\nOpenJAUS Node Manager Help\n");
	printf("   t - Print System Tree\n");
	printf("   T - Print Detailed System Tree\n");
	printf("   c - Clear console window\n");
	printf("   ? - This Help Menu\n");
	printf(" ESC - Exit Node Manager\n");
}
