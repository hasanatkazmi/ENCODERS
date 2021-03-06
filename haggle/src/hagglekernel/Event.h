/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 *   Sam Wood (SW)
 *   Hasnain Lakhani (HL)
 *   Yu-Ting Yu (yty)
 *   Hasanat Kazmi (HK)
 */

/* Copyright 2008-2009 Uppsala University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _EVENT_H
#define _EVENT_H

/*
 Forward declarations of all data types declared in this file. This is to
 avoid circular dependencies. If/when a data type is added to this file,
 remember to add it here.
 */
class EventHandler;
template<class T> class EventCallback;
class Event;

typedef int EventType;

#include <haggleutils.h>

#include <libcpphaggle/Heap.h>
#include <libcpphaggle/Timeval.h>

#include "DataObject.h"
#include "Interface.h"
#include "Node.h"
#include "Policy.h"
#include "Debug.h"
#include "Trace.h"

#define EVENT_TYPE_MIN  0
#define EVENT_TYPE_MAX  255
#define EVENT_TYPE_INVALID -1

#define EVENT_TYPE_PUBLIC_MIN EVENT_TYPE_MIN
#define EVENT_TYPE_PUBLIC_MAX (EVENT_TYPE_PRIVATE_MIN - 1)
#define EVENT_TYPE_PRIVATE_MAX (EVENT_TYPE_MAX)

#define MAX_NUM_PUBLIC_EVENT_TYPES (EVENT_TYPE_PRIVATE_MIN)
#define MAX_NUM_PRIVATE_EVENT_TYPES (EVENT_TYPE_MAX - EVENT_TYPE_PUBLIC_MAX)
#define MAX_NUM_EVENT_TYPES (EVENT_TYPE_MAX + 1)

#define EVENT_TYPE(t) ((t >= EVENT_TYPE_MIN) && (t < EVENT_TYPE_MAX))
#define EVENT_TYPE_PUBLIC(t) ((t >= EVENT_TYPE_PUBLIC_MIN) && (t <= EVENT_TYPE_PUBLIC_MAX))
#define EVENT_TYPE_PRIVATE(t) ((t >= EVENT_TYPE_PRIVATE_MIN) && (t < EVENT_TYPE_PRIVATE_MAX))

