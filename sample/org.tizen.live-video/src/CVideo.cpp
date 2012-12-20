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

#include <X11/Xlib.h>
#include <Ecore.h>
#include <Ecore_X.h>

#include <dlog.h>

#include <provider.h>
#include <provider_buffer.h>

#include <mm_types.h>
#include <mm_error.h>
#include <mm_message.h>
#include <mm_player.h>

#include "debug.h"
#include "CWindow.h"
#include "CVideo.h"
#include "CModel.h"

Eina_List *CVideo::s_pList = NULL;


CVideo *CVideo::Find(enum target_type type, const char *id)
{
	Eina_List *l;
	void *item;
	CVideo *video;

	EINA_LIST_FOREACH(s_pList, l, item) {
		video = (CVideo *)item;

		if (video->Type() == type && !strcmp(video->Id(), id))
			return video;
	}

	return NULL;
}

CVideo *CVideo::Create(enum target_type type, const char *id, int w, int h)
{
	CVideo *video;
	struct livebox_buffer *pBuffer;

	if (!ecore_x_damage_query()) {
		ErrPrint("Damage event is not supported\n");
		return NULL;
	}

	try {
		video = new CVideo(type, w, h);
	} catch (...) {
		ErrPrint("Failed to create a video instance\n");
		return NULL;
	}

	video->m_sId = strdup(id);
	if (!video->m_sId) {
		ErrPrint("Heap: %s\n", strerror(errno));
		video->Destroy();
		return NULL;	
	}

	pBuffer = provider_buffer_acquire(video->Type(), PKGNAME, video->Id(),
						video->Width(), video->Height(), sizeof(int),
						CVideo::s_BufferEventHandler, video);
	if (!pBuffer) {
		ErrPrint("Failed to acquire buffer handle\n");
		video->Destroy();
		return NULL;
	}

	/*!
	 * \note
	 * this function must has to be called before add an item to the s_pList
	 */
	video->SetBuffer(pBuffer);

	s_pList = eina_list_append(s_pList, video);
	return video;
}

Eina_Bool CVideo::s_DamageEventHandler(void *data, int type, void *event)
{
	CVideo *video = (CVideo *)data;
	Ecore_X_Event_Damage *e = (Ecore_X_Event_Damage *)event;

	if (e->drawable == video->PixmapId()) {
		switch (video->Type()) {
		case TYPE_LB:
			provider_send_updated(PKGNAME, video->Id(), video->Width(), video->Height(), 1.0f, NULL, NULL);
			break;
		case TYPE_PD:
			provider_send_desc_updated(PKGNAME, video->Id(), NULL);
			break;
		default:
			ErrPrint("Unknown video type\n");
			break;
		}

		video->Update();
	}

	return ECORE_CALLBACK_PASS_ON;
}

int CVideo::Update(void)
{
	ecore_x_damage_subtract(m_vDamage, None, None);
	return 0;
}

void CVideo::SetBuffer(struct livebox_buffer *pBuffer)
{
	if (!pBuffer) {
		if (m_pDamageHandler)
			ecore_event_handler_del(m_pDamageHandler);

		m_pDamageHandler = NULL;

		if (m_vDamage)
			ecore_x_damage_free(m_vDamage);

		m_vDamage = 0;
		return;
	}

	m_pBuffer = pBuffer;

	m_nId = provider_buffer_pixmap_id(m_pBuffer);
	if (m_nId == 0) {
		ErrPrint("Failed to get pixmap\n");
		return;
	}

	m_pDamageHandler = ecore_event_handler_add(ECORE_X_EVENT_DAMAGE_NOTIFY, CVideo::s_DamageEventHandler, this);
	if (!m_pDamageHandler) {
		ErrPrint("Failed to add a event handler\n");
		return;
	}

	m_vDamage = ecore_x_damage_new(m_nId, ECORE_X_DAMAGE_REPORT_RAW_RECTANGLES);
	if (!m_vDamage) {
		ErrPrint("Failed to create a new damage object\n");
		return;
	}

	DbgPrint("Prepare callback\n");
	Clear();
}

int CVideo::s_BufferEventHandler(struct livebox_buffer *buffer, enum buffer_event event, double timestamp, double x, double y, void *data)
{
	CVideo *video = (CVideo *)data;
	int ix;
	int iy;

	ix = video->Width() * x;
	iy = video->Height() * y;

	switch (event) {
	case BUFFER_EVENT_ENTER:
		break;
	case BUFFER_EVENT_LEAVE:
		break;
	case BUFFER_EVENT_DOWN:
		DbgPrint("Down: %dx%d\n", ix, iy);
		break;
	case BUFFER_EVENT_MOVE:
		break;
	case BUFFER_EVENT_UP:
		DbgPrint("Up: %dx%d\n", ix, iy);
		break;
	default:
		break;
	}

	return 0;
}

