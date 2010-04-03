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
// File Name: SystemTree.cpp
//
// Written By: Danny Kent (jaus AT dannykent DOT com) and Tom Galluzzo (galluzzo AT gmail DOT com)
//
// Version: 3.3.0a
//
// Date: 08/07/08
//
// Description: This file defines the SystemTree class which manages the list of 
//				JAUS subsystems and their associated nodes and components.

#include <cstdlib>
#include "nodeManager/SystemTree.h"
#include "utils/timeLib.h"
#include "nodeManager/events/SystemTreeEvent.h"

SystemTree::SystemTree(FileLoader *configData, EventHandler *handler)
{
	this->configData = configData;
	this->registerEventHandler(handler);

	mySubsystemId = configData->GetConfigDataInt("JAUS", "SubsystemId");
	myNodeId = configData->GetConfigDataInt("JAUS", "NodeId");

	memset(system, 0, sizeof(system));
	subsystemCount = 0;
}

SystemTree::~SystemTree(void)
{
	for(int i = JAUS_MINIMUM_SUBSYSTEM_ID; i < JAUS_MAXIMUM_SUBSYSTEM_ID; i++)
	{
		if(system[i])
		{
			jausSubsystemDestroy(system[i]);
		}
	}
	// TODO: Delete all the memory I own (system)
}

bool SystemTree::updateComponentTimestamp(JausAddress address)
{
	JausComponent cmpt = findComponent(address);
	if(cmpt)
	{
		jausComponentUpdateTimestamp(cmpt);
		return true;
	}
	else
	{
		return false;
	}
}

bool SystemTree::updateNodeTimestamp(JausAddress address)
{
	JausNode node = findNode(address);
	if(node)
	{
		jausNodeUpdateTimestamp(node);
		return true;
	}
	else
	{
		return false;
	}
}

bool SystemTree::updateSubsystemTimestamp(JausAddress address)
{
	JausSubsystem subs = system[address->subsystem];
	if(subs)
	{
		jausSubsystemUpdateTimestamp(subs);
		return true;
	}
	else
	{
		return false;
	}
}

unsigned char SystemTree::getNextInstanceId(JausAddress address)
{
	bool instanceAvailable[JAUS_MAXIMUM_INSTANCE_ID] = {true};
	int i = 0;
	
	for(i=0; i < JAUS_MAXIMUM_INSTANCE_ID; i++)
	{
		instanceAvailable[i] = true;
	}

	// Get this node
	JausNode node = findNode(address->subsystem, address->node);

	if(node)
	{
		// We'll go through all the attached components and check for matches
		for(i=0; i < node->components->elementCount; i++)
		{
			JausComponent cmpt = (JausComponent) node->components->elementData[i];
		
			// if this is the same cmpt Id, we'll mark that instance as used
			if(cmpt->address->component == address->component)
			{
				instanceAvailable[cmpt->address->instance] = false;
			}
		}

		// Find the lowest value instance that is available
		i = JAUS_MINIMUM_INSTANCE_ID;
		while(i <= JAUS_MAXIMUM_INSTANCE_ID)
		{
			if(instanceAvailable[i])
			{
				return i;
			}
			i++;
		}
		return JAUS_INVALID_INSTANCE_ID;
	}
	return JAUS_INVALID_INSTANCE_ID;
}

bool SystemTree::hasComponent(JausComponent cmpt)
{
	JausAddress address = cmpt->address;
	return hasComponent(address->subsystem, address->node, address->component, address->instance);
}

bool SystemTree::hasComponent(JausAddress address)
{
	return hasComponent(address->subsystem, address->node, address->component, address->instance);
}

