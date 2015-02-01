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

#include <dlog.h>

#include <app.h>
#include <dynamicbox_errno.h>

#include "provider.h"
#include "provider-app.h"
#include "provider-app_internal.h"
#include "debug.h"
#include "client.h"
#include "dynamicbox.h"

PAPI const char *dynamicbox_find_pkgname(const char *uri)
{
    return provider_pkgname();
}

PAPI int dynamicbox_request_update_by_id(const char *uri)
{
    return DBOX_STATUS_ERROR_DISABLED;
}

PAPI int dynamicbox_tirgger_update_monitor(const char *id, int is_pd)
{
    return DBOX_STATUS_ERROR_DISABLED;
}

PAPI int dynamicbox_update_extra_info(const char *id, const char *content, const char *title, const char *icon, const char *name)
{
    return DBOX_STATUS_ERROR_NONE;
}

/* End of a file */