int CVideo::Destroy(void)
{
	int ret;
	DbgPrint("Destroy a video object\n");

	s_pList = eina_list_remove(s_pList, this);

	Stop(); /*!< Just try to stop playing */

	if (m_nId)
		ecore_x_damage_free(m_vDamage);

	if (m_pDamageHandler)
		ecore_event_handler_del(m_pDamageHandler);

	free(m_sId);

	ret = provider_buffer_release(m_pBuffer);
	DbgPrint("release: %d\n", ret);
	delete this;
	return 0;
}

int CVideo::Resize(int w, int h)
{
	struct livebox_buffer *pBuffer;

	if (m_nWidth == w && m_nHeight == h) {
		DbgPrint("Size has no changes\n");
		return 0;
	}

	m_nWidth = w;
	m_nHeight = h;

	if (!m_pBuffer) {
		ErrPrint("Buffer is not available\n");
		return -EINVAL;
	}

	SetBuffer(NULL);

	provider_buffer_release(m_pBuffer);
	m_pBuffer = NULL;

	pBuffer = provider_buffer_acquire(m_vType, PKGNAME,
				m_sId, m_nWidth, m_nHeight, sizeof(int),
				CVideo::s_BufferEventHandler, this);

	SetBuffer(pBuffer);

	if (m_vState == PLAYING) {
		const char *uri;
		m_sError = NULL;

		uri = CModel::GetInstance()->VideoFilename();
		DbgPrint("Update PIXMAP: 0x%X (file: %s)\n", m_nId, uri);
		mm_player_set_attribute(m_vPlayer, &m_sError,
				"display_surface_type", MM_DISPLAY_SURFACE_X,
				"display_width", m_nWidth,
				"display_height", m_nHeight,
				"display_overlay", &m_nId, sizeof(m_nId),
				"profile_uri", uri, strlen(uri),
				"display_rotation", MM_DISPLAY_ROTATION_NONE,
				"profile_play_count", 1,
				NULL);
		if (m_sError)
			ErrPrint("SetAttribute: %s\n", m_sError);
	}

	return 0;
}

int CVideo::s_MMMessageHandler(int message, void *_param, void *user_param)
{
	CVideo *video = (CVideo *)user_param;
	// MMMessageParamType *param = (MMMessageParamType *)_param;

	switch (message) {
	case MM_MESSAGE_END_OF_STREAM:
		video->Stop();
		DbgPrint("End of stream\n");
		break;
	default:
		return FALSE;
	}
	
	return TRUE;
}

void CVideo::Clear(void)
{
	static unsigned int s_color = 0x00FF0000;
	GC gc;

	gc = XCreateGC((Display *)ecore_x_display_get(), PixmapId(), 0, 0);

	XSetForeground((Display *)ecore_x_display_get(), gc, 0xFF000000 | s_color);
	XFillRectangle((Display *)ecore_x_display_get(), PixmapId(), gc, 0, 0, Width(), Height());

	XSync((Display *)ecore_x_display_get(), FALSE);
	XFreeGC((Display *)ecore_x_display_get(), gc);

	s_color >>= 2;
	if (s_color == 0)
		s_color = 0xFF000000;
}

int CVideo::Play(const char *uri)
{
	if (m_vState == PLAYING)
		return -EBUSY;

	if (mm_player_create(&m_vPlayer) != MM_ERROR_NONE) {
		ErrPrint("Failed to create a player\n");
		return -EFAULT;
	}

	m_sError = NULL;
	mm_player_set_attribute(m_vPlayer, &m_sError,
				"display_surface_type", MM_DISPLAY_SURFACE_X,
				"display_width", m_nWidth,
				"display_height", m_nHeight,
				"display_overlay", &m_nId, sizeof(m_nId),
				"profile_uri", uri, strlen(uri),
				"display_rotation", MM_DISPLAY_ROTATION_NONE,
				"profile_play_count", 1,
				NULL);
	if (m_sError)
		ErrPrint("SetAttribute: %s\n", m_sError);

	if (mm_player_realize(m_vPlayer) != MM_ERROR_NONE) {
		ErrPrint("Failed to realize a player\n");
		mm_player_destroy(m_vPlayer);
		m_vPlayer = 0;
		return -EFAULT;
	}

	mm_player_set_message_callback(m_vPlayer, CVideo::s_MMMessageHandler, this);
	mm_player_start(m_vPlayer);
	m_vState = PLAYING;

	DbgPrint("Play file %s on %d\n", uri, m_nId);
	return 0;
}

int CVideo::Stop(void)
{
	if (m_vState == STOPPED)
		return -EALREADY;

	m_vState = STOPPED;

	if (m_vPlayer) {
		mm_player_unrealize(m_vPlayer);
		mm_player_destroy(m_vPlayer);
		m_vPlayer = 0;
	}

	DbgPrint("Stop playing\n");
	return 0;
}

CVideo::CVideo(enum target_type type, int width, int height)
: m_vState(STOPPED)
, m_vType(type)
, m_nWidth(width)
, m_nHeight(height)
, m_pBuffer(NULL)
, m_nId(0)
, m_sId(NULL)
, m_sError(NULL)
, m_vPlayer(0)
{
}

CVideo::~CVideo(void)
{
}

CVideo::TVideoState CVideo::State(void)
{
	return m_vState;
}

/* End of a file */