bool SystemTree::hasComponent(int subsystemId, int nodeId, int componentId, int instanceId)
{
	JausComponent cmpt = findComponent(subsystemId, nodeId, componentId, instanceId);
	if(cmpt)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool SystemTree::hasNode(JausNode node)
{
	return hasNode(node->subsystem->id, node->id);
}

bool SystemTree::hasNode(JausAddress address)
{
	return hasNode(address->subsystem, address->node);
}

bool SystemTree::hasNode(int subsystemId, int nodeId)
{
	JausNode node = findNode(subsystemId, nodeId);
	if(node)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool SystemTree::hasSubsystem(JausSubsystem subsystem)
{
	return hasSubsystem(subsystem->id);
}

bool SystemTree::hasSubsystem(JausAddress address)
{
	return hasSubsystem(address->subsystem);
}

bool SystemTree::hasSubsystem(int subsystemId)
{
	if(system[subsystemId])
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool SystemTree::hasSubsystemConfiguration(JausSubsystem subsystem)
{
	return hasSubsystemConfiguration(subsystem->id);
}

// TODO: Move this to JausSubsystem
bool SystemTree::hasSubsystemConfiguration(JausAddress address)
{
	return hasSubsystemConfiguration(address->subsystem);
}

bool SystemTree::hasSubsystemConfiguration(int subsId)
{
	if(system[subsId])
	{
		JausSubsystem subs = system[subsId];
		if(subs->nodes->elementCount > 0)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

bool SystemTree::hasSubsystemIdentification(JausSubsystem subsystem)
{
	return hasSubsystemIdentification(subsystem->id);
}

bool SystemTree::hasSubsystemIdentification(JausAddress address)
{
	return hasSubsystemIdentification(address->subsystem);
}

bool SystemTree::hasSubsystemIdentification(int subsId)
{
	JausSubsystem subs = system[subsId];
	if(subs && subs->identification)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool SystemTree::hasNodeIdentification(JausNode node)
{
	return hasNodeIdentification(node->subsystem->id, node->id);
}

bool SystemTree::hasNodeIdentification(JausAddress address)
{
	return hasNodeIdentification(address->subsystem, address->node);
}

bool SystemTree::hasNodeIdentification(int subsystemId, int nodeId)
{
	JausNode node = findNode(subsystemId, nodeId);
	if(node && node->identification)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool SystemTree::hasNodeConfiguration(JausAddress address)
{
	JausNode node = findNode(address);
	if(node && node->components->elementCount > 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool SystemTree::hasComponentIdentification(JausAddress address)
{
	return hasComponentIdentification(address->subsystem, address->node, address->component, address->instance);
}

bool SystemTree::hasComponentIdentification(int subsystemId, int nodeId, int componentId, int instanceId)
{
	JausComponent cmpt = findComponent(subsystemId, nodeId, componentId, instanceId);
	if(cmpt && cmpt->identification)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool SystemTree::hasComponentServices(JausAddress address)
{
	return hasComponentServices(address->subsystem, address->node, address->component, address->instance);
}

bool SystemTree::hasComponentServices(int subsystemId, int nodeId, int componentId, int instanceId)
{
	JausComponent cmpt = findComponent(subsystemId, nodeId, componentId, instanceId);
	if(cmpt && cmpt->services->elementCount > 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

JausAddress SystemTree::lookUpAddress(JausAddress address)
{
	return lookUpAddress(address->subsystem, address->node, address->component, address->instance);
}

JausAddress SystemTree::lookUpAddressInNode(JausNode node, int lookupCmpt, int lookupInst)
{
	if(node)
	{
		for(int i = 0; i < node->components->elementCount; i++)
		{
			JausComponent cmpt = (JausComponent)node->components->elementData[i];
			if(lookupCmpt == JAUS_ADDRESS_WILDCARD_OCTET)
			{
				if(lookupInst == JAUS_ADDRESS_WILDCARD_OCTET)
				{
					return jausAddressClone(cmpt->address);
				}
				else
				{
					if(cmpt->address->instance == lookupInst)
					{
						return jausAddressClone(cmpt->address);
					}
				}
			}
			else
			{
				if(cmpt->address->component == lookupCmpt)
				{
					if(lookupInst == JAUS_ADDRESS_WILDCARD_OCTET)
					{
						return jausAddressClone(cmpt->address);
					}
					else
					{
						if(cmpt->address->instance == lookupInst)
						{
							return jausAddressClone(cmpt->address);
						}
					}					
				}
			}
		}		
	}
	return NULL;
}

JausAddress SystemTree::lookUpAddressInSubsystem(JausSubsystem subs, int lookupNode, int lookupCmpt, int lookupInst)
{
	if(subs)
	{
		if(lookupNode == JAUS_ADDRESS_WILDCARD_OCTET)
		{
			for(int i = 0; i < subs->nodes->elementCount; i++)
			{
				JausNode node = (JausNode)subs->nodes->elementData[i];
				JausAddress address = lookUpAddressInNode(node, lookupCmpt, lookupInst);
				if(address)
				{
					return address;
				}
			}
		}
		else
		{
			return lookUpAddressInNode(findNode(subs->id, lookupNode), lookupCmpt, lookupInst);
		}
	}
	return NULL;
}

JausAddress SystemTree::lookUpAddress2(int lookupSubs, int lookupNode, int lookupCmpt, int lookupInst)
{
	if(lookupSubs == JAUS_ADDRESS_WILDCARD_OCTET)
	{
		for(int i = JAUS_MINIMUM_SUBSYSTEM_ID; i < JAUS_MAXIMUM_SUBSYSTEM_ID; i++)
		{
			JausAddress address = lookUpAddressInSubsystem(system[i], lookupNode, lookupCmpt, lookupInst);
			if(address)
			{
				return address;
			}
		}
	}
	else
	{
		return lookUpAddressInSubsystem(system[lookupSubs], lookupNode, lookupCmpt, lookupInst);
	}
	return NULL;
}

JausAddress SystemTree::lookUpAddress(int lookupSubs, int lookupNode, int lookupCmpt, int lookupInst)
{
	JausSubsystem subs = NULL;
	JausNode node = NULL;
	JausComponent cmpt = NULL;
	
	JausAddress address = jausAddressCreate();
	if(!address)
	{
		// TODO: Log Error. Throw Exception.
		return NULL;
	}
	address->subsystem = JAUS_INVALID_SUBSYSTEM_ID;
	address->node = JAUS_INVALID_NODE_ID;
	address->component = JAUS_INVALID_COMPONENT_ID;
	address->instance = JAUS_INVALID_INSTANCE_ID;
	address->next = NULL;

	JausAddress returnAddress = address;

	// Find this address
	// If the subsystem is a wild card 
	if(lookupSubs == JAUS_ADDRESS_WILDCARD_OCTET)
	{
		// Look through all subsystems
		for(int i = JAUS_MINIMUM_SUBSYSTEM_ID; i < JAUS_MAXIMUM_SUBSYSTEM_ID; i++)
		{
			subs = system[i];
			if(!subs)
			{
				continue;
			}

			// If the node is a wild card
			if(lookupNode == JAUS_ADDRESS_WILDCARD_OCTET)
			{
				// Look through all nodes
				for(int j = 0; j < subs->nodes->elementCount; j++)
				{
					node = (JausNode) subs->nodes->elementData[j];
					
					// If the component or the instance is a wild card
					if(lookupCmpt == JAUS_ADDRESS_WILDCARD_OCTET || lookupInst == JAUS_ADDRESS_WILDCARD_OCTET)
					{
						// Look through all components
						for(int k = 0; k < node->components->elementCount; k++)
						{
							cmpt = (JausComponent) node->components->elementData[k];

							// If the component is wild card and this is the instance we are looking for
							if(	lookupCmpt == JAUS_ADDRESS_WILDCARD_OCTET && cmpt->address->instance == lookupInst)
							{
								// If the address is valid (it should never be)
								if(jausAddressIsValid(address))
								{
									address->next = jausAddressClone(cmpt->address);	// Why do we clone if the address is valid??
									if(!address->next)
									{
										return returnAddress;
									}
									else
									{
										address = address->next;
									}
								}
								// Address was not valid
								else
								{
									jausAddressCopy(address, cmpt->address);	// Copy the address of the component we are looking at
									address->next = jausAddressCreate();		// Create a new address. Gets created even if we don't do anything else?
									if(!address->next)
									{
										return returnAddress;
									}
									else
									{
										address = address->next;
									}
								}
							}
							// If the instance is a wild card and this is the component we are looking for
							else if(lookupInst == JAUS_ADDRESS_WILDCARD_OCTET && cmpt->address->component == lookupCmpt)
							{
								if(jausAddressIsValid(address))
								{
									address->next = jausAddressClone(cmpt->address);
									if(!address->next)
									{
										return returnAddress;
									}
									else
									{
										address = address->next;
									}
								}
								else
								{
									jausAddressCopy(address, cmpt->address);
									address->next = jausAddressCreate();
									if(!address->next)
									{
										return returnAddress;
									}
									else
									{
										address = address->next;
									}
								}								
							}
						}
					}
					else
					{
						cmpt = findComponent(subs->id, node->id, lookupCmpt, lookupInst);

						if(cmpt)
						{
							if(jausAddressIsValid(address))
							{
								address->next = jausAddressClone(cmpt->address);
								if(!address->next)
								{
									return returnAddress;
								}
								else
								{
									address = address->next;
								}
							}
							else
							{
								jausAddressCopy(address, cmpt->address);
								address->next = jausAddressCreate();
								if(!address->next)
								{
									return returnAddress;
								}
								else
								{
									address = address->next;
								}
							}								
						}
					}
				}
			}
			else
			{
				node = findNode(i, lookupNode);
				if(lookupCmpt == JAUS_ADDRESS_WILDCARD_OCTET || lookupInst == JAUS_ADDRESS_WILDCARD_OCTET)
				{
					// Look through all components
					for(int k = 0; k < node->components->elementCount; k++)
					{
						cmpt = (JausComponent) node->components->elementData[k];

						if(	lookupCmpt == JAUS_ADDRESS_WILDCARD_OCTET &&
							cmpt->address->instance == lookupInst)
						{
							if(jausAddressIsValid(address))
							{
								address->next = jausAddressClone(cmpt->address);
								if(!address->next)
								{
									return returnAddress;
								}
								else
								{
									address = address->next;
								}
							}
							else
							{
								jausAddressCopy(address, cmpt->address);
								address->next = jausAddressCreate();
								if(!address->next)
								{
									return returnAddress;
								}
								else
								{
									address = address->next;
								}
							}
						}
						else if(lookupInst == JAUS_ADDRESS_WILDCARD_OCTET &&
								cmpt->address->component == lookupCmpt)
						{
							if(jausAddressIsValid(address))
							{
								address->next = jausAddressClone(cmpt->address);
								if(!address->next)
								{
									return returnAddress;
								}
								else
								{
									address = address->next;
								}
							}
							else
							{
								jausAddressCopy(address, cmpt->address);
								address->next = jausAddressCreate();
								if(!address->next)
								{
									return returnAddress;
								}
								else
								{
									address = address->next;
								}
							}								
						}
					}
				}
				else
				{
					cmpt = findComponent(subs->id, node->id, lookupCmpt, lookupInst);

					if(cmpt)
					{
						if(jausAddressIsValid(address))
						{
							address->next = jausAddressClone(cmpt->address);
							if(!address->next)
							{
								return returnAddress;
							}
							else
							{
								address = address->next;
							}
						}
						else
						{
							jausAddressCopy(address, cmpt->address);
							address->next = jausAddressCreate();
							if(!address->next)
							{
								return returnAddress;
							}
							else
							{
								address = address->next;
							}
						}								
					}
				}
			}
		}
	}
	// We are looking within a specific subsystem
	else
	{
		subs = system[lookupSubs];
		if(!subs)
		{
			// Subsystem not found
			// TODO: Log Error. Throw Exception.
			return address;
		}

		if(lookupNode == JAUS_ADDRESS_WILDCARD_OCTET)
		{
			// Look through all nodes
			for(int j = 0; j < subs->nodes->elementCount; j++)
			{
				node = (JausNode) subs->nodes->elementData[j];
				
				if(lookupCmpt == JAUS_ADDRESS_WILDCARD_OCTET || lookupInst == JAUS_ADDRESS_WILDCARD_OCTET)
				{
					// Look through all components
					for(int k = 0; k < node->components->elementCount; k++)
					{
						cmpt = (JausComponent) node->components->elementData[k];

						if(	lookupCmpt == JAUS_ADDRESS_WILDCARD_OCTET && cmpt->address->instance == lookupInst)
						{
							if(jausAddressIsValid(address))
							{
								address->next = jausAddressClone(cmpt->address);
								if(!address->next)
								{
									return returnAddress;
								}
								else
								{
									address = address->next;
								}
							}
							else
							{
								jausAddressCopy(address, cmpt->address);
								address->next = jausAddressCreate();
								if(!address->next)
								{
									return returnAddress;
								}
								else
								{
									address = address->next;
								}
							}
						}
						else if(lookupInst == JAUS_ADDRESS_WILDCARD_OCTET && cmpt->address->component == lookupCmpt)
						{
							if(jausAddressIsValid(address))
							{
								address->next = jausAddressClone(cmpt->address);
								if(!address->next)
								{
									return returnAddress;
								}
								else
								{
									address = address->next;
								}
							}
							else
							{
								jausAddressCopy(address, cmpt->address);
								address->next = jausAddressCreate();
								if(!address->next)
								{
									return returnAddress;
								}
								else
								{
									address = address->next;
								}
							}								
						}
					}
				}
				else
				{
					cmpt = findComponent(subs->id, node->id, lookupCmpt, lookupInst);

					if(cmpt)
					{
						if(jausAddressIsValid(address))
						{
							address->next = jausAddressClone(cmpt->address);
							if(!address->next)
							{
								return returnAddress;
							}
							else
							{
								address = address->next;
							}
						}
						else
						{
							jausAddressCopy(address, cmpt->address);
							address->next = jausAddressCreate();
							if(!address->next)
							{
								return returnAddress;
							}
							else
							{
								address = address->next;
							}
						}								
					}
				}
			}
		}
		else
		{
			node = findNode(subs->id, lookupNode);
			if(lookupCmpt == JAUS_ADDRESS_WILDCARD_OCTET || lookupInst == JAUS_ADDRESS_WILDCARD_OCTET)
			{
				// Look through all components
				for(int k = 0; k < node->components->elementCount; k++)
				{
					cmpt = (JausComponent) node->components->elementData[k];

					if(	lookupCmpt == JAUS_ADDRESS_WILDCARD_OCTET &&
						cmpt->address->instance == lookupInst)
					{
						if(jausAddressIsValid(address))
						{
							address->next = jausAddressClone(cmpt->address);
							if(!address->next)
							{
								return returnAddress;
							}
							else
							{
								address = address->next;
							}
						}
						else
						{
							jausAddressCopy(address, cmpt->address);
							address->next = jausAddressCreate();
							if(!address->next)
							{
								return returnAddress;
							}
							else
							{
								address = address->next;
							}
						}
					}
					else if(lookupInst == JAUS_ADDRESS_WILDCARD_OCTET &&
							cmpt->address->component == lookupCmpt)
					{
						if(jausAddressIsValid(address))
						{
							address->next = jausAddressClone(cmpt->address);
							if(!address->next)
							{
								return returnAddress;
							}
							else
							{
								address = address->next;
							}
						}
						else
						{
							jausAddressCopy(address, cmpt->address);
							address->next = jausAddressCreate();
							if(!address->next)
							{
								return returnAddress;
							}
							else
							{
								address = address->next;
							}
						}								
					}
				}
			}
			else
			{
				cmpt = findComponent(subs->id, node->id, lookupCmpt, lookupInst);

				if(cmpt)
				{
					if(jausAddressIsValid(address))
					{
						address->next = jausAddressClone(cmpt->address);
						if(!address->next)
						{
							return returnAddress;
						}
						else
						{
							address = address->next;
						}
					}
					else
					{
						jausAddressCopy(address, cmpt->address);
						address->next = jausAddressCreate();
						if(!address->next)
						{
							return returnAddress;
						}
						else
						{
							address = address->next;
						}
					}								
				}
			}
		}
	}
	return returnAddress;
}

JausAddress SystemTree::lookUpServiceInSystem(int commandCode, int serviceType)
{
	JausAddress list = NULL;
	JausAddress cur = list;

	if(serviceType != JAUS_SERVICE_INPUT_COMMAND && serviceType != JAUS_SERVICE_OUTPUT_COMMAND)
	{
		return NULL;
	}

	for(int i = JAUS_MINIMUM_SUBSYSTEM_ID; i < JAUS_MAXIMUM_SUBSYSTEM_ID; i++)
	{
		JausSubsystem subs = system[i];
		if(!subs)
		{
			continue;
		}

		for(int j = 0; j < subs->nodes->elementCount; j++)
		{
			JausNode node = (JausNode) subs->nodes->elementData[j];

			for(int k = 0; k < node->components->elementCount; k++)
			{
				JausComponent cmpt = (JausComponent) node->components->elementData[k];
				
				for(int m = 0; m < cmpt->services->elementCount; m++)
				{
					JausService service = (JausService) cmpt->services->elementData[m];
					JausCommand command = NULL;
					switch(serviceType)
					{
						case JAUS_SERVICE_INPUT_COMMAND:
							command = service->inputCommandList;
							break;
						
						case JAUS_SERVICE_OUTPUT_COMMAND:
							command = service->outputCommandList;
							break;
						
						default:
							// ERROR: Unknown service type
							// SHOULD NEVER GET HERE, CHECKED ABOVE
							return list;
					}

					while(command)
					{
						if(command->commandCode == commandCode)
						{
							// YAY... we found a matching commandCode
							// Add this component's address to our list
							JausAddress address = jausAddressClone(cmpt->address);
							address->next = NULL;
							
							if(list == NULL)
							{
								list = address;
								cur = list->next;
							}
							else
							{
								cur->next = address;
								cur = cur->next;
							}

							// This is a cheat to skip the rest of the commands
							// since a command should not repeat
							command = NULL;
						}
						else
						{
							command = command->next;
						}
					}
				}
			}
		}
	}

	return list;

}

JausSubsystem *SystemTree::getSystem(void)
{
	if(subsystemCount == 0)
	{
		return NULL;
	}

	JausSubsystem *systemClone = (JausSubsystem *)malloc(subsystemCount*sizeof(JausSubsystem));
	int systemCloneIndex = 0;
	
	for(int i = JAUS_MINIMUM_SUBSYSTEM_ID; i < JAUS_MAXIMUM_SUBSYSTEM_ID; i++)
	{
		if(system[i])
		{
			systemClone[systemCloneIndex] = jausSubsystemClone(system[i]);
			systemCloneIndex++;
		}
	}
	return systemClone;
}

JausSubsystem SystemTree::getSubsystem(JausSubsystem subsystem)
{
	return getSubsystem(subsystem->id);
}

JausSubsystem SystemTree::getSubsystem(JausAddress address)
{
	return getSubsystem(address->subsystem);
}

JausSubsystem SystemTree::getSubsystem(int subsystemId)
{
	JausSubsystem subs = system[subsystemId];
	if(subs)
	{
		return jausSubsystemClone(subs);
	}
	return NULL;
}

JausNode SystemTree::getNode(JausNode node)
{
	return getNode(node->subsystem->id, node->id);
}

JausNode SystemTree::getNode(JausAddress address)
{
	return getNode(address->subsystem, address->node);
}

JausNode SystemTree::getNode(int subsystemId, int nodeId)
{
	JausNode node = findNode(subsystemId, nodeId);
	if(node)
	{
		return jausNodeClone(node);
	}
	return NULL;
}

JausComponent SystemTree::getComponent(JausComponent cmpt)
{
	return getComponent(cmpt->address->subsystem, cmpt->address->node, cmpt->address->component, cmpt->address->instance);
}

JausComponent SystemTree::getComponent(JausAddress address)
{
	return getComponent(address->subsystem, address->node, address->component, address->instance);
}

JausComponent SystemTree::getComponent(int subsystemId, int nodeId, int componentId, int instanceId)
{
	JausComponent cmpt = findComponent(subsystemId, nodeId, componentId, instanceId);
	if(cmpt)
	{
		return jausComponentClone(cmpt);
	}
	return NULL;
}

JausNode SystemTree::findNode(JausNode node)
{
	return findNode(node->subsystem->id, node->id);
}

JausNode SystemTree::findNode(JausAddress address)
{
	return findNode(address->subsystem, address->node);
}

JausNode SystemTree::findNode(int subsId, int nodeId)
{
	JausSubsystem subs = system[subsId];
	if(subs)
	{
		for(int i = 0; i < subs->nodes->elementCount; i++)
		{
			JausNode node = (JausNode)subs->nodes->elementData[i];
			if(node->id == nodeId)
			{
				return node;
			}
		}
	}
	return NULL;
}

JausComponent SystemTree::findComponent(JausComponent cmpt)
{
	return findComponent(cmpt->address->subsystem, cmpt->address->node, cmpt->address->component, cmpt->address->instance);
}

JausComponent SystemTree::findComponent(JausAddress address)
{
	return findComponent(address->subsystem, address->node, address->component, address->instance);
}

JausComponent SystemTree::findComponent(int subsId, int nodeId, int cmptId, int instId)
{
	JausSubsystem subs = system[subsId];
	if(subs)
	{
		for(int i = 0; i < subs->nodes->elementCount; i++)
		{
			JausNode node = (JausNode)subs->nodes->elementData[i];
			if(node->id == nodeId)
			{
				for(int i = 0; i < node->components->elementCount; i++)
				{
					JausComponent cmpt = (JausComponent)node->components->elementData[i];
					if(	cmpt->address->component == cmptId && cmpt->address->instance == instId )
					{
						return cmpt;
					}
				}
				return NULL;
			}
		}
	}
	return NULL;
}

bool SystemTree::addComponent(JausComponent cmpt)
{
	return addComponent(cmpt->address->subsystem, cmpt->address->node, cmpt->address->component, cmpt->address->instance, cmpt);
}

bool SystemTree::addComponent(JausAddress address, JausComponent cmpt)
{
	return addComponent(address->subsystem, address->node, address->component, address->instance, cmpt);
}

bool SystemTree::addComponent(int subsystemId, int nodeId, int componentId, int instanceId, JausComponent cmpt)
{
	if(!this->hasComponent(subsystemId, nodeId, componentId, instanceId))
	{
		JausNode node = findNode(subsystemId, nodeId);
		if(node && componentId >= JAUS_MINIMUM_COMPONENT_ID && componentId <= JAUS_MAXIMUM_COMPONENT_ID
				&& instanceId >= JAUS_MINIMUM_INSTANCE_ID && instanceId <= JAUS_MAXIMUM_INSTANCE_ID)
		{
			// Add the component to the node
			if(cmpt)
			{
				cmpt = jausComponentClone(cmpt);
			}
			else
			{
				cmpt = jausComponentCreate();
				cmpt->address->instance = instanceId;
				cmpt->address->component = componentId;
				cmpt->address->node = nodeId;
				cmpt->address->subsystem = subsystemId;
			}
			cmpt->node = node;

			jausArrayAdd(node->components, cmpt);
			SystemTreeEvent *e = new SystemTreeEvent(SystemTreeEvent::ComponentAdded, cmpt);
			this->handleEvent(e);
			return true;
		}
	}

	return false;
}

bool SystemTree::addNode(JausNode node)
{
	return addNode(node->subsystem->id, node->id, node);
}

bool SystemTree::addNode(JausAddress address, JausNode node)
{
	return addNode(address->subsystem, address->node, node);
}

bool SystemTree::addNode(int subsystemId, int nodeId, JausNode node)
{
	if(!this->hasNode(subsystemId, nodeId))
	{
		JausSubsystem subs = system[subsystemId];
		if(subs && nodeId >= JAUS_MINIMUM_NODE_ID && nodeId <= JAUS_MAXIMUM_NODE_ID)
		{
			if(node)
			{
				node = jausNodeClone(node);
			}
			else
			{
				node = jausNodeCreate();
				node->id = nodeId;
			}

			node->subsystem = subs;
			
			jausArrayAdd(subs->nodes, node);

			SystemTreeEvent *e = new SystemTreeEvent(SystemTreeEvent::NodeAdded, node);
			this->handleEvent(e);

			return true;
		}
	}
	return false;
}

bool SystemTree::addSubsystem(JausSubsystem subs)
{
	return addSubsystem(subs->id, subs);
}

bool SystemTree::addSubsystem(JausAddress address, JausSubsystem subs)
{
	return addSubsystem(address->subsystem, subs);
}

bool SystemTree::addSubsystem(int subsystemId, JausSubsystem subs)
{
	if(!this->hasSubsystem(subsystemId))
	{
		// Test for valid ID
		if(subsystemId >= JAUS_MINIMUM_SUBSYSTEM_ID && subsystemId <= JAUS_MAXIMUM_SUBSYSTEM_ID)
		{
			if(subs)
			{
				subs = jausSubsystemClone(subs);
			}
			else
			{
				subs = jausSubsystemCreate();
				subs->id = subsystemId;
			}
			
			system[subs->id] = subs;
			subsystemCount++;

			SystemTreeEvent *e = new SystemTreeEvent(SystemTreeEvent::SubsystemAdded, subs);
			this->handleEvent(e);

			return true;
		}
	}
	return false;
}

bool SystemTree::removeNode(JausNode node)
{
	return removeNode(node->subsystem->id, node->id);
}

bool SystemTree::removeNode(JausAddress address)
{
	return removeNode(address->subsystem, address->node);
}

bool SystemTree::removeNode(int subsystemId, int nodeId)
{
	JausSubsystem subs = system[subsystemId];
	if(subs)
	{
		for(int i=0; i<subs->nodes->elementCount; i++)
		{
			JausNode node = (JausNode)subs->nodes->elementData[i];
			if(nodeId == node->id)
			{
				jausArrayRemoveAt(subs->nodes, i);

				SystemTreeEvent *e = new SystemTreeEvent(SystemTreeEvent::NodeRemoved, node);
				this->handleEvent(e);

				jausNodeDestroy(node);
				return true;
			}
		}
	}
	return false;
}

bool SystemTree::removeSubsystem(JausSubsystem subs)
{
	return removeSubsystem(subs->id);
}

bool SystemTree::removeSubsystem(JausAddress address)
{
	return removeSubsystem(address->subsystem);
}

bool SystemTree::removeSubsystem(int subsystemId)
{
	JausSubsystem subs = system[subsystemId];
	if(subs)
	{
		system[subsystemId] = NULL;

		SystemTreeEvent *e = new SystemTreeEvent(SystemTreeEvent::SubsystemRemoved, subs);
		this->handleEvent(e);

		jausSubsystemDestroy(subs);
		subsystemCount--;
		return true;
	}
	return false;
}

bool SystemTree::removeComponent(JausComponent cmpt)
{
	return removeComponent(cmpt->address->subsystem, cmpt->address->node, cmpt->address->component, cmpt->address->instance);
}

bool SystemTree::removeComponent(JausAddress address)
{
	return removeComponent(address->subsystem, address->node, address->component, address->instance);
}

bool SystemTree::removeComponent(int subsystemId, int nodeId, int componentId, int instanceId)
{
	JausNode node = findNode(subsystemId, nodeId);
	if(node)
	{
		for(int i = 0; i < node->components->elementCount; i++)
		{
			JausComponent cmpt = (JausComponent) node->components->elementData[i];
			if(cmpt->address->component == componentId && cmpt->address->instance == instanceId)
			{
				jausArrayRemoveAt(node->components, i);

				SystemTreeEvent *e = new SystemTreeEvent(SystemTreeEvent::ComponentRemoved, cmpt);
				this->handleEvent(e);

				jausComponentDestroy(cmpt);
				return true;
			}
		}
	}	
	return false;
}

bool SystemTree::replaceSubsystem(JausAddress address, JausSubsystem newSubs)
{
	return replaceSubsystem(address->subsystem, newSubs);
}

bool SystemTree::replaceSubsystem(int subsystemId, JausSubsystem newSubs)
{
	JausSubsystem currentSubs = system[subsystemId];
	if(!currentSubs)
	{
		addSubsystem(subsystemId, newSubs); // No subsystem to replace, so add the newSubs
		return true;
	}

	JausSubsystem cloneSubs = jausSubsystemClone(newSubs);
	if(!cloneSubs)
	{
		return false;
	}
	
	if(currentSubs->identification)
	{
		size_t stringLength = strlen(currentSubs->identification) + 1;
		cloneSubs->identification = (char *) realloc(cloneSubs->identification, stringLength);
		sprintf(cloneSubs->identification, "%s", currentSubs->identification);
	}

	for(int i = 0; i < cloneSubs->nodes->elementCount; i++)
	{
		JausNode cloneNode = (JausNode)cloneSubs->nodes->elementData[i];
		JausNode currentNode = findNode(cloneNode);
		if(currentNode)
		{
			if(currentNode->identification)
			{
				size_t stringLength = strlen(currentNode->identification) + 1;
				cloneNode->identification = (char *) realloc(cloneNode->identification, stringLength);
				sprintf(cloneNode->identification, "%s", currentNode->identification);
			}
			
			for(int i = 0; i < cloneNode->components->elementCount; i++)
			{
				JausComponent cloneCmpt = (JausComponent)cloneNode->components->elementData[i];
				JausComponent currentCmpt = findComponent(cloneCmpt);
				if(currentCmpt && currentCmpt->identification)
				{
					size_t stringLength = strlen(currentCmpt->identification) + 1;
					cloneCmpt->identification = (char *) realloc(cloneCmpt->identification, stringLength);
					sprintf(cloneCmpt->identification, "%s", currentNode->identification);
				}
			}
		}
	}
	jausSubsystemDestroy(system[subsystemId]);

	system[subsystemId] = cloneSubs;
	subsystemCount++;

	//char string[1024] = {0};
	//jausSubsystemTableToString(cloneSubs, string);
	//printf("Replaced Subsystem: \n%s\n", string);
	
	return true;
}

bool SystemTree::replaceNode(JausAddress address, JausNode newNode)
{
	return replaceNode(address->subsystem, address->node, newNode);
}

bool SystemTree::replaceNode(int subsystemId, int nodeId, JausNode newNode)
{
	JausNode currentNode = findNode(subsystemId, nodeId);
	if(!currentNode)
	{
		// No node to replace, so add the newNode
		addNode(subsystemId, nodeId, newNode);
		return true;
	}

	JausNode cloneNode = jausNodeCreate();
	if(!cloneNode)
	{
		return false;
	}
	cloneNode->id = currentNode->id;
	size_t stringLength = strlen(currentNode->identification) + 1;
	cloneNode->identification = (char *) realloc(cloneNode->identification, stringLength);
	sprintf(cloneNode->identification, "%s", currentNode->identification);
	cloneNode->subsystem = currentNode->subsystem;
	
	for(int i = 0; i < newNode->components->elementCount; i++)
	{
		JausComponent newCmpt = (JausComponent)newNode->components->elementData[i];
		JausComponent addCmpt = findComponent(newCmpt);
		if(addCmpt)
		{
			addCmpt = jausComponentClone(addCmpt);
		}
		else
		{
			addCmpt = jausComponentCreate();
			addCmpt->address->subsystem = subsystemId;
			addCmpt->address->node = nodeId;
			addCmpt->address->component = newCmpt->address->component;
			addCmpt->address->instance = newCmpt->address->instance;
		}
		addCmpt->node = cloneNode;
		jausArrayAdd(cloneNode->components, addCmpt);
	}

	for(int i = 0; i < system[subsystemId]->nodes->elementCount; i++)
	{
		JausNode node = (JausNode)system[subsystemId]->nodes->elementData[i];
		if(currentNode->id == node->id)
		{
			jausArrayRemoveAt(system[subsystemId]->nodes, i);
			jausNodeDestroy(node);
			break;
		}
	}
	
	cloneNode->subsystem = system[subsystemId];
	jausArrayAdd(system[subsystemId]->nodes, cloneNode);
	return true;
}

bool SystemTree::setSubsystemIdentification(JausAddress address, char *identification)
{
	JausSubsystem subs = system[address->subsystem];
	if(subs)
	{
		size_t stringLength = strlen(identification) + 1;
		subs->identification = (char *) realloc(subs->identification, stringLength);
		if(subs->identification)
		{
			sprintf(subs->identification, "%s", identification);
			return true;
		}
	}
	return false;
}

bool SystemTree::setNodeIdentification(JausAddress address, char *identification)
{
	JausNode node = findNode(address);
	if(node)
	{
		size_t stringLength = strlen(identification) + 1;
		node->identification = (char *) realloc(node->identification, stringLength);
		if(node->identification)
		{
			sprintf(node->identification, "%s", identification);
			return true;
		}
	}
	return false;
}

bool SystemTree::setComponentIdentification(JausAddress address, char *identification)
{
	JausComponent cmpt = findComponent(address);
	if(cmpt)
	{
		size_t stringLength = strlen(identification) + 1;
		cmpt->identification = (char *) realloc(cmpt->identification, stringLength);
		if(cmpt->identification)
		{
			sprintf(cmpt->identification, "%s", identification);
			return true;
		}
	}
	return false;
}

bool SystemTree::setComponentServices(JausAddress address, JausArray inputServices)
{
	JausComponent cmpt = findComponent(address);
	if(cmpt)
	{
		// Remove current service set
		if(cmpt->services) jausServicesDestroy(cmpt->services);
		cmpt->services = jausServicesClone(inputServices);
		return true;
	}
	return false;
}

char *SystemTree::getSubsystemIdentification(JausSubsystem subsystem)
{
	return getSubsystemIdentification(subsystem->id);
}

char *SystemTree::getSubsystemIdentification(JausAddress address)
{
	return getSubsystemIdentification(address->subsystem);
}

char *SystemTree::getSubsystemIdentification(int subsId)
{
	char *string = NULL;

	JausSubsystem subs = system[subsId];
	if(subs)
	{
		string = (char *)calloc(strlen(subs->identification)+1, sizeof(char));
		strcpy(string, subs->identification);
		return string;
	}
	else
	{
		return NULL;
	}
}

char *SystemTree::getNodeIdentification(JausNode node)
{
	return getNodeIdentification(node->subsystem->id, node->id);
}

char *SystemTree::getNodeIdentification(JausAddress address)
{
	return getNodeIdentification(address->subsystem, address->node);
}

char *SystemTree::getNodeIdentification(int subsId, int nodeId)
{
	char *string = NULL;

	JausNode node = findNode(subsId, nodeId);
	if(node)
	{
		string = (char *)calloc(strlen(node->identification)+1, sizeof(char));
		strcpy(string, node->identification);
		return string;
	}
	else
	{
		return NULL;
	}
}

std::string SystemTree::toString()
{
	string output = string();
	char buffer[4096] = {0};

	if(subsystemCount == 0)
	{
		return "System Tree Empty\n";
	}

	for(int i = JAUS_MINIMUM_SUBSYSTEM_ID; i < JAUS_MAXIMUM_SUBSYSTEM_ID; i++)
	{
		if(system[i])
		{
			jausSubsystemTableToString(system[i], buffer);
			output += buffer;
			output += "\n";
		}
	}

	return output;
}
	
std::string SystemTree::toDetailedString()
{
	string output = string();
	char buffer[20480] = {0};

	if(subsystemCount == 0)
	{
		return "System Tree Empty\n";
	}

	for(int i = JAUS_MINIMUM_SUBSYSTEM_ID; i < JAUS_MAXIMUM_SUBSYSTEM_ID; i++)
	{
		if(system[i])
		{
			jausSubsystemTableToDetailedString(system[i], buffer);
			output += buffer;
			output += "\n";
		}
	}

	return output;
}

void SystemTree::refresh()
{
	int i = 0;
	int j = 0;
	int k = 0;

	for(i = JAUS_MINIMUM_SUBSYSTEM_ID; i < JAUS_MAXIMUM_SUBSYSTEM_ID; i++)
	{
		if(system[i])
		{
			if(system[i]->id == mySubsystemId)
			{
				for(j = 0; j < system[i]->nodes->elementCount; j++)
				{
					JausNode node = (JausNode) system[i]->nodes->elementData[j];
					if(node->id == myNodeId)
					{
						for(k = 0; k < node->components->elementCount; k++)
						{
							JausComponent cmpt = (JausComponent) node->components->elementData[k];

							if(jausComponentIsTimedOut(cmpt))
							{
								//NOTE: The order here is important b/c event handlers may inspect the system tree and 
								//      we have to remove the cmpt from the tree before we notify of the change

								// Create Subsystem Event 
								SystemTreeEvent *e = new SystemTreeEvent(SystemTreeEvent::ComponentTimeout, cmpt);

								// Remove this component
								jausArrayRemoveAt(node->components, k); k--;

								// Destroy this memory
								jausComponentDestroy(cmpt);
							
								// Send off our event
								this->handleEvent(e);
							}
						}
					}
					else
					{
						if(jausNodeIsTimedOut(node))
						{
							//NOTE: The order here is important b/c event handlers may inspect the system tree and 
							//      we have to remove the node from the tree before we notify of the change
							
							// Create a timeout event
							SystemTreeEvent *e = new SystemTreeEvent(SystemTreeEvent::NodeTimeout, node);
							
							// Remove the Node
							removeNode(node);

							// Handle the event 
							this->handleEvent(e);
						}
					}
				}
			}
			else
			{
				if(jausSubsystemIsTimedOut(system[i]))
				{
					//NOTE: The order here is important b/c event handlers may inspect the system tree and 
					//      we have to remove the node from the tree before we notify of the change
					
					// Create our event with the proper information
					SystemTreeEvent *e = new SystemTreeEvent(SystemTreeEvent::SubsystemTimeout, system[i]);
					
					// Remove the dead subsystem
					removeSubsystem(system[i]->id);

					// Handle the event
					this->handleEvent(e);
				}
			}
		}
	}

}

bool SystemTree::registerEventHandler(EventHandler *handler)
{
	if(handler)
	{
		this->eventHandlers.push_back(handler);
		return true;
	}
	return false;
}

void SystemTree::handleEvent(NodeManagerEvent *e)
{
	// Send to all registered handlers
	std::list <EventHandler *>::iterator iter;
	for(iter = eventHandlers.begin(); iter != eventHandlers.end(); iter++)
	{
		(*iter)->handleEvent(e->cloneEvent());
	}
	delete e;
}