/*
 Haggle events.

 These are the different haggle events, and a short description of what they
 mean and what managers can/should do about them.

 EVENT_TYPE_PREPARE_STARTUP:
 FIXME: Undocumented. Please fill in.

 EVENT_TYPE_STARTUP:
 FIXME: Undocumented. Please fill in.

 EVENT_TYPE_PREPARE_SHUTDOWN:
 FIXME: Undocumented. Please fill in.

 EVENT_TYPE_SHUTDOWN:
 This event is posted once when the haggle kernel is starting to shut down,
 and is a signal to all managers that they should deregister.

 EVENT_TYPE_NODE_CONTACT_NEW:
 This event occurs when a new neighbor node is discovered.

 EVENT_TYPE_NODE_CONTACT_END:
 This event occurs when a neighbor node disappears.

 EVENT_TYPE_NODE_UPDATED:
 This event occurs when a neighbor node's node description is updated.

 EVENT_TYPE_NODE_DESCRIPTION_SEND:
 This event can be sent by a manager to trigger the local node's node
 description to be sent to the current neighbors. For instance when the
 interests change.

 EVENT_TYPE_NEIGHBOR_INTERFACE_UP:
 This event is sent by the connectivity manager when it finds a neighbor's
 interface.

 EVENT_TYPE_NEIGHBOR_INTERFACE_DOWN:
 This event is sent by the connectivity manager when it finds that a
 neighbor's interface is no longer available.

 EVENT_TYPE_LOCAL_INTERFACE_UP:
 This event is sent by the connectivity manager when it finds that a local
 interface is available.

 EVENT_TYPE_LOCAL_INTERFACE_DOWN:
 This event is sent by the connectivity manager when it finds that a local
 interface is no longer available.

 EVENT_TYPE_DATAOBJECT_NEW:
 This event is sent by the data manager when a data object has been inserted
 into the data store.

 EVENT_TYPE_DATAOBJECT_DELETED:
 This event is sent by the data manager when a data object has been removed
 from the data store.

 Flag bit:
 1         If this bit is set, the data object should also be deleted
 from a nodes bloomfilter.

 EVENT_TYPE_DATAOBJECT_FORWARD:
 This event is sent by any manager when that manager wishes to send a data
 object to another node. The forwarding manager will attempt to make sure
 that the data object reaches the node.

 EVENT_TYPE_DATAOBJECT_SEND:
 This event can be generated by any manager to send a data object to another
 node. It is processed by the protocol manager, which will send an
 EVENT_TYPE_DATAOBJECT_SEND_SUCCESSFUL or EVENT_TYPE_DATAOBJECT_SEND_FAILURE
 event in response, depending on wether it was able to send it or not.

 During processing of this event, it is safe to modify the data object to
 include manager-specific data in non-attribute tags. It is however not safe
 to add or modify attributes, as other managers might rely on the data object
 id.

 EVENT_TYPE_DATAOBJECT_VERIFIED:
 This event is sent by the security manager in response to an
 EVENT_TYPE_DATAOBJECT_RECEIVED event, no matter whether the data object
 passed verification or not. The result of the verification is specified
 in the data object.

 EVENT_TYPE_DATAOBJECT_RECEIVED:
 This event is sent by the protocol manager when a data object has been
 received.

 EVENT_TYPE_DATAOBJECT_SEND_SUCCESSFUL:
 This event is sent by the protocol manager in response to an
 EVENT_TYPE_DATAOBJECT_SEND event, if the data object was successfully sent.

 Flag bit: Meaning:
 1         If this bit is set, then this event was the result of a reject,
 rather than a fully sent data object.

 EVENT_TYPE_DATAOBJECT_SEND_FAILURE:
 This event is sent by the protocol manager in response to an
 EVENT_TYPE_DATAOBJECT_SEND event, if the data object was not successfully
 sent.

 EVENT_TYPE_DATAOBJECT_INCOMING:
 This event is sent by the protocol manager when it has received the header
 of a data object. The data object is still being constructed at this point,
 so all of its data may not be available.

 EVENT_TYPE_TARGET_NODES:
 This event is sent by the forwarding module in response to a new neighbor
 becoming available. It specifies which nodes the module thinks the neighbor
 is a good forwarder for. The nodes in the node list (the targets) are not
 complete - only the node id is set correctly.

 EVENT_TYPE_DELEGATE_NODES:
 This event is sent by the forwarding module in response to a request for
 good delegates for a certain node. It specifies which nodes the module
 thinks are likely to be able to forward data objects to the given node. The
 nodes in the node list (the targets) are not complete - only the node id is
 set correctly.

 EVENT_TYPE_RESOURCE_POLICY_NEW:
 This event is sent by the resource manager when it detects that the
 resources it is monitoring have changed significantly enough to change the
 resource policy.

 EVENT_TYPE_SECURITY_CONFIGURE:
 This event is sent by the application manager when an application adds
 new security configuration settings through the use of a control message;
 so that the security manager can configure appropriately.
 
 EVENT_TYPE_SEND_OBSERVER_DATAOBJECT:
 This event is sent by managers when they wish to send a message to
 any observer applications.

 EVENT_TYPE_DYNAMIC_CONFIGURE:
 This event is raised by the ApplicationManager whenever an application
 wishes to dynamically update the configuration.

 EVENT_TYPE_DEBUG_CMD:
 This event is only available when haggle is compiled with debugging enabled.
 It is used by the debug manager to tell other managers to do things in
 response to input from the user.

 EVENT_TYPE_CALLBACK:
 This event type is used to tell a specific manager about some event, without
 having to register a private event type, or posting a public event.
 */

