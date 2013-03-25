/*
 * Copyright 2013  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.tizenopensource.org/license
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
#include <unistd.h>
#include <dirent.h>

#include <Elementary.h>
#include <Ecore.h>
#include <Evas.h>
#include <Ecore_Evas.h>
#include <Ecore_X.h>

#include <aul.h>
#include <dlog.h>

#include <provider.h>
#include <provider_buffer.h>

#include <mm_message.h>
#include <mm_player.h>

#include "debug.h"
#include "CVideo.h"
#include "CWindow.h"
#include "CController.h"
#include "CModel.h"

CController *CController::m_pInstance = NULL;
Ecore_Timer *CController::m_pPing = NULL;
const double CController::m_nPingInterval = 120.0f;

struct TWindowData {
	Evas_Object *list;
	char *id;
};

int CController::s_CreateLB(struct event_arg *arg, int *width, int *height, double *priority, void *data)
{
	CVideo *video;

	*width = 348;
	*height = 348;
	arg->info.lb_create.out_content = NULL;
	arg->info.lb_create.out_title = NULL;

	DbgPrint("Content: %s\n", arg->info.lb_create.content);

	video = CVideo::Create(TYPE_LB, arg->id, *width, *height);
	if (!video)
		return -EFAULT;

	return 0;
}

int CController::s_RecreateLB(struct event_arg *arg, void *data)
{
	CVideo *video;

	DbgPrint("Content: %s\n", arg->info.lb_recreate.content);

	video = CVideo::Create(TYPE_LB, arg->id, arg->info.lb_recreate.width, arg->info.lb_recreate.height);
	if (!video)
		return -EFAULT;

	return 0;
}

int CController::s_DestroyLB(struct event_arg *arg, void *data)
{
	CVideo *video;
	CWindow *win;

	video = CVideo::Find(TYPE_LB, arg->id);
	if (video)
		video->Destroy();

	win = CWindow::Find(TYPE_PD, arg->id);
	if (win)
		win->Destroy();

	return 0;
}

int CController::s_ContentEvent(struct event_arg *arg, void *data)
{
	CVideo *video;

	video = CVideo::Find(TYPE_LB, arg->id);
	if (!video)
		return -ENOENT;

	return 0;
}

int CController::s_Clicked(struct event_arg *arg, void *data)
{
	CVideo *video;

	video = CVideo::Find(TYPE_LB, arg->id);
	if (!video) {
		ErrPrint("Instance is not found: %s\n", arg->id);
		return -ENOENT;
	}

	switch (video->State()) {
	case CVideo::PLAYING:
		video->Stop();
		break;
	case CVideo::STOPPED:
		video->Play(CModel::GetInstance()->VideoFilename());
		break;
	default:
		break;
	}

	return 0;
}

int CController::s_TextEvent(struct event_arg *arg, void *data)
{
	CVideo *video;

	video = CVideo::Find(TYPE_LB, arg->id);
	if (!video)
		return -ENOENT;

	return 0;
}

int CController::s_Resize(struct event_arg *arg, void *data)
{
	CVideo *video;

	video = CVideo::Find(TYPE_LB, arg->id);
	if (!video)
		return -ENOENT;

	video->Resize(arg->info.resize.w, arg->info.resize.h);
	return 0;
}

int CController::s_SetPeriod(struct event_arg *arg, void *data)
{
	CVideo *video;

	video = CVideo::Find(TYPE_LB, arg->id);
	if (!video)
		return -ENOENT;

	return 0;
}

int CController::s_ChangeGroup(struct event_arg *arg, void *data)
{
	CVideo *video;

	video = CVideo::Find(TYPE_LB, arg->id);
	if (!video)
		return -ENOENT;

	return 0;
}

int CController::s_Pinup(struct event_arg *arg, void *data)
{
	CVideo *video;

	video = CVideo::Find(TYPE_LB, arg->id);
	if (!video)
		return -ENOENT;

	return 0;
}

int CController::s_UpdateContent(struct event_arg *arg, void *data)
{
	CVideo *video;

	video = CVideo::Find(TYPE_LB, arg->id);
	if (!video)
		return -ENOENT;

	return 0;
}

int CController::s_Pause(struct event_arg *arg, void *data)
{
	if (m_pPing)
		ecore_timer_freeze(m_pPing);

	return 0;
}

int CController::s_Resume(struct event_arg *arg, void *data)
{
	if (m_pPing)
		ecore_timer_thaw(m_pPing);

	return 0;
}

int CController::s_Disconnected(struct event_arg *arg, void *data)
{
	if (m_pPing) {
		ecore_timer_del(m_pPing);
		m_pPing = NULL;
	}

	aul_terminate_pid(getpid());
	return 0;
}

Eina_Bool CController::s_PingHandler(void *data)
{
	provider_send_ping();
	return ECORE_CALLBACK_RENEW;
}

int CController::s_Connected(struct event_arg *arg, void *data)
{
	if (m_pPing)
		return -EINVAL;

	/*!
	 * \note
	 * Phase 2. Send "hello" signal to master
	 *          Add "ping" timer (Master will wait 240 seconds)
	 */
	if (provider_send_hello() < 0)
		return -EFAULT;

	m_pPing = ecore_timer_add(m_nPingInterval, s_PingHandler, NULL);
	if (!m_pPing)
		ErrPrint("Failed to add a ping timer\n");
	return 0;
}

