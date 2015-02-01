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
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <stddef.h>
#include <sys/shm.h>
#include <sys/ipc.h>

#include <dlog.h>
#include <dynamicbox_errno.h>
#include <dynamicbox_buffer.h>

#include "debug.h"
#include "util.h"
#include "dynamicbox_provider.h"
#include "dynamicbox_provider_buffer.h"
#include "provider_buffer_internal.h"
#include "fb.h"
#include "dlist.h"

int errno;

struct fb_info {
    char *id;
    int w;
    int h;
    int bufsz;
    void *buffer;

    int pixels;
    int handle;
};

int fb_init(void *disp)
{
    return DBOX_STATUS_ERROR_NONE;
}

int fb_fini(void)
{
    return DBOX_STATUS_ERROR_NONE;
}

static inline int sync_for_file(struct fb_info *info)
{
    int fd;
    dynamicbox_fb_t *buffer;

    buffer = info->buffer;
    if (!buffer) {
        DbgPrint("Buffer is NIL, skip sync\n");
        return DBOX_STATUS_ERROR_NONE;
    }

    if (buffer->state != CREATED) {
        ErrPrint("Invalid state of a FB\n");
        return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    if (buffer->type != BUFFER_TYPE_FILE) {
        DbgPrint("Ingore sync\n");
        return DBOX_STATUS_ERROR_NONE;
    }

    fd = open(util_uri_to_path(info->id), O_WRONLY | O_CREAT, 0644);
    if (fd < 0) {
        ErrPrint("open %s: %s\n", util_uri_to_path(info->id), strerror(errno));
        return DBOX_STATUS_ERROR_IO_ERROR;
    }

    if (write(fd, buffer->data, info->bufsz) != info->bufsz) {
        ErrPrint("write: %s\n", strerror(errno));
        if (close(fd) < 0) {
            ErrPrint("close: %s\n", strerror(errno));
        }
        return DBOX_STATUS_ERROR_IO_ERROR;
    }

    if (close(fd) < 0) {
        ErrPrint("close: %s\n", strerror(errno));
    }
    return DBOX_STATUS_ERROR_NONE;
}

int fb_sync(struct fb_info *info)
{
    if (!info) {
        ErrPrint("FB Handle is not valid\n");
        return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    if (!info->id || info->id[0] == '\0') {
        DbgPrint("Ingore sync\n");
        return DBOX_STATUS_ERROR_NONE;
    }

    if (!strncasecmp(info->id, SCHEMA_FILE, strlen(SCHEMA_FILE))) {
        return sync_for_file(info);
    } else if (!strncasecmp(info->id, SCHEMA_PIXMAP, strlen(SCHEMA_PIXMAP))) {
    } else if (!strncasecmp(info->id, SCHEMA_SHM, strlen(SCHEMA_SHM))) {
        return DBOX_STATUS_ERROR_NONE;
    }

    ErrPrint("Invalid URI: [%s]\n", info->id);
    return DBOX_STATUS_ERROR_INVALID_PARAMETER;
}

struct fb_info *fb_create(const char *id, int w, int h)
{
    struct fb_info *info;

    if (!id || id[0] == '\0') {
        ErrPrint("Invalid id\n");
        return NULL;
    }

    info = calloc(1, sizeof(*info));
    if (!info) {
        ErrPrint("Heap: %s\n", strerror(errno));
        return NULL;
    }

    info->id = strdup(id);
    if (!info->id) {
        ErrPrint("Heap: %s\n", strerror(errno));
        free(info);
        return NULL;
    }

    info->pixels = sizeof(int);

    if (sscanf(info->id, SCHEMA_SHM "%d", &info->handle) == 1) {
        DbgPrint("SHMID: %d\n", info->handle);
    } else if (sscanf(info->id, SCHEMA_PIXMAP "%d:%d", &info->handle, &info->pixels) == 2) {
        ErrPrint("Unsupported type\n");
        free(info->id);
        free(info);
        return NULL;
    } else if (!strncasecmp(info->id, SCHEMA_FILE, strlen(SCHEMA_FILE))) {
        info->handle = -1;
    } else {
        ErrPrint("Unsupported schema: %s\n", info->id);
        free(info->id);
        free(info);
        return NULL;
    }

    info->w = w;
    info->h = h;

    return info;
}

int fb_destroy(struct fb_info *info)
{
    if (!info) {
        ErrPrint("Handle is not valid\n");
        return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    if (info->buffer) {
        dynamicbox_fb_t *buffer;
        buffer = info->buffer;
        buffer->info = NULL;
    }

    free(info->id);
    free(info);
    return DBOX_STATUS_ERROR_NONE;
}

int fb_is_created(struct fb_info *info)
{
    if (!info) {
        ErrPrint("Handle is not valid\n");
        return 0;
    }

    if (!strncasecmp(info->id, SCHEMA_PIXMAP, strlen(SCHEMA_PIXMAP)) && info->handle != 0) {
        /* Dead Code */
        ErrPrint("Invalid FB is created\n");
        return 0;
    } else if (!strncasecmp(info->id, SCHEMA_SHM, strlen(SCHEMA_SHM)) && info->handle > 0) {
        return 1;
    } else {
        const char *path;
        path = util_uri_to_path(info->id);
        if (path && access(path, F_OK | R_OK) == 0) {
            return 1;
        } else {
            ErrPrint("access: %s (%s)\n", strerror(errno), path);
        }
    }

    return 0;
}

void *fb_acquire_buffer(struct fb_info *info)
{
    struct pixmap_info *pixmap_info;
    dynamicbox_fb_t *buffer;
    void *addr;

    if (!info) {
        return NULL;
    }

    if (!info->buffer) {
        if (!strncasecmp(info->id, SCHEMA_PIXMAP, strlen(SCHEMA_PIXMAP))) {
            /*!
             * Use the S/W backend
             */
            ErrPrint("Unsupported type\n");
            return NULL;
        } else if (!strncasecmp(info->id, SCHEMA_FILE, strlen(SCHEMA_FILE))) {
            info->bufsz = info->w * info->h * info->pixels;
            buffer = calloc(1, sizeof(*buffer) + info->bufsz);
            if (!buffer) {
                ErrPrint("Heap: %s\n", strerror(errno));
                info->bufsz = 0;
                return NULL;
            }

            buffer->type = BUFFER_TYPE_FILE;
            buffer->refcnt = 0;
            buffer->state = CREATED;
            buffer->info = info;
            info->buffer = buffer;
        } else if (!strncasecmp(info->id, SCHEMA_SHM, strlen(SCHEMA_SHM))) {
            buffer = shmat(info->handle, NULL, 0);
            if (buffer == (void *)-1) {
                ErrPrint("shmat: %s\n", strerror(errno));
                return NULL;
            }

            info->buffer = buffer;
        } else {
            DbgPrint("Buffer is NIL\n");
            return NULL;
        }
    }

    buffer = info->buffer;
    switch (buffer->type) {
    case BUFFER_TYPE_FILE:
        buffer->refcnt++;
        /* Fall through */
    case BUFFER_TYPE_SHM:
        addr = buffer->data;
        break;
    case BUFFER_TYPE_PIXMAP:
    default:
        addr = NULL;
        break;
    }

    return addr;
}

int fb_release_buffer(void *data)
{
    dynamicbox_fb_t *buffer;
    struct fb_info *info;

    if (!data) {
        return DBOX_STATUS_ERROR_NONE;
    }

    buffer = container_of(data, struct dynamicbox_fb, data);

    if (buffer->state != CREATED) {
        ErrPrint("Invalid handle\n");
        return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    switch (buffer->type) {
    case BUFFER_TYPE_SHM:
        /*!
         * \note
         * SHM can not use the "info"
         * it is NULL
         */
        if (shmdt(buffer) < 0) {
            ErrPrint("shmdt: %s\n", strerror(errno));
        }
        break;
    case BUFFER_TYPE_FILE:
        buffer->refcnt--;
        if (buffer->refcnt == 0) {
            struct fb_info *info;
            info = buffer->info;

            buffer->state = DESTROYED;
            free(buffer);

            if (info && info->buffer == buffer) {
                info->buffer = NULL;
            }
        }
        break;
    case BUFFER_TYPE_PIXMAP:
    default:
        ErrPrint("Unknown type\n");
        return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    return DBOX_STATUS_ERROR_NONE;
}

int fb_type(struct fb_info *info)
{
    dynamicbox_fb_t buffer;

    if (!info) {
        return BUFFER_TYPE_ERROR;
    }

    buffer = info->buffer;
    if (!buffer) {
        int type = BUFFER_TYPE_ERROR;
        /*!
         * \note
         * Try to get this from SCHEMA
         */
        if (info->id) {
            if (!strncasecmp(info->id, SCHEMA_FILE, strlen(SCHEMA_FILE))) {
                type = BUFFER_TYPE_FILE;
            } else if (!strncasecmp(info->id, SCHEMA_PIXMAP, strlen(SCHEMA_PIXMAP))) {
            } else if (!strncasecmp(info->id, SCHEMA_SHM, strlen(SCHEMA_SHM))) {
                type = BUFFER_TYPE_SHM;
            }
        }

        return type;
    }

    return buffer->type;
}

int fb_refcnt(void *data)
{
    dynamicbox_fb_t buffer;
    struct shmid_ds buf;
    int ret;

    if (!data) {
        return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    buffer = container_of(data, struct dynamicbox_fb, data);

    if (buffer->state != CREATED) {
        ErrPrint("Invalid handle\n");
        return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    switch (buffer->type) {
    case BUFFER_TYPE_SHM:
        if (shmctl(buffer->refcnt, IPC_STAT, &buf) < 0) {
            ErrPrint("shmctl: %s\n", strerror(errno));
            ret = DBOX_STATUS_ERROR_FAULT;
            break;
        }

        ret = buf.shm_nattch; /*!< This means attached count of process */
        break;
    case BUFFER_TYPE_FILE:
        ret = buffer->refcnt;
        break;
    case BUFFER_TYPE_PIXMAP:
    default:
        ret = DBOX_STATUS_ERROR_INVALID_PARAMETER;
        break;
    }

    return ret;
}

const char *fb_id(struct fb_info *info)
{
    return info ? info->id : NULL;
}

int fb_get_size(struct fb_info *info, int *w, int *h)
{
    if (!info) {
        ErrPrint("Handle is not valid\n");
        return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    *w = info->w;
    *h = info->h;
    return DBOX_STATUS_ERROR_NONE;
}

int fb_size(struct fb_info *info)
{
    if (!info) {
        ErrPrint("Handle is not valid\n");
        return 0;
    }

    info->bufsz = info->w * info->h * info->pixels;
    return info->bufsz;
}

int fb_sync_xdamage(struct fb_info *info, dynamicbox_damage_region_t *region)
{
    return DBOX_STATUS_ERROR_NONE;
}

/* End of a file */
