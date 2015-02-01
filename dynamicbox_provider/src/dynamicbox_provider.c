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
#include <sys/types.h>
#include <sys/shm.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

#include <com-core.h>
#include <packet.h>
#include <com-core_packet.h>

#include <dlog.h>
#include <dynamicbox_errno.h>
#include <dynamicbox_service.h> /* DBOX_ACCESS_STATUS_XXXX */
#include <dynamicbox_cmd_list.h>
#include <dynamicbox_conf.h>
#include <dynamicbox_buffer.h>

#include "dynamicbox_provider.h"
#include "dynamicbox_provider_buffer.h"
#include "provider_buffer_internal.h"
#include "dlist.h"
#include "util.h"
#include "debug.h"
#include "fb.h"
#include "event.h"

#define EAPI __attribute__((visibility("default")))

static struct info {
    int closing_fd;
    int fd;
    char *name;
    struct dynamicbox_event_table table;
    void *data;
    int prevent_overwrite;
} s_info = {
    .closing_fd = 0,
    .fd = -1,
    .name = NULL,
    .data = NULL,
    .prevent_overwrite = 0,
};

#define EAPI __attribute__((visibility("default")))

static double current_time_get(double timestamp)
{
    double ret;

    if (DYNAMICBOX_CONF_USE_GETTIMEOFDAY) {
	struct timeval tv;
	if (gettimeofday(&tv, NULL) < 0) {
	    ErrPrint("gettimeofday: %s\n", strerror(errno));
	    ret = timestamp;
	} else {
	    ret = (double)tv.tv_sec + ((double)tv.tv_usec / 1000000.0f);
	}
    } else {
	ret = timestamp;
    }

    return ret;
}

/* pkgname, id, emission, source, sx, sy, ex, ey, x, y, down, ret */
static struct packet *master_script(pid_t pid, int handle, const struct packet *packet)
{
    struct dynamicbox_event_arg arg;
    int ret;

    ret = packet_get(packet, "ssssddddddi",
	    &arg.pkgname, &arg.id,
	    &arg.info.content_event.emission, &arg.info.content_event.source,
	    &(arg.info.content_event.info.part.sx), &(arg.info.content_event.info.part.sy),
	    &(arg.info.content_event.info.part.ex), &(arg.info.content_event.info.part.ey),
	    &(arg.info.content_event.info.pointer.x), &(arg.info.content_event.info.pointer.y),
	    &(arg.info.content_event.info.pointer.down));
    if (ret != 11) {
	ErrPrint("Parameter is not valid\n");
	goto errout;
    }

    arg.type = DBOX_EVENT_CONTENT_EVENT;

    if (s_info.table.content_event) {
	(void)s_info.table.content_event(&arg, s_info.data);
    }

errout:
    return NULL;
}

/* pkgname, id, event, timestamp, x, y, ret */
static struct packet *master_clicked(pid_t pid, int handle, const struct packet *packet)
{
    int ret;
    struct dynamicbox_event_arg arg;

    ret = packet_get(packet, "sssddd",
	    &arg.pkgname, &arg.id,
	    &arg.info.clicked.event,
	    &arg.info.clicked.timestamp,
	    &arg.info.clicked.x, &arg.info.clicked.y);
    if (ret != 6) {
	ErrPrint("Parameter is not valid\n");
	goto errout;
    }

    arg.type = DBOX_EVENT_CLICKED;

    DbgPrint("Clicked: %s\n", arg.id);
    if (s_info.table.clicked) {
	(void)s_info.table.clicked(&arg, s_info.data);
    }

errout:
    return NULL;
}

/* pkgname, id, emission, source, x, y, ex, ey, ret */
static struct packet *master_text_signal(pid_t pid, int handle, const struct packet *packet)
{
    struct dynamicbox_event_arg arg;
    int ret;

    ret = packet_get(packet, "ssssdddd",
	    &arg.pkgname, &arg.id,
	    &arg.info.text_signal.emission, &arg.info.text_signal.source,
	    &(arg.info.text_signal.info.part.sx), &(arg.info.text_signal.info.part.sy),
	    &(arg.info.text_signal.info.part.ex), &(arg.info.text_signal.info.part.ey));

    if (ret != 8) {
	ErrPrint("Parameter is not valid\n");
	goto errout;
    }

    arg.info.text_signal.info.pointer.x = 0.0f;
    arg.info.text_signal.info.pointer.y = 0.0f;
    arg.info.text_signal.info.pointer.down = 0;
    arg.type = DBOX_EVENT_TEXT_SIGNAL;

    if (s_info.table.text_signal) {
	(void)s_info.table.text_signal(&arg, s_info.data);
    }

errout:
    return NULL;
}

/* pkgname, id, ret */
static struct packet *master_delete(pid_t pid, int handle, const struct packet *packet)
{
    struct packet *result;
    struct dynamicbox_event_arg arg;
    int ret;
    int type;

    ret = packet_get(packet, "ssi", &arg.pkgname, &arg.id, &type);
    if (ret != 3) {
	ErrPrint("Parameter is not valid\n");
	ret = DBOX_STATUS_ERROR_INVALID_PARAMETER;
	goto errout;
    }

    arg.type = DBOX_EVENT_DELETE;
    arg.info.dbox_destroy.type = type;
    DbgPrint("DBOX Deleted, reason(%d)\n", type);

    if (s_info.table.dbox_destroy) {
	ret = s_info.table.dbox_destroy(&arg, s_info.data);
    } else {
	ret = DBOX_STATUS_ERROR_NOT_IMPLEMENTED;
    }

errout:
    result = packet_create_reply(packet, "i", ret);
    return result;
}

/* pkgname, id, w, h, ret */
static struct packet *master_resize(pid_t pid, int handle, const struct packet *packet)
{
    struct dynamicbox_event_arg arg;
    int ret;
    struct packet *result;

    ret = packet_get(packet, "ssii", &arg.pkgname, &arg.id, &arg.info.resize.w, &arg.info.resize.h);
    if (ret != 4) {
	ErrPrint("Parameter is not valid\n");
	ret = DBOX_STATUS_ERROR_INVALID_PARAMETER;
	goto errout;
    }

    arg.type = DBOX_EVENT_RESIZE;

    if (s_info.table.resize) {
	ret = s_info.table.resize(&arg, s_info.data);
    } else {
	ret = DBOX_STATUS_ERROR_NOT_IMPLEMENTED;
    }

errout:
    result = packet_create_reply(packet, "i", ret);
    return result;
}

/* pkgname, id, content, timeout, has_Script, period, cluster, category, pinup, width, height, abi, ret */
static struct packet *master_renew(pid_t pid, int handle, const struct packet *packet)
{
    struct dynamicbox_event_arg arg;
    int ret;
    struct packet *result;
    char *content;
    char *title;

    arg.info.dbox_recreate.out_title = NULL;
    arg.info.dbox_recreate.out_content = NULL;
    arg.info.dbox_recreate.out_is_pinned_up = 0;

    ret = packet_get(packet, "sssiidssiisiis", &arg.pkgname, &arg.id,
	    &arg.info.dbox_recreate.content,
	    &arg.info.dbox_recreate.timeout,
	    &arg.info.dbox_recreate.has_script,
	    &arg.info.dbox_recreate.period,
	    &arg.info.dbox_recreate.cluster, &arg.info.dbox_recreate.category,
	    &arg.info.dbox_recreate.width, &arg.info.dbox_recreate.height,
	    &arg.info.dbox_recreate.abi,
	    &arg.info.dbox_recreate.hold_scroll,
	    &arg.info.dbox_recreate.active_update,
	    &arg.info.dbox_recreate.direct_addr);
    if (ret != 14) {
	ErrPrint("Parameter is not valid\n");
	ret = DBOX_STATUS_ERROR_INVALID_PARAMETER;
	goto errout;
    }

