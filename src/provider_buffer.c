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
#include <sys/stat.h>
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

static struct fb_info *send_acquire_request(enum target_type type, const char *pkgname, const char *id, int w, int h, int size)
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

EAPI struct livebox_buffer *provider_buffer_find_buffer(enum target_type type, const char *pkgname, const char *id)
{
	struct dlist *l;
	struct dlist *n;
	struct livebox_buffer *info;

	dlist_foreach_safe(s_info.buffer_list, l, n, info) {
		if (info->type != type) {
			continue;
		}

		if (!strcmp(info->pkgname, pkgname) && !strcmp(info->id, id)) {
			return info;
		}
	}

	return NULL;
}

EAPI int provider_buffer_set_user_data(struct livebox_buffer *handle, void *data)
{
	if (!handle || (handle->state != BUFFER_INITIALIZED && handle->state != BUFFER_CREATED)) {
		ErrPrint("info is not valid\n");
		return LB_STATUS_ERROR_INVALID;
	}

	handle->user_data = data;
	return LB_STATUS_SUCCESS;
}

EAPI void *provider_buffer_user_data(struct livebox_buffer *handle)
{
	if (!handle || (handle->state != BUFFER_INITIALIZED && handle->state != BUFFER_CREATED)) {
		ErrPrint("info is not valid\n");
		return NULL;
	}

	return handle->user_data;
}

static int destroy_lock_file(struct livebox_buffer *info)
{
	if (info->lock_fd < 0) {
		return -EINVAL;
	}

        if (!info->lock) {
                return -EINVAL;
        }

        if (close(info->lock_fd) < 0) {
                ErrPrint("close: %s\n", strerror(errno));
        }
        info->lock_fd = -1;

        if (unlink(info->lock) < 0) {
                ErrPrint("unlink: %s\n", strerror(errno));
        }

        free(info->lock);
        info->lock = NULL;
        return 0;
}

static int create_lock_file(struct livebox_buffer *info)
{
        int len;
        char *file;

        len = strlen(info->id);
        file = malloc(len + 20);
        if (!file) {
                ErrPrint("Heap: %s\n", strerror(errno));
                return -ENOMEM;
        }

        snprintf(file, len + 20, "%s.%s.lck", util_uri_to_path(info->id), info->type == TYPE_PD ? "pd" : "lb");
        info->lock_fd = open(file, O_WRONLY|O_CREAT, 0644);
        if (info->lock_fd < 0) {
                ErrPrint("open: %s\n", strerror(errno));
                free(file);
                return -EIO;
        }

        info->lock = file;
        return 0;
}

static int acquire_lock(struct livebox_buffer *info)
{
	struct flock flock;
	int ret;

	if (info->lock_fd < 0) {
		return 0;
	}

	flock.l_type = F_WRLCK;
	flock.l_whence = SEEK_SET;
	flock.l_start = 0;
	flock.l_len = 0;
	flock.l_pid = getpid();

	do {
		ret = fcntl(info->lock_fd, F_SETLKW, &flock);
		if (ret < 0) {
			ret = errno;
			ErrPrint("fcntl: %s\n", strerror(errno));
		}
	} while (ret == EINTR);

	return 0;
}

static int release_lock(struct livebox_buffer *info)
{
	struct flock flock;
	int ret;

	if (info->lock_fd < 0) {
		return 0;
	}

	flock.l_type = F_UNLCK;
	flock.l_whence = SEEK_SET;
	flock.l_start = 0;
	flock.l_len = 0;
	flock.l_pid = getpid();

	do {
		ret = fcntl(info->lock_fd, F_SETLKW, &flock);
		if (ret < 0) {
			ret = errno;
			ErrPrint("fcntl: %s\n", strerror(errno));
		}
	} while (ret == EINTR);

	return 0;
}

EAPI struct livebox_buffer *provider_buffer_create(enum target_type type, const char *pkgname, const char *id, int (*handler)(struct livebox_buffer *, struct buffer_event_data *, void *), void *data)
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

	if (!handler) {
		DbgPrint("Event handler is not speicified\n");
	}

	DbgPrint("acquire_buffer: [%s] %s, handler: %p\n",
						type == TYPE_LB ? "LB" : "PD", id, handler);

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

	info->fb = NULL;
	info->width = 0;
	info->height = 0;
	info->pixel_size = 0;
	info->handler_NEW = handler;
	info->handler = NULL;
	info->type = type;
	info->data = data;
	info->state = BUFFER_INITIALIZED;
	info->lock = NULL;
	info->lock_fd = -1;

	s_info.buffer_list = dlist_prepend(s_info.buffer_list, info);
	return info;
}

