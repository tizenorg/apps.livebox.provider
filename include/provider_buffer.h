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

#ifndef __PROVIDER_BUFFER_H
#define __PROVIDER_BUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * \NOTE
 * This enumeration value has to be sync'd with the liblivebox interface. (only for inhouse livebox)
 */
enum target_type {
	TYPE_LB, /*!< */
	TYPE_PD, /*!< */
	TYPE_ERROR /*!< */
};

/*!
 * \NOTE
 * This enumeration value should be sync'd with liblivebox interface. (only for inhouse livebox)
 */
enum buffer_event {
	BUFFER_EVENT_ENTER, /*!< */
	BUFFER_EVENT_LEAVE, /*!< */
	BUFFER_EVENT_DOWN, /*!< */
	BUFFER_EVENT_MOVE, /*!< */
	BUFFER_EVENT_UP, /*!< */

	BUFFER_EVENT_KEY_DOWN, /*!< */
	BUFFER_EVENT_KEY_UP, /*!< */

	BUFFER_EVENT_HIGHLIGHT,
	BUFFER_EVENT_HIGHLIGHT_NEXT,
	BUFFER_EVENT_HIGHLIGHT_PREV,
	BUFFER_EVENT_ACTIVATE,
	BUFFER_EVENT_ACTION_UP,
	BUFFER_EVENT_ACTION_DOWN,
	BUFFER_EVENT_SCROLL_UP,
	BUFFER_EVENT_SCROLL_MOVE,
	BUFFER_EVENT_SCROLL_DOWN,
	BUFFER_EVENT_UNHIGHLIGHT
};

struct livebox_buffer;

/*!
 * \brief
 * \param[in] type
 * \param[in] pkgname
 * \param[in] id
 * \param[in] width
 * \param[in] height
 * \param[in] pixel_size
 * \param[in] handler
 * \param[in] data
 */
extern struct livebox_buffer *provider_buffer_acquire(enum target_type type, const char *pkgname, const char *id, int width, int height, int pixel_size, int (*handler)(struct livebox_buffer *, enum buffer_event, double, double, double, void *data), void *data);

/*!
 * \brief
 * \param[in] info
 * \param[in] w
 * \param[in] h
 * \return int
 */
extern int provider_buffer_resize(struct livebox_buffer *info, int w, int h);

/*!
 * \brief
 * \param[in] info
 * \return address
 */
extern void *provider_buffer_ref(struct livebox_buffer *info);

/*!
 * \brief
 * \param[in] ptr
 * \return int
 */
extern int provider_buffer_unref(void *ptr);

/*!
 * \brief
 * \param[in] info
 * \return int
 */
extern int provider_buffer_release(struct livebox_buffer *info);

/*!
 *\brief
 \param[in] info
 \return int
 */
extern int provider_buffer_sync(struct livebox_buffer *info);

/*!
 * \brief Get the buffer type
 * \param[in] info
 * \return target type PD or LB
 */
extern enum target_type provider_buffer_type(struct livebox_buffer *info);

/*!
 * \brief Get the package name
 * \param[in] info
 * \return pkgname
 */
extern const char *provider_buffer_pkgname(struct livebox_buffer *info);

/*!
 * \brief
 * \param[in] info
 * \return id
 */
extern const char *provider_buffer_id(struct livebox_buffer *info);

/*!
 * \brief Give the URI of buffer information.
 * \param[in] info
 * \return uri
 */
extern const char *provider_buffer_uri(struct livebox_buffer *info);

/*!
 * \brief
 * \param[in] info
 * \param[out] w
 * \param[out] h
 * \param[out] pixel_size
 * \return int
 */
extern int provider_buffer_get_size(struct livebox_buffer *info, int *w, int *h, int *pixel_size);

/*!
 * \brief Getting the PIXMAP id of mapped on this livebox_buffer
 * \param[in] info
 * \return 0 if fails or pixmap ID
 */