    arg.type = DBOX_EVENT_RENEW;

    if (s_info.table.dbox_recreate) {
	ret = s_info.table.dbox_recreate(&arg, s_info.data);
    } else {
	ret = DBOX_STATUS_ERROR_NOT_IMPLEMENTED;
    }

errout:
    if (arg.info.dbox_recreate.out_content) {
	content = arg.info.dbox_recreate.out_content;
    } else {
	content = "";
    }

    if (arg.info.dbox_recreate.out_title) {
	title = arg.info.dbox_recreate.out_title;
    } else {
	title = "";
    }

    result = packet_create_reply(packet, "issi", ret, content, title, arg.info.dbox_recreate.out_is_pinned_up);
    /*!
     * \note
     * Release.
     */
    free(arg.info.dbox_recreate.out_title);
    free(arg.info.dbox_recreate.out_content);
    arg.info.dbox_recreate.out_title = NULL;
    arg.info.dbox_recreate.out_content = NULL;
    return result;
}

/* pkgname, id, content, timeout, has_script, period, cluster, category, pinup, skip_need_to_create, abi, result, width, height, priority */
static struct packet *master_new(pid_t pid, int handle, const struct packet *packet)
{
    struct dynamicbox_event_arg arg;
    int ret;
    struct packet *result;
    int width = 0;
    int height = 0;
    double priority = 0.0f;
    char *content;
    char *title;

    arg.info.dbox_create.out_content = NULL;
    arg.info.dbox_create.out_title = NULL;
    arg.info.dbox_create.out_is_pinned_up = 0;

    ret = packet_get(packet, "sssiidssisiis", &arg.pkgname, &arg.id,
	    &arg.info.dbox_create.content,
	    &arg.info.dbox_create.timeout,
	    &arg.info.dbox_create.has_script,
	    &arg.info.dbox_create.period,
	    &arg.info.dbox_create.cluster, &arg.info.dbox_create.category,
	    &arg.info.dbox_create.skip_need_to_create,
	    &arg.info.dbox_create.abi,
	    &arg.info.dbox_create.width,
	    &arg.info.dbox_create.height,
	    &arg.info.dbox_create.direct_addr);
    if (ret != 13) {
	ErrPrint("Parameter is not valid\n");
	ret = DBOX_STATUS_ERROR_INVALID_PARAMETER;
	goto errout;
    }

    arg.type = DBOX_EVENT_NEW;

    if (s_info.table.dbox_create) {
	ret = s_info.table.dbox_create(&arg, &width, &height, &priority, s_info.data);
    } else {
	ret = DBOX_STATUS_ERROR_NOT_IMPLEMENTED;
    }

errout:
    if (arg.info.dbox_create.out_content) {
	content = arg.info.dbox_create.out_content;
    } else {
	content = "";
    }

    if (arg.info.dbox_create.out_title) {
	title = arg.info.dbox_create.out_title;
    } else {
	title = "";
    }

    result = packet_create_reply(packet, "iiidssi", ret, width, height, priority, content, title, arg.info.dbox_create.out_is_pinned_up);

    /*!
     * Note:
     * Skip checking the address of out_content, out_title
     */
    free(arg.info.dbox_create.out_content);
    free(arg.info.dbox_create.out_title);
    return result;
}

/* pkgname, id, period, ret */
static struct packet *master_set_period(pid_t pid, int handle, const struct packet *packet)
{
    struct dynamicbox_event_arg arg;
    int ret;
    struct packet *result;

    ret = packet_get(packet, "ssd", &arg.pkgname, &arg.id, &arg.info.set_period.period);
    if (ret != 3) {
	ErrPrint("Parameter is not valid\n");
	ret = DBOX_STATUS_ERROR_INVALID_PARAMETER;
	goto errout;
    }

    arg.type = DBOX_EVENT_SET_PERIOD;

    if (s_info.table.set_period) {
	ret = s_info.table.set_period(&arg, s_info.data);
    } else {
	ret = DBOX_STATUS_ERROR_NOT_IMPLEMENTED;
    }

errout:
    result = packet_create_reply(packet, "i", ret);
    return result;
}

/* pkgname, id, cluster, category, ret */
static struct packet *master_change_group(pid_t pid, int handle, const struct packet *packet)
{
    struct dynamicbox_event_arg arg;
    int ret;
    struct packet *result;

    ret = packet_get(packet, "ssss", &arg.pkgname, &arg.id,
	    &arg.info.change_group.cluster, &arg.info.change_group.category);
    if (ret != 4) {
	ErrPrint("Parameter is not valid\n");
	ret = DBOX_STATUS_ERROR_INVALID_PARAMETER;
	goto errout;
    }

    arg.type = DBOX_EVENT_CHANGE_GROUP;

    if (s_info.table.change_group) {
	ret = s_info.table.change_group(&arg, s_info.data);
    } else {
	ret = DBOX_STATUS_ERROR_NOT_IMPLEMENTED;
    }

errout:
    result = packet_create_reply(packet, "i", ret);
    return result;
}

/* pkgname, id, pinup, ret */
static struct packet *master_pinup(pid_t pid, int handle, const struct packet *packet)
{
    struct dynamicbox_event_arg arg;
    int ret;
    struct packet *result;
    const char *content;

    ret = packet_get(packet, "ssi", &arg.pkgname, &arg.id, &arg.info.pinup.state);
    if (ret != 3) {
	ErrPrint("Parameter is not valid\n");
	ret = DBOX_STATUS_ERROR_INVALID_PARAMETER;
	goto errout;
    }

    arg.type = DBOX_EVENT_PINUP;

    if (s_info.table.pinup) {
	ret = s_info.table.pinup(&arg, s_info.data);
    } else {
	ret = DBOX_STATUS_ERROR_NOT_IMPLEMENTED;
    }

errout:
    content = "default";
    if (ret == 0 && arg.info.pinup.content_info) {
	content = arg.info.pinup.content_info;
    }

    result = packet_create_reply(packet, "is", ret, content);
    if (ret == 0) {
	free(arg.info.pinup.content_info);
    }
    return result;
}

/* pkgname, id, cluster, category, content, ret */
static struct packet *master_update_content(pid_t pid, int handle, const struct packet *packet)
{
    struct dynamicbox_event_arg arg;
    int ret;

    ret = packet_get(packet, "sssssi", &arg.pkgname, &arg.id, &arg.info.update_content.cluster, &arg.info.update_content.category, &arg.info.update_content.content, &arg.info.update_content.force);
    if (ret != 6) {
	ErrPrint("Parameter is not valid\n");
	goto errout;
    }

    arg.type = DBOX_EVENT_UPDATE_CONTENT;

    if (s_info.table.update_content) {
	(void)s_info.table.update_content(&arg, s_info.data);
    }

errout:
    return NULL;
}

static struct packet *master_dbox_pause(pid_t pid, int handle, const struct packet *packet)
{
    struct dynamicbox_event_arg arg;
    int ret;

    ret = packet_get(packet, "ss", &arg.pkgname, &arg.id);
    if (ret != 2) {
	ErrPrint("Invalid parameter\n");
	return NULL;
    }

    arg.type = DBOX_EVENT_DBOX_PAUSE;

    if (s_info.table.dbox_pause) {
	(void)s_info.table.dbox_pause(&arg, s_info.data);
    }

    return NULL;
}

static struct packet *master_dbox_resume(pid_t pid, int handle, const struct packet *packet)
{
    struct dynamicbox_event_arg arg;
    int ret;

    ret = packet_get(packet, "ss", &arg.pkgname, &arg.id);
    if (ret != 2) {
	ErrPrint("Invalid parameter\n");
	return NULL;
    }

    arg.type = DBOX_EVENT_DBOX_RESUME;

