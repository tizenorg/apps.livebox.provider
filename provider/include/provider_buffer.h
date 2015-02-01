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

/**
 * @addtogroup CAPI_PROVIDER_MODULE
 * @{
 */

/**
 * @note
 * This enumeration value has to be sync'd with the liblivebox interface. (only for inhouse livebox)
 */
enum target_type {
	TYPE_LB, /**< Livebox */
	TYPE_PD, /**< Progressive Disclosure */
	TYPE_ERROR /**< Error */
};

/**
 * @note
 * This enumeration value should be sync'd with liblivebox interface. (only for inhouse livebox)
 */
enum buffer_event {
    BUFFER_EVENT_ENTER, /**< */
    BUFFER_EVENT_LEAVE, /**< */
    BUFFER_EVENT_DOWN, /**< */
    BUFFER_EVENT_MOVE, /**< */
    BUFFER_EVENT_UP, /**< */

    BUFFER_EVENT_KEY_DOWN, /**< */
    BUFFER_EVENT_KEY_UP, /**< */
    BUFFER_EVENT_KEY_FOCUS_IN, /**< */
    BUFFER_EVENT_KEY_FOCUS_OUT, /**< */

    BUFFER_EVENT_HIGHLIGHT, /**< Accessibility Highlight event */
    BUFFER_EVENT_HIGHLIGHT_NEXT, /**< Accessibility Highlight Next event */
    BUFFER_EVENT_HIGHLIGHT_PREV, /**< Accessibility Highlight Prev event */
    BUFFER_EVENT_ACTIVATE, /**< Accessibility Activate event */
    BUFFER_EVENT_ACTION_UP, /**< Accessibility Action Up event */
    BUFFER_EVENT_ACTION_DOWN, /**< Accessibility Action Down event */
    BUFFER_EVENT_SCROLL_UP, /**< Accessibility Scroll Mouse Up event */
    BUFFER_EVENT_SCROLL_MOVE, /**< Accessibility Scroll Mouse Move event */
    BUFFER_EVENT_SCROLL_DOWN, /**< Accessibility Scroll Mouse Down event */
    BUFFER_EVENT_UNHIGHLIGHT, /**< Accessibility Unhighlight event */

    BUFFER_EVENT_ON_HOLD,	/**< To prevent from generating mouse clicked event */
    BUFFER_EVENT_OFF_HOLD, /**< Disable the mouse hold event */
    BUFFER_EVENT_ON_SCROLL, /**< Enable the scroll flag */
    BUFFER_EVENT_OFF_SCROLL, /**< Disable the scroll flag */

    /* Accessibility event */
    BUFFER_EVENT_VALUE_CHANGE,
    BUFFER_EVENT_MOUSE,
    BUFFER_EVENT_BACK,
    BUFFER_EVENT_OVER,
    BUFFER_EVENT_READ,
    BUFFER_EVENT_ENABLE,
    BUFFER_EVENT_DISABLE
};

struct buffer_event_data {
    enum buffer_event type; /**< Event type */
    double timestamp; /**< Timestamp */

    union __input_data {
	struct __mouse {
	    int x; /**< Touch X coordinate */
	    int y; /**< Touch Y coordinate */
	} pointer;

	struct __access {
	    int x;
	    int y;
	    unsigned int mouse_type;
	    unsigned int action_type;
	    unsigned int action_by;
	    int cycle;
	} access;

	unsigned int keycode; /**< Key code value */
    } info;
};

struct livebox_buffer;

/**
 * @brief Send request for acquiring buffer of given type.
 * @param[in] type #TYPE_LB or #TYPE_PD, select the type of buffer. is it for LB? or PD?
 * @param[in] pkgname Package name of a livebox instance
 * @param[in] id Instance Id
 * @param[in] auto_align
 * @param[in] handler Event handler. Viewer will send the events such as mouse down,move,up, accessibilty event, etc,. via this callback function.
 * @param[in] data Callback data for event handler
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return livebox_buffer
 * @retval Livebox buffer object handler
 * @retval @c NULL if it fails to acquire buffer
 * @see provider_buffer_release()
 * @see provider_buffer_acquire()
 */
extern struct livebox_buffer *provider_buffer_create(enum target_type type, const char *pkgname, const char *id, int auto_align, int (*handler)(struct livebox_buffer *, struct buffer_event_data *, void *), void *data);

/**
 * @brief Acquire a provider buffer
 * @param[in] width Width of buffer
 * @param[in] height Height of buffer
 * @param[in] pixel_size Normally, use "4" for this. it is fixed.
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return int status
 * @retval #LB_STATUS_ERROR_INVALID
 * @retval #LB_STATUS_ERROR_FAULT
 * @retval #LB_STATUS_SUCCESS
 * @see provider_buffer_create()
 * @see provider_buffer_release()
 */
extern int provider_buffer_acquire(struct livebox_buffer *info, int width, int height, int pixel_size);

/**
 * @brief Resize a buffer to given size
 * @param[in] info Buffer handle
 * @param[in] w Width in pixles size
 * @param[in] h Height in pixels size
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return int status
 * @retval #LB_STATUS_ERROR_INVALID Invalid parameters
 * @retval #LB_STATUS_SUCCESS Successfully resized
 */
