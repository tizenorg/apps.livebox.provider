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

#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>
#include <X11/Xutil.h>

#include <X11/Xproto.h>
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xfixes.h>

#include <dri2.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <tbm_bufmgr.h>

#include <dlog.h>
#include <dynamicbox_errno.h>
#include <dynamicbox_service.h>
#include <dynamicbox_buffer.h>

#include "debug.h"
#include "util.h"
#include "dynamicbox_provider.h"
#include "dynamicbox_provider_buffer.h"
#include "provider_buffer_internal.h"
#include "fb.h"
#include "dlist.h"

int errno;

struct pixmap_info {
    XShmSegmentInfo si;
    XImage *xim;
    GC gc;
};

struct gem_data {
    DRI2Buffer *dri2_buffer;
    unsigned int attachments[1];
    tbm_bo pixmap_bo;
    int count;
    int buf_count;
    int w;
    int h;
    int depth;
    Pixmap pixmap;
    void *data; /* Gem layer */
    int refcnt;

    void *compensate_data; /* Check the pitch value, copy this to data */
};

static struct info {
    tbm_bufmgr bufmgr;
    int evt_base;
    int err_base;
    int fd;
    struct dlist *gem_list;
    struct dlist *shm_list;

    Display *disp;
    int screen;
    Visual *visual;
    int disp_is_opened;
} s_info = {
    .bufmgr = NULL,
    .evt_base = 0,
    .err_base = 0,
    .fd = -1,
    .gem_list = NULL,
    .shm_list = NULL,

    .disp = NULL,
    .screen = 0,
    .visual = NULL,
    .disp_is_opened = 0,
};

int fb_init(void *disp)
{
    int dri2Major, dri2Minor;
    char *driverName, *deviceName;
    drm_magic_t magic;
    Screen *screen;

    s_info.disp = disp;
    if (!s_info.disp) {
        s_info.disp = XOpenDisplay(NULL);
        if (!s_info.disp) {
            ErrPrint("Failed to open a display\n");
            return DBOX_STATUS_ERROR_FAULT;
        }

        s_info.disp_is_opened = 1;
    }

    screen = DefaultScreenOfDisplay(s_info.disp);
    s_info.screen = DefaultScreen(s_info.disp);
    s_info.visual = DefaultVisualOfScreen(screen);

    if (!DRI2QueryExtension(s_info.disp, &s_info.evt_base, &s_info.err_base)) {
        ErrPrint("DRI2 is not supported\n");
        return DBOX_STATUS_ERROR_NONE;
    }

    if (!DRI2QueryVersion(s_info.disp, &dri2Major, &dri2Minor)) {
        ErrPrint("DRI2 is not supported\n");
        s_info.evt_base = 0;
        s_info.err_base = 0;
        return DBOX_STATUS_ERROR_NONE;
    }

    if (!DRI2Connect(s_info.disp, DefaultRootWindow(s_info.disp), &driverName, &deviceName)) {
        ErrPrint("DRI2 is not supported\n");
        s_info.evt_base = 0;
        s_info.err_base = 0;
        return DBOX_STATUS_ERROR_NONE;
    }

    s_info.fd = open(deviceName, O_RDWR);
    free(deviceName);
    free(driverName);
    if (s_info.fd < 0) {
        ErrPrint("Failed to open a drm device: (%s)\n", strerror(errno));
        s_info.evt_base = 0;
        s_info.err_base = 0;
        return DBOX_STATUS_ERROR_NONE;
    }

    drmGetMagic(s_info.fd, &magic);
    DbgPrint("DRM Magic: 0x%X\n", magic);
    if (!DRI2Authenticate(s_info.disp, DefaultRootWindow(s_info.disp), (unsigned int)magic)) {
        ErrPrint("Failed to do authenticate for DRI2\n");
        if (close(s_info.fd) < 0) {
            ErrPrint("close: %s\n", strerror(errno));
        }
        s_info.fd = -1;
        s_info.evt_base = 0;
        s_info.err_base = 0;
        return DBOX_STATUS_ERROR_NONE;
    }

    s_info.bufmgr = tbm_bufmgr_init(s_info.fd);
    if (!s_info.bufmgr) {
        ErrPrint("Failed to init bufmgr\n");
        if (close(s_info.fd) < 0) { 
            ErrPrint("close: %s\n", strerror(errno));
        }
        s_info.fd = -1;
        s_info.evt_base = 0;
        s_info.err_base = 0;
        return DBOX_STATUS_ERROR_NONE;
    }

    return DBOX_STATUS_ERROR_NONE;
}

