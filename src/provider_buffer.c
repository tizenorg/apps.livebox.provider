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
#include <livebox-service.h>

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

	packet = packet_create("acquire_buffer", "issiii", type, pkgname, id, w, h, size);
	if (!packet) {
		ErrPrint("Failed to build a packet\n");
		return NULL;
	}

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
	DbgPrint("type: 0x%X, name: %s, pkgname[%s], id[%s], w[%d], h[%d], size[%d], buffer_id[%s], fb[%p]\n", type, provider_name(), pkgname, id, w, h, size, buffer_id, ret);
	packet_unref(result);

	return ret;
}

static inline int send_release_request(enum target_type type, const char *pkgname, const char *id)
{
	struct packet *packet;
	struct packet *result;
	int ret;

	packet = packet_create("release_buffer", "iss", type, pkgname, id);
	if (!packet) {
		ErrPrint("Failed to build a packet\n");
		return LB_STATUS_ERROR_FAULT;
	}

	result = com_core_packet_oneshot_send(SLAVE_SOCKET, packet, 0.0f);
	packet_destroy(packet);
	if (!result) {
		ErrPrint("Failed to send a request\n");
		return LB_STATUS_ERROR_FAULT;
	}

	if (packet_get(result, "i", &ret) != 1) {
		ErrPrint("Invalid result packet\n");
		ret = LB_STATUS_ERROR_INVALID;
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

	packet = packet_create("resize_buffer", "issii", type, pkgname, id, w, h);
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

EAPI int provider_buffer_set_user_data(struct livebox_buffer *handle, void *data)
{
	if (!handle || handle->state != BUFFER_CREATED) {
		ErrPrint("info is not valid\n");
		return LB_STATUS_ERROR_INVALID;
	}

	handle->user_data = data;
	return LB_STATUS_SUCCESS;
}

EAPI void *provider_buffer_user_data(struct livebox_buffer *handle)
{
	if (!handle || handle->state != BUFFER_CREATED) {
		ErrPrint("info is not valid\n");
		return NULL;
	}

	return handle->user_data;
}

EAPI struct livebox_buffer *provider_buffer_acquire(enum target_type type, const char *pkgname, const char *id, int width, int height, int pixel_size, int (*handler)(struct livebox_buffer *, enum buffer_event, double, double, double, void *), void *data)
{
	struct livebox_buffer *info;

	if (width <= 0 || height <= 0 || pixel_size <= 0) {
		ErrPrint("Invalid size: %dx%d, %d\n", width, height, pixel_size);
		return NULL;
	}

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

	s_info.buffer_list = dlist_prepend(s_info.buffer_list, info);
	return info;
}

EAPI int provider_buffer_resize(struct livebox_buffer *info, int w, int h)
{
	struct fb_info *fb;

	if (!info || info->state != BUFFER_CREATED) {
		ErrPrint("info is not valid\n");
		return LB_STATUS_ERROR_INVALID;
	}

	fb = send_resize_request(info->type, info->pkgname, info->id, w, h);
	if (!fb)
		return LB_STATUS_ERROR_INVALID;

	/*!
	 * \note
	 * Even if we destroy the buffer object,
	 * if the user references it, it will not be destroyed,
	 * it only can be destroyed when there is no reference exists.
	 */
	info->fb = fb;
	return LB_STATUS_SUCCESS;
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
		return LB_STATUS_ERROR_INVALID;
	}

	return fb_release_buffer(ptr);
}

EAPI int provider_buffer_release(struct livebox_buffer *info)
{
	int ret;

	if (!info || info->state != BUFFER_CREATED) {
		ErrPrint("Buffer handler is NULL\n");
		return LB_STATUS_ERROR_INVALID;
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
	return LB_STATUS_SUCCESS;
}

EAPI int provider_buffer_sync(struct livebox_buffer *info)
{
	if (!info || info->state != BUFFER_CREATED) {
		ErrPrint("Buffer handler is NULL\n");
		return LB_STATUS_ERROR_INVALID;
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
		return LB_STATUS_ERROR_INVALID;
	}

	if (w)
		*w = info->width;

	if (h)
		*h = info->height;

	if (pixel_size)
		*pixel_size = info->pixel_size;

	return LB_STATUS_SUCCESS;
}

EAPI const char *provider_buffer_uri(struct livebox_buffer *info)
{
	if (!info || info->state != BUFFER_CREATED) {
		ErrPrint("Buffer handler is NULL\n");
		return NULL;
	}

	return fb_id(info->fb);
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
	if (!info)
		return LB_STATUS_ERROR_INVALID;

	return fb_has_gem(info->fb);
}

EAPI int provider_buffer_pixmap_create_hw(struct livebox_buffer *info)
{
	if (!fb_has_gem(info->fb))
		return LB_STATUS_ERROR_INVALID;

	return fb_create_gem(info->fb);
}

EAPI int provider_buffer_pixmap_destroy_hw(struct livebox_buffer *info)
{
	if (!info || !fb_has_gem(info->fb))
		return LB_STATUS_ERROR_INVALID;

	return fb_destroy_gem(info->fb);
}

EAPI void *provider_buffer_pixmap_hw_addr(struct livebox_buffer *info)
{
	void *addr;

	if (!info || !fb_has_gem(info->fb))
		return NULL;

	addr = fb_acquire_gem(info->fb);
	fb_release_gem(info->fb);

	return addr;
}

EAPI int provider_buffer_pre_render(struct livebox_buffer *info)
{
	int ret = LB_STATUS_SUCCESS;

	if (!info)
		return LB_STATUS_ERROR_INVALID;

	if (fb_has_gem(info->fb))
		ret = fb_acquire_gem(info->fb) ? 0 : -EFAULT;

	return ret;
}

EAPI int provider_buffer_post_render(struct livebox_buffer *info)
{
	int ret = LB_STATUS_SUCCESS;

	if (!info)
		return LB_STATUS_ERROR_INVALID;

	if (fb_has_gem(info->fb))
		ret = fb_release_gem(info->fb);

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

struct packet *provider_buffer_lb_key_down(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	int x;
	int y;
	double timestamp;
	struct livebox_buffer *info;

	if (packet_get(packet, "ssdii", &pkgname, &id, &timestamp, &x, &y) != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler) {
		int w;
		int h;

		if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
			ErrPrint("Failed to get buffer size [%s:%s]\n", pkgname, id);
			goto out;
		}

		(void)info->handler(info, BUFFER_EVENT_KEY_DOWN, timestamp, (double)x / (double)w, (double)y / (double)h, info->data);
	}

out:
	return NULL;
}

struct packet *provider_buffer_lb_key_up(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	int x;
	int y;
	double timestamp;
	struct livebox_buffer *info;

	if (packet_get(packet, "ssdii", &pkgname, &id, &timestamp, &x, &y) != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler) {
		int w;
		int h;

		if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
			ErrPrint("Failed to get buffer size [%s:%s]\n", pkgname, id);
			goto out;
		}

		(void)info->handler(info, BUFFER_EVENT_KEY_UP, timestamp, (double)x / (double)w, (double)y / (double)h, info->data);
	}

out:
	return NULL;
}

struct packet *provider_buffer_lb_mouse_enter(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	int x;
	int y;
	double timestamp;
	struct livebox_buffer *info;

	if (packet_get(packet, "ssdii", &pkgname, &id, &timestamp, &x, &y) != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler) {
		int w;
		int h;
		if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
			ErrPrint("Failed to get buffer size [%s:%s]\n", pkgname, id);
			goto out;
		}

		(void)info->handler(info, BUFFER_EVENT_ENTER, timestamp, (double)x / (double)w, (double)y / (double)h, info->data);
	}

out:
	return NULL;
}

