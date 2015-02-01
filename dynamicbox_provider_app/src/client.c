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

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>

#include <dynamicbox_errno.h>
#include <dynamicbox_service.h>
#include <dynamicbox_conf.h>

#include <app.h>
#include <dlog.h>

#include <Ecore.h>
#include <Eina.h>

#include <packet.h>
#include <com-core_packet.h>

#include "dynamicbox_provider.h"
#include "dynamicbox_provider_buffer.h"
#include "debug.h"
#include "dynamicbox_provider_app.h"
#include "dynamicbox_provider_app_internal.h"
#include "client.h"
#include "util.h"
#include "connection.h"

int errno;

struct item {
    char *id;
    void *update_timer;
    struct dynamicbox_provider_event_callback *table;
    int paused;
    void *data;
    struct connection *handle;
};

static struct info {
    int secured;
    char *abi;
    char *name;
    char *hw_acceleration;
    Ecore_Timer *ping_timer;
    Eina_List *inst_list;
    int paused;
    int initialized;
} s_info = {
    .secured = 0,
    .abi = NULL,
    .name = NULL,
    .hw_acceleration = NULL,
    .ping_timer = NULL,
    .inst_list = NULL,
    .paused = 0,
    .initialized = 0,
};

static Eina_Bool periodic_updator(void *data)
{
    struct item *item = data;

    if (item->table->update) {
	int ret;
	ret = item->table->update(item->id, NULL, 0, item->table->data);
	if (ret < 0) {
	    ErrPrint("Provider update: [%s] returns 0x%X\n", item->id, ret);
	}
    }

    return ECORE_CALLBACK_RENEW;
}

static struct method s_table[] = {
    {
	.cmd = NULL,
	.handler = NULL,
    },
};

static struct item *instance_create(const char *id, double period, struct dynamicbox_provider_event_callback *table, const char *direct_addr)
{
    struct item *item;

    item = calloc(1, sizeof(*item));
    if (!item) {
	ErrPrint("Heap: %s\n", strerror(errno));
	return NULL;
    }

    item->id = strdup(id);
    if (!item->id) {
	ErrPrint("Heap: %s\n", strerror(errno));
	free(item);
	return NULL;
    }

    /*
     * If the "secured" flag is toggled,
     * The master will send the update event to this provider app.
     */
    if (!s_info.secured && period > 0.0f) {
	item->update_timer = util_timer_add(period, periodic_updator, item);
	if (!item->update_timer) {
	    ErrPrint("Failed to create a timer\n");
	    free(item->id);
	    free(item);
	    return NULL;
	}
    }

    item->table = table;
    if (direct_addr) {
	item->handle = connection_find_by_addr(direct_addr);
	if (!item->handle) {
	    item->handle = connection_create(direct_addr, (void *)s_table);
	    if (!item->handle) {
		ErrPrint("Failed to create a new connection\n");
	    }
	} else {
	    item->handle = connection_ref(item->handle);
	}
    }

    s_info.inst_list = eina_list_append(s_info.inst_list, item);

    return item;
}

static void instance_destroy(struct item *item)
{
    s_info.inst_list = eina_list_remove(s_info.inst_list, item);
    if (item->update_timer) {
	util_timer_del(item->update_timer);
    }
    free(item->id);
    item->handle = connection_unref(item->handle);
    free(item);
}

static struct item *instance_find(const char *id)
{
    Eina_List *l;
    struct item *item;

    EINA_LIST_FOREACH(s_info.inst_list, l, item) {
	if (!strcmp(item->id, id)) {
	    return item;
	}
    }

    return NULL;
}