int fb_fini(void)
{
    if (s_info.fd >= 0) {
        if (close(s_info.fd) < 0) {
            ErrPrint("close: %s\n", strerror(errno));
        }
        s_info.fd = -1;
    }

    if (s_info.bufmgr) {
        tbm_bufmgr_deinit(s_info.bufmgr);
        s_info.bufmgr = NULL;
    }

    if (s_info.disp_is_opened && s_info.disp) {
        XCloseDisplay(s_info.disp);
        s_info.disp = NULL;
    }

    s_info.screen = 0;
    s_info.visual = NULL;
    return DBOX_STATUS_ERROR_NONE;
}

static inline int sync_for_file(struct fb_info *info)
{
    int fd;
    dynamicbox_fb_t buffer;

    buffer = info->buffer;
    if (!buffer) {
        DbgPrint("Buffer is NIL, skip sync\n");
        return DBOX_STATUS_ERROR_NONE;
    }

    if (buffer->state != DBOX_FB_STATE_CREATED) {
        ErrPrint("Invalid state of a FB\n");
        return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    if (buffer->type != DBOX_FB_TYPE_FILE) {
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

static inline int sync_for_pixmap(struct fb_info *info)
{
    dynamicbox_fb_t buffer;
    struct pixmap_info *pixmap_info;

    buffer = info->buffer;
    if (!buffer) {
        DbgPrint("Buffer is NIL, skip sync\n");
        return DBOX_STATUS_ERROR_NONE;
    }

    if (buffer->state != DBOX_FB_STATE_CREATED) {
        ErrPrint("Invalid state of a FB\n");
        return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    if (buffer->type != DBOX_FB_TYPE_PIXMAP) {
        DbgPrint("Invalid buffer\n");
        return DBOX_STATUS_ERROR_NONE;
    }

    if (info->handle == 0) {
        ErrPrint("Pixmap ID is not valid\n");
        return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    if (info->bufsz == 0) {
        DbgPrint("Nothing can be sync\n");
        return DBOX_STATUS_ERROR_NONE;
    }

    pixmap_info = (struct pixmap_info *)buffer->data;
    /*!
     * \note Do not send the event.
     *       Instead of X event, master will send the updated event to the viewer
     */
    XShmPutImage(s_info.disp, (Pixmap)info->handle, pixmap_info->gc, pixmap_info->xim, 0, 0, 0, 0, info->w, info->h, False);
    XSync(s_info.disp, False);
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
        return sync_for_pixmap(info);
    } else if (!strncasecmp(info->id, SCHEMA_SHM, strlen(SCHEMA_SHM))) {
        return DBOX_STATUS_ERROR_NONE;
    }

    ErrPrint("Invalid URI: [%s]\n", info->id);
    return DBOX_STATUS_ERROR_INVALID_PARAMETER;
}

static inline struct fb_info *find_shm_by_pixmap(Pixmap id)
{
    struct fb_info *info;
    struct dlist *l;

    info = NULL;
    dlist_foreach(s_info.shm_list, l, info) {
        if (info->handle == (int)id) {
            break;
        }
        info = NULL;
    }

    return info;
}

static inline struct fb_info *find_shm_by_canvas(void *canvas)
{
    struct pixmap_info *pixmap_info;
    dynamicbox_fb_t buffer;
    struct fb_info *info;
    struct dlist *l;

    info = NULL;
    dlist_foreach(s_info.shm_list, l, info) {
        buffer = info->buffer;
        if (!buffer) {
            ErrPrint("Info has invalid buffer ptr\n");
            continue;
        }

        pixmap_info = (struct pixmap_info *)buffer->data;
        if (pixmap_info->si.shmaddr == canvas) {
            break;
        }

        info = NULL;
    }

    return info;
}

static inline struct gem_data *find_gem_by_pixmap(Pixmap id)
{
    struct gem_data *gem;
    struct dlist *l;

    gem = NULL;
    dlist_foreach(s_info.gem_list, l, gem) {
        if (gem->pixmap == id) {
            break;
        }

        gem = NULL;
    }

    return gem;
}

static inline struct gem_data *find_gem_by_canvas(void *canvas)
{
    struct gem_data *gem;
    struct dlist *l;

    gem = NULL;
    dlist_foreach(s_info.gem_list, l, gem) {
        if (gem->data == canvas || gem->compensate_data == canvas) {
            break;
        }

        gem = NULL;
    }

    return gem;
}

static inline int create_pixmap_info(struct fb_info *info)
{
    struct pixmap_info *pixmap_info;
    dynamicbox_fb_t buffer;

    buffer = info->buffer;
    pixmap_info = (struct pixmap_info *)buffer->data;

    pixmap_info->si.shmid = shmget(IPC_PRIVATE, info->bufsz, IPC_CREAT | 0666);
    if (pixmap_info->si.shmid < 0) {
        ErrPrint("shmget: %s\n", strerror(errno));
        return DBOX_STATUS_ERROR_FAULT;
    }

    pixmap_info->si.readOnly = False;
    pixmap_info->si.shmaddr = shmat(pixmap_info->si.shmid, NULL, 0);
    if (pixmap_info->si.shmaddr == (void *)-1) {
        if (shmctl(pixmap_info->si.shmid, IPC_RMID, 0) < 0) {
            ErrPrint("shmctl: %s\n", strerror(errno));
        }

        return DBOX_STATUS_ERROR_FAULT;
    }

    /*!
     * \NOTE
     * XCreatePixmap can only uses 24 bits depth only.
     */
    pixmap_info->xim = XShmCreateImage(s_info.disp, s_info.visual, (info->pixels << 3), ZPixmap, NULL, &pixmap_info->si, info->w, info->h);
    if (pixmap_info->xim == NULL) {
        if (shmdt(pixmap_info->si.shmaddr) < 0) {
            ErrPrint("shmdt: %s\n", strerror(errno));
        }

        if (shmctl(pixmap_info->si.shmid, IPC_RMID, 0) < 0) {
            ErrPrint("shmctl: %s\n", strerror(errno));
        }

        return DBOX_STATUS_ERROR_FAULT;
    }

    pixmap_info->xim->data = pixmap_info->si.shmaddr;
    XShmAttach(s_info.disp, &pixmap_info->si);
    XSync(s_info.disp, False);

    pixmap_info->gc = XCreateGC(s_info.disp, (Pixmap)info->handle, 0, NULL);
    if (!pixmap_info->gc) {
        XShmDetach(s_info.disp, &pixmap_info->si);
        XDestroyImage(pixmap_info->xim);

        if (shmdt(pixmap_info->si.shmaddr) < 0) {
            ErrPrint("shmdt: %s\n", strerror(errno));
        }

        if (shmctl(pixmap_info->si.shmid, IPC_RMID, 0) < 0) {
            ErrPrint("shmctl: %s\n", strerror(errno));
        }

        return DBOX_STATUS_ERROR_FAULT;
    }

    s_info.shm_list = dlist_append(s_info.shm_list, info);
    DbgPrint("SHMID: %d (Size: %d), %p\n", pixmap_info->si.shmid, info->bufsz, pixmap_info->si.shmaddr);
    return DBOX_STATUS_ERROR_NONE;
}

static inline int destroy_pixmap_info(struct fb_info *info)
{
    struct pixmap_info *pixmap_info;
    dynamicbox_fb_t buffer;

    buffer = info->buffer;
    pixmap_info = (struct pixmap_info *)buffer->data;

    XFreeGC(s_info.disp, pixmap_info->gc);
    XShmDetach(s_info.disp, &pixmap_info->si);
    XDestroyImage(pixmap_info->xim);

    if (shmdt(pixmap_info->si.shmaddr) < 0) {
        ErrPrint("shmdt: %s\n", strerror(errno));
    }

    if (shmctl(pixmap_info->si.shmid, IPC_RMID, 0) < 0) {
        ErrPrint("shmctl: %s\n", strerror(errno));
    }

    dlist_remove_data(s_info.shm_list, info);
    DbgPrint("Destroy a pixmap info\n");
    return DBOX_STATUS_ERROR_NONE;
}

static inline struct gem_data *create_gem(Pixmap pixmap, int w, int h, int depth, int auto_align)
{
    struct gem_data *gem;

    if (!s_info.bufmgr) {
        /*
         * @note
         * In rare case,
         * if the mastser is disconnected, service provider should not try to create the gem buffer
         * To avoid this, returns NULL from here.
         */
        ErrPrint("Buffer manager is not initialized\n");
        return NULL;
    }

    gem = calloc(1, sizeof(*gem));
    if (!gem) {
        ErrPrint("Heap: %s\n", strerror(errno));
        return NULL;
    }

    gem->attachments[0] = DRI2BufferFrontLeft;
    gem->count = 1;
    gem->w = w;
    gem->h = h;
    gem->depth = depth;
    gem->pixmap = pixmap;

    DRI2CreateDrawable(s_info.disp, gem->pixmap);

    gem->dri2_buffer = DRI2GetBuffers(s_info.disp, gem->pixmap,
                &gem->w, &gem->h, gem->attachments,
                gem->count, &gem->buf_count);

    if (!gem->dri2_buffer || !gem->dri2_buffer->name) {
        ErrPrint("Failed tog et gem buffer\n");
        DRI2DestroyDrawable(s_info.disp, gem->pixmap);
        free(gem);
        return NULL;
    }

    gem->pixmap_bo = tbm_bo_import(s_info.bufmgr, gem->dri2_buffer->name);
    if (!gem->pixmap_bo) {
        ErrPrint("Failed to import BO\n");
        DRI2DestroyDrawable(s_info.disp, gem->pixmap);
        free(gem);
        return NULL;
    }

    if (auto_align && gem->dri2_buffer->pitch != gem->w * gem->depth) {
        gem->compensate_data = calloc(1, gem->w * gem->h * gem->depth);
        if (!gem->compensate_data) {
            ErrPrint("Failed to allocate heap\n");
        } else {
            DbgPrint("Allocate compensate buffer %p(%dx%d %d\n",
                        gem->compensate_data,
                        gem->w, gem->h, gem->depth);
        }
    }

    s_info.gem_list = dlist_append(s_info.gem_list, gem);

    DbgPrint("dri2_buffer: %p, name: %p, %dx%d (%dx%d), pitch: %d, buf_count: %d, gem: %p\n",
                gem->dri2_buffer, gem->dri2_buffer->name,
                gem->w, gem->h, w, h,
                gem->dri2_buffer->pitch, gem->buf_count, gem);
    return gem;
}

static inline int destroy_gem(struct gem_data *gem)
{
    dlist_remove_data(s_info.gem_list, gem);

    if (gem->compensate_data) {
        DbgPrint("Release compensate buffer %p\n", gem->compensate_data);
        free(gem->compensate_data);
        gem->compensate_data = NULL;
    }

    if (gem->pixmap_bo) {
        DbgPrint("unref pixmap bo\n");
        tbm_bo_unref(gem->pixmap_bo);
        gem->pixmap_bo = NULL;

        DRI2DestroyDrawable(s_info.disp, gem->pixmap);
    }

    free(gem);
    return DBOX_STATUS_ERROR_NONE;
}

static inline void *acquire_gem(struct gem_data *gem)
{
    if (!gem->data) {
        tbm_bo_handle handle;
        handle = tbm_bo_map(gem->pixmap_bo, TBM_DEVICE_CPU, TBM_OPTION_READ | TBM_OPTION_WRITE);
        gem->data = handle.ptr;
        if (!gem->data) {
            ErrPrint("Failed to get BO\n");
        }
    }

    gem->refcnt++;
    return gem->compensate_data ? gem->compensate_data : gem->data;
}

static inline void release_gem(struct gem_data *gem)
{
    gem->refcnt--;
    if (gem->refcnt == 0) {
        if (gem->compensate_data && gem->data) {
            register int x;
            register int y;
            int *gem_pixel;
            int *pixel;
            int gap;

            pixel = gem->compensate_data;
            gem_pixel = gem->data;
            gap = gem->dri2_buffer->pitch - (gem->w * gem->depth);

            for (y = 0; y < gem->h; y++) {
                for (x = 0; x < gem->w; x++) {
                    *gem_pixel++ = *pixel++;
                }

                gem_pixel = (int *)(((char *)gem_pixel) + gap);
            }
        }
        tbm_bo_unmap(gem->pixmap_bo);
        gem->data = NULL;
    } else if (gem->refcnt < 0) {
        ErrPrint("Invalid refcnt: %d (reset)\n", gem->refcnt);
        gem->refcnt = 0;
    }
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
        DbgPrint("PIXMAP: %d\n", info->handle);
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

int fb_has_gem(struct fb_info *info)
{
    return !strncasecmp(info->id, SCHEMA_PIXMAP, strlen(SCHEMA_PIXMAP)) && (s_info.fd >= 0 || info->gem);
}

int fb_create_gem(struct fb_info *info, int auto_align)
{
    if (info->gem) {
        DbgPrint("Already created\n");
        return DBOX_STATUS_ERROR_NONE;
    }

    info->gem = create_gem(info->handle, info->w, info->h, info->pixels, auto_align);
    return info->gem ? DBOX_STATUS_ERROR_NONE : DBOX_STATUS_ERROR_FAULT;
}

int fb_destroy_gem(struct fb_info *info)
{
    if (!info->gem) {
        ErrPrint("GEM is not created\n");
        return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    destroy_gem(info->gem);
    info->gem = NULL;
    return DBOX_STATUS_ERROR_NONE;
}

void *fb_acquire_gem(struct fb_info *info)
{
    if (!info->gem) {
        ErrPrint("Invalid FB info\n");
        return NULL;
    }

    return acquire_gem(info->gem);
}

int fb_release_gem(struct fb_info *info)
{
    if (!info->gem) {
        ErrPrint("Invalid FB info\n");
        return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    release_gem(info->gem);
    return DBOX_STATUS_ERROR_NONE;
}

int fb_destroy(struct fb_info *info)
{
    if (!info) {
        ErrPrint("Handle is not valid\n");
        return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    if (info->buffer) {
        dynamicbox_fb_t buffer;
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
        return 1;
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
    dynamicbox_fb_t buffer;
    void *addr;

    if (!info) {
        return NULL;
    }

    if (!info->buffer) {
        if (!strncasecmp(info->id, SCHEMA_PIXMAP, strlen(SCHEMA_PIXMAP))) {
            /*!
             * Use the S/W backend
             */
            info->bufsz = info->w * info->h * info->pixels;
            buffer = calloc(1, sizeof(*buffer) + sizeof(*pixmap_info));
            if (!buffer) {
                ErrPrint("Heap: %s\n", strerror(errno));
                info->bufsz = 0;
                return NULL;
            }

            buffer->type = DBOX_FB_TYPE_PIXMAP;
            buffer->refcnt = 0;
            buffer->state = DBOX_FB_STATE_CREATED;
            buffer->info = info;
            info->buffer = buffer;

            DbgPrint("Create PIXMAP Info\n");
            if (create_pixmap_info(info) < 0) {
                free(buffer);
                info->buffer = NULL;
                info->bufsz = 0;
                return NULL;
            }
        } else if (!strncasecmp(info->id, SCHEMA_FILE, strlen(SCHEMA_FILE))) {
            info->bufsz = info->w * info->h * info->pixels;
            buffer = calloc(1, sizeof(*buffer) + info->bufsz);
            if (!buffer) {
                ErrPrint("Heap: %s\n", strerror(errno));
                info->bufsz = 0;
                return NULL;
            }

            buffer->type = DBOX_FB_TYPE_FILE;
            buffer->refcnt = 0;
            buffer->state = DBOX_FB_STATE_CREATED;
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
    case DBOX_FB_TYPE_PIXMAP:
        buffer->refcnt++;
        pixmap_info = (struct pixmap_info *)buffer->data;
        addr = pixmap_info->si.shmaddr;
        break;
    case DBOX_FB_TYPE_FILE:
        buffer->refcnt++;
        /* Fall through */
    case DBOX_FB_TYPE_SHM:
        addr = buffer->data;
        break;
    default:
        addr = NULL;
        break;
    }

    return addr;
}

int fb_release_buffer(void *data)
{
    dynamicbox_fb_t buffer;
    struct fb_info *info;

    if (!data) {
        return DBOX_STATUS_ERROR_NONE;
    }

    info = find_shm_by_canvas(data);
    if (info) {
        buffer = info->buffer;
    } else {
        buffer = container_of(data, struct dynamicbox_fb, data);
    }

    if (buffer->state != DBOX_FB_STATE_CREATED) {
        ErrPrint("Invalid handle\n");
        return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    switch (buffer->type) {
    case DBOX_FB_TYPE_SHM:
        /*!
         * \note
         * SHM can not use the "info"
         * it is NULL
         */
        if (shmdt(buffer) < 0) {
            ErrPrint("shmdt: %s\n", strerror(errno));
        }
        break;
    case DBOX_FB_TYPE_PIXMAP:
        buffer->refcnt--;
        if (buffer->refcnt == 0) {
            if (info) {
                destroy_pixmap_info(info);
                if (info->buffer == buffer) {
                    info->buffer = NULL;
                }
            }
            buffer->state = DBOX_FB_STATE_DESTROYED;
            free(buffer);
        }
        break;
    case DBOX_FB_TYPE_FILE:
        buffer->refcnt--;
        if (buffer->refcnt == 0) {
            struct fb_info *info;
            info = buffer->info;
            if (info && info->buffer == buffer) {
                info->buffer = NULL;
            }
            buffer->state = DBOX_FB_STATE_DESTROYED;
            free(buffer);
        }
        break;
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
        return DBOX_FB_TYPE_ERROR;
    }

    buffer = info->buffer;
    if (!buffer) {
        int type = DBOX_FB_TYPE_ERROR;
        /*!
         * \note
         * Try to get this from SCHEMA
         */
        if (info->id) {
            if (!strncasecmp(info->id, SCHEMA_FILE, strlen(SCHEMA_FILE))) {
                type = DBOX_FB_TYPE_FILE;
            } else if (!strncasecmp(info->id, SCHEMA_PIXMAP, strlen(SCHEMA_PIXMAP))) {
                type = DBOX_FB_TYPE_PIXMAP;
            } else if (!strncasecmp(info->id, SCHEMA_SHM, strlen(SCHEMA_SHM))) {
                type = DBOX_FB_TYPE_SHM;
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

    if (buffer->state != DBOX_FB_STATE_CREATED) {
        ErrPrint("Invalid handle\n");
        return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    switch (buffer->type) {
    case DBOX_FB_TYPE_SHM:
        if (shmctl(buffer->refcnt, IPC_STAT, &buf) < 0) {
            ErrPrint("shmctl: %s\n", strerror(errno));
            ret = DBOX_STATUS_ERROR_FAULT;
            break;
        }

        ret = buf.shm_nattch; /*!< This means attached count of process */
        break;
    case DBOX_FB_TYPE_PIXMAP:
        ret = buffer->refcnt;
        break;
    case DBOX_FB_TYPE_FILE:
        ret = buffer->refcnt;
        break;
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
    XRectangle rect;
    XserverRegion _region;

    if (!info || s_info.fd < 0) {
        ErrPrint("Handle is not valid\n");
        return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    rect.x = region ? region->x : 0;
    rect.y = region ? region->y : 0;
    rect.width = (region && region->w) ? region->w : info->w;
    rect.height = (region && region->h) ? region->h : info->h;

    _region = XFixesCreateRegion(s_info.disp, &rect, 1);
    XDamageAdd(s_info.disp, (Pixmap)info->handle, _region);
    XFixesDestroyRegion(s_info.disp, _region);
    XFlush(s_info.disp);

    return DBOX_STATUS_ERROR_NONE;
}

int fb_stride(struct fb_info *info)
{
    int stride;

    if (info->gem) {
        if (((struct gem_data *)(info->gem))->dri2_buffer) {
            stride = ((struct gem_data *)(info->gem))->dri2_buffer->pitch;
        } else {
            ErrPrint("Failed to get dri2_buffer info\n");
            stride = DBOX_STATUS_ERROR_FAULT;
        }
    } else {
        stride = info->w * info->pixels;
    }
    DbgPrint("Stride: %d\n", stride);

    return stride;
}

/* End of a file */