EAPI int provider_buffer_acquire_NEW(struct livebox_buffer *info, int width, int height, int pixel_size)
{
	if (width <= 0 || height <= 0 || pixel_size <= 0) {
		ErrPrint("Invalid size: %dx%d, %d\n", width, height, pixel_size);
		return LB_STATUS_ERROR_INVALID;
	}

	DbgPrint("acquire_buffer: [%s] %s, %dx%d, size: %d, handler: %p\n",
						info->type == TYPE_LB ? "LB" : "PD", info->id,
						width, height, pixel_size, info->handler_NEW);

	info->fb = send_acquire_request(info->type, info->pkgname, info->id, width, height, pixel_size);
	if (!info->fb) {
		ErrPrint("Failed to acquire a info\n");
		return LB_STATUS_ERROR_FAULT;
	}

	info->width = width;
	info->height = height;
	info->pixel_size = pixel_size;
	info->state = BUFFER_CREATED;

	switch (fb_type(info->fb)) {
	case BUFFER_TYPE_FILE:
	case BUFFER_TYPE_SHM:
		(void)create_lock_file(info);
		break;
	case BUFFER_TYPE_PIXMAP:
	default:
		/* Do nothing */
		break;
	}

	return 0;
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

	if (!handler) {
		DbgPrint("Event handler is not speicified\n");
	}

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
	info->handler_NEW = NULL;
	info->type = type;
	info->data = data;
	info->state = BUFFER_CREATED;

	switch (fb_type(info->fb)) {
	case BUFFER_TYPE_FILE:
	case BUFFER_TYPE_SHM:
		create_lock_file(info);
		break;
	case BUFFER_TYPE_PIXMAP:
	default:
		info->lock = NULL;
		info->lock_fd = -1;
		break;
	}

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
	if (!fb) {
		return LB_STATUS_ERROR_INVALID;
	}

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

	(void)destroy_lock_file(info);

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

EAPI int provider_buffer_release_NEW(struct livebox_buffer *info)
{
	int ret;

	if (!info || info->state != BUFFER_CREATED) {
		ErrPrint("Buffer handler is NULL\n");
		return LB_STATUS_ERROR_INVALID;
	}

	(void)destroy_lock_file(info);

	ret = send_release_request(info->type, info->pkgname, info->id);
	if (ret < 0) {
		ErrPrint("Failed to send a release request [%d]\n", ret);
		/*!
		 * \note
		 * But go ahead to destroy this buffer object
		 */
	}

	info->fb = NULL;
	info->width = 0;
	info->height = 0;
	info->pixel_size = 0;
	info->state = BUFFER_INITIALIZED;

	return LB_STATUS_SUCCESS;
}

EAPI int provider_buffer_destroy(struct livebox_buffer *info)
{
	if (!info || info->state != BUFFER_INITIALIZED) {
		ErrPrint("Buffer handler is %p (%x)\n", info, info ? info->state : 0x0);
		return LB_STATUS_ERROR_INVALID;
	}

	dlist_remove_data(s_info.buffer_list, info);

	info->state = BUFFER_DESTROYED;
	free(info->pkgname);
	free(info->id);
	free(info);

	return LB_STATUS_SUCCESS;
}

EAPI int provider_buffer_sync(struct livebox_buffer *info)
{
	int ret;
	if (!info || info->state != BUFFER_CREATED) {
		ErrPrint("Buffer handler is NULL\n");
		return LB_STATUS_ERROR_INVALID;
	}

	if (fb_type(info->fb) == BUFFER_TYPE_FILE) {
		(void)acquire_lock(info);
		ret = fb_sync(info->fb);
		(void)release_lock(info);
	} else {
		ret = fb_sync(info->fb);
	}
	return ret;
}

EAPI enum target_type provider_buffer_type(struct livebox_buffer *info)
{
	if (!info || (info->state != BUFFER_CREATED && info->state != BUFFER_INITIALIZED)) {
		ErrPrint("Buffer handler is NULL\n");
		return TYPE_ERROR;
	}

	return info->type;
}

EAPI const char *provider_buffer_pkgname(struct livebox_buffer *info)
{
	if (!info || (info->state != BUFFER_INITIALIZED && info->state != BUFFER_CREATED)) {
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
	if (!info || (info->state != BUFFER_CREATED && info->state != BUFFER_INITIALIZED)) {
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

	if (w) {
		*w = info->width;
	}

	if (h) {
		*h = info->height;
	}

	if (pixel_size) {
		*pixel_size = info->pixel_size;
	}

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
	int size;

	if (!info || info->state != BUFFER_CREATED) {
		ErrPrint("Buffer handler is NULL\n");
		return 0;
	}

	id = fb_id(info->fb);
	if (!id) {
		return 0;
	}

	if (sscanf(id, SCHEMA_PIXMAP "%lu:%d", &pixmap, &size) != 2) {
		ErrPrint("Invalid ID: %s\n", id);
		return 0;
	}

	if (info->pixel_size != size) {
		DbgPrint("Pixel size is mismatched: %d <> %d\n", info->pixel_size, size);
	}

	return pixmap;
}

EAPI int provider_buffer_pixmap_is_support_hw(struct livebox_buffer *info)
{
	if (!info || info->state != BUFFER_CREATED) {
		return LB_STATUS_ERROR_INVALID;
	}

	return fb_has_gem(info->fb);
}

EAPI int provider_buffer_pixmap_create_hw(struct livebox_buffer *info)
{
	if (!info || info->state != BUFFER_CREATED) {
		return LB_STATUS_ERROR_INVALID;
	}

	if (!fb_has_gem(info->fb)) {
		return LB_STATUS_ERROR_INVALID;
	}

	return fb_create_gem(info->fb);
}

EAPI int provider_buffer_pixmap_destroy_hw(struct livebox_buffer *info)
{
	if (!info || info->state != BUFFER_CREATED || !fb_has_gem(info->fb)) {
		return LB_STATUS_ERROR_INVALID;
	}

	return fb_destroy_gem(info->fb);
}

EAPI void *provider_buffer_pixmap_hw_addr(struct livebox_buffer *info)
{
	void *addr;

	if (!info || info->state != BUFFER_CREATED || !fb_has_gem(info->fb)) {
		return NULL;
	}

	addr = fb_acquire_gem(info->fb);
	fb_release_gem(info->fb);

	return addr;
}

EAPI int provider_buffer_pre_render(struct livebox_buffer *info)
{
	int ret = LB_STATUS_SUCCESS;

	if (!info) {
		return LB_STATUS_ERROR_INVALID;
	}

	if (fb_has_gem(info->fb)) {
		ret = fb_acquire_gem(info->fb) ? LB_STATUS_SUCCESS : LB_STATUS_ERROR_FAULT;
	} else if (fb_type(info->fb) == BUFFER_TYPE_SHM) {
		ret = acquire_lock(info);
	}

	return ret;
}

EAPI int provider_buffer_post_render(struct livebox_buffer *info)
{
	int ret = LB_STATUS_SUCCESS;

	if (!info) {
		return LB_STATUS_ERROR_INVALID;
	}

	if (fb_has_gem(info->fb)) {
		ret = fb_release_gem(info->fb);
	} else if (fb_type(info->fb) == BUFFER_TYPE_SHM) {
		ret = release_lock(info);
	}

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
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_KEY_DOWN,
	};

	if (packet_get(packet, "ssdi", &pkgname, &id, &data.timestamp, &data.info.keycode) != 4) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler_NEW) {
		(void)info->handler_NEW(info, &data, info->data);
	} else if (info->handler) {
		DbgPrint("Unsupported\n");
		(void)provider_send_key_status(pkgname, id, LB_KEY_STATUS_ERROR);
	} else {
		(void)provider_send_key_status(pkgname, id, LB_KEY_STATUS_ERROR);
	}

out:
	return NULL;
}

struct packet *provider_buffer_lb_key_up(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_KEY_UP,
	};

	if (packet_get(packet, "ssdi", &pkgname, &id, &data.timestamp, &data.info.keycode) != 4) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler_NEW) {
		(void)info->handler_NEW(info, &data, info->data);
	} else if (info->handler) {
		DbgPrint("Unsupported\n");
		(void)provider_send_key_status(pkgname, id, LB_KEY_STATUS_ERROR);
	} else {
		(void)provider_send_key_status(pkgname, id, LB_KEY_STATUS_ERROR);
	}