struct packet *provider_buffer_lb_mouse_leave(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	int x;
	int y;
	double timestamp;
	struct livebox_buffer *info;

	if (packet_get(packet, "ssdii", &pkgname, &id, &timestamp, &x, &y) != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler) {
		int w;
		int h;
		if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
			ErrPrint("Failed to get buffer size [%s:%s]\n", pkgname, id);
			goto out;
		}

		(void)info->handler(info, BUFFER_EVENT_LEAVE, timestamp, (double)x / (double)w, (double)y / (double)h, info->data);
	}

out:
	return NULL;
}

struct packet *provider_buffer_lb_mouse_down(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	int x;
	int y;
	double timestamp;
	struct livebox_buffer *info;

	if (packet_get(packet, "ssdii", &pkgname, &id, &timestamp, &x, &y) != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler) {
		int w;
		int h;
		if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
			ErrPrint("Failed to get buffer size [%s:%s]\n", pkgname, id);
			goto out;
		}

		(void)info->handler(info, BUFFER_EVENT_DOWN, timestamp, (double)x / (double)w, (double)y / (double)h, info->data);
	}

out:
	return NULL;
}

struct packet *provider_buffer_lb_mouse_up(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	int x;
	int y;
	double timestamp;
	struct livebox_buffer *info;

	if (packet_get(packet, "ssdii", &pkgname, &id, &timestamp, &x, &y) != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler) {
		int w;
		int h;
		if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
			ErrPrint("Failed to get buffer size [%s:%s]\n", pkgname, id);
			goto out;
		}

		(void)info->handler(info, BUFFER_EVENT_UP, timestamp, (double)x / (double)w, (double)y / (double)h, info->data);
	}

