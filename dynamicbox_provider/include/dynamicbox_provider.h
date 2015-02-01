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

#include <dynamicbox_script.h>
#include <dynamicbox_service.h>

#ifndef __DBOX_PROVIDER_H
#define __DBOX_PROVIDER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file dynamicbox_provider.h
 * @brief This file declares API of libdynamicbox_provider library
 * @since_tizen 2.3
 */

/**
 * @addtogroup CAPI_DYNAMICBOX_PROVIDER_MODULE
 * @{
 */

/**
 * @internal
 * @brief Update the provider control
 * @since_tizen 2.3
 * @see provider_control()
 * THIS ENUM MUST HAS TO BE SYNC'd WITH THE DATA-PROVIDER-MASTER
 */
typedef enum dynamicbox_provider_ctrl {
    DBOX_PROVIDER_CTRL_DEFAULT = 0x00,              /**< Set default control operation */
    DBOX_PROVIDER_CTRL_MANUAL_TERMINATION = 0x01,   /**< Terminate process manually */
    DBOX_PROVIDER_CTRL_MANUAL_REACTIVATION = 0x02,  /**< Reactivate process manually */
} dynamicbox_provider_ctrl_e;

/**
 * @internal
 * @brief Argument data structure for Dyanmic Box Instance events
 * @since_tizen 2.3
 */
struct dynamicbox_event_arg {
    enum _event_type {
        DBOX_EVENT_NEW, /**< Master will send this to create a new dynamicbox instance */
        DBOX_EVENT_RENEW, /**< If the master detects any problems of your slave, it will terminate slave provider.
                               and after reactivating the provider slave, this request will be delievered to create a dynamicbox instance again */

        DBOX_EVENT_DELETE, /**< Master will send this to delete a dynamicbox instance */

        DBOX_EVENT_CONTENT_EVENT, /**< Any events are generated from your dynamicbox or Glance Bar, this event will be sent to you */
        DBOX_EVENT_CLICKED, /**< If a dynamicbox is clicked, the master will send this event */
        DBOX_EVENT_TEXT_SIGNAL, /**< Text type dynamicbox or Glance Bar will generate this event */

        DBOX_EVENT_RESIZE, /**< If a dynamicbox is resized, the master will send this event */
        DBOX_EVENT_SET_PERIOD, /**< To change the update period of a dynamicbox */
        DBOX_EVENT_CHANGE_GROUP, /**< To change the group(cluster/sub-cluster) of a dynamicbox */
        DBOX_EVENT_PINUP, /**< To make pin up of a dynamicbox */

        DBOX_EVENT_UPDATE_CONTENT, /**< It's time to update the content of a dynamicbox */

        DBOX_EVENT_PAUSE, /**< Freeze all timer and go to sleep mode */
        DBOX_EVENT_RESUME, /**< Thaw all timer and wake up */

        DBOX_EVENT_GBAR_CREATE, /**< Only for the buffer type */
        DBOX_EVENT_GBAR_DESTROY, /**< Only for the buffer type */
        DBOX_EVENT_GBAR_MOVE, /**< Only for the buffer type */

        DBOX_EVENT_DBOX_PAUSE, /**< Freeze the update timer of a specified dynamicbox */
        DBOX_EVENT_DBOX_RESUME, /**< Thaw the update timer of a specified dynamicbox */
    } type;
    const char *pkgname; /**< Package name of a dynamicbox */
    const char *id; /**< Instance Id of a dynamicbox */

    union _event_data {
        struct _gbar_create {
            int w;                   /**< Glance Bar buffer is created with width "w" */
            int h;                   /**< Glance Bar buffer is created with height "h" */

            double x;                /**< Relative X position of a dynamicbox from this Glance Bar */
            double y;                /**< Relative Y position of a dynamicbox from this Glance Bar */
        } gbar_create;

        struct _gbar_destroy {
            int reason;              /**< Error (status) code */
        } gbar_destroy;

