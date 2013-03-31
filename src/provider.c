/*
 * Copyright 2013  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.tizenopensource.org/license
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

#include <com-core.h>
#include <packet.h>
#include <com-core_packet.h>

#include <dlog.h>
#include <livebox-errno.h>

#include "dlist.h"
#include "util.h"
#include "provider.h"
#include "provider_buffer.h"
#include "provider_buffer_internal.h"
#include "debug.h"
#include "fb.h"

static struct info {
	int fd;
	char *name;
	struct event_handler table;
	void *data;
	int prevent_overwrite;
} s_info = {
	.fd = -1,
	.name = NULL,
	.data = NULL,
	.prevent_overwrite = 0,
};

#define EAPI __attribute__((visibility("default")))

/* pkgname, id, emission, source, sx, sy, ex, ey, x, y, down, ret */
static struct packet *master_script(pid_t pid, int handle, const struct packet *packet)
{
	struct event_arg arg;
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

	arg.type = EVENT_CONTENT_EVENT;

	if (s_info.table.content_event)
		(void)s_info.table.content_event(&arg, s_info.data);

errout:
	return NULL;
}

/* pkgname, id, event, timestamp, x, y, ret */
static struct packet *master_clicked(pid_t pid, int handle, const struct packet *packet)
{
	int ret;
	struct event_arg arg;

	ret = packet_get(packet, "sssddd",
			&arg.pkgname, &arg.id,
			&arg.info.clicked.event,
			&arg.info.clicked.timestamp,
			&arg.info.clicked.x, &arg.info.clicked.y);
	if (ret != 6) {
		ErrPrint("Parameter is not valid\n");
		goto errout;
	}

	arg.type = EVENT_CLICKED;

	if (s_info.table.clicked)
		(void)s_info.table.clicked(&arg, s_info.data);

errout:
	return NULL;
}

/* pkgname, id, emission, source, x, y, ex, ey, ret */
static struct packet *master_text_signal(pid_t pid, int handle, const struct packet *packet)
{
	struct event_arg arg;
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
	arg.type = EVENT_TEXT_SIGNAL;

	if (s_info.table.text_signal)
		(void)s_info.table.text_signal(&arg, s_info.data);

errout:
	return NULL;
}

/* pkgname, id, ret */
static struct packet *master_delete(pid_t pid, int handle, const struct packet *packet)
{
	struct packet *result;
	struct event_arg arg;
	int ret;

	ret = packet_get(packet, "ss", &arg.pkgname, &arg.id);
	if (ret != 2) {
		ErrPrint("Parameter is not valid\n");
		ret = LB_STATUS_ERROR_INVALID;
		goto errout;
	}

	arg.type = EVENT_DELETE;

	if (s_info.table.lb_destroy)
		ret = s_info.table.lb_destroy(&arg, s_info.data);
	else
		ret = LB_STATUS_ERROR_NOT_IMPLEMENTED;

errout:
	result = packet_create_reply(packet, "i", ret);
	return result;
}

/* pkgname, id, w, h, ret */
static struct packet *master_resize(pid_t pid, int handle, const struct packet *packet)
{
	struct event_arg arg;
	int ret;
	struct packet *result;

	ret = packet_get(packet, "ssii", &arg.pkgname, &arg.id, &arg.info.resize.w, &arg.info.resize.h);
	if (ret != 4) {
		ErrPrint("Parameter is not valid\n");
		ret = LB_STATUS_ERROR_INVALID;
		goto errout;
	}

	arg.type = EVENT_RESIZE;

	if (s_info.table.resize)
		ret = s_info.table.resize(&arg, s_info.data);
	else
		ret = LB_STATUS_ERROR_NOT_IMPLEMENTED;

errout:
	result = packet_create_reply(packet, "i", ret);
	return result;
}

/* pkgname, id, content, timeout, has_Script, period, cluster, category, pinup, width, height, abi, ret */
static struct packet *master_renew(pid_t pid, int handle, const struct packet *packet)
{
	struct event_arg arg;
	int ret;
	struct packet *result;
	char *content;
	char *title;

	arg.info.lb_recreate.out_title = NULL;
	arg.info.lb_recreate.out_content = NULL;
	arg.info.lb_recreate.out_is_pinned_up = 0;

	ret = packet_get(packet, "sssiidssiis", &arg.pkgname, &arg.id,
				&arg.info.lb_recreate.content,
				&arg.info.lb_recreate.timeout,
				&arg.info.lb_recreate.has_script,
				&arg.info.lb_recreate.period,
				&arg.info.lb_recreate.cluster, &arg.info.lb_recreate.category,
				&arg.info.lb_recreate.width, &arg.info.lb_recreate.height,
				&arg.info.lb_recreate.abi);
	if (ret != 11) {
		ErrPrint("Parameter is not valid\n");
		ret = LB_STATUS_ERROR_INVALID;
		goto errout;
	}