out:
	return NULL;
}

struct packet *provider_buffer_lb_key_focus_in(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_KEY_FOCUS_IN,
	};

	if (packet_get(packet, "ssdi", &pkgname, &id, &data.timestamp, &data.info.keycode) != 4) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler_NEW) {
		(void)info->handler_NEW(info, &data, info->data);
	} else if (info->handler) {
		DbgPrint("Unsupported\n");
		(void)provider_send_key_status(pkgname, id, LB_KEY_STATUS_ERROR);
	} else {
		(void)provider_send_key_status(pkgname, id, LB_KEY_STATUS_ERROR);
	}

out:
	return NULL;
}

struct packet *provider_buffer_lb_key_focus_out(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_KEY_FOCUS_OUT,
	};

	if (packet_get(packet, "ssdi", &pkgname, &id, &data.timestamp, &data.info.keycode) != 4) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler_NEW) {
		(void)info->handler_NEW(info, &data, info->data);
	} else if (info->handler) {
		DbgPrint("Unsupported\n");
		(void)provider_send_key_status(pkgname, id, LB_KEY_STATUS_ERROR);
	} else {
		(void)provider_send_key_status(pkgname, id, LB_KEY_STATUS_ERROR);
	}

out:
	return NULL;
}

struct packet *provider_buffer_lb_mouse_enter(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_ENTER,
	};

	if (packet_get(packet, "ssdii", &pkgname, &id, &data.timestamp, &data.info.pointer.x, &data.info.pointer.y) != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler || info->handler_NEW) {
		if (info->handler_NEW) {
			(void)info->handler_NEW(info, &data, info->data);
		} else if (info->handler) {
			double x;
			double y;
			int w;
			int h;

			if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
				ErrPrint("Failed to get buffer size [%s:%s]\n", pkgname, id);
				goto out;
			}

			x = (double)data.info.pointer.x / (double)w;
			y = (double)data.info.pointer.y / (double)h;

			(void)info->handler(info, data.type, data.timestamp, x, y, info->data);
		}
	}

out:
	return NULL;
}

struct packet *provider_buffer_lb_mouse_leave(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_LEAVE,
	};

	if (packet_get(packet, "ssdii", &pkgname, &id, &data.timestamp, &data.info.pointer.x, &data.info.pointer.y) != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler || info->handler_NEW) {
		if (info->handler_NEW) {
			(void)info->handler_NEW(info, &data, info->data);
		} else if (info->handler) {
			int w;
			int h;
			double x;
			double y;

			if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
				ErrPrint("Failed to get buffer size [%s:%s]\n", pkgname, id);
				goto out;
			}

			x = (double)data.info.pointer.x / (double)w;
			y = (double)data.info.pointer.y / (double)h;

			(void)info->handler(info, data.type, data.timestamp, x, y, info->data);
		}
	}

out:
	return NULL;
}

struct packet *provider_buffer_lb_mouse_down(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_DOWN,
	};

	if (packet_get(packet, "ssdii", &pkgname, &id, &data.timestamp, &data.info.pointer.x, &data.info.pointer.y) != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler || info->handler_NEW) {
		if (info->handler_NEW) {
			(void)info->handler_NEW(info, &data, info->data);
		} else if (info->handler) {
			int w;
			int h;
			double x;
			double y;

			if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
				ErrPrint("Failed to get buffer size [%s:%s]\n", pkgname, id);
				goto out;
			}

			x = (double)data.info.pointer.x / (double)w;
			y = (double)data.info.pointer.y / (double)h;

			(void)info->handler(info, data.type, data.timestamp, x, y, info->data);
		}
	}

out:
	return NULL;
}

struct packet *provider_buffer_lb_mouse_up(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_UP,
	};

	if (packet_get(packet, "ssdii", &pkgname, &id, &data.timestamp, &data.info.pointer.x, &data.info.pointer.y) != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler || info->handler_NEW) {
		if (info->handler_NEW) {
			(void)info->handler_NEW(info, &data, info->data);
		} else if (info->handler) {
			int w;
			int h;
			double x;
			double y;

			if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
				ErrPrint("Failed to get buffer size [%s:%s]\n", pkgname, id);
				goto out;
			}

			x = (double)data.info.pointer.x / (double)w;
			y = (double)data.info.pointer.y / (double)h;

			(void)info->handler(info, data.type, data.timestamp, x, y, info->data);
		}
	}