    if (s_info.table.dbox_resume) {
	(void)s_info.table.dbox_resume(&arg, s_info.data);
    }

    return NULL;
}

/* timestamp, ret */
static struct packet *master_pause(pid_t pid, int handle, const struct packet *packet)
{
    struct dynamicbox_event_arg arg;
    struct packet *result;
    int ret;

    ret = packet_get(packet, "d", &arg.info.pause.timestamp);
    if (ret != 1) {
	ErrPrint("Parameter is not valid\n");
	ret = DBOX_STATUS_ERROR_INVALID_PARAMETER;
	goto errout;
    }
    arg.pkgname = NULL;
    arg.id = NULL;
    arg.type = DBOX_EVENT_PAUSE;

    if (s_info.table.pause) {
	ret = s_info.table.pause(&arg, s_info.data);
    } else {
	ret = DBOX_STATUS_ERROR_NOT_IMPLEMENTED;
    }

errout:
    result = packet_create_reply(packet, "i", ret);
    return result;
}

static struct packet *master_update_mode(pid_t pid, int handle, const struct packet *packet)
{
    struct packet *result;
    struct dynamicbox_event_arg arg;
    int ret;

    ret = packet_get(packet, "ssi", &arg.pkgname, &arg.id, &arg.info.update_mode.active_update);
    if (ret != 3) {
	ErrPrint("Invalid parameter\n");
	ret = DBOX_STATUS_ERROR_INVALID_PARAMETER;
	goto errout;
    }

    if (s_info.table.update_mode) {
	ret = s_info.table.update_mode(&arg, s_info.data);
    } else {
	ret = DBOX_STATUS_ERROR_NOT_IMPLEMENTED;
    }

errout:
    result = packet_create_reply(packet, "i", ret);
    return result;
}

static struct packet *master_dbox_mouse_set(pid_t pid, int handle, const struct packet *packet)
{
    const char *pkgname;
    double timestamp;
    const char *id;
    int ret;
    int x;
    int y;
    int fd;

    ret = packet_get(packet, "ssdii", &pkgname, &id, &timestamp, &x, &y);
    if (ret != 5) {
	ErrPrint("Parameter is not matched\n");
	ret = DBOX_STATUS_ERROR_INVALID_PARAMETER;
	goto out;
    }

    fd = packet_fd(packet);
    DbgPrint("FD: %d\n", fd);
    if (fd >= 0 || event_input_fd() >= 0) {
	dynamicbox_buffer_h buffer_handler;

	buffer_handler = dynamicbox_provider_buffer_find_buffer(DBOX_TYPE_DBOX, pkgname, id);
	if (!buffer_handler) {
	    if (close(fd) < 0) {
		ErrPrint("close: %s\n", strerror(errno));
	    }

	    ErrPrint("Unable to find a buffer handler: %s\n", id);
	    return NULL;
	}

	ret = event_add_object(fd, buffer_handler, current_time_get(timestamp), x, y);
    }

out:
    return NULL;
}

static struct packet *master_dbox_mouse_unset(pid_t pid, int handle, const struct packet *packet)
{
    dynamicbox_buffer_h buffer_handler;
    const char *pkgname;
    double timestamp;
    const char *id;
    int ret;
    int x;
    int y;

    ret = packet_get(packet, "ssdii", &pkgname, &id, &timestamp, &x, &y);
    if (ret != 5) {
	ErrPrint("Parameter is not matched\n");
	ret = DBOX_STATUS_ERROR_INVALID_PARAMETER;
	goto out;
    }

    buffer_handler = dynamicbox_provider_buffer_find_buffer(DBOX_TYPE_DBOX, pkgname, id);
    if (buffer_handler) {
	ret = event_remove_object(buffer_handler);
    }

out:
    return NULL;
}

static struct packet *master_gbar_mouse_set(pid_t pid, int handle, const struct packet *packet)
{
    const char *pkgname;
    double timestamp;
    const char *id;
    int ret;
    int x;
    int y;
    int fd;

    ret = packet_get(packet, "ssdii", &pkgname, &id, &timestamp, &x, &y);
    if (ret != 5) {
	ErrPrint("Parameter is not matched\n");
	ret = DBOX_STATUS_ERROR_INVALID_PARAMETER;
	goto out;
    }

    fd = packet_fd(packet);
    DbgPrint("FD: %d\n", fd);
    if (fd >= 0 || event_input_fd() >= 0) {
	dynamicbox_buffer_h buffer_handler;

	buffer_handler = dynamicbox_provider_buffer_find_buffer(DBOX_TYPE_GBAR, pkgname, id);
	if (!buffer_handler) {
	    if (close(fd) < 0) {
		ErrPrint("close: %s\n", strerror(errno));
	    }

	    ErrPrint("Unable to find a buffer handler: %s\n", id);
	    return NULL;
	}

	ret = event_add_object(fd, buffer_handler, current_time_get(timestamp), x, y);
    }

out:
    return NULL;
}

static struct packet *master_gbar_mouse_unset(pid_t pid, int handle, const struct packet *packet)
{
    dynamicbox_buffer_h buffer_handler;
    const char *pkgname;
    double timestamp;
    const char *id;
    int ret;
    int x;
    int y;

    ret = packet_get(packet, "ssdii", &pkgname, &id, &timestamp, &x, &y);
    if (ret != 5) {
	ErrPrint("Parameter is not matched\n");
	ret = DBOX_STATUS_ERROR_INVALID_PARAMETER;
	goto out;
    }

    buffer_handler = dynamicbox_provider_buffer_find_buffer(DBOX_TYPE_GBAR, pkgname, id);
    if (buffer_handler) {
	ret = event_remove_object(buffer_handler);
    }

out:
    return NULL;
}

/* timestamp, ret */
static struct packet *master_resume(pid_t pid, int handle, const struct packet *packet)
{
    struct packet *result;
    struct dynamicbox_event_arg arg;
    int ret;

    ret = packet_get(packet, "d", &arg.info.resume.timestamp);
    if (ret != 1) {
	ErrPrint("Parameter is not valid\n");
	ret = DBOX_STATUS_ERROR_INVALID_PARAMETER;
	goto errout;
    }

    arg.pkgname = NULL;
    arg.id = NULL;
    arg.type = DBOX_EVENT_RESUME;

    if (s_info.table.resume) {
	ret = s_info.table.resume(&arg, s_info.data);
    } else {
	ret = DBOX_STATUS_ERROR_NOT_IMPLEMENTED;
    }

errout:
    result = packet_create_reply(packet, "i", ret);
    if (!result) {
	ErrPrint("Failed to create result packet\n");
    }
    return result;
}

struct packet *master_gbar_create(pid_t pid, int handle, const struct packet *packet)
{
    struct dynamicbox_event_arg arg;
    int ret;

    ret = packet_get(packet, "ssiidd", &arg.pkgname, &arg.id, &arg.info.gbar_create.w, &arg.info.gbar_create.h, &arg.info.gbar_create.x, &arg.info.gbar_create.y);
    if (ret != 6) {
	ErrPrint("Invalid packet\n");
	goto out;
    }

    arg.type = DBOX_EVENT_GBAR_CREATE;

    DbgPrint("PERF_DBOX\n");
    if (s_info.table.gbar_create) {
	(void)s_info.table.gbar_create(&arg, s_info.data);
    }

out:
    return NULL;
}

struct packet *master_disconnect(pid_t pid, int handle, const struct packet *packet)
{
    double timestamp;
    int ret;

    ret = packet_get(packet, "d", &timestamp);
    if (ret != 1) {
	ErrPrint("Invalid packet\n");
	goto errout;
    }