// Public events are those that more than one Manager can listen to
// Private events are specified by individual managers.
// NOTE: When adding a new event, do not forget to update the event names
// in the static array in Event.cpp
typedef enum {
    EVENT_TYPE_PREPARE_STARTUP,
    EVENT_TYPE_STARTUP,
    EVENT_TYPE_PREPARE_SHUTDOWN,
    EVENT_TYPE_SHUTDOWN,
    EVENT_TYPE_NODE_CONTACT_NEW,
    EVENT_TYPE_NODE_CONTACT_END,
    EVENT_TYPE_NODE_UPDATED,
    EVENT_TYPE_APP_NODE_UPDATED, // MOS
    EVENT_TYPE_APP_NODE_CHANGED, // SW: above EVENT_TYPE is never called? 
    EVENT_TYPE_ROUTE_TO_APP, // SW
    EVENT_TYPE_NODE_DESCRIPTION_SEND,
    EVENT_TYPE_NEIGHBOR_INTERFACE_UP,
    EVENT_TYPE_NEIGHBOR_INTERFACE_DOWN,
    EVENT_TYPE_LOCAL_INTERFACE_UP,
    EVENT_TYPE_LOCAL_INTERFACE_DOWN,
    EVENT_TYPE_DATAOBJECT_AGING_FRAGMENTATION,
    EVENT_TYPE_DATAOBJECT_AGING_NETWORKCODEDBLOCK,
    EVENT_TYPE_DATAOBJECT_AGING_DECODER_NETWORKCODING,
    EVENT_TYPE_DATAOBJECT_AGING_DECODER_FRAGMENTATION,
    EVENT_TYPE_DATAOBJECT_CONVERT_FRAGMENTATION_NETWORKCODING,
    EVENT_TYPE_DATAOBJECT_NEW,
    EVENT_TYPE_DATAOBJECT_DELETED,
    EVENT_TYPE_DATAOBJECT_DELETE_NETWORKCODEDBLOCK,
    EVENT_TYPE_DATAOBJECT_DELETE_ASSOCIATED_NETWORKCODEDBLOCKS,
    EVENT_TYPE_DATAOBJECT_DELETE_FRAGMENT,
    EVENT_TYPE_DATAOBJECT_DELETE_ASSOCIATED_FRAGMENTS,
    EVENT_TYPE_DATAOBJECT_DISABLE_NETWORKCODING,
    EVENT_TYPE_DATAOBJECT_ENABLE_NETWORKCODING,
    EVENT_TYPE_RECEIVE_BEACON,
    EVENT_TYPE_DATAOBJECT_FORWARD,
    EVENT_TYPE_DATAOBJECT_SEND,
    EVENT_TYPE_DATAOBJECT_SEND_TO_APP, // MOS
    EVENT_TYPE_DATAOBJECT_SEND_NETWORKCODING,
    EVENT_TYPE_DATAOBJECT_SEND_FRAGMENTATION,
    EVENT_TYPE_DATAOBJECT_VERIFIED,
    EVENT_TYPE_DATAOBJECT_RECEIVED,
    EVENT_TYPE_DATAOBJECT_SEND_NETWORKCODING_SUCCESSFUL,
    EVENT_TYPE_DATAOBJECT_SEND_FRAGMENTATION_SUCCESSFUL,
    EVENT_TYPE_DATAOBJECT_SEND_SUCCESSFUL,
    EVENT_TYPE_DATAOBJECT_SEND_FAILURE,
    EVENT_TYPE_DATAOBJECT_INCOMING,
    EVENT_TYPE_DATAOBJECT_INCOMING_FRAGMENTATION_NETWORKCODING_CONVERSION,
    EVENT_TYPE_TARGET_NODES,
    EVENT_TYPE_DELEGATE_NODES,
    EVENT_TYPE_RESOURCE_POLICY_NEW,
// SW: START: SendPriorityManager
    EVENT_TYPE_SEND_PRIORITY,
    EVENT_TYPE_SEND_PRIORITY_SUCCESS,
    EVENT_TYPE_SEND_PRIORITY_FAILURE,
// SW: END: SendPriorityManager
    EVENT_TYPE_SECURITY_CONFIGURE, // CBMEN, HL
    EVENT_TYPE_SEND_OBSERVER_DATAOBJECT, // CBMEN, HL
    EVENT_TYPE_DYNAMIC_CONFIGURE, // CBMEN, HL
    EVENT_TYPE_APP_NODE_INTERESTS_POLICIES_REQUESTED, // IRD, HK
    EVENT_TYPE_APP_NODE_INTERESTS_POLICIES_SEND, // IRD, HK
    EVENT_TYPE_DEBUG_CMD,
    EVENT_TYPE_CALLBACK,
// SW: START REPLICATION MANAGER
    EVENT_TYPE_REPL_MANAGER, 
// SW: END REPLICATION MANAGER
    EVENT_TYPE_PRIVATE_MIN,
} EventTypePublic;