out:
	return NULL;
}

struct packet *provider_buffer_lb_mouse_move(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_MOVE,
	};

	if (packet_get(packet, "ssdii", &pkgname, &id, &data.timestamp, &data.info.pointer.x, &data.info.pointer.y) != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler || info->handler_NEW) {
		if (info->handler_NEW) {
			(void)info->handler_NEW(info, &data, info->data);
		} else if (info->handler) {
			int w;
			int h;
			double x;
			double y;

			if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
				ErrPrint("Failed to get buffer size [%s:%s]\n", pkgname, id);
				goto out;
			}

			x = (double)data.info.pointer.x / (double)w;
			y = (double)data.info.pointer.y / (double)h;

			(void)info->handler(info, data.type, data.timestamp, x, y, info->data);
		}
	}

out:
	return NULL;
}

struct packet *provider_buffer_pd_key_down(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_KEY_DOWN,
	};

	if (packet_get(packet, "ssdi", &pkgname, &id, &data.timestamp, &data.info.keycode) != 4) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler_NEW) {
		(void)info->handler_NEW(info, &data, info->data);
	} else if (info->handler) {
		DbgPrint("Unsupported\n");
		(void)provider_send_key_status(pkgname, id, LB_KEY_STATUS_ERROR);
	} else {
		(void)provider_send_key_status(pkgname, id, LB_KEY_STATUS_ERROR);
	}

out:
	return NULL;
}

struct packet *provider_buffer_pd_key_up(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_KEY_UP,
	};

	if (packet_get(packet, "ssdi", &pkgname, &id, &data.timestamp, &data.info.keycode) != 4) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler_NEW) {
		(void)info->handler_NEW(info, &data, info->data);
	} else if (info->handler) {
		DbgPrint("Unsupported\n");
		(void)provider_send_key_status(pkgname, id, LB_KEY_STATUS_ERROR);
	} else {
		(void)provider_send_key_status(pkgname, id, LB_KEY_STATUS_ERROR);
	}

out:
	return NULL;
}

struct packet *provider_buffer_pd_key_focus_in(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_KEY_FOCUS_IN,
	};

	if (packet_get(packet, "ssdi", &pkgname, &id, &data.timestamp, &data.info.keycode) != 4) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler_NEW) {
		(void)info->handler_NEW(info, &data, info->data);
	} else if (info->handler) {
		DbgPrint("Unsupported\n");
		(void)provider_send_key_status(pkgname, id, LB_KEY_STATUS_ERROR);
	} else {
		(void)provider_send_key_status(pkgname, id, LB_KEY_STATUS_ERROR);
	}

out:
	return NULL;
}

struct packet *provider_buffer_pd_key_focus_out(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_KEY_FOCUS_OUT,
	};

	if (packet_get(packet, "ssdi", &pkgname, &id, &data.timestamp, &data.info.keycode) != 4) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler_NEW) {
		(void)info->handler_NEW(info, &data, info->data);
	} else if (info->handler) {
		DbgPrint("Unsupported\n");
		(void)provider_send_key_status(pkgname, id, LB_KEY_STATUS_ERROR);
	} else {
		(void)provider_send_key_status(pkgname, id, LB_KEY_STATUS_ERROR);
	}

out:
	return NULL;
}

struct packet *provider_buffer_pd_mouse_enter(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_ENTER,
	};

	if (packet_get(packet, "ssdii", &pkgname, &id, &data.timestamp, &data.info.pointer.x, &data.info.pointer.y) != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler || info->handler_NEW) {
		if (info->handler_NEW) {
			(void)info->handler_NEW(info, &data, info->data);
		} else if (info->handler) {
			int w;
			int h;
			double x;
			double y;

			if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
				ErrPrint("Failed to get buffer size [%s:%s]\n", pkgname, id);
				goto out;
			}

			x = (double)data.info.pointer.x / (double)w;
			y = (double)data.info.pointer.y / (double)h;

			(void)info->handler(info, data.type, data.timestamp, x, y, info->data);
		}
	}

out:
	return NULL;
}

struct packet *provider_buffer_pd_mouse_leave(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_LEAVE,
	};

	if (packet_get(packet, "ssdii", &pkgname, &id, &data.timestamp, &data.info.pointer.x, &data.info.pointer.y) != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler || info->handler_NEW) {
		if (info->handler_NEW) {
			(void)info->handler_NEW(info, &data, info->data);
		} else if (info->handler) {
			int w;
			int h;
			double x;
			double y;

			if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
				ErrPrint("Failed to get buffer size [%s:%s]\n", pkgname, id);
				goto out;
			}

			x = (double)data.info.pointer.x / (double)w;
			y = (double)data.info.pointer.y / (double)h;

			(void)info->handler(info, data.type, data.timestamp, x, y, info->data);
		}
	}

out:
	return NULL;
}

struct packet *provider_buffer_pd_mouse_down(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_DOWN,
	};

	if (packet_get(packet, "ssdii", &pkgname, &id, &data.timestamp, &data.info.pointer.x, &data.info.pointer.y) != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler || info->handler_NEW) {
		if (info->handler_NEW) {
			(void)info->handler_NEW(info, &data, info->data);
		} else if (info->handler) {
			int w;
			int h;
			double x;
			double y;

			if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
				ErrPrint("Failed to get buffer size [%s:%s]\n", pkgname, id);
				goto out;
			}

			x = (double)data.info.pointer.x / (double)w;
			y = (double)data.info.pointer.y / (double)h;

			(void)info->handler(info, data.type, data.timestamp, x, y, info->data);
		}
	}

out:
	return NULL;
}