    if (s_info.fd >= 0 && s_info.closing_fd == 0) {
	s_info.closing_fd = 1;
	com_core_packet_client_fini(s_info.fd);
	s_info.fd = -1;
	s_info.closing_fd = 0;
    }

errout:
    return NULL;
}

struct packet *master_gbar_move(pid_t pid, int handle, const struct packet *packet)
{
    struct dynamicbox_event_arg arg;
    int ret;

    ret = packet_get(packet, "ssiidd", &arg.pkgname, &arg.id, &arg.info.gbar_move.w, &arg.info.gbar_move.h, &arg.info.gbar_move.x, &arg.info.gbar_move.y);
    if (ret != 6) {
	ErrPrint("Invalid packet\n");
	goto out;
    }

    arg.type = DBOX_EVENT_GBAR_MOVE;

    if (s_info.table.gbar_move) {
	(void)s_info.table.gbar_move(&arg, s_info.data);
    }

out:
    return NULL;
}

struct packet *master_gbar_destroy(pid_t pid, int handle, const struct packet *packet)
{
    struct dynamicbox_event_arg arg;
    int ret;

    ret = packet_get(packet, "ssi", &arg.pkgname, &arg.id, &arg.info.gbar_destroy.reason);
    if (ret != 3) {
	ErrPrint("Invalid packet\n");
	goto out;
    }

    arg.type = DBOX_EVENT_GBAR_DESTROY;
    if (s_info.table.gbar_destroy) {
	(void)s_info.table.gbar_destroy(&arg, s_info.data);
    }

out:
    return NULL;
}

static struct method s_table[] = {
    /*!< For the buffer type */
    {
	.cmd = CMD_STR_GBAR_MOUSE_MOVE,
	.handler = provider_buffer_gbar_mouse_move,
    },
    {
	.cmd = CMD_STR_DBOX_MOUSE_MOVE,
	.handler = provider_buffer_dbox_mouse_move,
    },
    {
	.cmd = CMD_STR_GBAR_MOUSE_DOWN,
	.handler = provider_buffer_gbar_mouse_down,
    },
    {
	.cmd = CMD_STR_GBAR_MOUSE_UP,
	.handler = provider_buffer_gbar_mouse_up,
    },
    {
	.cmd = CMD_STR_DBOX_MOUSE_DOWN,
	.handler = provider_buffer_dbox_mouse_down,
    },
    {
	.cmd = CMD_STR_DBOX_MOUSE_UP,
	.handler = provider_buffer_dbox_mouse_up,
    },
    {
	.cmd = CMD_STR_GBAR_MOUSE_ENTER,
	.handler = provider_buffer_gbar_mouse_enter,
    },
    {
	.cmd = CMD_STR_GBAR_MOUSE_LEAVE,
	.handler = provider_buffer_gbar_mouse_leave,
    },
    {
	.cmd = CMD_STR_DBOX_MOUSE_ENTER,
	.handler = provider_buffer_dbox_mouse_enter,
    },
    {
	.cmd = CMD_STR_DBOX_MOUSE_LEAVE,
	.handler = provider_buffer_dbox_mouse_leave,
    },
    {
	.cmd = CMD_STR_DBOX_MOUSE_ON_SCROLL,
	.handler = provider_buffer_dbox_mouse_on_hold,
    },
    {
	.cmd = CMD_STR_DBOX_MOUSE_OFF_SCROLL,
	.handler = provider_buffer_dbox_mouse_off_hold,
    },
    {
	.cmd = CMD_STR_GBAR_MOUSE_ON_SCROLL,
	.handler = provider_buffer_gbar_mouse_on_hold,
    },
    {
	.cmd = CMD_STR_GBAR_MOUSE_OFF_SCROLL,
	.handler = provider_buffer_gbar_mouse_off_hold,
    },
    {
	.cmd = CMD_STR_DBOX_MOUSE_ON_HOLD,
	.handler = provider_buffer_dbox_mouse_on_hold,
    },
    {
	.cmd = CMD_STR_DBOX_MOUSE_OFF_HOLD,
	.handler = provider_buffer_dbox_mouse_off_hold,
    },
    {
	.cmd = CMD_STR_GBAR_MOUSE_ON_HOLD,
	.handler = provider_buffer_gbar_mouse_on_hold,
    },
    {
	.cmd = CMD_STR_GBAR_MOUSE_OFF_HOLD,
	.handler = provider_buffer_gbar_mouse_off_hold,
    },
    {
	.cmd = CMD_STR_CLICKED,
	.handler = master_clicked, /* pkgname, id, event, timestamp, x, y, ret */
    },
    {
	.cmd = CMD_STR_TEXT_SIGNAL,
	.handler = master_text_signal, /* pkgname, id, emission, source, x, y, ex, ey, ret */
    },
    {
	.cmd = CMD_STR_DELETE,
	.handler = master_delete, /* pkgname, id, ret */
    },
    {
	.cmd = CMD_STR_RESIZE,
	.handler = master_resize, /* pkgname, id, w, h, ret */
    },
    {
	.cmd = CMD_STR_NEW,
	.handler = master_new, /* pkgname, id, content, timeout, has_script, period, cluster, category, pinup, skip_need_to_create, abi, result, width, height, priority */
    },
    {
	.cmd = CMD_STR_SET_PERIOD,
	.handler = master_set_period, /* pkgname, id, period, ret */
    },
    {
	.cmd = CMD_STR_CHANGE_GROUP,
	.handler = master_change_group, /* pkgname, id, cluster, category, ret */
    },
    {
	.cmd = CMD_STR_GBAR_MOVE,
	.handler = master_gbar_move,
    },
    {
	.cmd = CMD_STR_GBAR_ACCESS_HL,
	.handler = provider_buffer_gbar_access_hl,
    },
    {
	.cmd = CMD_STR_GBAR_ACCESS_ACTIVATE,
	.handler = provider_buffer_gbar_access_activate,
    },
    {
	.cmd = CMD_STR_GBAR_ACCESS_ACTION,
	.handler = provider_buffer_gbar_access_action,
    },
    {
	.cmd = CMD_STR_GBAR_ACCESS_SCROLL,
	.handler = provider_buffer_gbar_access_scroll,
    },
    {
	.cmd = CMD_STR_GBAR_ACCESS_VALUE_CHANGE,
	.handler = provider_buffer_gbar_access_value_change,
    },
    {
	.cmd = CMD_STR_GBAR_ACCESS_MOUSE,
	.handler = provider_buffer_gbar_access_mouse,
    },
    {
	.cmd = CMD_STR_GBAR_ACCESS_BACK,
	.handler = provider_buffer_gbar_access_back,
    },
    {
	.cmd = CMD_STR_GBAR_ACCESS_OVER,
	.handler = provider_buffer_gbar_access_over,
    },
    {
	.cmd = CMD_STR_GBAR_ACCESS_READ,
	.handler = provider_buffer_gbar_access_read,
    },
    {
	.cmd = CMD_STR_GBAR_ACCESS_ENABLE,
	.handler = provider_buffer_gbar_access_enable,
    },
    {
	.cmd = CMD_STR_DBOX_ACCESS_HL,
	.handler = provider_buffer_dbox_access_hl,
    },
    {
	.cmd = CMD_STR_DBOX_ACCESS_ACTIVATE,
	.handler = provider_buffer_dbox_access_activate,
    },
    {
	.cmd = CMD_STR_DBOX_ACCESS_ACTION,
	.handler = provider_buffer_dbox_access_action,
    },
    {
	.cmd = CMD_STR_DBOX_ACCESS_SCROLL,
	.handler = provider_buffer_dbox_access_scroll,
    },
    {
	.cmd = CMD_STR_DBOX_ACCESS_VALUE_CHANGE,
	.handler = provider_buffer_dbox_access_value_change,
    },
    {
	.cmd = CMD_STR_DBOX_ACCESS_MOUSE,
	.handler = provider_buffer_dbox_access_mouse,
    },
    {
	.cmd = CMD_STR_DBOX_ACCESS_BACK,
	.handler = provider_buffer_dbox_access_back,
    },
    {
	.cmd = CMD_STR_DBOX_ACCESS_OVER,
	.handler = provider_buffer_dbox_access_over,
    },
    {
	.cmd = CMD_STR_DBOX_ACCESS_READ,
	.handler = provider_buffer_dbox_access_read,
    },
    {
	.cmd = CMD_STR_DBOX_ACCESS_ENABLE,
	.handler = provider_buffer_dbox_access_enable,
    },
    {
	.cmd = CMD_STR_DBOX_KEY_DOWN,
	.handler = provider_buffer_dbox_key_down,
    },
    {
	.cmd = CMD_STR_DBOX_KEY_UP,
	.handler = provider_buffer_dbox_key_up,
    },
    {
	.cmd = CMD_STR_DBOX_KEY_FOCUS_IN,
	.handler = provider_buffer_dbox_key_focus_in,
    },
    {
	.cmd = CMD_STR_DBOX_KEY_FOCUS_OUT,
	.handler = provider_buffer_dbox_key_focus_out,
    },
    {
	.cmd = CMD_STR_GBAR_KEY_DOWN,
	.handler = provider_buffer_gbar_key_down,
    },
    {
	.cmd = CMD_STR_GBAR_KEY_UP,
	.handler = provider_buffer_gbar_key_up,
    },
    {
	.cmd = CMD_STR_GBAR_KEY_FOCUS_IN,
	.handler = provider_buffer_dbox_key_focus_in,
    },
    {
	.cmd = CMD_STR_GBAR_KEY_FOCUS_OUT,
	.handler = provider_buffer_dbox_key_focus_out,
    },
    {
	.cmd = CMD_STR_UPDATE_MODE,
	.handler = master_update_mode,
    },
    {
	.cmd = CMD_STR_DBOX_MOUSE_SET,
	.handler = master_dbox_mouse_set,
    },
    {
	.cmd = CMD_STR_DBOX_MOUSE_UNSET,
	.handler = master_dbox_mouse_unset,
    },
    {
	.cmd = CMD_STR_GBAR_MOUSE_SET,
	.handler = master_gbar_mouse_set,
    },
    {
	.cmd = CMD_STR_GBAR_MOUSE_UNSET,
	.handler = master_gbar_mouse_unset,
    },
    {
	.cmd = CMD_STR_GBAR_SHOW,
	.handler = master_gbar_create,
    },
    {
	.cmd = CMD_STR_GBAR_HIDE,
	.handler = master_gbar_destroy,
    },
    {
	.cmd = CMD_STR_DBOX_PAUSE,
	.handler = master_dbox_pause,
    },
    {
	.cmd = CMD_STR_DBOX_RESUME,
	.handler = master_dbox_resume,
    },
    {
	.cmd = CMD_STR_SCRIPT,
	.handler = master_script, /* pkgname, id, emission, source, sx, sy, ex, ey, x, y, down, ret */
    },
    {
	.cmd = CMD_STR_RENEW,
	.handler = master_renew, /* pkgname, id, content, timeout, has_script, period, cluster, category, pinup, width, height, abi, ret */
    },
    {
	.cmd = CMD_STR_PINUP,
	.handler = master_pinup, /* pkgname, id, pinup, ret */
    },
    {
	.cmd = CMD_STR_UPDATE_CONTENT,
	.handler = master_update_content, /* pkgname, cluster, category, ret */
    },
    {
	.cmd = CMD_STR_PAUSE,
	.handler = master_pause, /* timestamp, ret */
    },
    {
	.cmd = CMD_STR_RESUME,
	.handler = master_resume, /* timestamp, ret */
    },
    {
	.cmd = CMD_STR_DISCONNECT,
	.handler = master_disconnect,
    },
    {
	.cmd = NULL,
	.handler = NULL,
    },
};