template<class T>
class EventCallback {
public:
    EventCallback(T* _obj, void (T::*_func)(Event*)) :
            obj(_obj), func(_func) {
    }
    virtual ~EventCallback() {
    }
    EventCallback(const EventCallback<T> &ec) :
            obj(ec.obj), func(ec.func) {
    }
    EventCallback<T> *copy() const {
        return new EventCallback<T>(*this);
    }
    const EventCallback<T>& operator=(const EventCallback<T> &) {
        HAGGLE_ERR("Copy assignment not implemented for EventCallback");
    }
    void operator()(Event* e) const {
        (obj->*func)(e);
    }
    T *obj;
    void (T::*func)(Event*);
};

// Classes that should be able to handle events should inherit from
// this class
/** */
class EventHandler {
    unsigned int num_events;
    EventCallback<EventHandler> *callbacks[MAX_NUM_PUBLIC_EVENT_TYPES];
protected:
    EventHandler() :
            num_events(0) {
        for (unsigned int i = 0; i < MAX_NUM_PUBLIC_EVENT_TYPES; i++)
            callbacks[i] = NULL;
    }
    virtual ~EventHandler() {
        for (unsigned int i = 0; i < MAX_NUM_PUBLIC_EVENT_TYPES; i++)
            if (callbacks[i]) {
                delete callbacks[i];
            }
    }
    EventCallback<EventHandler> *getEventInterest(EventType type) {
        if (EVENT_TYPE_PUBLIC(type))
            return callbacks[type];

        return NULL;
    }
    int addEventInterest(EventType type, EventCallback<EventHandler> *callback) {
        if (EVENT_TYPE_PUBLIC(type)) {
            if (callbacks[type]) {
                HAGGLE_ERR("EventHandler has already registered event type %u\n", type);
                delete callback;
            } else {
                callbacks[type] = callback;
                return 0;
            }
        }
        return -1;
    }
    int removeEventInterest(EventType type) {
        if (EVENT_TYPE_PUBLIC(type)) {
            delete callbacks[type];
            callbacks[type] = NULL;
            return 0;
        }
        return -1;
    }
};

/*
 In order to use these macros you HAVE to do the following
 #define __CLASS__ <class name>
 where <class name> is the name of the current class. The class name may
 NOT be parenthesized.
 */
#define newEventCallback(func) \
	(EventCallback < EventHandler > *) \
		new EventCallback < __CLASS__ > (\
			this, \
			&__CLASS__::func)
#define setEventHandler(type, func)  \
	addEventInterest(type, newEventCallback(func))
#define removeEventHandler(type) \
	removeEventInterest(type)
#define registerEventType(name, func) \
	Event::registerType(name, newEventCallback(func))