struct packet *provider_buffer_pd_mouse_up(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_UP,
	};

	if (packet_get(packet, "ssdii", &pkgname, &id, &data.timestamp, &data.info.pointer.x, &data.info.pointer.y) != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler || info->handler_NEW) {
		if (info->handler_NEW) {
			(void)info->handler_NEW(info, &data, info->data);
		} else if (info->handler) {
			int w;
			int h;
			double x;
			double y;

			if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
				ErrPrint("Failed to get buffer size [%s:%s]\n", pkgname, id);
				goto out;
			}

			x = (double)data.info.pointer.x / (double)w;
			y = (double)data.info.pointer.y / (double)h;

			(void)info->handler(info, data.type, data.timestamp, x, y, info->data);
		}
	}

out:
	return NULL;
}

struct packet *provider_buffer_pd_mouse_move(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_MOVE,
	};

	if (packet_get(packet, "ssdii", &pkgname, &id, &data.timestamp, &data.info.pointer.x, &data.info.pointer.y) != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler || info->handler_NEW) {
		if (info->handler_NEW) {
			(void)info->handler_NEW(info, &data, info->data);
		} else if (info->handler) {
			int w;
			int h;
			double x;
			double y;

			if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
				ErrPrint("Failed to get buffer size [%s:%s]\n", pkgname, id);
				goto out;
			}

			x = (double)data.info.pointer.x / (double)w;
			y = (double)data.info.pointer.y / (double)h;

			(void)info->handler(info, data.type, data.timestamp, x, y, info->data);
		}
	}

out:
	return NULL;
}