static int connected_cb(int handle, void *data)
{
    DbgPrint("Connected (%p) %d\n", s_info.table.connected, handle);

    if (s_info.fd >= 0) {
	ErrPrint("Already connected. Ignore this (%d)?\n", handle);
	return 0;
    }

    s_info.fd = handle;

    if (s_info.table.connected) {
	s_info.table.connected(NULL, s_info.data);
    }

    return 0;
}

static int disconnected_cb(int handle, void *data)
{
    if (s_info.fd != handle) {
	DbgPrint("%d is not my favor (%d)\n", handle, s_info.fd);
	return 0;
    }

    DbgPrint("Disconnected (%d)\n", handle);
    if (s_info.table.disconnected) {
	s_info.table.disconnected(NULL, s_info.data);
    }

    /* Reset the FD */
    s_info.fd = -1;
    return 0;
}

static int initialize_provider(void *disp, const char *name, dynamicbox_event_table_h table, void *data)
{
    int ret;

    s_info.name = strdup(name);
    if (!s_info.name) {
	ErrPrint("Heap: %s\n", strerror(errno));
	return DBOX_STATUS_ERROR_OUT_OF_MEMORY;
    }

    memcpy(&s_info.table, table, sizeof(*table));
    s_info.data = data;

    com_core_add_event_callback(CONNECTOR_DISCONNECTED, disconnected_cb, NULL);

    ret = com_core_packet_client_init(SLAVE_SOCKET, 0, s_table);
    if (ret < 0) {
	ErrPrint("Failed to establish the connection with the master\n");
	s_info.data = NULL;
	free(s_info.name);
	s_info.name = NULL;
	return (ret == -EACCES) ? DBOX_STATUS_ERROR_PERMISSION_DENIED : DBOX_STATUS_ERROR_FAULT;
    }

    connected_cb(ret, NULL);
    dynamicbox_provider_buffer_init(disp);

    DbgPrint("Slave is initialized\n");
    return DBOX_STATUS_ERROR_NONE;
}

static char *keep_file_in_safe(const char *id, int uri)
{
    const char *path;
    int len;
    int base_idx;
    char *new_path;

    path = uri ? util_uri_to_path(id) : id;
    if (!path) {
	ErrPrint("Invalid path\n");
	return NULL;
    }

    if (s_info.prevent_overwrite) {
	char *ret;

	ret = strdup(path);
	if (!ret) {
	    ErrPrint("Heap: %s\n", strerror(errno));
	}

	return ret;
    }

    if (access(path, R_OK | F_OK) != 0) {
	ErrPrint("[%s] %s\n", path, strerror(errno));
	return NULL;
    }

    len = strlen(path);
    base_idx = len - 1;

    while (base_idx > 0 && path[base_idx] != '/') base_idx--;
    base_idx += (path[base_idx] == '/');

    new_path = malloc(len + 10 + 30); /* for "tmp" tv_sec, tv_usec */
    if (!new_path) {
	ErrPrint("Heap: %s\n", strerror(errno));
	return NULL;
    }

    strncpy(new_path, path, base_idx);

#if defined(_USE_ECORE_TIME_GET)
    double tval;

    tval = util_timestamp();

    snprintf(new_path + base_idx, len + 10 - base_idx + 30, "reader/%lf.%s", tval, path + base_idx);
#else
    struct timeval tv;
    if (gettimeofday(&tv, NULL) < 0) {
	ErrPrint("gettimeofday: %s\n", strerror(errno));
	tv.tv_sec = rand();
	tv.tv_usec = rand();
    }

    snprintf(new_path + base_idx, len + 10 - base_idx + 30, "reader/%lu.%lu.%s", tv.tv_sec, tv.tv_usec, path + base_idx);
#endif

    /*!
     * To prevent from failing of rename a content file
     */
    (void)unlink(new_path);

    if (rename(path, new_path) < 0) {
	ErrPrint("Failed to keep content in safe: %s (%s -> %s)\n", strerror(errno), path, new_path);
    }

    return new_path;
}

