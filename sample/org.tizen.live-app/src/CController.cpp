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

#include <Ecore.h>
#include <Evas.h>
#include <Ecore_Evas.h>
#include <Ecore_X.h>
#include <Eina.h>

#include <aul.h>
#include <dlog.h>

#include <provider.h>
#include <provider_buffer.h>

#include "debug.h"
#include "CWindow.h"
#include "CController.h"
#include "CView.h"
#include "CModel.h"

CController *CController::m_pInstance = NULL;
Ecore_Timer *CController::m_pPing = NULL;
const double CController::m_nPingInterval = 120.0f;

int CController::s_CreateLB(struct event_arg *arg, int *width, int *height, double *priority, void *data)
{
	CWindow *win;
	CView *view;
	int ret;

	*width = 348;
	*height = 348;
	arg->info.lb_create.out_content = NULL;
	arg->info.lb_create.out_title = NULL;

	win = CWindow::Create(TYPE_LB, arg->id, *width, *height);
	if (!win)
		return -EFAULT;

	view = CView::Create(win);
	if (!view) {
		win->Destroy();
		return -EFAULT;
	}

	ret = CModel::GetInstance()->AddTaggedObject(arg->id, view);
	if (ret < 0) {
		view->Destroy();
		win->Destroy();
		return ret;
	}

	return 0;
}

int CController::s_RecreateLB(struct event_arg *arg, void *data)
{
	CWindow *win;
	CView *view;
	int ret;

	win = CWindow::Create(TYPE_LB, arg->id, arg->info.lb_recreate.width, arg->info.lb_recreate.height);
	if (!win) {
		ErrPrint("Failed to create a window %s\n", arg->id);
		return -EFAULT;
	}

	view = CView::Create(win);
	if (!view) {
		ErrPrint("Failed to create a view %s\n", arg->id);
		win->Destroy();
		return -EFAULT;
	}

	ret = CModel::GetInstance()->AddTaggedObject(arg->id, view);
	if (ret < 0) {
		ErrPrint("Failed to add a view %s\n", arg->id);
		view->Destroy();
		win->Destroy();
		return ret;
	}

	return 0;
}

int CController::s_DestroyLB(struct event_arg *arg, void *data)
{
	CWindow *win;
	CView *view;

	view = (CView *)CModel::GetInstance()->DelTaggedObject(arg->id);
	if (view)
		view->Destroy();

	win = CWindow::Find(TYPE_LB, arg->id);
	if (win)
		win->Destroy();

	return 0;
}

int CController::s_ContentEvent(struct event_arg *arg, void *data)
{
	CWindow *win;

	win = CWindow::Find(TYPE_LB, arg->id);
	if (!win)
		return -ENOENT;

	return 0;
}

int CController::s_Clicked(struct event_arg *arg, void *data)
{
	CWindow *win;

	win = CWindow::Find(TYPE_LB, arg->id);
	if (!win)
		return -ENOENT;

	return 0;
}

int CController::s_TextEvent(struct event_arg *arg, void *data)
{
	CWindow *win;

	win = CWindow::Find(TYPE_LB, arg->id);
	if (!win)
		return -ENOENT;

	return 0;
}

int CController::s_Resize(struct event_arg *arg, void *data)
{
	CWindow *win;

	win = CWindow::Find(TYPE_LB, arg->id);
	if (!win)
		return -ENOENT;

	win->Resize(arg->info.resize.w, arg->info.resize.h);
	return 0;
}

int CController::s_SetPeriod(struct event_arg *arg, void *data)
{
	CWindow *win;

	win = CWindow::Find(TYPE_LB, arg->id);
	if (!win)
		return -ENOENT;

	return 0;
}

int CController::s_ChangeGroup(struct event_arg *arg, void *data)
{
	CWindow *win;

	win = CWindow::Find(TYPE_LB, arg->id);
	if (!win)
		return -ENOENT;

	return 0;
}

int CController::s_Pinup(struct event_arg *arg, void *data)
{
	CWindow *win;

	win = CWindow::Find(TYPE_LB, arg->id);
	if (!win)
		return -ENOENT;

	return 0;
}