out:
	return NULL;
}

struct packet *provider_buffer_lb_mouse_move(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	int x;
	int y;
	double timestamp;
	struct livebox_buffer *info;

	if (packet_get(packet, "ssdii", &pkgname, &id, &timestamp, &x, &y) != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler) {
		int w;
		int h;
		if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
			ErrPrint("Failed to get buffer size [%s:%s]\n", pkgname, id);
			goto out;
		}
		(void)info->handler(info, BUFFER_EVENT_MOVE, timestamp, (double)x / (double)w, (double)y / (double)h, info->data);
	}

out:
	return NULL;
}

struct packet *provider_buffer_pd_key_down(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	int x;
	int y;
	double timestamp;
	struct livebox_buffer *info;

	if (packet_get(packet, "ssdii", &pkgname, &id, &timestamp, &x, &y) != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler) {
		int w;
		int h;
		if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
			ErrPrint("Failed to get buffer size [%s:%s]\n", pkgname, id);
			goto out;
		}

		(void)info->handler(info, BUFFER_EVENT_KEY_DOWN, timestamp, (double)x / (double)w, (double)y / (double)h, info->data);
	}

out:
	return NULL;
}

struct packet *provider_buffer_pd_key_up(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	int x;
	int y;
	double timestamp;
	struct livebox_buffer *info;

	if (packet_get(packet, "ssdii", &pkgname, &id, &timestamp, &x, &y) != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler) {
		int w;
		int h;
		if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
			ErrPrint("Failed to get buffer size [%s:%s]\n", pkgname, id);
			goto out;
		}

		(void)info->handler(info, BUFFER_EVENT_KEY_UP, timestamp, (double)x / (double)w, (double)y / (double)h, info->data);
	}

out:
	return NULL;
}

struct packet *provider_buffer_pd_mouse_enter(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	int x;
	int y;
	double timestamp;
	struct livebox_buffer *info;

	if (packet_get(packet, "ssdii", &pkgname, &id, &timestamp, &x, &y) != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler) {
		int w;
		int h;
		if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
			ErrPrint("Failed to get buffer size [%s:%s]\n", pkgname, id);
			goto out;
		}

		(void)info->handler(info, BUFFER_EVENT_ENTER, timestamp, (double)x / (double)w, (double)y / (double)h, info->data);
	}

out:
	return NULL;
}

struct packet *provider_buffer_pd_mouse_leave(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	int x;
	int y;
	double timestamp;
	struct livebox_buffer *info;

	if (packet_get(packet, "ssdii", &pkgname, &id, &timestamp, &x, &y) != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler) {
		int w;
		int h;
		if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
			ErrPrint("Failed to get buffer size [%s:%s]\n", pkgname, id);
			goto out;
		}

		(void)info->handler(info, BUFFER_EVENT_LEAVE, timestamp, (double)x / (double)w, (double)y / (double)h, info->data);
	}

out:
	return NULL;
}

struct packet *provider_buffer_pd_mouse_down(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	int x;
	int y;
	double timestamp;
	struct livebox_buffer *info;

	if (packet_get(packet, "ssdii", &pkgname, &id, &timestamp, &x, &y) != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler) {
		int w;
		int h;
		if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
			ErrPrint("Failed to get buffer size [%s:%s]\n", pkgname, id);
			goto out;
		}

		(void)info->handler(info, BUFFER_EVENT_DOWN, timestamp, (double)x / (double)w, (double)y / (double)h, info->data);
	}