extern unsigned long provider_buffer_pixmap_id(struct livebox_buffer *info);

/*!
 * \brief
 * \param[in] disp Display information for handling the XPixmap type.
 * \return int
 */
extern int provider_buffer_init(void *disp);

/*!
 * \brief
 * \return int
 */
extern int provider_buffer_fini(void);

/*!
 * \param[in] info
 * \return int
 */
extern int provider_buffer_pixmap_is_support_hw(struct livebox_buffer *info);

/*!
 * \param[in] info
 * \return int
 */
extern int provider_buffer_pixmap_create_hw(struct livebox_buffer *info);

/*!
 * \param[in] info
 * \return int
 */
extern int provider_buffer_pixmap_destroy_hw(struct livebox_buffer *info);

/*!
 * \brief Get the H/W system mapped buffer address(GEM buffer) if a buffer support it.
 * \param[in] info
 * \return void * H/W system mapped buffer address
 */
extern void *provider_buffer_pixmap_hw_addr(struct livebox_buffer *info);

/*!
 * \brief Prepare the render buffer to write or read something on it.
 * \param[in] info
 * \return int
 */
extern int provider_buffer_pre_render(struct livebox_buffer *info);

/*!
 * \brief Finish the render buffer acessing.
 * \param[in] info 
 * \return int
 */
extern int provider_buffer_post_render(struct livebox_buffer *info);

/*!
 */
extern void *provider_buffer_user_data(struct livebox_buffer *handle);

/*!
 */
extern int provider_buffer_set_user_data(struct livebox_buffer *handle, void *data);

extern struct packet *provider_buffer_pd_access_action_up(pid_t pid, int handle, const struct packet *packet);

extern struct packet *provider_buffer_pd_access_action_down(pid_t pid, int handle, const struct packet *packet);

extern struct packet *provider_buffer_pd_access_scroll_down(pid_t pid, int handle, const struct packet *packet);

extern struct packet *provider_buffer_pd_access_scroll_move(pid_t pid, int handle, const struct packet *packet);

extern struct packet *provider_buffer_pd_access_scroll_up(pid_t pid, int handle, const struct packet *packet);

extern struct packet *provider_buffer_pd_access_unhighlight(pid_t pid, int handle, const struct packet *packet);

extern struct packet *provider_buffer_pd_access_hl(pid_t pid, int handle, const struct packet *packet);

extern struct packet *provider_buffer_pd_access_hl_prev(pid_t pid, int handle, const struct packet *packet);

extern struct packet *provider_buffer_pd_access_hl_next(pid_t pid, int handle, const struct packet *packet);

extern struct packet *provider_buffer_pd_access_activate(pid_t pid, int handle, const struct packet *packet);

extern struct packet *provider_buffer_lb_access_unhighlight(pid_t pid, int handle, const struct packet *packet);

extern struct packet *provider_buffer_lb_access_hl(pid_t pid, int handle, const struct packet *packet);

extern struct packet *provider_buffer_lb_access_hl_prev(pid_t pid, int handle, const struct packet *packet);

extern struct packet *provider_buffer_lb_access_hl_next(pid_t pid, int handle, const struct packet *packet);

extern struct packet *provider_buffer_lb_access_action_up(pid_t pid, int handle, const struct packet *packet);

extern struct packet *provider_buffer_lb_access_action_down(pid_t pid, int handle, const struct packet *packet);

extern struct packet *provider_buffer_lb_access_scroll_down(pid_t pid, int handle, const struct packet *packet);

extern struct packet *provider_buffer_lb_access_scroll_move(pid_t pid, int handle, const struct packet *packet);

extern struct packet *provider_buffer_lb_access_scroll_up(pid_t pid, int handle, const struct packet *packet);

extern struct packet *provider_buffer_lb_access_activate(pid_t pid, int handle, const struct packet *packet);

#ifdef __cplusplus
}
#endif

#endif
/* End of a file */