int CController::s_UpdateContent(struct event_arg *arg, void *data)
{
	CWindow *win;

	win = CWindow::Find(TYPE_LB, arg->id);
	if (!win)
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

	delete m_pInstance;
	aul_terminate_pid(getpid());
	return 0;
}

Eina_Bool CController::s_PingHandler(void *data)
{
	DbgPrint("Send ping\n");
	provider_send_ping();
	return ECORE_CALLBACK_RENEW;
}

int CController::s_Connected(struct event_arg *arg, void *data)
{
	int ret;

	if (m_pPing) {
		DbgPrint("Already connected\n");
		return -EINVAL;
	}

	/*!
	 * \note
	 * Phase 2. Send "hello" signal to master
	 *          Add "ping" timer (Master will wait 240 seconds)
	 */
	ret = provider_send_hello();
	if (ret < 0) {
		ErrPrint("Failed to send a ping(%d)\n", ret);
		return -EFAULT;
	}

	m_pPing = ecore_timer_add(m_nPingInterval, s_PingHandler, NULL);
	if (!m_pPing)
		ErrPrint("Failed to add a ping timer\n");

	DbgPrint("Connected\n");
	return 0;
}

int CController::s_CreatePD(struct event_arg *arg, void *data)
{
	CWindow *win;
	CView *view;
	int ret;
	char *tagname;

	win = CWindow::Create(TYPE_PD, arg->id, arg->info.pd_create.w, arg->info.pd_create.h);
	if (!win) {
		ErrPrint("Failed to create a PD\n");
		return -EFAULT;
	}

	view = CView::Create(win);
	if (!view) {
		ErrPrint("Failed to create a view\n");
		win->Destroy();
		return -EFAULT;
	}

	ret = strlen(arg->id); /* reuse ret */
	tagname = (char *)malloc(ret + strlen(",pd,view") + 1);
	if (!tagname) {
		ErrPrint("Heap: %s\n", strerror(errno));
		win->Destroy();
		return -EFAULT;
	}

	strcpy(tagname, arg->id);
	strcpy(tagname + ret, ",pd,view");
	DbgPrint("TAGNAME: %s\n", tagname);
	
	ret = CModel::GetInstance()->AddTaggedObject(tagname, view);
	free(tagname);
	if (ret < 0) {
		DbgPrint("Failed to add a tagged object\n");
		view->Destroy();
		win->Destroy();
		return ret;
	}

	DbgPrint("PD is created\n");
	return 0;
}

int CController::s_DestroyPD(struct event_arg *arg, void *data)
{
	CWindow *win;
	CView *view;
	int ret;
	char *tagname;

	win = CWindow::Find(TYPE_PD, arg->id);
	if (!win) {
		ErrPrint("Failed to find a window (%s)\n", arg->id);
		return -ENOENT;
	}

	ret = strlen(arg->id);
	tagname = (char *)malloc(ret + strlen(",pd,view") + 1);
	if (!tagname) {
		ErrPrint("Heap: %s\n", strerror(errno));
		return -ENOMEM;
	}

	strcpy(tagname, arg->id);
	strcpy(tagname + ret, ",pd,view");

	view = (CView *)CModel::GetInstance()->DelTaggedObject(tagname);
	free(tagname);
	if (view) {
		DbgPrint("Destroy VIEW\n");
		view->Destroy();
	}

	DbgPrint("Destroy WIN\n");
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

	DbgPrint("Initialize %s\n", name);

	if (CController::m_pInstance) {
		ErrPrint("Controller is already exists\n");
		return -EALREADY;
	}

	try {
		m_pInstance = new CController();
	} catch (...) {
		ErrPrint("Failed to create a controller\n");
		return -ENOMEM;
	}

	m_pInstance->m_sName = strdup(name);
	if (!m_pInstance->m_sName) {
		ErrPrint("Heap: %s\n", strerror(errno));
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

	DbgPrint("Slave name is %s\n", m_pInstance->m_sName);
	ret = provider_init(ecore_x_display_get(), m_pInstance->m_sName, &table, NULL);
	if (ret < 0) {
		ErrPrint("Failed to initialize the provider %d\n", ret);
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