out:
	return NULL;
}

struct packet *provider_buffer_pd_mouse_up(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	int x;
	int y;
	double timestamp;
	struct livebox_buffer *info;

	if (packet_get(packet, "ssdii", &pkgname, &id, &timestamp, &x, &y) != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler) {
		int w;
		int h;
		if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
			ErrPrint("Failed to get buffer size [%s:%s]\n", pkgname, id);
			goto out;
		}

		(void)info->handler(info, BUFFER_EVENT_UP, timestamp, (double)x / (double)w, (double)y / (double)h, info->data);
	}

out:
	return NULL;
}

struct packet *provider_buffer_pd_mouse_move(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	int x;
	int y;
	double timestamp;
	struct livebox_buffer *info;

	if (packet_get(packet, "ssdii", &pkgname, &id, &timestamp, &x, &y) != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler) {
		int w;
		int h;
		if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
			ErrPrint("Failed to get buffer size [%s:%s]\n", pkgname, id);
			goto out;
		}

		(void)info->handler(info, BUFFER_EVENT_MOVE, timestamp, (double)x / (double)w, (double)y / (double)h, info->data);
	}

out:
	return NULL;
}

struct packet *provider_buffer_pd_access_action_up(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	double timestamp;
	int ret;
	int x;
	int y;
	struct livebox_buffer *info;

	ret = packet_get(packet, "ssdii", &pkgname, &id, &timestamp, &x, &y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler) {
		int w;
		int h;

		if (provider_buffer_get_size(info, &w, &h, NULL) < 0)
			ErrPrint("Failed to get buffer size[%s:%s]\n", pkgname, id);

		(void)info->handler(info, BUFFER_EVENT_ACTION_UP, timestamp, (double)x / (double)w, (double)y / (double)h, info->data);
	} else {
		/* Event handler is not ready */
		(void)provider_send_access_status(pkgname, id, LB_ACCESS_STATUS_ERROR);
	}

out:
	return NULL;
}

struct packet *provider_buffer_pd_access_action_down(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	double timestamp;
	int ret;
	int x;
	int y;
	struct livebox_buffer *info;

	ret = packet_get(packet, "ssdii", &pkgname, &id, &timestamp, &x, &y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler) {
		int w;
		int h;

		if (provider_buffer_get_size(info, &w, &h, NULL) < 0)
			ErrPrint("Failed to get buffer size[%s:%s]\n", pkgname, id);

		(void)info->handler(info, BUFFER_EVENT_ACTION_DOWN, timestamp, (double)x / (double)w, (double)y / (double)h, info->data);
	} else {
		/* Event handler is not ready */
		(void)provider_send_access_status(pkgname, id, LB_ACCESS_STATUS_ERROR);
	}

out:
	return NULL;
}

struct packet *provider_buffer_pd_access_scroll_down(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	double timestamp;
	int ret;
	int x;
	int y;
	struct livebox_buffer *info;

	ret = packet_get(packet, "ssdii", &pkgname, &id, &timestamp, &x, &y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler) {
		int w;
		int h;

		if (provider_buffer_get_size(info, &w, &h, NULL) < 0)
			ErrPrint("Failed to get buffer size[%s:%s]\n", pkgname, id);

		(void)info->handler(info, BUFFER_EVENT_SCROLL_DOWN, timestamp, (double)x / (double)w, (double)y / (double)h, info->data);
	} else {
		/* Event handler is not ready */
		(void)provider_send_access_status(pkgname, id, LB_ACCESS_STATUS_ERROR);
	}

out:
	return NULL;
}

struct packet *provider_buffer_pd_access_scroll_move(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	double timestamp;
	int ret;
	int x;
	int y;
	struct livebox_buffer *info;

	ret = packet_get(packet, "ssdii", &pkgname, &id, &timestamp, &x, &y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler) {
		int w;
		int h;

		if (provider_buffer_get_size(info, &w, &h, NULL) < 0)
			ErrPrint("Failed to get buffer size[%s:%s]\n", pkgname, id);

		(void)info->handler(info, BUFFER_EVENT_SCROLL_MOVE, timestamp, (double)x / (double)w, (double)y / (double)h, info->data);
	} else {
		/* Event handler is not ready */
		(void)provider_send_access_status(pkgname, id, LB_ACCESS_STATUS_ERROR);
	}

out:
	return NULL;
}