        struct _gbar_move {
            int w;                   /**< Glance Bar buffer width */
            int h;                   /**< Glance Bar buffer height */

            double x;                /**< Relative X position of a dynamicbox from this Glance Bar */
            double y;                /**< Relative Y position of a dynamicbox from this Glance Bar */
        } gbar_move;

        struct _dbox_create {
            const char *content;     /**< Content info */
            int timeout;             /**< Timeout */
            int has_script;          /**< Dynamicbox has script (buffer would be created from the master) */
            double period;           /**< Update period */
            const char *cluster;     /**< Cluster ID of this dynamicbox instance */
            const char *category;    /**< Sub-cluster ID of this dynamicbox instance */
            int skip_need_to_create; /**< Is this dynamicbox need to check the "need_to_create"? */
            int width;               /**< Width of a dynamicbox content in pixels */
            int height;              /**< Height of a dynamicbox content in pixels */
            const char *abi;         /**< ABI tag of this dynamicbox */
            char *out_content;       /**< Output content */
            char *out_title;         /**< Output title */
            int out_is_pinned_up;    /**< Is this pinned up? */
            const char *direct_addr; /** Event path for sending updated event to the viewer directly */
        } dbox_create; /**< Event information for the newly created instance */

        struct _dbox_recreate {
            const char *content;     /**< Content info */
            int timeout;             /**< Timeout */
            int has_script;          /**< Dynamicbox has script (buffer would be created from the master) */
            double period;           /**< Update period */
            const char *cluster;     /**< Cluster ID of this dynamicbox instance */
            const char *category;    /**< Sub-cluster ID of this dynamicbox instance */
            int width;               /**< Width of a dynamicbox content in pixels */
            int height;              /**< Height of a dynamicbox content in pixels */
            const char *abi;         /**< ABI tag of this dynamicbox */
            char *out_content;       /**< Output content */
            char *out_title;         /**< Output title */
            int out_is_pinned_up;    /**< Is this pinned up? */
            int hold_scroll;         /**< The scroller which is in viewer is locked */
            int active_update;       /**< Need to know, current update mode */
            const char *direct_addr; /**< Event path for sending updated event to the viewer directly */
        } dbox_recreate; /**< Event information for the re-created instance */

        struct _dbox_destroy { /**< This enumeration value must has to be sync with data-provider-master */
            dynamicbox_destroy_type_e type;
        } dbox_destroy; /**< Destroyed */

        struct _content_event {
            const char *emission;              /**< Event string */
            const char *source;                /**< Object ID which makes event */
            struct dynamicbox_event_info info; /**< Extra information */
        } content_event; /**< script */

        struct _clicked {
            const char *event; /**< Event type, currently only "click" supported */
            double timestamp;  /**< Timestamp of event occurred */
            double x;          /**< X position of the click event */
            double y;          /**< Y position of the click event */
        } clicked; /**< clicked */

        struct _text_signal {
            const char *emission;              /**< Event string */
            const char *source;                /**< Object ID which makes event */
            struct dynamicbox_event_info info; /**< Extra information */
        } text_signal; /**< text_signal */

        struct _resize {
            int w; /**< New width of a dynamicbox */
            int h; /**< New height of a dynamicbox */
        } resize; /**< resize */

        struct _set_period {
            double period; /**< New period */
        } set_period; /**< set_period */

        struct _change_group {
            const char *cluster;  /**< Cluster information is changed */
            const char *category; /**< Sub-cluster information is changed */
        } change_group; /**< change_group */

        struct _pinup {
            int state;          /**< Current state of Pin-up */
            char *content_info; /**< out value */
        } pinup; /**< pinup */

        struct _update_content {
            const char *cluster;  /**< Cluster information */
            const char *category; /**< Sub-cluster information */
            const char *content;  /**< Content information */
            int force;            /**< Updated by fault */
        } update_content; /**< update_content */

        struct _pause {
            double timestamp; /**< Timestamp of the provider pause event occurred */
        } pause; /**< pause */

