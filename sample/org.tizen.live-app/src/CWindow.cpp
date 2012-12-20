/*
 * Copyright 2012  Samsung Electronics Co., Ltd
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

#include <Elementary.h>

#include <Ecore_Evas.h>
#include <Ecore.h>
#include <Evas.h>

#include <dlog.h>

#include <provider.h>
#include <provider_buffer.h>

#include "debug.h"
#include "CWindow.h"

Eina_List *CWindow::s_pList = NULL;

void *CWindow::s_AllocateCanvas(void *data, int size)
{
	CWindow *win = (CWindow *)data;
	struct livebox_buffer *pBuffer;
	void *canvas;

	pBuffer = provider_buffer_acquire(win->Type(), PKGNAME, win->Id(),
					win->Width(), win->Height(), sizeof(int),
					CWindow::s_BufferEventHandler, win);

	if (!pBuffer)
		return NULL;

	win->SetBuffer(pBuffer);
	canvas = provider_buffer_ref(win->Buffer());
	return canvas;
}

void CWindow::s_ReleaseCanvas(void *data, void *canvas)
{
	int ret;
	CWindow *win = (CWindow *)data;

	ret = provider_buffer_unref(canvas);
	DbgPrint("Unref %d\n", ret);
		
	ret = provider_buffer_release(win->Buffer());
	DbgPrint("release %d\n", ret);

	win->SetBuffer(NULL);
}

void CWindow::s_PostRender(void *data, Evas *e, void *event_info)
{
	CWindow *win = (CWindow *)data;

	if (win->EvasObject() != e)
		return;

	switch (win->Type()) {
	case TYPE_PD:
		provider_buffer_sync(win->Buffer());
		provider_send_desc_updated(PKGNAME, win->Id(), NULL);
		break;
	case TYPE_LB:
		provider_buffer_sync(win->Buffer());
		provider_send_updated(PKGNAME, win->Id(), win->Width(), win->Height(), 1.0f, NULL, NULL);
		break;
	default:
		break;
	}
}

int CWindow::s_BufferEventHandler(struct livebox_buffer *buffer, enum buffer_event event, double timestamp, double x, double y, void *data)
{
	CWindow *win = (CWindow *)data;
	int ix;
	int iy;

	ix = win->Width() * x;
	iy = win->Height() * y;

	DbgPrint("POS(%dx%d)\n", ix, iy);

	switch (event) {
	case BUFFER_EVENT_ENTER:
		evas_event_feed_mouse_in(win->EvasObject(), timestamp, NULL);
		break;
	case BUFFER_EVENT_LEAVE:
		evas_event_feed_mouse_out(win->EvasObject(), timestamp, NULL);
		break;
	case BUFFER_EVENT_DOWN:
		//evas_event_feed_mouse_in(win->EvasObject(), timestamp, NULL);
		evas_event_feed_mouse_move(win->EvasObject(), ix, iy, timestamp + 0.01f, NULL);
		evas_event_feed_mouse_down(win->EvasObject(), 1, EVAS_BUTTON_NONE, timestamp + 0.02f, NULL);
		break;
	case BUFFER_EVENT_MOVE:
		evas_event_feed_mouse_move(win->EvasObject(), ix, iy, timestamp, NULL);
		break;
	case BUFFER_EVENT_UP:
		evas_event_feed_mouse_up(win->EvasObject(), 1, EVAS_BUTTON_NONE, timestamp, NULL);
		//evas_event_feed_mouse_out(win->EvasObject(), timestamp + 0.01f, NULL);
		break;
	default:
		break;
	}

	return 0;
}

CWindow *CWindow::Create(enum target_type type, const char *id, int w, int h)
{
	CWindow *win;

	try {
		win = new CWindow(type, w, h);
	} catch (...) {
		return NULL;
	}

	win->m_sID = strdup(id);
	if (!win->m_sID) {
		win->Destroy();
		return NULL;
	}

	win->m_pEcoreEvas = ecore_evas_buffer_allocfunc_new(win->m_nWidth, win->m_nHeight, CWindow::s_AllocateCanvas, CWindow::s_ReleaseCanvas, win);
	if (!win->m_pEcoreEvas) {
		ErrPrint("Failed to allocate a new EE\n");
		win->Destroy();
		return NULL;
	}

	win->m_pEvas = ecore_evas_get(win->m_pEcoreEvas);
	if (!win->m_pEvas) {
		win->Destroy();
		return NULL;
	}

	evas_event_callback_add(win->m_pEvas, EVAS_CALLBACK_RENDER_FLUSH_POST, s_PostRender, win);

	ecore_evas_alpha_set(win->m_pEcoreEvas, EINA_TRUE);
	ecore_evas_manual_render_set(win->m_pEcoreEvas, EINA_FALSE);
	ecore_evas_resize(win->m_pEcoreEvas, win->m_nWidth, win->m_nHeight);
	ecore_evas_show(win->m_pEcoreEvas);
	ecore_evas_activate(win->m_pEcoreEvas);

	win->m_pObject = evas_object_rectangle_add(win->m_pEvas);
	evas_object_resize(win->m_pObject, win->m_nWidth, win->m_nHeight);
	evas_object_color_set(win->m_pObject, 0, 0, 0, 255);
	evas_object_show(win->m_pObject);

	s_pList = eina_list_append(s_pList, win);
	return win;
}

CWindow *CWindow::Find(enum target_type type, const char *id)
{
	Eina_List *l;
	void *item;
	CWindow *win;

	EINA_LIST_FOREACH(s_pList, l, item) {
		win = (CWindow *)item;

		if (win->Type() == type && !strcmp(win->Id(), id))
			return win;
	}

	return NULL;
}

int CWindow::Destroy(void)
{
	s_pList = eina_list_remove(s_pList, this);

	if (m_pObject)
		evas_object_del(m_pObject);

	if (m_pEcoreEvas)
		ecore_evas_free(m_pEcoreEvas);

	free(m_sID);
	delete this;
	return 0;
}

int CWindow::Resize(int w, int h)
{
	m_nWidth = w;
	m_nHeight = h;

	if (m_pEcoreEvas)
		ecore_evas_resize(m_pEcoreEvas, w, h);

	return 0;
}

CWindow::CWindow(enum target_type type, int width, int height)
: m_vType(type)
, m_nWidth(width)
, m_nHeight(height)
, m_pBuffer(NULL)
, m_pEcoreEvas(NULL)
, m_pEvas(NULL)
, m_sID(NULL)
{
}

CWindow::~CWindow(void)
{
}

/* End of a file */