struct packet *provider_buffer_pd_access_action_up(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	int ret;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_ACTION_UP,
	};

	ret = packet_get(packet, "ssdii", &pkgname, &id, &data.timestamp, &data.info.pointer.x, &data.info.pointer.y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler || info->handler_NEW) {
		if (info->handler_NEW) {
			(void)info->handler_NEW(info, &data, info->data);
		} else if (info->handler) {
			int w;
			int h;
			double x;
			double y;
			
			if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
				ErrPrint("Failed to get buffer size[%s:%s]\n", pkgname, id);
			}

			x = (double)data.info.pointer.x / (double)w;
			y = (double)data.info.pointer.y / (double)h;

			(void)info->handler(info, data.type, data.timestamp, x, y, info->data);
		}
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
	int ret;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_ACTION_DOWN,
	};

	ret = packet_get(packet, "ssdii", &pkgname, &id, &data.timestamp, &data.info.pointer.x, &data.info.pointer.y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler || info->handler_NEW) {
		if (info->handler_NEW) {
			(void)info->handler_NEW(info, &data, info->data);
		} else if (info->handler) {
			int w;
			int h;
			double x;
			double y;

			if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
				ErrPrint("Failed to get buffer size[%s:%s]\n", pkgname, id);
			}

			x = (double)data.info.pointer.x / (double)w;
			y = (double)data.info.pointer.y / (double)h;

			(void)info->handler(info, data.type, data.timestamp, x, y, info->data);
		}
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
	int ret;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_SCROLL_DOWN,
	};

	ret = packet_get(packet, "ssdii", &pkgname, &id, &data.timestamp, &data.info.pointer.x, &data.info.pointer.y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler || info->handler_NEW) {
		if (info->handler_NEW) {
			(void)info->handler_NEW(info, &data, info->data);
		} else if (info->handler) {
			int w;
			int h;
			double x;
			double y;

			if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
				ErrPrint("Failed to get buffer size[%s:%s]\n", pkgname, id);
			}

			x = (double)data.info.pointer.x / (double)w;
			y = (double)data.info.pointer.y / (double)h;

			(void)info->handler(info, data.type, data.timestamp, x, y, info->data);
		}
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
	int ret;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_SCROLL_MOVE,
	};

	ret = packet_get(packet, "ssdii", &pkgname, &id, &data.timestamp, &data.info.pointer.x, &data.info.pointer.y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler || info->handler_NEW) {
		if (info->handler_NEW) {
			(void)info->handler_NEW(info, &data, info->data);
		} else if (info->handler) {
			int w;
			int h;
			double x;
			double y;

			if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
				ErrPrint("Failed to get buffer size[%s:%s]\n", pkgname, id);
			}

			x = (double)data.info.pointer.x / (double)w;
			y = (double)data.info.pointer.y / (double)h;

			(void)info->handler(info, data.type, data.timestamp, x, y, info->data);
		}
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
	int ret;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_SCROLL_UP,
	};

	ret = packet_get(packet, "ssdii", &pkgname, &id, &data.timestamp, &data.info.pointer.x, &data.info.pointer.y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler || info->handler_NEW) {
		if (info->handler_NEW) {
			(void)info->handler_NEW(info, &data, info->data);
		} else if (info->handler) {
			int w;
			int h;
			double x;
			double y;

			if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
				ErrPrint("Failed to get buffer size[%s:%s]\n", pkgname, id);
			}

			x = (double)data.info.pointer.x / (double)w;
			y = (double)data.info.pointer.y / (double)h;

			(void)info->handler(info, data.type, data.timestamp, x, y, info->data);
		}
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
	int ret;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_UNHIGHLIGHT,
	};

	ret = packet_get(packet, "ssdii", &pkgname, &id, &data.timestamp, &data.info.pointer.x, &data.info.pointer.y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler || info->handler_NEW) {
		if (info->handler_NEW) {
			(void)info->handler_NEW(info, &data, info->data);
		} else if (info->handler) {
			int w;
			int h;
			double x;
			double y;

			if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
				ErrPrint("Failed to get buffer size[%s:%s]\n", pkgname, id);
			}

			x = (double)data.info.pointer.x / (double)w;
			y = (double)data.info.pointer.y / (double)h;

			(void)info->handler(info, data.type, data.timestamp, x, y, info->data);
		}
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
	int ret;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_HIGHLIGHT,
	};

	ret = packet_get(packet, "ssdii", &pkgname, &id, &data.timestamp, &data.info.pointer.x, &data.info.pointer.y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler || info->handler_NEW) {
		if (info->handler_NEW) {
			(void)info->handler_NEW(info, &data, info->data);
		} else if (info->handler) {
			int w;
			int h;
			double x;
			double y;

			if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
				ErrPrint("Failed to get buffer size[%s:%s]\n", pkgname, id);
			}

			x = (double)data.info.pointer.x / (double)w;
			y = (double)data.info.pointer.y / (double)h;

			(void)info->handler(info, data.type, data.timestamp, x, y, info->data);
		}
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
	int ret;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_HIGHLIGHT_PREV,
	};

	ret = packet_get(packet, "ssdii", &pkgname, &id, &data.timestamp, &data.info.pointer.x, &data.info.pointer.y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler || info->handler_NEW) {
		if (info->handler_NEW) {
			(void)info->handler_NEW(info, &data, info->data);
		} else if (info->handler) {
			int w;
			int h;
			double x;
			double y;

			if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
				ErrPrint("Failed to get buffer size[%s:%s]\n", pkgname, id);
			}

			x = (double)data.info.pointer.x / (double)w;
			y = (double)data.info.pointer.y / (double)h;

			(void)info->handler(info, data.type, data.timestamp, x, y, info->data);
		}
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
	int ret;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_HIGHLIGHT_NEXT,
	};

	ret = packet_get(packet, "ssdii", &pkgname, &id, &data.timestamp, &data.info.pointer.x, &data.info.pointer.y);
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
		if (info->handler_NEW) {
			(void)info->handler_NEW(info, &data, info->data);
		} else if (info->handler) {
			int w;
			int h;
			double x;
			double y;

			if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
				ErrPrint("Failed to get buffer size[%s:%s]\n", pkgname, id);
			}

			x = (double)data.info.pointer.x / (double)w;
			y = (double)data.info.pointer.y / (double)h;

			(void)info->handler(info, data.type, data.timestamp, x, y, info->data);
		}
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
	int ret;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_ACTIVATE,
	};

	ret = packet_get(packet, "ssdii", &pkgname, &id, &data.timestamp, &data.info.pointer.x, &data.info.pointer.y);
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
		if (info->handler_NEW) {
			(void)info->handler_NEW(info, &data, info->data);
		} else if (info->handler) {
			int w;
			int h;
			double x;
			double y;

			if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
				ErrPrint("Failed to get buffer size[%s:%s]\n", pkgname, id);
			}

			x = (double)data.info.pointer.x / (double)w;
			y = (double)data.info.pointer.y / (double)h;

			(void)info->handler(info, data.type, data.timestamp, x, y, info->data);
		}
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
	int ret;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_UNHIGHLIGHT,
	};

	ret = packet_get(packet, "ssdii", &pkgname, &id, &data.timestamp, &data.info.pointer.x, &data.info.pointer.y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler || info->handler_NEW) {
		if (info->handler_NEW) {
			(void)info->handler_NEW(info, &data, info->data);
		} else if (info->handler) {
			int w;
			int h;
			double x;
			double y;

			if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
				ErrPrint("Failed to get buffer size[%s:%s]\n", pkgname, id);
			}

			x = (double)data.info.pointer.x / (double)w;
			y = (double)data.info.pointer.y / (double)h;

			(void)info->handler(info, data.type, data.timestamp, x, y, info->data);
		}
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
	int ret;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_HIGHLIGHT,
	};

	ret = packet_get(packet, "ssdii", &pkgname, &id, &data.timestamp, &data.info.pointer.x, &data.info.pointer.y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler || info->handler_NEW) {
		if (info->handler_NEW) {
			(void)info->handler_NEW(info, &data, info->data);
		} else if (info->handler) {
			int w;
			int h;
			double x;
			double y;

			if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
				ErrPrint("Failed to get buffer size[%s:%s]\n", pkgname, id);
			}

			x = (double)data.info.pointer.x / (double)w;
			y = (double)data.info.pointer.y / (double)h;

			(void)info->handler(info, data.type, data.timestamp, x, y, info->data);
		}
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
	int ret;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_HIGHLIGHT_PREV,
	};

	ret = packet_get(packet, "ssdii", &pkgname, &id, &data.timestamp, &data.info.pointer.x, &data.info.pointer.y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler || info->handler_NEW) {
		if (info->handler_NEW) {
			(void)info->handler_NEW(info, &data, info->data);
		} else if (info->handler) {
			int w;
			int h;
			double x;
			double y;

			if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
				ErrPrint("Failed to get buffer size[%s:%s]\n", pkgname, id);
			}

			x = (double)data.info.pointer.x / (double)w;
			y = (double)data.info.pointer.y / (double)h;

			(void)info->handler(info, data.type, data.timestamp, x, y, info->data);
		}
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
	int ret;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_HIGHLIGHT_NEXT,
	};

	ret = packet_get(packet, "ssdii", &pkgname, &id, &data.timestamp, &data.info.pointer.x, &data.info.pointer.y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler || info->handler_NEW) {
		if (info->handler_NEW) {
			(void)info->handler_NEW(info, &data, info->data);
		} else if (info->handler) {
			int w;
			int h;
			double x;
			double y;

			if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
				ErrPrint("Failed to get buffer size[%s:%s]\n", pkgname, id);
			}

			x = (double)data.info.pointer.x / (double)w;
			y = (double)data.info.pointer.y / (double)h;

			(void)info->handler(info, data.type, data.timestamp, x, y, info->data);
		}
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
	int ret;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_ACTION_UP,
	};

	ret = packet_get(packet, "ssdii", &pkgname, &id, &data.timestamp, &data.info.pointer.x, &data.info.pointer.y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler || info->handler_NEW) {
		if (info->handler_NEW) {
			(void)info->handler_NEW(info, &data, info->data);
		} else if (info->handler) {
			int w;
			int h;
			double x;
			double y;

			if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
				ErrPrint("Failed to get buffer size[%s:%s]\n", pkgname, id);
			}

			x = (double)data.info.pointer.x / (double)w;
			y = (double)data.info.pointer.y / (double)h;

			(void)info->handler(info, data.type, data.timestamp, x, y, info->data);
		}
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
	int ret;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_ACTION_DOWN,
	};

	ret = packet_get(packet, "ssdii", &pkgname, &id, &data.timestamp, &data.info.pointer.x, &data.info.pointer.y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler || info->handler_NEW) {
		if (info->handler_NEW) {
			(void)info->handler_NEW(info, &data, info->data);
		} else if (info->handler) {
			int w;
			int h;
			double x;
			double y;

			if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
				ErrPrint("Failed to get buffer size[%s:%s]\n", pkgname, id);
			}

			x = (double)data.info.pointer.x / (double)w;
			y = (double)data.info.pointer.y / (double)h;

			(void)info->handler(info, data.type, data.timestamp, x, y, info->data);
		}
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
	int ret;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_SCROLL_DOWN,
	};

	ret = packet_get(packet, "ssdii", &pkgname, &id, &data.timestamp, &data.info.pointer.x, &data.info.pointer.y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler || info->handler_NEW) {
		if (info->handler_NEW) {
			(void)info->handler_NEW(info, &data, info->data);
		} else if (info->handler) {
			int w;
			int h;
			double x;
			double y;

			if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
				ErrPrint("Failed to get buffer size[%s:%s]\n", pkgname, id);
			}

			x = (double)data.info.pointer.x / (double)w;
			y = (double)data.info.pointer.y / (double)h;

			(void)info->handler(info, data.type, data.timestamp, x, y, info->data);
		}
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
	int ret;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_SCROLL_MOVE,
	};

	ret = packet_get(packet, "ssdii", &pkgname, &id, &data.timestamp, &data.info.pointer.x, &data.info.pointer.y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler || info->handler_NEW) {
		if (info->handler_NEW) {
			(void)info->handler_NEW(info, &data, info->data);
		} else if (info->handler) {
			int w;
			int h;
			double x;
			double y;

			if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
				ErrPrint("Failed to get buffer size[%s:%s]\n", pkgname, id);
			}

			x = (double)data.info.pointer.x / (double)w;
			y = (double)data.info.pointer.y / (double)h;

			(void)info->handler(info, data.type, data.timestamp, x, y, info->data);
		}
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
	int ret;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_SCROLL_UP,
	};

	ret = packet_get(packet, "ssdii", &pkgname, &id, &data.timestamp, &data.info.pointer.x, &data.info.pointer.y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler || info->handler_NEW) {
		if (info->handler_NEW) {
			(void)info->handler_NEW(info, &data, info->data);
		} else if (info->handler) {
			int w;
			int h;
			double x;
			double y;

			if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
				ErrPrint("Failed to get buffer size[%s:%s]\n", pkgname, id);
			}

			x = (double)data.info.pointer.x / (double)w;
			y = (double)data.info.pointer.y / (double)h;

			(void)info->handler(info, data.type, data.timestamp, x, y, info->data);
		}
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
	int ret;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_ACTIVATE,
	};

	ret = packet_get(packet, "ssdii", &pkgname, &id, &data.timestamp, &data.info.pointer.x, &data.info.pointer.y);
	if (ret != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler || info->handler_NEW) {
		if (info->handler_NEW) {
			(void)info->handler_NEW(info, &data, info->data);
		} else if (info->handler) {
			int w;
			int h;
			double x;
			double y;

			if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
				ErrPrint("Failed to get buffer size[%s:%s]\n", pkgname, id);
			}

			x = (double)data.info.pointer.x / (double)w;
			y = (double)data.info.pointer.y / (double)h;

			(void)info->handler(info, data.type, data.timestamp, x, y, info->data);
		}
	} else {
		/* Event handler is not ready */
		(void)provider_send_access_status(pkgname, id, LB_ACCESS_STATUS_ERROR);
	}