        struct _resume {
            double timestamp; /**< Timestamp of the provider resume event occurred */
        } resume; /**< resume */

        struct _dbox_pause {
            /**< Dynamicbox is paused */
        } dbox_pause;

        struct _dbox_resume {
            /**< Dynamicbox is resumed */
        } dbox_resume;

        struct _update_mode {
            int active_update; /**< 1 for Active update mode or 0 */
        } update_mode; /**< Update mode */
    } info;
};

/**
 * @internal
 * @brief Event Handler tables for managing the life-cycle of dynamicbox instances
 * @since_tizen 2.3
 */
typedef struct dynamicbox_event_table {
    int (*dbox_create)(struct dynamicbox_event_arg *arg, int *width, int *height, double *priority, void *data); /**< new */
    int (*dbox_destroy)(struct dynamicbox_event_arg *arg, void *data); /**< delete */

    /**
     * @note
     * 'dbox' is short from "Dynamic Box"
     * Recover from the fault of slave
     * If a dbox is deleted by accidently, it will be re-created by provider master,
     * in that case you will get this event callback.
     * From here the dbox developer should recover its context.
     */
    int (*dbox_recreate)(struct dynamicbox_event_arg *arg, void *data); /**< Re-create */

    int (*dbox_pause)(struct dynamicbox_event_arg *arg, void *data); /**< Pause a specific Dynamicbox */
    int (*dbox_resume)(struct dynamicbox_event_arg *arg, void *data); /**< Resume a specific Dynamicbox */

    int (*content_event)(struct dynamicbox_event_arg *arg, void *data); /**< Content event */
    int (*clicked)(struct dynamicbox_event_arg *arg, void *data); /**< Clicked */
    int (*text_signal)(struct dynamicbox_event_arg *arg, void *data); /**< Text Signal */
    int (*resize)(struct dynamicbox_event_arg *arg, void *data); /**< Resize */
    int (*set_period)(struct dynamicbox_event_arg *arg, void *data); /**< Set Period */
    int (*change_group)(struct dynamicbox_event_arg *arg, void *data); /**< Change group */
    int (*pinup)(struct dynamicbox_event_arg *arg, void *data); /**< Pin up */
    int (*update_content)(struct dynamicbox_event_arg *arg, void *data); /**< Update content */

    /**
     * @note
     * Visibility changed event
     * these "pause" and "resume" callbacks are different with "dbox_pause" and "dbox_resume",
     */
    int (*pause)(struct dynamicbox_event_arg *arg, void *data); /**< Pause the service provider */
    int (*resume)(struct dynamicbox_event_arg *arg, void *data); /**< Resume the service provider */

    /**
     * @note
     * service status changed event
     * If a provider lost its connection to master,
     * the provider should clean all its instances up properly.
     * and terminate itself if it is required.
     * or waiting a new connection request from the master.
     * The master will try to re-connect to the provider again if the provider doesn't support the multiple instance (process)
     * "connected" callback will be called after the provider-app gets "launch"(app-control) request.
     */
    int (*disconnected)(struct dynamicbox_event_arg *arg, void *data); /**< Disconnected from master */
    int (*connected)(struct dynamicbox_event_arg *arg, void *data); /**< Connected to master */

    /**
     * @note
     * Only for the buffer type
     * 'gbar' is short from "Glance Bar"
     */
    int (*gbar_create)(struct dynamicbox_event_arg *arg, void *data); /**< Glance Bar is created */
    int (*gbar_destroy)(struct dynamicbox_event_arg *arg, void *data); /**< Glance Bar is destroyed */
    int (*gbar_move)(struct dynamicbox_event_arg *arg, void *data); /**< Glance Bar is moved */

    int (*update_mode)(struct dynamicbox_event_arg *arg, void *data); /**< Update mode */
} *dynamicbox_event_table_h;

/**
 * @internal
 * Damaged region for updated event
 */