const char *provider_name(void)
{
    return s_info.name;
}

EAPI int dynamicbox_provider_init(void *disp, const char *name, dynamicbox_event_table_h table, void *data, int prevent_overwrite, int com_core_use_thread)
{
    if (!name || !table) {
	ErrPrint("Invalid argument\n");
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    if (s_info.name) {
	ErrPrint("Provider is already initialized\n");
	return DBOX_STATUS_ERROR_ALREADY;
    }

    s_info.prevent_overwrite = prevent_overwrite;
    com_core_packet_use_thread(com_core_use_thread);

    return initialize_provider(disp, name, table, data);
}

EAPI void *dynamicbox_provider_fini(void)
{
    void *ret;
    static int provider_fini_called = 0;

    if (provider_fini_called) {
	ErrPrint("Provider finalize is already called\n");
	return NULL;
    }

    if (!s_info.name) {
	ErrPrint("Connection is not established (or cleared already)\n");
	return NULL;
    }

    provider_fini_called = 1;

    if (s_info.fd >= 0 && s_info.closing_fd == 0) {
	s_info.closing_fd = 1;
	com_core_packet_client_fini(s_info.fd);
	s_info.fd = -1;
	s_info.closing_fd = 0;
    }

    provider_fini_called = 0;

    com_core_del_event_callback(CONNECTOR_DISCONNECTED, disconnected_cb, NULL);

    dynamicbox_provider_buffer_fini();

    free(s_info.name);
    s_info.name = NULL;

    ret = s_info.data;
    s_info.data = NULL;

    return ret;
}

EAPI int dynamicbox_provider_send_call(const char *pkgname, const char *id, const char *funcname)
{
    struct packet *packet;
    unsigned int cmd = CMD_CALL;
    int ret;

    if (!pkgname || !id || !funcname) {
	ErrPrint("Invalid argument\n");
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    packet = packet_create_noack((const char *)&cmd, "sss", pkgname, id, funcname);
    if (!packet) {
	ErrPrint("Failed to create a packet\n");
	return DBOX_STATUS_ERROR_FAULT;
    }

    ret = com_core_packet_send_only(s_info.fd, packet);
    packet_destroy(packet);
    return ret < 0 ? DBOX_STATUS_ERROR_FAULT : DBOX_STATUS_ERROR_NONE;
}

EAPI int dynamicbox_provider_send_ret(const char *pkgname, const char *id, const char *funcname)
{
    struct packet *packet;
    unsigned int cmd = CMD_RET;
    int ret;

    if (!pkgname || !id || !funcname) {
	ErrPrint("Invalid argument\n");
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    packet = packet_create_noack((const char *)&cmd, "sss", pkgname, id, funcname);
    if (!packet) {
	ErrPrint("Failed to create a packet\n");
	return DBOX_STATUS_ERROR_FAULT;
    }

    ret = com_core_packet_send_only(s_info.fd, packet);
    packet_destroy(packet);
    return ret < 0 ? DBOX_STATUS_ERROR_FAULT : DBOX_STATUS_ERROR_NONE;
}

EAPI int dynamicbox_provider_send_faulted(const char *pkgname, const char *id, const char *funcname)
{
    struct packet *packet;
    unsigned int cmd = CMD_FAULTED;
    int ret;

    if (!pkgname || !id || !funcname) {
	ErrPrint("Invalid argument\n");
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    packet = packet_create_noack((const char *)&cmd, "sss", pkgname, id, funcname);
    if (!packet) {
	ErrPrint("Failed to create a packet\n");
	return DBOX_STATUS_ERROR_FAULT;
    }

    ret = com_core_packet_send_only(s_info.fd, packet);
    packet_destroy(packet);
    return ret < 0 ? DBOX_STATUS_ERROR_FAULT : DBOX_STATUS_ERROR_NONE;
}

EAPI int dynamicbox_provider_send_hello(void)
{
    struct packet *packet;
    unsigned int cmd = CMD_HELLO;
    int ret;
    DbgPrint("name[%s]\n", s_info.name);

    if (!s_info.name) {
	ErrPrint("Provider is not initialized\n");
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    if (s_info.fd < 0) {
	ErrPrint("Connection is not established\n");
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    packet = packet_create_noack((const char *)&cmd, "s", s_info.name);
    if (!packet) {
	ErrPrint("Failed to create a packet\n");
	return DBOX_STATUS_ERROR_FAULT;
    }

    ret = com_core_packet_send_only(s_info.fd, packet);
    packet_destroy(packet);
    return ret < 0 ? DBOX_STATUS_ERROR_FAULT : DBOX_STATUS_ERROR_NONE;
}

EAPI int dynamicbox_provider_send_ping(void)
{
    struct packet *packet;
    unsigned int cmd = CMD_PING;
    int ret;

    DbgPrint("name[%s]\n", s_info.name);
    if (!s_info.name) {
	ErrPrint("Provider is not initialized\n");
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    if (s_info.fd < 0) {
	ErrPrint("Connection is not established\n");
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    packet = packet_create_noack((const char *)&cmd, "s", s_info.name);
    if (!packet) {
	ErrPrint("Failed to create a a packet\n");
	return DBOX_STATUS_ERROR_FAULT;
    }

    ret = com_core_packet_send_only(s_info.fd, packet);
    packet_destroy(packet);
    return ret < 0 ? DBOX_STATUS_ERROR_FAULT : DBOX_STATUS_ERROR_NONE;
}

EAPI int dynamicbox_provider_send_dbox_update_begin(const char *pkgname, const char *id, double priority, const char *content_info, const char *title)
{
    struct packet *packet;
    unsigned int cmd = CMD_DBOX_UPDATE_BEGIN;
    int ret;

    if (!pkgname || !id) {
	ErrPrint("Invalid argument\n");
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    if (!content_info) {
	content_info = "";
    }

    if (!title) {
	title = "";
    }

    if (s_info.fd < 0) {
	ErrPrint("Connection is not established\n");
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    packet = packet_create_noack((const char *)&cmd, "ssdss",
	    pkgname, id, priority, content_info, title);
    if (!packet) {
	ErrPrint("Failed to build a packet\n");
	return DBOX_STATUS_ERROR_FAULT;
    }

    ret = com_core_packet_send_only(s_info.fd, packet);
    packet_destroy(packet);

    DbgPrint("[ACTIVE] DBOX BEGIN: %s (%d)\n", id, ret);
    return ret < 0 ? DBOX_STATUS_ERROR_FAULT : DBOX_STATUS_ERROR_NONE;
}

EAPI int dynamicbox_provider_send_dbox_update_end(const char *pkgname, const char *id)
{
    struct packet *packet;
    unsigned int cmd = CMD_DBOX_UPDATE_END;
    int ret;

    if (!pkgname || !id) {
	ErrPrint("Invalid argument\n");
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    if (s_info.fd < 0) {
	ErrPrint("Connection is not established\n");
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    packet = packet_create_noack((const char *)&cmd, "ss", pkgname, id);
    if (!packet) {
	ErrPrint("Failed to build a packet\n");
	return DBOX_STATUS_ERROR_FAULT;
    }

    ret = com_core_packet_send_only(s_info.fd, packet);
    packet_destroy(packet);

    DbgPrint("[ACTIVE] DBOX END: %s (%d)\n", id, ret);
    return ret < 0 ? DBOX_STATUS_ERROR_FAULT : DBOX_STATUS_ERROR_NONE;
}

EAPI int dynamicbox_provider_send_gbar_update_begin(const char *pkgname, const char *id)
{
    struct packet *packet;
    unsigned int cmd = CMD_GBAR_UPDATE_BEGIN;
    int ret;

    if (!pkgname || !id) {
	ErrPrint("Invalid argument\n");
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    if (s_info.fd < 0) {
	ErrPrint("Connection is not established\n");
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    packet = packet_create_noack((const char *)&cmd, "ss", pkgname, id);
    if (!packet) {
	ErrPrint("Failed to build a packet\n");
	return DBOX_STATUS_ERROR_FAULT;
    }

    ret = com_core_packet_send_only(s_info.fd, packet);
    packet_destroy(packet);

    DbgPrint("[ACTIVE] GBAR BEGIN: %s (%d)\n", id, ret);
    return ret < 0 ? DBOX_STATUS_ERROR_FAULT : DBOX_STATUS_ERROR_NONE;
}

EAPI int dynamicbox_provider_send_gbar_update_end(const char *pkgname, const char *id)
{
    struct packet *packet;
    unsigned int cmd = CMD_GBAR_UPDATE_END;
    int ret;

    if (!pkgname || !id) {
	ErrPrint("Invalid argument\n");
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    if (s_info.fd < 0) {
	ErrPrint("Connection is not established\n");
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    packet = packet_create_noack((const char *)&cmd, "ss", pkgname, id);
    if (!packet) {
	ErrPrint("Failed to build a packet\n");
	return DBOX_STATUS_ERROR_FAULT;
    }

    ret = com_core_packet_send_only(s_info.fd, packet);
    packet_destroy(packet);

    DbgPrint("[ACTIVE] GBAR END: %s (%d)\n", id, ret);
    return ret < 0 ? DBOX_STATUS_ERROR_FAULT : DBOX_STATUS_ERROR_NONE;
}

EAPI int dynamicbox_provider_send_extra_info(const char *pkgname, const char *id, double priority, const char *content_info, const char *title, const char *icon, const char *name)
{
    struct packet *packet;
    unsigned int cmd = CMD_EXTRA_INFO;
    int ret;

    if (!pkgname || !id) {
	ErrPrint("Invalid argument\n");
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    if (s_info.fd < 0) {
	ErrPrint("Connection is not established\n");
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    packet = packet_create_noack((const char *)&cmd, "ssssssd", pkgname, id, content_info, title, icon, name, priority);
    if (!packet) {
	ErrPrint("failed to build a packet\n");
	return DBOX_STATUS_ERROR_FAULT;
    }

    ret = com_core_packet_send_only(s_info.fd, packet);
    packet_destroy(packet);
    return ret < 0 ? DBOX_STATUS_ERROR_FAULT : DBOX_STATUS_ERROR_NONE;
}

__attribute__((always_inline)) static inline int send_extra_buffer_updated(int fd, const char *pkgname, const char *id, dynamicbox_buffer_h info, int idx, dynamicbox_damage_region_t *region, int direct, int is_gbar)
{
    int ret;
    struct packet *packet;
    unsigned int cmd;

    if (info->extra_buffer[idx] == 0u) {
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    fb_sync_xdamage(info->fb, region);

    cmd = CMD_EXTRA_UPDATED;

    if (direct) {
	packet = packet_create_noack((const char *)&cmd, "ssiiiiii", pkgname, id, is_gbar, idx, region->x, region->y, region->w, region->h);
    } else {
	packet = packet_create_noack((const char *)&cmd, "ssiiiiii", pkgname, id, is_gbar, idx, region->x, region->y, region->w, region->h);
    }

    if (!packet) {
	ErrPrint("failed to build a packet\n");
	return DBOX_STATUS_ERROR_FAULT;
    }

    ret = com_core_packet_send_only(fd, packet);
    packet_destroy(packet);

    return ret;
}

__attribute__((always_inline)) static inline int send_extra_updated(int fd, const char *pkgname, const char *id, int idx, dynamicbox_damage_region_t *region, int direct, int is_gbar)
{
    dynamicbox_buffer_h info;
    dynamicbox_damage_region_t _region = {
	.x = 0,
	.y = 0,
	.w = 0,
	.h = 0,
    };
    int ret;

    if (!pkgname || !id || is_gbar < 0 || idx < 0 || idx > DYNAMICBOX_CONF_EXTRA_BUFFER_COUNT) {
	ErrPrint("Invalid argument\n");
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    if (fd < 0) {
	ErrPrint("Connection is not established\n");
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    info = dynamicbox_provider_buffer_find_buffer(DBOX_TYPE_DBOX, pkgname, id);
    if (!info) {
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    if (!region) {
	int bpp = 0;
	(void)dynamicbox_provider_buffer_get_size(info, &_region.w, &_region.h, &bpp);
	region = &_region;
    }

    ret = send_extra_buffer_updated(fd, pkgname, id, info, idx, region, direct, is_gbar);
    return ret < 0 ? DBOX_STATUS_ERROR_FAULT : DBOX_STATUS_ERROR_NONE;
}

__attribute__((always_inline)) static inline int send_buffer_updated(int fd, const char *pkgname, const char *id, dynamicbox_buffer_h info, dynamicbox_damage_region_t *region, int direct, int for_gbar, const char *safe_filename)
{
    struct packet *packet;
    unsigned int cmd = for_gbar ? CMD_DESC_UPDATED : CMD_UPDATED;
    int ret;

    if (direct && info && dynamicbox_provider_buffer_uri(info)) {
	fb_sync_xdamage(info->fb, region);
	packet = packet_create_noack((const char *)&cmd, "ssssiiii", pkgname, id, dynamicbox_provider_buffer_uri(info), safe_filename, region->x, region->y, region->w, region->h);
    } else {
	if (info) {
	    fb_sync_xdamage(info->fb, region);
	}
	packet = packet_create_noack((const char *)&cmd, "sssiiii", pkgname, id, safe_filename, region->x, region->y, region->w, region->h);
    }

    if (!packet) {
	ErrPrint("failed to build a packet\n");
	return DBOX_STATUS_ERROR_FAULT;
    }

    ret = com_core_packet_send_only(fd, packet);
    packet_destroy(packet);

    return ret;
}

__attribute__((always_inline)) static inline int send_updated(int fd, const char *pkgname, const char *id, dynamicbox_damage_region_t *region, int direct, int for_gbar, const char *descfile)
{
    dynamicbox_buffer_h info;
    char *safe_filename = NULL;
    dynamicbox_damage_region_t _region = {
	.x = 0,
	.y = 0,
	.w = 0,
	.h = 0,
    };
    int ret;

    if (!pkgname || !id) {
	ErrPrint("Invalid argument\n");
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    if (fd < 0) {
	ErrPrint("Connection is not established\n");
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    if (for_gbar && !descfile) {
	descfile = util_uri_to_path(id); /* In case of the NULL descfilename, use the ID */
    }

    info = dynamicbox_provider_buffer_find_buffer(for_gbar ? DBOX_TYPE_GBAR : DBOX_TYPE_DBOX, pkgname, id);
    if (!info) {
	safe_filename = keep_file_in_safe(for_gbar ? descfile : id, 1);
	if (!safe_filename) {
	    return DBOX_STATUS_ERROR_INVALID_PARAMETER;
	}
    } else {
	if (for_gbar) {
	    safe_filename = strdup(descfile);
	    if (!safe_filename) {
		ErrPrint("Heap: %s\n", strerror(errno));
		return DBOX_STATUS_ERROR_OUT_OF_MEMORY;
	    }
	}
    }

    if (!region) {
	if (info) {
	    int bpp = 0;
	    (void)dynamicbox_provider_buffer_get_size(info, &_region.w, &_region.h, &bpp);
	}
	region = &_region;
    }

    ret = send_buffer_updated(fd, pkgname, id, info, region, direct, for_gbar, safe_filename);
    free(safe_filename);
    return ret < 0 ? DBOX_STATUS_ERROR_FAULT : DBOX_STATUS_ERROR_NONE;
}

EAPI int dynamicbox_provider_send_direct_updated(int fd, const char *pkgname, const char *id, int idx, dynamicbox_damage_region_t *region, int for_gbar, const char *descfile)
{
    int ret;

    if (idx == DBOX_PRIMARY_BUFFER) {
	ret = send_updated(fd, pkgname, id, region, 1, for_gbar, descfile);
    } else {
	ret = send_extra_updated(fd, pkgname, id, idx, region, 1, for_gbar);
    }

    return ret;
}

EAPI int dynamicbox_provider_send_updated(const char *pkgname, const char *id, int idx, dynamicbox_damage_region_t *region, int for_gbar, const char *descfile)
{
    int ret;

    if (idx == DBOX_PRIMARY_BUFFER) {
	ret = send_updated(s_info.fd, pkgname, id, region, 0, for_gbar, descfile);
    } else {
	ret = send_extra_updated(s_info.fd, pkgname, id, idx, region, 0, for_gbar);
    }

    return ret;
}

EAPI int dynamicbox_provider_send_direct_buffer_updated(int fd, dynamicbox_buffer_h handle, int idx, dynamicbox_damage_region_t *region, int for_gbar, const char *descfile)
{
    int ret;
    const char *pkgname;
    const char *id;

    pkgname = dynamicbox_provider_buffer_pkgname(handle);
    id = dynamicbox_provider_buffer_id(handle);

    if (idx == DBOX_PRIMARY_BUFFER) {
	ret = send_buffer_updated(fd, pkgname, id, handle, region, 1, for_gbar, descfile);
    } else {
	ret = send_extra_buffer_updated(fd, pkgname, id, handle, idx, region, 1, for_gbar);
    }

    return ret;
}

EAPI int dynamicbox_provider_send_buffer_updated(dynamicbox_buffer_h handle, int idx, dynamicbox_damage_region_t *region, int for_gbar, const char *descfile)
{
    int ret;
    const char *pkgname;
    const char *id;

    pkgname = dynamicbox_provider_buffer_pkgname(handle);
    id = dynamicbox_provider_buffer_id(handle);

    if (idx == DBOX_PRIMARY_BUFFER) {
	ret = send_buffer_updated(s_info.fd, pkgname, id, handle, region, 0, for_gbar, descfile);
    } else {
	ret = send_extra_buffer_updated(s_info.fd, pkgname, id, handle, idx, region, 0, for_gbar);
    }

    return ret;
}

EAPI int dynamicbox_provider_send_deleted(const char *pkgname, const char *id)
{
    struct packet *packet;
    unsigned int cmd = CMD_DELETED;
    int ret;

    if (!pkgname || !id) {
	ErrPrint("Invalid arguement\n");
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    if (s_info.fd < 0) {
	ErrPrint("Connection is not established\n");
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    packet = packet_create_noack((const char *)&cmd, "ss", pkgname, id);
    if (!packet) {
	ErrPrint("Failed to build a packet\n");
	return DBOX_STATUS_ERROR_FAULT;
    }

    ret = com_core_packet_send_only(s_info.fd, packet);
    packet_destroy(packet);
    return ret < 0 ? DBOX_STATUS_ERROR_FAULT : DBOX_STATUS_ERROR_NONE;
}

EAPI int dynamicbox_provider_send_hold_scroll(const char *pkgname, const char *id, int hold)
{
    struct packet *packet;
    unsigned int cmd = CMD_SCROLL;
    int ret;

    if (!pkgname || !id) {
	ErrPrint("Invalid argument\n");
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    if (s_info.fd < 0) {
	ErrPrint("Connection is not established\n");
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    packet = packet_create_noack((const char *)&cmd, "ssi", pkgname, id, !!hold);
    if (!packet) {
	ErrPrint("Failed to build a packet\n");
	return DBOX_STATUS_ERROR_FAULT;
    }

    ret = com_core_packet_send_only(s_info.fd, packet);
    packet_destroy(packet);
    DbgPrint("[HOLD] Send hold: %d (%s) ret(%d)\n", hold, id, ret);
    return ret < 0 ? DBOX_STATUS_ERROR_FAULT : DBOX_STATUS_ERROR_NONE;
}

EAPI int dynamicbox_provider_send_access_status(const char *pkgname, const char *id, int status)
{
    struct packet *packet;
    unsigned int cmd = CMD_ACCESS_STATUS;
    int ret;

    if (!pkgname || !id) {
	ErrPrint("Invalid argument\n");
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    if (s_info.fd < 0) {
	ErrPrint("Connection is not established\n");
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    packet = packet_create_noack((const char *)&cmd, "ssi", pkgname, id, status);
    if (!packet) {
	ErrPrint("Failed to build a packet\n");
	return DBOX_STATUS_ERROR_FAULT;
    }

    ret = com_core_packet_send_only(s_info.fd, packet);
    packet_destroy(packet);

    DbgPrint("[ACCESS] Send status: %d (%s) (%d)\n", status, id, ret);
    return ret < 0 ? DBOX_STATUS_ERROR_FAULT : DBOX_STATUS_ERROR_NONE;
}

EAPI int dynamicbox_provider_send_key_status(const char *pkgname, const char *id, int status)
{
    struct packet *packet;
    unsigned int cmd = CMD_KEY_STATUS;
    int ret;

    if (!pkgname || !id) {
	ErrPrint("Invalid argument\n");
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    if (s_info.fd < 0) {
	ErrPrint("Connection is not established\n");
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    packet = packet_create_noack((const char *)&cmd, "ssi", pkgname, id, status);
    if (!packet) {
	ErrPrint("Failed to build a packet\n");
	return DBOX_STATUS_ERROR_FAULT;
    }

    ret = com_core_packet_send_only(s_info.fd, packet);
    packet_destroy(packet);

    DbgPrint("[KEY] Send status: %d (%s) (%d)\n", status, id, ret);
    return ret < 0 ? DBOX_STATUS_ERROR_FAULT : DBOX_STATUS_ERROR_NONE;
}

EAPI int dynamicbox_provider_send_request_close_gbar(const char *pkgname, const char *id, int reason)
{
    struct packet *packet;
    unsigned int cmd = CMD_CLOSE_GBAR;
    int ret;

    if (!pkgname || !id) {
	ErrPrint("Invalid argument\n");
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    if (s_info.fd < 0) {
	ErrPrint("Connection is not established\n");
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    packet = packet_create_noack((const char *)&cmd, "ssi", pkgname, id, reason);
    if (!packet) {
	ErrPrint("Failed to build a packet\n");
	return DBOX_STATUS_ERROR_FAULT;
    }

    ret = com_core_packet_send_only(s_info.fd, packet);
    packet_destroy(packet);

    DbgPrint("[GBAR] Close GBAR: %d (%s) (%d)\n", reason, id, ret);
    return ret < 0 ? DBOX_STATUS_ERROR_FAULT : DBOX_STATUS_ERROR_NONE;
}

EAPI int dynamicbox_provider_control(int ctrl)
{
    struct packet *packet;
    unsigned int cmd = CMD_CTRL;
    int ret;

    if (s_info.fd < 0) {
	ErrPrint("Connection is not established\n");
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    packet = packet_create_noack((const char *)&cmd, "i", ctrl);
    if (!packet) {
	ErrPrint("Failed to build a packet\n");
	return DBOX_STATUS_ERROR_FAULT;
    }

    ret = com_core_packet_send_only(s_info.fd, packet);
    packet_destroy(packet);

    DbgPrint("[CTRL] Request: 0x%x\n", (unsigned int)ctrl);
    return ret < 0 ? DBOX_STATUS_ERROR_FAULT : DBOX_STATUS_ERROR_NONE;
}

/* End of a file */