out:
	return NULL;
}

struct packet *provider_buffer_lb_mouse_on_hold(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_ON_HOLD,
	};

	if (packet_get(packet, "ssdii", &pkgname, &id, &data.timestamp, &data.info.pointer.x, &data.info.pointer.y) != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler || info->handler_NEW) {
		if (info->handler_NEW) {
			(void)info->handler_NEW(info, &data, info->data);
		} else if (info->handler) {
			double x;
			double y;
			int w;
			int h;

			if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
				ErrPrint("Failed to get buffer size [%s:%s]\n", pkgname, id);
				goto out;
			}

			x = (double)data.info.pointer.x / (double)w;
			y = (double)data.info.pointer.y / (double)h;

			(void)info->handler(info, data.type, data.timestamp, x, y, info->data);
		}
	}

out:
	return NULL;
}

struct packet *provider_buffer_lb_mouse_off_hold(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_OFF_HOLD,
	};

	if (packet_get(packet, "ssdii", &pkgname, &id, &data.timestamp, &data.info.pointer.x, &data.info.pointer.y) != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler || info->handler_NEW) {
		if (info->handler_NEW) {
			(void)info->handler_NEW(info, &data, info->data);
		} else if (info->handler) {
			double x;
			double y;
			int w;
			int h;

			if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
				ErrPrint("Failed to get buffer size [%s:%s]\n", pkgname, id);
				goto out;
			}

			x = (double)data.info.pointer.x / (double)w;
			y = (double)data.info.pointer.y / (double)h;

			(void)info->handler(info, data.type, data.timestamp, x, y, info->data);
		}
	}