typedef struct dynamicbox_damage_region {
    int x; /**< Damaged region, Coordinates X of Left Top corner */
    int y; /**< Damaged region, Coordinates Y of Left Top corner */
    int w; /**< Width of Damaged region from X */
    int h; /**< Height of Damaged region from Y */
} dynamicbox_damage_region_t;

/**
 * @internal
 * @brief Initialize the provider service
 * @since_tizen 2.3
 * @details Some provider doesn't want to access environment value.\n
 *          This API will get some configuration info via argument instead of environment value.
 * @param[in] disp Display object, if you don't know what this is, set @c NULL
 * @param[in] name Slave name which is given by the master provider.
 * @param[in] table Event handler table
 * @param[in] data callback data
 * @param[in] prevent_overwrite 0 if newly created content should overwrite its old one, 1 to rename the old one before create new one.
 * @param[in] com_core_use_thread 1 to create a thread for socket communication, called packet pump, it will make a best performance for emptying the socket buffer.
 *                                if there is heavy communication, the system socket buffer can be overflow'd, to protect from socket buffer overflow, creating a
 *                                thread. created thread will do flushing all input stream of socket buffer, so the system will not suffer from buffer overflow.
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_NONE Successfully initialized
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER_PARAMETER Invalid arguments
 * @retval #DBOX_STATUS_ERROR_OUT_OF_MEMORY Out of memory
 * @retval #DBOX_STATUS_ERROR_ALREADY Already initialized
 * @retval #DBOX_STATUS_ERROR_FAULT Failed to initialize
 * @see dynamicbox_provider_fini()
 */
extern int dynamicbox_provider_init(void *disp, const char *name, dynamicbox_event_table_h table, void *data, int prevent_overwrite, int com_core_use_thread);

/**
 * @internal
 * @brief Finalize the provider service
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return void * Callback data
 * @retval callback data which is registered via provider_init. it should be cared by developer, if it should be released.
 * @see dynamicbox_provider_init()
 */
extern void *dynamicbox_provider_fini(void);

/**
 * @internal
 * @brief Send the hello signal to the master
 * @since_tizen 2.3
 * @remarks\n
 *        Master will activate connection of this only if a provider send this hello event to it.\n
 *        or the master will reject all requests from this provider.
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_NONE Successfully sent
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER_PARAMETER Invalid arguments
 * @retval #DBOX_STATUS_ERROR_FAULT Failed to initialize
 */
extern int dynamicbox_provider_send_hello(void);

/**
 * @internal
 * @brief Send the ping message to the master to notify that your provider is working properly.
 *        if the master couldn't get this ping master in ping interval, your provider will be dealt as a faulted one.
 *        you have to call this periodically. if you want keep your service provider alive.
 * @since_tizen 2.3
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_NONE Successfully sent
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER_PARAMETER provider is not initialized
 * @retval #DBOX_STATUS_ERROR_FAULT Failed to send ping message
 */
extern int dynamicbox_provider_send_ping(void);

/**
 * @internal
 * @brief Send the updated event to the master regarding updated dynamicbox content.
 * @details\n
 *  If you call this function, the output filename which is pointed by @a id will be renamed to uniq filename.\n
 *  So you cannot access the output file after this function calls.\n
 *  It only be happend when you set "prevent_overwrite" to "1" for dynamicbox_provider_init().\n
 *  But if the @a prevent_overwrite option is disabled, the output filename will not be changed.\n
 *  This is only happens when the dynamicbox uses file for content sharing method as image or description.
 * @since_tizen 2.3
 * @param[in] pkgname Package name which is an updated dynamicbox.
 * @param[in] id Instance ID of an updated dynamicbox
 * @param[in] w Width of an updated content
 * @param[in] h Height of an updated content
 * @param[in] for_gbar 1 for updating GBAR or 0 (DBox)
 * @param[in] descfile if the dbox(or gbar) is developed as script type, this can be used to deliever the name of a content description file.
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_NONE Successfully sent update event
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER_PARAMETER Invalid arguments
 * @retval #DBOX_STATUS_ERROR_FAULT There is error and it could not be recoverable
 * @see dynamicbox_provider_send_direct_updated()
 */