struct packet *provider_buffer_pd_access_scroll_up(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	double timestamp;
	int ret;
	int x;
	int y;
	struct livebox_buffer *info;

	ret = packet_get(packet, "ssdii", &pkgname, &id, &timestamp, &x, &y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler) {
		int w;
		int h;

		if (provider_buffer_get_size(info, &w, &h, NULL) < 0)
			ErrPrint("Failed to get buffer size[%s:%s]\n", pkgname, id);

		(void)info->handler(info, BUFFER_EVENT_SCROLL_UP, timestamp, (double)x / (double)w, (double)y / (double)h, info->data);
	} else {
		/* Event handler is not ready */
		(void)provider_send_access_status(pkgname, id, LB_ACCESS_STATUS_ERROR);
	}

out:
	return NULL;
}

struct packet *provider_buffer_pd_access_unhighlight(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	double timestamp;
	int ret;
	int x;
	int y;
	struct livebox_buffer *info;

	ret = packet_get(packet, "ssdii", &pkgname, &id, &timestamp, &x, &y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler) {
		int w;
		int h;

		if (provider_buffer_get_size(info, &w, &h, NULL) < 0)
			ErrPrint("Failed to get buffer size[%s:%s]\n", pkgname, id);

		(void)info->handler(info, BUFFER_EVENT_UNHIGHLIGHT, timestamp, (double)x / (double)w, (double)y / (double)h, info->data);
	} else {
		/* Event handler is not ready */
		(void)provider_send_access_status(pkgname, id, LB_ACCESS_STATUS_ERROR);
	}

out:
	return NULL;
}

struct packet *provider_buffer_pd_access_hl(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	double timestamp;
	int ret;
	int x;
	int y;
	struct livebox_buffer *info;

	ret = packet_get(packet, "ssdii", &pkgname, &id, &timestamp, &x, &y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler) {
		int w;
		int h;

		if (provider_buffer_get_size(info, &w, &h, NULL) < 0)
			ErrPrint("Failed to get buffer size[%s:%s]\n", pkgname, id);

		(void)info->handler(info, BUFFER_EVENT_HIGHLIGHT, timestamp, (double)x / (double)w, (double)y / (double)h, info->data);
	} else {
		/* Event handler is not ready */
		(void)provider_send_access_status(pkgname, id, LB_ACCESS_STATUS_ERROR);
	}

out:
	return NULL;
}

struct packet *provider_buffer_pd_access_hl_prev(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	double timestamp;
	int ret;
	int x;
	int y;
	struct livebox_buffer *info;

	ret = packet_get(packet, "ssdii", &pkgname, &id, &timestamp, &x, &y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler) {
		int w;
		int h;

		if (provider_buffer_get_size(info, &w, &h, NULL) < 0)
			ErrPrint("Failed to get buffer size[%s:%s]\n", pkgname, id);

		(void)info->handler(info, BUFFER_EVENT_HIGHLIGHT_PREV, timestamp, (double)x / (double)w, (double)y / (double)h, info->data);
	} else {
		/* Event handler is not ready */
		(void)provider_send_access_status(pkgname, id, LB_ACCESS_STATUS_ERROR);
	}

out:
	return NULL;
}

struct packet *provider_buffer_pd_access_hl_next(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	double timestamp;
	int ret;
	int x;
	int y;
	struct livebox_buffer *info;

	ret = packet_get(packet, "ssdii", &pkgname, &id, &timestamp, &x, &y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler) {
		int w;
		int h;

		if (provider_buffer_get_size(info, &w, &h, NULL) < 0)
			ErrPrint("Failed to get buffer size[%s:%s]\n", pkgname, id);

		(void)info->handler(info, BUFFER_EVENT_HIGHLIGHT_NEXT, timestamp, (double)x / (double)w, (double)y / (double)h, info->data);
	} else {
		/* Event handler is not ready */
		(void)provider_send_access_status(pkgname, id, LB_ACCESS_STATUS_ERROR);
	}

out:
	return NULL;
}