	arg.type = EVENT_RENEW;

	if (s_info.table.lb_recreate)
		ret = s_info.table.lb_recreate(&arg, s_info.data);
	else
		ret = LB_STATUS_ERROR_NOT_IMPLEMENTED;

errout:
	if (arg.info.lb_recreate.out_content)
		content = arg.info.lb_recreate.out_content;
	else
		content = "";

	if (arg.info.lb_recreate.out_title)
		title = arg.info.lb_recreate.out_title;
	else
		title = "";

	result = packet_create_reply(packet, "issi", ret, content, title, arg.info.lb_recreate.out_is_pinned_up);
	/*!
	 * \note
	 * Release.
	 */
	free(arg.info.lb_recreate.out_title);
	free(arg.info.lb_recreate.out_content);
	return result;
}

/* pkgname, id, content, timeout, has_script, period, cluster, category, pinup, skip_need_to_create, abi, result, width, height, priority */
static struct packet *master_new(pid_t pid, int handle, const struct packet *packet)
{
	struct event_arg arg;
	int ret;
	struct packet *result;
	int width = 0;
	int height = 0;
	double priority = 0.0f;
	char *content;
	char *title;

	arg.info.lb_create.out_content = NULL;
	arg.info.lb_create.out_title = NULL;
	arg.info.lb_create.out_is_pinned_up = 0;

	ret = packet_get(packet, "sssiidssisii", &arg.pkgname, &arg.id,
				&arg.info.lb_create.content,
				&arg.info.lb_create.timeout,
				&arg.info.lb_create.has_script,
				&arg.info.lb_create.period,
				&arg.info.lb_create.cluster, &arg.info.lb_create.category,
				&arg.info.lb_create.skip_need_to_create,
				&arg.info.lb_create.abi,
				&arg.info.lb_create.width,
				&arg.info.lb_create.height);
	if (ret != 12) {
		ErrPrint("Parameter is not valid\n");
		ret = LB_STATUS_ERROR_INVALID;
		goto errout;
	}

	arg.type = EVENT_NEW;

	if (s_info.table.lb_create)
		ret = s_info.table.lb_create(&arg, &width, &height, &priority, s_info.data);
	else
		ret = LB_STATUS_ERROR_NOT_IMPLEMENTED;

errout:
	if (arg.info.lb_create.out_content)
		content = arg.info.lb_create.out_content;
	else
		content = "";

	if (arg.info.lb_create.out_title)
		title = arg.info.lb_create.out_title;
	else
		title = "";

	result = packet_create_reply(packet, "iiidssi", ret, width, height, priority, content, title, arg.info.lb_create.out_is_pinned_up);

	/*!
	 * Note:
	 * Skip checking the address of out_content, out_title
	 */
	free(arg.info.lb_create.out_content);
	free(arg.info.lb_create.out_title);
	return result;
}

/* pkgname, id, period, ret */
static struct packet *master_set_period(pid_t pid, int handle, const struct packet *packet)
{
	struct event_arg arg;
	int ret;
	struct packet *result;

	ret = packet_get(packet, "ssd", &arg.pkgname, &arg.id, &arg.info.set_period.period);
	if (ret != 3) {
		ErrPrint("Parameter is not valid\n");
		ret = LB_STATUS_ERROR_INVALID;
		goto errout;
	}

	arg.type = EVENT_SET_PERIOD;

	if (s_info.table.set_period)
		ret = s_info.table.set_period(&arg, s_info.data);
	else
		ret = LB_STATUS_ERROR_NOT_IMPLEMENTED;

errout:
	result = packet_create_reply(packet, "i", ret);
	return result;
}

/* pkgname, id, cluster, category, ret */
static struct packet *master_change_group(pid_t pid, int handle, const struct packet *packet)
{
	struct event_arg arg;
	int ret;
	struct packet *result;

	ret = packet_get(packet, "ssss", &arg.pkgname, &arg.id,
				&arg.info.change_group.cluster, &arg.info.change_group.category);
	if (ret != 4) {
		ErrPrint("Parameter is not valid\n");
		ret = LB_STATUS_ERROR_INVALID;
		goto errout;
	}

	arg.type = EVENT_CHANGE_GROUP;

	if (s_info.table.change_group)
		ret = s_info.table.change_group(&arg, s_info.data);
	else
		ret = LB_STATUS_ERROR_NOT_IMPLEMENTED;

errout:
	result = packet_create_reply(packet, "i", ret);
	return result;
}

/* pkgname, id, pinup, ret */
static struct packet *master_pinup(pid_t pid, int handle, const struct packet *packet)
{
	struct event_arg arg;
	int ret;
	struct packet *result;
	const char *content;

	ret = packet_get(packet, "ssi", &arg.pkgname, &arg.id, &arg.info.pinup.state);
	if (ret != 3) {
		ErrPrint("Parameter is not valid\n");
		ret = LB_STATUS_ERROR_INVALID;
		goto errout;
	}