extern int provider_buffer_resize(struct livebox_buffer *info, int w, int h);

/**
 * @brief Get the buffer address of given livebox buffer handle.
 * @param[in] info Handle of livebox buffer
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return address
 * @retval Address of buffer
 * @retval @c NULL
 * @see provider_buffer_unref()
 */
extern void *provider_buffer_ref(struct livebox_buffer *info);

/**
 * @brief Decrease the reference count of given buffer handle.
 * @param[in] ptr Address that gets by provider_buffer_ref
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return int status
 * @retval #LB_STATUS_SUCCESS Successfully un-referenced
 * @retval #LB_STATUS_ERROR_INVALID Invalid parameters
 * @see provider_buffer_ref()
 */
extern int provider_buffer_unref(void *ptr);

/**
 * @brief Release the acquired buffer handle.
 * @remarks This API will not destroy the buffer.
 * @param[in] info Handle of livebox buffer
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return int status
 * @retval #LB_STATUS_ERROR_INVALID Invalid parameters
 * @retval #LB_STATUS_SUCCESS Successfully released
 * @see provider_buffer_create()
 * @see provider_buffer_acquire()
 */
extern int provider_buffer_release(struct livebox_buffer *info);

/**
 * @brief Destroy Buffer Object
 * @param[in] info Buffer handler
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return int status
 * @retval #LB_STATUS_SUCCESS Successfully destroyed
 * @retval #LB_STATUS_ERROR_INVALID Invalid parameter
 * @see provider_buffer_create()
 * @see provider_buffer_release()
 */
extern int provider_buffer_destroy(struct livebox_buffer *info);

/**
 * @brief Make content sync with master.
 * @param[in] info Handle of livebox buffer
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return int status
 * @retval #LB_STATUS_ERROR_INVALID Invalid paramter
 * @retval #LB_STATUS_SUCCESS Successfully sync'd
 */
extern int provider_buffer_sync(struct livebox_buffer *info);

/**
 * @brief Get the buffer type
 * @param[in] info Handle of a livebox buffer
 * @privlevel N/P
 * @return target type PD or LB
 * @retval #TYPE_LB Livebox
 * @retval #TYPE_PD Progressive Disclosure
 * @retval #TYPE_ERROR Undefined error
 */
extern enum target_type provider_buffer_type(struct livebox_buffer *info);

/**
 * @brief Get the package name of given buffer handler
 * @param[in] info Handle of a livebox buffer
 * @privlevel N/P
 * @return const char *
 * @retval @c NULL
 * @retval pkgname package name
 * @see provider_buffer_create()
 * @see provider_buffer_id()
 */
extern const char *provider_buffer_pkgname(struct livebox_buffer *info);

/**
 * @brief Get the Instance Id of given buffer handler
 * @param[in] info Handle of livebox buffer
 * @privlevel N/P
 * @return const char *
 * @retval instance Id in string
 * @retval @c NULL
 * @see provider_buffer_create()
 * @see provider_buffer_pkgname()
 */
extern const char *provider_buffer_id(struct livebox_buffer *info);

/**
 * @brief Give the URI of buffer information.
 * @details Every buffer information has their own URI.\n
 *          URI is consists with scheme and resource name.\n
 *          It looks like "pixmap://123456:4"\n
 *          "file:///tmp/abcd.png"\n
 *          "shm://1234"\n
 *          So you can recognize what method is used for sharing content by parse this URI.
 * @param[in] info Handle of livebox buffer
 * @privlevel N/P
 * @return const char *
 * @retval uri URI of given buffer handler
 * @retval @c NULL
 * @see provider_buffer_create()
 * @see provider_buffer_acquire()
 */
extern const char *provider_buffer_uri(struct livebox_buffer *info);

/**
 * @brief Get the size information of given buffer handler
 * @param[in] info Handle of livebox buffer
 * @param[out] w Width in pixels
 * @param[out] h Height in pixels
 * @param[out] pixel_size Bytes Per Pixels
 * @privlevel N/P
 * @return int status
 * @retval #LB_STATUS_SUCCESS Successfully get the information
 * @retval #LB_STATUS_ERROR_INVALID Invalid parameters
 * @see provider_buffer_acquire()
 * @see provider_buffer_resize()
 */
extern int provider_buffer_get_size(struct livebox_buffer *info, int *w, int *h, int *pixel_size);

/**
 * @brief Getting the PIXMAP id of mapped on this livebox_buffer
 * @param[in] info Handle of livebox buffer
 * @privlevel N/P
 * @return int status
 * @retval \c 0 if fails
 * @retval pixmap id, has to be casted to unsigned int type
 * @pre provider_buffer_acquire() must be called or this function will returns @c 0
 * @see provider_buffer_uri()
 * @see provider_buffer_create()
 * @see provider_buffer_acquire()
 */
extern unsigned long provider_buffer_pixmap_id(struct livebox_buffer *info);

