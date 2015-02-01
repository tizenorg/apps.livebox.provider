/*
 * Copyright 2013  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.1 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://floralicense.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __DBOX_PROVIDER_DOC_H__
#define __DBOX_PROVIDER_DOC_H__

/**
 * @defgroup DYNAMICBOX_PROVIDER_MODULE dynamicbox
 * @brief To create a dynamic box provider, platform developer only.
 * @ingroup CAPI_DYNAMICBOX_FRAMEWORK 
 * @section DYNAMICBOX_PROVIDER_MODULE_HEADER Required Header
 *   \#include <dynamicbox_provider.h>
 *   \#include <dynamicbox_provider_buffer.h>
 * @section DYNAMICBOX_PROVIDER_MODULE_OVERVIEW Overview

<H1>Dynamic Box Provider - for Platform developer</H1>
This library supports to create a dynamic box provider.
But it is more low-level APIs than DYNAMICBOX_PROVIDER_APP_MODULE.

You have to use these APIs only if you know what you have to do.

At the first time, when your app is launched, you will get some extra data from master provider.
It will be passed to you via app-control data.

<TABLE>
<TH><TD>KEY</TD><TD>TYPE</TD><TD>VALUE</TD></TH>
<TR><TD>name</TD><TD>String</TD><TD>Name of your service provider app</TD></TR>
<TR><TD>secured</TD><TD>Integer</TD><TD>1 if your provider will gets only one package request or 0</TD></TR>
<TR><TD>abi</TD><TD>String</TD><TD>html, c, cpp, osp, ... defined in /usr/share/data-provider-master/abi.ini</TD></TR>
</TABLE>

After get data, you can initiate a provider.

\code
#include <dynamicbox_errnor.h>
int init_function(void)
{
    int ret;
    char *name;
    char *secured;
    char *abi;
    struct dynamicbox_event_handler table = {
        ...
    };

    ret = app_control_get_extra_data(service, "name", &name);
    if (ret != APP_CONTROL_ERROR_NONE) {
        LOGE("Name is not valid\n");
        return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    ret = app_control_get_extra_data(service, "secured", &secured);
    if (ret != APP_CONTROL_ERROR_NONE) {
        free(name);
        LOGE("Secured is not valid\n");
        return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    ret = app_control_get_extra_data(service, "abi", &abi);
    if (ret != APP_CONTROL_ERROR_NONE) {
        free(name);
        free(secured);
        return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    ret = dynamicbox_provider_init(NULL, name, &table, NULL, 1, 1);
    free(abi);
    free(name);
    free(secured);
    if (ret != DBOX_STATUS_ERROR_NONE) {
        LOGE("Failed to initiate dynamic box provider\n");
    }

    return ret;
}
\endcode

After initiate dynamicbox provider, the provider library will try to make a connection with master provider automatically.
If it gets connection successfully, you will get connected event callback.
The connected event callback is defined in dynamicbox_event_handler table.

\code
int (*disconnected)(struct event_arg *arg, void *data);
int (*connected)(struct event_arg *arg, void *data);
\endcode

Here is a sample for you with empty body functions.

\code
static int dbox_master_disconnected_cb(struct event_arg *arg, void *data)
{
    // If you lost connection from master, this callback will be called.
}

static int dbox_master_connected_cb(struct event_arg *arg, void *data)
{
    // If you get connection to master, this callback will be called.
}

int init_function(void)
{
    struct dynamicbox_event_handler table = {
        .disconnected = dbox_master_disconnected_cb,
        .connected = dbox_master_connected_cb,
        ...
    };
    ...
    ret = dynamicbox_provider_init(...);
    if (ret != DBOX_STATUS_ERROR_NONE) {
        // Failed to initiate the provider
    }
}
\endcode

When you get the connected callback, you have to send hello message to the master.
using dyanmicbox_provider_send_hello(). And you have to register the S/W watchdog timer.
This timer should send "ping" to master periodically. Its default period is 240 secs.
If your provider could not send the "ping" in 240 secs, the master will destroy your service process.
Basically, I recommend you to use 120 secs for period of watchdog timer.

\code
// Create a S/W watchdog
static struct info {
    Ecore_Timer *ping_timer;
} s_info = {
    .ping_timer = NULL,
};

static int dbox_master_connected_cb(struct event_arg *arg, void *data)
{
    int ret;

    ret = dynamicbox_provider_send_hello();
    if (ret != DBOX_STATUS_ERROR_NONE) {
        LOGE("Failed to send hello\n");
        return ret;
    }

    s_info.ping_timer = ecore_timer_add(120.0f, send_ping_cb, NULL);
    if (!s_info.ping_timer) {
        LOGE("Failed to add ping timer\n");
        return DBOX_STATUS_ERROR_FAULT;
    }

    return DBOX_STATUS_ERROR_NONE;
}
\endcode

Now we are ready to handling the events which will be comes from the master provider.
Here is the list of events.

<TABLE>
<TH><TD>Event Type</TD><TD>Event Handler</TD></TH>
<TR><TD>DBOX_EVENT_NEW</TD><TD>Master will send this to create a new dynamicbox instance</TD></TR>
<TR><TD>DBOX_EVENT_RENEW</TD><TD>If the master detects any problems of your slave, it will terminate slave provider.<BR/>and after reactivating the provider slave, this request will be delievered to create<BR/>a dynamicbox instance again</TD></TR>
<TR><TD>DBOX_EVENT_DELETE</TD><TD>Master will send this to delete a dynamicbox instance</TD></TR>
<TR><TD>DBOX_EVENT_CONTENT_EVENT</TD><TD>Any events are generated from your dynamicbox or Glance Bar, this event will be sent to you</TD></TR>
<TR><TD>DBOX_EVENT_CLICKED</TD><TD>If a dynamicbox is clicked, the master will send this event</TD></TR>
<TR><TD>DBOX_EVENT_TEXT_SIGNAL</TD><TD>Text type dynamicbox or Glance Bar will generate this event</TD></TR>
<TR><TD>DBOX_EVENT_RESIZE</TD><TD><TD>If a dynamicbox is resized, the master will send this event</TD></TR>
<TR><TD>DBOX_EVENT_SET_PERIOD</TD><TD>To change the update period of a dynamicbox</TD></TR>
<TR><TD>DBOX_EVENT_CHANGE_GROUP</TD><TD>To change the group(cluster/sub-cluster) of a dynamicbox</TD></TR>
<TR><TD>DBOX_EVENT_PINUP</TD><TD>To make pin up of a dynamicbox</TD></TR>
<TR><TD>DBOX_EVENT_UPDATE_CONTENT</TD><TD>It's time to update the content of a dynamicbox</TD></TR>
<TR><TD>DBOX_EVENT_PAUSE</TD><TD>Freeze all timer and go to sleep mode</TD></TR>
<TR><TD>DBOX_EVENT_RESUME</TD><TD>Thaw all timer and wake up</TD></TR>
<TR><TD>DBOX_EVENT_GBAR_CREATE</TD><TD>Only for the buffer type</TD></TR>
<TR><TD>DBOX_EVENT_GBAR_DESTROY</TD><TD>Only for the buffer type</TD></TR>
<TR><TD>DBOX_EVENT_GBAR_MOVE</TD><TD>Only for the buffer type</TD></TR>
<TR><TD>DBOX_EVENT_DBOX_PAUSE</TD><TD>Freeze the update timer of a specified dynamicbox</TD></TR>
<TR><TD>DBOX_EVENT_DBOX_RESUME</TD><TD>Thaw the update timer of a specified dynamicbox</TD></TR>
</TABLE>

These events types are delievered to you via event callbacks.

If a Dynamic Box is newly created, dbox_create() callback will be called.
It requires 3 out-parameters.
@a width, @a height, @priority
These parameters must be filled by you before return from dbox_create() callback.

More detailed information will be delievered to you via @a arg argument.
@arg includes many kinds of information. you can validate it by type field in it.

\code
int (*dbox_create)(struct event_arg *arg, int *width, int *height, double *priority, void *data);
int (*dbox_destroy)(struct event_arg *arg, void *data);
\endcode

If there is some error, the Dynamic Box could be recreated by dbox_recreate() callback.
\code
int (*dbox_recreate)(struct event_arg *arg, void *data);
\endcode

Following event callbacks will be called if a viewer(homescreen) is paused or resumed.
Those event callbacks are called when the viewer calls dynamicbox_viewer_set_paused() or dynamicbox_viewer_set_resumed()
When the LCD gets off, then the pause() callback will be called too. But it is not sent by viewer. it will be sent by master provider.
If a box is paused you will get dbox_pause() callback.

\code
int (*pause)(struct event_arg *arg, void *data);
int (*resume)(struct event_arg *arg, void *data);

int (*dbox_pause)(struct event_arg *arg, void *data);
int (*dbox_resume)(struct event_arg *arg, void *data);
\endcode

If some contents has changes or gets event from a user or system, it can generates specific content event.
the content event has event name & event source.

So you may be able to recognize which object (source) has changes (emission).
Basically the content_event() is called by script type Dynamic Box.
And the text_signal() is called when the viewer emits it.

\code
int (*content_event)(struct event_arg *arg, void *data);
int (*text_signal)(struct event_arg *arg, void *data);
\endcode

If a user clicks a dynamic box, the viewer will calls "dynamicbox_click()" function.
then clicked() callback will be called.

This clicked() callback is used for image type dynamic box.
But it also can be called even if you choose the script or buffer type dynamic box.

\code
int (*clicked)(struct event_arg *arg, void *data);
\endcode

When a user tries to resize your dynamic box, the resize() callback will be called.
If you get resize() callback call, you have to release previous buffer and acquire new one again.

To do that, you can use below listed API.

Of course, you may already creates a buffer using dynamicbox_provider_buffer_create() function.

 - dynamicbox_provider_buffer_create()
 - dynamicbox_provider_buffer_acquire()
 - dynamicbox_provider_buffer_release()
 - dynamicbox_provider_buffer_destroy()

To resize the buffer, you can use below one API simply.
 - dynamicbox_provider_buffer_resize()

It is more easy and fast then do manually release & acquire it again.

\code
int (*resize)(struct event_arg *arg, void *data);
\endcode

Sometimes, user or application service wants to change the update period of their dynamic box.
But it is only supported for installed instance. (running one)
If they calls dynamicbox_service_change_period() function (from libdynamicbox_service),
you will get this event callback.

If you get this callback call, you have to change the update-period.

\code
int (*set_period)(struct event_arg *arg, void *data);
\endcode

Dynamic Box has grouped by cluster & sub-cluster (aks. category).
(This is different with "category" tag in XML)

If your box has changed its group by user, this callback will be called.

\code
int (*change_group)(struct event_arg *arg, void *data);
\endcode


Also the dynamicbox supports Context Aware features.
If your box is created automatically by system event, user is able to pin up its contents to prevent from auto deleting or updating.
When its status is changed by calling the dynamicbox_pinup() function, you will get this callback.

\code
int (*pinup)(struct event_arg *arg, void *data);
\endcode

From the application or service, user or system can triggers update event manually using dynamicbox_service_trigger_update().
Then this callback will be called.
\code
int (*update_content)(struct event_arg *arg, void *data);
\endcode

There is one more content board called Glance Bar.
User can creates it by defined event (homescreen defines it) such as flick down.
To create the Glance Bar, homescreen application developer should call the dynamicbox_create_glance_bar()
Then the gbar_create() function will be called.

\code
int (*gbar_create)(struct event_arg *arg, void *data);
\endcode

Also if dynamic box viewer calles dynamicbox_destroy_glance_bar(), the provider will invoke your gbar_destroy() callback.
\code
int (*gbar_destroy)(struct event_arg *arg, void *data);
\endcode

and dynamicbox_move_glance_bar() function will invokes gbar_move() callback.

\code
int (*gbar_move)(struct event_arg *arg, void *data);
\endcode

There is a additional callback called "Update Mode".
To optimize the update event processing, there is "Auto Update Mode".
When the "Auto Update Mode" is turned on, the provider doesn't need to send update event for every updated frame.
Because the viewer will gets updated frame as possible as it can do.
In this case, only the "Locking mechanism" used for sync'ing with provider & viewer.
But you have to seriously consider to use this "Auto Update Mode".

If a viewer calls dynamicbox_set_update_mode() with true for @a active_mode, update_mode() callback will be called.

\code
int (*update_mode)(struct event_arg *arg, void *data);
\endcode
 */

#endif /* __DBOX_PROVIDER_DOC_H__ */
