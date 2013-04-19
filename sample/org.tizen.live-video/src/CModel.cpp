/*
 * Copyright 2013  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the "License");
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
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <dlog.h>

#include "debug.h"
#include "CModel.h"

CModel *CModel::m_pInstance = NULL;

CModel::CModel(void)
{
	m_sFilename = strdup("/opt/usr/media/Videos/Helicopter.mp4");
	if (!m_sFilename)
		throw -ENOMEM;
}

CModel::~CModel(void)
{
	free(m_sFilename);
}

CModel *CModel::GetInstance(void)
{
	if (!m_pInstance) {
		try {
			m_pInstance = new CModel();
		} catch (...) {
			return NULL;
		}
	}

	return m_pInstance;
}

const char *CModel::VideoFilename(void)
{
	return m_sFilename;
}

int CModel::SetVideoFilename(const char *filename)
{
	char *tmp;

	tmp = strdup(filename);
	if (!tmp) {
		ErrPrint("Heap: %s\n", strerror(errno));
		return -ENOMEM;
	}

	free(m_sFilename);
	m_sFilename = tmp;
	return 0;
}

/* End of a file */