out:
	return NULL;
}

struct packet *provider_buffer_lb_mouse_on_scroll(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_ON_SCROLL,
	};

	if (packet_get(packet, "ssdii", &pkgname, &id, &data.timestamp, &data.info.pointer.x, &data.info.pointer.y) != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler || info->handler_NEW) {
		if (info->handler_NEW) {
			(void)info->handler_NEW(info, &data, info->data);
		} else if (info->handler) {
			double x;
			double y;
			int w;
			int h;

			if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
				ErrPrint("Failed to get buffer size [%s:%s]\n", pkgname, id);
				goto out;
			}

			x = (double)data.info.pointer.x / (double)w;
			y = (double)data.info.pointer.y / (double)h;

			(void)info->handler(info, data.type, data.timestamp, x, y, info->data);
		}
	}

out:
	return NULL;
}

struct packet *provider_buffer_lb_mouse_off_scroll(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_OFF_SCROLL,
	};

	if (packet_get(packet, "ssdii", &pkgname, &id, &data.timestamp, &data.info.pointer.x, &data.info.pointer.y) != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_LB, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler || info->handler_NEW) {
		if (info->handler_NEW) {
			(void)info->handler_NEW(info, &data, info->data);
		} else if (info->handler) {
			double x;
			double y;
			int w;
			int h;

			if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
				ErrPrint("Failed to get buffer size [%s:%s]\n", pkgname, id);
				goto out;
			}

			x = (double)data.info.pointer.x / (double)w;
			y = (double)data.info.pointer.y / (double)h;

			(void)info->handler(info, data.type, data.timestamp, x, y, info->data);
		}
	}

out:
	return NULL;
}

struct packet *provider_buffer_pd_mouse_on_hold(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_ON_HOLD,
	};

	if (packet_get(packet, "ssdii", &pkgname, &id, &data.timestamp, &data.info.pointer.x, &data.info.pointer.y) != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler || info->handler_NEW) {
		if (info->handler_NEW) {
			(void)info->handler_NEW(info, &data, info->data);
		} else if (info->handler) {
			int w;
			int h;
			double x;
			double y;

			if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
				ErrPrint("Failed to get buffer size [%s:%s]\n", pkgname, id);
				goto out;
			}

			x = (double)data.info.pointer.x / (double)w;
			y = (double)data.info.pointer.y / (double)h;

			(void)info->handler(info, data.type, data.timestamp, x, y, info->data);
		}
	}

out:
	return NULL;
}

struct packet *provider_buffer_pd_mouse_off_hold(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_OFF_HOLD,
	};

	if (packet_get(packet, "ssdii", &pkgname, &id, &data.timestamp, &data.info.pointer.x, &data.info.pointer.y) != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler || info->handler_NEW) {
		if (info->handler_NEW) {
			(void)info->handler_NEW(info, &data, info->data);
		} else if (info->handler) {
			int w;
			int h;
			double x;
			double y;

			if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
				ErrPrint("Failed to get buffer size [%s:%s]\n", pkgname, id);
				goto out;
			}

			x = (double)data.info.pointer.x / (double)w;
			y = (double)data.info.pointer.y / (double)h;

			(void)info->handler(info, data.type, data.timestamp, x, y, info->data);
		}
	}

out:
	return NULL;
}

struct packet *provider_buffer_pd_mouse_on_scroll(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_ON_SCROLL,
	};

	if (packet_get(packet, "ssdii", &pkgname, &id, &data.timestamp, &data.info.pointer.x, &data.info.pointer.y) != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler || info->handler_NEW) {
		if (info->handler_NEW) {
			(void)info->handler_NEW(info, &data, info->data);
		} else if (info->handler) {
			int w;
			int h;
			double x;
			double y;

			if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
				ErrPrint("Failed to get buffer size [%s:%s]\n", pkgname, id);
				goto out;
			}

			x = (double)data.info.pointer.x / (double)w;
			y = (double)data.info.pointer.y / (double)h;

			(void)info->handler(info, data.type, data.timestamp, x, y, info->data);
		}
	}

out:
	return NULL;
}

struct packet *provider_buffer_pd_mouse_off_scroll(pid_t pid, int handle, const struct packet *packet)
{
	const char *pkgname;
	const char *id;
	struct livebox_buffer *info;
	struct buffer_event_data data = {
		.type = BUFFER_EVENT_OFF_SCROLL,
	};

	if (packet_get(packet, "ssdii", &pkgname, &id, &data.timestamp, &data.info.pointer.x, &data.info.pointer.y) != 5) {
		ErrPrint("Invalid packet\n");
		goto out;
	}

	info = provider_buffer_find_buffer(TYPE_PD, pkgname, id);
	if (!info) {
		ErrPrint("Failed to find a buffer [%s:%s]\n", pkgname, id);
		goto out;
	}

	if (info->handler || info->handler_NEW) {
		if (info->handler_NEW) {
			(void)info->handler_NEW(info, &data, info->data);
		} else if (info->handler) {
			int w;
			int h;
			double x;
			double y;

			if (provider_buffer_get_size(info, &w, &h, NULL) < 0) {
				ErrPrint("Failed to get buffer size [%s:%s]\n", pkgname, id);
				goto out;
			}

			x = (double)data.info.pointer.x / (double)w;
			y = (double)data.info.pointer.y / (double)h;

			(void)info->handler(info, data.type, data.timestamp, x, y, info->data);
		}
	}

out:
	return NULL;
}

/* End of a file */