	arg.type = EVENT_PINUP;

	if (s_info.table.pinup)
		ret = s_info.table.pinup(&arg, s_info.data);
	else
		ret = LB_STATUS_ERROR_NOT_IMPLEMENTED;

errout:
	content = "default";
	if (ret == 0 && arg.info.pinup.content_info)
		content = arg.info.pinup.content_info;

	result = packet_create_reply(packet, "is", ret, content);
	if (ret == 0)
		free(arg.info.pinup.content_info);
	return result;
}

/* pkgname, id, cluster, category, ret */
static struct packet *master_update_content(pid_t pid, int handle, const struct packet *packet)
{
	struct event_arg arg;
	int ret;

	ret = packet_get(packet, "ssss", &arg.pkgname, &arg.id, &arg.info.update_content.cluster, &arg.info.update_content.category);
	if (ret != 4) {
		ErrPrint("Parameter is not valid\n");
		goto errout;
	}

	arg.type = EVENT_UPDATE_CONTENT;

	if (s_info.table.update_content)
		(void)s_info.table.update_content(&arg, s_info.data);

errout:
	return NULL;
}

static struct packet *master_lb_pause(pid_t pid, int handle, const struct packet *packet)
{
	struct event_arg arg;
	int ret;

	ret = packet_get(packet, "ss", &arg.pkgname, &arg.id);
	if (ret != 2) {
		ErrPrint("Invalid parameter\n");
		return NULL;
	}

	arg.type = EVENT_LB_PAUSE;

	if (s_info.table.lb_pause)
		(void)s_info.table.lb_pause(&arg, s_info.data);

	return NULL;
}

static struct packet *master_lb_resume(pid_t pid, int handle, const struct packet *packet)
{
	struct event_arg arg;
	int ret;

	ret = packet_get(packet, "ss", &arg.pkgname, &arg.id);
	if (ret != 2) {
		ErrPrint("Invalid parameter\n");
		return NULL;
	}

	arg.type = EVENT_LB_RESUME;

	if (s_info.table.lb_resume)
		(void)s_info.table.lb_resume(&arg, s_info.data);

	return NULL;
}

/* timestamp, ret */
static struct packet *master_pause(pid_t pid, int handle, const struct packet *packet)
{
	struct event_arg arg;
	struct packet *result;
	int ret;

	ret = packet_get(packet, "d", &arg.info.pause.timestamp);
	if (ret != 1) {
		ErrPrint("Parameter is not valid\n");
		ret = LB_STATUS_ERROR_INVALID;
		goto errout;
	}
	arg.pkgname = NULL;
	arg.id = NULL;
	arg.type = EVENT_PAUSE;

	if (s_info.table.pause)
		ret = s_info.table.pause(&arg, s_info.data);
	else
		ret = LB_STATUS_ERROR_NOT_IMPLEMENTED;

errout:
	result = packet_create_reply(packet, "i", ret);
	return result;
}

/* timestamp, ret */
static struct packet *master_resume(pid_t pid, int handle, const struct packet *packet)
{
	struct packet *result;
	struct event_arg arg;
	int ret;

	ret = packet_get(packet, "d", &arg.info.resume.timestamp);
	if (ret != 1) {
		ErrPrint("Parameter is not valid\n");
		ret = LB_STATUS_ERROR_INVALID;
		goto errout;
	}

	arg.pkgname = NULL;
	arg.id = NULL;
	arg.type = EVENT_RESUME;

	if (s_info.table.resume)
		ret = s_info.table.resume(&arg, s_info.data);
	else
		ret = LB_STATUS_ERROR_NOT_IMPLEMENTED;

errout:
	result = packet_create_reply(packet, "i", ret);
	if (!result)
		ErrPrint("Failed to create result packet\n");
	return result;
}

struct packet *master_pd_create(pid_t pid, int handle, const struct packet *packet)
{
	struct event_arg arg;
	int ret;

	ret = packet_get(packet, "ssiidd", &arg.pkgname, &arg.id, &arg.info.pd_create.w, &arg.info.pd_create.h, &arg.info.pd_create.x, &arg.info.pd_create.y);
	if (ret != 6) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	arg.type = EVENT_PD_CREATE;

	if (s_info.table.pd_create)
		(void)s_info.table.pd_create(&arg, s_info.data);

out:
	return NULL;
}

struct packet *master_pd_move(pid_t pid, int handle, const struct packet *packet)
{
	struct event_arg arg;
	int ret;

	ret = packet_get(packet, "ssiidd", &arg.pkgname, &arg.id, &arg.info.pd_move.w, &arg.info.pd_move.h, &arg.info.pd_move.x, &arg.info.pd_move.y);
	if (ret != 6) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	arg.type = EVENT_PD_MOVE;

