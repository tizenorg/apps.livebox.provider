/*
 * Copyright 2012  Samsung Electronics Co., Ltd
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
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#include <com-core.h>
#include <packet.h>
#include <com-core_packet.h>

#include <dlog.h>

#include "dlist.h"
#include "util.h"
#include "debug.h"
#include "fb.h"
#include "provider.h"
#include "provider_buffer.h"
#include "provider_buffer_internal.h"

#define EAPI __attribute__((visibility("default")))

static struct {
	struct dlist *buffer_list;
} s_info = {
	.buffer_list = NULL,
};

static inline struct fb_info *send_acquire_request(enum target_type type, const char *pkgname, const char *id, int w, int h, int size)
{
	struct packet *packet;
	struct packet *result;
	const char *buffer_id;
	struct fb_info *ret;
	int status;

	packet = packet_create("acquire_buffer", "isssiii", type, provider_name(), pkgname, id, w, h, size);
	if (!packet) {
		ErrPrint("Failed to build a packet\n");
		return NULL;
	}

	DbgPrint("type: 0x%X, name: %s, pkgname[%s], id[%s], w[%d], h[%d], size[%d]\n", type, provider_name(), pkgname, id, w, h, size);

	result = com_core_packet_oneshot_send(SLAVE_SOCKET, packet, 0.0f);
	packet_destroy(packet);

	if (!result) {
		ErrPrint("Failed to send a request\n");
		return NULL;
	}

	if (packet_get(result, "is", &status, &buffer_id) != 2) {
		ErrPrint("Failed to get a result packet\n");
		packet_unref(result);
		return NULL;
	}

	if (status != 0) {
		ErrPrint("Failed to acquire buffer: %d\n", status);
		packet_unref(result);
		return NULL;
	}

	ret = fb_create(buffer_id, w, h);
	/*!
	 * \TODO: Implements me
	 */
	DbgPrint("buffer id[%s], fb[%p]\n", buffer_id, ret);
	packet_unref(result);

	return ret;
}

static inline int send_release_request(enum target_type type, const char *pkgname, const char *id)
{
	struct packet *packet;
	struct packet *result;
	int ret;

	packet = packet_create("release_buffer", "isss", type, provider_name(), pkgname, id);
	if (!packet) {
		ErrPrint("Failed to build a packet\n");
		return -EFAULT;
	}

	result = com_core_packet_oneshot_send(SLAVE_SOCKET, packet, 0.0f);
	packet_destroy(packet);
	if (!result) {
		ErrPrint("Failed to send a request\n");
		return -EFAULT;
	}

	if (packet_get(result, "i", &ret) != 1) {
		ErrPrint("Invalid result packet\n");
		ret = -EINVAL;
	}

	packet_unref(result);
	return ret;
}

static inline struct fb_info *send_resize_request(enum target_type type, const char *pkgname, const char *id, int w, int h)
{
	struct packet *packet;
	struct packet *result;
	int ret;
	const char *buffer_id;
	struct fb_info *fb;

	packet = packet_create("resize_buffer", "isssii", type, provider_name(), pkgname, id, w, h);
	if (!packet) {
		ErrPrint("Faield to build a packet\n");
		return NULL;
	}

	result = com_core_packet_oneshot_send(SLAVE_SOCKET, packet, 0.0f);
	packet_destroy(packet);
	if (!result) {
		ErrPrint("Failed to send a request\n");
		return NULL;
	}

	if (packet_get(result, "is", &ret, &buffer_id) != 2) {
		ErrPrint("Invalid result packet\n");
		packet_unref(result);
		return NULL;
	}

	fb = (ret == 0) ? fb_create(buffer_id, w, h) : NULL;
	packet_unref(result);
	return fb;
}

struct livebox_buffer *provider_buffer_find_buffer(enum target_type type, const char *pkgname, const char *id)
{
	struct dlist *l;
	struct dlist *n;
	struct livebox_buffer *info;