extern int dynamicbox_provider_send_updated(const char *pkgname, const char *id, int idx, dynamicbox_damage_region_t *region, int for_gbar, const char *descfile);

/**
 * @brief Send the updated event to the viewer directly.
 * @details\n
 *  If you call this function, the output filename which is pointed by @a id will be renamed to uniq filename.\n
 *  So you cannot access the output file after this function calls.\n
 *  But if the @a prevent_overwrite option is enabled, the output filename will not be changed.\n
 *  This is only happens when the dynamicbox uses file for content sharing method as image or description.
 * @param[in] fd connection handler
 * @param[in] pkgname Package name which is an updated dynamicbox
 * @param[in] id Instance ID of an updated dynamicbox
 * @param[in] w Width of an updated content
 * @param[in] h Height of an updated content
 * @param[in] for_gbar 1 for updating GBAR or 0 (DBox)
 * @param[in] descfile if the dbox(or gbar) is developed as script type, this can be used to deliever the name of a content description file.
 * @privlevel platform
 * @privilege %http://developer.samsung.com/privilege/core/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_NONE Successfully sent update event
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER Invalid arguments
 * @retval #DBOX_STATUS_ERROR_FAULT There is error and it could not be recoverable
 * @see dynamicbox_provider_send_updated()
 */
extern int dynamicbox_provider_send_direct_updated(int fd, const char *pkgname, const char *id, int idx, dynamicbox_damage_region_t *region, int for_gbar, const char *descfile);

/**
 * @internal
 * @brief Send extra information to the master regarding given dynamicbox instance.
 * @since_tizen 2.3
 * @param[in] priority Priority of an updated content, could be in between 0.0 and 1.0
 * @param[in] content_info Content string, used by dynamicbox.
 * @param[in] title Title string that is summarizing the content to a short text
 * @param[in] icon Absolute path of an icon file to use it for representing the contents of a box alternatively
 * @param[in] name Name for representing the contents of a box alternatively
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_NONE Successfully sent update event
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER_PARAMETER Invalid arguments
 * @retval #DBOX_STATUS_ERROR_FAULT There is error and it could not be recoverable
 */
extern int dynamicbox_provider_send_extra_info(const char *pkgname, const char *id, double priority, const char *content_info, const char *title, const char *icon, const char *name);

/**
 * @internal
 * @brief If the client requests async update mode, service provider must has to send update begin when it really start to update its contents
 * @since_tizen 2.3
 * @param[in] pkgname Package name of dynamicbox
 * @param[in] id Instance Id of dynamicbox
 * @param[in] priority Priority of current content of dynamicbox
 * @param[in] content_info Content info could be come again via dynamicbox_create() interface
 * @param[in] title A sentence for describing content of dynamicbox
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_NONE
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER_PARAMETER
 * @retval #DBOX_STATUS_ERROR_FAULT
 * @see dynamicbox_provider_send_dbox_update_end()
 */
extern int dynamicbox_provider_send_dbox_update_begin(const char *pkgname, const char *id, double priority, const char *content_info, const char *title);

/**
 * @internal
 * @brief If the client requests async update mode, service provider must has to send update end when the update is done
 * @since_tizen 2.3
 * @param[in] pkgname Package name of dynamicbox
 * @param[in] id Instance Id of dynamicbox
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_NONE
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER_PARAMETER
 * @retval #DBOX_STATUS_ERROR_FAULT
 * @see dynamicbox_provider_send_dbox_update_begin()
 */
extern int dynamicbox_provider_send_dbox_update_end(const char *pkgname, const char *id);

/**
 * @internal
 * @brief If the client requests async update mode, service provider must has to send update begin when it really start to update its contents
 * @since_tizen 2.3
 * @param[in] pkgname Package name of dynamicbox
 * @param[in] id Instance Id of dynamicbox
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_NONE
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER
 * @retval #DBOX_STATUS_ERROR_FAULT
 * @see dynamicbox_provider_send_gbar_update_end()
 */