static void press_cb(void *data, Evas_Object *list, void *event_info)
{
	CVideo *video;
	Elm_Object_Item *item;
	const char *filename;
	char *path;
	int pathlen;
	TWindowData *win_data = (TWindowData *)data;

	video = CVideo::Find(TYPE_LB, win_data->id);
	if (!video) {
		ErrPrint("Video is not exists\n");
		return;
	}

	item = elm_list_selected_item_get(list);
	if (!item) {
		ErrPrint("Item is not selected\n");
		return;
	}

	filename = elm_object_item_part_text_get(item, NULL);
	if (!filename) {
		ErrPrint("Selected item is not valid\n");
		return;
	}

	pathlen = strlen(filename) + strlen("/opt/usr/media/Videos/") + 1;
	path = (char *)malloc(pathlen);
	if (!path) {
		ErrPrint("Heap: %s\n", strerror(errno));
		return;
	}

	snprintf(path, pathlen, "/opt/usr/media/Videos/%s", filename);
	CModel::GetInstance()->SetVideoFilename(path);
	DbgPrint("Updated Video file: %s\n", path);
	free(path);

	if (video->State() == CVideo::PLAYING)
		video->Stop();

	video->Play(CModel::GetInstance()->VideoFilename());
}

int CController::s_CreatePD(struct event_arg *arg, void *data)
{
	Evas_Object *list;
	TWindowData *win_data;
	CWindow *win;
	Evas_Object *parent;
	int w;
	int h;

	DIR *handle;

	win_data = (TWindowData *)malloc(sizeof(*win_data));
	if (!win_data) {
		ErrPrint("Heap: %s\n", strerror(errno));
		return -ENOMEM;
	}

	win = CWindow::Create(TYPE_PD, arg->id, arg->info.pd_create.w, arg->info.pd_create.h);
	if (!win) {
		ErrPrint("Window for PID is not able to create\n");
		free(win_data);
		return -EFAULT;
	}

	parent = win->Object();
	if (!parent) {
		ErrPrint("Window is not valid\n");
		free(win_data);
		return -EFAULT;
	}

	list = elm_list_add(parent);
	evas_object_geometry_get(parent, NULL, NULL, &w, &h);
	evas_object_size_hint_weight_set(list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_list_multi_select_set(list, EINA_FALSE);
	elm_list_mode_set(list, ELM_LIST_COMPRESS);
	elm_scroller_bounce_set(list, EINA_TRUE, EINA_FALSE);
	evas_object_resize(list, w, h);
	evas_object_move(list, 0, 0);
	evas_object_show(list);

	handle = opendir("/opt/usr/media/Videos/");
	if (handle) {
		struct dirent *ent;

		while ((ent = readdir(handle)))
			elm_list_item_append(list, ent->d_name, NULL, NULL /* end */, press_cb, win_data);

		closedir(handle);
	}

	elm_list_go(list);

	win_data->id = strdup(arg->id);
	if (!win_data->id) {
		evas_object_del(list);
		free(win_data);
		return -ENOMEM;
	}
	win_data->list = list;

	evas_object_data_set(parent, "win_data", win_data);
	return 0;
}

int CController::s_DestroyPD(struct event_arg *arg, void *data)
{
	CWindow *win;
	TWindowData *win_data;

	win = CWindow::Find(TYPE_PD, arg->id);
	if (!win) {
		ErrPrint("PD Window for %s is not exists\n", arg->id);
		return -ENOENT;
	}

	win_data = (TWindowData *)evas_object_data_del(win->Object(), "win_data");
	if (win_data) {
		evas_object_del(win_data->list);
		free(win_data->id);
		free(win_data);
	}

	win->Destroy();
	return 0;
}

CController::CController(void)
: m_sName(NULL)
{
}

CController::~CController(void)
{
	free(m_sName);
	m_sName = NULL;

	m_pInstance = NULL;

	if (m_pPing) {
		ecore_timer_del(m_pPing);
		m_pPing = NULL;
	}
}

int CController::Initialize(const char *name)
{
	int ret;
	struct event_handler table;

	if (CController::m_pInstance)
		return -EALREADY;

	try {
		m_pInstance = new CController();
	} catch (...) {
		return -ENOMEM;
	}

	m_pInstance->m_sName = strdup(name);
	if (!m_pInstance->m_sName) {
		delete m_pInstance;
		return -ENOMEM;
	}

	table.lb_create = CController::s_CreateLB;
	table.lb_recreate = CController::s_RecreateLB;
	table.lb_destroy = CController::s_DestroyLB;
	table.content_event = CController::s_ContentEvent;
	table.clicked = CController::s_Clicked;
	table.text_signal = CController::s_TextEvent;
	table.resize = CController::s_Resize;
	table.set_period = CController::s_SetPeriod;
	table.change_group = CController::s_ChangeGroup;
	table.pinup = CController::s_Pinup;
	table.update_content = CController::s_UpdateContent;
	table.pause = CController::s_Pause;
	table.resume = CController::s_Resume;
	table.disconnected = CController::s_Disconnected;
	table.connected = CController::s_Connected;
	table.pd_create = CController::s_CreatePD;
	table.pd_destroy = CController::s_DestroyPD;

	ret = provider_init(ecore_x_display_get(), m_pInstance->m_sName, &table, NULL);
	if (ret < 0) {
		delete m_pInstance;
		return ret;
	}

	return 0;
}

int CController::Finalize(void)
{
	if (!m_pInstance)
		return -EINVAL;

	provider_fini();

	delete m_pInstance;
	return 0;
}

/* End of a file */