	dlist_foreach_safe(s_info.buffer_list, l, n, info) {
		if (info->type != type)
			continue;

		if (!strcmp(info->pkgname, pkgname) && !strcmp(info->id, id))
			return info;
	}

	return NULL;
}

EAPI struct livebox_buffer *provider_buffer_acquire(enum target_type type, const char *pkgname, const char *id, int width, int height, int pixel_size, int (*handler)(struct livebox_buffer *, enum buffer_event, double, double, double, void *), void *data)
{
	struct livebox_buffer *info;

	if (!pkgname) {
		ErrPrint("Invalid parameter: pkgname is NIL\n");
		return NULL;
	}

	if (!id) {
		ErrPrint("Invalid ID: id is NIL\n");
		return NULL;
	}

	if (!handler)
		DbgPrint("Event handler is not speicified\n");

	DbgPrint("acquire_buffer: [%s] %s, %dx%d, size: %d, handler: %p\n",
						type == TYPE_LB ? "LB" : "PD", id,
						width, height, pixel_size, handler);

	info = malloc(sizeof(*info));
	if (!info) {
		ErrPrint("Heap: %s\n", strerror(errno));
		return NULL;
	}

	info->pkgname = strdup(pkgname);
	if (!info->pkgname) {
		ErrPrint("Heap: %s\n", strerror(errno));
		free(info);
		return NULL;
	}

	info->id = strdup(id);
	if (!info->id) {
		ErrPrint("Heap: %s\n", strerror(errno));
		free(info->pkgname);
		free(info);
		return NULL;
	}

	info->fb = send_acquire_request(type, pkgname, id, width, height, pixel_size);
	if (!info->fb) {
		ErrPrint("Failed to acquire a info\n");
		free(info->pkgname);
		free(info->id);
		free(info);
		return NULL;
	}

	info->width = width;
	info->height = height;
	info->pixel_size = pixel_size;
	info->handler = handler;
	info->type = type;
	info->data = data;
	info->state = BUFFER_CREATED;

	s_info.buffer_list = dlist_append(s_info.buffer_list, info);
	return info;
}

EAPI int provider_buffer_resize(struct livebox_buffer *info, int w, int h)
{
	struct fb_info *fb;

	if (!info || info->state != BUFFER_CREATED) {
		ErrPrint("info is not valid\n");
		return -EINVAL;
	}

	fb = send_resize_request(info->type, info->pkgname, info->id, w, h);
	if (!fb)
		return -EINVAL;

	/*!
	 * \note
	 * Even if we destroy the buffer object,
	 * if the user references it, it will not be destroyed,
	 * it only can be destroyed when there is no reference exists.
	 */
	info->fb = fb;
	return 0;
}

EAPI void *provider_buffer_ref(struct livebox_buffer *info)
{
	if (!info || info->state != BUFFER_CREATED) {
		ErrPrint("info is not valid\n");
		return NULL;
	}

	return fb_acquire_buffer(info->fb);
}

EAPI int provider_buffer_unref(void *ptr)
{
	if (!ptr) {
		ErrPrint("PTR is not valid\n");
		return -EINVAL;
	}

	return fb_release_buffer(ptr);
}

EAPI int provider_buffer_release(struct livebox_buffer *info)
{
	int ret;

	if (!info || info->state != BUFFER_CREATED) {
		ErrPrint("Buffer handler is NULL\n");
		return -EINVAL;
	}

	dlist_remove_data(s_info.buffer_list, info);

	ret = send_release_request(info->type, info->pkgname, info->id);
	if (ret < 0) {
		ErrPrint("Failed to send a release request\n");
		/*!
		 * \note
		 * But go ahead to destroy this buffer object
		 */
	}

	info->state = BUFFER_DESTROYED;
	free(info->pkgname);
	free(info->id);
	free(info);
	return 0;
}

EAPI int provider_buffer_sync(struct livebox_buffer *info)
{
	if (!info || info->state != BUFFER_CREATED) {
		ErrPrint("Buffer handler is NULL\n");
		return -EINVAL;
	}

	return fb_sync(info->fb);
}