struct packet *provider_buffer_pd_access_activate(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	double timestamp;
	int ret;
	int x;
	int y;
	struct livebox_buffer *info;

	ret = packet_get(packet, "ssdii", &pkgname, &id, &timestamp, &x, &y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler) {
		int w;
		int h;

		if (provider_buffer_get_size(info, &w, &h, NULL) < 0)
			ErrPrint("Failed to get buffer size[%s:%s]\n", pkgname, id);

		(void)info->handler(info, BUFFER_EVENT_ACTIVATE, timestamp, (double)x / (double)w, (double)y / (double)h, info->data);
	} else {
		/* Event handler is not ready */
		(void)provider_send_access_status(pkgname, id, LB_ACCESS_STATUS_ERROR);
	}

out:
	return NULL;
}

struct packet *provider_buffer_lb_access_unhighlight(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	double timestamp;
	int ret;
	int x;
	int y;
	struct livebox_buffer *info;

	ret = packet_get(packet, "ssdii", &pkgname, &id, &timestamp, &x, &y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler) {
		int w;
		int h;

		if (provider_buffer_get_size(info, &w, &h, NULL) < 0)
			ErrPrint("Failed to get buffer size[%s:%s]\n", pkgname, id);

		(void)info->handler(info, BUFFER_EVENT_UNHIGHLIGHT, timestamp, (double)x / (double)w, (double)y / (double)h, info->data);
	} else {
		/* Event handler is not ready */
		(void)provider_send_access_status(pkgname, id, LB_ACCESS_STATUS_ERROR);
	}

out:
	return NULL;
}

struct packet *provider_buffer_lb_access_hl(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	double timestamp;
	int ret;
	int x;
	int y;
	struct livebox_buffer *info;

	ret = packet_get(packet, "ssdii", &pkgname, &id, &timestamp, &x, &y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler) {
		int w;
		int h;

		if (provider_buffer_get_size(info, &w, &h, NULL) < 0)
			ErrPrint("Failed to get buffer size[%s:%s]\n", pkgname, id);

		(void)info->handler(info, BUFFER_EVENT_HIGHLIGHT, timestamp, (double)x / (double)w, (double)y / (double)h, info->data);
	} else {
		/* Event handler is not ready */
		(void)provider_send_access_status(pkgname, id, LB_ACCESS_STATUS_ERROR);
	}

out:
	return NULL;
}

struct packet *provider_buffer_lb_access_hl_prev(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	double timestamp;
	int ret;
	int x;
	int y;
	struct livebox_buffer *info;

	ret = packet_get(packet, "ssdii", &pkgname, &id, &timestamp, &x, &y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler) {
		int w;
		int h;

		if (provider_buffer_get_size(info, &w, &h, NULL) < 0)
			ErrPrint("Failed to get buffer size[%s:%s]\n", pkgname, id);

		(void)info->handler(info, BUFFER_EVENT_HIGHLIGHT_PREV, timestamp, (double)x / (double)w, (double)y / (double)h, info->data);
	} else {
		/* Event handler is not ready */
		(void)provider_send_access_status(pkgname, id, LB_ACCESS_STATUS_ERROR);
	}

out:
	return NULL;
}

struct packet *provider_buffer_lb_access_hl_next(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	double timestamp;
	int ret;
	int x;
	int y;
	struct livebox_buffer *info;

	ret = packet_get(packet, "ssdii", &pkgname, &id, &timestamp, &x, &y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler) {
		int w;
		int h;

		if (provider_buffer_get_size(info, &w, &h, NULL) < 0)
			ErrPrint("Failed to get buffer size[%s:%s]\n", pkgname, id);

		(void)info->handler(info, BUFFER_EVENT_HIGHLIGHT_NEXT, timestamp, (double)x / (double)w, (double)y / (double)h, info->data);
	} else {
		/* Event handler is not ready */
		(void)provider_send_access_status(pkgname, id, LB_ACCESS_STATUS_ERROR);
	}

out:
	return NULL;
}

struct packet *provider_buffer_lb_access_action_up(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	double timestamp;
	int ret;
	int x;
	int y;
	struct livebox_buffer *info;

	ret = packet_get(packet, "ssdii", &pkgname, &id, &timestamp, &x, &y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler) {
		int w;
		int h;

		if (provider_buffer_get_size(info, &w, &h, NULL) < 0)
			ErrPrint("Failed to get buffer size[%s:%s]\n", pkgname, id);

		(void)info->handler(info, BUFFER_EVENT_ACTION_UP, timestamp, (double)x / (double)w, (double)y / (double)h, info->data);
	} else {
		/* Event handler is not ready */
		(void)provider_send_access_status(pkgname, id, LB_ACCESS_STATUS_ERROR);
	}

out:
	return NULL;
}

