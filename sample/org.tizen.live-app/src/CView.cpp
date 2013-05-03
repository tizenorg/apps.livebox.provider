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

#include <Elementary.h>

#include <stdio.h>
#include <dlog.h>
#include <errno.h>

#include <provider.h>
#include <provider_buffer.h>

#include "debug.h"
#include "CWindow.h"
#include "CView.h"

CView::CView(void)
: m_pWindow(NULL)
, m_pScroller(NULL)
{
}

CView::~CView(void)
{
}

static void del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
	CView *view = (CView *)data;

	view->ResetLayout();
}

void CView::ResetLayout(void)
{
	m_pScroller = NULL;
}

int CView::InitLayout(void)
{
	Evas_Object *pScroller;
	Evas_Object *pIcon;
	char sTitle[256];
	int w;
	int h;
	int i;

	pScroller = elm_list_add(m_pWindow->Object());
	if (!pScroller)
		return -EFAULT;

	evas_object_geometry_get(m_pWindow->Object(), NULL, NULL, &w, &h);
	evas_object_size_hint_weight_set(pScroller, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_list_multi_select_set(pScroller, EINA_FALSE);
	elm_list_mode_set(pScroller, ELM_LIST_COMPRESS);
	elm_scroller_bounce_set(pScroller, EINA_TRUE, EINA_FALSE);
	evas_object_resize(pScroller, w, h);
	evas_object_move(pScroller, 0, 0);
	evas_object_show(pScroller);

	for (i = 0; i < 100; i++) {
		pIcon = elm_icon_add(m_pWindow->Object());
		elm_icon_file_set(pIcon, "/opt/usr/live/org.tizen.live-app/res/image/unknown.png", NULL);
		elm_icon_resizable_set(pIcon, 1, 1);
		snprintf(sTitle, sizeof(sTitle), "%d List item: %d", i, i);
		elm_list_item_append(pScroller, sTitle, pIcon, NULL /* end */, NULL, NULL);
	}

	elm_list_go(pScroller);
	m_pScroller = pScroller;
	evas_object_event_callback_add(m_pScroller, EVAS_CALLBACK_DEL, del_cb, this);
	return 0;
}

CView *CView::Create(CWindow *win)
{
	CView *pInst;

	if (!win)
		return NULL;

	try {
		pInst = new CView();
	} catch (...) {
		return NULL;
	}

	pInst->m_pWindow = win;
	pInst->InitLayout();
	return pInst;
}

int CView::Destroy(void)
{
	if (m_pScroller) {
		evas_object_del(m_pScroller);
		m_pScroller = NULL;
	}

	delete this;
	return 0;
}

/* End of a file */