/**
 * @brief Initialize the provider buffer system
 * @param[in] disp Display information for handling the XPixmap type.
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return int status
 * @retval #LB_STATUS_ERROR_FAULT Failed to initialize the provider buffer system
 * @retval #LB_STATUS_SUCCESS Successfully initialized
 * @see provider_buffer_fini()
 */
extern int provider_buffer_init(void *disp);

/**
 * @brief Finalize the provider buffer system
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return int status
 * @retval #LB_STATUS_SUCCESS Successfully finalized
 * @see provider_buffer_init()
 */
extern int provider_buffer_fini(void);

/**
 * @brief Check whether current livebox buffer support H/W(GEM) buffer
 * @param[in] info Handle of livebox buffer
 * @privlevel N/P
 * @return int status
 * @retval #LB_STATUS_ERROR_INVALID Invalid parameter
 * @retval 1 if support
 * @retval 0 if not supported
 * @pre provider_buffer_create must be called first
 * @see provider_buffer_create()
 */
extern int provider_buffer_pixmap_is_support_hw(struct livebox_buffer *info);

/**
 * @brief Create H/W(GEM) Buffer
 * @details Allocate system buffer for sharing the contents between provider & consumer without copying overhead.
 * @param[in] info Handle of livebox buffer
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return int status
 * @retval #LB_STATUS_SUCCESS Successfully created
 * @retval #LB_STATUS_ERROR_INVALID Invalid parameter or not supported
 * @retval #LB_STATUS_ERROR_FAULT Unrecoverable error occurred
 * @pre you can use this only if the provider_buffer_pixmap_is_support_hw() function returns @c true(1)
 * @post N/A
 * @see provider_buffer_pixmap_is_support_hw()
 * @see provider_buffer_pixmap_destroy_hw()
 */
extern int provider_buffer_pixmap_create_hw(struct livebox_buffer *info);

/**
 * @brief Destroy H/W(GEM) Buffer
 * @param[in] info Handle of livebox buffer
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return int status
 * @retval #LB_STATUS_SUCCESS Successfully destroyed
 * @retval #LB_STATUS_ERROR_INVALID Invalid parameter
 * @see provider_buffer_pixmap_create_hw()
 */
extern int provider_buffer_pixmap_destroy_hw(struct livebox_buffer *info);

/**
 * @brief Get the H/W system mapped buffer address(GEM buffer) if a buffer support it.
 * @param[in] info Handle of livebox buffer
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return void *
 * @retval H/W system mapped buffer address
 * @retval @c NULL fails to get buffer address
 * @see provider_buffer_pixmap_create_hw()
 * @see provider_buffer_pixmap_destroy_hw()
 */
extern void *provider_buffer_pixmap_hw_addr(struct livebox_buffer *info);

/**
 * @brief Prepare the render buffer to write or read something on it.
 * @param[in] info Handle of livebox buffer
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return int status
 * @retval #LB_STATUS_SUCCESS
 * @retval #LB_STATUS_ERROR_INVALID
 * @retval #LB_STATUS_ERROR_FAULT
 * @see provider_buffer_post_render()
 */
extern int provider_buffer_pre_render(struct livebox_buffer *info);

/**
 * @brief Finish the render buffer acessing.
 * @param[in] info Handle of livebox buffer
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return int status
 * @retval #LB_STATUS_SUCCESS
 * @retval #LB_STATUS_ERROR_INVALID
 * @see provider_buffer_pre_render()
 */
extern int provider_buffer_post_render(struct livebox_buffer *info);

/**
 * @brief Get the user data registered on buffer handler.
 * @param[in] handle Handle of livebox buffer
 * @privlevel N/P
 * @return void *
 * @retval User data
 * @pre provider_buffer_set_user_data() function must be called before
 * @see provider_buffer_set_user_data()
 */
extern void *provider_buffer_user_data(struct livebox_buffer *handle);

/**
 * @brief Set a user data on given buffer handler.
 * @param[in] handle Handle of livebox buffer
 * @param[in] data User data
 * @privlevel N/P
 * @return int status
 * @retval #LB_STATUS_SUCCESS Successfully updated
 * @retval #LB_STATUS_ERROR_INVALID Invalid parameter
 * @see provider_buffer_user_data()
 */
extern int provider_buffer_set_user_data(struct livebox_buffer *handle, void *data);

/**
 * @brief Find a buffer handler using given information
 * @param[in] type #TYPE_LB or #TYPE_PD can be used
 * @param[in] pkgname Package name
 * @param[in] id Instance Id
 * @privlevel N/P
 * @return livebox_buffer
 * @retval @c NULL If it fails to find one
 * @retval handle If it found
 * @see provider_buffer_create()
 */
extern struct livebox_buffer *provider_buffer_find_buffer(enum target_type type, const char *pkgname, const char *id);

/**
 * @brief Get the stride information of given buffer handler
 * @param[in] info Buffer handler
 * @privlevel N/P
 * @return int stride size in bytes
 * @retval #LB_STATUS_ERROR_INVALID Invalid parameter
 * @retval stride_size in bytes
 */
extern int provider_buffer_stride(struct livebox_buffer *info);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
/* End of a file */