	if (s_info.table.pd_move)
		(void)s_info.table.pd_move(&arg, s_info.data);

out:
	return NULL;
}

struct packet *master_pd_destroy(pid_t pid, int handle, const struct packet *packet)
{
	struct event_arg arg;
	int ret;

	ret = packet_get(packet, "ss", &arg.pkgname, &arg.id);
	if (ret != 2) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	arg.type = EVENT_PD_DESTROY;
	if (s_info.table.pd_destroy)
		(void)s_info.table.pd_destroy(&arg, s_info.data);

out:
	return NULL;
}

struct packet *master_pd_access_value_change(pid_t pid, int handle, const struct packet *packet)
{
	struct event_arg arg;
	double timestamp;
	int ret;

	ret = packet_get(packet, "ssdii", &arg.pkgname, &arg.id, &timestamp, &arg.info.pd_access.x, &arg.info.pd_access.y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	arg.type = EVENT_PD_ACCESS;
	arg.info.pd_access.event = ACCESS_VALUE_CHANGE;
	if (s_info.table.pd_access)
		(void)s_info.table.pd_access(&arg, s_info.data);

out:
	return NULL;
}

struct packet *master_pd_access_scroll(pid_t pid, int handle, const struct packet *packet)
{
	struct event_arg arg;
	double timestamp;
	int ret;

	ret = packet_get(packet, "ssdii", &arg.pkgname, &arg.id, &timestamp, &arg.info.pd_access.x, &arg.info.pd_access.y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	arg.type = EVENT_PD_ACCESS;
	arg.info.pd_access.event = ACCESS_SCROLL;
	if (s_info.table.pd_access)
		(void)s_info.table.pd_access(&arg, s_info.data);

out:
	return NULL;
}

struct packet *master_pd_access_hl(pid_t pid, int handle, const struct packet *packet)
{
	struct event_arg arg;
	double timestamp;
	int ret;

	ret = packet_get(packet, "ssdii", &arg.pkgname, &arg.id,
					 &timestamp,
					 &arg.info.pd_access.x, &arg.info.pd_access.y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	arg.type = EVENT_PD_ACCESS;
	arg.info.pd_access.event = ACCESS_HIGHLIGHT;
	if (s_info.table.pd_access)
		(void)s_info.table.pd_access(&arg, s_info.data);

out:
	return NULL;
}

struct packet *master_pd_access_hl_prev(pid_t pid, int handle, const struct packet *packet)
{
	struct event_arg arg;
	double timestamp;
	int ret;

	ret = packet_get(packet, "ssdii", &arg.pkgname, &arg.id,
					 &timestamp,
					 &arg.info.pd_access.x, &arg.info.pd_access.y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	arg.type = EVENT_PD_ACCESS;
	arg.info.pd_access.event = ACCESS_HIGHLIGHT_PREV;
	if (s_info.table.pd_access)
		(void)s_info.table.pd_access(&arg, s_info.data);

out:
	return NULL;
}

struct packet *master_pd_access_hl_next(pid_t pid, int handle, const struct packet *packet)
{
	struct event_arg arg;
	double timestamp;
	int ret;

	ret = packet_get(packet, "ssdii", &arg.pkgname, &arg.id,
					 &timestamp,
					 &arg.info.pd_access.x, &arg.info.pd_access.y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	arg.type = EVENT_PD_ACCESS;
	arg.info.pd_access.event = ACCESS_HIGHLIGHT_NEXT;
	if (s_info.table.pd_access)
		(void)s_info.table.pd_access(&arg, s_info.data);

out:
	return NULL;
}

struct packet *master_pd_access_activate(pid_t pid, int handle, const struct packet *packet)
{
	struct event_arg arg;
	double timestamp;
	int ret;

	ret = packet_get(packet, "ssdii", &arg.pkgname, &arg.id,
					 &timestamp,
					 &arg.info.pd_access.x, &arg.info.pd_access.y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	arg.type = EVENT_PD_ACCESS;
	arg.info.pd_access.event = ACCESS_ACTIVATE;
	if (s_info.table.pd_access)
		(void)s_info.table.pd_access(&arg, s_info.data);

out:
	return NULL;
}

struct packet *master_lb_access_hl(pid_t pid, int handle, const struct packet *packet)
{
	struct event_arg arg;
	double timestamp;
	int ret;

	ret = packet_get(packet, "ssdii", &arg.pkgname, &arg.id,
					 &timestamp,
					 &arg.info.lb_access.x, &arg.info.lb_access.y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	arg.type = EVENT_LB_ACCESS;
	arg.info.lb_access.event = ACCESS_HIGHLIGHT;
	if (s_info.table.lb_access)
		(void)s_info.table.lb_access(&arg, s_info.data);

out:
	return NULL;
}

struct packet *master_lb_access_hl_prev(pid_t pid, int handle, const struct packet *packet)
{
	struct event_arg arg;
	double timestamp;
	int ret;

