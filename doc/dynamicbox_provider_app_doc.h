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

#ifndef __DBOX_PROVIDER_APP_DOC_H__
#define __DBOX_PROVIDER_APP_DOC_H__

/**
 * @defgroup DYNAMICBOX_PROVIDER_APP_MODULE dynamicbox
 * @brief To create a dynamic box provider
 * @ingroup CAPI_DYNAMICBOX_FRAMEWORK 
 * @section DYNAMICBOX_PROVIDER_APP_MODULE_HEADER Required Header
 *   \#include <dynamicbox_provider_app.h>
 * @section DYNAMICBOX_PROVIDER_APP_MODULE_OVERVIEW Overview
<H1>1. Dynamic Box Provider</H1>
Dynamic Box is a widget on the homescreen.
If you want your application to show real time updates on the Home application screen, you can use Dynamic Box to create a small application within your existing application to provide such updates both periodically and asynchronously.

There are so many kinds of development model of Dynamic Box provider.

Basically, Tizen supports prcess model for Dynamic Box provider.

You will get a new process and its life cycle will be managed by Dynamic Box Service Manager.
Of course, user can create and remove the process by adding / removing it from the home application screen.

<H1>2. How the Dynamic Box provider application is able to communicate with the home application</H1>
@TODO insert overview image
When your Dynamic Box provider application is launched, you will get app_control event.
Then you have to initate the provider using dynamicbox_provider_app_init().

At that time your application will be connected to the Dynamic Box Service Manager.
After it is connected, it will try to manage your provider application.

If your provider application is launched by Dynamic Box Service Manager, it means, user tries
to add your box to the home application screen.

<H1>3. How to create a new project for developing the Dynamic Box</H1>
@TODO insert image for SDK

<H1>4. How to write code</H1>

The provider application library requires a table of event functions.
You have to fill them first or just leave it with NULL if you don't need to service it.

There are 3 major event set.

<UL>
<LI>Dynamic Box Events</LI>
<LI>Glance Bar events</LI>
<LI>Common events</LI>
</UL>

@code
#include <stdio.h>
#include <stdlib.h> // calloc
#include <string.h> // strerror
#include <errno.h> // errno

#include <dynamicbox_provider_app.h>

int errno;

static void _app_control_cb(app_control_h service, void *data)
{
    char *value = NULL;
    int ret;
    struct provider_event_callback table = {
        .dbox = {
            .create = dbox_create,
            .resize = dbox_resize,
            .destroy = dbox_destroy,
        },
        .gbar = {
            .create = gbar_create,
            .resize_move = gbar_resize_move,
            .destroy = gbar_destroy,
        },
        .clicked = lb_clicked,
        .update = dbox_update,
        .script_event = dbox_script_event,
        .pause = dbox_pause,
        .resume = dbox_resume,
        .connected = provider_connected,
        .disconnected = provider_disconnected,
        .data = NULL,
    };

    if (dynamicbox_provider_app_init(service, &table) < 0) {
        // Case 1, Already initialized
        // Case 2, Invalid service parameter
        // Case 3, error
    }
}

int main(int argc, char *argv[])
{
    app_event_callback_s app_callback = {0,};

    app_callback.create = _create_cb;
    app_callback.terminate = _terminate_cb;
    app_callback.pause = _pause_cb;
    app_callback.resume = _resume_cb;
    app_callback.app_control = _app_control_cb;
    app_callback.language_changed = NULL;

    return app_efl_main(&argc, &argv, &app_callback, NULL);
}
@endcode

<H2>4.1 Creating a new dynamic box</H2>

At the first time, when you get the app_control event, you should initiate the connection with Dynamic Box Service Manager.
To do it, you have to call the dynamicbox_provider_app_init() function.

When you get the "name" & "secured" & "abi" data via app_control data,
you have to initiate the connection. only if those three information is given to you at same time,
then the Dynamic Box Service Manager initiate your provider app.

So you have to call dynamicbox_provider_app_init() function.

After initiate provider, you are able to receive events from the Dynamic Box Service Manager.

If a user tries to add a new dynamic box to the homescreen, the provider will calls dbox_create() callback.

@code

struct my_dbox_data {
    // ...
};

static int dbox_create(const char *id, const char *content, int w, int h, void *data)
{
    struct my_dbox_data *dbox_data;

    dbox_data = calloc(1, sizeof(*dbox_data));
    if (!dbox_data) {
        LOGE("Failed to allocate a memory for instance data: %s\n", strerror(errno));
        return LB_STATUS_ERROR_MEMORY;
    }

    // Link "info" with "Id"
    dynamicbox_provider_app_set_data(id, info);
    return LB_STATUS_SUCCESS;
}
@endcode

As you see in the above code, dynamicbox_provider_app_set_data() will help you to manage the private data for each instance.
When a new dynamic box is created, you will receive the uniq @a id.
The @a id will be kept while the system is running, it could be changed however if a user reboots device, the homescreen will adds your dynamic box again, then you will get dbox_create() event again. But the @a id is not same with before.

So you have to manage your own uniq id for your dynamic box instance.

The second parameter @a content is designated for your own uniq id.
At the first time, when the dynamic box is created, the @a content inidicates @c NULL.
But if you change it, you will get changed string.

To change it you can use the dynamicbox_set_extra_info() function.
it is supported from DYANMICBOX_UTILITY_MODULE (libdynamic package).

But it will updates its content string only if your box is really updated.
so you have to updates(renders) your content screen after set that extra info.

<H2>4.2 Resizing a dynamoc box</H2>
@code
static int dbox_resize(const char *id, int w, int h, void *data)
{
    struct my_dbox_data *dbox_data;

    dbox_data = dynamicbox_provider_app_data(id);
    if (!dbox_data) {
        return LB_STATUS_ERROR_NOT_EXIST;
    }

    // TODO: Resize the contents

    return DBOX_STATUS_ERROR_NONE;
}
@endcode

If a user tries to resize your dynamicbox in the homescreen, the dbox_resize() callback will be called.
(which is registerd on the event table by you)

Then you just need to resize the window of the dynamic box.

<H2>4.3 Destroying a dynamic box</H2>
@code
#include <dynamicbox_service.h>

static int dbox_destroy(const char *id, int reason, void *data)
{
    struct my_dbox_data *dbox_data;

    dbox_data = dynamicbox_provider_app_data(id);
    if (!dbox_data) {
        return LB_STATUS_ERROR_NOT_EXIST;
    }

    dynamicbox_provider_app_set_data(id, NULL);

    switch (reason) {
    case DBOX_DESTROY_TYPE_DEFAULT:
        // Permanently deleted by user.
        break;
    case DBOX_DESTROY_TYPE_UPGRADE:
        // Delete for upgrading. If the box still exsists after upgrading, it will be created again.
        break;
    case DBOX_DESTROY_TYPE_UNINSTALL:
        // Deleted by uninstalling.
        break;
    case DBOX_DESTROY_TYPE_TERMINATE:
        // Deleted by power off (or homescreen is killed??).
        break;
    case DBOX_DESTROY_TYPE_FAULT:
        // Delete by fault. does your widget have any problems?
        break;
    case DBOX_DESTROY_TYPE_TEMPORARY:
        // Will be created again.
        break;
    case DBOX_DESTROY_TYPE_UNKNOWN:
    default:
        // System error or unsupported event type.
        break;
    }

    free(dbox_data);

    return LB_STATUS_SUCCESS;
}
@endcode

You should manage the data if it is stored in the non-volatile storage.
Or your data will be remained forever before user uninstalls your package from the device.

<H2>4.4 Periodic updator</H2>
You may need to set update period in your package manifest(XML) file.
Then the provider will parses it and set the period for periodic updator.

The provider will call update() callback of yours when the update timer is expired.
And it will be re-armed automatically.

But if your box is paused, the provider will not call it anymore.
It will be called only if your box is resumed.

@code
static int dbox_update(const char *id, int reason, void *data)
{
}
@endcode

<H2>4.5 Glance Bar</H2>
Glance Bar is a extra contents. it will help user to access more detail information without launching applications.
But this is only supported for the high-end devices. this is not mandatory service.
So you can skip this section.

@code
@endcode

 */

#endif /* __DBOX_PROVIDER_APP_DOC_H__ */