EAPI enum target_type provider_buffer_type(struct livebox_buffer *info)
{
	if (!info || info->state != BUFFER_CREATED) {
		ErrPrint("Buffer handler is NULL\n");
		return TYPE_ERROR;
	}

	return info->type;
}

EAPI const char *provider_buffer_pkgname(struct livebox_buffer *info)
{
	if (!info || info->state != BUFFER_CREATED) {
		ErrPrint("Buffer handler is NULL\n");
		return NULL;
	}

	return info->pkgname;
}

/*!
 * \brief
 * Getting the URI of given buffer.
 */
EAPI const char *provider_buffer_id(struct livebox_buffer *info)
{
	if (!info || info->state != BUFFER_CREATED) {
		ErrPrint("Buffer handler is NULL\n");
		return NULL;
	}

	return info->id;
}

EAPI int provider_buffer_get_size(struct livebox_buffer *info, int *w, int *h, int *pixel_size)
{
	if (!info || info->state != BUFFER_CREATED) {
		ErrPrint("Buffer handler is NULL\n");
		return -EINVAL;
	}

	if (w)
		*w = info->width;

	if (h)
		*h = info->height;

	if (pixel_size)
		*pixel_size = info->pixel_size;

	return 0;
}

/*!
 * \brief
 * If the given buffer is created as pixmap,
 * This function returns pixmap ID.
 */
EAPI unsigned long provider_buffer_pixmap_id(struct livebox_buffer *info)
{
	const char *id;
	unsigned long pixmap;

	if (!info || info->state != BUFFER_CREATED) {
		ErrPrint("Buffer handler is NULL\n");
		return 0;
	}

	id = fb_id(info->fb);
	if (!id)
		return 0;

	if (sscanf(id, SCHEMA_PIXMAP "%lu", &pixmap) != 1) {
		ErrPrint("Invalid ID: %s\n", id);
		return 0;
	}

	return pixmap;
}

EAPI int provider_buffer_pixmap_is_support_hw(struct livebox_buffer *info)
{
	return fb_has_gem(info->fb);
}

EAPI int provider_buffer_pixmap_create_hw(struct livebox_buffer *info)
{
	if (!fb_has_gem(info->fb))
		return -EINVAL;

	return fb_create_gem(info->fb);
}

EAPI int provider_buffer_pixmap_destroy_hw(struct livebox_buffer *info)
{
	if (!fb_has_gem(info->fb))
		return -EINVAL;

	return fb_destroy_gem(info->fb);
}

EAPI void *provider_buffer_pixmap_hw_addr(struct livebox_buffer *info)
{
	void *addr;

	if (!fb_has_gem(info->fb))
		return NULL;

	addr = fb_acquire_gem(info->fb);
	fb_release_gem(info->fb);

	DbgPrint("Canvas address is %p\n", addr);
	return addr;
}

EAPI int provider_buffer_pre_render(struct livebox_buffer *info)
{
	int ret = 0;

	if (fb_has_gem(info->fb))
		ret = fb_acquire_gem(info->fb) ? 0 : -EFAULT;

	return ret;
}

EAPI int provider_buffer_post_render(struct livebox_buffer *info)
{
	int ret = 0;

	if (fb_has_gem(info->fb))
		ret = fb_release_gem(info->fb) ? 0 : -EFAULT;

	return ret;
}

int provider_buffer_init(void *display)
{
	return fb_init(display);
}

int provider_buffer_fini(void)
{
	return fb_fini();
}

struct packet *provider_buffer_lb_mouse_enter(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	int w;
	int h;
	double x;
	double y;
	double timestamp;
	struct livebox_buffer *info;

	if (packet_get(packet, "ssiiddd", &pkgname, &id, &w, &h, &timestamp, &x, &y) != 7) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler)
		(void)info->handler(info, BUFFER_EVENT_ENTER, timestamp, x, y, info->data);