	ret = packet_get(packet, "ssdii", &arg.pkgname, &arg.id,
					 &timestamp,
					 &arg.info.lb_access.x, &arg.info.lb_access.y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	arg.type = EVENT_LB_ACCESS;
	arg.info.lb_access.event = ACCESS_HIGHLIGHT_PREV;
	if (s_info.table.lb_access)
		(void)s_info.table.lb_access(&arg, s_info.data);

out:
	return NULL;
}

struct packet *master_lb_access_hl_next(pid_t pid, int handle, const struct packet *packet)
{
	struct event_arg arg;
	double timestamp;
	int ret;

	ret = packet_get(packet, "ssdii", &arg.pkgname, &arg.id,
					 &timestamp,
					 &arg.info.lb_access.x, &arg.info.lb_access.y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	arg.type = EVENT_LB_ACCESS;
	arg.info.lb_access.event = ACCESS_HIGHLIGHT_NEXT;
	if (s_info.table.lb_access)
		(void)s_info.table.lb_access(&arg, s_info.data);

out:
	return NULL;
}

struct packet *master_lb_access_value_change(pid_t pid, int handle, const struct packet *packet)
{
	struct event_arg arg;
	double timestamp;
	int ret;

	ret = packet_get(packet, "ssdii", &arg.pkgname, &arg.id,
					 &timestamp,
					 &arg.info.lb_access.x, &arg.info.lb_access.y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	arg.type = EVENT_LB_ACCESS;
	arg.info.lb_access.event = ACCESS_VALUE_CHANGE;
	if (s_info.table.lb_access)
		(void)s_info.table.lb_access(&arg, s_info.data);

out:
	return NULL;
}

struct packet *master_lb_access_scroll(pid_t pid, int handle, const struct packet *packet)
{
	struct event_arg arg;
	double timestamp;
	int ret;

	ret = packet_get(packet, "ssdii", &arg.pkgname, &arg.id,
					 &timestamp,
					 &arg.info.lb_access.x, &arg.info.lb_access.y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	arg.type = EVENT_LB_ACCESS;
	arg.info.lb_access.event = ACCESS_SCROLL;
	if (s_info.table.lb_access)
		(void)s_info.table.lb_access(&arg, s_info.data);

out:
	return NULL;
}

struct packet *master_lb_access_activate(pid_t pid, int handle, const struct packet *packet)
{
	struct event_arg arg;
	double timestamp;
	int ret;

	ret = packet_get(packet, "ssdii", &arg.pkgname, &arg.id,
					 &timestamp,
					 &arg.info.lb_access.x, &arg.info.lb_access.y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	arg.type = EVENT_LB_ACCESS;
	arg.info.lb_access.event = ACCESS_ACTIVATE;
	if (s_info.table.lb_access)
		(void)s_info.table.lb_access(&arg, s_info.data);

out:
	return NULL;
}

static struct method s_table[] = {
	/*!< For the buffer type */
	{
		.cmd = "lb_mouse_move",
		.handler = provider_buffer_lb_mouse_move,
	},
	{
		.cmd = "pd_mouse_move",
		.handler = provider_buffer_pd_mouse_move,
	},
	{
		.cmd = "pd_show",
		.handler = master_pd_create,
	},
	{
		.cmd = "pd_hide",
		.handler = master_pd_destroy,
	},
	{
		.cmd = "pd_move",
		.handler = master_pd_move,
	},
	{
		.cmd = "pd_mouse_up",
		.handler = provider_buffer_pd_mouse_up,
	},
	{
		.cmd = "pd_mouse_down",
		.handler = provider_buffer_pd_mouse_down,
	},
	{
		.cmd = "pd_mouse_enter",
		.handler = provider_buffer_pd_mouse_enter,
	},
	{
		.cmd = "pd_mouse_leave",
		.handler = provider_buffer_pd_mouse_leave,
	},
	{
		.cmd = "lb_mouse_up",
		.handler = provider_buffer_lb_mouse_up,
	},
	{
		.cmd = "lb_mouse_down",
		.handler = provider_buffer_lb_mouse_down,
	},
	{
		.cmd = "lb_mouse_enter",
		.handler = provider_buffer_lb_mouse_enter,
	},
	{
		.cmd = "lb_mouse_leave",
		.handler = provider_buffer_lb_mouse_leave,
	},
	{
		.cmd = "lb_key_down",
		.handler = provider_buffer_lb_key_down,
	},
	{
		.cmd = "lb_key_up",
		.handler = provider_buffer_lb_key_up,
	},
	{
		.cmd = "pd_key_down",
		.handler = provider_buffer_pd_key_down,
	},
	{
		.cmd = "pd_key_up",
		.handler = provider_buffer_pd_key_up,
	},
	{
		.cmd = "lb_pause",
		.handler = master_lb_pause,
	},
	{
		.cmd = "lb_resume",
		.handler = master_lb_resume,
	},
	{
		.cmd = "script",
		.handler = master_script, /* pkgname, id, emission, source, sx, sy, ex, ey, x, y, down, ret */
	},
	{
		.cmd = "clicked",
		.handler = master_clicked, /* pkgname, id, event, timestamp, x, y, ret */
	},
	{
		.cmd = "text_signal",
		.handler = master_text_signal, /* pkgname, id, emission, source, x, y, ex, ey, ret */
	},
	{
		.cmd = "delete",
		.handler = master_delete, /* pkgname, id, ret */
	},
	{
		.cmd = "resize",
		.handler = master_resize, /* pkgname, id, w, h, ret */
	},
	{
		.cmd = "renew",
		.handler = master_renew, /* pkgname, id, content, timeout, has_script, period, cluster, category, pinup, width, height, abi, ret */
	},
	{
		.cmd = "new",
		.handler = master_new, /* pkgname, id, content, timeout, has_script, period, cluster, category, pinup, skip_need_to_create, abi, result, width, height, priority */
	},
	{
		.cmd = "set_period",
		.handler = master_set_period, /* pkgname, id, period, ret */
	},
	{
		.cmd = "change_group",
		.handler = master_change_group, /* pkgname, id, cluster, category, ret */
	},
	{
		.cmd = "pinup",
		.handler = master_pinup, /* pkgname, id, pinup, ret */
	},
	{
		.cmd = "update_content",
		.handler = master_update_content, /* pkgname, cluster, category, ret */
	},
	{
		.cmd = "pause",
		.handler = master_pause, /* timestamp, ret */
	},
	{
		.cmd = "resume",
		.handler = master_resume, /* timestamp, ret */
	},