extern int dynamicbox_provider_send_gbar_update_begin(const char *pkgname, const char *id);

/**
 * @internal
 * @brief If the client requests async update mode, service provider must has to send update end when it really finish the update its contents
 * @since_tizen 2.3
 * @param[in] pkgname Package name of dynamicbox
 * @param[in] id Instance Id of dynamicbox
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_NONE
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER
 * @retval #DBOX_STATUS_ERROR_FAULT
 * @see dynamicbox_provider_send_gbar_update_begin()
 */
extern int dynamicbox_provider_send_gbar_update_end(const char *pkgname, const char *id);

/**
 * @internal
 * @brief Send the deleted event of specified dynamicbox instance
 * @since_tizen 2.3
 * @param[in] pkgname Package name of the dynamicbox
 * @param[in] id Dynamicbox instance ID
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_NONE
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER
 * @retval #DBOX_STATUS_ERROR_FAULT
 * @see dynamicbox_provider_send_faulted()
 * @see dynamicbox_provider_send_hold_scroll()
 */
extern int dynamicbox_provider_send_deleted(const char *pkgname, const char *id);

/**
 * @internal
 * @brief If you want to use the fault management service of the master provider,\n
 *        Before call any functions of a dynamicbox, call this.
 * @since_tizen 2.3
 * @param[in] pkgname Package name of the dynamicbox
 * @param[in] id Instance ID of the dynamicbox
 * @param[in] funcname Function name which will be called
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_NONE Successfully sent
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER
 * @retval #DBOX_STATUS_ERROR_FAULT
 * @see dynamicbox_provider_send_call()
 */
extern int dynamicbox_provider_send_ret(const char *pkgname, const char *id, const char *funcname);

/**
 * @internal
 * @brief If you want to use the fault management service of the master provider,\n
 *        After call any functions of a dynamicbox, call this.
 * @since_tizen 2.3
 * @param[in] pkgname Package name of the dynamicbox
 * @param[in] id Instance ID of the dynamicbox
 * @param[in] funcname Function name which is called by the slave
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_NONE Successfully sent
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER Invalid paramters
 * @retval #DBOX_STATUS_ERROR_FAULT Failed to send a request
 * @see dynamicbox_provider_send_ret()
 */
extern int dynamicbox_provider_send_call(const char *pkgname, const char *id, const char *funcname);

/**
 * @internal
 * @brief If you want to send the fault event to the master provider,
 *        Use this API
 * @since_tizen 2.3
 * @param[in] pkgname Package name of the dynamicbox
 * @param[in] id ID of the dynamicbox instance
 * @param[in] funcname Reason of the fault error
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_NONE Successfully sent
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER Invalid parameters
 * @retval #DBOX_STATUS_ERROR_FAULT Failed to send a request
 * @see dynamicbox_provider_send_deleted()
 * @see dynamicbox_provider_send_hold_scroll()
 * @see dynamicbox_provider_send_access_status()
 * @see dynamicbox_provider_send_key_status()
 * @see dynamicbox_provider_send_request_close_gbar()
 */
extern int dynamicbox_provider_send_faulted(const char *pkgname, const char *id, const char *funcname);

/**
 * @internal
 * @brief If you want notify to viewer to seize the screen,
 *        prevent moving a box from user event
 * @since_tizen 2.3
 * @param[in] pkgname Package name of the dynamicbox
 * @param[in] id ID of the dynamicbox instance
 * @param[in] seize 1 if viewer needs to hold a box, or 0
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_NONE Successfully sent
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER Invalid parameters
 * @retval #DBOX_STATUS_ERROR_FAULT Failed to send a request
 * @see dynamicbox_provider_send_faulted()
 * @see dynamicbox_provider_send_deleted()
 * @see dynamicbox_provider_send_access_status()
 * @see dynamicbox_provider_send_key_status()
 * @see dynamicbox_provider_send_request_close_gbar()
 */