out:
	return NULL;
}

struct packet *provider_buffer_lb_mouse_leave(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	int w;
	int h;
	double x;
	double y;
	double timestamp;
	struct livebox_buffer *info;

	if (packet_get(packet, "ssiiddd", &pkgname, &id, &w, &h, &timestamp, &x, &y) != 7) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler)
		(void)info->handler(info, BUFFER_EVENT_LEAVE, timestamp, x, y, info->data);

out:
	return NULL;
}

struct packet *provider_buffer_lb_mouse_down(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	int w;
	int h;
	double x;
	double y;
	double timestamp;
	struct livebox_buffer *info;

	if (packet_get(packet, "ssiiddd", &pkgname, &id, &w, &h, &timestamp, &x, &y) != 7) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler)
		(void)info->handler(info, BUFFER_EVENT_DOWN, timestamp, x, y, info->data);

out:
	return NULL;
}

struct packet *provider_buffer_lb_mouse_up(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	int w;
	int h;
	double x;
	double y;
	double timestamp;
	struct livebox_buffer *info;

	if (packet_get(packet, "ssiiddd", &pkgname, &id, &w, &h, &timestamp, &x, &y) != 7) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler)
		(void)info->handler(info, BUFFER_EVENT_UP, timestamp, x, y, info->data);

out:
	return NULL;
}

struct packet *provider_buffer_lb_mouse_move(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	int w;
	int h;
	double x;
	double y;
	double timestamp;
	struct livebox_buffer *info;

	if (packet_get(packet, "ssiiddd", &pkgname, &id, &w, &h, &timestamp, &x, &y) != 7) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler)
		(void)info->handler(info, BUFFER_EVENT_MOVE, timestamp, x, y, info->data);

out:
	return NULL;
}

struct packet *provider_buffer_pd_mouse_enter(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	int w;
	int h;
	double x;
	double y;
	double timestamp;
	struct livebox_buffer *info;

	if (packet_get(packet, "ssiiddd", &pkgname, &id, &w, &h, &timestamp, &x, &y) != 7) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler)
		(void)info->handler(info, BUFFER_EVENT_ENTER, timestamp, x, y, info->data);

out:
	return NULL;
}

struct packet *provider_buffer_pd_mouse_leave(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	int w;
	int h;
	double x;
	double y;
	double timestamp;
	struct livebox_buffer *info;

	if (packet_get(packet, "ssiiddd", &pkgname, &id, &w, &h, &timestamp, &x, &y) != 7) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler)
		(void)info->handler(info, BUFFER_EVENT_LEAVE, timestamp, x, y, info->data);

out:
	return NULL;
}

struct packet *provider_buffer_pd_mouse_down(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	int w;
	int h;
	double x;
	double y;
	double timestamp;
	struct livebox_buffer *info;

	if (packet_get(packet, "ssiiddd", &pkgname, &id, &w, &h, &timestamp, &x, &y) != 7) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler)
		(void)info->handler(info, BUFFER_EVENT_DOWN, timestamp, x, y, info->data);

out:
	return NULL;
}

struct packet *provider_buffer_pd_mouse_up(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	int w;
	int h;
	double x;
	double y;
	double timestamp;
	struct livebox_buffer *info;

	if (packet_get(packet, "ssiiddd", &pkgname, &id, &w, &h, &timestamp, &x, &y) != 7) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler)
		(void)info->handler(info, BUFFER_EVENT_UP, timestamp, x, y, info->data);

out:
	return NULL;
}

struct packet *provider_buffer_pd_mouse_move(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	int w;
	int h;
	double x;
	double y;
	double timestamp;
	struct livebox_buffer *info;

	if (packet_get(packet, "ssiiddd", &pkgname, &id, &w, &h, &timestamp, &x, &y) != 7) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler)
		(void)info->handler(info, BUFFER_EVENT_MOVE, timestamp, x, y, info->data);

out:
	return NULL;
}

/* End of a file */