static int method_new(struct dynamicbox_event_arg *arg, int *width, int *height, double *priority, void *data)
{
    struct dynamicbox_provider_event_callback *table = data;
    struct item *inst;
    int ret;

    DbgPrint("Create: pkgname[%s], id[%s], content[%s], timeout[%d], has_script[%d], period[%lf], cluster[%s], category[%s], skip[%d], abi[%s], size: %dx%d\n",
	    arg->pkgname, arg->id,
	    arg->info.dbox_create.content,
	    arg->info.dbox_create.timeout,
	    arg->info.dbox_create.has_script,
	    arg->info.dbox_create.period,
	    arg->info.dbox_create.cluster, arg->info.dbox_create.category,
	    arg->info.dbox_create.skip_need_to_create,
	    arg->info.dbox_create.abi,
	    arg->info.dbox_create.width,
	    arg->info.dbox_create.height);

    if (!table->dbox.create) {
	ErrPrint("Function is not implemented\n");
	return DBOX_STATUS_ERROR_NOT_IMPLEMENTED;
    }

    inst = instance_create(arg->id, arg->info.dbox_create.period, table, arg->info.dbox_create.direct_addr);
    if (!inst) {
	return DBOX_STATUS_ERROR_FAULT;
    }

    ret = table->dbox.create(util_uri_to_path(arg->id), arg->info.dbox_create.content, arg->info.dbox_create.width, arg->info.dbox_create.height, table->data);
    if (ret != DBOX_STATUS_ERROR_NONE) {
	ErrPrint("Failed to create an instance\n");
	instance_destroy(inst);
    } else {
	struct dynamicbox_buffer *buffer;

	buffer = dynamicbox_provider_buffer_find_buffer(DBOX_TYPE_DBOX, arg->pkgname, arg->id);
	if (buffer) {
	    int bps;
	    (void)dynamicbox_provider_buffer_get_size(buffer, width, height, &bps);
	}

	if (s_info.paused && table->pause) {
	    table->pause(util_uri_to_path(arg->id), table->data);
	}
    }

    return ret;
}

static int method_renew(struct dynamicbox_event_arg *arg, void *data)
{
    struct dynamicbox_provider_event_callback *table = data;
    struct item *inst;
    int ret;

    DbgPrint("Re-create: pkgname[%s], id[%s], content[%s], timeout[%d], has_script[%d], period[%lf], cluster[%s], category[%s], abi[%s], size: %dx%d\n",
	    arg->pkgname, arg->id,
	    arg->info.dbox_recreate.content,
	    arg->info.dbox_recreate.timeout,
	    arg->info.dbox_recreate.has_script,
	    arg->info.dbox_recreate.period,
	    arg->info.dbox_recreate.cluster,
	    arg->info.dbox_recreate.category,
	    arg->info.dbox_recreate.abi,
	    arg->info.dbox_recreate.width,
	    arg->info.dbox_recreate.height);

    if (!table->dbox.create) {
	ErrPrint("Function is not implemented\n");
	return DBOX_STATUS_ERROR_NOT_IMPLEMENTED;
    }

    inst = instance_create(arg->id, arg->info.dbox_recreate.period, table, arg->info.dbox_recreate.direct_addr);
    if (!inst) {
	return DBOX_STATUS_ERROR_FAULT;
    }

    ret = table->dbox.create(util_uri_to_path(arg->id), arg->info.dbox_recreate.content, arg->info.dbox_recreate.width, arg->info.dbox_recreate.height, table->data);
    if (ret < 0) {
	instance_destroy(inst);
    } else {
	if (s_info.paused && table->pause) {
	    table->pause(util_uri_to_path(arg->id), table->data);
	}
    }

    return ret;
}