	{
		.cmd = "pd_access_hl",
		.handler = master_pd_access_hl,
	},
	{
		.cmd = "pd_access_hl_prev",
		.handler = master_pd_access_hl_prev,
	},
	{
		.cmd = "pd_access_hl_next",
		.handler = master_pd_access_hl_next,
	},
	{
		.cmd = "pd_access_activate",
		.handler = master_pd_access_activate,
	},
	{
		.cmd = "pd_access_scroll",
		.handler = master_pd_access_scroll,
	},
	{
		.cmd = "pd_access_value_change",
		.handler = master_pd_access_value_change,
	},

	{
		.cmd = "lb_access_hl",
		.handler = master_lb_access_hl,
	},
	{
		.cmd = "lb_access_hl_prev",
		.handler = master_lb_access_hl_prev,
	},
	{
		.cmd = "lb_access_hl_next",
		.handler = master_lb_access_hl_next,
	},
	{
		.cmd = "lb_access_activate",
		.handler = master_lb_access_activate,
	},
	{
		.cmd = "lb_access_scroll",
		.handler = master_lb_access_scroll,
	},
	{
		.cmd = "lb_access_value_change",
		.handler = master_lb_access_value_change,
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

	if (s_info.table.connected)
		s_info.table.connected(NULL, s_info.data);

	return 0;
}

static int disconnected_cb(int handle, void *data)
{
	if (s_info.fd != handle) {
		DbgPrint("%d is not my favor (%d)\n", handle, s_info.fd);
		return 0;
	}

	DbgPrint("Disconnected (%d)\n", handle);
	if (s_info.table.disconnected)
		s_info.table.disconnected(NULL, s_info.data);

	/* Reset the FD */
	s_info.fd = -1;
	return 0;
}

EAPI int provider_init(void *disp, const char *name, struct event_handler *table, void *data)
{
	int ret;
	const char *option;

	option = getenv("PROVIDER_DISABLE_PREVENT_OVERWRITE");
	if (option && !strcasecmp(option, "true"))
		s_info.prevent_overwrite = 1;

	option = getenv("PROVIDER_COM_CORE_THREAD");
	if (option && !strcasecmp(option, "true"))
		com_core_packet_use_thread(1);

	if (!name || !table) {
		ErrPrint("Invalid argument\n");
		return LB_STATUS_ERROR_INVALID;
	}

	if (s_info.name) {
		ErrPrint("Provider is already initialized\n");
		return LB_STATUS_ERROR_ALREADY;
	}

	s_info.name = strdup(name);
	if (!s_info.name) {
		ErrPrint("Heap: %s\n", strerror(errno));
		return LB_STATUS_ERROR_MEMORY;
	}

	memcpy(&s_info.table, table, sizeof(*table));
	s_info.data = data;

	com_core_add_event_callback(CONNECTOR_DISCONNECTED, disconnected_cb, NULL);
	com_core_add_event_callback(CONNECTOR_CONNECTED, connected_cb, NULL);

	ret = com_core_packet_client_init(SLAVE_SOCKET, 0, s_table);
	if (ret < 0) {
		ErrPrint("Failed to establish the connection with the master\n");
		return LB_STATUS_ERROR_FAULT;
	}

	provider_buffer_init(disp);

	DbgPrint("Slave is initialized\n");
	return LB_STATUS_SUCCESS;
}

EAPI void *provider_fini(void)
{
	void *ret;

	if (!s_info.name) {
		ErrPrint("Connection is not established (or cleared already)\n");
		return NULL;
	}

	if (s_info.fd >= 0) {
		com_core_packet_client_fini(s_info.fd);
		s_info.fd = -1;
	}

	com_core_del_event_callback(CONNECTOR_DISCONNECTED, disconnected_cb, NULL);
	com_core_del_event_callback(CONNECTOR_CONNECTED, connected_cb, NULL);

	provider_buffer_fini();

	free(s_info.name);
	s_info.name = NULL;

	ret = s_info.data;
	s_info.data = NULL;
	return ret;
}

EAPI int provider_send_call(const char *pkgname, const char *id, const char *funcname)
{
	struct packet *packet;
	int ret;

	if (!pkgname || !id || !funcname) {
		ErrPrint("Invalid argument\n");
		return LB_STATUS_ERROR_INVALID;
	}

	packet = packet_create_noack("call", "sss", pkgname, id, funcname);
	if (!packet) {
		ErrPrint("Failed to create a packet\n");
		return LB_STATUS_ERROR_FAULT;
	}

	ret = com_core_packet_send_only(s_info.fd, packet);
	packet_destroy(packet);
	return ret < 0 ? LB_STATUS_ERROR_FAULT : LB_STATUS_SUCCESS;
}

EAPI int provider_send_ret(const char *pkgname, const char *id, const char *funcname)
{
	struct packet *packet;
	int ret;

	if (!pkgname || !id || !funcname) {
		ErrPrint("Invalid argument\n");
		return LB_STATUS_ERROR_INVALID;
	}

	packet = packet_create_noack("ret", "sss", pkgname, id, funcname);
	if (!packet) {
		ErrPrint("Failed to create a packet\n");
		return LB_STATUS_ERROR_FAULT;
	}

	ret = com_core_packet_send_only(s_info.fd, packet);
	packet_destroy(packet);
	return ret < 0 ? LB_STATUS_ERROR_FAULT : LB_STATUS_SUCCESS;
}

EAPI int provider_send_faulted(const char *pkgname, const char *id, const char *funcname)
{
	struct packet *packet;
	int ret;

	if (!pkgname || !id || !funcname) {
		ErrPrint("Invalid argument\n");
		return LB_STATUS_ERROR_INVALID;
	}

	packet = packet_create_noack("faulted", "sss", pkgname, id, funcname);
	if (!packet) {
		ErrPrint("Failed to create a packet\n");
		return LB_STATUS_ERROR_FAULT;
	}

	ret = com_core_packet_send_only(s_info.fd, packet);
	packet_destroy(packet);
	return ret < 0 ? LB_STATUS_ERROR_FAULT : LB_STATUS_SUCCESS;
}

EAPI int provider_send_hello(void)
{
	struct packet *packet;
	int ret;
	DbgPrint("name[%s]\n", s_info.name);

	if (!s_info.name) {
		ErrPrint("Provider is not initialized\n");
		return LB_STATUS_ERROR_INVALID;
	}

	if (s_info.fd < 0) {
		ErrPrint("Connection is not established\n");
		return LB_STATUS_ERROR_INVALID;
	}

	packet = packet_create_noack("hello", "s", s_info.name);
	if (!packet) {
		ErrPrint("Failed to create a packet\n");
		return LB_STATUS_ERROR_FAULT;
	}

	ret = com_core_packet_send_only(s_info.fd, packet);
	packet_destroy(packet);
	return ret < 0 ? LB_STATUS_ERROR_FAULT : LB_STATUS_SUCCESS;
}

EAPI int provider_send_ping(void)
{
	struct packet *packet;
	int ret;

	DbgPrint("name[%s]\n", s_info.name);
	if (!s_info.name) {
		ErrPrint("Provider is not initialized\n");
		return LB_STATUS_ERROR_INVALID;
	}

	if (s_info.fd < 0) {
		ErrPrint("Connection is not established\n");
		return LB_STATUS_ERROR_INVALID;
	}

	packet = packet_create_noack("ping", "s", s_info.name);
	if (!packet) {
		ErrPrint("Failed to create a a packet\n");
		return LB_STATUS_ERROR_FAULT;
	}

	ret = com_core_packet_send_only(s_info.fd, packet);
	packet_destroy(packet);
	return ret < 0 ? LB_STATUS_ERROR_FAULT : LB_STATUS_SUCCESS;
}

static inline void keep_file_in_safe(const char *id)
{
	const char *path;
	int len;
	int base_idx;
	char *new_path;

	path = util_uri_to_path(id);
	if (!path) {
		ErrPrint("Invalid path\n");
		return;
	}

	/*!
	 * \TODO: REMOVE ME
	 */
	if (s_info.prevent_overwrite)
		return;

	if (access(path, R_OK | F_OK) != 0) {
		ErrPrint("[%s] %s\n", path, strerror(errno));
		return;
	}

	len = strlen(path);
	base_idx = len - 1;

	while (base_idx > 0 && path[base_idx] != '/') base_idx--;
	base_idx += (path[base_idx] == '/');

	new_path = malloc(len + 10); /* for "tmp" */
	if (!new_path) {
		ErrPrint("Heap: %s\n", strerror(errno));
		return;
	}

	strncpy(new_path, path, base_idx);
	snprintf(new_path + base_idx, len + 10 - base_idx, "reader/%s", path + base_idx);

	if (unlink(new_path) < 0)
		ErrPrint("Unlink(%s): %s\n", new_path, strerror(errno));

	if (rename(path, new_path) < 0)
		ErrPrint("Failed to keep content in safe: %s (%s -> %s)\n", strerror(errno), path, new_path);

	free(new_path);
}

EAPI int provider_send_updated(const char *pkgname, const char *id, int w, int h, double priority, const char *content_info, const char *title)
{
	struct packet *packet;
	int ret;

	if (!pkgname || !id) {
		ErrPrint("Invalid argument\n");
		return LB_STATUS_ERROR_INVALID;
	}

	if (!content_info)
		content_info = "";

	if (!title)
		title = "";

	if (s_info.fd < 0) {
		ErrPrint("Connection is not established\n");
		return LB_STATUS_ERROR_INVALID;
	}

	keep_file_in_safe(id);

	packet = packet_create_noack("updated", "ssiidss",
					pkgname, id, w, h, priority, content_info, title);
	if (!packet) {
		ErrPrint("failed to build a packet\n");
		return LB_STATUS_ERROR_FAULT;
	}

	ret = com_core_packet_send_only(s_info.fd, packet);
	packet_destroy(packet);
	return ret < 0 ? LB_STATUS_ERROR_FAULT : LB_STATUS_SUCCESS;
}

EAPI int provider_send_desc_updated(const char *pkgname, const char *id, const char *descfile)
{
	struct packet *packet;
	int ret;

	if (!pkgname || !id) {
		ErrPrint("Invalid argument\n");
		return LB_STATUS_ERROR_INVALID;
	}

	if (s_info.fd < 0) {
		ErrPrint("Connection is not established\n");
		return LB_STATUS_ERROR_INVALID;
	}

	if (!descfile)
		descfile = util_uri_to_path(id); /* In case of the NULL descfilename, use the ID */

	packet = packet_create_noack("desc_updated", "sss", pkgname, id, descfile);
	if (!packet) {
		ErrPrint("Failed to build a packet\n");
		return LB_STATUS_ERROR_FAULT;
	}

	ret = com_core_packet_send_only(s_info.fd, packet);
	packet_destroy(packet);
	return ret < 0 ? LB_STATUS_ERROR_FAULT : LB_STATUS_SUCCESS;
}

EAPI int provider_send_deleted(const char *pkgname, const char *id)
{
	struct packet *packet;
	int ret;

	if (!pkgname || !id) {
		ErrPrint("Invalid arguement\n");
		return LB_STATUS_ERROR_INVALID;
	}

	if (s_info.fd < 0) {
		ErrPrint("Connection is not established\n");
		return LB_STATUS_ERROR_INVALID;
	}

	packet = packet_create_noack("deleted", "ss", pkgname, id);
	if (!packet) {
		ErrPrint("Failed to build a packet\n");
		return LB_STATUS_ERROR_FAULT;
	}

	ret = com_core_packet_send_only(s_info.fd, packet);
	packet_destroy(packet);
	return ret < 0 ? LB_STATUS_ERROR_FAULT : LB_STATUS_SUCCESS;
}

EAPI int provider_send_hold_scroll(const char *pkgname, const char *id, int hold)
{
	struct packet *packet;
	int ret;

	if (!pkgname || !id) {
		ErrPrint("Invalid argument\n");
		return LB_STATUS_ERROR_INVALID;
	}

	if (s_info.fd < 0) {
		ErrPrint("Connection is not established\n");
		return LB_STATUS_ERROR_INVALID;
	}

	packet = packet_create_noack("scroll", "ssi", pkgname, id, !!hold);
	if (!packet) {
		ErrPrint("Failed to build a packet\n");
		return LB_STATUS_ERROR_FAULT;
	}

	ret = com_core_packet_send_only(s_info.fd, packet);
	packet_destroy(packet);
	return ret < 0 ? LB_STATUS_ERROR_FAULT : LB_STATUS_SUCCESS;
}

const char *provider_name(void)
{
	return s_info.name;
}

/* End of a file */
