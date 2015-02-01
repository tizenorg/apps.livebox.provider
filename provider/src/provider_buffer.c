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
#include <dynamicbox_errno.h>
#include <dynamicbox_service.h>
#include <dynamicbox_provider.h>
#include <dynamicbox_provider_buffer.h>

#include "dlist.h"
#include "util.h"
#include "debug.h"
#include "provider.h"
#include "provider_buffer.h"

#define EAPI __attribute__((visibility("default")))

EAPI struct livebox_buffer *provider_buffer_create(enum target_type type, const char *pkgname, const char *id, int auto_align, int (*handler)(struct livebox_buffer *, struct buffer_event_data *, void *), void *data)
{
    dynamicbox_buffer_h handle;
    int (*_handler)(dynamicbox_buffer_h, dynamicbox_buffer_event_data_t, void *) = (void *)handler;
    
    handle = dynamicbox_provider_buffer_create((dynamicbox_target_type_e)type, pkgname, id, auto_align, _handler, data);

    return (struct livebox_buffer *)handle;
}

EAPI int provider_buffer_acquire(struct livebox_buffer *info, int width, int height, int pixel_size)
{
    return dynamicbox_provider_buffer_acquire((dynamicbox_buffer_h)info, width, height, pixel_size);
}

EAPI int provider_buffer_resize(struct livebox_buffer *info, int w, int h)
{
    return dynamicbox_provider_buffer_resize((dynamicbox_buffer_h)info, w, h);
}

EAPI void *provider_buffer_ref(struct livebox_buffer *info)
{
    return dynamicbox_provider_buffer_ref((dynamicbox_buffer_h)info);
}

EAPI int provider_buffer_unref(void *ptr)
{
    return dynamicbox_provider_buffer_unref(ptr);
}

EAPI int provider_buffer_release(struct livebox_buffer *info)
{
    return dynamicbox_provider_buffer_release((dynamicbox_buffer_h)info);
}

EAPI int provider_buffer_destroy(struct livebox_buffer *info)
{
    return dynamicbox_provider_buffer_destroy((dynamicbox_buffer_h)info);
}

EAPI int provider_buffer_sync(struct livebox_buffer *info)
{
    return dynamicbox_provider_buffer_sync((dynamicbox_buffer_h)info);
}

EAPI enum target_type provider_buffer_type(struct livebox_buffer *info)
{
    return (enum target_type)dynamicbox_provider_buffer_type((dynamicbox_buffer_h)info);
}

EAPI const char *provider_buffer_pkgname(struct livebox_buffer *info)
{
    return dynamicbox_provider_buffer_pkgname((dynamicbox_buffer_h)info);
}

EAPI const char *provider_buffer_id(struct livebox_buffer *info)
{
    return dynamicbox_provider_buffer_id((dynamicbox_buffer_h)info);
}

EAPI const char *provider_buffer_uri(struct livebox_buffer *info)
{
    return dynamicbox_provider_buffer_uri((dynamicbox_buffer_h)info);
}

EAPI int provider_buffer_get_size(struct livebox_buffer *info, int *w, int *h, int *pixel_size)
{
    return dynamicbox_provider_buffer_get_size((dynamicbox_buffer_h)info, w, h, pixel_size);
}

EAPI unsigned long provider_buffer_pixmap_id(struct livebox_buffer *info)
{
    return (unsigned long)dynamicbox_provider_buffer_resource_id((dynamicbox_buffer_h)info);
}

EAPI int provider_buffer_init(void *disp)
{
    return dynamicbox_provider_buffer_init(disp);
}

EAPI int provider_buffer_fini(void)
{
    return dynamicbox_provider_buffer_fini();
}

EAPI int provider_buffer_pixmap_is_support_hw(struct livebox_buffer *info)
{
    return dynamicbox_provider_buffer_is_support_hw((dynamicbox_buffer_h)info);
}

EAPI int provider_buffer_pixmap_create_hw(struct livebox_buffer *info)
{
    return dynamicbox_provider_buffer_create_hw((dynamicbox_buffer_h)info);
}

EAPI int provider_buffer_pixmap_destroy_hw(struct livebox_buffer *info)
{
    return dynamicbox_provider_buffer_destroy_hw((dynamicbox_buffer_h)info);
}

EAPI void *provider_buffer_pixmap_hw_addr(struct livebox_buffer *info)
{
    return dynamicbox_provider_buffer_hw_addr((dynamicbox_buffer_h)info);
}

EAPI int provider_buffer_pre_render(struct livebox_buffer *info)
{
    return dynamicbox_provider_buffer_pre_render((dynamicbox_buffer_h)info);
}

EAPI int provider_buffer_post_render(struct livebox_buffer *info)
{
    return dynamicbox_provider_buffer_post_render((dynamicbox_buffer_h)info);
}

EAPI void *provider_buffer_user_data(struct livebox_buffer *handle)
{
    return dynamicbox_provider_buffer_user_data((dynamicbox_buffer_h)handle);
}

EAPI int provider_buffer_set_user_data(struct livebox_buffer *handle, void *data)
{
    return dynamicbox_provider_buffer_set_user_data((dynamicbox_buffer_h)handle, data);
}

EAPI struct livebox_buffer *provider_buffer_find_buffer(enum target_type type, const char *pkgname, const char *id)
{
    return (struct livebox_buffer *)dynamicbox_provider_buffer_find_buffer(type, pkgname, id);
}

EAPI int provider_buffer_stride(struct livebox_buffer *info)
{
    return dynamicbox_provider_buffer_stride((dynamicbox_buffer_h)info);
}

/* End of a file */
