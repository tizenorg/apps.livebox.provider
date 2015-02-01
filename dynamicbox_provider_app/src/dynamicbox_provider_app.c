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
#include <stdbool.h>

#include <app.h>
#include <dlog.h>

#include <dynamicbox_conf.h>
#include <dynamicbox_errno.h>

#include <Ecore.h>
#include <dynamicbox_provider.h>

#include "dynamicbox_provider_buffer.h"
#include "dynamicbox_provider_app.h"
#include "dynamicbox_provider_app_internal.h"

#include "client.h"
#include "util.h"
#include "debug.h"

int errno;

static struct info {
    int initialized;
    char *pkgname;
} s_info = {
    .initialized = 0,
    .pkgname = NULL,
};

const char *provider_pkgname(void)
{
    return s_info.pkgname;
}

PAPI int dynamicbox_provider_app_is_initialized(void)
{
    return client_is_initialized();
}

PAPI int dynamicbox_provider_app_init(app_control_h service, dynamicbox_provider_event_callback_t table)
{
    int ret;
    char *pkgname;

    if (app_get_id(&pkgname) != APP_ERROR_NONE || pkgname == NULL) {
        ErrPrint("Failed to get app package name\n");
        return DBOX_STATUS_ERROR_FAULT;
    }

    ret = client_init(service, table);
    if (ret < 0) {
        ErrPrint("Client init returns: 0x%X\n", ret);
        free(pkgname);
        return ret;
    }

    DbgPrint("Package: %s\n", pkgname);
    if (s_info.pkgname) {
        free(s_info.pkgname);
        s_info.pkgname = NULL;
    }

    s_info.pkgname = pkgname;
    return DBOX_STATUS_ERROR_NONE;
}

PAPI int dynamicbox_provider_app_manual_control(int flag)
{
    if (flag) {
        flag = DBOX_PROVIDER_CTRL_MANUAL_REACTIVATION | DBOX_PROVIDER_CTRL_MANUAL_TERMINATION;
    }

    return dynamicbox_provider_control(flag);
}

PAPI dynamicbox_buffer_h dynamicbox_provider_app_create_buffer(const char *id, int for_pd, int (*dbox_handler)(dynamicbox_buffer_h , dynamicbox_buffer_event_data_t, void *), void *data)
{
    dynamicbox_buffer_h handle;
    char *uri;

    uri = util_path_to_uri(id);
    if (!uri) {
        ErrPrint("Failed to conver uri from id\n");
        return NULL;
    }

    handle = dynamicbox_provider_buffer_create(for_pd ? DBOX_TYPE_GBAR : DBOX_TYPE_DBOX, s_info.pkgname, uri, DYNAMICBOX_CONF_AUTO_ALIGN, dbox_handler, data);
    free(uri);
    if (!handle) {
        ErrPrint("Unable to create buffer (%s)\n", id);
        return NULL;
    }

    return handle;
}

PAPI int dynamicbox_provider_app_acquire_buffer(dynamicbox_buffer_h handle, int w, int h, int bpp)
{
    return dynamicbox_provider_buffer_acquire(handle, w, h, bpp);
}

PAPI int dynamicbox_provider_app_acquire_extra_buffer(dynamicbox_buffer_h handle, int idx, int w, int h, int bpp)
{
    return dynamicbox_provider_buffer_extra_acquire(handle, idx, w, h, bpp);
}

PAPI int dynamicbox_provider_app_release_extra_buffer(dynamicbox_buffer_h handle, int idx)
{
    return dynamicbox_provider_buffer_extra_release(handle, idx);
}

PAPI int dynamicbox_provider_app_release_buffer(dynamicbox_buffer_h handle)
{
    return dynamicbox_provider_buffer_release(handle);
}

PAPI int dynamicbox_provider_app_destroy_buffer(dynamicbox_buffer_h handle)
{
    return dynamicbox_provider_buffer_destroy(handle);
}

PAPI int dynamicbox_provider_app_send_access_status(dynamicbox_buffer_h handle, int status)
{
    return dynamicbox_provider_send_access_status(dynamicbox_provider_buffer_pkgname(handle), dynamicbox_provider_buffer_id(handle), status);
}

PAPI int dynamicbox_provider_app_send_key_status(dynamicbox_buffer_h handle, int status)
{
    return dynamicbox_provider_send_key_status(dynamicbox_provider_buffer_pkgname(handle), dynamicbox_provider_buffer_id(handle), status);
}

PAPI unsigned int dynamicbox_provider_app_buffer_resource_id(dynamicbox_buffer_h handle)
{
    return dynamicbox_provider_buffer_resource_id(handle);
}

PAPI unsigned int dynamicbox_provider_app_extra_buffer_resource_id(dynamicbox_buffer_h handle, int idx)
{
    return dynamicbox_provider_buffer_extra_resource_id(handle, idx);
}

PAPI int dynamicbox_provider_app_hold_scroll(const char *id, int seize)
{
    char *uri;
    int ret;

    uri = util_path_to_uri(id);
    if (!uri) {
        ErrPrint("Failed to convert uri from id\n");
        return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    ret = dynamicbox_provider_send_hold_scroll(s_info.pkgname, uri, seize);
    free(uri);
    return ret;
}

PAPI void dynamicbox_provider_app_fini(void)
{
    client_fini();
    free(s_info.pkgname);
    s_info.pkgname = NULL;
}

/* End of a file */