#define registerEventTypeForFilter(evt, name, func, filter) \
	do{ \
		evt = registerEventType(name, func); \
		Filter registerEventTypeForFilter_filter(filter, evt); \
		kernel->getDataStore()->insertFilter( \
			registerEventTypeForFilter_filter); \
	} while(0)
#define unregisterEventTypeForFilter(evt) \
	do{ \
		Event::unregisterType(evt); \
		kernel->getDataStore()->deleteFilter(evt); \
	} while(0)
/** */
#ifdef DEBUG_LEAKS
class Event : public LeakMonitor, public HeapItem
#else
class Event: public HeapItem
#endif
{
private:
    static const char *eventNames[MAX_NUM_EVENT_TYPES];
    static EventCallback<EventHandler> *privCallbacks[MAX_NUM_PRIVATE_EVENT_TYPES];
    static int num_event_types;
    const EventType type;
    Timeval timeout;bool scheduled;bool autoDelete;
    bool canceled; // CBMEN, HL
    const EventCallback<EventHandler> *callback;
    /*
     Data type contained in events:

     If any new types of data are added here, a corresponding constructor
     and modification to the destructor should be done.

     The data types below are Reference types that are reference counted.
     These data types are "well known" ones that may be passed in any
     type of event; public, private or callbacks. These types are only
     references to a "single" instance of an object of the specified type
     and may be referenced by several threads at the same time. As they
     are based on the Reference class, the object is protected by a mutex
     as long as dereferenced through the Reference reference. If an event
     handler wants to touch internal parts of the object without using the
     reference as the access method (e.g., iterating an interface list that
     is part of the object), then the caller must lock the reference for
     the duration of the operation.

     When receiveing an event with a reference counted object, there is no
     need to free the object. The event handler only owns the reference,
     not the object itself. The thread that processes the handler function
     may keep the reference to avoid the object from being deleted until the
     thread has finished with the object.
     */

    DataObjectRef dObj;
    NodeRef node;
    InterfaceRef iface;
    PolicyRef policy;
    DebugCmdRef dbgCmd;
    DataObjectRefList dObjs;

    /*
     This list of nodes is used in only the events EVENT_TYPE_TARGET_NODES
     and EVENT_TYPE_DELEGATE_NODES. In these events, it contains nodes where
     only the node id is set properly. All other information must be
     retreived from either the node store, or the data store.
     */
    NodeRefList nodes;

    /**
     Events support, in addition to the "well known" data types above, also
     generic data types. These are pointed to by the "data" pointer in the
     event. Generic data types are (in general) NOT reference counted, and
     as such the receiver of the event is either responsible for the
     destruction of the data once finished (private, callback event), or it
     has to make a copy of the object if it wants to keep the object outside
     its handler function (in the case of public events).

     */
    void *data;bool doesHaveData;

    /**
     Flags. The meaning of these event flags are specific to each type of
     event that uses them. See the list of haggle events above.
     */
    unsigned long flags;

    /* Static class functions that are private */
    static int privTypeToCallbackIndex(EventType type) {
        if (EVENT_TYPE_PRIVATE(type))
            return (type - MAX_NUM_PUBLIC_EVENT_TYPES);
        else
            return -1;
    }
public:
    Event(EventType _type, const DataObjectRef& _dObj, unsigned long flags = 0, double _delay = 0.0);
    Event(EventType _type, const InterfaceRef& _iface, double _delay = 0.0);
    Event(EventType _type, const NodeRef& _node, double _delay = 0.0);
    Event(EventType _type, const PolicyRef& _policy, double _delay = 0.0);
    Event(EventType _type, const DataObjectRef& _dObj, const NodeRef& _node, unsigned long flags = 0, double _delay =
            0.0);
    Event(const DebugCmdRef& _dbgCmdRef, double _delay = 0.0);
    Event(EventType _type, const NodeRef& _node, const NodeRefList& _nodes, double _delay = 0.0);
    Event(EventType _type, const DataObjectRef& _dObj, const NodeRefList& _nodes, double _delay = 0.0);
    Event(EventType _type, const DataObjectRef& _dObj, const NodeRef& _node, const NodeRefList& _nodes, double _delay =
            0.0);
    Event(EventType _type, const DataObjectRefList& _dObjs, unsigned long flags = 0, double _delay = 0.0);
    Event(EventType _type, void *_data = NULL, double _delay = 0.0);
    Event(const EventCallback<EventHandler> *_callback, void *_data, double _delay = 0.0);
    Event(const EventCallback<EventHandler> *_callback, const DataObjectRef& _dObj, double _delay = 0.0);
    Event(const EventCallback<EventHandler> *_callback, const InterfaceRef& _iface, double _delay = 0.0);
    Event(const EventCallback<EventHandler> *_callback, const NodeRef& _node, double _delay = 0.0);
    Event(const EventCallback<EventHandler> *_callback, const PolicyRef& _policy, double _delay = 0.0);
    Event(const EventCallback<EventHandler> *_callback, const DataObjectRefList& _dObjs, double _delay = 0.0);
    Event(const EventCallback<EventHandler> *_callback, const DebugCmdRef& _dbgCmd, double _delay = 0.0);
    // Event(const Event &); // CBMEN, HL - define this

    // const Event& operator=(const Event &); // CBMEN, HL - define this
    ~Event();

    EventType getType() const {
        return type;
    }

    const Timeval& getTimeout() const {
        return timeout;
    }
    void setTimeout(double t);

    bool hasData() {
        return doesHaveData;
    }
    void *getData() {
        return data;
    }

    void setScheduled(bool _scheduled = true);bool isScheduled() const;
    void setAutoDelete(bool _autoDelete = true);bool shouldDelete() const;
    void setCanceled(bool _canceled = true); bool isCanceled() const; // CBMEN, HL

    DataObjectRef& getDataObject() {
        return dObj;
    }
    DataObjectRefList& getDataObjectList() {
        return dObjs;
    }
    InterfaceRef& getInterface() {
        return iface;
    }
    NodeRef& getNode() {
        return node;
    }
    PolicyRef& getPolicy() {
        return policy;
    }
    DebugCmdRef& getDebugCmd() {
        return dbgCmd;
    }
    NodeRefList& getNodeList() {
        return nodes;
    }
    unsigned long getFlags() {
        return flags;
    }
    /* Static class functions that are private */
    static int registerType(const char *name, EventCallback<EventHandler> *_callback);
    static int unregisterType(EventType type);

    bool isCallback() {
        return ((type == EVENT_TYPE_CALLBACK) ? true : false);
    }
    bool isPrivate() {
        return ((EVENT_TYPE_PRIVATE(type) && privCallbacks[privTypeToCallbackIndex(type)]) ? true : false);
    }
    static bool isPrivate(EventType type) {
        return ((EVENT_TYPE_PRIVATE(type) && privCallbacks[privTypeToCallbackIndex(type)]) ? true : false);
    }
    static const char *getPublicName(EventType _type) {
        return (EVENT_TYPE_PUBLIC(_type) ? eventNames[_type] : NULL);
    }
    const char *getName() const {
        if (eventNames[type] != NULL)
            return eventNames[type];
        else
            return "[Unknown event type]";
    }
    string getDescription(void);
    void doPrivateCallback() {

        if (!EVENT_TYPE_PRIVATE(type))
            return;
        EventCallback<EventHandler> *privCallback = privCallbacks[privTypeToCallbackIndex(type)];
        if (privCallback)
            (*privCallback)(this);
    }
    void doCallback() {
        if (type != EVENT_TYPE_CALLBACK)
            return;
        (*callback)(this);
    }
    bool compare_less(const HeapItem& i) const;bool compare_greater(const HeapItem& i) const;
};

#endif /* _EVENT_H */