struct packet *provider_buffer_lb_access_action_down(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	double timestamp;
	int ret;
	int x;
	int y;
	struct livebox_buffer *info;

	ret = packet_get(packet, "ssdii", &pkgname, &id, &timestamp, &x, &y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler) {
		int w;
		int h;

		if (provider_buffer_get_size(info, &w, &h, NULL) < 0)
			ErrPrint("Failed to get buffer size[%s:%s]\n", pkgname, id);

		(void)info->handler(info, BUFFER_EVENT_ACTION_DOWN, timestamp, (double)x / (double)w, (double)y / (double)h, info->data);
	} else {
		/* Event handler is not ready */
		(void)provider_send_access_status(pkgname, id, LB_ACCESS_STATUS_ERROR);
	}

out:
	return NULL;
}

struct packet *provider_buffer_lb_access_scroll_down(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	double timestamp;
	int ret;
	int x;
	int y;
	struct livebox_buffer *info;

	ret = packet_get(packet, "ssdii", &pkgname, &id, &timestamp, &x, &y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler) {
		int w;
		int h;

		if (provider_buffer_get_size(info, &w, &h, NULL) < 0)
			ErrPrint("Failed to get buffer size[%s:%s]\n", pkgname, id);

		(void)info->handler(info, BUFFER_EVENT_SCROLL_DOWN, timestamp, (double)x / (double)w, (double)y / (double)h, info->data);
	} else {
		/* Event handler is not ready */
		(void)provider_send_access_status(pkgname, id, LB_ACCESS_STATUS_ERROR);
	}

out:
	return NULL;
}

struct packet *provider_buffer_lb_access_scroll_move(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	double timestamp;
	int ret;
	int x;
	int y;
	struct livebox_buffer *info;

	ret = packet_get(packet, "ssdii", &pkgname, &id, &timestamp, &x, &y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler) {
		int w;
		int h;

		if (provider_buffer_get_size(info, &w, &h, NULL) < 0)
			ErrPrint("Failed to get buffer size[%s:%s]\n", pkgname, id);

		(void)info->handler(info, BUFFER_EVENT_SCROLL_MOVE, timestamp, (double)x / (double)w, (double)y / (double)h, info->data);
	} else {
		/* Event handler is not ready */
		(void)provider_send_access_status(pkgname, id, LB_ACCESS_STATUS_ERROR);
	}

out:
	return NULL;
}

struct packet *provider_buffer_lb_access_scroll_up(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	double timestamp;
	int ret;
	int x;
	int y;
	struct livebox_buffer *info;

	ret = packet_get(packet, "ssdii", &pkgname, &id, &timestamp, &x, &y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler) {
		int w;
		int h;

		if (provider_buffer_get_size(info, &w, &h, NULL) < 0)
			ErrPrint("Failed to get buffer size[%s:%s]\n", pkgname, id);

		(void)info->handler(info, BUFFER_EVENT_SCROLL_UP, timestamp, (double)x / (double)w, (double)y / (double)h, info->data);
	} else {
		/* Event handler is not ready */
		(void)provider_send_access_status(pkgname, id, LB_ACCESS_STATUS_ERROR);
	}

out:
	return NULL;
}

struct packet *provider_buffer_lb_access_activate(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	double timestamp;
	int ret;
	int x;
	int y;
	struct livebox_buffer *info;

	ret = packet_get(packet, "ssdii", &pkgname, &id, &timestamp, &x, &y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler) {
		int w;
		int h;

		if (provider_buffer_get_size(info, &w, &h, NULL) < 0)
			ErrPrint("Failed to get buffer size[%s:%s]\n", pkgname, id);

		(void)info->handler(info, BUFFER_EVENT_ACTIVATE, timestamp, (double)x / (double)w, (double)y / (double)h, info->data);
	} else {
		/* Event handler is not ready */
		(void)provider_send_access_status(pkgname, id, LB_ACCESS_STATUS_ERROR);
	}

out:
	return NULL;
}

/* End of a file */
