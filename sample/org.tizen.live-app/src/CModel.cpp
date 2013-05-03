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

#include <stdio.h>
#include <errno.h>

#include <dlog.h>
#include <Eina.h>

#include "debug.h"
#include "CModel.h"

struct TItem {
/* public: */
	char *tag;
	void *data;
};

CModel *CModel::m_pInstance = NULL;

CModel::CModel(void)
: m_pList(NULL)
{
}

CModel::~CModel(void)
{
}

CModel *CModel::GetInstance()
{
	if (m_pInstance)
		return m_pInstance;

	try {
		m_pInstance = new CModel();
	} catch (...) {
		return NULL;
	}

	return m_pInstance;
}

void *CModel::GetTaggedObject(const char *tag)
{
	Eina_List *l;
	void *ptr;
	TItem *item;

	EINA_LIST_FOREACH(m_pList, l, ptr) {
		item = (TItem *)ptr;
		if (!strcmp(item->tag, tag))
			return item->data;
	}

	return NULL;
}

int CModel::AddTaggedObject(const char *tag, void *object)
{
	TItem *item;

	try {
		item = new TItem();
	} catch (...) {
		return -ENOMEM;
	}

	item->tag = strdup(tag);
	if (!item->tag) {
		delete item;
		return -ENOMEM;
	}

	item->data = object;

	m_pList = eina_list_append(m_pList, item);
	return 0;
}

void *CModel::DelTaggedObject(const char *tag)
{
	Eina_List *l;
	Eina_List *n;
	void *ptr;
	TItem *item;

	EINA_LIST_FOREACH_SAFE(m_pList, l, n, ptr) {
		item = (TItem *)ptr;
		if (!strcmp(item->tag, tag)) {
			void *ret;
			m_pList = eina_list_remove(m_pList, ptr);

			ret = item->data;
			free(item->tag);
			free(item);

			return ret;
		}
	}

	return NULL;
}

/* End of a file */