extern int dynamicbox_provider_send_hold_scroll(const char *pkgname, const char *id, int seize);

/**
 * @internal
 * @brief Notify to viewer for accessiblity event processing status.
 * @since_tizen 2.3
 * @param[in] pkgname Package name of dynamicbox
 * @param[in] id Instance Id of dynamicbox
 * @param[in] status \n
 *    You should returns one in below list, those are defined in dynamicbox-service.h header\n
 *    #DBOX_ACCESS_STATUS_ERROR If there is some error while processing accessibility events\n
 *    #DBOX_ACCESS_STATUS_DONE Successfully processed\n
 *    #DBOX_ACCESS_STATUS_FIRST Reach to the first focusable object\n
 *    #DBOX_ACCESS_STATUS_LAST Reach to the last focusable object\n
 *    #DBOX_ACCESS_STATUS_READ Read done
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_NONE Successfully sent
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER Invalid parameter
 * @retval #DBOX_STATUS_ERROR_FAULT Failed to send a status
 * @see dynamicbox_provider_send_deleted()
 * @see dynamicbox_provider_send_faulted()
 * @see dynamicbox_provider_send_hold_scroll()
 * @see dynamicbox_provider_send_key_status()
 * @see dynamicbox_provider_send_request_close_gbar()
 */
extern int dynamicbox_provider_send_access_status(const char *pkgname, const char *id, int status);

/**
 * @internal
 * @brief Notify to viewer for key event processing status.
 * @since_tizen 2.3
 * @param[in] pkgname Package name of dynamicbox
 * @param[in] id Instance Id of dynamicbox
 * @param[in] status \n
 *          Send the key event processing result to the viewer\n
 *        #DBOX_KEY_STATUS_ERROR If there is some error while processing key event\n
 *        #DBOX_KEY_STATUS_DONE Key event is successfully processed\n
 *        #DBOX_KEY_STATUS_FIRST Reach to the first focusable object\n
 *        #DBOX_KEY_STATUS_LAST Reach to the last focusable object
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_NONE Successfully sent
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER Invalid paramter
 * @retval #DBOX_STATUS_ERROR_FAULT Failed to send a status
 * @see dynamicbox_provider_send_deleted()
 * @see dynamicbox_provider_send_faulted()
 * @see dynamicbox_provider_send_hold_scroll()
 * @see dynamicbox_provider_send_access_status()
 * @see dynamicbox_provider_send_request_close_gbar()
 */
extern int dynamicbox_provider_send_key_status(const char *pkgname, const char *id, int status);

/**
 * @internal
 * @brief Send request to close the Glance Bar if it is opened.
 * @since_tizen 2.3
 * @param[in] pkgname Package name to send a request to close the Glance Bar
 * @param[in] id Instance Id of dynamicbox which should close its Glance Bar
 * @param[in] reason Reserved for future use
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_NONE Successfully sent
 * @retval #DBOX_STATUS_ERROR_FAULT Failed to send request to the viewer
 * @retval #DBOX_STATUS_ERROR_INVALID_PARAMETER Invalid argument
 * @see dynamicbox_provider_send_deleted()
 * @see dynamicbox_provider_send_faulted()
 * @see dynamicbox_provider_send_hold_scroll()
 * @see dynamicbox_provider_send_access_status()
 * @see dynamicbox_provider_send_key_status()
 */
extern int dynamicbox_provider_send_request_close_gbar(const char *pkgname, const char *id, int reason);

/**
 * @internal
 * @brief Change the provider control policy.
 * @since_tizen 2.3
 * @param[in] ctrl enumeration list - enum PROVIDER_CTRL
 * @privlevel public
 * @privilege %http://tizen.org/privilege/dynamicbox.provider
 * @return int status
 * @retval #DBOX_STATUS_ERROR_NONE Successfully changed
 */
extern int dynamicbox_provider_control(int ctrl);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

/* End of a file */
