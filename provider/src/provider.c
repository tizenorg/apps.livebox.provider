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
#include <dynamicbox_service.h>
#include <dynamicbox_buffer.h>

#include "dlist.h"
#include "util.h"
#include "dynamicbox_provider.h"
#include "provider.h"
#include "provider_buffer.h"
#include "debug.h"

#define EAPI __attribute__((visibility("default")))

EAPI int provider_init(void *disp, const char *name, struct event_handler *table, void *data)
{
    return DBOX_STATUS_ERROR_NONE;
}

EAPI int provider_init_with_options(void *disp, const char *name, struct event_handler *table, void *data, int prevent_overwrite, int com_core_use_thread)
{
    return dynamicbox_provider_init(disp, name, (dynamicbox_event_table_h)table, data, prevent_overwrite, com_core_use_thread);
}

EAPI void *provider_fini(void)
{
    return dynamicbox_provider_fini();
}

EAPI int provider_send_hello(void)
{
    return dynamicbox_provider_send_hello();
}

EAPI int provider_send_ping(void)
{
    return dynamicbox_provider_send_ping();
}

EAPI int provider_send_updated(const char *pkgname, const char *id, int w, int h)
{
    dynamicbox_damage_region_t region = {
	.x = 0,
	.y = 0,
	.w = w,
	.h = h,
    };

    return dynamicbox_provider_send_updated(pkgname, id, DBOX_PRIMARY_BUFFER, &region, 0, NULL);
}

EAPI int provider_send_direct_updated(int fd, const char *pkgname, const char *id, int w, int h)
{
    dynamicbox_damage_region_t region = {
	.x = 0,
	.y = 0,
	.w = w,
	.h = h,
    };

    return dynamicbox_provider_send_direct_updated(fd, pkgname, id, DBOX_PRIMARY_BUFFER, &region, 0, NULL);
}

EAPI int provider_send_extra_info(const char *pkgname, const char *id, double priority, const char *content_info, const char *title, const char *icon, const char *name)
{
    return dynamicbox_provider_send_extra_info(pkgname, id, priority, content_info, title, icon, name);
}

EAPI int provider_send_desc_updated(const char *pkgname, const char *id, const char *descfile)
{
    return dynamicbox_provider_send_updated(pkgname, id, DBOX_PRIMARY_BUFFER, NULL, 1, descfile);
}

EAPI int provider_send_direct_desc_updated(int fd, const char *pkgname, const char *id, const char *descfile)
{
    return dynamicbox_provider_send_direct_updated(fd, pkgname, id, DBOX_PRIMARY_BUFFER, NULL, 1, descfile);
}

EAPI int provider_send_lb_update_begin(const char *pkgname, const char *id, double priority, const char *content_info, const char *title)
{
    return dynamicbox_provider_send_dbox_update_begin(pkgname, id, priority, content_info, title);
}

EAPI int provider_send_lb_update_end(const char *pkgname, const char *id)
{
    return dynamicbox_provider_send_dbox_update_end(pkgname, id);
}

EAPI int provider_send_pd_update_begin(const char *pkgname, const char *id)
{
    return dynamicbox_provider_send_gbar_update_begin(pkgname, id);
}

EAPI int provider_send_pd_update_end(const char *pkgname, const char *id)
{
    return dynamicbox_provider_send_gbar_update_end(pkgname, id);
}

EAPI int provider_send_deleted(const char *pkgname, const char *id)
{
    return dynamicbox_provider_send_deleted(pkgname, id);
}

EAPI int provider_send_ret(const char *pkgname, const char *id, const char *funcname)
{
    return dynamicbox_provider_send_ret(pkgname, id, funcname);
}

EAPI int provider_send_call(const char *pkgname, const char *id, const char *funcname)
{
    return dynamicbox_provider_send_call(pkgname, id, funcname);
}

EAPI int provider_send_faulted(const char *pkgname, const char *id, const char *funcname)
{
    return dynamicbox_provider_send_faulted(pkgname, id, funcname);
}

EAPI int provider_send_hold_scroll(const char *pkgname, const char *id, int seize)
{
    return dynamicbox_provider_send_hold_scroll(pkgname, id, seize);
}

EAPI int provider_send_access_status(const char *pkgname, const char *id, int status)
{
    return dynamicbox_provider_send_access_status(pkgname, id, status);
}

EAPI int provider_send_key_status(const char *pkgname, const char *id, int status)
{
    return dynamicbox_provider_send_key_status(pkgname, id, status);
}

EAPI int provider_send_request_close_pd(const char *pkgname, const char *id, int reason)
{
    return dynamicbox_provider_send_request_close_gbar(pkgname, id, reason);
}

EAPI int provider_control(int ctrl)
{
    return dynamicbox_provider_control(ctrl);
}

/* End of a file */
