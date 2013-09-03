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
 * \addtogroup CAPI_PROVIDER_MODULE
 * \{
 */

/*!
 * \note
 * This enumeration value has to be sync'd with the liblivebox interface. (only for inhouse livebox)
 */
enum target_type {
	TYPE_LB, /*!< */
	TYPE_PD, /*!< */
	TYPE_ERROR /*!< */
};

/*!
 * \note
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
 * \brief Send request for acquiring buffer of given type.
 * \details N/A
 * \remarks N/A
 * \param[in] type TYPE_LB or TYPE_PD, select the type of buffer. is it for LB? or PD?
 * \param[in] pkgname Package name of a livebox instance
 * \param[in] id Instance Id
 * \param[in] width Width of buffer
 * \param[in] height Height of buffer
 * \param[in] pixel_size Normally, use "4" for this. it is fixed.
 * \param[in] handler Event handler. Viewer will send the events such as mouse down,move,up, accessibilty event, etc,. via this callback function.
 * \param[in] data Callback data for event handler
 * \return livebox_buffer
 * \retval Livebox buffer object handler
 * \retval NULL if it fails to acquire buffer
 * \pre N/A
 * \post N/A
 * \see provider_buffer_release
 */
extern struct livebox_buffer *provider_buffer_acquire(enum target_type type, const char *pkgname, const char *id, int width, int height, int pixel_size, int (*handler)(struct livebox_buffer *, enum buffer_event, double, double, double, void *data), void *data);

/*!
 * \brief
 * \details N/A
 * \remarks N/A
 * \param[in] info
 * \param[in] w
 * \param[in] h
 * \return int
 * \retval LB_STATUS_ERROR_INVALID
 * \retval LB_STATUS_SUCCESS
 * \pre N/A
 * \post N/A
 * \see N/A
 */
extern int provider_buffer_resize(struct livebox_buffer *info, int w, int h);

/*!
 * \brief Get the buffer address of given livebox buffer handle.
 * \details N/A
 * \remarks N/A
 * \param[in] info Handle of livebox buffer
 * \return address
 * \retval Address of buffer
 * \retval NULL
 * \pre N/A
 * \post N/A
 * \see provider_buffer_unref
 */
extern void *provider_buffer_ref(struct livebox_buffer *info);

/*!
 * \brief Decrease the reference count of given buffer handle.
 * \details N/A
 * \remarks N/A
 * \param[in] ptr Address that gets by provider_buffer_ref
 * \return int
 * \pre N/A
 * \post N/A
 * \see provider_buffer_ref
 */
extern int provider_buffer_unref(void *ptr);

/*!
 * \brief Release the acquired buffer handle.
 * \details N/A
 * \remarks N/A
 * \param[in] info Handle of livebox buffer
 * \return int
 * \retval LB_STATUS_ERROR_INVALID
 * \retval LB_STATUS_SUCCESS
 * \pre N/A
 * \post N/A
 * \see provider_buffer_acquire
 */
extern int provider_buffer_release(struct livebox_buffer *info);

/*!
 *\brief Make content sync with master.
 * \details N/A
 * \remarks N/A
 * \param[in] info Handle of livebox buffer
 * \return int
 * \retval LB_STATUS_ERROR_INVALID
 * \retval LB_STATUS_SUCCESS
 * \pre N/A
 * \post N/A
 * \see N/A
 */
extern int provider_buffer_sync(struct livebox_buffer *info);

/*!
 * \brief Get the buffer type
 * \details N/A
 * \remarks N/A
 * \param[in] info Handle of livebox buffer
 * \return target type PD or LB
 * \retval TYPE_LB
 * \retval TYPE_PD
 * \retval TYPE_ERROR
 * \pre N/A
 * \post N/A
 * \see N/A
 */
extern enum target_type provider_buffer_type(struct livebox_buffer *info);

/*!
 * \brief Get the package name
 * \details N/A
 * \remarks N/A
 * \param[in] info
 * \return const char *
 * \retval NULL
 * \retval pkgname package name
 * \pre N/A
 * \post N/A
 * \see N/A
 */
extern const char *provider_buffer_pkgname(struct livebox_buffer *info);

/*!
 * \brief
 * \details N/A
 * \remarks N/A
 * \param[in] info Handle of livebox buffer
 * \return const char *
 * \retval instance Id
 * \retval NULL
 * \pre N/A
 * \post N/A
 * \see N/A
 */
extern const char *provider_buffer_id(struct livebox_buffer *info);

/*!
 * \brief Give the URI of buffer information.
 * \details N/A
 * \remarks N/A
 * \param[in] info Handle of livebox buffer
 * \return const char *
 * \retval uri
 * \retval NULL
 * \pre N/A
 * \post N/A
 * \see N/A
 */
extern const char *provider_buffer_uri(struct livebox_buffer *info);

/*!
 * \brief
 * \details N/A
 * \remarks N/A
 * \param[in] info Handle of livebox buffer
 * \param[out] w
 * \param[out] h
 * \param[out] pixel_size
 * \return int
 * \retval LB_STATUS_SUCCESS
 * \retval LB_STATUS_ERROR_INVALID
 * \pre N/A
 * \post N/A
 * \see N/A
 */
extern int provider_buffer_get_size(struct livebox_buffer *info, int *w, int *h, int *pixel_size);

/*!
 * \brief Getting the PIXMAP id of mapped on this livebox_buffer
 * \details N/A
 * \remarks N/A
 * \param[in] info Handle of livebox buffer
 * \return int
 * \retval 0 if fails
 * \retval pixmap id, has to be casted to unsigned int type
 * \pre N/A
 * \post N/A
 * \see N/A
 */
extern unsigned long provider_buffer_pixmap_id(struct livebox_buffer *info);

/*!
 * \brief Initialize the provider buffer system
 * \details N/A
 * \remarks N/A
 * \param[in] disp Display information for handling the XPixmap type.
 * \return int
 * \retval LB_STATUS_ERROR_FAULT
 * \retval LB_STATUS_SUCCESS
 * \pre N/A
 * \post N/A
 * \see N/A
 */
extern int provider_buffer_init(void *disp);

/*!
 * \brief Finalize the provider buffer system
 * \details N/A
 * \remarks N/A
 * \return int
 * \retval LB_STATUS_SUCCESS
 * \pre N/A
 * \post N/A
 * \see N/A
 */
extern int provider_buffer_fini(void);

/*!
 * \brief Check whether current livebox buffer support H/W(GEM) buffer
 * \details N/A
 * \remarks N/A
 * \param[in] info Handle of livebox buffer
 * \return int
 * \retval LB_STATUS_ERROR_INVALID
 * \retval 1 if support
 * \retval 0 if not supported
 * \pre N/A
 * \post N/A
 * \see N/A
 */
extern int provider_buffer_pixmap_is_support_hw(struct livebox_buffer *info);

/*!
 * \brief Create H/W(GEM) Buffer
 * \details N/A
 * \remarks N/A
 * \param[in] info Handle of livebox buffer
 * \return int
 * \retval LB_STATUS_SUCCESS
 * \retval LB_STATUS_ERROR_INVALID
 * \retval LB_STATUS_ERROR_FAULT
 * \pre N/A
 * \post N/A
 * \see provider_buffer_pixmap_destroy_hw
 */
extern int provider_buffer_pixmap_create_hw(struct livebox_buffer *info);

/*!
 * \brief Destroy H/W(GEM) Buffer
 * \details N/A
 * \remarks N/A
 * \param[in] info Handle of livebox buffer
 * \return int
 * \retval LB_STATUS_SUCCESS
 * \retval LB_STATUS_ERROR_INVALID
 * \pre N/A
 * \post N/A
 * \see provider_buffer_pixmap_create_hw
 */
extern int provider_buffer_pixmap_destroy_hw(struct livebox_buffer *info);

/*!
 * \brief Get the H/W system mapped buffer address(GEM buffer) if a buffer support it.
 * \details N/A
 * \remarks N/A
 * \param[in] info Handle of livebox buffer
 * \return void *
 * \retval H/W system mapped buffer address
 * \retval NULL fails to get buffer address
 * \pre N/A
 * \post N/A
 * \see provider_buffer_pixmap_create_hw
 * \see provider_buffer_pixmap_destroy_hw
 */
extern void *provider_buffer_pixmap_hw_addr(struct livebox_buffer *info);

/*!
 * \brief Prepare the render buffer to write or read something on it.
 * \details N/A
 * \remarks N/A
 * \param[in] info Handle of livebox buffer
 * \return int
 * \retval LB_STATUS_SUCCESS
 * \retval LB_STATUS_ERROR_INVALID
 * \retval LB_STATUS_ERROR_FAULT
 * \pre N/A
 * \post N/A
 * \see provider_buffer_post_render
 */
extern int provider_buffer_pre_render(struct livebox_buffer *info);

/*!
 * \brief Finish the render buffer acessing.
 * \details N/A
 * \remarks N/A
 * \param[in] info Handle of livebox buffer
 * \return int
 * \retval LB_STATUS_SUCCESS
 * \retval LB_STATUS_ERROR_INVALID
 * \pre N/A
 * \post N/A
 * \see provider_buffer_pre_render
 */
extern int provider_buffer_post_render(struct livebox_buffer *info);

/*!
 * \brief
 * \details N/A
 * \remarks N/A
 * \param[in] handle Handle of livebox buffer
 * \return void *
 * \retval User data
 * \pre N/A
 * \post N/A
 * \see provider_buffer_set_user_data
 */
extern void *provider_buffer_user_data(struct livebox_buffer *handle);

/*!
 * \brief
 * \details N/A
 * \remarks N/A
 * \param[in] handle Handle of livebox buffer
 * \param[in] data User data
 * \return int
 * \retval LB_STATUS_SUCCESS
 * \retval LB_STATUS_ERROR_INVALID
 * \pre N/A
 * \post N/A
 * \see provider_buffer_user_data
 */
extern int provider_buffer_set_user_data(struct livebox_buffer *handle, void *data);

/*!
 * \}
 */

#ifdef __cplusplus
}
#endif

#endif
/* End of a file */