static int method_delete(struct dynamicbox_event_arg *arg, void *data)
{
    struct dynamicbox_provider_event_callback *table = data;
    struct item *inst;
    int ret;

    DbgPrint("pkgname[%s] id[%s]\n", arg->pkgname, arg->id);

    if (!table->dbox.destroy) {
	ErrPrint("Function is not implemented\n");
	return DBOX_STATUS_ERROR_NOT_IMPLEMENTED;
    }

    inst = instance_find(arg->id);
    if (!inst) {
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    ret = table->dbox.destroy(util_uri_to_path(arg->id), arg->info.dbox_destroy.type, table->data);
    instance_destroy(inst);
    return ret;
}

static int method_content_event(struct dynamicbox_event_arg *arg, void *data)
{
    struct dynamicbox_provider_event_callback *table = data;
    int ret;

    if (!table->script_event) {
	ErrPrint("Function is not implemented\n");
	return DBOX_STATUS_ERROR_NOT_IMPLEMENTED;
    }

    ret = table->script_event(util_uri_to_path(arg->id),
	    arg->info.content_event.emission, arg->info.content_event.source,
	    &arg->info.content_event.info,
	    table->data);

    return ret;
}

static int method_clicked(struct dynamicbox_event_arg *arg, void *data)
{
    int ret;
    struct dynamicbox_provider_event_callback *table = data;

    DbgPrint("pkgname[%s] id[%s] event[%s] timestamp[%lf] x[%lf] y[%lf]\n",
	    arg->pkgname, arg->id,
	    arg->info.clicked.event, arg->info.clicked.timestamp,
	    arg->info.clicked.x, arg->info.clicked.y);
    if (!table->clicked) {
	ErrPrint("Function is not implemented\n");
	return DBOX_STATUS_ERROR_NOT_IMPLEMENTED;
    }

    ret = table->clicked(util_uri_to_path(arg->id), arg->info.clicked.event, arg->info.clicked.x, arg->info.clicked.y, arg->info.clicked.timestamp, table->data);

    return ret;
}

static int method_text_signal(struct dynamicbox_event_arg *arg, void *data)
{
    struct dynamicbox_provider_event_callback *table = data;
    int ret;

    if (!table->script_event) {
	ErrPrint("Function is not implemented\n");
	return DBOX_STATUS_ERROR_NOT_IMPLEMENTED;
    }

    ret = table->script_event(util_uri_to_path(arg->id),
	    arg->info.text_signal.emission, arg->info.text_signal.source,
	    &arg->info.text_signal.info, table->data);

    return ret;
}

static int method_resize(struct dynamicbox_event_arg *arg, void *data)
{
    struct dynamicbox_provider_event_callback *table = data;
    int ret;

    if (!table->dbox.resize) {
	ErrPrint("Function is not implemented\n");
	return DBOX_STATUS_ERROR_NOT_IMPLEMENTED;
    }

    DbgPrint("pkgname[%s] id[%s] w[%d] h[%d]\n", arg->pkgname, arg->id, arg->info.resize.w, arg->info.resize.h);
    ret = table->dbox.resize(util_uri_to_path(arg->id), arg->info.resize.w, arg->info.resize.h, table->data);
    return ret;
}

static int method_set_period(struct dynamicbox_event_arg *arg, void *data)
{
    struct item *inst;

    DbgPrint("pkgname[%s] id[%s] period[%lf]\n", arg->pkgname, arg->id, arg->info.set_period.period);
    inst = instance_find(arg->id);
    if (!inst) {
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    if (inst->update_timer) {
	util_timer_interval_set(inst->update_timer, arg->info.set_period.period);
    }

    return DBOX_STATUS_ERROR_NONE;
}

static int method_change_group(struct dynamicbox_event_arg *arg, void *data)
{
    return DBOX_STATUS_ERROR_NOT_IMPLEMENTED;
}

static int method_pinup(struct dynamicbox_event_arg *arg, void *data)
{
    return DBOX_STATUS_ERROR_NOT_IMPLEMENTED;
}

static int method_update_content(struct dynamicbox_event_arg *arg, void *data)
{
    struct dynamicbox_provider_event_callback *table = data;
    int ret;

    if (!table->update) {
	return DBOX_STATUS_ERROR_NOT_IMPLEMENTED;
    }

    ret = table->update(util_uri_to_path(arg->id), arg->info.update_content.content, arg->info.update_content.force, table->data);
    return ret;
}

static int method_pause(struct dynamicbox_event_arg *arg, void *data)
{
    struct dynamicbox_provider_event_callback *table = data;
    Eina_List *l;
    struct item *inst;

    if (s_info.paused == 1) {
	DbgPrint("Already paused\n");
	return DBOX_STATUS_ERROR_NONE;
    }

    s_info.paused = 1;

    EINA_LIST_FOREACH(s_info.inst_list, l, inst) {
	if (inst->paused == 1) {
	    continue;
	}

	if (inst->update_timer) {
	    util_timer_freeze(inst->update_timer);
	}

	if (table->pause) {
	    table->pause(util_uri_to_path(inst->id), table->data);
	}
    }

    if (s_info.ping_timer) {
	ecore_timer_freeze(s_info.ping_timer);
    }

    return DBOX_STATUS_ERROR_NONE;
}

static int method_resume(struct dynamicbox_event_arg *arg, void *data)
{
    struct dynamicbox_provider_event_callback *table = data;
    Eina_List *l;
    struct item *inst;

    if (s_info.paused == 0) {
	DbgPrint("Already resumed\n");
	return DBOX_STATUS_ERROR_NONE;
    }

    s_info.paused = 0;

    EINA_LIST_FOREACH(s_info.inst_list, l, inst) {
	if (inst->paused == 1) {
	    continue;
	}

	if (inst->update_timer) {
	    util_timer_thaw(inst->update_timer);
	}

	if (table->resume) {
	    table->resume(util_uri_to_path(inst->id), table->data);
	}
    }

    if (s_info.ping_timer) {
	ecore_timer_thaw(s_info.ping_timer);
    }

    return DBOX_STATUS_ERROR_NONE;
}

static Eina_Bool send_ping_cb(void *data)
{
    dynamicbox_provider_send_ping();

    return ECORE_CALLBACK_RENEW;
}

static int method_disconnected(struct dynamicbox_event_arg *arg, void *data)
{
    struct dynamicbox_provider_event_callback *table = data;
    struct item *item;
    Eina_List *l;
    Eina_List *n;

    /* arg == NULL */

    if (s_info.ping_timer) {
	ecore_timer_del(s_info.ping_timer);
	s_info.ping_timer = NULL;
    }

    if (table->disconnected) {
	table->disconnected(table->data);
    }

    client_fini();

    /**
     * After clean up all connections to master,
     * invoke destroy callback for every instances.
     */
    EINA_LIST_FOREACH_SAFE(s_info.inst_list, l, n, item) {
	if (table->dbox.destroy) {
	    table->dbox.destroy(util_uri_to_path(item->id), DBOX_DESTROY_TYPE_FAULT, item->table->data);
	}

	/**
	 * instance_destroy will remove the "item" from this "inst_list"
	 */
	instance_destroy(item);
    }

    return DBOX_STATUS_ERROR_NONE;
}

static int method_connected(struct dynamicbox_event_arg *arg, void *data)
{
    int ret;
    struct dynamicbox_provider_event_callback *table = data;

    ret = dynamicbox_provider_send_hello();
    if (ret == 0) {
	s_info.ping_timer = ecore_timer_add(DYNAMICBOX_CONF_DEFAULT_PING_TIME, send_ping_cb, NULL);
	if (!s_info.ping_timer) {
	    ErrPrint("Failed to add a ping timer\n");
	}
    }

    if (table->connected) {
	table->connected(table->data);
    }

    return DBOX_STATUS_ERROR_NONE;
}

static int method_gbar_created(struct dynamicbox_event_arg *arg, void *data)
{
    struct dynamicbox_provider_event_callback *table = data;
    int ret;

    if (!table->gbar.create) {
	return DBOX_STATUS_ERROR_NOT_IMPLEMENTED;
    }

    ret = table->gbar.create(util_uri_to_path(arg->id), arg->info.gbar_create.w, arg->info.gbar_create.h, arg->info.gbar_create.x, arg->info.gbar_create.y, table->data);
    return ret;
}

static int method_gbar_destroyed(struct dynamicbox_event_arg *arg, void *data)
{
    struct dynamicbox_provider_event_callback *table = data;
    int ret;

    if (!table->gbar.destroy) {
	return DBOX_STATUS_ERROR_NOT_IMPLEMENTED;
    }

    ret = table->gbar.destroy(util_uri_to_path(arg->id), arg->info.gbar_destroy.reason, table->data);
    return ret;
}

static int method_gbar_moved(struct dynamicbox_event_arg *arg, void *data)
{
    struct dynamicbox_provider_event_callback *table = data;
    int ret;

    if (!table->gbar.resize_move) {
	return DBOX_STATUS_ERROR_NOT_IMPLEMENTED;
    }

    ret = table->gbar.resize_move(util_uri_to_path(arg->id), arg->info.gbar_move.w, arg->info.gbar_move.h, arg->info.gbar_move.x, arg->info.gbar_move.y, table->data);
    return ret;
}

static int method_dbox_pause(struct dynamicbox_event_arg *arg, void *data)
{
    struct dynamicbox_provider_event_callback *table = data;
    int ret;
    struct item *inst;

    inst = instance_find(arg->id);
    if (!inst) {
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    if (inst->paused == 1) {
	DbgPrint("Already paused\n");
	return DBOX_STATUS_ERROR_ALREADY;
    }

    inst->paused = 1;

    if (s_info.paused == 1) {
	return DBOX_STATUS_ERROR_NONE;
    }

    if (inst->update_timer) {
	util_timer_freeze(inst->update_timer);
    }

    if (!table->pause) {
	return DBOX_STATUS_ERROR_NOT_IMPLEMENTED;
    }

    ret = table->pause(util_uri_to_path(arg->id), table->data);
    return ret;
}

static int method_dbox_resume(struct dynamicbox_event_arg *arg, void *data)
{
    struct dynamicbox_provider_event_callback *table = data;
    struct item *inst;
    int ret;

    inst = instance_find(arg->id);
    if (!inst) {
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    if (inst->paused == 0) {
	return DBOX_STATUS_ERROR_ALREADY;
    }

    inst->paused = 0;

    if (s_info.paused == 1) {
	return DBOX_STATUS_ERROR_NONE;
    }

    if (inst->update_timer) {
	util_timer_thaw(inst->update_timer);
    }

    if (!table->resume) {
	return DBOX_STATUS_ERROR_NOT_IMPLEMENTED;
    }

    ret = table->resume(util_uri_to_path(arg->id), table->data);
    return ret;
}

static int connection_disconnected_cb(int handle, void *data)
{
    struct connection *connection;
    struct item *inst;
    Eina_List *l;

    connection = connection_find_by_fd(handle);
    if (!connection) {
	return 0;
    }

    EINA_LIST_FOREACH(s_info.inst_list, l, inst) {
	if (inst->handle == connection) {
	    /**
	     * @note
	     * This instance has connection to client
	     * but now it is lost.
	     * by reset "handle",
	     * the provider_app_send_updated function will send event to the master.
	     */
	    DbgPrint("Disconnected: %s\n", inst->id);

	    /**
	     * To prevent from nested callback call.
	     * reset handle first.
	     */
	    inst->handle = NULL;

	    (void)connection_unref(connection);
	}
    }

    return 0;
}

void client_fini(void)
{
    if (!s_info.initialized) {
	LOGE("Provider is not initialized\n");
	return;
    }

    DbgPrint("Finalize the Provider App Connection\n");
    s_info.initialized = 0;

    dynamicbox_provider_fini();

    DbgPrint("Provider is disconnected(%s)\n", s_info.abi);
    free(s_info.name);
    free(s_info.abi);
    free(s_info.hw_acceleration);

    s_info.name = NULL;
    s_info.abi = NULL;
    s_info.hw_acceleration = NULL;

    connection_del_event_handler(CONNECTION_EVENT_TYPE_DISCONNECTED, connection_disconnected_cb);
}

int client_is_initialized(void)
{
    return s_info.initialized;
}

int client_init(app_control_h service, struct dynamicbox_provider_event_callback *table)
{
    int ret;
    char *secured = NULL;
    char *name = NULL;
    char *abi = NULL;
    char *hw_acceleration = NULL;
    struct dynamicbox_event_table method_table = {
	.dbox_create = method_new,
	.dbox_recreate = method_renew,
	.dbox_destroy = method_delete,
	.gbar_create = method_gbar_created,
	.gbar_destroy = method_gbar_destroyed,
	.gbar_move = method_gbar_moved,
	.resize = method_resize,
	.update_content = method_update_content,
	.content_event = method_content_event,
	.clicked = method_clicked,
	.text_signal = method_text_signal,
	.set_period = method_set_period,
	.change_group = method_change_group,
	.pinup = method_pinup,
	.pause = method_pause,
	.resume = method_resume,
	.dbox_pause = method_dbox_pause,
	.dbox_resume = method_dbox_resume,
	.disconnected = method_disconnected,
	.connected = method_connected,
    };

    if (s_info.initialized == 1) {
	DbgPrint("Provider App is already initialized\n");
	return DBOX_STATUS_ERROR_NONE;
    }

    if (s_info.name && s_info.abi) {
	DbgPrint("Name and ABI is assigned already\n");
	return DBOX_STATUS_ERROR_NONE;
    }

    if (!dynamicbox_conf_is_loaded()) {
	dynamicbox_conf_reset();
	if (dynamicbox_conf_load() < 0) {
	    ErrPrint("Failed to load conf\n");
	}
    }

    ret = app_control_get_extra_data(service, DYNAMICBOX_CONF_BUNDLE_SLAVE_NAME, &name);
    if (ret != APP_CONTROL_ERROR_NONE) {
	ErrPrint("Name is not valid\n");
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    ret = app_control_get_extra_data(service, DYNAMICBOX_CONF_BUNDLE_SLAVE_SECURED, &secured);
    if (ret != APP_CONTROL_ERROR_NONE) {
	free(name);
	ErrPrint("Secured is not valid\n");
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    ret = app_control_get_extra_data(service, DYNAMICBOX_CONF_BUNDLE_SLAVE_ABI, &abi);
    if (ret != APP_CONTROL_ERROR_NONE) {
	free(name);
	free(secured);
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    ret = app_control_get_extra_data(service, DYNAMICBOX_CONF_BUNDLE_SLAVE_HW_ACCELERATION, &hw_acceleration);
    if (ret != APP_CONTROL_ERROR_NONE) {
	DbgPrint("hw-acceleration is not set\n");
    }

    if (name && abi && secured) {
	DbgPrint("Name assigned: %s (%s)\n", name, abi);
	DbgPrint("Secured: %s\n", s_info.secured);
	DbgPrint("hw-acceleration: %s\n", hw_acceleration);

	ret = dynamicbox_provider_init(util_screen_get(), name, &method_table, table, 1, 1);
	if (ret < 0) {
	    free(name);
	    free(abi);
	} else {
	    s_info.initialized = 1;
	    s_info.name = name;
	    s_info.abi = abi;
	    s_info.secured = !strcasecmp(secured, "true");
	    s_info.hw_acceleration = hw_acceleration;

	    if (connection_add_event_handler(CONNECTION_EVENT_TYPE_DISCONNECTED, connection_disconnected_cb, NULL) < 0) {
		ErrPrint("Failed to add a disconnected event callback\n");
	    }
	}
	free(secured);
    } else {
	free(name);
	free(abi);
	free(secured);
	free(hw_acceleration);
    }

    return ret;
}

PAPI int dynamicbox_provider_app_updated(const char *id, int idx, int x, int y, int w, int h, int for_gbar)
{
    struct item *inst;
    char *uri;
    int ret = DBOX_STATUS_ERROR_DISABLED;
    char *desc_name = NULL;
    dynamicbox_damage_region_t region = {
	.x = x,
	.y = y,
	.w = w,
	.h = h,
    };

    if (!id) {
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    uri = util_path_to_uri(id);
    if (!uri) {
	return DBOX_STATUS_ERROR_OUT_OF_MEMORY;
    }

    if (for_gbar) {
	int len;

	len = strlen(id) + strlen(".desc") + 3;
	desc_name = malloc(len);
	if (!len) {
	    free(uri);
	    return DBOX_STATUS_ERROR_OUT_OF_MEMORY;
	}
	snprintf(desc_name, len, "%s.desc", id);
    }

    /**
     * Do we have to search instance in this function?
     * This function is very frequently called one.
     * So we have to do not heavy operation in here!!!
     * Keep it in mind.
     */
    inst = instance_find(uri);
    if (inst && inst->handle) {
	ret = dynamicbox_provider_send_direct_updated(connection_handle(inst->handle), provider_pkgname(), uri, idx, &region, for_gbar, desc_name);
    }

    /**
     * by this logic,
     * Even if we lost direct connection to the viewer,
     * we will send this to the master again.
     */
    if (ret != DBOX_STATUS_ERROR_NONE) {
	ret = dynamicbox_provider_send_updated(provider_pkgname(), uri, idx, &region, for_gbar, desc_name);
    }

    free(desc_name);
    free(uri);

    return ret;
}

PAPI int dynamicbox_provider_app_buffer_updated(dynamicbox_buffer_h handle, int idx, int x, int y, int w, int h, int for_gbar)
{
    struct item *inst;
    int ret = DBOX_STATUS_ERROR_DISABLED;
    char *desc_name = NULL;
    const char *uri;
    dynamicbox_damage_region_t region = {
	.x = x,
	.y = y,
	.w = w,
	.h = h,
    };

    if (!handle) {
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    uri = dynamicbox_provider_buffer_id(handle);
    if (!uri) {
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    if (for_gbar) {
	int len;
	const char *id;

	id = util_uri_to_path(uri);
	if (!id) {
	    return DBOX_STATUS_ERROR_FAULT;
	}

	len = strlen(id) + strlen(".desc") + 3;
	desc_name = malloc(len);
	if (!len) {
	    return DBOX_STATUS_ERROR_OUT_OF_MEMORY;
	}
	snprintf(desc_name, len, "%s.desc", id);
    }

    /**
     * Do we have to search instance in this function?
     * This function is very frequently called one.
     * So we have to do not heavy operation in here!!!
     * Keep it in mind.
     */
    inst = instance_find(uri);
    if (inst && inst->handle) {
	ret = dynamicbox_provider_send_direct_buffer_updated(connection_handle(inst->handle), handle, idx, &region, for_gbar, desc_name);
    }

    /**
     * by this logic,
     * Even if we lost direct connection to the viewer,
     * we will send this to the master again.
     */
    if (ret != DBOX_STATUS_ERROR_NONE) {
	ret = dynamicbox_provider_send_buffer_updated(handle, idx, &region, for_gbar, desc_name);
    }

    free(desc_name);

    return ret;
}

PAPI int dynamicbox_provider_app_extra_info_updated(const char *id, double priority, const char *content_info, const char *title, const char *icon, const char *name)
{
    char *uri;
    int ret;

    if (!id) {
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    uri = util_path_to_uri(id);
    if (!uri) {
	return DBOX_STATUS_ERROR_OUT_OF_MEMORY;
    }

    ret = dynamicbox_provider_send_extra_info(provider_pkgname(), uri, priority, content_info, title, icon, name);
    free(uri);

    return ret;
}

PAPI int dynamicbox_provider_app_set_data(const char *id, void *data)
{
    struct item *item;
    char *uri;

    if (!id) {
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    uri = util_path_to_uri(id);
    if (!uri) {
	return DBOX_STATUS_ERROR_OUT_OF_MEMORY;
    }

    item = instance_find(uri);
    free(uri);
    if (!item) {
	return DBOX_STATUS_ERROR_EXIST;
    }

    item->data = data;

    return DBOX_STATUS_ERROR_NONE;
}

PAPI void *dynamicbox_provider_app_data(const char *id)
{
    struct item *item;
    char *uri;

    if (!id) {
	return NULL;
    }

    uri = util_path_to_uri(id);
    if (!uri) {
	return NULL;
    }

    item = instance_find(uri);
    free(uri);
    if (!item) {
	return NULL;
    }

    return item->data;
}

PAPI void *dynamicbox_provider_app_data_list(void)
{
    struct item *item;
    Eina_List *return_list = NULL;
    Eina_List *l;

    EINA_LIST_FOREACH(s_info.inst_list, l, item) {
	return_list = eina_list_append(return_list, item->data);
    }

    return return_list;
}

/*!
 * } End of a file
 */
