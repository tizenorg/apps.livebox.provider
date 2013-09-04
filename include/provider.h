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

#ifndef __PROVIDER_H
#define __PROVIDER_H

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * \addtogroup CAPI_PROVIDER_MODULE
 * \{
 */

/*!
 * \brief
 * Text signal & Content event uses this data structure.
 */
struct event_info {
        struct pointer {
                double x; /*!< X value of current mouse(touch) position */
                double y; /*!< Y value of current mouse(touch) position */
                int down; /*!< Is it pressed(1) or not(0) */
        } pointer;

        struct part {
                double sx; /*!< Pressed object's left top X */
                double sy; /*!< Pressed object's left top Y */
                double ex; /*!< Pressed object's right bottom X */
                double ey; /*!< Pressed object's right bottom Y */
        } part;
};

struct event_arg {
	enum event_type {
		EVENT_NEW, /*!< Master will send this to create a new livebox instance */
		EVENT_RENEW, /*!< If the master detects any problems of your slave, it will terminate slave provider.
		                  and after reactivating the provider slave, this request will be delievered to create
				  a livebox instance again */

		EVENT_DELETE, /*!< Master will send this to delete a livebox instance */

		EVENT_CONTENT_EVENT, /*!< Any events are generated from your livebox or PD, this event will be sent to you */
		EVENT_CLICKED, /*!< If a livebox is clicked, the master will send this event */
		EVENT_TEXT_SIGNAL, /*!< Text type livebox or PD will generate this event */

		EVENT_RESIZE, /*!< If a livebox is resized, the master will send this event */
		EVENT_SET_PERIOD, /*!< To change the update period of a livebox */
		EVENT_CHANGE_GROUP, /*!< To change the group(cluster/sub-cluster) of a livebox */
		EVENT_PINUP, /*!< To make pin up of a livebox */

		EVENT_UPDATE_CONTENT, /*!< It's time to update the content of a livebox */

		EVENT_PAUSE, /*!< Freeze all timer and go to sleep mode */
		EVENT_RESUME, /*!< Thaw all timer and wake up */

		EVENT_PD_CREATE, /*!< Only for the buffer type */
		EVENT_PD_DESTROY, /*!< Only for the buffer type */
		EVENT_PD_MOVE, /*!< Only for the buffer type */

		EVENT_LB_PAUSE, /*!< Freeze the update timer of a specified livebox */
		EVENT_LB_RESUME, /*!< Thaw the update timer of a specified livebox */

		EVENT_PD_ACCESS, /* PD: Accessibility event */
		EVENT_LB_ACCESS /* LB: Accessibility event */
	} type;
	const char *pkgname; /*!< Package name of a livebox */
	const char *id; /*!< Instance Id of a livebox */

	union event_data {
		struct pd_create {
			int w; /*!< PD buffer is created with width "w" */
			int h; /*!< PD buffer is created with height "h" */

			double x; /*!< Relative X position of a livebox from this PD */
			double y; /*!< Relative Y position of a livebox from this PD */
		} pd_create;

		struct pd_destroy {
		} pd_destroy;

		struct pd_move {
			int w; /*!< PD buffer width */
			int h; /*!< PD buffer height */

			double x; /*!< Relative X position of a livebox from this PD */
			double y; /*!< Relative Y position of a livebox from this PD */
		} pd_move;

		struct lb_create {
			const char *content; /*!< Content info */
			int timeout; /*!< Timeout */
			int has_script; /*!< Livebox has script (buffer is created from the master) */
			double period; /*!< Update period */
			const char *cluster; /*!< Cluster ID of this livebox instance */
			const char *category; /*!< Sub-cluster ID of this livebox instance */
			int skip_need_to_create; /*!< Is this livebox need to check the "need_to_create"? */
			int width; /*!< Width of a livebox content */
			int height; /*!< Height of a livebox content */
			const char *abi; /*!< ABI tag of this livebox */
			char *out_content; /*!< Output content */
			char *out_title; /*!< Output title */
			int out_is_pinned_up; /*!< Is this pinned up? */
		} lb_create; /*!< "new" */

		struct lb_recreate {
			const char *content;
			int timeout;
			int has_script;
			double period;
			const char *cluster;
			const char *category;
			int width; /*!< Width of a livebox content */
			int height; /*!< Height of a livebox content */
			const char *abi;
			char *out_content; /*!< Output content */
			char *out_title; /*!< Output title */
			int out_is_pinned_up; /*!< Is this pinned up? */
			int hold_scroll; /*!< The scroller which is in viewer is locked */
			int active_update; /*!< Need to know, current update mode */
		} lb_recreate; /*!< renew */

		struct lb_destroy {
			/* This enumeration value must has to be sync with data-provider-master */
			enum instance_destroy_type {
				INSTANCE_DESTROY_DEFAULT,
				INSTANCE_DESTROY_PKGMGR,
				INSTANCE_DESTROY_TERMINATE,
				INSTANCE_DESTROY_FAULT,
				INSTANCE_DESTROY_UNKNOWN,
			} type;
		} lb_destroy; /*!< delete */

		struct content_event {
			const char *emission; /*!< Event string */
			const char *source; /*!< Object ID which makes event */
			struct event_info info;
		} content_event; /*!< script */

		struct clicked {
			const char *event; /*!< Event type, currently only "click" supported */
			double timestamp; /*!< Timestamp of event occurred */
			double x; /*!< X position of the click event */
			double y; /*!< Y position of the click event */
		} clicked; /*!< clicked */

		struct text_signal {
			const char *emission; /*!< Event string */
			const char *source; /*!< Object ID which makes event */
			struct event_info info;
		} text_signal; /*!< text_signal */

		struct resize {
			int w; /*!< New width of a livebox */
			int h; /*!< New height of a livebox */
		} resize; /*!< resize */

		struct set_period {
			double period; /*!< New period */
		} set_period; /*!< set_period */

		struct change_group {
			const char *cluster;
			const char *category;
		} change_group; /*!< change_group */

		struct pinup {
			int state;
			char *content_info; /* out value */
		} pinup; /*!< pinup */

		struct update_content {
			const char *cluster;
			const char *category;
		} update_content; /*! update_content */

		struct pause {
			double timestamp;
		} pause; /*!< pause */

		struct resume {
			double timestamp;
		} resume; /*!< resume */

		struct lb_pause {
			/*!< */
		} lb_pause;

		struct lb_resume {
			/*!< */
		} lb_resume;

		struct update_mode {
			int active_update;
		} update_mode;
	} info;
};

struct event_handler {
	int (*lb_create)(struct event_arg *arg, int *width, int *height, double *priority, void *data); /*!< new */
	int (*lb_destroy)(struct event_arg *arg, void *data); /*!< delete */

	/* Recover from the fault of slave */
	int (*lb_recreate)(struct event_arg *arg, void *data); /*!< Re create */

	int (*lb_pause)(struct event_arg *arg, void *data); /*!< Pause Livebox */
	int (*lb_resume)(struct event_arg *arg, void *data); /*!< Resume Livebox */

	int (*content_event)(struct event_arg *arg, void *data); /*!< Content event */
	int (*clicked)(struct event_arg *arg, void *data); /*!< Clicked */
	int (*text_signal)(struct event_arg *arg, void *data); /*!< Text Signal */
	int (*resize)(struct event_arg *arg, void *data); /*!< Resize */
	int (*set_period)(struct event_arg *arg, void *data); /*!< Set Period */
	int (*change_group)(struct event_arg *arg, void *data); /*!< Change group */
	int (*pinup)(struct event_arg *arg, void *data); /*!< Pin up */
	int (*update_content)(struct event_arg *arg, void *data); /*!< Update content */

	int (*pause)(struct event_arg *arg, void *data); /*!< Pause service provider */
	int (*resume)(struct event_arg *arg, void *data); /*!< Resume service provider */

	int (*disconnected)(struct event_arg *arg, void *data); /*!< Disconnected from master */
	int (*connected)(struct event_arg *arg, void *data); /*!< Connected to master */

	/*!
	 * \note
	 * Only for the buffer type
	 */
	int (*pd_create)(struct event_arg *arg, void *data); /*!< PD is created */
	int (*pd_destroy)(struct event_arg *arg, void *data); /*!< PD is destroyed */
	int (*pd_move)(struct event_arg *arg, void *data); /*!< PD is moved */

	int (*update_mode)(struct event_arg *arg, void *data); /*!< Update mode */
};

/*!
 * \brief Initialize the provider service
 * \details N/A
 * \remarks N/A
 * \param[in] disp XDisplay object, if you don't know what this is, set NULL
 * \param[in] name Slave name which is given by the master provider.
 * \param[in] table Event handler table
 * \param[in] data callback data
 * \return int
 * \retval LB_STATUS_SUCCESS
 * \retval LB_STATUS_ERROR_INVALID
 * \retval LB_STATUS_ERROR_MEMORY
 * \retval LB_STATUS_ERROR_ALREADY
 * \retval LB_STATUS_ERROR_FAULT
 * \pre N/A
 * \post N/A
 * \see provider_fini
 */
extern int provider_init(void *disp, const char *name, struct event_handler *table, void *data);

/*!
 * \brief Finalize the provider service
 * \details N/A
 * \remarks N/A
 * \return void *
 * \retval callback data which is registered via provider_init
 * \pre N/A
 * \post N/A
 * \see provider_init
 */
extern void *provider_fini(void);

/*!
 * \brief Send the hello signal to the master
 * \details N/A
 * \remarks N/A
 *        Master will activate connection of this only if you send this hello event.
 *        or the master will reject all requests from your provider.
 * \return int
 * \retval LB_STATUS_SUCCESS
 * \retval LB_STATUS_ERROR_INVALID
 * \retval LB_STATUS_ERROR_FAULT
 * \pre N/A
 * \post N/A
 * \see N/A
 */
extern int provider_send_hello(void);

/*!
 * \brief Send the ping message to the master to notify that your provider is working properly.
 * \details N/A
 * \remarks N/A
 * \return int
 * \retval LB_STATUS_SUCCESS
 * \retval LB_STATUS_ERROR_INVALID
 * \retval LB_STATUS_ERROR_FAULT
 * \pre N/A
 * \post N/A
 * \see N/A
 */
extern int provider_send_ping(void);

/*!
 * \brief Send the updated event to the master about livebox.
 * \details
 *  If you call this function, the output filename which is pointed by "id" will be renamed to uniq filename.
 *  So you cannot access the output file after this function calls.
 *  But if the "prevent_overwrite" option is enabled, the output filename will not be changed.
 *  This is only happes when the livebox uses "image file" or "text description file".
 * \remarks N/A
 * \param[in] pkgname Package name which of an updated livebox.
 * \param[in] id Instance ID of an updated livebox.
 * \param[in] w Width of an updated content
 * \param[in] h Height of an updated content
 * \param[in] priority Priority of an updated content
 * \param[in] content Content string
 * \param[in] title Title string that is summarizing the content to a short text
 * \return int
 * \retval LB_STATUS_SUCCESS Successfully sent update event.
 * \retval LB_STATUS_ERROR_INVALID Invalid arguments.
 * \retval LB_STATUS_ERROR_FAULT There is error and it could not be recoverable.
 * \pre N/A
 * \post N/A
 * \see provider_send_desc_updated
 */
extern int provider_send_updated(const char *pkgname, const char *id, int w, int h, double priority, const char *content, const char *title);

/*!
 * \brief Send the description data updated event to the master about PD.
 * \details
 *  If you call this function, the descfile will be renamed.
 * \remarks N/A
 * \param[in] pkgname Package name of an updated livebox.
 * \param[in] id Instance ID of an updated livebox.
 * \param[in] descfile The filename of a description file.
 * \return int
 * \retval LB_STATUS_SUCCESS
 * \retval LB_STATUS_ERROR_INVALID
 * \retval LB_STATUS_ERROR_MEMORY
 * \retval LB_STATUS_ERROR_FAULT
 * \pre N/A
 * \post N/A
 * \see provider_send_updated
 */
extern int provider_send_desc_updated(const char *pkgname, const char *id, const char *descfile);

/*!
 * \brief If the client requests async update mode, service provider must has to send update begin when it really start to update its contents
 * \details N/A
 * \remarks N/A
 * \param[in] pkgname Package name of livebox
 * \param[in] id Instance Id of livebox
 * \param[in] priority Priority of current content of livebox
 * \param[in] content_info Content info should be come back again via livebox_create event.
 * \param[in] title A sentence for describing content of livebox
 * \return int
 * \retval LB_STATUS_SUCCESS
 * \retval LB_STATUS_ERROR_INVALID
 * \retval LB_STATUS_ERROR_FAULT
 * \pre N/A
 * \post N/A
 * \see provider_send_lb_update_end
 */
extern int provider_send_lb_update_begin(const char *pkgname, const char *id, double priority, const char *content_info, const char *title);

/*!
 * \brief If the client requests async update mode, service provider must has to send update end when the update is done
 * \details N/A
 * \remarks N/A
 * \param[in] pkgname Package name of livebox
 * \param[in] id Instance Id of livebox
 * \return int
 * \retval LB_STATUS_SUCCESS
 * \retval LB_STATUS_ERROR_INVALID
 * \retval LB_STATUS_ERROR_FAULT
 * \pre N/A
 * \post N/A
 * \see provider_send_lb_update_begin
 */
extern int provider_send_lb_update_end(const char *pkgname, const char *id);

/*!
 * \brief If the client requests async update mode, service provider must has to send update begin when it really start to update its contents
 * \details N/A
 * \remarks N/A
 * \param[in] pkgname Package name of livebox
 * \param[in] id Instance Id of livebox
 * \return int
 * \retval LB_STATUS_SUCCESS
 * \retval LB_STATUS_ERROR_INVALID
 * \retval LB_STATUS_ERROR_FAULT
 * \pre N/A
 * \post N/A
 * \see provider_send_pd_update_end
 */
extern int provider_send_pd_update_begin(const char *pkgname, const char *id);

/*!
 * \brief If the client requests async update mode, service provider must has to send update end when it really finish the update its contents
 * \details N/A
 * \remarks N/A
 * \param[in] pkgname Package name of livebox
 * \param[in] id Instance Id of livebox
 * \return int
 * \retval LB_STATUS_SUCCESS
 * \retval LB_STATUS_ERROR_INVALID
 * \retval LB_STATUS_ERROR_FAULT
 * \pre N/A
 * \post N/A
 * \see provider_send_pd_update_begin
 */
extern int provider_send_pd_update_end(const char *pkgname, const char *id);

/*!
 * \brief Send the deleted event of specified livebox instance
 * \details N/A
 * \remarks N/A
 * \param[in] pkgname Package name of the livebox
 * \param[in] id Livebox instance ID
 * \return int
 * \retval LB_STATUS_SUCCESS
 * \retval LB_STATUS_ERROR_INVALID
 * \retval LB_STATUS_ERROR_FAULT
 * \pre N/A
 * \post N/A
 * \see provider_send_faulted
 * \see provider_send_hold_scroll
 */
extern int provider_send_deleted(const char *pkgname, const char *id);

/*!
 * \brief If you want to use the fault management service of the master provider,
 *        Before call any functions of a livebox, call this.
 * \param[in] pkgname Package name of the livebox
 * \param[in] id Instance ID of the livebox
 * \param[in] funcname Function name which will be called
 * \return int
 * \retval LB_STATUS_SUCCESS
 * \retval LB_STATUS_ERROR_INVALID
 * \retval LB_STATUS_ERROR_FAULT
 * \pre N/A
 * \post N/A
 * \see provider_send_call
 */
extern int provider_send_ret(const char *pkgname, const char *id, const char *funcname);

/*!
 * \brief If you want to use the fault management service of the master provider,
 * \details N/A
 * \remarks N/A
 *        After call any functions of a livebox, call this.
 * \param[in] pkgname Package name of the livebox
 * \param[in] id Instance ID of the livebox
 * \param[in] funcname Function name which is called by the slave
 * \return int
 * \retval LB_STATUS_SUCCESS
 * \retval LB_STATUS_ERROR_INVALID
 * \retval LB_STATUS_ERROR_FAULT
 * \pre N/A
 * \post N/A
 * \see provider_send_ret
 */
extern int provider_send_call(const char *pkgname, const char *id, const char *funcname);

/*!
 * \brief If you want to send the fault event to the master provider,
 * \details N/A
 * \remarks N/A
 *        Use this API
 * \param[in] pkgname Package name of the livebox
 * \param[in] id ID of the livebox instance
 * \param[in] funcname Reason of the fault error
 * \return int
 * \retval LB_STATUS_SUCCESS
 * \retval LB_STATUS_ERROR_INVALID
 * \retval LB_STATUS_ERROR_FAULT
 * \pre N/A
 * \post N/A
 * \see provider_send_deleted
 * \see provider_send_hold_scroll
 * \see provider_send_access_status
 */
extern int provider_send_faulted(const char *pkgname, const char *id, const char *funcname);

/*!
 * \brief If you want notify to viewer to seize the screen,
 * \details N/A
 * \remarks N/A
 *        prevent moving a box from user event
 * \param[in] pkgname Package name of the livebox
 * \param[in] id ID of the livebox instance
 * \param[in] seize 1 if viewer needs to hold a box, or 0
 * \return int
 * \retval LB_STATUS_SUCCESS
 * \retval LB_STATUS_ERROR_INVALID
 * \retval LB_STATUS_ERROR_FAULT
 * \pre N/A
 * \post N/A
 * \see provider_send_faulted
 * \see provider_send_deleted
 * \see provider_send_access_status
 */
extern int provider_send_hold_scroll(const char *pkgname, const char *id, int seize);

/*!
 * \brief Notify to viewer for accessiblity event processing status.
 * \details N/A
 * \remarks N/A
 * \param[in] pkgname Package name of livebox
 * \param[in] id Instance Id of livebox
 * \param[in] status
 * \return int
 * \retval LB_STATUS_SUCCESS
 * \retval LB_STATUS_ERROR_INVALID
 * \retval LB_STATUS_ERROR_FAULT
 * \pre N/A
 * \post N/A
 * \see provider_send_deleted
 * \see provider_send_faulted
 * \see provider_send_hold_scroll
 */
extern int provider_send_access_status(const char *pkgname, const char *id, int status);

/*!
 * \}
 */

#ifdef __cplusplus
}
#endif

#endif

/* End of a file */
